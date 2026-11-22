param([ValidateRange(1,50)][int]$Count=10,[string]$Release='RuleTactics_20261122',[string]$LogPrefix='RuleRunStress',[ValidateSet('Easy','Normal','Hard')][string]$Difficulty='Normal',[switch]$Editor,[string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$executable=Join-Path $projectRoot "Releases\$Release\Windows\KowloonFgame\Binaries\Win64\KowloonFgame.exe"
if($Editor){$executable=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'}
$results=@()
for($batch=1;$batch -le $Count;$batch+=2){
 $running=@()
 for($index=$batch;$index -le [Math]::Min($batch+1,$Count);$index++){
  $log=Join-Path $projectRoot ('Saved\Logs\{0}{1:D2}.log' -f $LogPrefix,$index)
  $started=Get-Date
  $difficultyIndex=@{Easy=0;Normal=1;Hard=2}[$Difficulty]
  $arguments=@('/Game/KTW/Maps/L_RuleArena','-KTWRogueSoak',"-KTWRogueDifficulty=$difficultyIndex",'-benchmark','-fps=30','-nullrhi','-unattended','-nosound',"-abslog=$log")
  if($Editor){$arguments=@(('"'+(Join-Path $projectRoot 'KowloonFgame.uproject')+'"'),'-game')+$arguments}
  $process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
  $running+=@{Process=$process;Log=$log;Index=$index;Started=$started}
 }
 foreach($entry in $running){
  if(!$entry.Process.WaitForExit(240000)){
   foreach($pending in $running){if(!$pending.Process.HasExited){Stop-Process -Id $pending.Process.Id -Force}}
   throw "Run $($entry.Index) timed out: $($entry.Log)"
  }
  if(!(Test-Path -LiteralPath $entry.Log) -or (Get-Item -LiteralPath $entry.Log).LastWriteTime -lt $entry.Started){throw "No fresh log for run $($entry.Index)"}
  $lines=Get-Content -LiteralPath $entry.Log
  $completion=$lines | Select-String 'KTW ROGUE SOAK COMPLETE failures=0 wave=(\d+) won=(\d+) lives=(\d+)' | Select-Object -Last 1
  if($entry.Process.ExitCode -ne 0 -or !$completion -or $lines -match 'Fatal error:|Invalid Simulate Options:|COMPLETE failures=[1-9]|Accessed None|contains NaN|is NaN'){throw "Run $($entry.Index) failed: $($entry.Log)"}
  $match=$completion.Matches[0]
  $pose=$lines | Select-String 'tilt=([\d.]+) walks=(\d+) attacks=(\d+) gunshots=(\d+)' | Select-Object -Last 1
  if(!$pose -or [double]$pose.Matches[0].Groups[1].Value -ge 2){throw "Missing upright movement evidence: $($entry.Log)"}
  $results+=[pscustomobject]@{Run=$entry.Index;Difficulty=$Difficulty;Wave=[int]$match.Groups[1].Value;Won=[bool][int]$match.Groups[2].Value;Lives=[int]$match.Groups[3].Value;MaxTiltDegrees=[double]$pose.Matches[0].Groups[1].Value;Walks=[int]$pose.Matches[0].Groups[2].Value;Attacks=[int]$pose.Matches[0].Groups[3].Value;Gunshots=[int]$pose.Matches[0].Groups[4].Value;Log=$entry.Log}
  Write-Output "PASS run $($entry.Index) wave=$($match.Groups[1].Value) won=$($match.Groups[2].Value) lives=$($match.Groups[3].Value)"
 }
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $projectRoot "Saved\Logs\${LogPrefix}Summary.json") -Encoding utf8
Write-Output "PASS all $Count physical runs"
