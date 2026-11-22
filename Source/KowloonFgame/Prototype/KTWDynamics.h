#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KTWDynamics.generated.h"
class AKTWTestUnit; class AKTWPrototypeHUD; class UInstancedStaticMeshComponent; class UAudioComponent; class APostProcessVolume; class AExponentialHeightFog;
UCLASS() class KOWLOONFGAME_API AKTWCommandMarker:public AActor{
 GENERATED_BODY()
public:
 AKTWCommandMarker();virtual void Tick(float D)override;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Ring;
 float Age=0;static void Show(UWorld* W,FVector Point,int32 Style=0);
};
UCLASS() class KOWLOONFGAME_API AKTWFireBottle:public AActor{
 GENERATED_BODY()
public:
 AKTWFireBottle();virtual void Tick(float D)override;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Visual;
 TWeakObjectPtr<AKTWTestUnit> Source;FVector Start,End;float Age=0,NextDamage=0;bool Landed=false,Cosmetic=false;TWeakObjectPtr<AActor> StoneTarget;
 static void Launch(AKTWTestUnit* U,FVector Aim);
 static void Stone(AKTWTestUnit* U,AActor* Target);
};
/** Cosmetic seeded fifth PCG system; never changes combat/economy random streams. */
UCLASS() class KOWLOONFGAME_API AKTWWeather:public AActor{
 GENERATED_BODY()
public:
 AKTWWeather();virtual void BeginPlay()override;virtual void Tick(float D)override;
 static AKTWWeather* Find(const UWorld* W);void ApplySeed(int32 Seed);void Draw(AKTWPrototypeHUD* H);
 static const TCHAR* Name(int32 P);UPROPERTY(VisibleAnywhere) int32 Preset=0;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WetRoads;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SnowRoofs;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Lamps;
 UPROPERTY() TArray<TObjectPtr<class UPointLightComponent>> Neon;
 UPROPERTY() TObjectPtr<UAudioComponent> WeatherAudio;
 UPROPERTY() TObjectPtr<APostProcessVolume> Grade;
 UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
 int32 AppliedSeed=INDEX_NONE;FRandomStream Random;float FlashUntil=-1,NextFlash=8;
 float AmbientSun=8;TArray<FVector> Drops;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Lightning;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Splashes;
 UPROPERTY() TObjectPtr<class UPointLightComponent> StormLight;
 UPROPERTY() TObjectPtr<class USkyAtmosphereComponent> Sky;
 UPROPERTY() TObjectPtr<class USoundAttenuation> ThunderAttenuation;
 TArray<float> DropFloors,SplashTimes;
 TArray<FVector> SplashLocations;
 int32 NextSplash=0,LightningEvents=0;
 void ResetDrop(int32 Index,bool Initial=false);
 void BuildPrecipitation();
 void TriggerLightning();
 void RunTests();
};
