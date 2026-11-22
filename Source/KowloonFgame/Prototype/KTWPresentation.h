#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "KTWPresentation.generated.h"
class AKTWTestUnit;
class UStaticMeshComponent;
UCLASS() class KOWLOONFGAME_API AKTWStreetWeapon:public AActor {
 GENERATED_BODY()
public:
 AKTWStreetWeapon();
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
 UPROPERTY(VisibleAnywhere) int32 Kind=0;
 UPROPERTY(VisibleAnywhere) int32 Lane=0;
 UPROPERTY(VisibleAnywhere) int32 Hits=6;
 UPROPERTY(VisibleAnywhere) bool Equipped=false;
 UPROPERTY(VisibleAnywhere) bool Collected=false;
 void BuildModel();
 bool Accepts(AKTWTestUnit* U)const;
 float Multiplier()const{return Kind==2?1.35f:Kind==1?1.25f:1.2f;}
 void Equip(AKTWTestUnit* U);
 void Drop();
 void Consume();
 TWeakObjectPtr<AKTWTestUnit> ClaimedBy;
 float ClaimHealth=0;double CollectAt=-1;bool ManualClaim=false;
 FVector PickupPoint=FVector::ZeroVector;float ClosestClaimDistance=MAX_flt;double ClaimStarted=-1,ClaimProgressAt=-1,RetryAfter=-1;TWeakObjectPtr<AKTWTestUnit> RejectedBy;
};
UCLASS() class KOWLOONFGAME_API AKTWCivilian:public ACharacter {
 GENERATED_BODY()
public:
 AKTWCivilian();
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 virtual float TakeDamage(float Amount,FDamageEvent const& E,AController* I,AActor* C)override{return 0;}
 UPROPERTY(VisibleAnywhere) bool Panicked=false;
private: double EvacuateAt=-1;
 double NextGoal=0;FVector Destination=FVector::ZeroVector;
};
UCLASS() class KOWLOONFGAME_API AKTWStreetLife:public AActor {
 GENERATED_BODY()
public:
 AKTWStreetLife();
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 UPROPERTY() TArray<TObjectPtr<AKTWStreetWeapon>> GroundWeapons;
 static AKTWStreetLife* Find(const UWorld* World);
 AKTWStreetWeapon* Equipment(AKTWTestUnit* Unit)const;
 bool TryWeapon(AKTWTestUnit* Unit);
 AKTWStreetWeapon* Throwable(AKTWTestUnit* U)const;
 bool RequestGear(AKTWTestUnit* U,AKTWStreetWeapon* W,bool Manual);
 bool Busy(AKTWTestUnit* U)const;
 void ProcessGear(AKTWStreetWeapon* W);
 void CancelGear(AKTWStreetWeapon* W);
 static void Dress(ACharacter* Unit,bool Boss);
 static void Impact(AActor* Target);
private:
 float NextRefill=25;
 int32 LootWave=0;
 double NextFootstep=0;
 TMap<TWeakObjectPtr<AKTWTestUnit>,double> GearSearchAt;
 void Refill();
 void RunTests();
};
