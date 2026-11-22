#include "KTWMatch.h"
#include "KTWStreetWar.h"
#include "KTWSession.h"
#include "KTWTactics.h"
#include "KTWGeneration.h"
#include "KTWGold.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

AKTWMatch::AKTWMatch(){PrimaryActorTick.bCanEverTick=true;Accounts.SetNum(2);}
AKTWMatch* AKTWMatch::Find(const UWorld* World){if(World)for(TActorIterator<AKTWMatch>I(World);I;++I)return *I;return nullptr;}
void AKTWMatch::BeginPlay(){Super::BeginPlay();Feedback=TEXT("Preparation: buy units, right-click to deploy, SPACE to begin.");
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWProgressionTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWMatch::RunProgressionTests,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWEconomyTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWMatch::RunEconomyTests,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWRestartTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWMatch::RunRestartProbe,1.f,false);}
#endif
}
int32 AKTWMatch::Population(int32 Faction)const{int32 N=0;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0)++N;for(const auto& E:Queue)if(E.Faction==Faction)++N;return N;}
int32 AKTWMatch::QueueCount(int32 Faction,int32 Lane)const{int32 N=0;for(const auto& E:Queue)if(E.Faction==Faction&&E.Lane==Lane)++N;return N;}
bool AKTWMatch::CashConserved()const{for(const auto& A:Accounts)if(A.Cash<0||600+A.TimeIncome+A.BountyIncome!=A.Cash+A.Purchases-A.Refunds)return false;return true;}
bool AKTWMatch::FindSpawn(int32 Faction,int32 Lane,FVector& Point)const{
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!NS)return false;
 const FVector Base=AKTWStreetDirector::BasePosition(Faction);const FVector Dir=(AKTWStreetDirector::Route(Lane,Faction)[1]-Base).GetSafeNormal2D();const FVector Side(-Dir.Y,Dir.X,0);
 for(int32 I=0;I<60;++I){const FVector P=Base+Dir*(240+(I/5)*48)+Side*((I%5-2)*120);FNavLocation Nav;
  if(!AKTWStreetDirector::IsRoadPosition(P,-100)||!NS->ProjectPointToNavigation(P+FVector(0,0,100),Nav,FVector(50,50,200))||FVector::Dist2D(P,Nav.Location)>65)continue;
  bool Occupied=false;for(TActorIterator<ACharacter>It(GetWorld());It;++It)if(FVector::Dist2D(It->GetActorLocation(),Nav.Location)<110){Occupied=true;break;}
  if(!Occupied){Point=Nav.Location+FVector(0,0,90);return true;}
 }return false;
}
AKTWTestUnit* AKTWMatch::SpawnRecruit(const FKTWRecruit& E){FVector Point;if(!FindSpawn(E.Faction,E.Lane,Point))return nullptr;
 auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(Point),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);if(!U)return nullptr;
 U->Faction=E.Faction;U->UnitRole=E.UnitRole;U->Lane=E.Lane;U->bAutoPatrol=Phase==EKTWMatchPhase::Battle;U->bEnableCombat=U->bAutoPatrol;U->FinishSpawning(FTransform(Point));U->LaneEntry=AKTWStreetDirector::Route(E.Lane,E.Faction)[1];U->bLaneDeparturePending=!UKTWSession::Diagnostic();return U;
}
bool AKTWMatch::Purchase(int32 Faction,EKTWRole UnitRole,int32 Lane){
 auto Fail=[&](const TCHAR* Why){if(Faction==0)Feedback=Why;return false;};
 if(!Accounts.IsValidIndex(Faction)||Lane<0||Lane>2||int32(UnitRole)>3||UGameplayStatics::IsGamePaused(GetWorld())||(Phase==EKTWMatchPhase::Finished||(Phase==EKTWMatchPhase::Battle&&BattleSeconds>=AKTWStreetWar::Limit(GetWorld()))))return Fail(TEXT("Recruit rejected: invalid state."));
 auto Rows=UKTWUnitBalance::Defaults();if(auto* B=LoadObject<UKTWUnitBalance>(nullptr,TEXT("/Game/KTW/Data/DA_UnitBalance_V4.DA_UnitBalance_V4")))if(B->Units.Num()==4)Rows=B->Units;
 const int32 Price=bBoxPurchase?130:Rows[int32(UnitRole)].Price;if(Price<=0||Accounts[Faction].Cash<Price)return Fail(TEXT("Not enough CASH (gold is never spent)."));
 if(Population(Faction)>=20)return Fail(TEXT("Population full: 20 including queued units."));
 if(Phase==EKTWMatchPhase::Battle&&QueueCount(Faction,Lane)>=3)return Fail(TEXT("This lane queue is full (3)."));
 FVector P;if(!FindSpawn(Faction,Lane,P))return Fail(TEXT("No free legal base spawn point. Try another lane."));
 FKTWRecruit E;E.Faction=Faction;E.Lane=Lane;E.UnitRole=UnitRole;E.Paid=Price;
 if(Phase==EKTWMatchPhase::Preparation){auto* U=SpawnRecruit(E);if(!U)return Fail(TEXT("Spawn failed; no cash charged."));if(!bBoxPurchase)Refundable.Add(U,Price);}
 else Queue.Add(E);
 Accounts[Faction].Cash-=Price;Accounts[Faction].Purchases+=Price;if(Faction==0)UKTWSession::Cue(this,TEXT("Buy"));
 if(Faction==0)Feedback=Phase==EKTWMatchPhase::Preparation?TEXT("Purchased. Select and right-click your base area to deploy."):TEXT("Reinforcement queued; 3 seconds per unit, per lane.");return true;
}
bool AKTWMatch::Refund(AKTWTestUnit* Unit){if(Phase!=EKTWMatchPhase::Preparation||UGameplayStatics::IsGamePaused(GetWorld())||!IsValid(Unit)||Unit->Health<=0||!Refundable.Contains(Unit))return false;
 const int32 Team=Unit->Faction;const int32 Paid=Refundable.FindChecked(Unit);Refundable.Remove(Unit);Accounts[Team].Cash+=Paid;Accounts[Team].Refunds+=Paid;Unit->Health=0;Unit->Destroy();Feedback=TEXT("Preparation purchase refunded once.");return true;
}
bool AKTWMatch::Deploy(AKTWTestUnit* U,FVector P){
 if(Phase!=EKTWMatchPhase::Preparation||UGameplayStatics::IsGamePaused(GetWorld())||!IsValid(U)||U->Health<=0||U->Faction<0||U->Faction>1||U->GetWorld()!=GetWorld())return false;
 P.X=FMath::GridSnap(P.X,120.f);P.Y=FMath::GridSnap(P.Y,120.f);
 if(!AKTWStreetDirector::IsRoadPosition(P,-100)||FVector::Dist2D(P,AKTWStreetDirector::BasePosition(U->Faction))>1050){Feedback=TEXT("Deploy inside your base area on a road.");return false;}
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());FNavLocation N;if(!NS||!NS->ProjectPointToNavigation(P+FVector(0,0,100),N,FVector(50,50,200)))return false;
 for(TActorIterator<ACharacter>I(GetWorld());I;++I)if(*I!=U&&FVector::Dist2D(I->GetActorLocation(),N.Location)<100){Feedback=TEXT("Deployment position occupied.");return false;}
 U->SetActorLocation(N.Location+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);U->Lane=SelectedLane;U->LaneEntry=AKTWStreetDirector::Route(U->Lane,U->Faction)[1];Feedback=TEXT("Deployment updated.");return true;
}
void AKTWMatch::StartBattle(){if(Phase!=EKTWMatchPhase::Preparation||UGameplayStatics::IsGamePaused(GetWorld()))return;
 // Computer opening is committed before the player confirms, using the same public purchase rules.
 Phase=EKTWMatchPhase::Battle;Refundable.Empty();BattleSeconds=IncomeRemainder=0;
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I){I->bAutoPatrol=true;I->bEnableCombat=true;}
 Feedback=TEXT("Battle started. Scout gold bags. Convoys are announced 30s early; diamond arrives at 04:30.");
}
void AKTWMatch::FinishBattle(){if(Phase==EKTWMatchPhase::Finished)return;Phase=EKTWMatchPhase::Finished;Queue.Empty();Refundable.Empty();
 for(TActorIterator<ACharacter>I(GetWorld());I;++I){if(auto* AI=Cast<AAIController>(I->GetController())){AI->StopMovement();if(AI->GetBrainComponent())AI->GetBrainComponent()->StopLogic(TEXT("Match finished"));}I->GetCharacterMovement()->StopMovementImmediately();if(auto* U=Cast<AKTWTestUnit>(*I))U->SetAura(false);}
 Feedback=TEXT("Match finished. Press N to restart.");
}
void AKTWMatch::RegisterHit(AActor* V,int32 Attacker){if(CanAct()&&IsValid(V)&&Attacker>=0&&Attacker<2)HostileHits.Add(V,GetWorld()->GetTimeSeconds());}
void AKTWMatch::AwardDefeat(AActor* V,int32 Team,int32 Price,bool Boss){if(!CanAct()||!Accounts.IsValidIndex(Team)||RewardedDeaths.Contains(V))return;
 const double* Hit=HostileHits.Find(V);if(!Hit||GetWorld()->GetTimeSeconds()-*Hit>10)return;
 RewardedDeaths.Add(V);const int32 Reward=Boss?30:FMath::RoundToInt(Price*.15f);Accounts[1-Team].Cash+=Reward;Accounts[1-Team].BountyIncome+=Reward;
}
void AKTWMatch::NewLife(AActor* U){HostileHits.Remove(U);RewardedDeaths.Remove(U);}
void AKTWMatch::Tick(float D){Super::Tick(D);if(!CanAct()||UGameplayStatics::IsGamePaused(GetWorld()))return;
 const float Step=FMath::Min(D,AKTWStreetWar::Limit(GetWorld())-BattleSeconds);BattleSeconds+=Step;IncomeRemainder+=Step;
 while(IncomeRemainder>=10){IncomeRemainder-=10;for(auto& A:Accounts){A.Cash+=30;A.TimeIncome+=30;}}
 for(int32 Team=0;Team<2;++Team)for(int32 Lane=0;Lane<3;++Lane){for(int32 I=0;I<Queue.Num();++I)if(Queue[I].Faction==Team&&Queue[I].Lane==Lane){Queue[I].Remaining-=Step;if(Queue[I].Remaining<=0){if(SpawnRecruit(Queue[I]))Queue.RemoveAt(I);else if(Team==0)Feedback=TEXT("Spawn blocked; paid unit retained and retrying.");}break;}}
 for(auto It=HostileHits.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 for(auto It=RewardedDeaths.CreateIterator();It;++It)if(!It->IsValid())It.RemoveCurrent();

 // GoldSystem resolves after transport component ticks.
 if(BattleSeconds>=AKTWStreetWar::Limit(GetWorld()) && !AKTWGoldSystem::Find(GetWorld()))ResolveVictory();
}

void AKTWMatch::RunEconomyTests(){
#if !UE_BUILD_SHIPPING
 int32 Failures=0;auto Check=[&](bool OK,const TCHAR* Label){UE_LOG(LogTemp,Display,TEXT("KTW ECON %s: %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)++Failures;};
 Check(Accounts[0].Cash==600&&Accounts[0].Gold==0&&Accounts[1].Cash==600,TEXT("initial independent accounts"));
 const EKTWRole Roles[]={EKTWRole::Runner,EKTWRole::Knife,EKTWRole::Pipe,EKTWRole::Thrower};for(int32 I=0;I<4;++I)Check(Purchase(0,Roles[I],I%3),TEXT("preparation purchase"));
 Check(Accounts[0].Cash==0&&Population(0)==4,TEXT("four prices and free boss population"));
 Check(!Purchase(0,EKTWRole::Runner,0)&&Accounts[0].Cash==0,TEXT("insufficient cash leaves state intact"));
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->UnitRole==EKTWRole::Thrower)Check(Refund(*I),TEXT("thrower refund"));
 AKTWTestUnit* Unit=nullptr;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->UnitRole==EKTWRole::Pipe)Unit=*I;
 Check(Refund(Unit)&&!Refund(Unit)&&Accounts[0].Cash==400,TEXT("refund exactly once"));
 Check(!Purchase(0,EKTWRole::Runner,9),TEXT("invalid lane rejected"));
 Check(!Deploy(nullptr,FVector::ZeroVector),TEXT("invalid deployment rejected"));
 AKTWTestUnit* Runner=nullptr;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->UnitRole==EKTWRole::Runner)Runner=*I;
 Check(Runner&&Deploy(Runner,AKTWStreetDirector::BasePosition(0)+FVector(0,-500,0))&&!Deploy(Runner,FVector::ZeroVector),TEXT("legal deployment and invalid destination"));
 Tick(10);Check(Accounts[0].Cash==400,TEXT("preparation has no income"));StartBattle();
 Tick(10);Check(Accounts[0].Cash==430&&Accounts[1].Cash==630&&Accounts[0].Gold==0,TEXT("ten second cash income only"));
 Check(Purchase(0,EKTWRole::Runner,0)&&Purchase(0,EKTWRole::Runner,0)&&Purchase(0,EKTWRole::Runner,0)&&!Purchase(0,EKTWRole::Runner,0),TEXT("lane queue max three"));
 {const int32 Money=Accounts[0].Cash;const float Remaining=Queue[0].Remaining;UGameplayStatics::SetGamePaused(GetWorld(),true);Tick(10);Check(Accounts[0].Cash==Money&&Queue[0].Remaining==Remaining&&!Purchase(0,EKTWRole::Runner,1),TEXT("pause freezes cash queue and purchase"));UGameplayStatics::SetGamePaused(GetWorld(),false);}
 const int32 Before=Population(0);Tick(2.9f);Check(QueueCount(0,0)==3,TEXT("queue waits three seconds"));Tick(.11f);Check(QueueCount(0,0)==2&&Population(0)==Before,TEXT("spawn retains reserved population"));
 Check(!Refund(Unit),TEXT("battle refunds rejected"));
 int32 Old=Accounts[0].Cash;auto* Victim=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(FVector(200,-200,100)));Victim->Faction=1;Victim->UnitRole=EKTWRole::Knife;Victim->bEnableCombat=false;Victim->FinishSpawning(FTransform(FVector(200,-200,100)));
 Runner->SetActorLocation(FVector(100,100,100),false,nullptr,ETeleportType::TeleportPhysics);FDamageEvent Damage;Victim->TakeDamage(0,Damage,nullptr,Runner);AwardDefeat(Victim,1,120);Check(Accounts[0].Cash==Old,TEXT("zero damage cannot qualify for bounty"));
 Victim->TakeDamage(1000,Damage,nullptr,Runner);AwardDefeat(Victim,1,120);Check(Accounts[0].Cash==Old+18,TEXT("actual defeat cash credited once"));Victim->Destroy();
 Tick(500);for(int32 I=0;I<30&&Population(0)<20;++I){Purchase(0,EKTWRole::Runner,I%3);Tick(3.1f);}
 Old=Accounts[0].Cash;Check(Population(0)==20&&!Purchase(0,EKTWRole::Runner,2)&&Accounts[0].Cash==Old,TEXT("population cap counts active and queued"));
 Check(CashConserved(),TEXT("cash ledger conservation"));FinishBattle();Old=Accounts[0].Cash;Tick(20);Check(!Purchase(0,EKTWRole::Runner,0)&&Accounts[0].Cash==Old&&Queue.IsEmpty(),TEXT("finished freezes income and purchases"));
 UE_LOG(LogTemp,Display,TEXT("KTW ECON COMPLETE failures=%d"),Failures);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
#endif
}

