#pragma once
#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "GameFramework/Actor.h"
#include "KTWGeneration.generated.h"

class UPCGComponent;
class USplineComponent;
class UInstancedStaticMeshComponent;
UENUM(BlueprintType)
enum class EKTWLayer : uint8 { Facades, Barriers, Weapons, GoldSources, Atmosphere };
USTRUCT(BlueprintType)
struct FKTWPlacement
{
 GENERATED_BODY()
 UPROPERTY() FTransform Transform;
 UPROPERTY() int32 Kind = 0;
 UPROPERTY() int32 BuildingStyleId = -1;
 UPROPERTY() int32 Lane = -1;
};

/** A source node shared by three independent PCG Graph assets. No gameplay random stream is consumed. */
UCLASS(BlueprintType, ClassGroup=(Procedural))
class KOWLOONFGAME_API UKTWPCGSettings : public UPCGSettings
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon") EKTWLayer Layer = EKTWLayer::Facades;
#if WITH_EDITOR
 virtual FName GetDefaultNodeName() const override { return TEXT("KowloonLayout"); }
 virtual FText GetDefaultNodeTitle() const override { return FText::FromString(TEXT("Kowloon constrained layout")); }
 virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
#endif
protected:
 virtual TArray<FPCGPinProperties> InputPinProperties() const override { return {}; }
 virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
 virtual FPCGElementPtr CreateElement() const override;
};

UCLASS()
class KOWLOONFGAME_API AKTWStreetDirector : public AActor
{
 GENERATED_BODY()
public:
 AKTWStreetDirector();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USplineComponent> Boundary;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPCGComponent> Facades;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPCGComponent> Barriers;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPCGComponent> Weapons;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPCGComponent> GoldSources;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPCGComponent> Atmosphere;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Foundation;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RoadMarkings;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> UpperRoad;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> MainRoad;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> LowerRoad;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> LinkAlley;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Generation") int32 MapSeed = 5990;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Generation", meta=(ClampMin="0",ClampMax="2")) int32 WeaponsPerLane = 2;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Generation") bool bFacadesEnabled = true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Generation") bool bBarriersEnabled = true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Generation") bool bWeaponsEnabled = true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Kowloon|Readability",meta=(ClampMin="1",ClampMax="20")) float StreetWeaponVisualScale=7.125f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Kowloon|Validation") FString ValidationReport;
 UFUNCTION(CallInEditor, BlueprintCallable, Category="Kowloon") void Regenerate();
 UFUNCTION(CallInEditor, BlueprintCallable, Category="Kowloon") void NextSeed();
 UFUNCTION(CallInEditor, BlueprintCallable, Category="Kowloon") bool ValidateLayout();
 TArray<FKTWPlacement> BuildLayer(EKTWLayer Layer, int32 SeedOverride = INDEX_NONE) const;
 void BuildFoundation();
 void AssignGraphs();
 bool IsGenerating() const;
 UFUNCTION(BlueprintPure, Category="Kowloon|Routes") static FVector BasePosition(int32 Faction);
 UFUNCTION(BlueprintPure, Category="Kowloon|Routes") static TArray<FVector> Route(int32 Lane, int32 Faction=0);
 static TArray<FVector> Junctions();
 static TArray<FVector> GoldAnchors();
 static bool IsRoadPosition(FVector Point, float ExtraClearance=0);
 static bool IsScaffoldLot(FVector Center);
 static TArray<FVector> BuildingLots();
};
