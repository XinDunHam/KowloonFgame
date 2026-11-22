param([string]$EngineRoot='D:\UE\UE_5.8')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$projectFile=Join-Path $projectRoot 'KowloonFgame.uproject'
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') Development Win64 "-Project=$projectFile" -TargetType=Editor -NoHotReloadFromIDE
if($LASTEXITCODE -ne 0){throw "Compilation failed: $LASTEXITCODE"}
& (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $projectFile -run=KTWBuildPrototype -unattended -nop4 -nullrhi -nosound ("-abslog=" + (Join-Path $projectRoot "Saved\Logs\KTW_V4_AssetBuild.log"))
if($LASTEXITCODE -ne 0){throw "Asset generation failed: $LASTEXITCODE"}