void AKTWMatch::RunRestartProbe(){
#if !UE_BUILD_SHIPPING
 static int32 Runs=0,Failures=0;++Runs;int32 Bosses=0;bool BossesClean=true;
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){++Bosses;BossesClean&=!I->bRecovering&&I->Health==I->MaxHealth&&!I->bLowHealthRetreat;}
 auto* Gold=AKTWGoldSystem::Find(GetWorld());const bool GoldClean=Gold&&Gold->GeneratedGold==0&&Gold->Piles.Num()==6&&Gold->GroundGold()==0&&Gold->CarriedGold()==0&&Gold->GoldConserved();
 const bool Clean=Accounts[0].Cash==600&&Accounts[0].Gold==0&&Accounts[0].TimeIncome==0&&Accounts[0].Purchases==0&&Accounts[1].Cash==140&&Accounts[1].Purchases==460&&Queue.IsEmpty()&&Phase==EKTWMatchPhase::Preparation&&BattleSeconds==0&&Population(0)==0&&Population(1)==4&&Bosses==2&&BossesClean&&CashConserved()&&GoldClean;
 if(!Clean)++Failures;UE_LOG(LogTemp,Display,TEXT("KTW RESTART cycle=%d clean cash units queue boss: %s"),Runs,Clean?TEXT("PASS"):TEXT("FAIL"));
 if(Runs>=10){UE_LOG(LogTemp,Display,TEXT("KTW RESTART COMPLETE cycles=%d failures=%d"),Runs,Failures);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);return;}
 Purchase(0,EKTWRole::Runner,1);StartBattle();Tick(20);if(Gold)Gold->Tick(0);Purchase(0,EKTWRole::Knife,0);
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==0)I->ApplyTestDamage(1000);
 UGameplayStatics::OpenLevel(GetWorld(),FName(TEXT("/Game/KTW/Maps/L_Kowloon_ReferenceV4")));
