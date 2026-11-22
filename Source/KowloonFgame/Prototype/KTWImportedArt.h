#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KTWGeneration.h"
#include "KTWImportedArt.generated.h"
class UStaticMesh;
UCLASS(BlueprintType)
class KOWLOONFGAME_API UKTWImportedArtSet : public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<int32,TObjectPtr<UStaticMesh>> Meshes;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 TextureLimit=2048;
};
struct KOWLOONFGAME_API FKTWImportedArt {
 static bool Enabled();
 static UKTWImportedArtSet* Data();
 static bool Replaces(const FKTWPlacement& P,EKTWLayer Layer);
 static void Render(AKTWStreetDirector* Director,const TArray<FKTWPlacement>& Layout,EKTWLayer Layer);
 static bool Configure(class UInstancedStaticMeshComponent* Component,int32 Kind);
 static FTransform Fit(const FTransform& Target,const UStaticMesh* Mesh);
#if WITH_EDITOR
 static int32 BuildContent();
#endif
};
