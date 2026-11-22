#include "KTWCommander.h"
#include "KTWStreetWar.h"
#include "KTWSession.h"
#include "KTWMatch.h"
#include "KTWGold.h"
#include "KTWTactics.h"
#include "KTWGeneration.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

AKTWCommander::AKTWCommander(){PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.5f;}
bool AKTWCommander::FixtureMode(){
 const TCHAR* Flags[]={TEXT("KTWProgressionTest"),TEXT("KTWEconomyTest"),TEXT("KTWGoldTest"),TEXT("KTWGoldAutoTest"),TEXT("KTWPhase3Test"),TEXT("KTWRestartTest"),TEXT("KTWCommandTest"),TEXT("KTWCommanderTest")};
 for(const TCHAR* Flag:Flags)if(FParse::Param(FCommandLine::Get(),Flag))return true;return false;
}
void AKTWCommander::BeginPlay(){Super::BeginPlay();if(FixtureMode())SetActorTickEnabled(false);
#if !UE_BUILD_SHIPPING
 if(Faction==1&&FParse::Param(FCommandLine::Get(),TEXT("KTWAIPlayTest"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AKTWCommander::RunAIPlayTest,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWCommandTest"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AKTWCommander::RunCommandTests,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWCommanderTest"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AKTWCommander::RunCommanderTests,2.f,false);}
#endif
}
void AKTWCommander::Tick(float D){Super::Tick(D);auto* M=AKTWMatch::Find(GetWorld());if(M&&M->CanAct()&&!UGameplayStatics::IsGamePaused(GetWorld())&&M->BattleSeconds>=NextDecision){NextDecision=M->BattleSeconds+(UKTWSession::Get(this)?UKTWSession::Get(this)->DecisionSeconds():3.f);Decide();}}
void AKTWCommander::Observe(){
 auto* M=AKTWMatch::Find(GetWorld());if(!M)return;const double Now=M->BattleSeconds;VisibleContacts=0;
 TArray<ACharacter*> Observers;
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0)Observers.Add(*I);
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==Faction&&!I->bRecovering)Observers.Add(*I);
 for(TActorIterator<ACharacter>I(GetWorld());I;++I){bool Enemy=false,Boss=false;EKTWRole SeenRole=EKTWRole::Runner;
  if(auto* U=Cast<AKTWTestUnit>(*I)){Enemy=U->Faction!=Faction&&U->Health>0;SeenRole=U->UnitRole;}
  if(auto* B=Cast<AKTWBoss>(*I)){Enemy=B->Faction!=Faction&&!B->bRecovering;Boss=true;}
  if(!Enemy)continue;bool Visible=false;if(AKTWStreetWar::Find(GetWorld()))Visible=AKTWStreetWar::Visible(GetWorld(),*I,Faction);else for(const auto* O:Observers)if(FKTWTactics::CanSee(O,*I)){Visible=true;break;}if(!Visible)continue;
  ++VisibleContacts;FKTWContact* C=Contacts.FindByPredicate([&](const FKTWContact& V){return V.Actor.Get()==*I;});if(!C)C=&Contacts.AddDefaulted_GetRef();C->Actor=*I;C->LastPosition=I->GetActorLocation();C->Role=SeenRole;C->Boss=Boss;C->SeenAt=Now;
 }
 // Out-of-sight memory stores snapshots only, never reads a hidden enemy's live position or health.
 Contacts.RemoveAll([&](const FKTWContact& C){return !C.Actor.IsValid()||Now-C.SeenAt>3;});
}
EKTWRole AKTWCommander::DesiredRecruit()const{
 auto* M=AKTWMatch::Find(GetWorld());int32 Own[4]={0,0,0,0};for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0)++Own[int32(I->UnitRole)];if(M)for(const auto& Q:M->Queue)if(Q.Faction==Faction)++Own[int32(Q.UnitRole)];
 if(Own[0]<2)return EKTWRole::Runner;
 int32 Hostile[4]={0,0,0,0};for(const auto& C:Contacts)if(!C.Boss)++Hostile[int32(C.Role)];int32 Threat=-1,Best=0;for(int32 I=1;I<4;++I)if(Hostile[I]>Best){Best=Hostile[I];Threat=I;}
 if(Threat==int32(EKTWRole::Pipe))return EKTWRole::Thrower;if(Threat==int32(EKTWRole::Thrower))return EKTWRole::Knife;if(Threat==int32(EKTWRole::Knife))return EKTWRole::Pipe;
 if(Own[0]<3&&Own[1]+Own[2]+Own[3]>=4)return EKTWRole::Runner;
 const EKTWRole Mix[]={EKTWRole::Knife,EKTWRole::Pipe,EKTWRole::Thrower};return Mix[RecruitmentTurn%3];
}
void AKTWCommander::Recruit(int32 Lane,bool Emergency){
 auto* M=AKTWMatch::Find(GetWorld());if(!M||M->BattleSeconds<NextRecruit)return;NextRecruit=M->BattleSeconds+3;
 const EKTWRole RecruitRole=DesiredRecruit();if(RecruitRole==EKTWRole::Runner){int32 Counts[3]={0,0,0};for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0&&I->bRunner)++Counts[FMath::Clamp(I->Lane,0,2)];for(const auto& Q:M->Queue)if(Q.Faction==Faction&&Q.UnitRole==EKTWRole::Runner)++Counts[Q.Lane];Lane=0;for(int32 L=1;L<3;++L)if(Counts[L]<Counts[Lane])Lane=L;}
 auto Rows=UKTWUnitBalance::Defaults();if(auto* B=LoadObject<UKTWUnitBalance>(nullptr,TEXT("/Game/KTW/Data/DA_UnitBalance_V4.DA_UnitBalance_V4")))if(B->Units.Num()==4)Rows=B->Units;
 const int32 Price=Rows[int32(RecruitRole)].Price,Money=M->Accounts[Faction].Cash;
 if(!Emergency&&(Price>Money*.45f||Money-Price<80))return;
 if(!Emergency&&Money>=290&&Contacts.Num()==0&&(M->LowBoxes[Faction]>=3||RecruitmentTurn%4==2)&&M->BoxRemaining(Faction)<=0&&M->OpenBox(Faction,Lane)){++Purchases;++RecruitmentTurn;return;}
 for(int32 Try=0;Try<3;++Try)if(M->Purchase(Faction,RecruitRole,(Lane+Try)%3)){++Purchases;++RecruitmentTurn;break;}
}
bool AKTWCommander::Available(AKTWTestUnit* U,bool Emergency)const{
 if(auto* War=AKTWStreetWar::Find(GetWorld()))if(U&&War->DiamondCarrier&&U->Order==EKTWUnitOrder::Escort&&U->CommandTarget()==War->DiamondCarrier)return false;
 if(!IsValid(U)||U->Health<=0||U->Faction!=Faction||AKTWStreetWar::Busy(U)||(AKTWStreetWar::Find(GetWorld())&&(U->Order==EKTWUnitOrder::Attack||U->Order==EKTWUnitOrder::Retreat)))return false;auto* C=U->FindComponentByClass<UKTWGoldCarrier>();if(!C||C->Carried>0||C->Task==EKTWGoldTask::PickingUp||C->Task==EKTWGoldTask::Delivering||C->Task==EKTWGoldTask::Healing)return false;
 const double* Until=AssignedUntil.Find(U);auto* M=AKTWMatch::Find(GetWorld());return Emergency||!Until||!M||M->BattleSeconds>=*Until;
}
void AKTWCommander::Decide(bool AllowRecruit){
 auto* M=AKTWMatch::Find(GetWorld());if(!M||!M->CanAct()||M->BattleSeconds>=720||UGameplayStatics::IsGamePaused(GetWorld()))return;
 Observe();for(auto It=AssignedUntil.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 float Pressure[3]={0,0,0};for(const auto& Contact:Contacts){float Best=2500;int32 Lane=-1;for(int32 L=0;L<3;++L){const float D=FVector::Dist2D(Contact.LastPosition,UKTWGoldCarrier::VaultPosition(Faction,L));if(D<Best){Best=D;Lane=L;}}if(Lane>=0)Pressure[Lane]+=Contact.Boss?2.f:1.f;}
 int32 DangerLane=1;for(int32 L=0;L<3;++L)if(Pressure[L]>Pressure[DangerLane])DangerLane=L;
 const bool Threat=Pressure[DangerLane]>0,Hold=M->Accounts[Faction].Gold>=500;
 Plan=Hold?TEXT("HOLD GOLD"):Threat?TEXT("DEFEND VAULT"):TEXT("COLLECT / ESCORT / RAID");
 if(AllowRecruit)Recruit(DangerLane,Threat||Hold);
 TArray<AKTWTestUnit*> Fighters,Carriers;AKTWBoss* Boss=nullptr;
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0){auto* C=I->FindComponentByClass<UKTWGoldCarrier>();if(C&&C->Carried>0)Carriers.Add(*I);if(!I->bRunner)Fighters.Add(*I);}
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==Faction){Boss=*I;break;}
 TSet<AKTWTestUnit*> Used;
 auto Assign=[&](AKTWTestUnit* U,EKTWUnitOrder O,FVector P,AActor* Target,bool Emergency){if(!Available(U,Emergency)||Used.Contains(U))return false;
  Used.Add(U);if(U->Order==O&&((O==EKTWUnitOrder::Escort&&U->CommandTarget()==Target)||(O==EKTWUnitOrder::Defend&&FVector::Dist2D(U->CommandDestination(),P)<100))){AssignedUntil.Add(U,M->BattleSeconds+4);return true;}
  if(!U->IssueOrder(O,P,Target))return false;AssignedUntil.Add(U,M->BattleSeconds+5);if(O==EKTWUnitOrder::Defend)++DefendOrders;if(O==EKTWUnitOrder::Escort)++EscortOrders;return true;
 };
 if(Threat||Hold){const int32 Quota=FMath::Min(Fighters.Num(),Hold?3:FMath::Max(1,FMath::CeilToInt(Pressure[DangerLane])));
  for(int32 Slot=0;Slot<Quota;++Slot){const int32 Lane=Hold?(DangerLane+Slot)%3:DangerLane;const FVector Vault=UKTWGoldCarrier::VaultPosition(Faction,Lane);
   Fighters.Sort([&](const AKTWTestUnit& A,const AKTWTestUnit& B){return FVector::DistSquared2D(A.GetActorLocation(),Vault)<FVector::DistSquared2D(B.GetActorLocation(),Vault);});for(auto* U:Fighters)if(Assign(U,EKTWUnitOrder::Defend,Vault,nullptr,true))break;
  }
 }

 Carriers.Sort([](const AKTWTestUnit& A,const AKTWTestUnit& B){return A.FindComponentByClass<UKTWGoldCarrier>()->Carried>B.FindComponentByClass<UKTWGoldCarrier>()->Carried;});
 for(auto* Courier:Carriers){bool Guard=false;for(auto* U:Fighters)if(!Used.Contains(U)&&U->Order==EKTWUnitOrder::Escort&&U->CommandTarget()==Courier){Used.Add(U);Guard=true;break;}if(Guard)continue;
  Fighters.Sort([&](const AKTWTestUnit& A,const AKTWTestUnit& B){return FVector::DistSquared2D(A.GetActorLocation(),Courier->GetActorLocation())<FVector::DistSquared2D(B.GetActorLocation(),Courier->GetActorLocation());});
  for(auto* U:Fighters)if(Assign(U,EKTWUnitOrder::Escort,Courier->GetActorLocation(),Courier,false))break;
 }
 if(!Hold&&!Threat&&M->BattleSeconds>=60&&M->Accounts[1-Faction].Gold>=60){const int32 Limit=Boss&&!Boss->bRecovering&&!Boss->bLowHealthRetreat?2:1;int32 Raiders=0;for(auto* U:Fighters){auto* C=U->FindComponentByClass<UKTWGoldCarrier>();if(C&&C->IsRaiding()){Used.Add(U);++Raiders;}}
  for(auto* U:Fighters)if(Raiders<Limit&&!Used.Contains(U)&&Available(U,false)){if(U->FindComponentByClass<UKTWGoldCarrier>()->RequestRaid()){Used.Add(U);AssignedUntil.Add(U,M->BattleSeconds+6);++Raiders;++RaidOrders;}}
 }
 for(auto* U:Fighters)if(!Used.Contains(U)&&Available(U,false)&&(U->Order==EKTWUnitOrder::Defend||U->Order==EKTWUnitOrder::Escort)){if(U->IssueOrder(EKTWUnitOrder::Advance,U->GetActorLocation()))AssignedUntil.Add(U,M->BattleSeconds+4);}
 if(Boss&&!Boss->bRecovering&&!Boss->bLowHealthRetreat&&Boss->AcceptsAutomaticTask()){EKTWBossTask Task=EKTWBossTask::Sit;AActor* Target=nullptr;FVector Goal=AKTWStreetDirector::BasePosition(Faction);
  if(Threat||Hold)Goal=UKTWGoldCarrier::VaultPosition(Faction,DangerLane);
  else if(Carriers.Num()){Target=Carriers[0];Task=EKTWBossTask::Escort;Goal=Target->GetActorLocation();}
  else if(Fighters.Num()){AKTWTestUnit* Front=nullptr;float Best=MAX_flt;for(auto* U:Fighters){const float D=FVector::DistSquared2D(U->GetActorLocation(),FVector::ZeroVector);if(D<Best){Best=D;Front=U;}}if(Front){Target=Front;Task=EKTWBossTask::Follow;Goal=Target->GetActorLocation();}}
  if(Boss->Task!=Task||Boss->CommandTarget()!=Target||(Task==EKTWBossTask::Sit&&FVector::Dist2D(Boss->CommandDestination(),Goal)>100))if(Boss->IssueTask(Task,Goal,Target))++BossOrders;
 }
}