#endif
}

void AKTWMatch::GoldChanged(int32 Team){
 if(!Accounts.IsValidIndex(Team)||!CanAct())return;
 if(Accounts[Team].Gold<500)HoldStarted[Team]=-1;
 else if(HoldStarted[Team]<0)HoldStarted[Team]=BattleSeconds;
}
float AKTWMatch::HoldRemaining(int32 Team)const{return HoldStarted[Team]<0?15.f:FMath::Max(0.f,15.f-(BattleSeconds-HoldStarted[Team]));}
void AKTWMatch::ResolveVictory(){
 if(!CanAct()||UGameplayStatics::IsGamePaused(GetWorld()))return;
 if(auto* War=AKTWStreetWar::Find(GetWorld())){War->Resolve();if(!CanAct())return;}
 for(int32 T=0;T<2;++T)GoldChanged(T);
 const bool A=Accounts[0].Gold>=500&&HoldRemaining(0)<=.001f;
 const bool B=Accounts[1].Gold>=500&&HoldRemaining(1)<=.001f;
 if(!A&&!B&&BattleSeconds<AKTWStreetWar::Limit(GetWorld()))return;
 if(A!=B)Winner=A?0:1;
 else {Winner=Accounts[0].Gold==Accounts[1].Gold?-1:(Accounts[0].Gold>Accounts[1].Gold?0:1);if(Winner==-1)if(auto* War=AKTWStreetWar::Find(GetWorld()))if(War->Bases[0]&&War->Bases[1]){const float X=War->Bases[0]->Health/War->Bases[0]->MaxHealth,Y=War->Bases[1]->Health/War->Bases[1]->MaxHealth;if(!FMath::IsNearlyEqual(X,Y))Winner=X>Y?0:1;}}
 UKTWSession::Cue(this,Winner==0?TEXT("Win"):TEXT("Lose"));
 VictoryReason=(A||B)?TEXT("500 GOLD HELD FOR 15 SECONDS"):(AKTWStreetWar::Enabled(GetWorld())?TEXT("6 MINUTE GOLD / VAN ADJUDICATION"):TEXT("12 MINUTE GOLD ADJUDICATION"));
 FinishBattle();Feedback=VictoryReason;
}

