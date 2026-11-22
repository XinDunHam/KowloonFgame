#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "KTWStreetWar.generated.h"
class AKTWTestUnit;
class AKTWHoloHUD;
class AKTWGoldPile;
class UStaticMeshComponent;

UENUM() enum class EKTWFieldKind:uint8 { Base, Truck, Shelter, Diamond };
UCLASS() class KOWLOONFGAME_API AKTWFieldObject:public AActor {
 GENERATED_BODY()
public:
 AKTWFieldObject();
 virtual void BeginPlay()override;
 virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer)override;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
 UPROPERTY() EKTWFieldKind Kind=EKTWFieldKind::Shelter;
 UPROPERTY() int32 Faction=-1;
 UPROPERTY() float Health=1200;
 UPROPERTY() float MaxHealth=1200;
 UPROPERTY() bool DiamondTruck=false;
 UPROPERTY() bool Open=false;
 float Progress=0;
 UPROPERTY() TArray<TObjectPtr<class AKTWSecurity>> Guards;
 UPROPERTY() TArray<TObjectPtr<AKTWTestUnit>> Occupants;
 bool Guarded()const;
};
UCLASS() class KOWLOONFGAME_API AKTWSecurity:public ACharacter {
 GENERATED_BODY()
public:
 AKTWSecurity();
 virtual void BeginPlay()override;
 virtual void Tick(float D)override;
 virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer)override;
 UPROPERTY() float Health=180;
 UPROPERTY() bool Gun=false;
 UPROPERTY() TObjectPtr<AKTWFieldObject> Truck;
 float Cooldown=0;
 FVector GuardOffset=FVector::ZeroVector;
};
USTRUCT() struct FKTWFieldTask {
 GENERATED_BODY()
 UPROPERTY() TObjectPtr<AKTWFieldObject> Target;
 float Progress=0,HealthAtStart=0;
 bool Inside=false;
};
UCLASS() class KOWLOONFGAME_API AKTWStreetWar:public AActor {
 GENERATED_BODY()
public:
 AKTWStreetWar();
 virtual void BeginPlay()override;
 virtual void Tick(float D)override;
 static AKTWStreetWar* Find(const UWorld* W);
 static bool Enabled(const UWorld* W);
 static float Limit(const UWorld* W);
 static float DamageScale(const AActor* Source);
 void RunEngagementTests();
 void RunOrdersLootTests();
 static bool Visible(const UWorld* W,const AActor* A,int32 Team=0);
 static bool Busy(const AKTWTestUnit* U);
 static bool Concealed(const AActor* A);
 bool PointVisible(FVector P,int32 Team)const;
 bool Interact(AKTWTestUnit* U,AKTWFieldObject* Target);
 void Cancel(AKTWTestUnit* U);
 void Draw(AKTWHoloHUD* H);
 void BossDown(int32 Team);
 void Resolve();
 void SpawnConvoy(int32 Index);
 void Initialize();
 AKTWGoldPile* AddGold(FVector P,int32 Amount);
 FVector EventPoint(int32 Index)const;
 UPROPERTY() TArray<TObjectPtr<AKTWFieldObject>> Objects;
 UPROPERTY() TMap<TObjectPtr<AKTWTestUnit>,FKTWFieldTask> Tasks;
 UPROPERTY() TObjectPtr<AKTWTestUnit> DiamondCarrier;
 UPROPERTY() TObjectPtr<AKTWFieldObject> Diamond;
 UPROPERTY() TObjectPtr<AKTWFieldObject> Bases[2];
 UPROPERTY() TObjectPtr<class UTexture2D> VisionTexture;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> FogMaterial;
 UPROPERTY() TObjectPtr<class UPostProcessComponent> Fog;
 float DiamondProgress=0;
 float DiamondHealth=0;
 float ArmAt=250;
 bool Late=false,Initialized=false;
 bool BossLost[2]={false,false};
 bool Spawned[4]={false,false,false,false};
 float VisionClock=0,DecisionClock=0;
 bool Soak=false;float SoakCheckpoint=0;
 FString EventText;
 TArray<uint8> Vision;
 void UpdateVision();
 void RunTests();
};
