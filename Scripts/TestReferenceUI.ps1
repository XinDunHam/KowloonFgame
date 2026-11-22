param([string]$Release='ReferenceHD_20261123',[int]$Width=1920,[int]$Height=1080,[string[]]$Cases=@('RogueChinese','RogueEnglish','Street','StreetResult','Controls'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root "Releases\$Release\Windows\KowloonFgame.exe"
$shots=Join-Path $root "Releases\$Release\Windows\KowloonFgame\Saved\Screenshots"
New-Item -ItemType Directory -Path (Join-Path $root "Releases\$Release\Windows\KowloonFgame\Saved\Logs") -Force > $null
$archive=Join-Path $root "SourceAssets\$Release\${Width}x${Height}"
New-Item -ItemType Directory -Path $archive -Force > $null
foreach($case in $Cases){
 $log=Join-Path $root "Saved\Logs\Reference${Width}${case}.log"
 $map=if($case.StartsWith('Rogue')){'/Game/KTW/Maps/L_RuleArena'}else{'/Game/KTW/Maps/L_Kowloon_ReferenceV4'}
 $arguments=@($map,'-unattended','-nosound','-windowed',"-ResX=$Width","-ResY=$Height",'-KTWReferenceAudit',"-abslog=$log")
 if($case.StartsWith('Rogue')){$arguments+='-KTWRogueCapture';if($case -eq 'RogueEnglish'){$arguments+='-KTWEnglish'}}
 elseif($case -eq 'Controls'){$arguments+=@('-KTWReferenceControlsTest','-KTWStreetWarCapture')}
 else{$arguments+=@('-KTWStreetWarCapture','-KTWCapture');if($case -eq 'StreetResult'){$arguments+='-KTWReferenceResultCapture'}}
 $began=Get-Date
 $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
 if(!$process.WaitForExit(180000)){Stop-Process -Id $process.Id -Force;throw "Timeout $case"}
 $lines=Get-Content -LiteralPath $log
 if($process.ExitCode -ne 0 -or $lines -match 'Fatal error:|Assertion failed:|COMPLETE failures=[1-9]|Failed to load font|Failed to load.*UIFont'){throw "Failed $case ($log)"}
 if(!($lines -match 'KTW REFERENCE AUDIT COMPLETE failures=0 components=184 states=176 frames=264 backgrounds=6 native=1')){throw "Missing full resource/native-resolution audit $case"}
 if($case -eq 'Controls'){if(!($lines -match 'KTW REFERENCE CONTROLS COMPLETE failures=0')){throw 'Missing controls completion'}}
 else{
  $names=if($case.StartsWith('Rogue')){@('RuleRun_Deployment.png','RuleRun_Battle.png','RuleRun_Result.png','RuleRun_Draft.png','RuleRun_Shop.png')}else{@('KTW_V4.png')}
  foreach($name in $names){$picture=Join-Path $shots $name;if(!(Test-Path -LiteralPath $picture) -or (Get-Item -LiteralPath $picture).LastWriteTime -lt $began){throw "No fresh screenshot $name"};Copy-Item -LiteralPath $picture -Destination (Join-Path $archive "${case}_${name}") -Force}
 }
 Write-Output "PASS $Width x $Height $case"
}