float AKTWMatch::BoxRemaining(int32 Team)const{return Team>=0&&Team<2?FMath::Max(0.f,float(BoxReadyAt[Team]-GetWorld()->GetTimeSeconds())):0;}
EKTWRole AKTWMatch::BoxResult(float Roll,int32 LowStreak){if(LowStreak>=3)return Roll<.625f?EKTWRole::Pipe:EKTWRole::Thrower;return Roll<.3f?EKTWRole::Runner:Roll<.6f?EKTWRole::Knife:Roll<.85f?EKTWRole::Pipe:EKTWRole::Thrower;}
bool AKTWMatch::OpenBox(int32 Team,int32 Lane){
 if(!Accounts.IsValidIndex(Team)||Lane<0||Lane>2||Phase==EKTWMatchPhase::Finished||UGameplayStatics::IsGamePaused(GetWorld())||BoxRemaining(Team)>0||Accounts[Team].Cash<130){if(Team==0)Feedback=TEXT("BOX unavailable: needs 130 CASH, ready cooldown and active match.");return false;}
 const auto Previous=BoxRandom[Team];const EKTWRole Result=BoxResult(BoxRandom[Team].FRand(),LowBoxes[Team]);bBoxPurchase=true;const bool Success=Purchase(Team,Result,Lane);bBoxPurchase=false;
 if(!Success){BoxRandom[Team]=Previous;return false;}
 LowBoxes[Team]=int32(Result)<=1?LowBoxes[Team]+1:0;BoxReadyAt[Team]=GetWorld()->GetTimeSeconds()+20;
 if(Team==0)Feedback=FString::Printf(TEXT("BOX: %s (130 CASH). Next odds: %s"),Result==EKTWRole::Runner?TEXT("Runner"):Result==EKTWRole::Knife?TEXT("Knife"):Result==EKTWRole::Pipe?TEXT("Pipe"):TEXT("Thrower"),LowBoxes[Team]>=3?TEXT("Pipe 62.5%, Thrower 37.5%"):TEXT("30 / 30 / 25 / 15%"));return true;
}

