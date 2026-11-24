param([string]$Release='HomeUI_20261124',[int]$Width=1920,[int]$Height=1080)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root "Releases/$Release/Windows/KowloonFgame/Binaries/Win64/KowloonFgame.exe"
$log=Join-Path $root "Saved/Logs/HomeUI${Width}.log"
$start=Get-Date
$p=Start-Process -FilePath $exe -ArgumentList @('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-KTWHomeTest','-windowed',"-ResX=$Width","-ResY=$Height",'-unattended','-nosound',"-abslog=$log") -WindowStyle Hidden -PassThru
if(!$p.WaitForExit(150000)){Stop-Process -Id $p.Id -Force;throw 'Home UI test timeout'}
$lines=Get-Content -LiteralPath $log
if($p.ExitCode -ne 0 -or $lines -match ': FAIL$|Fatal error:|Assertion failed:' -or !($lines -match 'KTW HOME COMPLETE failures=0')){throw "Home UI test failed: $log"}
$archive=Join-Path $root "SourceAssets/$Release/${Width}x${Height}";New-Item -ItemType Directory -Path $archive -Force > $null
foreach($name in @('Home_English.png','Home_Chinese.png','Home_Difficulty.png')){$file=Join-Path $root "Releases/$Release/Windows/KowloonFgame/Saved/Screenshots/$name";if(!(Test-Path $file) -or (Get-Item $file).LastWriteTime -lt $start){throw "No fresh capture $name"};Copy-Item -LiteralPath $file -Destination (Join-Path $archive $name) -Force}
Write-Output "PASS Home UI $Width x $Height"
