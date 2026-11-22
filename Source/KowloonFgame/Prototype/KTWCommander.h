#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KTWPrototype.h"
#include "KTWCommander.generated.h"

struct FKTWContact {
 TWeakObjectPtr<AActor> Actor;
 FVector LastPosition=FVector::ZeroVector;
 EKTWRole Role=EKTWRole::Runner;
 double SeenAt=0;
 bool Boss=false;
};

/** Uses only public score, own units, and line-of-sight contacts. Never writes resource accounts. */
UCLASS()
class KOWLOONFGAME_API AKTWCommander:public AActor {
 GENERATED_BODY()
public:
 AKTWCommander();
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Faction=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Plan=TEXT("PREPARE");
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Purchases=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DefendOrders=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 EscortOrders=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 RaidOrders=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 BossOrders=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 VisibleContacts=0;
 void Decide(bool AllowRecruit=true);
 void Observe();
 EKTWRole DesiredRecruit()const;
 int32 KnownContacts()const{return Contacts.Num();}
 static bool FixtureMode();
private:
 TArray<FKTWContact> Contacts;
 TMap<TWeakObjectPtr<AKTWTestUnit>,double> AssignedUntil;
 double NextRecruit=0,NextDecision=0;
 int32 RecruitmentTurn=0;
 void Recruit(int32 PreferredLane,bool Emergency);
 bool Available(AKTWTestUnit* Unit,bool Emergency)const;
 void RunCommandTests();
 void RunCommanderTests();
 void RunAIPlayTest();
};
