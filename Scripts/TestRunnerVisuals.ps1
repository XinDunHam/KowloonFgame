param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$runnerRoot=Split-Path $PSScriptRoot -Parent
$runnerLog=Join-Path $runnerRoot 'Saved\Logs\KTW_RunnerVisualTest.log'
$runnerStarted=Get-Date
$runnerArgs=@(('"'+(Join-Path $runnerRoot 'KowloonFgame.uproject')+'"'),'/Game/KTW/Maps/L_Kowloon_ReferenceV4','-game','-nullrhi','-unattended','-nosound','-KTWRunnerVisualTest',('-abslog="'+$runnerLog+'"'))
$runnerProcess=Start-Process -FilePath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') -ArgumentList $runnerArgs -WindowStyle Hidden -PassThru -Wait
if($runnerProcess.ExitCode -ne 0){throw "Runner integration test exited $($runnerProcess.ExitCode)"}
if(!(Test-Path -LiteralPath $runnerLog) -or (Get-Item -LiteralPath $runnerLog).LastWriteTime -lt $runnerStarted){throw 'Missing fresh Runner test log'}
$runnerText=Get-Content -LiteralPath $runnerLog -Raw
if($runnerText -notmatch 'KTW RUNNER COMPLETE failures=0' -or $runnerText -match 'KTW RUNNER .*: FAIL'){throw 'Runner integration checks failed'}
Write-Output 'Runner runtime integration PASS (headless; not visual-quality or performance acceptance).'
