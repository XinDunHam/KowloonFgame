#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KTWWorkGuard.generated.h"

/** Replans stalled movement without minting resources, teleporting units or erasing player orders. */
UCLASS() class KOWLOONFGAME_API UKTWWorkGuard:public UActorComponent{
 GENERATED_BODY()
public:
 UKTWWorkGuard();
 virtual void TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F)override;
 bool IsDetouring()const;
 void CancelRecovery();
 bool RequestPath(FVector Goal,bool Immediate);
 void PathFailed(bool Failed){bPathFailed=Failed;}
 UPROPERTY(VisibleAnywhere) int32 Recoveries=0;
private:
 FVector LastPosition=FVector::ZeroVector,RecoveryGoal=FVector::ZeroVector;
 FVector RequestedGoal=FVector::ZeroVector;double RequestedAt=-10;bool bPathFailed=false;
 double LastMotionAt=-1,DetourUntil=-1,LastAttempt=-10;
};
