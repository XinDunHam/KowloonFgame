#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "KTWPrototype.h"
#include "KTWGold.generated.h"

class UKTWGoldCarrier;
class AKTWMatch;
struct FKTWPlacement;

UENUM(BlueprintType)
enum class EKTWGoldTask : uint8 { Idle, Seeking, PickingUp, Returning, Delivering, Healing, Waiting, Raiding };

UCLASS()
class KOWLOONFGAME_API AKTWGoldPile : public AActor {
 GENERATED_BODY()
 friend class AKTWGoldSystem;
 friend class UKTWGoldCarrier;
public:
 AKTWGoldPile();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Amount=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Lane=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 SourceIndex=-1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Capacity=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Unrewarded=0;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UInstancedStaticMeshComponent> Bars;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Marker;
 void UpdateVisuals();
private:
 TWeakObjectPtr<UKTWGoldCarrier> ReservedBy;
 double ReservedUntil=0;
};

USTRUCT()
struct FKTWPendingGoldDrop {
 GENERATED_BODY()
 UPROPERTY() int32 Amount=0;
 UPROPERTY() int32 Unrewarded=0;
 UPROPERTY() int32 Lane=0;
 UPROPERTY() int32 Faction=0;
 UPROPERTY() FVector Position=FVector::ZeroVector;
};

/** Authoritative gold ledger. All transfers preserve amount and unawarded transport provenance. */
UCLASS()
class KOWLOONFGAME_API AKTWGoldSystem : public AActor {
 GENERATED_BODY()
 friend class UKTWGoldCarrier;
public:
 AKTWGoldSystem();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 static AKTWGoldSystem* Find(const UWorld* World);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<TObjectPtr<AKTWGoldPile>> Piles;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 GeneratedGold=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float NextRefresh=20;
 UPROPERTY(VisibleAnywhere) TArray<FKTWPendingGoldDrop> PendingDrops;
 bool GoldConserved() const;
 void CreatePCGSources(const TArray<FKTWPlacement>& Layout);
 bool SourcesReady=false;
 int32 GroundGold() const;
 int32 CarriedGold(int32 Faction=-1) const;
 bool IsSafe(FVector Point,int32 Faction) const;
 AKTWGoldPile* FindGold(UKTWGoldCarrier* Carrier,float MaxDistance) const;
 bool Available(AKTWGoldPile* Pile,UKTWGoldCarrier* Carrier) const;
 bool Reserve(AKTWGoldPile* Pile,UKTWGoldCarrier* Carrier,bool Started=false);
 void Release(UKTWGoldCarrier* Carrier);
 void Drop(UKTWGoldCarrier* Carrier);
private:
 bool CommitPickup(UKTWGoldCarrier* Carrier,AKTWGoldPile* Pile);
 bool CommitDelivery(UKTWGoldCarrier* Carrier);
 bool ProjectDrop(FVector From,int32 Faction,FVector& Point) const;
 bool MaterializeDrop(const FKTWPendingGoldDrop& Drop);
 void RunGoldTests();
 void RunPhase3Tests();
 void RunAutomaticTransportTest();
 int32 TestFailures=0;
 void TestCheck(bool OK,const TCHAR* Name);
};

UCLASS()
class KOWLOONFGAME_API UKTWGoldCarrier : public UActorComponent {
 GENERATED_BODY()
 friend class AKTWGoldSystem;
public:
 UKTWGoldCarrier();
 virtual void TickComponent(float DeltaSeconds,ELevelTick Type,FActorComponentTickFunction* Fn) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Carried=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Unrewarded=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DeliveredTotal=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float TransportExperience=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EKTWGoldTask Task=EKTWGoldTask::Idle;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ChannelProgress=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bDeferredOrder=false;
 bool ControlsMovement() const { return bControlsMovement; }
 bool RequestPickup(AKTWGoldPile* Pile);
 bool RequestRaid();
 bool UnderThreat() const;
 bool IsRaiding() const {return bRaid;}
 static FVector VaultPosition(int32 Faction,int32 Lane);
 bool DeferOrder(EKTWUnitOrder NewOrder,FVector Point,AActor* Target);
 void OnAcceptedOrder(EKTWUnitOrder NewOrder);
 void OnMoveArrived(){if(Carried>0)bReturning=true;}
 void Interrupt();
 void Defeated();
 float PickupSeconds()const;
 float ChannelFraction()const;
 float ChannelSecondsLeft()const;
 double LastTransferAt=-100;
 int32 LastTransferAmount=0;
 bool bLastBanked=false;
 bool IsLiving() const;
 const TCHAR* TaskLabel() const;
private:
 TWeakObjectPtr<AKTWGoldPile> TargetPile;
 TWeakObjectPtr<AKTWGoldSystem> System;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> CargoVisual;
 TWeakObjectPtr<AActor> DeferredTarget;
 EKTWUnitOrder DeferredOrder=EKTWUnitOrder::Advance;
 EKTWUnitOrder SavedOrder=EKTWUnitOrder::Advance;
 FVector SavedPoint=FVector::ZeroVector;
 TWeakObjectPtr<AActor> SavedTarget;
 bool bSavedTask=false;
 FVector DeferredPoint=FVector::ZeroVector;
 double LastDamage=-100,NextAttempt=0,NextMove=0,NoSafeGoldSince=-1;
 uint64 DamageFrame=MAX_uint64;
 bool bControlsMovement=false,bManualPickup=false,bReturning=false,bHealing=false;
 float ChannelRequired=0;
 double ChannelStartedAt=-1;
 float ChannelFinishTime=0;
 void AdvanceChannel(float Duration,float MatchTime);
 bool bRaid=false,bRaidAfterDelivery=false;
 double RaidEmptySince=-1;
 AKTWTestUnit* Unit() const;
 void SetCarried(int32 Amount,int32 NewUnrewarded);
 void MoveTo(FVector Point,float Acceptance=65);
 void ResetChannel();
 void ResumeAfterDelivery();
};
