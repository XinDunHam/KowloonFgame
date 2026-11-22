#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "Engine/DataAsset.h"
#include "KTWPrototype.generated.h"

UENUM(BlueprintType)
enum class EKTWBossTask : uint8 { Sit, Follow, Escort, Return, Attack };
UENUM(BlueprintType)
enum class EKTWRole : uint8 { Runner, Knife, Pipe, Thrower };
UENUM(BlueprintType)
enum class EKTWUnitOrder : uint8 { Advance, Move, Attack, Defend, Escort, Retreat, Pickup, PickupGear };
USTRUCT(BlueprintType)
struct FKTWUnitStats {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Price=80;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float MaxHealth=80;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Damage=8;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Interval=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Speed=460;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Armor=0;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Range=150;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Capacity=60;
};
UCLASS(BlueprintType)
class KOWLOONFGAME_API UKTWUnitBalance : public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FKTWUnitStats> Units;
 static TArray<FKTWUnitStats> Defaults();
};
UCLASS()
class KOWLOONFGAME_API AKTWTestUnit : public ACharacter
{
 GENERATED_BODY()
 friend class UKTWCombatAction;
 friend class UKTWGoldCarrier;
public:
 AKTWTestUnit();
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test") int32 Faction=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test") int32 Lane=1;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test") bool bRunner=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test") bool bCarryingGold=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Test") bool bAura=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Test") float DamageMultiplier=1;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test") bool bAutoPatrol=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat") EKTWRole UnitRole=EKTWRole::Pipe;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") FKTWUnitStats Stats;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") float Health=80;
 UPROPERTY(VisibleAnywhere) bool bLaneDeparturePending=false;
 FVector LaneEntry;
 float NextFireHit=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat") bool bEnableCombat=true;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Order") EKTWUnitOrder Order=EKTWUnitOrder::Advance;
 UFUNCTION(BlueprintCallable,Category="Order") bool IssueOrder(EKTWUnitOrder NewOrder,FVector Location,AActor* Target=nullptr);
 virtual float TakeDamage(float Amount,struct FDamageEvent const& Event,AController* Instigator,AActor* Causer) override;
 UFUNCTION(BlueprintCallable,Category="Test") void SetAura(bool Enabled);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Experience=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Level=1;
 void GainExperience(float Amount);
 TMap<TWeakObjectPtr<AKTWTestUnit>,TArray<FVector2D>> DamageCredits;
 UPROPERTY(VisibleAnywhere) bool bHasLocalWork=false;
 FVector WorkAnchor,WorkPatrolGoal;
 double NextWorkPatrol=0;int32 WorkPatrolStep=0;
 void BeginLocalWork();
 bool LocalWorkStep();
 FVector CommandDestination() const {return OrderAnchor;}
 AActor* CommandTarget() const {return OrderTarget.Get();}

private:
 float BaseHealth=80,BaseDamage=8;
 FTimerHandle PatrolTimer;
 int32 PatrolIndex=1;
 bool bPatrolReturning=false;
 FVector OrderAnchor;
 FVector CombatAnchor;
 TWeakObjectPtr<AActor> OrderTarget;
 TWeakObjectPtr<AActor> Enemy;
 FVector LastSeenEnemy=FVector::ZeroVector;
 double LastSeenAt=-100;
 double LastAttack=-10;
 double StunnedUntil=0;
 double StunImmuneUntil=0;
 int32 PipeHits=0;
 bool CombatStep();
 void PatrolStep();
};
UCLASS()
class KOWLOONFGAME_API AKTWBoss : public ACharacter
{
 GENERATED_BODY()
public:
 AKTWBoss();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual float TakeDamage(float Amount,struct FDamageEvent const& Event,AController* Instigator,AActor* Causer) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") int32 Faction=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float MaxHealth=320;
 float NextFireHit=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float Armor=.10f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float AuraRadius=800;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float RecoverySeconds=45;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float RunnerSpeedBonus=.12f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Boss") float CombatDamageBonus=.10f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Boss") float Health=320;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Boss") bool bRecovering=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Boss") float RecoveryRemaining=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Boss") EKTWBossTask Task=EKTWBossTask::Sit;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Boss") bool bLowHealthRetreat=false;
 UFUNCTION(BlueprintCallable,Category="Boss") bool IssueTask(EKTWBossTask NewTask,FVector Anchor,AActor* Target);
 UFUNCTION(BlueprintCallable,Category="Boss") void ApplyTestDamage(float RawDamage);
 void ReadyForFinalPhase();
 bool IssuePlayerOrder(FVector Point,AActor* Target=nullptr);
 UPROPERTY(VisibleAnywhere) bool bPlayerOrdered=false;
 FVector CommandDestination()const{return TaskAnchor;}
 AActor* CommandTarget()const{return FollowTarget.Get();}
 void PublishDestination(FVector Point,bool Immediate=false);
 bool PlaceForPreparation(FVector Point);
 bool AcceptsAutomaticTask()const;
 UPROPERTY(VisibleAnywhere) bool bHoldPosition=false;
 void ChooseAutomaticWork();
 double NextWorkDecision=0;int32 WorkPatrolStep=0;

