#include "KTWPrototype.h"
#include "KTWRogue.h"
#include "KTWStreetWar.h"
#include "KTWSession.h"
#include "KTWWorkGuard.h"
#include "KTWHoloHUD.h"
#include "KTWDynamics.h"
#include "KTWPresentation.h"
#include "KTWTactics.h"
#include "Engine/DamageEvents.h"
#include "KTWGeneration.h"
#include "KTWAI.h"
#include "KTWRunnerVisuals.h"
#include "KTWMatch.h"
#include "KTWCommander.h"
#include "KTWCombatAction.h"
#include "KTWGold.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Engine/GameViewportClient.h"

namespace {
float MovementScale(){auto* V=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));return V?FMath::Clamp(V->MovementSpeedMultiplier,.5f,3.f):1.75f;}
void AddBody(ACharacter* C,FName Name,FVector Scale,float Height) {
 auto* M=C->CreateDefaultSubobject<UStaticMeshComponent>(Name);
 M->SetupAttachment(C->GetRootComponent());
 M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 M->SetRelativeLocation(FVector(0,0,Height)); M->SetRelativeScale3D(Scale);
 M->SetCollisionEnabled(ECollisionEnabled::NoCollision); M->SetCanEverAffectNavigation(false);
}
void ApplyFactionMaterial(AActor* Actor,int32 Faction) {
 const TCHAR* Path=Faction==0?TEXT("/Game/KTW/Materials/M_Blue.M_Blue"):TEXT("/Game/KTW/Materials/M_Red.M_Red");
 if(auto* Material=LoadObject<UMaterialInterface>(nullptr,Path)){TArray<UStaticMeshComponent*> Meshes;Actor->GetComponents(Meshes);for(auto* Mesh:Meshes)Mesh->SetMaterial(0,Material);}
}
void SetupCharacter(ACharacter* C,bool Boss,int32 Faction){
 auto* V=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));if(!V)return;
 auto* RunnerUnit=Boss?nullptr:Cast<AKTWTestUnit>(C);auto* RunnerMesh=FKTWRunnerVisuals::ResolveMesh(RunnerUnit);
 const TCHAR* Roles[]={TEXT("Runner"),TEXT("Knife"),TEXT("Pipe"),TEXT("Thrower")};
 const FString UserName=FString::Printf(TEXT("SK_%s_%s"),Faction==0?TEXT("Blue"):TEXT("Red"),Boss?TEXT("Boss"):Roles[int32(RunnerUnit->UnitRole)]);
 auto* UserMesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/KTW/Characters/UserNPC/")+UserName+TEXT(".")+UserName));
 if(UserMesh)RunnerMesh=nullptr;
 auto* Mesh=UserMesh?UserMesh:RunnerMesh?RunnerMesh:(Boss?V->BossMesh.LoadSynchronous():V->SoldierMesh.LoadSynchronous());auto* Anim=V->AnimationClass.LoadSynchronous();if(!Mesh||!Anim)return;
 if(RunnerMesh)if(auto* Original=V->SoldierMesh.LoadSynchronous())C->GetMesh()->SetPhysicsAsset(Original->GetPhysicsAsset(),true);
 C->GetMesh()->SetSkeletalMeshAsset(Mesh);C->GetMesh()->SetRelativeLocation(FVector(0,0,-C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
 const float DisplayScale=(Boss?V->BossScale:V->SoldierScale)*FMath::Clamp(V->OverviewScaleMultiplier,1.f,20.f);
 C->GetMesh()->SetRelativeRotation(V->MeshRotation);C->GetMesh()->SetRelativeScale3D(FVector(DisplayScale));C->GetMesh()->SetAnimInstanceClass(Anim);
 C->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->GetMesh()->SetCanEverAffectNavigation(false);
 TArray<UStaticMeshComponent*> Fallback;C->GetComponents(Fallback);for(auto* M:Fallback)M->SetVisibility(false);
 if(!Boss){if(auto* U=Cast<AKTWTestUnit>(C))if(U->UnitRole!=EKTWRole::Runner){auto* W=NewObject<UStaticMeshComponent>(C,TEXT("HeldWeapon"));C->AddInstanceComponent(W);W->SetupAttachment(C->GetMesh(),TEXT("hand_r"));W->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));W->SetCollisionEnabled(ECollisionEnabled::NoCollision);W->SetCanEverAffectNavigation(false);W->SetRelativeScale3D(U->UnitRole==EKTWRole::Knife?FVector(.45,.035,.025):(U->UnitRole==EKTWRole::Pipe?FVector(.8,.06,.06):FVector(.12,.12,.2)));W->RegisterComponent();}}

 if(UserMesh)return; // Preserve the supplied clothing, skin and faction textures.
 if(RunnerMesh){FKTWRunnerVisuals::Refresh(RunnerUnit);return;}
 const TCHAR* Path=Faction==0?TEXT("/Game/KTW/Materials/M_Blue.M_Blue"):TEXT("/Game/KTW/Materials/M_Red.M_Red");
 if(auto* Material=LoadObject<UMaterialInterface>(nullptr,Path))for(int32 I=0;I<C->GetMesh()->GetNumMaterials();++I)C->GetMesh()->SetMaterial(I,Material);
}
const TCHAR* RoleLabel(EKTWRole R){const TCHAR* Names[]={TEXT("Runner"),TEXT("Knife"),TEXT("Pipe"),TEXT("Thrower")};return Names[int32(R)];}
FColor TeamColor(int32 Team) { return Team==0?FColor(65,165,255):FColor(255,95,70); }
}
AKTWTestUnit::AKTWTestUnit() {
 CreateDefaultSubobject<UKTWWorkGuard>(TEXT("WorkGuard"));
 CreateDefaultSubobject<UKTWCombatAction>(TEXT("CombatAction"));
 CreateDefaultSubobject<UKTWGoldCarrier>(TEXT("GoldCarrier"));
 GetCapsuleComponent()->InitCapsuleSize(36,88);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned; AIControllerClass=AAIController::StaticClass();
 GetCharacterMovement()->bOrientRotationToMovement=true;
 GetCharacterMovement()->bUseRVOAvoidance=true;GetCharacterMovement()->AvoidanceConsiderationRadius=600;
 AddBody(this,TEXT("PlaceholderBody"),FVector(.55,.55,1.3),-15);
}

