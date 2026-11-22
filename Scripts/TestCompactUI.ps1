param([string]$Release='CompactUI_20261123',[int]$Width=1920,[int]$Height=1080,[switch]$SkipControls)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!$SkipControls){& "$PSScriptRoot/TestReferenceUI.ps1" -Release $Release -Width $Width -Height $Height -Cases @('Controls')}
$exe=Join-Path $root "Releases\$Release\Windows\KowloonFgame.exe"
$log=Join-Path $root "Saved\Logs\CompactUI${Width}Capture.log"
$started=Get-Date
$process=Start-Process -FilePath $exe -ArgumentList @('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-KTWStreetWarCapture','-KTWCompactCapture','-KTWReferenceAudit','-unattended','-nosound','-windowed',"-ResX=$Width","-ResY=$Height","-abslog=$log") -WindowStyle Hidden -PassThru
if(!$process.WaitForExit(180000)){Stop-Process -Id $process.Id -Force;throw 'Compact UI capture timeout'}
$lines=Get-Content -LiteralPath $log
if($process.ExitCode -ne 0 -or $lines -match 'Fatal error:|GPU Crashed|Assertion failed:|COMPLETE failures=[1-9]' -or !($lines -match 'KTW COMPACT CAPTURE COMPLETE')){throw "Failed capture $log"}
$archive=Join-Path $root "SourceAssets\$Release\${Width}x${Height}"
New-Item -ItemType Directory -Path $archive -Force > $null
foreach($name in @('Compact_Z.png','Compact_X.png','Compact_C.png','Compact_3D.png','Compact_Map.png')){
 $file=Join-Path $root "Releases\$Release\Windows\KowloonFgame\Saved\Screenshots\$name"
 if(!(Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).LastWriteTime -lt $started){throw "No fresh screenshot $name"}
 Copy-Item -LiteralPath $file -Destination (Join-Path $archive $name) -Force
}
Write-Output "PASS compact UI captures $Width x $Height"
