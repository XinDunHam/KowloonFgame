param([switch]$Packaged,[ValidateSet('Rules','Soak','Opening','Diamond','Engagement','OrdersLoot')][string]$Test='Rules',[string]$Release='StreetWar_20261120',[string]$EngineRoot='D:\UE\UE_5.8',[string]$LogPrefix='StreetWarFinal')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$log=Join-Path $projectRoot "Saved\Logs\${LogPrefix}${Test}.log"
$arguments=@('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-unattended','-nosound',"-abslog=$log")
if($Packaged){$executable=Join-Path $projectRoot "Releases\$Release\Windows\KowloonFgame\Binaries\Win64\KowloonFgame.exe"}
else{$executable=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$arguments=@(('"'+(Join-Path $projectRoot 'KowloonFgame.uproject')+'"'),'-game')+$arguments}
switch($Test){
 'OrdersLoot' {$arguments+=@('-KTWOrdersLootTest','-windowed','-ResX=1600','-ResY=900');$expected='KTW ORDERS LOOT COMPLETE failures=0'}
 'Engagement' {$arguments+=@('-KTWEngagementTest','-windowed','-ResX=1600','-ResY=900');$expected='KTW ENGAGEMENT COMPLETE failures=0'}
 'Rules' {$arguments+=@('-KTWStreetWarTest','-nullrhi');$expected='KTW STREET WAR COMPLETE failures=0'}
 'Soak' {$arguments+=@('-KTWStreetWarSoak','-benchmark','-fps=30','-nullrhi');$expected='KTW STREET SOAK COMPLETE failures=0'}
 'Opening' {$arguments+=@('-KTWStreetWarCapture','-KTWCapture','-windowed','-ResX=1600','-ResY=900');$expected='Tracing Screenshot'}
 'Diamond' {$arguments+=@('-KTWStreetWarCapture','-KTWStreetWarLate','-KTWCapture','-windowed','-ResX=1600','-ResY=900');$expected='Tracing Screenshot'}
}
$started=Get-Date
$testProcess=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(!$testProcess.WaitForExit(240000)){throw "Test exceeded 240 seconds; inspect process $($testProcess.Id) and $log"}
if(!(Test-Path -LiteralPath $log) -or (Get-Item -LiteralPath $log).LastWriteTime -lt $started){throw 'No fresh test log'}
$lines=Get-Content -LiteralPath $log
if($lines -match 'KTW STREET .*: FAIL$|COMPLETE failures=[1-9]|Fatal error:'){throw "Test failed. Inspect $log"}
if($Test -in @('Opening','Diamond')){
 if($Packaged){$picture=Join-Path $projectRoot "Releases\$Release\Windows\KowloonFgame\Saved\Screenshots\KTW_V4.png"}
 else{$picture=Join-Path $projectRoot 'Saved\Screenshots\KTW_V4.png'}
 if(!(Test-Path -LiteralPath $picture) -or (Get-Item -LiteralPath $picture).LastWriteTime -lt $started){throw "No fresh screenshot: $picture"}
}elseif(!($lines -match [regex]::Escape($expected))){throw "Missing completion: $expected. Inspect $log"}
Write-Output "PASS $Test $log"

