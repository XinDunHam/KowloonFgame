param([switch]$Packaged,[ValidateSet('Rules','Soak','Modes','Capture','Menu','Deployment')][string]$Test='Rules',[string]$Release='RuleTactics_20261122',[string]$EngineRoot='D:\UE\UE_5.8',[string]$LogPrefix='RuleRunFinal')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$log=Join-Path $projectRoot "Saved\Logs\${LogPrefix}${Test}.log"
$map=if($Test -in @('Modes','Menu')){'/Game/KTW/Maps/L_Kowloon_ReferenceV4'}else{'/Game/KTW/Maps/L_RuleArena'}
$arguments=@($map,'-unattended','-nosound',"-abslog=$log")
switch($Test){
 'Deployment' {$arguments+=@('-KTWRogueDeployTest','-windowed','-ResX=1600','-ResY=900');$expected='KTW DEPLOY COMPLETE failures=0'}
 'Rules' {$arguments+=@('-KTWRogueTest','-nullrhi');$expected='KTW ROGUE COMPLETE failures=0'}
 'Soak' {$arguments+=@('-KTWRogueSoak','-benchmark','-fps=30','-nullrhi');$expected='KTW ROGUE SOAK COMPLETE failures=0'}
 'Modes' {$arguments+=@('-KTWModeTest','-windowed','-ResX=1600','-ResY=900');$expected='KTW MODE COMPLETE failures=0'}
 'Capture' {$arguments+=@('-KTWRogueCapture','-windowed','-ResX=1600','-ResY=900');$expected='RuleRun_Shop.png'}
 'Menu' {$arguments+=@('-KTWModeCapture','-windowed','-ResX=1600','-ResY=900');$expected='ModeSelect.png'}
}
if($Packaged){$executable=Join-Path $projectRoot "Releases\$Release\Windows\KowloonFgame\Binaries\Win64\KowloonFgame.exe"}
else{$executable=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$arguments=@(('"'+(Join-Path $projectRoot 'KowloonFgame.uproject')+'"'),'-game')+$arguments}
$started=Get-Date
$testProcess=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(!$testProcess.WaitForExit(240000)){throw "Test exceeded 240 seconds: process $($testProcess.Id), log $log"}
if(!(Test-Path -LiteralPath $log) -or (Get-Item -LiteralPath $log).LastWriteTime -lt $started){throw 'No fresh test log'}
$lines=Get-Content -LiteralPath $log
if($testProcess.ExitCode -ne 0 -or $lines -match 'KTW .*: FAIL$|COMPLETE failures=[1-9]|Fatal error:|Invalid Simulate Options:'){throw "Rule run failed: $log"}
if($Test -in @('Capture','Menu')){
 $saved=if($Packaged){Join-Path $projectRoot "Releases\$Release\Windows\KowloonFgame\Saved"}else{Join-Path $projectRoot 'Saved'}
 $picture=Join-Path $saved "Screenshots\$expected"
 if(!(Test-Path -LiteralPath $picture) -or (Get-Item -LiteralPath $picture).LastWriteTime -lt $started){throw "No fresh screenshot: $picture"}
}elseif(!($lines -match [regex]::Escape($expected))){throw "Missing completion: $expected"}
Write-Output "PASS $Test $log"
