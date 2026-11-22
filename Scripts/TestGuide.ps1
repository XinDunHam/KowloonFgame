param([string]$Release='Release_20261123')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root "Releases/$Release/Windows/KowloonFgame.exe"
$log=Join-Path $root 'Saved/Logs/ReleaseGuideCapture.log'
$began=Get-Date
$p=Start-Process -FilePath $exe -ArgumentList @('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-KTWStreetWarCapture','-KTWGuideCapture','-unattended','-nosound','-windowed','-ResX=1600','-ResY=900',"-abslog=$log") -WindowStyle Hidden -PassThru
if(!$p.WaitForExit(120000)){Stop-Process -Id $p.Id -Force;throw 'Guide capture timeout'}
$lines=Get-Content -LiteralPath $log
if($p.ExitCode -ne 0 -or $lines -match 'Fatal error:|Assertion failed:' -or !($lines -match 'KTW GUIDE CAPTURE COMPLETE')){throw 'Guide capture failed'}
$archive=Join-Path $root 'SourceAssets/Release_20261123/Guide';New-Item -ItemType Directory -Path $archive -Force > $null
for($i=0;$i -lt 12;$i++){$name='Guide_{0:D2}.png' -f $i;$file=Join-Path $root "Releases/$Release/Windows/KowloonFgame/Saved/Screenshots/$name";if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $began){throw "Missing guide capture $name"};Copy-Item -LiteralPath $file -Destination (Join-Path $archive $name) -Force}
Write-Output 'PASS 12 bilingual guide pages captured'
