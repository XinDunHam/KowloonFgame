#include "KTWArtPerf.h"
#include "KTWPrototype.h"
#include "KTWSession.h"
#include "KTWMatch.h"
#include "KTWGold.h"
#include "KTWCommander.h"
#include "KTWImportedArt.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
namespace {bool Ready=false,Done=false;float StartAt=0;double LastAt=0;int View=-1;TArray<float> Times[3];FString Raw=TEXT("scene,elapsed,frame_ms\n");}
bool FKTWArtPerf::Active(){return FParse::Param(FCommandLine::Get(),TEXT("KTWArtPerfTest"));}
void FKTWArtPerf::Start(AKTWCameraPawn* Pawn){
 FTimerHandle T;Pawn->GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateWeakLambda(Pawn,[Pawn](){
  if(auto* S=UKTWSession::Get(Pawn)){S->InMenu=false;S->Action(TEXT("Quality1"));}
  if(GEngine)GEngine->bSmoothFrameRate=false;
  if(auto* PC=Cast<APlayerController>(Pawn->GetController())){PC->ConsoleCommand(TEXT("t.MaxFPS 0"),false);PC->ConsoleCommand(TEXT("r.VSync 0"),false);PC->ConsoleCommand(TEXT("r.ScreenPercentage 100"),false);}
  for(TActorIterator<AKTWCommander>I(Pawn->GetWorld());I;++I)I->SetActorTickEnabled(false);for(TActorIterator<AKTWTestUnit>I(Pawn->GetWorld());I;++I)I->Destroy();
  if(auto* M=AKTWMatch::Find(Pawn->GetWorld())){M->StartBattle();M->Tick(20);M->SetActorTickEnabled(false);}if(auto* G=AKTWGoldSystem::Find(Pawn->GetWorld())){G->Tick(0);G->SetActorTickEnabled(false);}
  for(int Team=0;Team<2;++Team)for(int I=0;I<20;++I){const float Along=(Team?1:-1)*(1000+(I%5)*220),Across=(I/5)*70;const FVector P(Along+Across,-Along+Across,100);auto* U=Pawn->GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->Faction=Team;U->UnitRole=EKTWRole(I%4);U->bAutoPatrol=true;U->FinishSpawning(FTransform(P));U->Level=3;U->Experience=160;U->Stats.MaxHealth=100000;U->Health=100000;}
  for(TActorIterator<AKTWBoss>I(Pawn->GetWorld());I;++I){I->MaxHealth=100000;I->Health=100000;}
  StartAt=Pawn->GetWorld()->GetTimeSeconds();LastAt=FPlatformTime::Seconds();Ready=true;UE_LOG(LogTemp,Display,TEXT("KTW PERF START imported=%d forty units plus bosses medium quality"),FKTWImportedArt::Enabled());
 }),4.f,false);
}
void FKTWArtPerf::Tick(AKTWCameraPawn* P){if(!Ready||Done)return;const double Now=FPlatformTime::Seconds();const float MS=(Now-LastAt)*1000;LastAt=Now;const float E=P->GetWorld()->GetTimeSeconds()-StartAt;const int Scene=E<27?0:E<50?1:2;
 if(View!=Scene){View=Scene;const FVector Eye=Scene==0?FVector(0,0,26000):Scene==1?FVector(-3200,3200,1500):FVector(-14500,-14500,11000);P->bReady=true;P->bStreetView=Scene!=0;P->FreePosition=Eye;P->FocusGoal=FVector(Eye.X,Eye.Y,0);P->TopHeight=26000;P->SetActorLocation(Eye);P->Camera->SetRelativeRotation(Scene==0?FRotator(-90,-90,0):Scene==1?FRotator(-8,-45,0):FRotator(-29,45,0));P->Camera->FieldOfView=65;}
 if((E>=12&&E<27)||(E>=35&&E<50)||(E>=55&&E<70)){Times[Scene].Add(MS);Raw+=FString::Printf(TEXT("%d,%.4f,%.4f\n"),Scene,E,MS);}
 if(E<70)return;Done=true;const FString Name=FParse::Param(FCommandLine::Get(),TEXT("KTWLegacyStreetArt"))?TEXT("Legacy"):TEXT("Imported");FString Report=TEXT("scene,frames,avg_ms,p95_ms,p99_ms,max_ms,fps,over50ms\n");
 int Units=0;for(TActorIterator<AKTWTestUnit>I(P->GetWorld());I;++I)if(I->Health>0)++Units;
 for(int I=0;I<3;++I){auto A=Times[I];A.Sort();if(A.Num()<10){FPlatformMisc::RequestExitWithStatus(false,1);return;}double Sum=0;int Hitches=0;for(float X:A){Sum+=X;Hitches+=X>50;}const double Mean=Sum/A.Num();Report+=FString::Printf(TEXT("%d,%d,%.4f,%.4f,%.4f,%.4f,%.2f,%d\n"),I,A.Num(),Mean,A[FMath::Min(A.Num()-1,FMath::FloorToInt(A.Num()*.95))],A[FMath::Min(A.Num()-1,FMath::FloorToInt(A.Num()*.99))],A.Last(),1000/Mean,Hitches);}
 const FString Dir=FPaths::ProjectSavedDir()/TEXT("Performance");IFileManager::Get().MakeDirectory(*Dir,true);FFileHelper::SaveStringToFile(Report,*(Dir/(TEXT("KTW_")+Name+TEXT("_summary.csv"))));FFileHelper::SaveStringToFile(Raw,*(Dir/(TEXT("KTW_")+Name+TEXT("_frames.csv"))));UE_LOG(LogTemp,Display,TEXT("KTW PERF COMPLETE mode=%s units=%d"),*Name,Units);FPlatformMisc::RequestExitWithStatus(false,Units==40?0:1);
}