TArray<FKTWUnitStats> UKTWUnitBalance::Defaults(){
 TArray<FKTWUnitStats> Rows;auto Add=[&](int32 Price,float HP,float D,float I,float S,float A,float R,int32 C){FKTWUnitStats V;V.Price=Price;V.MaxHealth=HP;V.Damage=D;V.Interval=I;V.Speed=S;V.Armor=A;V.Range=R;V.Capacity=C;Rows.Add(V);};
 Add(80,80,8,1,460,0,150,60);Add(120,120,24,.8f,400,0,150,20);Add(180,220,32,1.35f,300,.2f,150,30);Add(220,90,26,1.2f,340,0,1050,20);return Rows;
}
void AKTWTestUnit::BeginPlay(){
 Super::BeginPlay();auto Rows=UKTWUnitBalance::Defaults();if(auto* B=LoadObject<UKTWUnitBalance>(nullptr,TEXT("/Game/KTW/Data/DA_UnitBalance_V4.DA_UnitBalance_V4")))if(B->Units.Num()==4)Rows=B->Units;
 Stats=Rows[int32(UnitRole)];BaseHealth=Stats.MaxHealth;BaseDamage=Stats.Damage;Stats.Speed*=MovementScale();bRunner=UnitRole==EKTWRole::Runner;Health=Stats.MaxHealth;OrderAnchor=GetActorLocation();
 ApplyFactionMaterial(this,Faction);SetupCharacter(this,false,Faction);if(!FKTWRunnerVisuals::IsActive(this)&&!(GetMesh()->GetSkeletalMeshAsset()&&GetMesh()->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("/UserNPC/"))))AKTWStreetLife::Dress(this,false);SetAura(false);GetWorldTimerManager().SetTimer(PatrolTimer,this,&AKTWTestUnit::PatrolStep,.2f,true);
}
void AKTWTestUnit::SetAura(bool Enabled){bAura=Enabled;DamageMultiplier=(!bRunner&&Enabled)?1.1f:1;GetCharacterMovement()->MaxWalkSpeed=Stats.Speed*(bCarryingGold?.85f:1.f)*((bRunner&&Enabled)?1.12f:1.f);}
bool AKTWTestUnit::IssueOrder(EKTWUnitOrder O,FVector P,AActor* Target){
 if(auto* War=AKTWStreetWar::Find(GetWorld()))War->Cancel(this);
 if(auto* M=AKTWMatch::Find(GetWorld()))if(!M->CanAct()||M->BattleSeconds>=AKTWStreetWar::Limit(GetWorld())||UGameplayStatics::IsGamePaused(GetWorld()))return false;
 if(Health<=0||Target==this)return false;
 if(O==EKTWUnitOrder::Escort){auto* Friend=Cast<AKTWTestUnit>(Target);auto* Boss=Cast<AKTWBoss>(Target);if((!Friend&&!Boss)||(Friend&&(Friend->Health<=0||Friend->Faction!=Faction))||(Boss&&(Boss->bRecovering||Boss->Faction!=Faction)))return false;P=Target->GetActorLocation();}
 if(O==EKTWUnitOrder::Attack){if(!FKTWTactics::Hostile(this,Target)||!AKTWStreetWar::Visible(GetWorld(),Target,Faction)||!FKTWTactics::AttackPoint(this,Target,Faction,P))return false;LastSeenEnemy=Target->GetActorLocation();LastSeenAt=GetWorld()->GetTimeSeconds();}
 if(O==EKTWUnitOrder::PickupGear){auto* L=AKTWStreetLife::Find(GetWorld());if(!L||!L->RequestGear(this,Cast<AKTWStreetWeapon>(Target),true))return false;}
 FVector Nav;if(!FKTWTactics::ProjectOrder(this,P,Faction,Nav))return false;
 if(auto* Gold=FindComponentByClass<UKTWGoldCarrier>()){if(O==EKTWUnitOrder::Pickup)return Gold->RequestPickup(Cast<AKTWGoldPile>(Target));if(Gold->DeferOrder(O,Nav,Target))return true;Gold->OnAcceptedOrder(O);}
 if(auto* Action=FindComponentByClass<UKTWCombatAction>())Action->CancelAction();
 if(auto* Guard=FindComponentByClass<UKTWWorkGuard>())Guard->CancelRecovery();if(O==EKTWUnitOrder::Move||O==EKTWUnitOrder::Defend||O==EKTWUnitOrder::Retreat||O==EKTWUnitOrder::Advance)bHasLocalWork=false;
 bLaneDeparturePending=false;GetCharacterMovement()->SetAvoidanceEnabled(true);Order=O;OrderAnchor=Nav;OrderTarget=Target;Enemy.Reset();bAutoPatrol=O==EKTWUnitOrder::Advance;CombatAnchor=GetActorLocation();
 if(O==EKTWUnitOrder::Attack){LastSeenEnemy=Target->GetActorLocation();LastSeenAt=GetWorld()->GetTimeSeconds();}
 if(O==EKTWUnitOrder::Advance){const auto R=AKTWStreetDirector::Route(Lane,Faction);float Best=MAX_flt;for(int32 I=1;I<R.Num();++I){const float D=FVector::DistSquared2D(GetActorLocation(),FMath::ClosestPointOnSegment(GetActorLocation(),R[I-1],R[I]));if(D<Best){Best=D;PatrolIndex=I;}}bPatrolReturning=false;}
 if(GetWorld()->GetTimeSeconds()>=StunnedUntil){if(O==EKTWUnitOrder::Move||O==EKTWUnitOrder::Defend||O==EKTWUnitOrder::Retreat)FKTWTactics::Move(this,O==EKTWUnitOrder::Retreat?AKTWStreetDirector::BasePosition(Faction):Nav,Faction,70,true);else if(O==EKTWUnitOrder::Escort)FKTWTactics::Move(this,Nav,Faction,300,true);}
 return true;
}
float AKTWTestUnit::TakeDamage(float Amount,FDamageEvent const&,AController*,AActor* Causer){
 if(!FKTWTactics::CanDamage(Causer,this))return 0;
 if(auto* M=AKTWMatch::Find(GetWorld())){if(!M->CanAct()||Health<=0||Amount<=0)return 0;if(auto* U=Cast<AKTWTestUnit>(Causer)){if(U->Faction!=Faction)M->RegisterHit(this,U->Faction);}else if(auto* B=Cast<AKTWBoss>(Causer))if(B->Faction!=Faction)M->RegisterHit(this,B->Faction);}
 if(Health<=0||Amount<=0)return 0;float Armor=Stats.Armor;
 if(auto* U=Cast<AKTWTestUnit>(Causer))if(U->UnitRole==EKTWRole::Thrower)Armor*=.5f;
 const float Actual=FMath::Min(Health,float(FMath::Max(1,FMath::RoundToInt(Amount*(1-Armor)))));Health-=Actual;
 if(auto* Attacker=Cast<AKTWTestUnit>(Causer))if(Attacker!=this&&Attacker->Faction!=Faction&&Attacker->Health>0){auto& Credit=DamageCredits.FindOrAdd(Attacker);Credit.RemoveAll([&](const FVector2D& V){return GetWorld()->GetTimeSeconds()-V.Y>10;});Credit.Add(FVector2D(Actual,GetWorld()->GetTimeSeconds()));Attacker->GainExperience(Actual/5.f);}
 if(Health<=0){float Total=0;const double Now=GetWorld()->GetTimeSeconds();for(const auto& Pair:DamageCredits)if(Pair.Key.IsValid()&&Pair.Key->Health>0)for(const auto& Hit:Pair.Value)if(Now-Hit.Y<=10)Total+=Hit.X;if(Total>0)for(const auto& Pair:DamageCredits)if(Pair.Key.IsValid()&&Pair.Key->Health>0){float Share=0;for(const auto& Hit:Pair.Value)if(Now-Hit.Y<=10)Share+=Hit.X;Pair.Key->GainExperience(20*Share/Total);}DamageCredits.Empty();}
 if(auto* Gold=FindComponentByClass<UKTWGoldCarrier>())Gold->Interrupt();
 if(Health<=0){UKTWSession::Cue(this,TEXT("Down"),.65f,GetActorLocation());if(auto* Life=AKTWStreetLife::Find(GetWorld()))if(auto* W=Life->Equipment(this))W->Drop();Experience=0;Level=1;GetWorldTimerManager().ClearTimer(PatrolTimer);SetAura(false);if(auto* AI=Cast<AAIController>(GetController()))AI->StopMovement();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetCharacterMovement()->DisableMovement();
  if(auto* M=AKTWMatch::Find(GetWorld()))M->AwardDefeat(this,Faction,Stats.Price);
  if(auto* Gold=FindComponentByClass<UKTWGoldCarrier>())Gold->Defeated();
  int32 CorpseCount=0;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Health<=0&&*I!=this&&I->GetMesh()->IsSimulatingPhysics())++CorpseCount;
  if(CorpseCount<10&&GetMesh()->GetPhysicsAsset()){GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));GetMesh()->SetSimulatePhysics(true);}SetLifeSpan(8);
 }
 return Actual;
}
bool AKTWTestUnit::CombatStep(){
 if(!bEnableCombat||Health<=0)return false;const double Now=GetWorld()->GetTimeSeconds();auto* AI=Cast<AAIController>(GetController());if(!AI)return false;
 if(Now<StunnedUntil){AI->StopMovement();return true;}if(auto* Action=FindComponentByClass<UKTWCombatAction>())if(Action->bPlaying&&((UnitRole!=EKTWRole::Thrower&&UnitRole!=EKTWRole::Knife)||(AKTWStreetLife::Find(GetWorld())&&AKTWStreetLife::Find(GetWorld())->Busy(this)))){AI->StopMovement();return true;}
 auto Valid=[&](AActor* A){if(A&&Order==EKTWUnitOrder::Escort&&OrderTarget.IsValid()&&FVector::Dist2D(A->GetActorLocation(),OrderTarget->GetActorLocation())>500)return false;if(A&&Order==EKTWUnitOrder::Defend&&FVector::Dist2D(A->GetActorLocation(),OrderAnchor)>800)return false;if(!(Order==EKTWUnitOrder::Attack&&A==OrderTarget.Get())&&!FKTWTactics::CanDamage(this,A))return false;if(auto* U=Cast<AKTWTestUnit>(A))return U!=this&&U->Faction!=Faction&&U->Health>0;if(auto* B=Cast<AKTWBoss>(A))return B->Faction!=Faction&&!B->bRecovering;if(auto* Guard=Cast<AKTWSecurity>(A))return Guard->Health>0;return false;};
 auto Visible=[&](AActor* A){return FKTWTactics::CanSee(this,A,1800);};
 if(Order==EKTWUnitOrder::Attack&&!Valid(OrderTarget.Get())){Order=EKTWUnitOrder::Advance;bAutoPatrol=true;OrderTarget.Reset();}
 if(Order==EKTWUnitOrder::Escort){auto* T=Cast<ACharacter>(OrderTarget.Get());if(!T||FVector::Dist2D(GetActorLocation(),T->GetActorLocation())>900){Enemy.Reset();return false;}}
 if(Order==EKTWUnitOrder::Attack&&Valid(OrderTarget.Get())){if(AKTWStreetWar::Visible(GetWorld(),OrderTarget.Get(),Faction)){LastSeenEnemy=OrderTarget->GetActorLocation();LastSeenAt=Now;}else{Enemy.Reset();if(Now-LastSeenAt<3){FKTWTactics::Move(this,LastSeenEnemy,Faction,90);return true;}Order=EKTWUnitOrder::Advance;OrderTarget.Reset();bAutoPatrol=true;}}
 if(Order==EKTWUnitOrder::Attack&&Valid(OrderTarget.Get())&&(!Visible(OrderTarget.Get())||FVector::Dist2D(GetActorLocation(),OrderTarget->GetActorLocation())>Stats.Range)){FVector Goal;if(FKTWTactics::AttackPoint(this,OrderTarget.Get(),Faction,Goal))FKTWTactics::Move(this,Goal,Faction,FMath::Max(25.f,Stats.Range-80));return true;}
 AActor* Target=Enemy.Get();
 if(Order==EKTWUnitOrder::Attack&&Valid(OrderTarget.Get()))Target=OrderTarget.Get();
 if(!Valid(Target)||FVector::Dist2D(GetActorLocation(),Target->GetActorLocation())>1700||!Visible(Target))Target=nullptr;
 if(!Target){float Best=1400;for(TActorIterator<ACharacter>I(GetWorld());I;++I){if(!Valid(*I))continue;float D=FVector::Dist2D(GetActorLocation(),I->GetActorLocation());if(D<Best&&Visible(*I)){Best=D;Target=*I;}}
  if(Target)CombatAnchor=GetActorLocation();
 }
 if(!Target){GetCharacterMovement()->SetAvoidanceEnabled(true);Enemy.Reset();return false;}
 if(Order==EKTWUnitOrder::Defend&&FVector::Dist2D(OrderAnchor,Target->GetActorLocation())>800){Enemy.Reset();return false;}
 if(Order==EKTWUnitOrder::Escort&&OrderTarget.IsValid()&&FVector::Dist2D(OrderTarget->GetActorLocation(),Target->GetActorLocation())>500){Enemy.Reset();return false;}
 if(Order!=EKTWUnitOrder::Attack&&Order!=EKTWUnitOrder::Escort&&FVector::Dist2D(CombatAnchor,Target->GetActorLocation())>1000){Enemy.Reset();return false;}
 GetCharacterMovement()->SetAvoidanceEnabled(UnitRole==EKTWRole::Thrower||bRunner);
 if(Enemy.Get()!=Target)PipeHits=0;Enemy=Target;const float D=FVector::Dist2D(GetActorLocation(),Target->GetActorLocation());
 if(D>220&&D<=1100&&Now-LastAttack>=Stats.Interval)if(auto* A=FindComponentByClass<UKTWCombatAction>())if(A->ThrowConsumable(Target)){LastAttack=Now;AI->StopMovement();return true;}
 if(bRunner&&Order!=EKTWUnitOrder::Attack&&D>190){AI->MoveToLocation(AKTWStreetDirector::BasePosition(Faction),100,true,true,true,false,FKTWTactics::Filter(Faction),false);return true;}
 if(D>Stats.Range){AI->MoveToActor(Target,FMath::Max(10.f,Stats.Range-110),false,true,false,FKTWTactics::Filter(Faction),false);return true;}
 bool Kiting=false;
 if(UnitRole==EKTWRole::Thrower&&D<700){const FVector Away=(GetActorLocation()-Target->GetActorLocation()).GetSafeNormal2D();auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());FNavLocation N;const FVector P=GetActorLocation()+Away*350;if(NS&&AKTWStreetDirector::IsRoadPosition(P,-100)&&!FKTWTactics::Protected(P,1-Faction)&&NS->ProjectPointToNavigation(P,N,FVector(80,80,200))){AI->MoveToLocation(N.Location,30,false,true,true,false,FKTWTactics::Filter(Faction),false);Kiting=true;}}
 if(!Kiting){if(UnitRole==EKTWRole::Knife)AI->MoveToActor(Target,5,false,true,false,FKTWTactics::Filter(Faction),false);else AI->StopMovement();}if(Now-LastAttack<Stats.Interval)return true;float Counter=1;
 if(auto* U=Cast<AKTWTestUnit>(Target)){
  if((UnitRole==EKTWRole::Knife&&U->UnitRole==EKTWRole::Thrower)||(UnitRole==EKTWRole::Pipe&&U->UnitRole==EKTWRole::Knife)||(UnitRole==EKTWRole::Thrower&&U->UnitRole==EKTWRole::Pipe))Counter=1.3f;
  if(UnitRole==EKTWRole::Knife&&U->UnitRole==EKTWRole::Pipe)Counter=.85f;
 }
 if(auto* Action=FindComponentByClass<UKTWCombatAction>())if(Action->Attack(Target,Stats.Damage*DamageMultiplier*Counter,Stats.Range,UnitRole==EKTWRole::Thrower))LastAttack=Now;
 return true;
}
void AKTWTestUnit::PatrolStep(){
 if(AKTWStreetWar::Busy(this))return;
 if(auto* M=AKTWMatch::Find(GetWorld()))if(!M->CanAct())return;
 if(Health<=0)return;auto* AI=Cast<AAIController>(GetController());if(!AI){SpawnDefaultController();return;}
 if(GetWorld()->GetTimeSeconds()<StunnedUntil){AI->StopMovement();return;}if(auto* Guard=FindComponentByClass<UKTWWorkGuard>())if(Guard->IsDetouring())return;
 if(bLaneDeparturePending&&Order==EKTWUnitOrder::Advance){if(FVector::Dist2D(GetActorLocation(),LaneEntry)<180)bLaneDeparturePending=false;else{if(CombatStep())return;FKTWTactics::Move(this,LaneEntry,Faction,100);return;}}
 if(auto* Gold=FindComponentByClass<UKTWGoldCarrier>())if(Gold->ControlsMovement()&&!(!bRunner&&bEnableCombat&&Order!=EKTWUnitOrder::Move&&FKTWTactics::FindEnemy(this,Faction,1400)))return;
 if(Order!=EKTWUnitOrder::Retreat&&Order!=EKTWUnitOrder::Move&&(!bRunner||Order==EKTWUnitOrder::Attack)&&CombatStep())return;
 if(auto* Life=AKTWStreetLife::Find(GetWorld()))if(Life->Busy(this))return;
 if(Order==EKTWUnitOrder::Defend||Order==EKTWUnitOrder::Move){
  // Tighten path stopping distance so nav projection still lands inside the arrival test.
  if(FVector::Dist2D(GetActorLocation(),OrderAnchor)>90)FKTWTactics::Move(this,OrderAnchor,Faction,Order==EKTWUnitOrder::Move?20.f:65.f);
  else{AI->StopMovement();if(Order==EKTWUnitOrder::Move){auto* C=FindComponentByClass<UKTWGoldCarrier>();if(C)C->OnMoveArrived();if(!UKTWSession::Diagnostic()){BeginLocalWork();}else if(C&&C->Carried>0){Order=EKTWUnitOrder::Advance;bAutoPatrol=true;}else{Order=EKTWUnitOrder::Defend;bAutoPatrol=false;}}}return;
 }
 if(Order==EKTWUnitOrder::Retreat){FKTWTactics::Move(this,AKTWStreetDirector::BasePosition(Faction),Faction,150);return;}
 if(Order==EKTWUnitOrder::Escort){auto* T=Cast<ACharacter>(OrderTarget.Get());bool Alive=IsValid(T);if(auto* U=Cast<AKTWTestUnit>(T))Alive=U->Health>0;if(auto* B=Cast<AKTWBoss>(T))Alive=!B->bRecovering;
  if(Alive){FVector Goal;const FVector Desired=FKTWTactics::EscortPoint(this,T);if(!FKTWTactics::ProjectOrder(this,Desired,Faction,Goal))Goal=T->GetActorLocation();FKTWTactics::Move(this,Goal,Faction,100);return;}Order=EKTWUnitOrder::Advance;bAutoPatrol=true;OrderTarget.Reset();
 }
 if(auto* Life=AKTWStreetLife::Find(GetWorld()))if(Life->TryWeapon(this))return;
 if(LocalWorkStep())return;
 if(!bAutoPatrol)return;auto R=AKTWStreetDirector::Route(Lane,bPatrolReturning?1-Faction:Faction);if(R.Num()>0)R.Last()=UKTWGoldCarrier::VaultPosition(bPatrolReturning?Faction:1-Faction,Lane);if(!R.IsValidIndex(PatrolIndex))return;
 if(FVector::Dist2D(GetActorLocation(),R[PatrolIndex])<180){++PatrolIndex;if(PatrolIndex>=R.Num()){bPatrolReturning=!bPatrolReturning;PatrolIndex=1;return;}}
 if(R.IsValidIndex(PatrolIndex))FKTWTactics::Move(this,R[PatrolIndex],Faction,120);
}
AKTWBoss::AKTWBoss() {
 CreateDefaultSubobject<UKTWWorkGuard>(TEXT("WorkGuard"));
 CreateDefaultSubobject<UKTWCombatAction>(TEXT("CombatAction"));
 GetCapsuleComponent()->InitCapsuleSize(42,96);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned; AIControllerClass=AKTWBossAIController::StaticClass();
 GetCharacterMovement()->MaxWalkSpeed=340; GetCharacterMovement()->bOrientRotationToMovement=true;
 GetCharacterMovement()->bUseRVOAvoidance=true;GetCharacterMovement()->AvoidanceConsiderationRadius=600;
 AddBody(this,TEXT("BossBody"),FVector(.8,.8,1.6),-12);
 AddBody(this,TEXT("BossCrown"),FVector(.95,.95,.15),80);
}
void AKTWBoss::BeginPlay() {
 Super::BeginPlay(); ApplyFactionMaterial(this,Faction);SetupCharacter(this,true,Faction);if(!(GetMesh()->GetSkeletalMeshAsset()&&GetMesh()->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("/UserNPC/"))))AKTWStreetLife::Dress(this,true);Health=MaxHealth; Home=GetActorLocation();TaskAnchor=Home;GetCharacterMovement()->MaxWalkSpeed=340*MovementScale();
 GetWorldTimerManager().SetTimer(LogicTimer,this,&AKTWBoss::UpdateLogic,.2f,true);
}
void AKTWBoss::EndPlay(const EEndPlayReason::Type Reason) { ClearAura();GetWorldTimerManager().ClearTimer(LogicTimer);Super::EndPlay(Reason); }
bool AKTWBoss::PlaceForPreparation(FVector Point){
 auto* M=AKTWMatch::Find(GetWorld());if(!M||M->Phase!=EKTWMatchPhase::Preparation||UGameplayStatics::IsGamePaused(GetWorld())||bRecovering||FVector::Dist2D(Point,AKTWStreetDirector::BasePosition(Faction))>1050)return false;
 Point.X=FMath::GridSnap(Point.X,120.f);Point.Y=FMath::GridSnap(Point.Y,120.f);FVector P;if(!FKTWTactics::ProjectOrder(this,Point,Faction,P))return false;
 for(TActorIterator<ACharacter>I(GetWorld());I;++I)if(*I!=this&&!I->IsHidden()&&FVector::Dist2D(P,I->GetActorLocation())<110)return false;
 SetActorLocation(P+FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),false,nullptr,ETeleportType::TeleportPhysics);Task=EKTWBossTask::Sit;TaskAnchor=P;FollowTarget.Reset();PublishDestination(P,true);return true;
}
bool AKTWBoss::AcceptsAutomaticTask()const{return !bPlayerOrdered&&GetWorld()->GetTimeSeconds()>=AutomaticTaskAfter;}
void AKTWBoss::PublishDestination(FVector P,bool Immediate){
 if(auto* AI=Cast<AAIController>(GetController())){if(auto* BB=AI->GetBlackboardComponent()){BB->SetValueAsVector(TEXT("Goal"),P);BB->SetValueAsBool(TEXT("Recovering"),bRecovering);BB->SetValueAsBool(TEXT("Retreating"),bLowHealthRetreat||Task==EKTWBossTask::Return);BB->SetValueAsInt(TEXT("TaskType"),int32(Task));BB->SetValueAsObject(TEXT("FollowTarget"),FollowTarget.Get());}
 if(!bRecovering)FKTWTactics::Move(this,P,Faction,70,Immediate);}
}
bool AKTWBoss::IssueTask(EKTWBossTask T,FVector Anchor,AActor* Target){
 if(auto* M=AKTWMatch::Find(GetWorld()))if(!M->CanAct()||M->BattleSeconds>=AKTWStreetWar::Limit(GetWorld())||UGameplayStatics::IsGamePaused(GetWorld()))return false;
 if(bRecovering||(bLowHealthRetreat&&!bPlayerOrdered))return false;
 if(T==EKTWBossTask::Follow||T==EKTWBossTask::Escort){auto* U=Cast<AKTWTestUnit>(Target);if(!U||U->Health<=0||U->Faction!=Faction)return false;Anchor=U->GetActorLocation();}
 if(T==EKTWBossTask::Return)Anchor=Home;FVector P;if(T==EKTWBossTask::Attack){if(!FKTWTactics::Hostile(this,Target)||!AKTWStreetWar::Visible(GetWorld(),Target,Faction)||!FKTWTactics::AttackPoint(this,Target,Faction,P))return false;}else if(!FKTWTactics::ProjectOrder(this,Anchor,Faction,P))return false;
 if(auto* Action=FindComponentByClass<UKTWCombatAction>())Action->CancelAction();if(auto* Guard=FindComponentByClass<UKTWWorkGuard>())Guard->CancelRecovery();bHoldPosition=false;NextWorkDecision=GetWorld()->GetTimeSeconds()+2;
 Task=T;TaskAnchor=P;FollowTarget=Target;bRetreatArrived=false;bEscortingCargo=false;if(auto* U=Cast<AKTWTestUnit>(Target)){FollowLane=U->Lane;auto* C=U->FindComponentByClass<UKTWGoldCarrier>();bEscortingCargo=T==EKTWBossTask::Escort&&C&&C->Carried>0;}PublishDestination(P,true);return true;
}
bool AKTWBoss::IssuePlayerOrder(FVector P,AActor* Target){const bool Was=bPlayerOrdered;bPlayerOrdered=true;const EKTWBossTask T=FKTWTactics::Hostile(this,Target)?EKTWBossTask::Attack:Cast<AKTWTestUnit>(Target)?EKTWBossTask::Escort:EKTWBossTask::Sit;if(!IssueTask(T,P,Target)){bPlayerOrdered=Was;return false;}bLowHealthRetreat=false;bHoldPosition=T==EKTWBossTask::Sit;return true;}
float AKTWBoss::TakeDamage(float Amount,FDamageEvent const& E,AController* I,AActor* C) {
 if(!FKTWTactics::CanDamage(C,this))return 0;
 if(auto* M=AKTWMatch::Find(GetWorld())){if(!M->CanAct()||bRecovering||Amount<=0)return 0;if(auto* U=Cast<AKTWTestUnit>(C))if(U->Faction!=Faction)M->RegisterHit(this,U->Faction);}
 if(bRecovering||Amount<=0) return 0;
 float EffectiveArmor=Armor;if(auto* U=Cast<AKTWTestUnit>(C))if(U->UnitRole==EKTWRole::Thrower)EffectiveArmor*=.5f;
 const float Actual=FMath::Min(Health,Amount*(1-FMath::Clamp(EffectiveArmor,0.f,.95f)));
 Health-=Actual;LastHit=GetWorld()->GetTimeSeconds();
 if(Health<=0) EnterRecovery();
 return Actual;
}
void AKTWBoss::ReadyForFinalPhase(){if(bRecovering){RecoveryUntil=GetWorld()->GetTimeSeconds();UpdateLogic();}}
void AKTWBoss::ApplyTestDamage(float Raw) { FDamageEvent E;TakeDamage(Raw,E,nullptr,nullptr); }
void AKTWBoss::ClearAura() {
 for(auto& Pair:AuraLastSeen) if(auto* U=Pair.Key.Get()) U->SetAura(false);
 AuraLastSeen.Empty();
}
void AKTWBoss::EnterRecovery() {
 if(bRecovering) return;
 if(auto* War=AKTWStreetWar::Find(GetWorld()))War->BossDown(Faction);
 if(auto* M=AKTWMatch::Find(GetWorld()))M->AwardDefeat(this,Faction,0,true);
 bPlayerOrdered=false;bRecovering=true;RecoveryUntil=GetWorld()->GetTimeSeconds()+RecoverySeconds;RecoveryRemaining=RecoverySeconds;
 ClearAura(); if(auto* AI=Cast<AAIController>(GetController())){AI->StopMovement();if(auto* BB=AI->GetBlackboardComponent())BB->SetValueAsBool(TEXT("Recovering"),true);}
 SetActorHiddenInGame(true);SetActorEnableCollision(false);GetCharacterMovement()->DisableMovement();FollowTarget.Reset();
 UE_LOG(LogTemp,Display,TEXT("KTW Boss faction=%d down; recovery %.1fs. Cash bounty uses the match ledger."),Faction,RecoverySeconds);
}
void AKTWBoss::UpdateLogic() {
 if(auto* M=AKTWMatch::Find(GetWorld()))if(!M->CanAct()){ClearAura();return;}
 const double Now=GetWorld()->GetTimeSeconds();
 if(bRecovering) {
  RecoveryRemaining=FMath::Max(0.f,float(RecoveryUntil-Now));
  if(RecoveryRemaining>0) return;
  SetActorLocation(Home,false,nullptr,ETeleportType::TeleportPhysics);Health=MaxHealth;
  SetActorHiddenInGame(false);SetActorEnableCollision(true);GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  bRecovering=false;bLowHealthRetreat=false;Task=EKTWBossTask::Sit;TaskAnchor=Home;bRetreatArrived=false;AutomaticTaskAfter=Now+5;
  if(auto* M=AKTWMatch::Find(GetWorld()))M->NewLife(this);
 }
 if(Health<MaxHealth*.3f&&!bLowHealthRetreat&&!bPlayerOrdered) { bLowHealthRetreat=true;Task=EKTWBossTask::Return;TaskAnchor=Home;FollowTarget.Reset(); }
 const FVector Here=GetActorLocation();
 if(FVector::Dist2D(Here,Home)<180 && Now-LastHit>=5) {
  Health=FMath::Min(MaxHealth,Health+MaxHealth*.04f*.2f);
  if(bLowHealthRetreat&&Health>=MaxHealth*.8f){bLowHealthRetreat=false;Task=EKTWBossTask::Sit;TaskAnchor=Home;AutomaticTaskAfter=Now+5;}
 }
 if(Task==EKTWBossTask::Follow||Task==EKTWBossTask::Escort){auto* U=Cast<AKTWTestUnit>(FollowTarget.Get());auto* C=U?U->FindComponentByClass<UKTWGoldCarrier>():nullptr;
  if(C&&C->Carried>0&&Task==EKTWBossTask::Escort)bEscortingCargo=true;
  if(!U||U->Health<=0||(Task==EKTWBossTask::Escort&&bEscortingCargo&&C&&C->Carried==0)){FollowTarget.Reset();Task=EKTWBossTask::Sit;TaskAnchor=UKTWGoldCarrier::VaultPosition(Faction,FollowLane);bEscortingCargo=false;}
 }
 if(Task==EKTWBossTask::Return&&!bLowHealthRetreat&&FVector::Dist2D(Here,Home)<180&&Health>=MaxHealth*.8f&&!UKTWSession::Diagnostic()){Task=EKTWBossTask::Sit;TaskAnchor=Home;NextWorkDecision=Now+2;}
 ChooseAutomaticWork();
 FVector Destination=TaskAnchor;
 if(Task==EKTWBossTask::Follow||Task==EKTWBossTask::Escort) {
  if(auto* Target=FollowTarget.Get()) {FVector P;const FVector Desired=FKTWTactics::EscortPoint(this,Cast<ACharacter>(Target),350);Destination=FKTWTactics::ProjectOrder(this,Desired,Faction,P)?P:Target->GetActorLocation();}
  else {Task=EKTWBossTask::Sit;TaskAnchor=Here;Destination=Here;}
 }
 ACharacter* EnemyTarget=nullptr;AActor* Forced=Task==EKTWBossTask::Attack?FollowTarget.Get():nullptr;float AttackRange=150.f;
 if(Task==EKTWBossTask::Attack){if(!FKTWTactics::Hostile(this,Forced)||!AKTWStreetWar::Visible(GetWorld(),Forced,Faction)){FollowTarget.Reset();Task=EKTWBossTask::Sit;TaskAnchor=Here;bHoldPosition=bPlayerOrdered;Forced=nullptr;}else{if(auto* Base=Cast<AKTWFieldObject>(Forced))AttackRange=Base->Body->Bounds.BoxExtent.Size2D()+220.f;FVector P;if(FKTWTactics::AttackPoint(this,Forced,Faction,P))Destination=P;}}
 if(Task!=EKTWBossTask::Attack&&!(bPlayerOrdered&&FVector::Dist2D(Here,TaskAnchor)>170)&&Task!=EKTWBossTask::Return&&!bLowHealthRetreat&&!FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"))){float Best=1100;const FVector Leash=(Task==EKTWBossTask::Follow||Task==EKTWBossTask::Escort)&&FollowTarget.IsValid()?FollowTarget->GetActorLocation():TaskAnchor;
  for(TActorIterator<ACharacter>I(GetWorld());I;++I){bool Hostile=false;if(auto* U=Cast<AKTWTestUnit>(*I))Hostile=U->Faction!=Faction&&U->Health>0;if(auto* B=Cast<AKTWBoss>(*I))Hostile=B!=this&&B->Faction!=Faction&&!B->bRecovering;if(!Hostile||!FKTWTactics::CanDamage(this,*I)||FVector::Dist2D(I->GetActorLocation(),Leash)>800||!FKTWTactics::CanSee(this,*I,1100))continue;const float Range=FVector::Dist2D(Here,I->GetActorLocation());if(Range<Best){Best=Range;EnemyTarget=*I;Destination=Range>125?I->GetActorLocation():Here;}}
 }
 auto* Action=FindComponentByClass<UKTWCombatAction>();auto* Guard=FindComponentByClass<UKTWWorkGuard>();if(Action&&Action->bPlaying)Destination=Here;if(!Guard||!Guard->IsDetouring())PublishDestination(Destination);
 if(Forced&&FKTWTactics::CanDamage(this,Forced)&&FKTWTactics::CanSee(this,Forced,AttackRange+10)&&FVector::Dist2D(Here,Forced->GetActorLocation())<=AttackRange&&Action&&Now-LastBossAttack>=1.2)if(Action->Attack(Forced,20,AttackRange,false))LastBossAttack=Now;
 if(EnemyTarget&&Now-LastBossAttack>=1.2&&FVector::Dist2D(Here,EnemyTarget->GetActorLocation())<=150&&Action)if(Action->Attack(EnemyTarget,20,150,false))LastBossAttack=Now;
 for(TActorIterator<AKTWTestUnit> It(GetWorld());It;++It) {
  auto* U=*It; if(U->Faction!=Faction||U->Health<=0) continue;
  FHitResult Block;FCollisionQueryParams Q;Q.AddIgnoredActor(this);Q.AddIgnoredActor(U);
  const bool Occluded=GetWorld()->LineTraceSingleByChannel(Block,Here,U->GetActorLocation(),ECC_Visibility,Q);
  const bool Inside=FVector::Dist2D(Here,U->GetActorLocation())<=AuraRadius&&!Occluded;
  if(Inside){AuraLastSeen.FindOrAdd(U)=Now;U->SetAura(true);
   U->DamageMultiplier=U->bRunner?1.f:1.f+CombatDamageBonus;
   if(U->bRunner) U->GetCharacterMovement()->MaxWalkSpeed=U->Stats.Speed*(U->bCarryingGold?.85f:1.f)*(1+RunnerSpeedBonus);
  }
 }
 for(auto It=AuraLastSeen.CreateIterator();It;++It) if(!It.Key().IsValid()||It.Key()->Health<=0||Now-It.Value()>1) {if(auto* U=It.Key().Get())U->SetAura(false);It.RemoveCurrent();}
 if(AKTWStreetWar::Visible(GetWorld(),this))DrawDebugCircle(GetWorld(),Here-FVector(0,0,85),AuraRadius,64,TeamColor(Faction),false,.21f,0,4,FVector(1,0,0),FVector(0,1,0),false);
 for(auto& Pair:AuraLastSeen) if(auto* U=Pair.Key.Get()) if(AKTWStreetWar::Visible(GetWorld(),this)&&AKTWStreetWar::Visible(GetWorld(),U))DrawDebugLine(GetWorld(),Here,U->GetActorLocation(),TeamColor(Faction),false,.21f,0,2);
}
AKTWPrototypeController::AKTWPrototypeController(){bShowMouseCursor=true;bEnableClickEvents=true;bShouldPerformFullTickWhenPaused=true;PrimaryActorTick.bTickEvenWhenPaused=true;}
bool AKTWPrototypeController::SelectLaneKey(FKey Key){auto* M=AKTWMatch::Find(GetWorld());if(!M||M->Phase==EKTWMatchPhase::Finished||UGameplayStatics::IsGamePaused(GetWorld()))return false;int32 L=Key==EKeys::Z?0:Key==EKeys::X?1:Key==EKeys::C?2:-1;if(L<0)return false;M->SelectedLane=L;M->Feedback=FString::Printf(TEXT("Recruit lane: %s. New units march through this lane before automatic tasks."),L==0?TEXT("UPPER"):L==1?TEXT("MIDDLE"):TEXT("LOWER"));return true;}
bool AKTWPrototypeController::HandleRecruitClick(float X,float Y,int32 Width,int32 Height){
 if(auto* H=Cast<AKTWHoloHUD>(GetHUD()))if(H->HandleHudClick(X,Y))return true;
 auto* M=AKTWMatch::Find(GetWorld());if(!M)return false;
 if(X>=12&&X<=332&&Y>=Height-160&&Y<Height-108){M->OpenBox(0,M->SelectedLane);return true;}
 if(X<Width-266||X>Width-16)return false;const float V=130+(Y-130)/RecruitScale(Height);
 if(V>=135&&V<175){const int32 L=FMath::Clamp(int32((X-(Width-270))/85),0,2);SelectLaneKey(L==0?EKeys::Z:L==1?EKeys::X:EKeys::C);return true;}
 for(int32 Row=0;Row<7;++Row){const float Top=175+Row*50;if(V<Top||V>=Top+46)continue;
  if(Row<4)M->Purchase(0,EKTWRole(Row),M->SelectedLane);
  else if(Row==4)M->OpenBox(0,M->SelectedLane);else if(Row==5)M->StartBattle();else{for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U))M->Refund(U);SelectedUnits.Empty();SelectedUnit=nullptr;}return true;
 }return false;
}
void AKTWPrototypeController::PlayerTick(float D) {
 Super::PlayerTick(D);
 if(auto* S=UKTWSession::Get(this))if(S->Handle(this)){if(auto* Rogue=AKTWRogueRun::Find(GetWorld()))Rogue->Selecting=false;if(auto* Cam=Cast<AKTWCameraPawn>(GetPawn())){Cam->SetLookCaptured(false);Cam->bRightPending=false;}bSelecting=false;return;}
 if(auto* Rogue=AKTWRogueRun::Find(GetWorld())){Rogue->Handle(this);return;}
 auto* StreetCamera=Cast<AKTWCameraPawn>(GetPawn());
 const bool StreetCommand=StreetCamera?StreetCamera->UpdateLookGesture(D):false;
 if(StreetCamera&&(StreetCamera->bLookCaptured||StreetCamera->bRightPending)){bSelecting=false;return;}
 auto* Match=AKTWMatch::Find(GetWorld());
 if(Match){
  if(WasInputKeyJustPressed(EKeys::P))UGameplayStatics::SetGamePaused(GetWorld(),!UGameplayStatics::IsGamePaused(GetWorld()));
  if(WasInputKeyJustPressed(EKeys::N)){UGameplayStatics::SetGamePaused(GetWorld(),false);UGameplayStatics::OpenLevel(GetWorld(),FName(TEXT("/Game/KTW/Maps/L_Kowloon_ReferenceV4")));return;}
  if(UGameplayStatics::IsGamePaused(GetWorld()))return;
  if(WasInputKeyJustPressed(EKeys::SpaceBar))Match->StartBattle();
  if(WasInputKeyJustPressed(EKeys::Six))Match->OpenBox(0,Match->SelectedLane);
  for(const FKey K:{EKeys::Z,EKeys::X,EKeys::C})if(WasInputKeyJustPressed(K))SelectLaneKey(K);
  const FKey BuyKeys[]={EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five};for(int32 I=0;I<4;++I)if(WasInputKeyJustPressed(BuyKeys[I]))Match->Purchase(0,EKTWRole(I),Match->SelectedLane);
  if(WasInputKeyJustPressed(EKeys::BackSpace)){for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U))Match->Refund(U);SelectedUnits.Empty();SelectedUnit=nullptr;}
  if(WasInputKeyJustPressed(EKeys::LeftMouseButton)){float X,Y;GetMousePosition(X,Y);int32 SX,SY;GetViewportSize(SX,SY);if(HandleRecruitClick(X,Y,SX,SY))bUIConsumed=true;}
  if(bUIConsumed){if(WasInputKeyJustReleased(EKeys::LeftMouseButton))bUIConsumed=false;return;}
 }

 if(IsInputKeyDown(EKeys::MiddleMouseButton)){bSelecting=false;return;}
 if(WasInputKeyJustPressed(EKeys::One)){for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==0){SelectedBoss=*I;SelectedUnit=nullptr;SelectedUnits.Empty();}}
 const bool Tactical=!StreetCamera||!StreetCamera->bStreetView;bool TacticalCommand=false;
 if((WasInputKeyJustPressed(EKeys::LeftMouseButton)||(Tactical&&WasInputKeyJustPressed(EKeys::RightMouseButton)))&&!bSelecting){float X,Y;if(GetMousePosition(X,Y)&&IsWorldCursor(FVector2D(X,Y))){SelectionStart=FVector2D(X,Y);bSelecting=true;bRightSelecting=WasInputKeyJustPressed(EKeys::RightMouseButton);}}
 if(bSelecting&&WasInputKeyJustReleased(bRightSelecting?EKeys::RightMouseButton:EKeys::LeftMouseButton)){
  bSelecting=false;float X,Y;if(GetMousePosition(X,Y)&&IsWorldCursor(FVector2D(X,Y))){const bool Add=IsInputKeyDown(EKeys::LeftShift)||IsInputKeyDown(EKeys::RightShift);const bool ForceGround=bRightSelecting&&(IsInputKeyDown(EKeys::LeftAlt)||IsInputKeyDown(EKeys::RightAlt))&&FVector2D::Distance(SelectionStart,FVector2D(X,Y))<=12;TacticalCommand=ForceGround||(!SelectAtScreen(SelectionStart,FVector2D(X,Y),Add,bRightSelecting)&&bRightSelecting);}
 }
 if(Tactical?TacticalCommand:StreetCommand){
  float MX,MY;GetMousePosition(MX,MY);int32 SX,SY;GetViewportSize(SX,SY);if(MY<120||(MX>=SX-270&&MY>=130&&MY<640))return;
  FVector Ground=FVector::ZeroVector;const bool HasGround=GroundAtCursor(Ground);
  AActor* Context=nullptr;const bool GroundOnly=IsInputKeyDown(EKeys::LeftAlt)||IsInputKeyDown(EKeys::RightAlt);
  if(!GroundOnly){Context=PickCharacterAt(FVector2D(MX,MY));float Best=18;
   if(!Context&&SelectedUnits.Num()>0){Best=18;for(TActorIterator<AKTWGoldPile>I(GetWorld());I;++I)if(I->Amount>0){FVector2D S;if(ProjectWorldLocationToScreen(I->GetActorLocation()+FVector(0,0,70),S)){const float D2=FVector2D::Distance(S,FVector2D(MX,MY));if(D2<Best){Best=D2;Context=*I;}}}}
   if(!Context&&SelectedUnits.Num()>0)if(auto* L=AKTWStreetLife::Find(GetWorld()))for(AKTWStreetWeapon* W:L->GroundWeapons)if(IsValid(W)&&!W->Equipped){FVector2D S;if(ProjectWorldLocationToScreen(W->GetActorLocation()+FVector(0,0,130),S)&&FVector2D::Distance(S,FVector2D(MX,MY))<24){Context=W;break;}}
   if(!Context&&SelectedUnits.Num()>0&&Match&&Match->CanAct())for(int32 Lane=0;Lane<3;++Lane){FVector2D S;if(ProjectWorldLocationToScreen(UKTWGoldCarrier::VaultPosition(1,Lane),S)&&FVector2D::Distance(S,FVector2D(MX,MY))<18){int32 Accepted=0;for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U)&&U->Health>0){const int32 Previous=U->Lane;U->Lane=Lane;auto* C=U->FindComponentByClass<UKTWGoldCarrier>();if(C&&C->RequestRaid())++Accepted;else U->Lane=Previous;}if(SelectedBoss)SelectedBoss->IssueTask(EKTWBossTask::Sit,Ground,nullptr);Match->Feedback=FString::Printf(TEXT("Raid accepted: %d. Clear defenders, channel 4s, then deliver home."),Accepted);return;}}
  }
  if(!Context&&!GroundOnly)Context=PickFieldAt(FVector2D(MX,MY));
  if(!Context&&!HasGround){if(Match)Match->Feedback=TEXT("Aim at a visible road or unit; the sky is not a move target.");return;}
  CommandAtWorld(Ground,Context);
 }

 for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U)&&U->Health>0){
  if(WasInputKeyJustPressed(EKeys::V))if(auto* Cargo=U->FindComponentByClass<UKTWGoldCarrier>())Cargo->RequestRaid();
  if(WasInputKeyJustPressed(EKeys::F))U->IssueOrder(EKTWUnitOrder::Defend,U->GetActorLocation());
  if(WasInputKeyJustPressed(EKeys::G))U->IssueOrder(EKTWUnitOrder::Advance,U->GetActorLocation());
  if(WasInputKeyJustPressed(EKeys::B))U->IssueOrder(EKTWUnitOrder::Retreat,AKTWStreetDirector::BasePosition(0));
  DrawDebugCircle(GetWorld(),U->GetActorLocation()-FVector(0,0,80),70,24,FColor::Green,false,0,0,3,FVector(1,0,0),FVector(0,1,0),false);
 }
 if(WasInputKeyJustPressed(EKeys::H)&&SelectedBoss){AKTWTestUnit* Best=nullptr;float Score=-MAX_flt;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==SelectedBoss->Faction&&I->bRunner&&I->Health>0){auto* C=I->FindComponentByClass<UKTWGoldCarrier>();const float Value=(C?C->Carried*1000.f:0)-FVector::Dist2D(I->GetActorLocation(),SelectedBoss->GetActorLocation());if(Value>Score){Score=Value;Best=*I;}}if(Best)SelectedBoss->IssueTask(EKTWBossTask::Escort,Best->GetActorLocation(),Best);}
 if(WasInputKeyJustPressed(EKeys::G)&&SelectedBoss){AKTWTestUnit* Target=IsValid(SelectedUnit)?SelectedUnit.Get():nullptr;if(!Target){float Best=MAX_flt;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->Health>0&&!I->bRunner){const float D2=FVector::DistSquared2D(I->GetActorLocation(),SelectedBoss->GetActorLocation());if(D2<Best){Best=D2;Target=*I;}}}if(Target)SelectedBoss->IssueTask(EKTWBossTask::Follow,Target->GetActorLocation(),Target);}
 if(WasInputKeyJustPressed(EKeys::F)&&SelectedBoss){SelectedBoss->IssueTask(EKTWBossTask::Sit,SelectedBoss->GetActorLocation(),nullptr);SelectedBoss->bHoldPosition=true;}

 if(WasInputKeyJustPressed(EKeys::B)&&SelectedBoss)SelectedBoss->IssueTask(EKTWBossTask::Return,FVector::ZeroVector,nullptr);
 if(!Match&&WasInputKeyJustPressed(EKeys::K)&&SelectedBoss)SelectedBoss->ApplyTestDamage(1000);
 if(!Match&&WasInputKeyJustPressed(EKeys::J)&&SelectedBoss)SelectedBoss->ApplyTestDamage(260);
 if(!Match&&WasInputKeyJustPressed(EKeys::R))for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)I->NextSeed();
}
bool AKTWPrototypeController::IsWorldCursor(FVector2D P)const{int32 W=0,H=0;GetViewportSize(W,H);return P.Y>125&&P.Y<H-85&&P.X<W-280&&P.X>=0&&!(P.X<335&&P.Y<450);}
bool AKTWPrototypeController::SelectAtScreen(FVector2D Start,FVector2D End,bool Add,bool OnlyFriendlyClick){
 const bool Drag=FVector2D::Distance(Start,End)>12;ACharacter* Picked=Drag?nullptr:PickCharacterAt(End);
 auto Friendly=[](const ACharacter* C){if(auto* U=Cast<AKTWTestUnit>(C))return U->Faction==0&&U->Health>0;if(auto* B=Cast<AKTWBoss>(C))return B->Faction==0&&!B->bRecovering;return false;};
 if(!Drag&&OnlyFriendlyClick&&!Friendly(Picked))return false;
 if(!Add){SelectedBoss=nullptr;SelectedUnit=nullptr;SelectedUnits.Empty();}
 if(Drag){FBox2D Box(ForceInit);Box+=Start;Box+=End;for(TActorIterator<ACharacter>I(GetWorld());I;++I)if(Friendly(*I)){FVector2D Foot,Head;if(ProjectWorldLocationToScreen(I->GetActorLocation(),Foot)&&ProjectWorldLocationToScreen(I->GetMesh()->GetSocketLocation(TEXT("head")),Head)){FBox2D Body(ForceInit);Body+=Foot;Body+=Head;if(Box.Intersect(Body.ExpandBy(8))){if(auto* U=Cast<AKTWTestUnit>(*I))SelectedUnits.AddUnique(U);else SelectedBoss=Cast<AKTWBoss>(*I);}}}}
 else if(Friendly(Picked)){if(auto* U=Cast<AKTWTestUnit>(Picked))SelectedUnits.AddUnique(U);else SelectedBoss=Cast<AKTWBoss>(Picked);}
 SelectedUnit=SelectedUnits.Num()?SelectedUnits[0]:nullptr;return true;
}
bool AKTWPrototypeController::CommandAtWorld(FVector Point,AActor* Context){
 auto* M=AKTWMatch::Find(GetWorld());if(M&&UGameplayStatics::IsGamePaused(GetWorld()))return false;
 if(M&&M->Phase==EKTWMatchPhase::Preparation){int32 N=0,Slot=0;if(IsValid(SelectedBoss)){N+=SelectedBoss->PlaceForPreparation(Point)?1:0;++Slot;}for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U)){const FVector Offset=Slot?FVector(((Slot+1)%3-1)*120,((Slot+1)/3)*120,0):FVector::ZeroVector;N+=M->Deploy(U,Point+Offset)?1:0;++Slot;}M->Feedback=N?TEXT("Preparation placement updated inside own base."):TEXT("Deploy on a free road inside your own base area.");return N>0;}
 if(M&&!M->CanAct())return false;
 if(auto* Object=Cast<AKTWFieldObject>(Context))if(auto* War=AKTWStreetWar::Find(GetWorld())){int32 Accepted=0;if(IsValid(SelectedBoss)&&Object->Kind==EKTWFieldKind::Base&&Object->Faction!=SelectedBoss->Faction)Accepted+=SelectedBoss->IssuePlayerOrder(Object->GetActorLocation(),Object)?1:0;for(auto U:SelectedUnits)if(War->Interact(U,Object))++Accepted;if(M)M->Feedback=Accepted?TEXT("Field order accepted: approach, interact; new orders cancel hiding."):TEXT("Cannot interact: check cargo, capacity or target.");return Accepted>0;}
 if(Context==SelectedBoss||(Cast<AKTWTestUnit>(Context)&&SelectedUnits.Contains(Cast<AKTWTestUnit>(Context))))Context=nullptr;
 const FVector Destination=IsValid(Context)?Context->GetActorLocation():Point;int32 Accepted=0,Total=0;
 if(IsValid(SelectedBoss)){++Total;Accepted+=SelectedBoss->IssuePlayerOrder(Destination,Context)?1:0;}
 int32 Slot=0;FVector Center=FVector::ZeroVector;int32 Live=0;for(AKTWTestUnit* V:SelectedUnits)if(IsValid(V)&&V->Health>0){Center+=V->GetActorLocation();++Live;}if(Live)Center/=Live;const FVector Forward=(Destination-Center).GetSafeNormal2D(),Side(-Forward.Y,Forward.X,0);
 for(AKTWTestUnit* U:SelectedUnits)if(IsValid(U)&&U->Health>0){++Total;FVector UnitGoal=Destination;if(!Context&&Live>1){const int32 Columns=FMath::Min(3,Live);const float Offset=(Slot%Columns-(Columns-1)*.5f)*160;FVector Candidate=Destination+Side*Offset-Forward*(Slot/Columns)*160;if(AKTWStreetDirector::IsRoadPosition(Candidate,-80)&&!FKTWTactics::Protected(Candidate,1-U->Faction))UnitGoal=Candidate;++Slot;}EKTWUnitOrder O=EKTWUnitOrder::Move;if(Cast<AKTWStreetWeapon>(Context))O=EKTWUnitOrder::PickupGear;else if(Cast<AKTWGoldPile>(Context))O=EKTWUnitOrder::Pickup;else if(auto* C=Cast<ACharacter>(Context)){int32 Team=-1;if(auto* V=Cast<AKTWTestUnit>(C))Team=V->Faction;if(auto* B=Cast<AKTWBoss>(C))Team=B->Faction;if(Cast<AKTWSecurity>(C))Team=2;if(Team>=0)O=Team==U->Faction?EKTWUnitOrder::Escort:EKTWUnitOrder::Attack;}Accepted+=U->IssueOrder(O,UnitGoal,Context)?1:0;}
 if(Accepted>0){LastCommandPoint=Destination;LastCommandTime=GetWorld()->GetTimeSeconds();AKTWCommandMarker::Show(GetWorld(),Destination,Cast<ACharacter>(Context)?1:0);}
 if(M)M->Feedback=Accepted?FString::Printf(TEXT("Command accepted %d/%d. New move replaces old goal; units resume work on arrival."),Accepted,Total):TEXT("Command rejected: select your units; use a reachable road outside enemy safe zone. Recovering bosses cannot move.");
 return Accepted>0;
}
void AKTWPrototypeHUD::DrawHUD() {

 Super::DrawHUD();if(!Canvas)return;if(auto* W=AKTWWeather::Find(GetWorld()))W->Draw(this);if(auto* S=UKTWSession::Get(this))if(S->Draw(this))return;float Y=18;
 auto Line=[&](const FString& T,FColor C=FColor::White){DrawText(T,C,20,Y,nullptr,1.f);Y+=23;};
 auto* Match=AKTWMatch::Find(GetWorld());
 if(Match){auto* Gold=AKTWGoldSystem::Find(GetWorld());
  const float Width=FMath::Min(430.f,(Canvas->SizeX-36)/2);
  for(int32 Team=0;Team<2;++Team){const float BX=Team?Canvas->SizeX-Width-12:12;
   DrawRect(FLinearColor(.015f,.025f,.04f,.95f),BX,12,Width,106);
   DrawRect(FLinearColor(TeamColor(Team)),BX,12,5,106);
   int32 Queued=0;for(const auto& E:Match->Queue)if(E.Faction==Team)++Queued;
   DrawText(Team?TEXT("RED / COMPUTER"):TEXT("BLUE / YOUR GANG"),TeamColor(Team),BX+16,20,nullptr,1.05f);
   DrawText(FString::Printf(TEXT("CASH %d    GOLD %d / 500"),Match->Accounts[Team].Cash,Match->Accounts[Team].Gold),FColor::White,BX+16,45,nullptr,1.25f);
   DrawText(FString::Printf(TEXT("UNITS %d + %d queued / 20   CARRY %d"),Match->Population(Team)-Queued,Queued,Gold?Gold->CarriedGold(Team):0),FColor(190,205,220),BX+16,72,nullptr,.9f);
   DrawRect(FLinearColor(.13f,.15f,.18f),BX+16,99,Width-32,7);
   const float Fraction=Match->Accounts[Team].Gold>=500?1-Match->HoldRemaining(Team)/15.f:FMath::Clamp(Match->Accounts[Team].Gold/500.f,0.f,1.f);
   DrawRect(Match->Accounts[Team].Gold>=500?FLinearColor::Yellow:FLinearColor(TeamColor(Team)),BX+16,99,(Width-32)*Fraction,7);
   if(Match->Accounts[Team].Gold>=500)DrawText(FString::Printf(TEXT("HOLD %.1fs"),Match->HoldRemaining(Team)),FColor::Yellow,BX+Width-110,23,nullptr,.8f);
  }
  const FString State=UGameplayStatics::IsGamePaused(GetWorld())?TEXT("PAUSED"):(Match->Phase==EKTWMatchPhase::Preparation?TEXT("PREPARE"):Match->Phase==EKTWMatchPhase::Battle?TEXT("BATTLE"):TEXT("FINISHED"));
  if(Canvas->SizeX<=1100)DrawText(FString::Printf(TEXT("%s %02d:%02d"),*State,int32(Match->BattleSeconds)/60,int32(Match->BattleSeconds)%60),FColor::Yellow,240,23,nullptr,.9f);
  if(Canvas->SizeX>1100){DrawRect(FLinearColor(0,0,0,.9f),Canvas->SizeX/2-100,12,200,92);DrawText(State,FColor::Yellow,Canvas->SizeX/2-65,25,nullptr,1.1f);DrawText(FString::Printf(TEXT("%02d:%02d / 12:00"),int32(Match->BattleSeconds)/60,int32(Match->BattleSeconds)%60),FColor::White,Canvas->SizeX/2-80,55,nullptr,1.f);}
  const float X=Canvas->SizeX-270;const float Scale=AKTWPrototypeController::RecruitScale(Canvas->SizeY);auto UIY=[&](float Y){return 130+(Y-130)*Scale;};DrawRect(FLinearColor(.01f,.015f,.025f,.93f),X,130,260,405*Scale);
  auto Button=[&](float BY,const FString& Text){DrawRect(FLinearColor(.04f,.09f,.13f),X+4,UIY(BY),250,46*Scale);DrawText(Text,FColor::White,X+12,UIY(BY+11),nullptr,.9f*Scale);};
  for(int32 L=0;L<3;++L){const float LX=X+4+L*84;DrawRect(L==Match->SelectedLane?FLinearColor(.26,.35,.11):FLinearColor(.03,.07,.10),LX,UIY(135),80,35*Scale);DrawText(L==0?TEXT("Z UPPER"):L==1?TEXT("X MID"):TEXT("C LOWER"),L==Match->SelectedLane?FColor::Yellow:FColor::White,LX+5,UIY(145),nullptr,.83f*Scale);}
  Button(175,TEXT("[2] Runner 80 | carry 60"));Button(225,TEXT("[3] Knife 120 | vs Thrower"));Button(275,TEXT("[4] Pipe 180 | vs Knife"));Button(325,TEXT("[5] Thrower 220 | vs Pipe"));
  Button(375,FString::Printf(TEXT("[6] MYSTERY BOX 130 | CD %.0fs"),Match->BoxRemaining(0)));
  Button(425,Match->Phase==EKTWMatchPhase::Preparation?TEXT("[SPACE] Start battle"):TEXT("Battle started"));Button(475,TEXT("[Backspace] Prep refund"));
  DrawRect(FLinearColor(0,0,0,.85f),10,125,FMath::Max(10.f,Canvas->SizeX-290),48);DrawText(Match->Feedback,FColor::Yellow,18,135,nullptr,.75f);
  DrawRect(FLinearColor(.12f,.18f,.25f,.95f),12,Canvas->SizeY-160,320,52);
  DrawText(FString::Printf(TEXT("[6] BOX 130 | CD %.1fs | pity %d/3"),Match->BoxRemaining(0),Match->LowBoxes[0]),FColor::Yellow,20,Canvas->SizeY-155,nullptr,.8f);
  DrawText(Match->LowBoxes[0]>=3?TEXT("Runner 0% Knife 0% Pipe 62.5% Throw 37.5%"):TEXT("Runner 30% Knife 30% Pipe 25% Throw 15%"),FColor::White,20,Canvas->SizeY-132,nullptr,.65f);
  float QY=545;for(int32 Lane=0;Lane<3;++Lane){float ETA=0;for(const auto& E:Match->Queue)if(E.Faction==0&&E.Lane==Lane){ETA=FMath::Max(0.f,E.Remaining);break;}DrawText(FString::Printf(TEXT("Lane %d queue %d/3 | %.1fs"),Lane+1,Match->QueueCount(0,Lane),ETA),FColor::White,X+8,UIY(QY),nullptr,.9f*Scale);QY+=22;}
  if(Gold&&Match->CanAct()){const float Seconds=FMath::Max(0.f,Gold->NextRefresh-Match->BattleSeconds);DrawText(FString::Printf(TEXT("Next gold %.1fs | ground %d"),Seconds,Gold->GroundGold()),Seconds<=5?FColor::Yellow:FColor::White,X+8,UIY(QY),nullptr,.8f*Scale);}
 }else for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)Line(FString::Printf(TEXT("Seed %d | diagnostic fixture"),I->MapSeed));
 auto Label=[&](const TCHAR* Text,FVector P,FColor C){auto S=Project(P);DrawText(Text,C,S.X-45,S.Y-18,nullptr,.85f);};
 Label(TEXT("BLUE BASE"),AKTWStreetDirector::BasePosition(0),TeamColor(0));Label(TEXT("RED BASE"),AKTWStreetDirector::BasePosition(1),TeamColor(1));
 Label(TEXT("UPPER / BACK ALLEY"),FVector(-1500,-6000,20),FColor(120,200,255));Label(TEXT("LOWER / MARKET"),FVector(0,6000,20),FColor(255,210,100));
 Label(TEXT("MAIN ROAD"),FVector(1700,-1700,20),FColor::White);Label(TEXT("LINK ALLEY"),FVector(-2300,-2300,20),FColor(255,210,100));
 Y=Canvas->SizeY-65;DrawRect(FLinearColor(0,0,0,.8f),10,Y-5,600,60);
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)Line(FString::Printf(TEXT("%s boss HP %.0f/%.0f | %s %.1fs | retreat %s"),I->Faction==0?TEXT("Blue"):TEXT("Red"),I->Health,I->MaxHealth,I->bRecovering?TEXT("RECOVER"):TEXT("ACTIVE"),I->RecoveryRemaining,I->bLowHealthRetreat?TEXT("YES"):TEXT("NO")),TeamColor(I->Faction));
 if(auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController()))if(PC->bSelecting){float X,Z;PC->GetMousePosition(X,Z);const FVector2D A=PC->SelectionStart;DrawLine(A.X,A.Y,X,A.Y,FLinearColor::Green);DrawLine(X,A.Y,X,Z,FLinearColor::Green);DrawLine(X,Z,A.X,Z,FLinearColor::Green);DrawLine(A.X,Z,A.X,A.Y,FLinearColor::Green);}
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Health>0){auto P=Project(I->GetActorLocation()+FVector(0,0,180));DrawRect(FLinearColor(0,0,0,.85f),P.X-22,P.Y,44,5);DrawRect(FLinearColor(TeamColor(I->Faction)),P.X-22,P.Y,44*I->Health/I->Stats.MaxHealth,5);auto* Cargo=I->FindComponentByClass<UKTWGoldCarrier>();DrawText(FString::Printf(TEXT("%s L%d | GOLD %d/%d"),RoleLabel(I->UnitRole),I->Level,Cargo?Cargo->Carried:0,I->Stats.Capacity),TeamColor(I->Faction),P.X-65,P.Y-18,nullptr,.8f);}
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){
  const FVector Head=I->GetMesh()->GetSocketLocation(TEXT("head"))+FVector(0,0,130);auto P=Project(I->bRecovering?AKTWStreetDirector::BasePosition(I->Faction)+FVector(0,0,250):Head);
  DrawRect(FLinearColor(.005f,.005f,.008f,.95f),P.X-44,P.Y-2,88,12);DrawRect(FLinearColor(TeamColor(I->Faction)),P.X-42,P.Y,84*FMath::Clamp(I->Health/I->MaxHealth,0.f,1.f),8);
  DrawText(I->bRecovering?FString::Printf(TEXT("BOSS / RECOVER %.0fs"),I->RecoveryRemaining):FString::Printf(TEXT("BOSS  %.0f / %.0f"),I->Health,I->MaxHealth),FColor::White,P.X-58,P.Y-20,nullptr,.95f);
 }
 if(auto* Life=AKTWStreetLife::Find(GetWorld()))for(AKTWStreetWeapon* Weapon:Life->GroundWeapons)if(IsValid(Weapon)&&!Weapon->Equipped){auto P=Project(Weapon->GetActorLocation()+FVector(0,0,130));const TCHAR* Names[]={TEXT("KNIFE"),TEXT("PIPE"),TEXT("STOOL"),TEXT("MEDKIT"),TEXT("MOLOTOV")};DrawRect(FLinearColor(.01f,.025f,.04f,.9f),P.X-38,P.Y-20,76,18);DrawText(FString::Printf(TEXT("%s [%d]"),Names[FMath::Clamp(Weapon->Kind,0,4)],Weapon->Hits),FColor::Cyan,P.X-34,P.Y-18,nullptr,.85f);}
 for(int32 Team=0;Team<2;++Team)for(int32 L=0;L<3;++L){auto P=Project(UKTWGoldCarrier::VaultPosition(Team,L));DrawRect(FLinearColor(TeamColor(Team)),P.X-10,P.Y-10,20,20);DrawText(TEXT("VAULT"),FColor::Yellow,P.X-22,P.Y+12,nullptr,.7f);}
 DrawText(TEXT("WASD pan / wheel zoom | select / drag | right-click order / Alt ground | F hold / G auto-follow / B return / H escort / V raid | P pause / N restart"),FColor::White,20,Canvas->SizeY-88,nullptr,.75f);
 if(Match&&Match->Phase==EKTWMatchPhase::Finished){const float X=Canvas->SizeX/2-270,Z=Canvas->SizeY/2-90;DrawRect(FLinearColor(.01f,.02f,.035f,.97f),X,Z,540,180);DrawText(Match->Winner==-1?TEXT("DRAW"):Match->Winner==0?TEXT("BLUE VICTORY"):Match->Winner==1?TEXT("RED VICTORY"):TEXT("MATCH FINISHED"),FColor::Yellow,X+100,Z+24,nullptr,2.f);DrawText(Match->VictoryReason,FColor::White,X+25,Z+82,nullptr,.95f);DrawText(TEXT("Press N to play again"),FColor::White,X+130,Z+125,nullptr,1.f);}

 for(TActorIterator<AKTWGoldPile>I(GetWorld());I;++I){auto S=Project(I->GetActorLocation()+FVector(0,0,120));DrawText(FString::Printf(TEXT("GOLD %d"),I->Amount),I->Amount>0?FColor::Yellow:FColor(150,140,90),S.X-25,S.Y,nullptr,.85f);}
 for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)if(I->Faction==1)DrawText(FString::Printf(TEXT("Computer: %s"),*I->Plan),FColor(190,205,220),Canvas->SizeX-262,Canvas->SizeY-42,nullptr,.8f);
 if(auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController()))if(IsValid(PC->SelectedBoss)){const TCHAR* Names[]={TEXT("HOLD"),TEXT("FOLLOW"),TEXT("ESCORT"),TEXT("RETURN"),TEXT("ATTACK")};DrawText(FString::Printf(TEXT("BOSS SELECTED | %s | right-click move / F hold / G follow / H courier / B return"),Names[int32(PC->SelectedBoss->Task)]),FColor::Cyan,20,224,nullptr,.85f);}
 if(auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController()))if(IsValid(PC->SelectedUnit))if(auto* C=PC->SelectedUnit->FindComponentByClass<UKTWGoldCarrier>()){DrawText(FString::Printf(TEXT("%s | G %d/%d | channel %.1fs | %s | XP %.1f / %d"),C->TaskLabel(),C->Carried,PC->SelectedUnit->Stats.Capacity,C->ChannelProgress,C->bDeferredOrder?TEXT("ORDER AFTER DELIVERY"):TEXT(""),FMath::Min(PC->SelectedUnit->Experience,160.f),PC->SelectedUnit->Level<2?60:160),FColor::Yellow,20,180,nullptr,.8f);}
 if(auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController()))if(IsValid(PC->SelectedUnit))if(auto* L=AKTWStreetLife::Find(GetWorld())){auto* W=L->Equipment(PC->SelectedUnit);DrawText(FString::Printf(TEXT("GEAR %s | +%.0f%% damage | uses %d | MOLOTOV %d"),W?(W->Kind==0?TEXT("KNIFE"):W->Kind==1?TEXT("PIPE"):TEXT("STOOL")):TEXT("NONE"),W?(W->Multiplier()-1)*100:0,W?W->Hits:0,L->Throwable(PC->SelectedUnit)?1:0),FColor::Cyan,20,203,nullptr,.8f);}
 if(auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController())){for(AKTWTestUnit* U:PC->SelectedUnits)if(IsValid(U)&&U->Health>0){auto P=Project(U->GetActorLocation());for(int32 I=0;I<20;++I){float A=I*2*PI/20,B=(I+1)*2*PI/20;DrawLine(P.X+FMath::Cos(A)*18,P.Y+FMath::Sin(A)*8,P.X+FMath::Cos(B)*18,P.Y+FMath::Sin(B)*8,FLinearColor(0.f,1.f,1.f),1.5f);}}if(GetWorld()->GetTimeSeconds()-PC->LastCommandTime<1.15f){auto P=Project(PC->LastCommandPoint);DrawText(TEXT("ORDER CONFIRMED"),FColor::Cyan,P.X-50,P.Y-25,nullptr,.85f);}}
 if(auto* S=UKTWSession::Get(this))S->DrawOverlay(this);
}
AKTWPrototypeGameMode::AKTWPrototypeGameMode(){DefaultPawnClass=AKTWCameraPawn::StaticClass();PlayerControllerClass=AKTWPrototypeController::StaticClass();HUDClass=AKTWHoloHUD::StaticClass();}
void AKTWPrototypeGameMode::BeginPlay() {
 Super::BeginPlay();
 if((UKTWSession::Get(this)&&UKTWSession::Get(this)->ModeChoice==1&&!UKTWSession::Get(this)->InMenu)||GetWorld()->GetMapName().Contains(TEXT("L_RuleArena"))){if(auto* S=UKTWSession::Get(this)){S->ModeChoice=1;S->InMenu=false;}GetWorld()->SpawnActor<AKTWRogueRun>();return;}
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWUIProfileTest"))){FTimerHandle Test;GetWorldTimerManager().SetTimer(Test,[this](){UKTWSession::Get(this)->RunUIProfileTest();},2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWModeTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,[this](){UKTWSession::Get(this)->RunModeTest();},4.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWModeCapture"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,[this](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ModeSelect.png"),true,false);FTimerHandle End;GetWorldTimerManager().SetTimer(End,[](){FPlatformMisc::RequestExit(false);},1.f,false);},6.f,false);}
#endif
 if(AKTWStreetWar::Enabled(GetWorld()))GetWorld()->SpawnActor<AKTWStreetWar>();
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWBalanceTest"))){FTimerHandle Setup;GetWorldTimerManager().SetTimer(Setup,FTimerDelegate::CreateLambda([this](){
  for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)I->Destroy();
  auto Winners=MakeShared<TArray<TWeakObjectPtr<AKTWTestUnit>>>();auto Losers=MakeShared<TArray<TWeakObjectPtr<AKTWTestUnit>>>();
  const FVector Centers[]={FVector(-6000,0,100),FVector(0,-6000,100),FVector(6000,0,100)};
  const EKTWRole A[]={EKTWRole::Knife,EKTWRole::Pipe,EKTWRole::Thrower},B[]={EKTWRole::Thrower,EKTWRole::Knife,EKTWRole::Pipe};
  for(int32 I=0;I<3;++I){const FVector Offset=I==1?FVector(100,0,0):FVector(0,I==2?450:100,0);AKTWTestUnit* Pair[2];for(int32 T=0;T<2;++T){const FVector P=Centers[I]+Offset*(T?1:-1);Pair[T]=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);Pair[T]->Faction=T;Pair[T]->UnitRole=T?B[I]:A[I];Pair[T]->bAutoPatrol=false;Pair[T]->FinishSpawning(FTransform(P));}for(int32 T=0;T<2;++T)Pair[T]->IssueOrder(EKTWUnitOrder::Attack,Pair[1-T]->GetActorLocation(),Pair[1-T]);Winners->Add(Pair[0]);Losers->Add(Pair[1]);}
  FTimerHandle Result;GetWorldTimerManager().SetTimer(Result,FTimerDelegate::CreateLambda([Winners,Losers](){int32 Fail=0;for(int32 I=0;I<3;++I){const bool OK=(*Winners)[I].IsValid()&&(*Winners)[I]->Health>0&&(!(*Losers)[I].IsValid()||(*Losers)[I]->Health<=0);Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW BALANCE pair=%d winnerHP=%.0f loserHP=%.0f %s"),I,(*Winners)[I].IsValid()?(*Winners)[I]->Health:0,(*Losers)[I].IsValid()?(*Losers)[I]->Health:0,OK?TEXT("PASS"):TEXT("FAIL"));}UE_LOG(LogTemp,Display,TEXT("KTW BALANCE COMPLETE failures=%d"),Fail);FPlatformMisc::RequestExitWithStatus(false,Fail?1:0);}),22.f,false);
 }),2.f,false);}
