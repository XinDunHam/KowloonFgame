#include "KTWStreetWar.h"
#include "KTWPrototype.h"
#include "KTWMatch.h"
#include "KTWCommander.h"
#include "KTWCombatAction.h"
#include "KTWGeneration.h"
#include "KTWAI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "TimerManager.h"
#include "Misc/Paths.h"

void AKTWStreetWar::RunEngagementTests(){
#if !UE_BUILD_SHIPPING
 auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const TCHAR* Name){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW ENGAGEMENT %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto* M=AKTWMatch::Find(GetWorld());auto* PC=Cast<AKTWPrototypeController>(GetWorld()->GetFirstPlayerController());auto* Cam=PC?Cast<AKTWCameraPawn>(PC->GetPawn()):nullptr;
 Check(M&&PC&&Cam&&Initialized,TEXT("scenario initialized"));if(!M||!PC||!Cam||!Initialized){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)I->SetActorTickEnabled(false);
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){I->SetActorLocation(FVector(14000,14000,100));if(auto* AI=Cast<AAIController>(I->GetController()))AI->UnPossess();I->SetActorTickEnabled(false);}
 M->StartBattle();M->BattleSeconds=10;Late=false;M->Winner=-2;
 Check(Bases[0]->MaxHealth==1200&&Bases[1]->MaxHealth==1200,TEXT("both vans have reduced 1200 durability"));
 SpawnConvoy(0);auto* Truck=Objects.FindByPredicate([](const TObjectPtr<AKTWFieldObject>& O){return O&&O->Kind==EKTWFieldKind::Truck;})->Get();
 const FVector BaseSize=Bases[0]->Body->Bounds.BoxExtent*2,TruckSize=Truck->Body->Bounds.BoxExtent*2;
 Check(TruckSize.X>BaseSize.X*.85f&&TruckSize.X<BaseSize.X*.95f,TEXT("convoy length is about ninety percent of base van"));
 for(auto G:Truck->Guards)G->Destroy();Truck->SetActorLocation(FVector(-6000,0,2));Truck->Open=true;
 auto Spawn=[this](int Team,EKTWRole UnitRole,FVector P){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->Faction=Team;U->UnitRole=UnitRole;U->bAutoPatrol=false;U->bEnableCombat=false;U->FinishSpawning(FTransform(P));U->IssueOrder(EKTWUnitOrder::Defend,P);return U;};
 TArray<AKTWTestUnit*> Units;TArray<FVector> Starts;for(int I=0;I<4;++I){const FVector P(-6000,5000-I*900,100);Units.Add(Spawn(0,EKTWRole(I),P));Starts.Add(P);}
 auto* Enemy=Spawn(1,EKTWRole::Runner,FVector(-6000,1100,100));
 auto* Boss=GetWorld()->SpawnActorDeferred<AKTWBoss>(AKTWBoss::StaticClass(),FTransform(FVector(-6000,5900,100)));Boss->Faction=0;Boss->FinishSpawning(FTransform(FVector(-6000,5900,100)));Boss->SetActorTickEnabled(false);if(auto* AI=Cast<AAIController>(Boss->GetController()))AI->UnPossess();
 Cam->PanTo(FVector(-6000,3500,0));Cam->TopHeight=9000;
 FTimerHandle Select;GetWorldTimerManager().SetTimer(Select,FTimerDelegate::CreateWeakLambda(this,[this,M,PC,Cam,Units,Starts,Enemy,Boss,Truck,Check,Fail,Spawn](){
  FBox2D GroupBox(ForceInit);bool AllRoles=true;
  for(auto* U:Units){FVector2D Foot,Head;PC->ProjectWorldLocationToScreen(U->GetActorLocation(),Foot);PC->ProjectWorldLocationToScreen(U->GetMesh()->GetSocketLocation(TEXT("head")),Head);const FVector2D Click=(Foot+Head)*.5;AllRoles&=PC->SelectAtScreen(Click,Click,false,true)&&PC->SelectedUnit==U;GroupBox+=Foot;GroupBox+=Head;}
  Check(AllRoles,TEXT("right click can select every friendly soldier role"));
  FVector2D BF,BH;PC->ProjectWorldLocationToScreen(Boss->GetActorLocation(),BF);PC->ProjectWorldLocationToScreen(Boss->GetMesh()->GetSocketLocation(TEXT("head")),BH);Check(PC->SelectAtScreen((BF+BH)*.5,(BF+BH)*.5,false,true)&&PC->SelectedBoss==Boss,TEXT("right click can select friendly boss"));
  FVector2D EP;PC->ProjectWorldLocationToScreen(Enemy->GetActorLocation(),EP);const bool Consumed=PC->SelectAtScreen(EP,EP,false,true);Check(!Consumed&&PC->SelectedBoss==Boss,TEXT("enemy click keeps friendly selection for attack order"));
  Check(PC->SelectAtScreen(GroupBox.Min-FVector2D(15),GroupBox.Max+FVector2D(15),false,true)&&PC->SelectedUnits.Num()==4&&!PC->SelectedUnits.Contains(Enemy),TEXT("right drag selects four soldiers without enemy units"));
  Check(PC->CommandAtWorld(FVector(-6000,1400,20)),TEXT("selected group accepts move order"));
  FTimerHandle Move;GetWorldTimerManager().SetTimer(Move,FTimerDelegate::CreateWeakLambda(this,[this,M,PC,Cam,Units,Starts,Truck,Check,Fail,Spawn](){
   bool Moved=true;for(int I=0;I<Units.Num();++I)Moved&=FVector::Dist2D(Units[I]->GetActorLocation(),Starts[I])>80;Check(Moved,TEXT("all four selected NPCs actually move"));
   for(auto* U:Units){U->SetActorLocation(FVector(-6000,6000,100));U->IssueOrder(EKTWUnitOrder::Defend,U->GetActorLocation());}
   auto* Target=Spawn(0,EKTWRole::Runner,FVector(-6000,120,100));Target->Health=Target->Stats.MaxHealth=10000;
   TArray<AKTWSecurity*> Guards;for(int I=0;I<2;++I){const FVector P(-6000,I?-600:0,100);auto* G=GetWorld()->SpawnActorDeferred<AKTWSecurity>(AKTWSecurity::StaticClass(),FTransform(P));G->Gun=I==1;G->Truck=Truck;G->FinishSpawning(FTransform(P));Guards.Add(G);}
   TArray<FVector> Hands;for(auto* G:Guards){G->GetMesh()->TickAnimation(.01f,false);G->GetMesh()->RefreshBoneTransforms();Hands.Add(G->GetMesh()->GetSocketLocation(TEXT("hand_r")));G->Tick(.1f);G->SetActorTickEnabled(false);}
   Check(Guards[0]->FindComponentByClass<UKTWCombatAction>()->bPlaying&&Guards[1]->FindComponentByClass<UKTWCombatAction>()->bPlaying,TEXT("melee and gun guard AI start attack playback"));
   Check(Target->Health==10000,TEXT("guard animation windup precedes damage"));
   Cam->PanTo(FVector(-6000,0,0));Cam->TopHeight=6500;
   FTimerHandle Hit;GetWorldTimerManager().SetTimer(Hit,FTimerDelegate::CreateWeakLambda(this,[this,M,Target,Guards,Hands,Check,Fail](){
    bool Animated=true,Hit=true;for(int I=0;I<2;++I){Animated&=FVector::Dist(Hands[I],Guards[I]->GetMesh()->GetSocketLocation(TEXT("hand_r")))>25;Hit&=Guards[I]->FindComponentByClass<UKTWCombatAction>()->ConfirmedHits==1;}
    Check(Animated,TEXT("both guard attacks visibly move the weapon hand"));Check(Hit&&Target->Health==9965,TEXT("melee and gun each apply one timed normal hit"));
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Engagement_GuardAttack.png"),true,false);
    for(auto* G:Guards)G->FindComponentByClass<UKTWCombatAction>()->CancelAction();Target->Health=10000;
    Late=true;ArmAt=250;M->BattleSeconds=249;Check(DamageScale(Target)==1,TEXT("grace period has no damage boost"));M->BattleSeconds=250;
    Check(DamageScale(Target)==1.5f&&DamageScale(Guards[0])==1.5f,TEXT("armed sudden death boosts soldiers and guards equally"));
    for(auto* G:Guards)G->FindComponentByClass<UKTWCombatAction>()->Attack(Target,G->Gun?15:20,G->Gun?1100:160,false,G->Gun);
    FTimerHandle LateHit;GetWorldTimerManager().SetTimer(LateHit,FTimerDelegate::CreateWeakLambda(this,[this,Target,Guards,Check,Fail](){
     Check(Target->Health==9947,TEXT("actual sudden death damage is fifty percent higher with rounding"));
     FTimerHandle Restore;GetWorldTimerManager().SetTimer(Restore,FTimerDelegate::CreateWeakLambda(this,[this,Guards,Check,Fail](){
      bool Restored=true;for(auto* G:Guards)Restored&=!G->FindComponentByClass<UKTWCombatAction>()->bPlaying&&G->GetMesh()->GetAnimInstance()!=nullptr;Check(Restored,TEXT("guard attacks recover to locomotion"));
      UE_LOG(LogTemp,Display,TEXT("KTW ENGAGEMENT COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);
     }),.8f,false);
    }),.32f,false);
   }),.32f,false);
  }),1.5f,false);
 }),1.5f,false);
#endif
}

