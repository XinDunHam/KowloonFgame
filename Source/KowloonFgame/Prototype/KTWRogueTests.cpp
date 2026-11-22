#include "KTWRogue.h"
#include "KTWSession.h"
#include "KTWMatch.h"
#include "KTWStreetWar.h"
#include "KTWGold.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void AKTWRogueRun::RunTests(){
#if !UE_BUILD_SHIPPING
 auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const TCHAR* Label){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW ROGUE %s: %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 Check(ConfigOK&&SoldierTypes.Num()==6&&CardTypes.Num()==8&&Waves.Num()==5,TEXT("external data loads all soldiers cards and waves"));Check(!AKTWMatch::Find(GetWorld())&&!AKTWStreetWar::Find(GetWorld())&&!AKTWGoldSystem::Find(GetWorld()),TEXT("rule run is isolated from street match economy and fog"));Check(Roster.Num()==3&&Coins==200&&Lives==3,TEXT("new run starts with three soldiers two hundred gold three lives"));
 if(!ConfigOK||Units.Num()<4){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 Check(Buy(5)&&Roster.Last()==5&&Coins==80&&Units.Last()->Stats.Ranged&&Units.Last()->Mesh->GetSkeletalMeshAsset()->GetFName()==TEXT("SK_Security"),TEXT("gunner recruits at one hundred twenty gold with security model"));Roster.SetNum(3);Positions.SetNum(3);Coins=200;Prepare();
 bool Layouts=true;TSet<FString> Tactics;int LastBudget=0;for(int W=0;W<Waves.Num();++W){Tactics.Add(Waves[W].Formation);const auto Points=EnemyFormation(W),Again=EnemyFormation(W);Layouts&=Points.Num()==Waves[W].Soldiers.Num()&&Points==Again;for(int I=0;I<Points.Num();++I){Layouts&=Points[I].X>250&&Points[I].X<1650&&FMath::Abs(Points[I].Y)<750;for(int J=0;J<I;++J)Layouts&=FVector::Dist2D(Points[I],Points[J])>110;}const int Budget=EnemyBudget(W);Layouts&=Budget>LastBudget&&(!LastBudget||Budget<=LastBudget*2);LastBudget=Budget;}Check(Layouts&&Tactics.Num()==5,TEXT("five distinct deterministic formations have safe spacing and bounded rising budgets"));
 auto* Ally=Units[0].Get();Check(Place(Ally,FVector(-1000,-450,0))&&Positions[0].Equals(FVector(-1000,-450,0)),TEXT("single soldier deployment stores exact custom position"));AutoDeploy();const FVector A=Positions[0],B=Positions[1];Check(PlaceSquad({Units[0],Units[1]},{B,A})&&Positions[0]==B&&Positions[1]==A,TEXT("squad placement can atomically swap occupied ally slots"));Check(!PlaceSquad({Units[0],Units[1]},{FVector(1000,0,0),FVector(-1000,-400,0)})&&Positions[0]==B&&Positions[1]==A,TEXT("invalid squad placement leaves every soldier unchanged"));AutoDeploy();
 Check(!Place(Ally,FVector(1000,0,0))&&!Place(Ally,Units[1]->GetActorLocation()),TEXT("deployment rejects enemy side and overlapping placement"));const int Before=Coins;Check(Buy(2)&&Roster.Num()==4&&Coins==Before-SoldierTypes[2].Cost,TEXT("recruit charges once and extends persistent roster"));const int Money=Coins;Check(!Buy(4)&&Coins==Money,TEXT("unaffordable recruit cannot change balance"));
 ActiveCards={0,1,2,5,6};RebuildRules();Check(FMath::IsNearlyEqual(Rules.Gravity,.4f)&&Rules.Knockback==2&&Rules.Friction==.05f,TEXT("combined rule context changes gravity knockback friction"));StartBattle();Check(Phase==EKTWRoguePhase::Battle&&Ally->Body->IsSimulatingPhysics(),TEXT("start battle activates real rigid bodies"));AKTWRogueUnit* Enemy=nullptr;for(auto U:Units)if(U->Team==1){Enemy=U;break;}Check(Enemy&&Enemy->Body->GetMass()<Ally->Body->GetMass(),TEXT("enemy lightweight card affects physical mass"));Selected={Ally};Ally->Windup=.1f;const FVector MovePoint(-1200,100,0);CommandSelected(MovePoint);Check(Ally->MovingOrder&&Ally->Goal.Equals(MovePoint)&&Ally->Windup<0,TEXT("single selected move reaches exact point and cancels attack windup"));Ally->Windup=.1f;CommandSelected(Enemy->GetActorLocation(),Enemy);Check(Ally->Target==Enemy&&!Ally->MovingOrder&&Ally->Windup<0,TEXT("new focus order cancels old windup and keeps chosen enemy"));
 Ally->OrderMove(FVector(-1200,100,0));Ally->Body->SetPhysicsLinearVelocity(FVector(100,0,0));Ally->Tick(0);auto* Walk=Ally->Mesh->GetSingleNodeInstance();Check(Walk&&Walk->GetCurrentAsset()&&Walk->GetCurrentAsset()->GetFName()==TEXT("ThirdPersonWalk")&&Ally->Body->GetUpVector().Z>.999,TEXT("moving living soldier uses upright walking animation"));Ally->Body->SetPhysicsLinearVelocity(FVector::ZeroVector);Ally->OrderAttack(Enemy);
 const FVector EnemyStart=Enemy->GetActorLocation();Enemy->SetActorLocation(Ally->GetActorLocation()+FVector(120,0,0));Ally->Cooldown=0;Ally->Windup=-1;Ally->Tick(0);auto* Animation=Ally->Mesh->GetSingleNodeInstance();Check(Ally->Windup>=0&&Animation&&Animation->GetCurrentAsset()&&Animation->GetCurrentAsset()->GetFName()==TEXT("A_StreetStrike"),TEXT("melee animation survives attack startup frame"));const auto OriginalStats=Ally->Stats;Ally->Stats.Ranged=true;Ally->Stats.Visual=5;Ally->Cooldown=0;Ally->Windup=-1;Ally->Tick(0);Animation=Ally->Mesh->GetSingleNodeInstance();Check(Animation&&Animation->GetCurrentAsset()&&Animation->GetCurrentAsset()->GetFName()==TEXT("A_SecurityShoot"),TEXT("security shooting animation survives attack startup frame"));Ally->Strike();Check(GunnerShots>0&&!Projectiles.IsEmpty(),TEXT("gunner shooting action launches a damaging projectile"));Ally->Stats=OriginalStats;Ally->Windup=-1;Enemy->SetActorLocation(EnemyStart);
 for(int I=0;I<5;++I)KillEvent.Broadcast(Ally,Enemy);Check(Ally->Stacks==4&&Ally->SizeFactor*Ally->Stats.Scale<=ScaleLimit&&Ally->DamageFactor>2,TEXT("kill hook grows units with four stack and size limits"));
 ActiveCards.AddUnique(3);RebuildRules();ProjectileLimit=2;for(int I=0;I<5;++I)Fire(Ally,Enemy);Check(Projectiles.Num()==2,TEXT("projectile pool recycles oldest at hard cap"));auto* Reused=Projectiles[0].Get();auto* Older=Projectiles[1].Get();Reused->Deactivate();Fire(Ally,Enemy);Check(Projectiles.Last()==Reused&&Projectiles[0]==Older,TEXT("reused projectile becomes newest in recycle order"));auto* Shot=Projectiles[0].Get();const float ShotDamage=Shot->Damage;Shot->PreviousVelocity=FVector(300,0,-300);FHitResult Surface;Surface.ImpactNormal=FVector::UpVector;Shot->Contact(nullptr,this,nullptr,FVector::ZeroVector,Surface);Check(Shot->Bounces==2&&FMath::IsNearlyEqual(Shot->Damage,ShotDamage*1.2f),TEXT("bounce hook reflects shot and increases damage"));const float Z=Enemy->GetActorLocation().Z;Enemy->Receive(Ally,1,FVector(60000,0,60000));
 Enemy->Body->AddAngularImpulseInRadians(FVector(200000,200000,0));
 FTimerHandle Physics;GetWorldTimerManager().SetTimer(Physics,FTimerDelegate::CreateWeakLambda(this,[this,Ally,Enemy,Z,Check,Fail](){
  Check(Enemy->Body->GetUpVector().Z>.999&&WorstLiveTilt<2,TEXT("living capsules stay upright under hits and angular impulses"));
  Check(Enemy->GetActorLocation().Z>Z+10&&Enemy->Stagger>0,TEXT("real impulse lifts and staggers the opponent"));Check(Enemy->Body->GetPhysicsLinearVelocity().Size()<=VelocityLimit+1,TEXT("rigid body velocity stays within safety limit"));const int SavedRoster=Roster.Num(),SavedGold=Coins;Complete(false);const int Remaining=Lives;Complete(false);Check(!Selecting&&Remaining==2&&Lives==Remaining&&Coins==SavedGold,TEXT("defeat consumes exactly one life without duplicate rewards"));Retry();Check(Phase==EKTWRoguePhase::Deployment&&Living(0)==SavedRoster&&Units[0]->Health==Units[0]->MaxHealth&&Units[0]->Stacks==0,TEXT("retry restores roster full health and resets battle growth"));
  ActiveCards={4,7};RebuildRules();StartBattle();auto* Victim=Units[0].Get();auto* Friend=Units[1].Get();Friend->SetActorLocation(Victim->GetActorLocation()+FVector(150,0,0));const float HP=Friend->Health;AKTWRogueUnit* NearEnemy=nullptr;for(auto U:Units)if(U->Team==1){NearEnemy=U;break;}NearEnemy->SetActorLocation(Victim->GetActorLocation()+FVector(200,100,0));const float EnemyHP=NearEnemy->Health;Victim->Die(nullptr);Victim->Die(nullptr);Check(Friend->Health==HP&&NearEnemy->Health<EnemyHP,TEXT("death blast damages enemy while safe card protects allies"));Complete(false);Retry();ActiveCards.Empty();RebuildRules();ProjectileLimit=150;
  bool Flow=true;for(int W=0;W<5;++W){StartBattle();for(auto U:Units)if(U->Team==1&&U->Alive)U->Die(Units[0]);Tick(0);Flow&=Won&&Coins>=0;if(W<4){Flow&=Phase==EKTWRoguePhase::Result;Action(TEXT("Draft"));Flow&=Phase==EKTWRoguePhase::Draft&&Choices.Num()==3&&Choices[0]!=Choices[1]&&Choices[1]!=Choices[2]&&Choices[0]!=Choices[2];const int Card=Choices[0];Action(FString::Printf(TEXT("Card%d"),Card));const int Num=ActiveCards.Num();Action(FString::Printf(TEXT("Card%d"),Card));Flow&=Phase==EKTWRoguePhase::Shop&&ActiveCards.Num()==Num&&Shop.Num()==4;Buy(Shop[0]);NextWave();Flow&=Phase==EKTWRoguePhase::Deployment&&Wave==W+1;}else Flow&=Phase==EKTWRoguePhase::Victory;}
  Check(Flow&&ActiveCards.Num()==4,TEXT("all five waves follow reward unique draft shop deployment victory flow"));
  Phase=EKTWRoguePhase::Battle;Lives=1;Complete(false);Check(Phase==EKTWRoguePhase::GameOver&&Lives==0,TEXT("last defeat enters terminal game over"));
  Wave=0;ActiveCards.Empty();Prepare();StartBattle();for(auto U:Units)U->Die(nullptr);Tick(0);Check(Won,TEXT("same step mutual annihilation favours player"));
  Wave=0;Prepare();StartBattle();Elapsed=Deadline;Tick(0);Check(Phase!=EKTWRoguePhase::Battle,TEXT("deadline resolves a surviving stalemate"));
  UE_LOG(LogTemp,Display,TEXT("KTW ROGUE COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);
 }),.25f,false);
#endif
}

void AKTWRogueRun::RunDeploymentTests(){
#if !UE_BUILD_SHIPPING
 int Failures=0;auto Check=[&](bool OK,const TCHAR* Label){Failures+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW DEPLOY %s: %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!ConfigOK){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 Check(Buttons.ContainsByPredicate([](const FKTWRogueButton& B){return B.Id==TEXT("Buy5")&&B.Enabled;}),TEXT("visible deployment panel offers affordable gunner"));
 Action(TEXT("Buy5"));Check(Roster.Num()==4&&Roster.Last()==5&&Coins==80,TEXT("gunner purchase button adds exactly one soldier"));
 TArray<FVector> BeforeEnemies;for(auto Unit:Units)if(Unit->Team==1)BeforeEnemies.Add(Unit->GetActorLocation());
 int Index=0;for(auto Unit:Units)if(Unit->Team==0){
  FVector2D Cursor;const bool Projected=PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(),Cursor);ClickWorld(PC,Cursor,true,false);
  Check(Projected&&Selected.Num()==1&&Selected[0]==Unit,TEXT("left click selects individual allied soldier"));
  const FVector Point(-1000,-550+Index*280,0);PC->ProjectWorldLocationToScreen(Point,Cursor);ClickWorld(PC,Cursor,true,false);
  UE_LOG(LogTemp,Display,TEXT("KTW DEPLOY placement desired=%s actual=%s stored=%s"),*Point.ToString(),*Unit->GetActorLocation().ToString(),*Positions[Unit->RosterSlot].ToString());
  // UE deprojection truncates to an integer screen pixel; verify within one pixel.
  FVector2D PlacedScreen;PC->ProjectWorldLocationToScreen(Positions[Unit->RosterSlot],PlacedScreen);
  Check(FVector2D::Distance(PlacedScreen,Cursor)<1.5&&FVector::Dist2D(Unit->GetActorLocation(),Positions[Unit->RosterSlot])<.01,TEXT("left ground click places selected soldier within one screen pixel"));++Index;
 }
 auto* Picked=Selected.Num()?Selected[0].Get():nullptr;const FVector Saved=Picked?Picked->GetActorLocation():FVector::ZeroVector;
 FVector2D Invalid;PC->ProjectWorldLocationToScreen(FVector(1000,650,0),Invalid);ClickWorld(PC,Invalid,true,false);
 Check(Picked&&Picked->GetActorLocation().Equals(Saved),TEXT("enemy half cannot move an allied deployment"));
 const auto SavedPositions=Positions;Prepare();bool Stable=Positions==SavedPositions;int EnemyIndex=0;for(auto Unit:Units)if(Unit->Team==1)Stable&=Unit->GetActorLocation().Equals(BeforeEnemies[EnemyIndex++]);Check(Stable,TEXT("retry preserves player placement and identical enemy formation"));
 UE_LOG(LogTemp,Display,TEXT("KTW DEPLOY COMPLETE failures=%d"),Failures);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
#endif
}

void UKTWSession::RunModeTest(){
#if !UE_BUILD_SHIPPING
 auto Check=[this](bool OK,const TCHAR* Label){ModeTestFailures+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW MODE %s: %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));};
 if(!Ready()){FTimerHandle T;GetWorld()->GetTimerManager().SetTimer(T,this,&UKTWSession::RunModeTest,1.f,false);return;}
 if(ModeTestStage==0){Check(InMenu,TEXT("mode selector starts on main menu"));Check(Buttons.ContainsByPredicate([](const FKTWMenuButton& B){return B.Id==TEXT("ModeStreet");})&&Buttons.ContainsByPredicate([](const FKTWMenuButton& B){return B.Id==TEXT("ModeRogue");}),TEXT("both visible mode buttons registered"));Action(TEXT("ModeRogue"));Check(ModeChoice==1,TEXT("rule run selected"));ModeTestStage=1;Action(TEXT("Play"));return;}
 if(ModeTestStage==1){auto* Run=AKTWRogueRun::Find(GetWorld());Check(!InMenu&&ModeChoice==1&&Run&&Run->ConfigOK&&!AKTWMatch::Find(GetWorld()),TEXT("play opens isolated rule arena with external data"));ModeTestStage=2;Action(TEXT("Menu"));return;}
 if(ModeTestStage==2){Check(InMenu&&ModeChoice==1&&AKTWMatch::Find(GetWorld()),TEXT("return to menu preserves choice and restores street backdrop"));Action(TEXT("ModeStreet"));Check(ModeChoice==0,TEXT("street mode can be reselected"));ModeTestStage=3;Action(TEXT("Play"));return;}
 Check(!InMenu&&ModeChoice==0&&AKTWMatch::Find(GetWorld())&&AKTWStreetWar::Find(GetWorld())&&!AKTWRogueRun::Find(GetWorld()),TEXT("street mode entry restores original gameplay systems"));UE_LOG(LogTemp,Display,TEXT("KTW MODE COMPLETE failures=%d"),ModeTestFailures);FPlatformMisc::RequestExitWithStatus(false,ModeTestFailures?1:0);
#endif
}