#endif

 const bool Diagnostic=FParse::Param(FCommandLine::Get(),TEXT("KTWBalanceTest"))||FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"))||FParse::Param(FCommandLine::Get(),TEXT("KTWCombatTest"))||FParse::Param(FCommandLine::Get(),TEXT("KTWActionCapture"));
 for(int32 Team=0;Team<2;++Team) {
  bool Exists=false;for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==Team)Exists=true;
  const FVector Base=AKTWStreetDirector::BasePosition(Team);
  if(!Exists){auto* B=GetWorld()->SpawnActorDeferred<AKTWBoss>(AKTWBoss::StaticClass(),FTransform(Base));B->Faction=Team;B->FinishSpawning(FTransform(Base));}
  if(bSpawnTestUnits&&Diagnostic)for(int32 I=0;I<4;++I){const FVector P=Base+FVector(Team==0?250:-250,I*200-300,-10);auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->Faction=Team;U->UnitRole=EKTWRole(I);U->Lane=I%3;U->bEnableCombat=!FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWCombatTest"));U->bAutoPatrol=!FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWCombatTest"));U->bRunner=I==0;U->bCarryingGold=I==0&&FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"));U->FinishSpawning(FTransform(P));}
 }
 if(!Diagnostic){auto* Match=GetWorld()->SpawnActor<AKTWMatch>();GetWorld()->SpawnActor<AKTWGoldSystem>();GetWorld()->SpawnActor<AKTWCommander>();if(!UKTWSession::Diagnostic()||FParse::Param(FCommandLine::Get(),TEXT("KTWPresentationTest"))){GetWorld()->SpawnActor<AKTWStreetLife>();GetWorld()->SpawnActor<AKTWWeather>();}
  if(!FParse::Param(FCommandLine::Get(),TEXT("KTWEconomyTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWGoldTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWGoldAutoTest"))){const EKTWRole Opening[]={EKTWRole::Runner,EKTWRole::Runner,EKTWRole::Knife,EKTWRole::Pipe};const int32 Lanes[]={0,2,2,1};for(int32 I=0;I<4;++I)Match->Purchase(1,Opening[I],Lanes[I]);}
  if(FParse::Param(FCommandLine::Get(),TEXT("KTWCapture"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWPickupCapture"))){for(int32 I=0;I<4;++I)Match->Purchase(0,EKTWRole(I),I%3);Match->StartBattle();}
 }
 for(TActorIterator<AKTWCameraPawn>I(GetWorld());I;++I)I->SetActorLocation(FVector(0,-800,26000));
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWOrderCapture"))){FTimerHandle T,S;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([this](){auto* PC=Cast<AKTWPrototypeController>(GetWorld()->GetFirstPlayerController());if(!PC)return;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->Health>0)PC->SelectedUnits.Add(*I);if(PC->SelectedUnits.Num())PC->SelectedUnit=PC->SelectedUnits[0];PC->CommandAtWorld(FVector(-3000,3000,20));}),5.f,false);GetWorldTimerManager().SetTimer(S,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/KTW_V49_Order.png"),true,false);}),5.35f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWRevisionTest"))){
  FTimerHandle T;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([this](){
   auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const TCHAR* N){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW REVISION %s: %s"),N,OK?TEXT("PASS"):TEXT("FAIL"));};
   auto* M=AKTWMatch::Find(GetWorld());auto* G=AKTWGoldSystem::Find(GetWorld());auto* PC=Cast<AKTWPrototypeController>(GetWorld()->GetFirstPlayerController());auto* Life=AKTWStreetLife::Find(GetWorld());
   Check(M&&G&&PC&&Life,TEXT("all gameplay systems exist"));if(!M||!G||!PC||!Life){FPlatformMisc::RequestExitWithStatus(false,1);return;}
   Check(G->SourcesReady&&G->Piles.Num()==6,TEXT("gold PCG creates exactly six sources"));int32 Link=0;bool NavOK=true;auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
   for(AKTWGoldPile* P:G->Piles){const FVector V=P->GetActorLocation();if(FMath::Abs(V.X-V.Y)<20)++Link;for(int32 F=0;F<2;++F){auto* Path=NS?NS->FindPathToLocationSynchronously(GetWorld(),AKTWStreetDirector::BasePosition(F)+FVector(0,0,100),V+FVector(0,0,80),nullptr,FKTWTactics::Filter(F)):nullptr;NavOK&=Path&&Path->IsValid()&&!Path->IsPartial();}}
   Check(Link==6,TEXT("all six gold sources on transverse link"));Check(NavOK,TEXT("both factions can reach every generated gold source"));
   bool SeedsOK=true;for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I){for(int32 Seed=0;Seed<100;++Seed){auto Points=I->BuildLayer(EKTWLayer::GoldSources,Seed);SeedsOK&=Points.Num()==6;for(const auto& P:Points)SeedsOK&=AKTWStreetDirector::IsRoadPosition(P.Transform.GetLocation(),-180)&&P.Transform.GetLocation().Size2D()<6000;}auto A=I->BuildLayer(EKTWLayer::GoldSources,1),B=I->BuildLayer(EKTWLayer::GoldSources,2);SeedsOK&=!A[0].Transform.GetLocation().Equals(B[0].Transform.GetLocation());}
   Check(SeedsOK,TEXT("100 gold seeds stay central and vary"));
   const int32 Before=M->Accounts[0].Cash;const float BoxY=130+(398-130)*PC->RecruitScale(900);Check(PC->HandleRecruitClick(1500,BoxY,1600,900)&&M->Accounts[0].Cash==Before-130&&M->Population(0)==1,TEXT("visible recruit panel box click charges 130 and deploys"));
   Check(!PC->HandleRecruitClick(1500,130+(423-130)*PC->RecruitScale(900),1600,900),TEXT("button gap cannot buy or start battle"));
   const int32 Cash=M->Accounts[0].Cash;PC->HandleRecruitClick(1500,BoxY,1600,900);Check(M->Accounts[0].Cash==Cash,TEXT("cooldown blocks duplicate click charges"));
   Check(Life->GroundWeapons.Num()==12,TEXT("twelve map-wide loot actors visible"));for(AKTWStreetWeapon* W:Life->GroundWeapons)Check(W&&W->Parts.Num()>=2&&!W->Equipped,TEXT("ground weapon has overview beacon"));
   int32 Civ=0;for(TActorIterator<AKTWCivilian>I(GetWorld());I;++I)++Civ;Check(Civ==8,TEXT("eight residents before battle"));
   M->StartBattle();M->Tick(20);G->Tick(0);Check(G->GeneratedGold==70&&G->GoldConserved(),TEXT("first central gold wave remains exactly seventy"));
   bool SurfaceOK=true;int32 SurfaceCount=0;for(int32 X=-6600;X<=6600;X+=400)for(int32 Y=-6600;Y<=6600;Y+=400){FVector V(X,Y,100);if(!AKTWStreetDirector::IsRoadPosition(V,-130))continue;for(int32 F=0;F<2;++F){if(FKTWTactics::Protected(V,1-F))continue;auto* Path=NS?NS->FindPathToLocationSynchronously(GetWorld(),AKTWStreetDirector::BasePosition(F)+FVector(0,0,100),V,nullptr,FKTWTactics::Filter(F)):nullptr;++SurfaceCount;SurfaceOK&=Path&&Path->IsValid()&&!Path->IsPartial();}}
   UE_LOG(LogTemp,Display,TEXT("KTW REVISION road surface path queries=%d"),SurfaceCount);Check(SurfaceOK,TEXT("road surfaces and all central turns stay reachable after new facades"));
   FTimerHandle End;GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateLambda([this,Fail,Check](){int32 Count=0;for(TActorIterator<AKTWCivilian>I(GetWorld());I;++I)++Count;Check(Count==0,TEXT("all residents flee and despawn after battle starts"));auto* G=AKTWGoldSystem::Find(GetWorld());Check(G&&G->GoldConserved(),TEXT("central gold relocation preserves ledger"));UE_LOG(LogTemp,Display,TEXT("KTW REVISION COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);}),12.f,false);
  }),4.f,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWFrontendTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([this](){if(auto* S=UKTWSession::Get(this))S->RunFrontendTest();}),4.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWUserNPCTest"))){
  FTimerHandle Setup;GetWorldTimerManager().SetTimer(Setup,FTimerDelegate::CreateLambda([this](){
   for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)I->SetActorTickEnabled(false);
   for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();
   for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)I->Destroy();
   auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const FString& Name){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW USER NPC %s: %s"),*Name,OK?TEXT("PASS"):TEXT("FAIL"));};
   const TCHAR* Roles[]={TEXT("Runner"),TEXT("Knife"),TEXT("Pipe"),TEXT("Thrower"),TEXT("Boss")};
   for(int32 Team=0;Team<2;++Team)for(int32 RoleID=0;RoleID<5;++RoleID){
    const FVector P(30000+RoleID*1700,Team*2000,200);ACharacter* Character=nullptr;
    if(RoleID==4){auto* B=GetWorld()->SpawnActorDeferred<AKTWBoss>(AKTWBoss::StaticClass(),FTransform(P));B->Faction=Team;B->FinishSpawning(FTransform(P));B->SetActorTickEnabled(false);Character=B;}
    else{auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->Faction=Team;U->UnitRole=EKTWRole(RoleID);U->bEnableCombat=false;U->bAutoPatrol=false;U->FinishSpawning(FTransform(P));U->GetWorldTimerManager().ClearAllTimersForObject(U);Character=U;const auto HP=U->Health;U->GainExperience(160);Check(U->Level==3&&U->Health>=HP,TEXT("promotion retains model and stats"));}
    Character->GetCharacterMovement()->DisableMovement();Character->SetActorRotation(FRotator(0,-90,0));
    auto* Mesh=Character->GetMesh();const FString Expected=FString::Printf(TEXT("SK_%s_%s"),Team?TEXT("Red"):TEXT("Blue"),Roles[RoleID]);
    Check(Mesh->GetSkeletalMeshAsset()&&Mesh->GetSkeletalMeshAsset()->GetName()==Expected,Expected+TEXT(" mapping"));
    Check(Mesh->GetAnimInstance()!=nullptr,Expected+TEXT(" animation instance"));Check(Mesh->GetBoneIndex(TEXT("hand_r"))!=INDEX_NONE&&Mesh->GetBoneIndex(TEXT("spine_03"))!=INDEX_NONE,Expected+TEXT(" gameplay sockets"));
    Check(Mesh->GetMaterial(0)&&Mesh->GetMaterial(0)->GetPathName().Contains(TEXT("/UserNPC/Materials/")),Expected+TEXT(" original material"));
   }
   for(TActorIterator<AKTWCameraPawn>I(GetWorld());I;++I){I->SetActorLocation(FVector(33500,-10000,8000));I->Camera->SetRelativeRotation(FRotator(-28,90,0));I->Camera->FieldOfView=55;}
   FTimerHandle Shot,End;GetWorldTimerManager().SetTimer(Shot,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/UserNPC_Lineup.png"),true,false);}),3.f,false);
   GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateLambda([Fail](){UE_LOG(LogTemp,Display,TEXT("KTW USER NPC COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);}),5.f,false);
  }),3.f,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWMenuCapture"))){FTimerHandle T,Q;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/KTW_V47_Menu.png"),true,false);}),4.f,false);GetWorldTimerManager().SetTimer(Q,FTimerDelegate::CreateLambda([](){FPlatformMisc::RequestExit(false);}),7.f,false);return;}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWPackageSmoke"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([this](){auto* M=AKTWMatch::Find(GetWorld());auto* S=UKTWSession::Get(this);const bool OK=M&&S&&S->Ready()&&M->Purchase(0,EKTWRole::Runner,0);UE_LOG(LogTemp,Display,TEXT("KTW PACKAGE SMOKE %s"),OK?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,OK?0:1);}),5.f,false);}
#endif

#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWAutomation"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWCombatTest"))) {
  struct FProbe {int32 Step=0;int32 Fail=0;TWeakObjectPtr<AKTWBoss> Boss;TWeakObjectPtr<AKTWTestUnit> Runner;};
  auto Probe=MakeShared<FProbe>();
  for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==0)Probe->Boss=*I;
  for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==0&&I->bRunner)Probe->Runner=*I;
  FTimerHandle Handle;
  GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateLambda([this,Probe]() {
   auto Check=[&](bool OK,const TCHAR* Name){UE_LOG(LogTemp,Display,TEXT("KTW TEST %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)++Probe->Fail;};
   auto* B=Probe->Boss.Get();auto* U=Probe->Runner.Get();if(!B||!U){FPlatformMisc::RequestExitWithStatus(false,1);return;}
   ++Probe->Step;
   switch(Probe->Step) {
    case 1: U->SetActorLocation(B->GetActorLocation()+FVector(150,0,0));break;
    case 2: Check(U->bAura&&FMath::IsNearlyEqual(U->GetCharacterMovement()->MaxWalkSpeed,U->Stats.Speed*.85f*1.12f,.1f)&&U->GetMesh()->GetAnimInstance()!=nullptr,TEXT("runner carrying aura speed"));U->SetActorLocation(B->GetActorLocation()+FVector(0,-2300,0));break;
    case 4: Check(!U->bAura&&FMath::IsNearlyEqual(U->GetCharacterMovement()->MaxWalkSpeed,U->Stats.Speed*.85f,.1f),TEXT("aura removed after leave"));U->SetActorLocation(B->GetActorLocation()+FVector(150,0,0));break;
    case 5: Check(U->bAura,TEXT("aura reentry"));B->RecoverySeconds=2;B->ApplyTestDamage(1000);Check(B->bRecovering&&!U->bAura&&!B->GetActorEnableCollision(),TEXT("defeat clears aura collision"));break;
    case 8: Check(!B->bRecovering&&B->Health==B->MaxHealth&&B->GetActorEnableCollision(),TEXT("recovery returns full health"));B->IssueTask(EKTWBossTask::Sit,FVector(-4600,4600,100),nullptr);break;
    case 12: UE_LOG(LogTemp,Display,TEXT("KTW NAV DIAGNOSTIC boss=%s controller=%s nav=%s"),*B->GetActorLocation().ToString(),*GetNameSafe(B->GetController()),*GetNameSafe(FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())));Check(B->GetActorLocation().X> -5850&&B->GetController()&&Cast<AAIController>(B->GetController())->GetBrainComponent()!=nullptr,TEXT("behavior tree navigation movement"));B->ApplyTestDamage(260);break;
    case 13: Check(B->bLowHealthRetreat&&B->Task==EKTWBossTask::Return,TEXT("low health auto retreat"));break;
    case 16: {
     int32 BossCount=0;for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)++BossCount;
     Check(BossCount==2,TEXT("one boss per faction"));
     UE_LOG(LogTemp,Display,TEXT("KTW RUNTIME TESTS COMPLETE failures=%d"),Probe->Fail);
     if(!FParse::Param(FCommandLine::Get(),TEXT("KTWCrossingTest"))){FPlatformMisc::RequestExitWithStatus(false,Probe->Fail?1:0);break;}
     B->Health=B->MaxHealth;B->bLowHealthRetreat=false;B->SetActorLocation(FVector(-1300,1300,110),false,nullptr,ETeleportType::TeleportPhysics);B->IssueTask(EKTWBossTask::Sit,FVector(1300,1300,100),nullptr);
     for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==1){I->SetActorLocation(FVector(1300,1300,110),false,nullptr,ETeleportType::TeleportPhysics);I->IssueTask(EKTWBossTask::Sit,FVector(-1300,1300,100),nullptr);}
     break;
    }
    case 30: {
     Check(FVector::Dist2D(B->GetActorLocation(),FVector(1300,1300,0))<250,TEXT("blue main-road to link-alley crossing"));
     for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)if(I->Faction==1)Check(FVector::Dist2D(I->GetActorLocation(),FVector(-1300,1300,0))<250,TEXT("red link-alley to main-road crossing"));
     UE_LOG(LogTemp,Display,TEXT("KTW V4 CROSSING TESTS COMPLETE failures=%d"),Probe->Fail);FPlatformMisc::RequestExitWithStatus(false,Probe->Fail?1:0);break;
    }
   }
  }),1.f,true);
 }

 if(FParse::Param(FCommandLine::Get(),TEXT("KTWCombatTest"))){
  auto SpawnProbe=[&](EKTWRole R,int32 F,FVector P,bool Enabled){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->UnitRole=R;U->Faction=F;U->bEnableCombat=Enabled;U->bAutoPatrol=false;U->FinishSpawning(FTransform(P));return U;};
  auto* Knife=SpawnProbe(EKTWRole::Knife,0,FVector(-200,200,100),true);
  auto* Thrower=SpawnProbe(EKTWRole::Thrower,1,FVector(-100,100,100),false);
  auto* Pipe=SpawnProbe(EKTWRole::Pipe,0,FVector(-800,800,100),false);
  FDamageEvent Damage;Pipe->TakeDamage(100,Damage,nullptr,Thrower);
  auto Failed=MakeShared<bool>(!FMath::IsNearlyEqual(Pipe->Health,130.f));
  UE_LOG(LogTemp,Display,TEXT("KTW COMBAT TEST ranged armor penetration: %s"),*Failed?TEXT("FAIL"):TEXT("PASS"));
  TWeakObjectPtr<AKTWTestUnit> WKnife=Knife,WThrow=Thrower;FTimerHandle Windup,FirstHit,Finish;
  GetWorldTimerManager().SetTimer(Windup,FTimerDelegate::CreateLambda([WKnife,WThrow,Failed](){auto* Action=WKnife.IsValid()?WKnife->FindComponentByClass<UKTWCombatAction>():nullptr;const bool OK=Action&&Action->bPlaying&&Action->ConfirmedHits==0&&WThrow.IsValid()&&FMath::IsNearlyEqual(WThrow->Health,90.f);*Failed|=!OK;UE_LOG(LogTemp,Display,TEXT("KTW COMBAT TEST animation windup before damage: %s"),OK?TEXT("PASS"):TEXT("FAIL"));}),.28f,false);
  GetWorldTimerManager().SetTimer(FirstHit,FTimerDelegate::CreateLambda([WKnife,WThrow,Failed](){auto* Action=WKnife.IsValid()?WKnife->FindComponentByClass<UKTWCombatAction>():nullptr;bool OK=WThrow.IsValid()&&FMath::IsNearlyEqual(WThrow->Health,59.f)&&Action&&Action->ConfirmedHits==1;*Failed|=!OK;UE_LOG(LogTemp,Display,TEXT("KTW COMBAT TEST actual knife counter strike: %s HP=%.0f"),OK?TEXT("PASS"):TEXT("FAIL"),WThrow.IsValid()?WThrow->Health:-1);}),.6f,false);
  GetWorldTimerManager().SetTimer(Finish,FTimerDelegate::CreateLambda([WKnife,WThrow,Failed](){bool OK=WKnife.IsValid()&&WThrow.IsValid()&&WKnife->Health>0&&WThrow->Health<=0;*Failed|=!OK;UE_LOG(LogTemp,Display,TEXT("KTW COMBAT TEST autonomous defeat: %s"),OK?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Failed?1:0);}),6.f,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWCapture"))) {
  if(FParse::Param(FCommandLine::Get(),TEXT("KTWPickupCapture"))){auto* M=AKTWMatch::Find(GetWorld());auto* S=AKTWGoldSystem::Find(GetWorld());if(!M||!S||S->Piles.IsEmpty())return;M->StartBattle();M->Tick(20);S->Tick(0);auto* P=S->Piles[0].Get();
   const FVector Focus=P->GetActorLocation();for(TActorIterator<AKTWCameraPawn>I(GetWorld());I;++I){I->SetActorLocation(Focus+FVector(-80,3300,3850));I->Camera->SetRelativeRotation(FRotator(-44,-90,0));I->Camera->FieldOfView=60;}
   auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(Focus+FVector(100,0,100)));U->Faction=0;U->UnitRole=EKTWRole::Runner;U->bEnableCombat=false;U->bAutoPatrol=false;U->Order=EKTWUnitOrder::Defend;U->FinishSpawning(FTransform(Focus+FVector(100,0,100)));TWeakObjectPtr<AKTWTestUnit> W=U;TWeakObjectPtr<AKTWGoldPile> G=P;
   FTimerHandle Begin,Shot,Quit;GetWorldTimerManager().SetTimer(Begin,FTimerDelegate::CreateLambda([W,G](){if(W.IsValid()&&G.IsValid())W->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(G.Get());}),2.f,false);
   GetWorldTimerManager().SetTimer(Shot,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/KTW_V43_Pickup.png"),true,false);}),2.15f,false);GetWorldTimerManager().SetTimer(Quit,FTimerDelegate::CreateLambda([](){FPlatformMisc::RequestExit(false);}),4.f,false);return;
  }
  const bool ActionShot=FParse::Param(FCommandLine::Get(),TEXT("KTWActionCapture"));
  const bool Detail=FParse::Param(FCommandLine::Get(),TEXT("KTWCharacterCapture"));
  if(ActionShot){for(TActorIterator<AKTWCameraPawn>I(GetWorld());I;++I){I->SetActorLocation(FVector(-80,3300,3850));I->Camera->SetRelativeRotation(FRotator(-44,-90,0));I->Camera->FieldOfView=60;}
   auto Spawn=[&](EKTWRole Type,int32 Team,FVector P){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->UnitRole=Type;U->Faction=Team;U->bEnableCombat=false;U->bAutoPatrol=false;U->FinishSpawning(FTransform(P));return U;};
   auto* Blue=Spawn(EKTWRole::Knife,0,FVector(-150,0,100));auto* Red=Spawn(EKTWRole::Thrower,1,FVector(0,0,100));TWeakObjectPtr<AKTWTestUnit> A=Blue,B=Red;FTimerHandle StartAction;
   GetWorldTimerManager().SetTimer(StartAction,FTimerDelegate::CreateLambda([A,B](){if(A.IsValid()&&B.IsValid())A->FindComponentByClass<UKTWCombatAction>()->Attack(B.Get(),18,180,false);}),2.f,false);
   FTimerHandle Shot,Quit;GetWorldTimerManager().SetTimer(Shot,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/KTW_V42_Action.png"),true,false);}),2.25f,false);GetWorldTimerManager().SetTimer(Quit,FTimerDelegate::CreateLambda([](){FPlatformMisc::RequestExit(false);}),4.f,false);return;
  }
  if(Detail){for(TActorIterator<AKTWCameraPawn>I(GetWorld());I;++I){I->SetActorLocation(AKTWStreetDirector::BasePosition(0)+FVector(300,900,1000));I->Camera->SetRelativeRotation(FRotator(-45,-90,0));I->Camera->FieldOfView=75;}}
  FTimerHandle Capture,Exit;
  const bool GoldShot=FParse::Param(FCommandLine::Get(),TEXT("KTWGoldCapture"));
  GetWorldTimerManager().SetTimer(Capture,FTimerDelegate::CreateLambda([](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/KTW_V4.png"),true,false);}),GoldShot?40.f:(Detail?2.f:10.f),false);
  GetWorldTimerManager().SetTimer(Exit,FTimerDelegate::CreateLambda([](){FPlatformMisc::RequestExit(false);}),GoldShot?44.f:(Detail?6.f:14.f),false);
 }
#endif

}




// Explicit opt-in diagnostics. These never run in a normal Play session.
// Tests exercise real world timers, navigation and actor lifecycle rather than mirroring formulas only.






void AKTWTestUnit::GainExperience(float Amount){
 if(Health<=0||Amount<=0||!FMath::IsFinite(Amount))return;
 Experience+=Amount;const int32 NewLevel=Experience>=160?3:Experience>=60?2:1;if(NewLevel<=Level)return;
 const float NewFactor=NewLevel==3?1.3f:1.15f;
 const float PreviousMax=Stats.MaxHealth;Stats.MaxHealth=BaseHealth*NewFactor;Stats.Damage=BaseDamage*NewFactor;Stats.Capacity+=(NewLevel-Level)*5;Level=NewLevel;Health=FMath::Min(Stats.MaxHealth,Health+Stats.MaxHealth-PreviousMax);UKTWSession::Cue(this,TEXT("Level"),.8f,GetActorLocation());FKTWRunnerVisuals::Refresh(this);
}