private:
 FTimerHandle LogicTimer;
 FVector Home;
 FVector TaskAnchor;
 TWeakObjectPtr<AActor> FollowTarget;
 TMap<TWeakObjectPtr<AKTWTestUnit>,double> AuraLastSeen;
 double RecoveryUntil=0;
 double LastHit=-100;
 double LastBossAttack=-10;
 bool bRetreatArrived=false,bEscortingCargo=false;
 int32 FollowLane=1;
 double AutomaticTaskAfter=0;
 void UpdateLogic();
 void ClearAura();
 void EnterRecovery();
};
UCLASS()
class KOWLOONFGAME_API AKTWCameraPawn : public APawn
{
 GENERATED_BODY()
public:
 AKTWCameraPawn();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> Camera;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bStreetView=false;
 FVector FocusGoal=FVector::ZeroVector;
 float TopHeight=26000,OrbitDistance=4500,OrbitPitch=-8,OrbitYaw=-90;
 bool bReady=false;
 FVector FreePosition=FVector(0,0,1450),CursorAnchor=FVector::ZeroVector,FollowOffset=FVector::ZeroVector;
 bool bHasCursorAnchor=false,bStreetTransition=false;
 FVector TransitionCruisePoint=FVector::ZeroVector;
 bool CursorWorldAt(FVector2D Screen,FVector& Point)const;
 bool SampleCursor();
 void EnterStreetAt(FVector Point);
 bool FindStreetLanding(FVector Requested,FVector& Eye,float& Yaw)const;
 float StreetEyeHeight(FVector Point)const;
 void ApplyLookDelta(float X,float Y);
 void SetLookCaptured(bool On);
 bool UpdateLookGesture(float Seconds);
 bool bLookCaptured=false,bRightPending=false,bRightDragged=false;
 double RightPressedAt=0;
 FVector2D RightPress=FVector2D::ZeroVector,LookRestore=FVector2D::ZeroVector;
 void TranslateFree(FVector Direction,float Seconds);
 void RunContinuityTest();
 TWeakObjectPtr<ACharacter> FollowTarget;
 UFUNCTION(BlueprintCallable) void SetStreetView(bool On,bool FrameSelection=true);
 void PanTo(FVector Point);
 bool FrameSelection(bool Follow=false);
 FVector DesiredViewLocation()const;
 FVector ResolveViewLocation(FVector Point)const;
 void RunImmersionTest();
};
UCLASS()
class KOWLOONFGAME_API AKTWPrototypeController : public APlayerController
{
 GENERATED_BODY()
public:
 AKTWPrototypeController();
 virtual void PlayerTick(float DeltaSeconds) override;
 UPROPERTY() TObjectPtr<AKTWBoss> SelectedBoss;
 UPROPERTY() TObjectPtr<AKTWTestUnit> SelectedUnit;
 UPROPERTY() TArray<TObjectPtr<AKTWTestUnit>> SelectedUnits;
 FVector2D SelectionStart;
 bool bSelecting=false,bRightSelecting=false;
 bool SelectAtScreen(FVector2D Start,FVector2D End,bool Add,bool OnlyFriendlyClick=false);
 bool IsWorldCursor(FVector2D Cursor)const;
 bool bUIConsumed=false;
 UFUNCTION(BlueprintCallable) bool CommandAtWorld(FVector Point,AActor* Context=nullptr);
 FVector LastCommandPoint=FVector::ZeroVector;
 double LastCommandTime=-100;
 static float RecruitScale(int32 Height){return FMath::Clamp((Height-395.f)/485.f,.45f,1.f);}
 bool HandleRecruitClick(float X,float Y,int32 Width,int32 Height);
 bool SelectLaneKey(FKey Key);
 ACharacter* PickCharacterAt(FVector2D Cursor)const;
 bool GroundAtCursor(FVector& Point)const;
 class AKTWFieldObject* PickFieldAt(FVector2D Cursor)const;

};
UCLASS()
class KOWLOONFGAME_API AKTWPrototypeHUD : public AHUD
{
 GENERATED_BODY()
public: virtual void DrawHUD() override;
 UCanvas* GameCanvas() const{return Canvas;}
 void DrawText(const FString& Text,FLinearColor Color,float X,float Y,UFont* Font=nullptr,float Scale=1.f,bool ScalePosition=false);
};
UCLASS()
class KOWLOONFGAME_API AKTWPrototypeGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AKTWPrototypeGameMode();
 virtual void BeginPlay() override;
 UPROPERTY(EditDefaultsOnly,Category="Prototype") bool bSpawnTestUnits=true;
};

