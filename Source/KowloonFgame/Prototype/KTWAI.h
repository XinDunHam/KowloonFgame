#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "Engine/DataAsset.h"
#include "KTWAI.generated.h"
class USkeletalMesh;
class UAnimInstance;

/** Editor-configurable visual contract: replace the mesh and matching animation BP together. */
UCLASS(BlueprintType)
class KOWLOONFGAME_API UKTWCharacterVisuals : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bActionMontages=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> BossMesh;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> SoldierMesh;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftClassPtr<UAnimInstance> AnimationClass;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FRotator MeshRotation=FRotator(0,-90,0);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float BossScale=1.05f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float SoldierScale=.95f;
 /** Readability multiplier for rendered characters only; gameplay distances remain in world units. */
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Readability",meta=(ClampMin="1.0",ClampMax="20.0")) float OverviewScaleMultiplier=7.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Movement",meta=(ClampMin="0.5",ClampMax="3.0")) float MovementSpeedMultiplier=1.75f;
};
UCLASS()
class KOWLOONFGAME_API AKTWBossAIController : public AAIController
{
 GENERATED_BODY()
protected: virtual void OnPossess(APawn* InPawn) override;
};
UCLASS()
class KOWLOONFGAME_API UBTTask_KTWMoveOrder : public UBTTaskNode
{
 GENERATED_BODY()
public:
 UBTTask_KTWMoveOrder();
 virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory) override;
};
UCLASS()
class KOWLOONFGAME_API UBTDecorator_KTWBossState : public UBTDecorator
{
 GENERATED_BODY()
public:
 UBTDecorator_KTWBossState();
 UPROPERTY(EditAnywhere,Category="State") bool bRecoveryBranch=false;
protected: virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory)const override;
};
