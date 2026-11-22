param([string]$Release='BilingualUI_20261122')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $root "Releases\$Release\Windows\KowloonFgame\Binaries\Win64\KowloonFgame.exe"
$archive=Join-Path $root "SourceAssets\$Release"
New-Item -ItemType Directory -Path $archive -Force > $null
foreach($case in @('Profile','Chinese','English','Collection')){
 $log=Join-Path $root "Saved\Logs\BilingualUI${case}.log"
 $args=@('/Game/KTW/Maps/L_Kowloon_ReferenceV4','-unattended','-nosound',"-abslog=$log")
 if($case -eq 'Profile'){$args+=@('-KTWUIProfileTest','-nullrhi')}
 else{$args+=@('-KTWModeCapture','-windowed','-ResX=1600','-ResY=900');if($case -eq 'English'){$args+='-KTWEnglish'};if($case -eq 'Collection'){$args+='-KTWCollectionCapture'}}
 $began=Get-Date
 $process=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 if(!$process.WaitForExit(180000)){throw "Timeout: $case"}
 $lines=Get-Content -LiteralPath $log
 if($process.ExitCode -ne 0 -or $lines -match 'Error:|Fatal error:|Accessed None'){throw "Failed $case ($log)"}
 if($case -eq 'Profile'){if(!($lines -match 'KTW UI PROFILE COMPLETE failures=0')){throw 'Missing profile completion'}}
 else{$picture=Join-Path $root "Releases\$Release\Windows\KowloonFgame\Saved\Screenshots\ModeSelect.png";if((Get-Item -LiteralPath $picture).LastWriteTime -lt $began){throw 'No fresh capture'};Copy-Item -LiteralPath $picture -Destination (Join-Path $archive "$case.png") -Force}
 Write-Output "PASS $case"
}

