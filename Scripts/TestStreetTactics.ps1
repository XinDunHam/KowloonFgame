param([string]$Release='GameplayPlus_20261123')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root "Releases/$Release/Windows/KowloonFgame.exe"
$log=Join-Path $root 'Saved/Logs/GameplayPlusTactics.log'
$p=Start-Process -FilePath $exe -ArgumentList @('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-KTWStreetWarCapture','-KTWStreetTacticsTest','-nullrhi','-unattended','-nosound',"-abslog=$log") -WindowStyle Hidden -PassThru
if(!$p.WaitForExit(120000)){Stop-Process -Id $p.Id -Force;throw 'Tactics test timed out'}
$lines=Get-Content -LiteralPath $log
if($p.ExitCode -ne 0 -or $lines -match 'Fatal error:|COMPLETE failures=[1-9]|: FAIL$' -or !($lines -match 'KTW STREET TACTICS COMPLETE failures=0')){throw "Tactics test failed: $log"}
Write-Output "PASS Street tactics $log"
