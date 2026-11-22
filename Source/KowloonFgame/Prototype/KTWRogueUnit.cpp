#include "KTWRogue.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodyInstance.h"

AKTWRogueUnit::AKTWRogueUnit(){PrimaryActorTick.bCanEverTick=true;Body=CreateDefaultSubobject<UCapsuleComponent>(TEXT("PhysicalBody"));RootComponent=Body;Body->InitCapsuleSize(36,88);Body->SetCollisionProfileName(TEXT("PhysicsActor"));Body->SetCollisionObjectType(ECC_PhysicsBody);Body->SetEnableGravity(false);Body->SetUseCCD(true);Mesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Character"));Mesh->SetupAttachment(Body);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetRelativeLocation(FVector(0,0,-88));Mesh->SetRelativeRotation(FRotator(0,-90,0));}
void AKTWRogueUnit::BeginPlay(){Super::BeginPlay();if(!Run||!Run->SoldierTypes.IsValidIndex(Type)){Destroy();return;}Stats=Run->SoldierTypes[Type];Health=MaxHealth=Stats.HP*(Team?Run->DifficultyScale:1);const TCHAR* Roles[]={TEXT("Runner"),TEXT("Knife"),TEXT("Pipe"),TEXT("Thrower"),TEXT("Boss")};FString Path=Stats.Visual==5?TEXT("/Game/KTW/GameplayModels/SK_Security.SK_Security"):FString::Printf(TEXT("/Game/KTW/Characters/UserNPC/SK_%s_%s.SK_%s_%s"),Team?TEXT("Red"):TEXT("Blue"),Roles[FMath::Clamp(Stats.Visual,0,4)],Team?TEXT("Red"):TEXT("Blue"),Roles[FMath::Clamp(Stats.Visual,0,4)]);Mesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,*Path));if(Stats.Visual==5){auto* Gun=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Gun);Gun->SetupAttachment(Mesh,TEXT("hand_r"));Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/KTW/GameplayModels/SM_Pistol.SM_Pistol")));Gun->SetRelativeRotation(FRotator(0,-90,0));Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);Gun->RegisterComponent();}Mesh->PlayAnimation(LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle")),true);SetActorScale3D(FVector(Stats.Scale));Body->SetMassOverrideInKg(NAME_None,60*Stats.Mass,true);Body->SetAngularDamping(3);Body->SetLinearDamping(1.2f);Activate(false);}
void AKTWRogueUnit::Activate(bool On){
 Body->SetSimulatePhysics(false);
 Body->SetCollisionEnabled(On?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::QueryOnly);
 // Living soldiers use animation for locomotion. Contact impulses may translate them,
 // but must never tip their capsule over or replace walking with rolling.
 Body->BodyInstance.bLockXRotation=true;Body->BodyInstance.bLockYRotation=true;Body->BodyInstance.bLockZRotation=false;
 Body->SetWorldRotation(FRotator(0,GetActorRotation().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
 Body->SetConstraintMode(EDOFMode::SixDOF);
 Body->SetEnableGravity(false);
 if(On){Body->SetSimulatePhysics(true);Body->SetMassOverrideInKg(NAME_None,60*Stats.Mass*MassFactor*(Team?Run->Rules.EnemyMass:1),true);Body->SetPhysicsLinearVelocity(FVector::ZeroVector);Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);}
 Cooldown=.1f;Windup=-1;
}
void AKTWRogueUnit::OrderMove(FVector P){Windup=-1;Attacked=false;Goal=P;MovingOrder=true;Holding=false;Target=nullptr;}
void AKTWRogueUnit::OrderAttack(AKTWRogueUnit* Enemy){if(Enemy&&Enemy->Alive&&Enemy->Team!=Team){Windup=-1;Attacked=false;Target=Enemy;MovingOrder=false;Holding=false;}}
void AKTWRogueUnit::Tick(float D){
 Super::Tick(D);if(!Run)return;
 if(!Alive){
  if(Run->Phase==EKTWRoguePhase::Battle&&Body->IsSimulatingPhysics()){Body->AddForce(FVector(0,0,-980*Run->Rules.Gravity*Body->GetMass()));Body->SetPhysicsLinearVelocity(Body->GetPhysicsLinearVelocity().GetClampedToMaxSize(Run->VelocityLimit));}
  CorpseAge+=D;if(CorpseAge>5){SetActorHiddenInGame(true);Body->SetSimulatePhysics(false);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);}return;
 }
 if(Run->Phase!=EKTWRoguePhase::Battle)return;
 Run->WorstLiveTilt=FMath::Max(Run->WorstLiveTilt,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(Body->GetUpVector().Z),-1.f,1.f))));
 RecentHitAge+=D;if(GetActorLocation().Z<-1000){Die(RecentHitAge<8?RecentAttacker.Get():nullptr);return;}
 const float Mass=Body->GetMass();Body->AddForce(FVector(0,0,-980*Run->Rules.Gravity*Mass));
 FVector V=Body->GetPhysicsLinearVelocity();if(V.Size()>Run->VelocityLimit){V=V.GetClampedToMaxSize(Run->VelocityLimit);Body->SetPhysicsLinearVelocity(V);}
 Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
 auto Face=[&](FVector Direction){const float Yaw=Direction.IsNearlyZero()?GetActorRotation().Yaw:FMath::FixedTurn(GetActorRotation().Yaw,Direction.Rotation().Yaw,540*D);Body->SetWorldRotation(FRotator(0,Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);};
 auto Locomotion=[&](bool Walking){auto* Anim=LoadObject<UAnimSequence>(nullptr,Walking?TEXT("/Game/Mannequin/Animations/ThirdPersonWalk.ThirdPersonWalk"):TEXT("/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle"));if(Anim&&(!Mesh->GetSingleNodeInstance()||Mesh->GetSingleNodeInstance()->GetCurrentAsset()!=Anim))Mesh->PlayAnimation(Anim,true);Mesh->SetPlayRate(Walking?FMath::Clamp(float(V.Size2D())/160.f,.7f,2.2f):1.f);if(Walking)++Run->WalkingSamples;};
 Stagger=FMath::Max(0.f,Stagger-D);Cooldown-=D;
 if(Stagger>0){Windup=-1;Face(FVector::ZeroVector);Locomotion(false);return;}
 if(!IsValid(Target)||!Target->Alive)Target=nullptr;
 if(!Target){float Best=MAX_flt;for(auto U:Run->Units)if(IsValid(U)&&U->Alive&&U->Team!=Team){const float Dist=FVector::DistSquared2D(GetActorLocation(),U->GetActorLocation());if(Dist<Best){Best=Dist;Target=U;}}}
 if(Windup>=0){
  Face(Target?(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D():FVector::ZeroVector);
  Body->AddForce(-FVector(V.X,V.Y,0)*Mass*8);
  Windup+=D;if(!Attacked&&Windup>=.18f){Attacked=true;Strike();}
  if(Windup>=(Stats.Visual==5?.85f:.65f)){Windup=-1;Locomotion(false);}return;
 }
 FVector Wanted=FVector::ZeroVector;const FVector Here=GetActorLocation();const float Dist=Target?FVector::Dist2D(Here,Target->GetActorLocation()):MAX_flt;
 if(MovingOrder){if(FVector::Dist2D(Here,Goal)>65)Wanted=(Goal-Here).GetSafeNormal2D();else{MovingOrder=false;Holding=true;}}
 if(!MovingOrder&&Target){
  if(Dist>Stats.Range+40&&!Holding)Wanted=(Target->GetActorLocation()-Here).GetSafeNormal2D();
  else if(Dist<=Stats.Range+40&&Cooldown<=0){
   Cooldown=Stats.Cooldown;Windup=0;Attacked=false;++Run->AttackSamples;
   auto* Anim=LoadObject<UAnimSequence>(nullptr,Stats.Ranged?(Stats.Visual==5?TEXT("/Game/KTW/Animations/A_SecurityShoot.A_SecurityShoot"):TEXT("/Game/KTW/Animations/A_StreetThrow.A_StreetThrow")):TEXT("/Game/KTW/Animations/A_StreetStrike.A_StreetStrike"));Mesh->SetPlayRate(1);if(Anim)Mesh->PlayAnimation(Anim,false);
  }
 }
 FVector Separation=FVector::ZeroVector;for(auto U:Run->Units)if(IsValid(U)&&U!=this&&U->Alive&&U->Team==Team){const FVector Diff=Here-U->GetActorLocation();const float Distance=Diff.Size2D();if(Distance>1&&Distance<110*Stats.Scale)Separation+=Diff.GetSafeNormal2D()*(1-Distance/(110*Stats.Scale));}
 if(Windup<0)Locomotion(V.Size2D()>45&&!Wanted.IsNearlyZero());
 if(!Wanted.IsNearlyZero())Wanted=(Wanted+Separation*.5f).GetSafeNormal2D();
 const float Steering=Windup>=0?8.f:Run->Rules.Friction<.1f?1.1f:6.f;
 Body->AddForce((Wanted*Stats.Speed-FVector(V.X,V.Y,0))*Mass*Steering);
 Face(Target&&!MovingOrder?(Target->GetActorLocation()-Here).GetSafeNormal2D():Wanted);
}
void AKTWRogueUnit::Strike(){if(!Run||!Target||!Target->Alive||!Alive)return;if(Stats.Ranged){if(Stats.Visual==5)++Run->GunnerShots;Run->Fire(this,Target);return;}const FVector Dir=(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();TArray<FHitResult> Hits;FCollisionQueryParams Q;Q.AddIgnoredActor(this);GetWorld()->SweepMultiByObjectType(Hits,GetActorLocation(),GetActorLocation()+Dir*(Stats.Range+40),FQuat::Identity,FCollisionObjectQueryParams(ECC_PhysicsBody),FCollisionShape::MakeSphere(45*Stats.Scale),Q);TSet<AKTWRogueUnit*> HitUnits;for(const auto& H:Hits)if(auto* U=Cast<AKTWRogueUnit>(H.GetActor()))if(U->Alive&&U->Team!=Team)HitUnits.Add(U);if(FVector::Dist(GetActorLocation(),Target->GetActorLocation())<=Stats.Range+70)HitUnits.Add(Target);for(auto* U:HitUnits){const FVector Push=((U->GetActorLocation()-GetActorLocation()).GetSafeNormal2D()+FVector(0,0,.35f)).GetSafeNormal();U->Receive(this,Stats.Damage*DamageFactor*(Team?Run->DifficultyScale:1),Push*Stats.Impulse*Run->Rules.Knockback);}}
void AKTWRogueUnit::Receive(AKTWRogueUnit* SourceUnit,float Damage,FVector Impulse){if(!Alive||!Run||Run->Phase!=EKTWRoguePhase::Battle)return;if(SourceUnit&&SourceUnit->Team==0&&Team==0&&Run->Rules.FriendlySafe)return;if(SourceUnit&&SourceUnit->Team!=Team){RecentAttacker=SourceUnit;RecentHitAge=0;}Health-=Damage*(1+FMath::Max(0.f,Run->Elapsed-Run->CollapseAt)*.05f);if(!Impulse.IsNearlyZero()){Body->AddImpulse(Impulse);if(Impulse.Size()/FMath::Max(Body->GetMass(),1.f)>Stats.Balance*20){Stagger=FMath::Max(Stagger,.4f);}}if(Health<=0)Die(SourceUnit);}
void AKTWRogueUnit::Die(AKTWRogueUnit* Killer){if(!Alive)return;Alive=false;Health=0;Windup=-1;Mesh->bPauseAnims=true;Body->SetConstraintMode(EDOFMode::None);if(Body->IsSimulatingPhysics())Body->AddAngularImpulseInRadians(GetActorRightVector()*Body->GetMass()*120);if(Run){Run->DeathEvent.Broadcast(this,Killer);if(Killer&&Killer!=this&&Killer->Team!=Team)Run->KillEvent.Broadcast(Killer,this);}}
void AKTWRogueUnit::Grow(){if(!Run||Stacks>=Run->Rules.GrowthStacks||Team!=0)return;++Stacks;SizeFactor=FMath::Min(Run->ScaleLimit/Stats.Scale,SizeFactor*Run->Rules.GrowthScale);MassFactor*=Run->Rules.GrowthMass;DamageFactor*=Run->Rules.GrowthDamage;const float OldHeight=Body->GetScaledCapsuleHalfHeight();SetActorScale3D(FVector(Stats.Scale*SizeFactor));AddActorWorldOffset(FVector(0,0,Body->GetScaledCapsuleHalfHeight()-OldHeight),false,nullptr,ETeleportType::TeleportPhysics);Body->SetMassOverrideInKg(NAME_None,60*Stats.Mass*MassFactor,true);}

AKTWRogueProjectile::AKTWRogueProjectile(){PrimaryActorTick.bCanEverTick=true;Body=CreateDefaultSubobject<USphereComponent>(TEXT("PhysicalShot"));RootComponent=Body;Body->InitSphereRadius(12);Body->SetCollisionProfileName(TEXT("PhysicsActor"));Body->SetUseCCD(true);Body->SetNotifyRigidBodyCollision(true);Body->OnComponentHit.AddDynamic(this,&AKTWRogueProjectile::Contact);auto* Art=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));Art->SetupAttachment(Body);Art->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Art->SetRelativeScale3D(FVector(.24f));Art->SetCollisionEnabled(ECollisionEnabled::NoCollision);Art->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/KTW/Materials/M_NeonGold.M_NeonGold")));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
void AKTWRogueProjectile::Launch(AKTWRogueUnit* Unit,AKTWRogueUnit* Enemy){Source=Unit;Run=Unit->Run;Active=true;Age=HitDelay=0;Damage=Unit->Stats.Damage*Unit->DamageFactor*(Unit->Team?Run->DifficultyScale:1);Impulse=Unit->Stats.Impulse;Bounces=Run->Rules.Bounces;FVector Dir=(Enemy->GetActorLocation()-Unit->GetActorLocation()).GetSafeNormal2D();SetActorLocation(Unit->GetActorLocation()+Dir*(36*Unit->Stats.Scale*Unit->SizeFactor+40)+FVector(0,0,60));const float Travel=FMath::Clamp(FVector::Dist2D(GetActorLocation(),Enemy->GetActorLocation())/(Unit->Stats.Visual==5?1800.f:1050.f),.2f,1.4f);const FVector End=Enemy->GetActorLocation()+Enemy->Body->GetPhysicsLinearVelocity()*Travel*.3f;FVector Velocity=(End-GetActorLocation())/Travel;Velocity.Z+=.5f*980*Run->Rules.Gravity*Travel;PreviousVelocity=Velocity;SetActorHiddenInGame(false);Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Body->SetSimulatePhysics(true);Body->SetEnableGravity(false);Body->SetMassOverrideInKg(NAME_None,2,true);Body->SetPhysicsLinearVelocity(Velocity);}
void AKTWRogueProjectile::Deactivate(){Active=false;Body->SetSimulatePhysics(false);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorHiddenInGame(true);}
void AKTWRogueProjectile::Tick(float D){Super::Tick(D);if(!Active||!Run)return;if(Run->Phase!=EKTWRoguePhase::Battle){Deactivate();return;}Age+=D;HitDelay-=D;Body->AddForce(FVector(0,0,-980*Run->Rules.Gravity*Body->GetMass()));PreviousVelocity=Body->GetPhysicsLinearVelocity().GetClampedToMaxSize(Run->VelocityLimit);Body->SetPhysicsLinearVelocity(PreviousVelocity);if(Age>10||GetActorLocation().Z<-1000)Deactivate();}
void AKTWRogueProjectile::Contact(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,FVector,const FHitResult& Hit){if(!Active||!Run||HitDelay>0||Other==Source.Get())return;HitDelay=.07f;if(auto* Unit=Cast<AKTWRogueUnit>(Other))if(Unit->Alive){const FVector Push=(PreviousVelocity.GetSafeNormal()+FVector(0,0,.25f)).GetSafeNormal();Unit->Receive(Source.Get(),Damage,Push*Impulse*Run->Rules.Knockback);}if(Bounces<=0){Deactivate();return;}--Bounces;Damage*=Run->Rules.BounceDamage;const FVector Reflected=FMath::GetReflectionVector(PreviousVelocity,Hit.ImpactNormal);Body->SetPhysicsLinearVelocity(Reflected);}