#if !UE_BUILD_SHIPPING
namespace {
 AKTWTestUnit* TestUnit(UWorld* W,int32 Team,EKTWRole Role,FVector P,int32 Lane=1){
  auto* U=W->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->Faction=Team;U->UnitRole=Role;U->Lane=Lane;U->bEnableCombat=false;U->bAutoPatrol=false;U->Order=EKTWUnitOrder::Defend;U->FinishSpawning(FTransform(P));return U;
 }
 struct FChecks{int32 Pass=0,Fail=0;const TCHAR* Tag;explicit FChecks(const TCHAR* T):Tag(T){}void Check(bool OK,const TCHAR* Name){if(OK)++Pass;else ++Fail;UE_LOG(LogTemp,Display,TEXT("KTW %s %s: %s"),Tag,Name,OK?TEXT("PASS"):TEXT("FAIL"));}void Finish(){UE_LOG(LogTemp,Display,TEXT("KTW %s COMPLETE passed=%d failures=%d"),Tag,Pass,Fail);FPlatformMisc::RequestExitWithStatus(false,Fail?1:0);}};
}
#endif
void AKTWCommander::RunCommandTests(){
#if !UE_BUILD_SHIPPING
 auto C=MakeShared<FChecks>(TEXT("COMMAND"));auto* M=AKTWMatch::Find(GetWorld());auto* PC=Cast<AKTWPrototypeController>(GetWorld()->GetFirstPlayerController());
 if(!M||!PC){C->Check(false,TEXT("fixture ready"));C->Finish();return;}
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();AKTWBoss* Boss=nullptr;for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){if(I->Faction==0)Boss=*I;else I->Destroy();}
 PC->SelectedBoss=Boss;PC->SelectedUnits.Empty();C->Check(PC->CommandAtWorld(AKTWStreetDirector::BasePosition(0)+FVector(360,0,1500)),TEXT("boss accepts legal preparation placement"));
 M->StartBattle();M->SetActorTickEnabled(false);auto* Gold=AKTWGoldSystem::Find(GetWorld());Gold->SetActorTickEnabled(false);
 Boss->SetActorLocation(FVector(-1000,1000,100),false,nullptr,ETeleportType::TeleportPhysics);
 auto* U=TestUnit(GetWorld(),0,EKTWRole::Knife,FVector(-1200,1200,100));PC->SelectedBoss=Boss;PC->SelectedUnits={U};PC->SelectedUnit=U;
 C->Check(PC->CommandAtWorld(FVector(1300,-1300,1600)),TEXT("mixed command accepts ground even when input height matches a roof"));
 auto* AI=Cast<AAIController>(Boss->GetController());C->Check(AI&&AI->GetBlackboardComponent()&&FVector::Dist2D(AI->GetBlackboardComponent()->GetValueAsVector(TEXT("Goal")),FVector(1300,-1300,0))<100,TEXT("boss blackboard updated in same call"));
 const TWeakObjectPtr<AKTWBoss> WB=Boss;const TWeakObjectPtr<AKTWTestUnit> WU=U;const TWeakObjectPtr<AKTWPrototypeController> WP=PC;
 FTimerHandle Redirect;GetWorldTimerManager().SetTimer(Redirect,FTimerDelegate::CreateLambda([C,WB,WU,WP](){
  const FVector Goal(-1300,-1300,20);C->Check(WP.IsValid()&&WP->CommandAtWorld(Goal),TEXT("second right click replaces in-flight move"));C->Check(WB.IsValid()&&WU.IsValid()&&FVector::Dist2D(WB->CommandDestination(),Goal)<100&&FVector::Dist2D(WU->CommandDestination(),Goal)<100,TEXT("boss and soldier store latest destination"));
  const FVector Before=WB->CommandDestination();C->Check(!WP->CommandAtWorld(FVector(-3000,-4500,20))&&WB->CommandDestination().Equals(Before,1),TEXT("blocked ground rejects without erasing old command"));
  UGameplayStatics::SetGamePaused(WP->GetWorld(),true);C->Check(!WP->CommandAtWorld(FVector(1300,1300,20)),TEXT("paused mouse command rejected"));UGameplayStatics::SetGamePaused(WP->GetWorld(),false);
 }),.15f,false);
 FTimerHandle Arrived;GetWorldTimerManager().SetTimer(Arrived,FTimerDelegate::CreateLambda([this,C,WB,WU,WP](){
  const FVector Goal(-1300,-1300,20);C->Check(WB.IsValid()&&FVector::Dist2D(WB->GetActorLocation(),Goal)<160,TEXT("boss physically reaches newest click"));C->Check(WU.IsValid()&&FVector::Dist2D(WU->GetActorLocation(),Goal)<160&&WU->Order==EKTWUnitOrder::Defend,TEXT("soldier reaches click and holds instead of auto-running away"));
  auto* Enemy=TestUnit(GetWorld(),1,EKTWRole::Pipe,FVector(-1000,-1000,100));Enemy->SetActorHiddenInGame(true);C->Check(!WU->IssueOrder(EKTWUnitOrder::Attack,Enemy->GetActorLocation(),Enemy),TEXT("hidden enemy cannot be issued as attack target"));Enemy->Destroy();
  auto* Courier=TestUnit(GetWorld(),0,EKTWRole::Runner,FVector(-1000,-1000,100));C->Check(WU->IssueOrder(EKTWUnitOrder::Escort,Courier->GetActorLocation(),Courier)&&WB->IssueTask(EKTWBossTask::Escort,Courier->GetActorLocation(),Courier),TEXT("soldier and boss accept live courier escort"));
  FDamageEvent Hurt;Courier->TakeDamage(1,Hurt,nullptr,nullptr);
  C->Check(Courier->IssueOrder(EKTWUnitOrder::Move,FVector(1300,1300,20)),TEXT("escort target starts moving across junction"));
  const TWeakObjectPtr<AKTWTestUnit> WC=Courier;FTimerHandle Follow;GetWorldTimerManager().SetTimer(Follow,FTimerDelegate::CreateLambda([this,C,WB,WU,WC](){
   C->Check(WC.IsValid()&&FVector::Dist2D(WC->GetActorLocation(),FVector(1300,1300,20))<160,TEXT("injured runner honors explicit move instead of automatic flee"));
   C->Check(WC.IsValid()&&WU.IsValid()&&FVector::Dist2D(WC->GetActorLocation(),WU->GetActorLocation())<650,TEXT("escort catches moving target and remains nearby"));
   C->Check(WC.IsValid()&&WB.IsValid()&&FVector::Dist2D(WC->GetActorLocation(),WB->GetActorLocation())<750,TEXT("boss follows moving courier destination"));
   auto* Threat=TestUnit(GetWorld(),1,EKTWRole::Pipe,FVector(1950,1950,100));WU->bEnableCombat=true;const TWeakObjectPtr<AKTWTestUnit> WT=Threat;
   FTimerHandle Leash;GetWorldTimerManager().SetTimer(Leash,FTimerDelegate::CreateLambda([this,C,WB,WU,WC,WT](){
    C->Check(WT.IsValid()&&WT->Health==WT->Stats.MaxHealth&&WU.IsValid()&&WU->Order==EKTWUnitOrder::Escort,TEXT("escort ignores enemy outside courier five hundred leash"));if(WT.IsValid())WT->Destroy();if(WC.IsValid())WC->Destroy();
    FTimerHandle Lost;GetWorldTimerManager().SetTimer(Lost,FTimerDelegate::CreateLambda([this,C,WB,WU](){
     C->Check(WU.IsValid()&&WU->Order!=EKTWUnitOrder::Escort,TEXT("dead escort target releases soldier task"));C->Check(WB.IsValid()&&WB->Task==EKTWBossTask::Sit,TEXT("dead escort target leaves boss holding position"));
     if(WB.IsValid())WB->Health=WB->MaxHealth*.2f;
     FTimerHandle Retreat;GetWorldTimerManager().SetTimer(Retreat,FTimerDelegate::CreateLambda([this,C,WB](){
      C->Check(WB.IsValid()&&WB->bLowHealthRetreat&&WB->Task==EKTWBossTask::Return&&!WB->IssueTask(EKTWBossTask::Sit,FVector(0,0,20),nullptr),TEXT("low-health safety retreat takes priority over attack move"));
      if(WB.IsValid()){WB->Health=WB->MaxHealth*.8f;WB->SetActorLocation(AKTWStreetDirector::BasePosition(0)+FVector(0,0,100),false,nullptr,ETeleportType::TeleportPhysics);}
      FTimerHandle Ready;GetWorldTimerManager().SetTimer(Ready,FTimerDelegate::CreateLambda([C,WB](){C->Check(WB.IsValid()&&!WB->bLowHealthRetreat&&WB->Task==EKTWBossTask::Sit&&!WB->AcceptsAutomaticTask(),TEXT("eighty percent recovery holds boss at home before AI retasking"));C->Finish();}),.4f,false);
     }),.4f,false);
    }),.8f,false);
   }),.6f,false);

  }),10.f,false);
 }),6.f,false);
