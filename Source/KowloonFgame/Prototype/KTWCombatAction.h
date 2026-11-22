#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KTWCombatAction.generated.h"
class UAnimInstance;class UAnimSequence;class UAnimMontage;

/** Small action playback contract: animation wind-up, one validated hit, then locomotion restoration. */
UCLASS()
class KOWLOONFGAME_API UKTWCombatAction : public UActorComponent {
 GENERATED_BODY()
public:
 UKTWCombatAction();
 bool Attack(AActor* Target,float Damage,float Range,bool Throw,bool Gun=false);
 bool PlayPickup(float Seconds);
 bool ThrowConsumable(AActor* Target);
 bool PlayAction(UAnimSequence* Anim,float Rate);
 void CancelInteraction();
 void CancelAction(){if(bPlaying)Restore();}
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bPlaying=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 ConfirmedHits=0;
private:
 TWeakObjectPtr<AActor> Victim;
 TWeakObjectPtr<class AKTWStreetWeapon> UsedWeapon;
 UPROPERTY() TSubclassOf<UAnimInstance> Locomotion;
 float Elapsed=0,HitDamage=0,HitRange=150;
 UPROPERTY() TObjectPtr<UAnimMontage> Montage;
 bool bFireThrow=false,bRanged=false,bProjectileShown=false;
 bool bResolved=false,bGunShooting=false;
 bool bInteraction=false;
 float InteractionSeconds=.6f;
 void Restore();
};
