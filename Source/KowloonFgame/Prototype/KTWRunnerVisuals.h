#pragma once
#include "CoreMinimal.h"
class AKTWTestUnit;
class USkeletalMesh;
class UStaticMeshComponent;

/** Prototype appearance only. Never changes gameplay state or actor/capsule scale. */
struct FKTWRunnerVisuals
{
 static USkeletalMesh* ResolveMesh(const AKTWTestUnit* Unit);
 static bool IsActive(const AKTWTestUnit* Unit);
 static void Refresh(AKTWTestUnit* Unit);
 static bool ConfigureCargo(AKTWTestUnit* Unit,UStaticMeshComponent* Cargo);
 static void RunIntegrationTest(class UWorld* World);
};