#endif
}
void AKTWCommander::RunCommanderTests(){
#if !UE_BUILD_SHIPPING
 auto C=MakeShared<FChecks>(TEXT("COMMANDER"));auto* M=AKTWMatch::Find(GetWorld());auto* S=AKTWGoldSystem::Find(GetWorld());if(!M||!S){C->Check(false,TEXT("fixture ready"));C->Finish();return;}
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();AKTWBoss* Boss=nullptr;for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){if(I->Faction==1)Boss=*I;else I->Destroy();}
 M->StartBattle();M->SetActorTickEnabled(false);S->SetActorTickEnabled(false);
 const FVector Home=AKTWStreetDirector::BasePosition(1);auto* R1=TestUnit(GetWorld(),1,EKTWRole::Runner,Home+FVector(-500,0,100),0);TestUnit(GetWorld(),1,EKTWRole::Runner,Home+FVector(0,500,100),2);
 for(int32 I=0;I<3;++I)TestUnit(GetWorld(),1,EKTWRole(I+1),UKTWGoldCarrier::VaultPosition(1,1)+FVector(-100-I*100,100,100));
 M->Tick(30);Decide();C->Check(Purchases==0&&M->Accounts[1].Cash==230,TEXT("computer respects normal reserve and percentage budget"));
 M->Tick(20);Decide();C->Check(Purchases==1&&M->Accounts[1].Cash==170&&M->QueueCount(1,1)==1&&M->CashConserved(),TEXT("AI recruit pays identical price and reserves lane queue"));
 const int32 Before=M->Population(1);M->Tick(3.1f);C->Check(M->QueueCount(1,1)==0&&M->Population(1)==Before,TEXT("computer reinforcement waits same spawn queue"));
 auto* E=TestUnit(GetWorld(),0,EKTWRole::Pipe,FVector(-6000,0,100));Observe();C->Check(VisibleContacts==0&&KnownContacts()==0,TEXT("distant enemy army is not read globally"));
 E->SetActorLocation(UKTWGoldCarrier::VaultPosition(1,1)+FVector(-300,300,100),false,nullptr,ETeleportType::TeleportPhysics);Decide(false);C->Check(VisibleContacts==1&&DefendOrders>0,TEXT("observed home threat assigns defenders"));C->Check(DesiredRecruit()==EKTWRole::Thrower,TEXT("observed pipe composition selects thrower counter"));
 const FVector Remembered=Contacts.Num()?Contacts[0].LastPosition:FVector::ZeroVector;E->SetActorHiddenInGame(true);E->SetActorLocation(FVector(-6000,0,100),false,nullptr,ETeleportType::TeleportPhysics);M->Tick(1);Observe();C->Check(VisibleContacts==0&&Contacts.Num()==1&&Contacts[0].LastPosition.Equals(Remembered),TEXT("hidden contacts keep snapshot rather than live positions"));M->Tick(3.1f);Observe();C->Check(KnownContacts()==0,TEXT("lost contacts expire after three seconds"));E->Destroy();
 M->Tick(20);M->Accounts[0].Gold=100;S->GeneratedGold=100;AssignedUntil.Empty();Decide(false);C->Check(RaidOrders>0,TEXT("unguarded public bank creates legal raid assignments"));C->Check(M->Accounts[0].Gold==100&&M->Accounts[1].Gold==0&&S->GoldConserved(),TEXT("issuing raid never credits gold early"));
 S->Tick(0);auto* P=S->Piles[1].Get();R1->SetActorLocation(P->GetActorLocation()+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);auto* Cargo=R1->FindComponentByClass<UKTWGoldCarrier>();C->Check(Cargo->RequestPickup(P),TEXT("AI courier uses standard pickup component"));
 const TWeakObjectPtr<AKTWTestUnit> WR=R1;const TWeakObjectPtr<AKTWBoss> WB=Boss;FTimerHandle Transport;
 GetWorldTimerManager().SetTimer(Transport,FTimerDelegate::CreateLambda([this,C,WR,WB](){auto* M=AKTWMatch::Find(GetWorld());auto* S=AKTWGoldSystem::Find(GetWorld());AssignedUntil.Empty();Decide(false);
  auto* Cargo=WR.IsValid()?WR->FindComponentByClass<UKTWGoldCarrier>():nullptr;C->Check(Cargo&&Cargo->Carried>0&&EscortOrders>0,TEXT("valuable loaded courier receives an ordinary escort"));C->Check(WB.IsValid()&&WB->Task==EKTWBossTask::Escort&&WB->CommandTarget()==WR.Get(),TEXT("boss switches to loaded courier escort"));
  M->Accounts[1].Gold=500;S->GeneratedGold+=500;M->GoldChanged(1);Decide(false);C->Check(Plan==TEXT("HOLD GOLD")&&DefendOrders>=2,TEXT("winning threshold switches to guarded vault defense"));C->Check(Cargo&&Cargo->Carried>0,TEXT("defense never overrides loaded courier transport"));
  bool LaneGuard[3]={false,false,false};for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0&&I->Order==EKTWUnitOrder::Defend)for(int32 L=0;L<3;++L)if(FVector::Dist2D(I->CommandDestination(),UKTWGoldCarrier::VaultPosition(Faction,L))<100)LaneGuard[L]=true;C->Check(LaneGuard[0]&&LaneGuard[1]&&LaneGuard[2],TEXT("threshold defense covers all three shared-bank interfaces"));
  const int32 Orders=Purchases+DefendOrders+EscortOrders+RaidOrders+BossOrders,Money=M->Accounts[1].Cash;UGameplayStatics::SetGamePaused(GetWorld(),true);Decide();C->Check(Orders==Purchases+DefendOrders+EscortOrders+RaidOrders+BossOrders&&M->Accounts[1].Cash==Money,TEXT("pause stops decisions and spending"));UGameplayStatics::SetGamePaused(GetWorld(),false);
  if(WB.IsValid()&&WR.IsValid())C->Check(WB->IssueTask(EKTWBossTask::Escort,WR->GetActorLocation(),WR.Get()),TEXT("loaded boss escort explicitly restored after defense fixture"));if(WR.IsValid()){if(auto* AI=Cast<AAIController>(WR->GetController()))AI->StopMovement();WR->GetCharacterMovement()->StopMovementImmediately();WR->SetActorLocation(AKTWStreetDirector::BasePosition(Faction)+FVector(-200,0,0),false,nullptr,ETeleportType::TeleportPhysics);UE_LOG(LogTemp,Display,TEXT("KTW TELEPORT DIAGNOSTIC faction=%d pos=%s order=%d paused=%d"),Faction,*WR->GetActorLocation().ToString(),int32(WR->Order),UGameplayStatics::IsGamePaused(GetWorld()));}
  FTimerHandle Delivered;GetWorldTimerManager().SetTimer(Delivered,FTimerDelegate::CreateLambda([this,C,WR,WB,Money](){auto* M=AKTWMatch::Find(GetWorld());auto* S=AKTWGoldSystem::Find(GetWorld());auto* Cargo=WR.IsValid()?WR->FindComponentByClass<UKTWGoldCarrier>():nullptr;
   UE_LOG(LogTemp,Display,TEXT("KTW DELIVERY DIAGNOSTIC cargo=%d task=%d bossTask=%d bossGoal=%s courierPos=%s"),Cargo?Cargo->Carried:-1,Cargo?int32(Cargo->Task):-1,WB.IsValid()?int32(WB->Task):-1,WB.IsValid()?*WB->CommandDestination().ToString():TEXT("none"),WR.IsValid()?*WR->GetActorLocation().ToString():TEXT("none"));
   C->Check(Cargo&&Cargo->Carried==0&&WB.IsValid()&&WB->Task==EKTWBossTask::Sit&&FVector::Dist2D(WB->CommandDestination(),UKTWGoldCarrier::VaultPosition(Faction,0))<100,TEXT("escort delivery completes with boss holding own forward vault"));
   M->FinishBattle();Decide();C->Check(M->Accounts[1].Cash==Money&&M->CashConserved()&&S->GoldConserved(),TEXT("finished AI preserves cash and gold ledgers"));C->Finish();
  }),1.6f,false);
 }),.8f,false);
