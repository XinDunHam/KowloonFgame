#include "KTWStreetWar.h"
#include "KTWPrototype.h"
#include "KTWPresentation.h"
#include "KTWCommander.h"
#include "KTWWorkGuard.h"
#include "KTWGold.h"
#include "KTWMatch.h"
#include "KTWGeneration.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void AKTWStreetWar::RunTacticsTests(){
#if !UE_BUILD_SHIPPING
 if(!Initialized){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWStreetWar::RunTacticsTests,1,false);return;}
 int Fail=0;auto Check=[&](bool OK,const TCHAR* Name){Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW STREET TACTICS %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto* M=AKTWMatch::Find(GetWorld());auto* Life=AKTWStreetLife::Find(GetWorld());Check(M&&Life,TEXT("real street systems available"));if(!M||!Life){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)I->SetActorTickEnabled(false);
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();Tasks.Empty();DecisionClock=10000;M->StartBattle();M->BattleSeconds=10;
 for(auto W:Life->GroundWeapons)if(IsValid(W))W->Destroy();Life->GroundWeapons.Empty();
 const FVector Home=AKTWStreetDirector::BasePosition(1);
 auto Spawn=[&](int Team,FVector P){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->Faction=Team;U->UnitRole=EKTWRole::Knife;U->bEnableCombat=false;U->bAutoPatrol=false;U->FinishSpawning(FTransform(P));return U;};
 TArray<AKTWTestUnit*> Crew;for(int I=0;I<5;++I)Crew.Add(Spawn(1,Home+FVector(-900-I*150,0,100)));
 auto* Threat=Spawn(0,Home+FVector(-1600,150,100));DecideField();int Defense=0;for(auto* U:Crew)Defense+=U->Order==EKTWUnitOrder::Attack&&U->CommandTarget()==Threat;Check(Defense==2,TEXT("visible base threat receives two defenders without sending the whole army"));
 Threat->Destroy();auto* Courier=Crew.Last();auto* Gold=Courier->FindComponentByClass<UKTWGoldCarrier>();Gold->Carried=20;const auto Order=Courier->Order;Crew[0]->Health=Crew[0]->Stats.MaxHealth*.2f;DecideField();Check(Crew[0]->Order==EKTWUnitOrder::Retreat,TEXT("wounded combatant retreats instead of starting a new convoy fight"));Check(Gold->Carried==20&&Courier->Order==Order,TEXT("loaded courier retains delivery and resources"));Gold->Carried=0;
 Diamond=GetWorld()->SpawnActorDeferred<AKTWFieldObject>(AKTWFieldObject::StaticClass(),FTransform(Home+FVector(-1800,0,30)));Diamond->Kind=EKTWFieldKind::Diamond;Diamond->FinishSpawning(FTransform(Home+FVector(-1800,0,30)));Objects.Add(Diamond);DecideField();int Collectors=0;for(const auto& T:Tasks)Collectors+=T.Value.Target==Diamond;Check(Collectors==1,TEXT("only one free unit is assigned to collect the diamond"));
 auto* Carrier=Tasks.IsEmpty()?nullptr:Tasks.CreateIterator().Key().Get();if(Carrier){Cancel(Carrier);DiamondCarrier=Carrier;DecideField();int Escorts=0;for(auto* U:Crew)Escorts+=U->Order==EKTWUnitOrder::Escort&&U->CommandTarget()==Carrier;Check(Escorts<=2&&Escorts>0,TEXT("diamond carrier receives a bounded escort"));}else Check(false,TEXT("diamond collection task exists"));if(DiamondCarrier){int Before=0,After=0;for(auto* Soldier:Crew)Before+=Soldier->Order==EKTWUnitOrder::Escort&&Soldier->CommandTarget()==DiamondCarrier;for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)if(I->Faction==1)I->Decide(false);for(auto* Soldier:Crew)After+=Soldier->Order==EKTWUnitOrder::Escort&&Soldier->CommandTarget()==DiamondCarrier;Check(Before>0&&Before==After,TEXT("ordinary commander preserves the diamond escort assignment"));}DiamondCarrier=nullptr;
 auto* U=Crew[1];Cancel(U);U->IssueOrder(EKTWUnitOrder::Defend,U->GetActorLocation());U->Health=U->Stats.MaxHealth;
 auto* W=GetWorld()->SpawnActor<AKTWStreetWeapon>(U->GetActorLocation()+FVector(-400,0,-78),FRotator::ZeroRotator);W->Kind=1;W->BuildModel();Life->GroundWeapons.Add(W);
 Check(U->IssueOrder(EKTWUnitOrder::PickupGear,W->GetActorLocation(),W)&&W->ClaimedBy==U,TEXT("manual pickup reserves a reachable single-use item"));W->ClaimProgressAt=GetWorld()->GetTimeSeconds()-4;Life->ProcessGear(W);Check(!W->ClaimedBy.IsValid()&&!W->Collected&&U->Order==EKTWUnitOrder::Defend,TEXT("stalled pickup releases claim without consuming item or locking unit"));Check(W->RejectedBy==U&&W->RetryAfter>GetWorld()->GetTimeSeconds(),TEXT("automatic retry backs off from the stalled item"));
 if(auto* Guard=U->FindComponentByClass<UKTWWorkGuard>()){Check(Guard->RequestPath(Home,true)&&!Guard->RequestPath(Home,false)&&Guard->RequestPath(Home,true),TEXT("path requests are throttled while explicit commands remain immediate"));}else Check(false,TEXT("movement recovery component attached"));
 UE_LOG(LogTemp,Display,TEXT("KTW STREET TACTICS COMPLETE failures=%d"),Fail);FPlatformMisc::RequestExitWithStatus(false,Fail?1:0);
#endif
}
