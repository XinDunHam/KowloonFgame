param([string]$EngineRoot='D:\UE\UE_5.8',[string]$Release='V0.15')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$projectFile=Join-Path $projectRoot 'KowloonFgame.uproject'
$destination=Join-Path $projectRoot ("Releases\"+$Release)
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun ("-project="+$projectFile) -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iostore -archive ("-archivedirectory="+$destination) -map=/Game/KTW/Maps/L_Kowloon_ReferenceV4+/Game/KTW/Maps/L_RuleArena -unattended -utf8output
if($LASTEXITCODE -ne 0){throw "Packaging failed ($LASTEXITCODE). Check Saved/Logs and AutomationTool logs."}
$exe=Join-Path $destination 'Windows\KowloonFgame.exe'
if(!(Test-Path -LiteralPath $exe)){throw "Missing packaged launcher: $exe"}
Write-Output "KTW PACKAGE COMPLETE $exe"