#endif
}
void AKTWCommander::RunAIPlayTest(){
#if !UE_BUILD_SHIPPING
 auto* M=AKTWMatch::Find(GetWorld());if(!M)return;const EKTWRole Opening[]={EKTWRole::Runner,EKTWRole::Runner,EKTWRole::Knife,EKTWRole::Pipe};const int32 Lanes[]={0,2,2,1};for(int32 I=0;I<4;++I)M->Purchase(0,Opening[I],Lanes[I]);
 auto* Blue=GetWorld()->SpawnActorDeferred<AKTWCommander>(AKTWCommander::StaticClass(),FTransform::Identity);Blue->Faction=0;Blue->FinishSpawning(FTransform::Identity);M->StartBattle();
 const TWeakObjectPtr<AKTWCommander> WBlue=Blue;FTimerHandle End;GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateLambda([this,WBlue](){auto C=MakeShared<FChecks>(TEXT("AI PLAY"));auto* M=AKTWMatch::Find(GetWorld());auto* S=AKTWGoldSystem::Find(GetWorld());C->Check(M&&M->CashConserved()&&S&&S->GoldConserved(),TEXT("three minutes AI versus AI conserves both resources"));C->Check(WBlue.IsValid()&&WBlue->Purchases>0&&Purchases>0,TEXT("both factions recruit through paid transactions"));C->Check(WBlue.IsValid()&&WBlue->BossOrders>0&&BossOrders>0,TEXT("both bosses receive autonomous tasks"));C->Check(M&&(M->Accounts[0].Gold+S->CarriedGold(0)>0)&&(M->Accounts[1].Gold+S->CarriedGold(1)>0),TEXT("both factions collect and transport real gold"));C->Finish();}),180.f,false);
#endif
}
