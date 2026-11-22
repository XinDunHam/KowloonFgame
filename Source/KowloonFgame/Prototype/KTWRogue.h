#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "KTWRogue.generated.h"
class UCapsuleComponent;class USphereComponent;class USkeletalMeshComponent;class UPrimitiveComponent;class AKTWPrototypeController;class AKTWPrototypeHUD;class AKTWCameraPawn;class AKTWRogueRun;
struct FKTWRogueSoldier {FString Id,Name;int32 Visual=0,Cost=40;float HP=70,Mass=1,Speed=420,Range=130,Cooldown=.9f,Damage=12,Impulse=18000,Balance=12,Scale=1;bool Ranged=false;};
struct FKTWRogueCard {FString Id,Name,Description,NameCN,DescriptionCN;float Gravity=1,Knockback=1,Friction=.6f,EnemyMass=1,DeathRadius=0,DeathDamage=0,DeathImpulse=0,GrowthScale=1,GrowthMass=1,GrowthDamage=1,BounceDamage=1;int32 GrowthStacks=0,Bounces=0;bool FriendlySafe=false;float Health=1,Speed=1,Melee=1,Ranged=1,Haste=1,Range=1,Regen=0,Leech=0,Resistance=1,AllyMass=1,EnemySpeed=1;};
struct FKTWRogueWave {FString Title,Formation;TArray<int32> Soldiers;};
struct FKTWRogueRules {float Gravity=1,Knockback=1,Friction=.6f,EnemyMass=1,DeathRadius=0,DeathDamage=0,DeathImpulse=0,GrowthScale=1,GrowthMass=1,GrowthDamage=1,BounceDamage=1;int32 GrowthStacks=0,Bounces=0;bool FriendlySafe=false;float Health=1,Speed=1,Melee=1,Ranged=1,Haste=1,Range=1,Regen=0,Leech=0,Resistance=1,AllyMass=1,EnemySpeed=1;};
UENUM() enum class EKTWRoguePhase:uint8 {Deployment,Battle,Result,Draft,Shop,Victory,GameOver};
UCLASS() class KOWLOONFGAME_API AKTWRogueUnit:public APawn {
 GENERATED_BODY()
public:
 AKTWRogueUnit();virtual void BeginPlay()override;virtual void Tick(float D)override;
 UPROPERTY() TObjectPtr<UCapsuleComponent> Body;
 UPROPERTY() TObjectPtr<USkeletalMeshComponent> Mesh;
 UPROPERTY() TObjectPtr<AKTWRogueRun> Run;
 UPROPERTY() TObjectPtr<AKTWRogueUnit> Target;
 UPROPERTY() TWeakObjectPtr<AKTWRogueUnit> RecentAttacker;
 float RecentHitAge=100;
 int32 Team=0,Type=0,RosterSlot=-1,Stacks=0;FKTWRogueSoldier Stats;
 float Health=70,MaxHealth=70,MassFactor=1,DamageFactor=1,SizeFactor=1,Cooldown=0,Windup=-1,Stagger=0,CorpseAge=0;
 bool Alive=true,MovingOrder=false,Holding=false,Attacked=false;FVector Goal=FVector::ZeroVector;
 void Activate(bool On);void OrderMove(FVector P);void OrderAttack(AKTWRogueUnit* Enemy);
 void Receive(AKTWRogueUnit* Source,float Damage,FVector Impulse);void Die(AKTWRogueUnit* Killer);void Grow();void Strike();
};
UCLASS() class KOWLOONFGAME_API AKTWRogueProjectile:public AActor {
 GENERATED_BODY()
public:
 AKTWRogueProjectile();virtual void Tick(float D)override;
 UPROPERTY() TObjectPtr<USphereComponent> Body;
 UPROPERTY() TObjectPtr<AKTWRogueRun> Run;
 UPROPERTY() TWeakObjectPtr<AKTWRogueUnit> Source;
 bool Active=false;float Damage=0,Impulse=0,Age=0,HitDelay=0;int32 Bounces=0;FVector PreviousVelocity;
 void Launch(AKTWRogueUnit* OwnerUnit,AKTWRogueUnit* Enemy);void Deactivate();
 UFUNCTION() void Contact(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
};
DECLARE_MULTICAST_DELEGATE_TwoParams(FKTWRogueDeath,AKTWRogueUnit*,AKTWRogueUnit*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FKTWRogueKill,AKTWRogueUnit*,AKTWRogueUnit*);
struct FKTWRogueButton {FString Id;FVector4 Rect;bool Enabled=true;};
UCLASS() class KOWLOONFGAME_API AKTWRogueRun:public AActor {
 GENERATED_BODY()
public:
 AKTWRogueRun();virtual void BeginPlay()override;virtual void Tick(float D)override;
 static AKTWRogueRun* Find(const UWorld* World);
 bool LoadData();void BuildArena();void Prepare();void StartBattle();void Complete(bool Victory);void Action(const FString& Id);void RebuildRules();void DraftRules();bool RefreshRules();void RefreshShop(bool Paid);bool Buy(int32 Type);void Retry();void NextWave();bool Place(AKTWRogueUnit* Unit,FVector Point);void AutoDeploy();
 void CommandSelected(FVector Point,AKTWRogueUnit* Enemy=nullptr);
 TArray<FVector> EnemyFormation(int32 WaveIndex)const;int32 EnemyBudget(int32 WaveIndex)const;
 bool PlaceSquad(const TArray<AKTWRogueUnit*>& Squad,const TArray<FVector>& Goals);
 AKTWRogueUnit* PickUnit(APlayerController* PC,FVector2D Cursor)const;void ClickWorld(APlayerController* PC,FVector2D Cursor,bool Primary,bool Add);
 void Handle(AKTWPrototypeController* PC);void Draw(AKTWPrototypeHUD* HUD);void UpdateCamera(AKTWCameraPawn* Cam,float D);
 void ApplyDeath(AKTWRogueUnit* Victim,AKTWRogueUnit* Killer);void ApplyKill(AKTWRogueUnit* Killer,AKTWRogueUnit* Victim);void Fire(AKTWRogueUnit* Source,AKTWRogueUnit* Enemy);
 AKTWRogueUnit* Spawn(int32 Type,int32 Team,FVector Position,int32 Slot=-1);int32 Living(int32 Team)const;void RunTests();void RunDeploymentTests();
 UPROPERTY() TArray<TObjectPtr<AKTWRogueUnit>> Units;
 UPROPERTY() TArray<TObjectPtr<AKTWRogueProjectile>> Projectiles;
 UPROPERTY() TObjectPtr<class UPhysicalMaterial> FloorMaterial;
 TArray<FKTWRogueSoldier> SoldierTypes;TArray<FKTWRogueCard> CardTypes;TArray<FKTWRogueWave> Waves;FKTWRogueRules Rules;
 TArray<int32> Roster,ActiveCards,Choices,Shop;TArray<FVector> Positions;TArray<TWeakObjectPtr<AKTWRogueUnit>> Selected;TArray<FKTWRogueButton> Buttons;
 FKTWRogueDeath DeathEvent;FKTWRogueKill KillEvent;
 EKTWRoguePhase Phase=EKTWRoguePhase::Deployment;int32 Wave=0,Lives=3,Coins=200;float Elapsed=0,Speed=1,CameraDistance=4600,CameraYaw=-90,CameraPitch=-58;FVector CameraFocus=FVector::ZeroVector;
 UPROPERTY() bool FreeRuleRefresh=true;
 bool ConfigOK=false,Won=false,Selecting=false;FVector2D SelectionStart;FString Notice;FRandomStream Random;
 float WorstLiveTilt=0;int32 WalkingSamples=0,AttackSamples=0,GunnerShots=0;
 int32 StartingCoins=200,StartingLives=3,MaxRoster=20,BaseReward=180,WaveReward=40,SurvivorReward=15,RefreshCost=20,ProjectileLimit=150;float CollapseAt=45,Deadline=65,VelocityLimit=4500,ScaleLimit=2.5f,DifficultyScale=1;
};
