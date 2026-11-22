param([string]$EngineRoot='D:\UE\UE_5.8',[ValidateSet('Crossing','Combat','Economy','Restart','Gold','GoldAuto','Phase3','Balance','Command','Commander','AIPlay','Progression','Frontend','Presentation','Revision','Season','Immersion','Continuity')][string]$Test='Crossing')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$projectFile=Join-Path $projectRoot 'KowloonFgame.uproject'
[string[]]$flags=switch($Test){
 'Continuity' { '-KTWContinuityTest' }
 'Immersion' { '-KTWImmersionTest' }
 'Season' { '-KTWSeasonTest' }
 'Revision' { '-KTWRevisionTest' }
 'Frontend' { '-KTWFrontendTest' }
 'Presentation' { '-KTWPresentationTest' }
 'Crossing' { '-KTWAutomation'; '-KTWCrossingTest' }
 'Phase3' { '-KTWPhase3Test' }
 'Balance' { '-KTWBalanceTest' }
 'Economy' { '-KTWEconomyTest' }
 'Restart' { '-KTWRestartTest' }
 'Gold' { '-KTWGoldTest' }
 'GoldAuto' { '-KTWGoldAutoTest' }
 'Progression' { '-KTWProgressionTest' }
 'Combat' { '-KTWCombatTest' }
 'Command' { '-KTWCommandTest' }
 'Commander' { '-KTWCommanderTest' }
 'AIPlay' { '-KTWAIPlayTest'; '-benchmark'; '-fps=30' }
}
& (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $projectFile /Game/KTW/Maps/L_Kowloon_ReferenceV4 -game -nullrhi -unattended -nosound @flags ("-abslog="+(Join-Path $projectRoot "Saved\Logs\KTW_V4_$Test.log"))
if($LASTEXITCODE -ne 0){throw "V4 $Test failed with $LASTEXITCODE"}

$logPath=Join-Path $projectRoot "Saved\Logs\KTW_V4_$Test.log"
if(!(Test-Path -LiteralPath $logPath)){throw "Missing test log"}
$testLines=Get-Content -LiteralPath $logPath | Where-Object {$_ -match 'KTW '}
if($testLines -match ': FAIL| failures=[1-9]|Fatal error'){throw "Test assertions failed. Inspect $logPath"}

$expected=switch($Test){'Continuity' {'KTW CONTINUITY COMPLETE'} 'Immersion' {'KTW IMMERSION COMPLETE'} 'Season' {'KTW SEASON COMPLETE'} 'Revision' {'KTW REVISION COMPLETE'} 'Frontend' {'KTW FRONTEND COMPLETE'} 'Presentation' {'KTW PRESENTATION COMPLETE'} 'Progression' {'KTW PROGRESSION COMPLETE'} 'Command' {'KTW COMMAND COMPLETE'} 'Commander' {'KTW COMMANDER COMPLETE'} 'AIPlay' {'KTW AI PLAY COMPLETE'} 'Phase3' {'KTW PHASE3 COMPLETE'} 'Balance' {'KTW BALANCE COMPLETE'} 'Gold' {'KTW GOLD COMPLETE'} 'Economy' {'KTW ECON COMPLETE'} 'Restart' {'KTW RESTART COMPLETE'}
 'Combat' {'KTW COMBAT TEST autonomous defeat: PASS'} 'GoldAuto' {'KTW GOLD AUTO COMPLETE'} 'Crossing' {'KTW V4 CROSSING TESTS COMPLETE'}}
if($expected -and !(Select-String -LiteralPath $logPath -SimpleMatch $expected)){throw "Missing expected test completion: $expected"}
