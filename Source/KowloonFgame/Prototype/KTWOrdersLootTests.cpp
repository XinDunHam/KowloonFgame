#include "KTWStreetWar.h"
#include "KTWPrototype.h"
#include "KTWPresentation.h"
#include "KTWCombatAction.h"
#include "KTWCommander.h"
#include "KTWMatch.h"
#include "KTWGold.h"
#include "KTWGeneration.h"
#include "KTWTactics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "TimerManager.h"
#include "Misc/Paths.h"

void AKTWStreetWar::RunOrdersLootTests(){
#if !UE_BUILD_SHIPPING
 if(!Initialized){FTimerHandle Wait;GetWorldTimerManager().SetTimer(Wait,this,&AKTWStreetWar::RunOrdersLootTests,1.f,false);return;}
 auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const TCHAR* Name){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW ORDERS LOOT %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto* M=AKTWMatch::Find(GetWorld());auto* PC=Cast<AKTWPrototypeController>(GetWorld()->GetFirstPlayerController());auto* Life=AKTWStreetLife::Find(GetWorld());AKTWBoss* Boss=nullptr;
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){if(I->Faction==0)Boss=*I;else I->Destroy();}
 Check(M&&PC&&Life&&Boss,TEXT("live fixture initialized"));if(!M||!PC||!Life||!Boss){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 M->StartBattle();M->BattleSeconds=10;
 for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)I->SetActorTickEnabled(false);
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();
 for(auto W:Life->GroundWeapons)if(IsValid(W))W->Destroy();Life->GroundWeapons.Empty();
 auto Spawn=[this](EKTWRole UnitRole,int Team,FVector P,bool Combat){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->Faction=Team;U->UnitRole=UnitRole;U->bAutoPatrol=false;U->bEnableCombat=Combat;U->FinishSpawning(FTransform(P));U->IssueOrder(EKTWUnitOrder::Defend,P);return U;};
 Boss->SetActorLocation(FVector(-6000,5000,100));Boss->bPlayerOrdered=false;Boss->bLowHealthRetreat=false;Boss->Health=Boss->MaxHealth;
 const FVector Start=Boss->GetActorLocation();PC->SelectedBoss=Boss;PC->SelectedUnit=nullptr;PC->SelectedUnits.Empty();
 Check(Cast<AAIController>(Boss->GetController())!=nullptr,TEXT("boss retains live AI controller"));
 Check(PC->CommandAtWorld(FVector(-6000,3300,20)),TEXT("boss accepts player ground move"));
 auto* Attacker=Spawn(EKTWRole::Pipe,0,FVector(-6000,2200,100),true);auto* Enemy=Spawn(EKTWRole::Pipe,1,FVector(-6000,1500,100),false);auto* Decoy=Spawn(EKTWRole::Pipe,1,FVector(-5700,2200,100),false);
 for(auto* U:{Enemy,Decoy}){U->Health=U->Stats.MaxHealth=50000;GetWorldTimerManager().ClearAllTimersForObject(U);}
 PC->SelectedBoss=nullptr;PC->SelectedUnits={Attacker};PC->SelectedUnit=Attacker;
 Check(PC->CommandAtWorld(Enemy->GetActorLocation(),Enemy)&&Attacker->CommandTarget()==Enemy,TEXT("soldier accepts exact enemy target over nearer decoy"));
 TArray<AKTWTestUnit*> Looters;for(int I=0;I<2;++I){auto* U=Spawn(EKTWRole::Knife,0,FVector(-6000,-I*200,100),false);U->Health=40;Looters.Add(U);}
 auto MakeLoot=[this,Life](int Kind,FVector P){auto* W=GetWorld()->SpawnActor<AKTWStreetWeapon>(P,FRotator::ZeroRotator);W->Kind=Kind;W->Hits=Kind==2?3:1;W->BuildModel();Life->GroundWeapons.Add(W);return W;};
 TWeakObjectPtr<AKTWStreetWeapon> Kit=MakeLoot(3,FVector(-6000,80,22));
 FTimerHandle Move;GetWorldTimerManager().SetTimer(Move,FTimerDelegate::CreateWeakLambda(this,[this,M,PC,Life,Boss,Start,Attacker,Enemy,Decoy,Looters,Kit,MakeLoot,Check,Fail](){
  Check(FVector::Dist2D(Start,Boss->GetActorLocation())>300,TEXT("boss actually moves with live behavior tree"));
  Boss->ChooseAutomaticWork();Check(Boss->bPlayerOrdered&&!Boss->AcceptsAutomaticTask()&&FVector::Dist2D(Boss->CommandDestination(),FVector(-6000,3300,20))<120,TEXT("automatic work cannot overwrite player boss destination"));
  Check(Enemy->Health<50000&&Decoy->Health==50000,TEXT("specified enemy takes damage while nearer decoy is ignored"));
  Check(!Kit.IsValid()&&FMath::IsNearlyEqual(Looters[0]->Health+Looters[1]->Health,140.f),TEXT("automatic medkit pickup vanishes and heals only one NPC once"));
  auto* Duplicate=MakeLoot(3,FVector(-6000,0,22));Looters[0]->Health=20;Duplicate->Equip(Looters[0]);Duplicate->Equip(Looters[0]);Check(Looters[0]->Health==80&&Duplicate->Collected&&Duplicate->IsActorBeingDestroyed(),TEXT("repeated medkit equip cannot apply twice in same frame"));
  auto* Weapon=MakeLoot(2,FVector(-6000,-80,22));TWeakObjectPtr<AKTWStreetWeapon> WeakWeapon=Weapon;
  const FVector BaseP=Bases[1]->GetActorLocation(),Approach=BaseP+FVector(-1200,1200,0);auto* Siege=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(Approach));Siege->Faction=0;Siege->UnitRole=EKTWRole::Knife;Siege->FinishSpawning(FTransform(Approach));
  PC->SelectedBoss=nullptr;PC->SelectedUnits={Siege};PC->SelectedUnit=Siege;Check(PC->CommandAtWorld(BaseP,Bases[1]),TEXT("soldier accepts enemy base attack from diagonal approach"));
  Boss->SetActorLocation(Approach+FVector(-100,100,0));PC->SelectedUnits.Empty();PC->SelectedUnit=nullptr;PC->SelectedBoss=Boss;Check(PC->CommandAtWorld(BaseP,Bases[1])&&Boss->Task==EKTWBossTask::Attack,TEXT("boss-only selection accepts enemy base attack"));
  const float Before=Bases[1]->Health;auto* Cam=Cast<AKTWCameraPawn>(PC->GetPawn());if(Cam){Cam->PanTo(BaseP);Cam->TopHeight=6500;}
  FTimerHandle Attack;GetWorldTimerManager().SetTimer(Attack,FTimerDelegate::CreateWeakLambda(this,[this,PC,Life,Boss,Siege,Before,Looters,WeakWeapon,Check,Fail](){
   Check(Bases[1]->Health<Before&&Siege->FindComponentByClass<UKTWCombatAction>()->ConfirmedHits>0,TEXT("soldier reaches legal base perimeter and damages van"));
   Check(Boss->FindComponentByClass<UKTWCombatAction>()->ConfirmedHits>0,TEXT("boss actually attacks designated enemy van"));
   bool Equipped=WeakWeapon.IsValid()&&WeakWeapon->Equipped&&WeakWeapon->Collected&&Cast<AKTWTestUnit>(WeakWeapon->GetOwner());Check(Equipped,TEXT("idle NPC automatically equips nearby weapon and removes ground pickup"));
   if(Equipped){auto* W=WeakWeapon.Get();auto* Owner=Cast<AKTWTestUnit>(W->GetOwner());auto* Other=Owner==Looters[0]?Looters[1]:Looters[0];W->Equip(Other);Check(W->GetOwner()==Owner&&!W->Accepts(Other),TEXT("claimed weapon cannot be picked by another NPC"));W->Drop();Check(!WeakWeapon.IsValid()&&!Life->Equipment(Owner),TEXT("collected equipment is destroyed instead of dropping as reusable ground loot"));}
   FVector2D Screen;PC->ProjectWorldLocationToScreen(Bases[1]->Body->Bounds.Origin,Screen);Check(PC->PickFieldAt(Screen)==Bases[1],TEXT("enemy car body is selectable as field target"));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/OrdersLoot_BaseHealth.png"),true,false);
   FTimerHandle Done;GetWorldTimerManager().SetTimer(Done,FTimerDelegate::CreateWeakLambda(this,[Fail](){UE_LOG(LogTemp,Display,TEXT("KTW ORDERS LOOT COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);}),.5f,false);
  }),4.f,false);
 }),3.f,false);
#endif
}
