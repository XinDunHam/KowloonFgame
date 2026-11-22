#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KTWPrototype.h"
#include "KTWMatch.generated.h"

UENUM(BlueprintType)
enum class EKTWMatchPhase : uint8 { Preparation, Battle, Finished };

USTRUCT(BlueprintType)
struct FKTWAccount {
 GENERATED_BODY()
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Cash=600;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Gold=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 TimeIncome=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 BountyIncome=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Purchases=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Refunds=0;
};

USTRUCT()
struct FKTWRecruit {
 GENERATED_BODY()
 UPROPERTY() int32 Faction=0;
 UPROPERTY() int32 Lane=1;
 UPROPERTY() EKTWRole UnitRole=EKTWRole::Runner;
 UPROPERTY() int32 Paid=0;
 UPROPERTY() float Remaining=3;
};

/** Sole writer for phase-one cash transactions. Gold is intentionally read-only until transport is implemented. */
UCLASS()
class KOWLOONFGAME_API AKTWMatch : public AActor {
 GENERATED_BODY()
public:
 AKTWMatch();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 static AKTWMatch* Find(const UWorld* World);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EKTWMatchPhase Phase=EKTWMatchPhase::Preparation;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<FKTWAccount> Accounts;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BattleSeconds=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Feedback;
 UPROPERTY() TArray<FKTWRecruit> Queue;
 UPROPERTY() TMap<TObjectPtr<AKTWTestUnit>,int32> Refundable;
 UPROPERTY() int32 SelectedLane=1;
 UFUNCTION(BlueprintCallable) bool Purchase(int32 Faction,EKTWRole UnitRole,int32 Lane);
 UFUNCTION(BlueprintCallable) bool Refund(AKTWTestUnit* Unit);
 UFUNCTION(BlueprintCallable) bool Deploy(AKTWTestUnit* Unit,FVector Point);
 UFUNCTION(BlueprintCallable) void StartBattle();
 UFUNCTION(BlueprintCallable) void FinishBattle();
 int32 Population(int32 Faction) const;
 int32 QueueCount(int32 Faction,int32 Lane) const;
 bool CashConserved() const;
 bool CanAct() const { return Phase==EKTWMatchPhase::Battle; }
 void RegisterHit(AActor* Victim,int32 AttackerFaction);
 void AwardDefeat(AActor* Victim,int32 VictimFaction,int32 Price,bool Boss=false);
 void NewLife(AActor* Unit);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Winner=-2;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString VictoryReason;
 float HoldStarted[2]={-1,-1};
 void GoldChanged(int32 Team);
 void ResolveVictory();
 float HoldRemaining(int32 Team) const;
 UFUNCTION(BlueprintCallable) bool OpenBox(int32 Faction,int32 Lane);
 float BoxRemaining(int32 Team)const;
 static EKTWRole BoxResult(float Roll,int32 LowStreak);
 int32 LowBoxes[2]={0,0};
 double BoxReadyAt[2]={0,0};
 FRandomStream BoxRandom[2]={FRandomStream(49173),FRandomStream(91337)};
 void RunProgressionTests();
private:
 bool bBoxPurchase=false;
 float IncomeRemainder=0;
 TMap<TWeakObjectPtr<AActor>,double> HostileHits;
 TSet<TWeakObjectPtr<AActor>> RewardedDeaths;
 AKTWTestUnit* SpawnRecruit(const FKTWRecruit& Entry);
 bool FindSpawn(int32 Faction,int32 Lane,FVector& Point) const;
 void RunEconomyTests();
 void RunRestartProbe();
};