void AKTWMatch::RunProgressionTests(){
#if !UE_BUILD_SHIPPING
 int32 Fail=0;auto Check=[&](bool OK,const TCHAR* Label){UE_LOG(LogTemp,Display,TEXT("KTW PROGRESSION %s: %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));Fail+=!OK;};
 Check(!OpenBox(0,9)&&Accounts[0].Cash==600,TEXT("invalid box request preserves balance"));
 Check(OpenBox(0,1)&&Accounts[0].Cash==470&&Population(0)==1&&Refundable.Num()==4,TEXT("box pays exactly 130 and cannot refund"));
 Check(!OpenBox(0,1)&&Accounts[0].Cash==470,TEXT("shared twenty second cooldown"));
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0)Check(!Refund(*I)&&Deploy(*I,AKTWStreetDirector::BasePosition(0)+FVector(0,-600,0)),TEXT("boxed unit deploys but cannot refund"));
 int32 Counts[4]={0,0,0,0},High=0;FRandomStream R(1024);for(int32 I=0;I<100000;++I){++Counts[int32(BoxResult(R.FRand(),0))];if(BoxResult(R.FRand(),3)==EKTWRole::Pipe)++High;}
 Check(FMath::Abs(Counts[0]-30000)<1000&&FMath::Abs(Counts[1]-30000)<1000&&FMath::Abs(Counts[2]-25000)<1000&&FMath::Abs(Counts[3]-15000)<1000,TEXT("hundred thousand base draws match public probabilities"));
 Check(FMath::Abs(High-62500)<1000,TEXT("hundred thousand guarantee draws match conditional odds"));
 int32 Streak=0,MaxStreak=0;for(int32 I=0;I<100000;++I){const auto Pick=BoxResult(R.FRand(),Streak);Streak=int32(Pick)<=1?Streak+1:0;MaxStreak=FMath::Max(MaxStreak,Streak);}Check(MaxStreak<=3,TEXT("no fourth consecutive low-price result"));
 BoxReadyAt[0]=0;LowBoxes[0]=3;Check(OpenBox(0,1)&&LowBoxes[0]==0,TEXT("fourth box guarantees high role and resets streak"));
 UGameplayStatics::SetGamePaused(GetWorld(),true);Check(!OpenBox(1,1),TEXT("pause rejects blind recruitment"));UGameplayStatics::SetGamePaused(GetWorld(),false);
 StartBattle();SetActorTickEnabled(false);if(auto* S=AKTWGoldSystem::Find(GetWorld()))S->SetActorTickEnabled(false);
 auto Spawn=[&](int32 Team,EKTWRole Type,FVector P){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->Faction=Team;U->UnitRole=Type;U->bAutoPatrol=false;U->bEnableCombat=false;U->Order=EKTWUnitOrder::Defend;U->FinishSpawning(FTransform(P));return U;};
 auto* A=Spawn(0,EKTWRole::Knife,FVector(-300,300,100));const float HP=A->Stats.MaxHealth,Damage=A->Stats.Damage;A->Health-=20;A->GainExperience(59);Check(A->Level==1,TEXT("no premature promotion"));A->GainExperience(1);Check(A->Level==2&&FMath::IsNearlyEqual(A->Stats.MaxHealth,HP*1.15f)&&FMath::IsNearlyEqual(A->Health,A->Stats.MaxHealth-20)&&A->Stats.Capacity==25,TEXT("level two preserves missing health and adds capacity"));A->GainExperience(100);Check(A->Level==3&&FMath::IsNearlyEqual(A->Stats.Damage,Damage*1.3f)&&A->Stats.Capacity==30,TEXT("level three uses base factor not compounding"));A->Health=0;A->GainExperience(100);Check(A->Health==0,TEXT("experience cannot revive a dead unit"));A->Destroy();
 auto* Killer=Spawn(0,EKTWRole::Knife,FVector(-300,300,100));auto* Victim=Spawn(1,EKTWRole::Runner,FVector(-200,200,100));FDamageEvent E;Victim->TakeDamage(1000,E,nullptr,Killer);Check(FMath::IsNearlyEqual(Killer->Experience,36),TEXT("actual damage only plus twenty defeat experience"));Victim->TakeDamage(1000,E,nullptr,Killer);Check(FMath::IsNearlyEqual(Killer->Experience,36),TEXT("dead victim cannot pay twice"));
 BoxReadyAt[0]=0;const int32 OldCash=Accounts[0].Cash;Check(OpenBox(0,2)&&Accounts[0].Cash==OldCash-130&&Queue.Last().Paid==130,TEXT("battle box reserves identical three second queue"));Check(CashConserved(),TEXT("blind and direct recruitment conserve cash"));
 auto Failures=MakeShared<int32>(Fail);auto* Guard=Spawn(0,EKTWRole::Knife,FVector(-2000,2000,100));Guard->bEnableCombat=true;auto* Foe=Spawn(1,EKTWRole::Runner,FVector(-1800,1800,100));Guard->IssueOrder(EKTWUnitOrder::Move,FVector(-2600,2600,20));
 AKTWBoss* Boss=nullptr;for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==0){Boss=*I;break;}Boss->SetActorLocation(FVector(0,0,110),false,nullptr,ETeleportType::TeleportPhysics);Boss->IssueTask(EKTWBossTask::Sit,FVector(0,0,20),nullptr);auto* NearBoss=Spawn(1,EKTWRole::Runner,FVector(300,-300,100));
 Tick(20);auto* S=AKTWGoldSystem::Find(GetWorld());S->Tick(0);auto* Courier=Spawn(0,EKTWRole::Runner,S->Piles[0]->GetActorLocation()+FVector(0,0,90));Courier->bEnableCombat=true;Courier->Lane=0;auto* Interceptor=Spawn(1,EKTWRole::Pipe,S->Piles[0]->GetActorLocation()+FVector(0,-650,90));Courier->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(S->Piles[0]);
 const TWeakObjectPtr<AKTWTestUnit> WF=Foe,WN=NearBoss,WC=Courier,WI=Interceptor;FTimerHandle Verify;GetWorldTimerManager().SetTimer(Verify,FTimerDelegate::CreateLambda([this,Failures,WF,WN,WC,WI](){auto Verify=[&](bool OK,const TCHAR* Name){*Failures+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW PROGRESSION %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 Verify(!WF.IsValid()||WF->Health<WF->Stats.MaxHealth,TEXT("combat soldier automatically attacks during move"));Verify(!WN.IsValid()||WN->Health<WN->Stats.MaxHealth,TEXT("positioned boss automatically engages nearby enemy"));Verify(WC.IsValid()&&WC->FindComponentByClass<UKTWGoldCarrier>()->Carried>0&&WI.IsValid()&&WI->Health==WI->Stats.MaxHealth,TEXT("runner prioritizes carrying gold over attacking"));FinishBattle();Verify(!OpenBox(0,1),TEXT("finished game rejects box"));UE_LOG(LogTemp,Display,TEXT("KTW PROGRESSION COMPLETE failures=%d"),*Failures);FPlatformMisc::RequestExitWithStatus(false,*Failures?1:0);
 }),4.f,false);
#endif
}
