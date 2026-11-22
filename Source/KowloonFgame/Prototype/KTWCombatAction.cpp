#include "KTWCombatAction.h"
#include "KTWPrototype.h"
#include "KTWAI.h"
#include "KTWMatch.h"
#include "KTWStreetWar.h"
#include "KTWPresentation.h"
#include "KTWDynamics.h"
#include "KTWSession.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
UKTWCombatAction::UKTWCombatAction(){PrimaryComponentTick.bCanEverTick=true;}
bool UKTWCombatAction::PlayAction(UAnimSequence* Anim,float Rate){auto* C=Cast<ACharacter>(GetOwner());if(!C||!Anim)return false;auto* Instance=C->GetMesh()->GetAnimInstance();if(Instance)Locomotion=Instance->GetClass();Montage=nullptr;auto* V=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));if(V&&V->bActionMontages&&Instance)Montage=Instance->PlaySlotAnimationAsDynamicMontage(Anim,TEXT("DefaultSlot"),.10f,.14f,Rate,1);if(!Montage){C->GetMesh()->PlayAnimation(Anim,false);if(auto* Single=C->GetMesh()->GetSingleNodeInstance())Single->SetPlayRate(Rate);}return true;}
bool UKTWCombatAction::Attack(AActor* Target,float Damage,float Range,bool Throw,bool Gun){auto* C=Cast<ACharacter>(GetOwner());if(!C||!IsValid(Target)||bPlaying||Damage<=0)return false;auto* Anim=LoadObject<UAnimSequence>(nullptr,Gun?TEXT("/Game/KTW/Animations/A_SecurityShoot.A_SecurityShoot"):Throw?TEXT("/Game/KTW/Animations/A_StreetThrow.A_StreetThrow"):TEXT("/Game/KTW/Animations/A_StreetStrike.A_StreetStrike"));if(!Anim)return false;UsedWeapon.Reset();if(auto* U=Cast<AKTWTestUnit>(C))if(auto* Life=AKTWStreetLife::Find(GetWorld()))if(auto* W=Life->Equipment(U)){Damage*=W->Multiplier();UsedWeapon=W;}Victim=Target;HitDamage=Damage;HitRange=Range;Elapsed=0;bResolved=false;bPlaying=true;bInteraction=false;bFireThrow=false;bRanged=Throw||Gun;bGunShooting=Gun;bProjectileShown=false;C->SetActorRotation(FRotator(0,(Target->GetActorLocation()-C->GetActorLocation()).Rotation().Yaw,0));if(auto* AI=Cast<AAIController>(C->GetController())){auto* U=Cast<AKTWTestUnit>(C);if(!U||U->UnitRole!=EKTWRole::Knife)AI->StopMovement();}PlayAction(Anim,1);return true;}
bool UKTWCombatAction::ThrowConsumable(AActor* Target){auto* U=Cast<AKTWTestUnit>(GetOwner());auto* L=AKTWStreetLife::Find(GetWorld());if(!U||!L||!L->Throwable(U)||U->bCarryingGold||!IsValid(Target)||FVector::Dist2D(U->GetActorLocation(),Target->GetActorLocation())>1100)return false;if(!Attack(Target,1,1100,true))return false;bFireThrow=true;return true;}
void UKTWCombatAction::Restore(){if(auto* C=Cast<ACharacter>(GetOwner()))if(!C->GetMesh()->IsSimulatingPhysics()){if(Montage){if(auto* I=C->GetMesh()->GetAnimInstance())I->Montage_Stop(.12f,Montage);}else if(Locomotion)C->GetMesh()->SetAnimInstanceClass(Locomotion);}Montage=nullptr;bPlaying=false;Victim.Reset();bFireThrow=false;}
bool UKTWCombatAction::PlayPickup(float Seconds){auto* C=Cast<ACharacter>(GetOwner());if(!C||bPlaying||Seconds<=0)return false;auto* Anim=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/KTW/Animations/A_StreetPickup.A_StreetPickup"));if(!Anim)return false;bPlaying=true;bInteraction=true;bFireThrow=false;Elapsed=0;InteractionSeconds=Seconds;return PlayAction(Anim,.6f/Seconds);}
void UKTWCombatAction::CancelInteraction(){if(bPlaying&&bInteraction)Restore();}
void UKTWCombatAction::TickComponent(float D,ELevelTick Type,FActorComponentTickFunction* Fn){Super::TickComponent(D,Type,Fn);if(!bPlaying)return;if(auto* U=Cast<AKTWTestUnit>(GetOwner())){if(U->Health<=0){bPlaying=false;Victim.Reset();return;}if(GetWorld()->GetTimeSeconds()<U->StunnedUntil){Restore();return;}}if(auto* B=Cast<AKTWBoss>(GetOwner()))if(B->bRecovering){Restore();return;}if(auto* M=AKTWMatch::Find(GetWorld()))if(!M->CanAct()){Restore();return;}Elapsed+=D;if(bInteraction){if(Elapsed>=InteractionSeconds)Restore();return;}
 auto* Target=Victim.Get();auto* C=Cast<ACharacter>(GetOwner());if(bRanged&&!bGunShooting&&!bFireThrow&&!bProjectileShown&&Elapsed>=.18f){bProjectileShown=true;if(auto* U=Cast<AKTWTestUnit>(C))if(IsValid(Target))AKTWFireBottle::Stone(U,Target);}
 if(!bResolved&&Elapsed>=(bGunShooting?.22f:bRanged&&!bFireThrow?.40f:.18f)){bResolved=true;bool Alive=IsValid(Target);if(auto* U=Cast<AKTWTestUnit>(Target))Alive=U->Health>0;if(auto* B=Cast<AKTWBoss>(Target))Alive=!B->bRecovering;FCollisionQueryParams Q;Q.AddIgnoredActor(GetOwner());Q.AddIgnoredActor(Target);FHitResult H;
  if(Alive&&C&&FVector::Dist2D(C->GetActorLocation(),Target->GetActorLocation())<=HitRange+30&&!GetWorld()->LineTraceSingleByChannel(H,C->GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Q)){
   if(bFireThrow){if(auto* U=Cast<AKTWTestUnit>(C))if(auto* Life=AKTWStreetLife::Find(GetWorld()))if(auto* W=Life->Throwable(U)){AKTWFireBottle::Launch(U,Target->GetActorLocation());W->Consume();}}
   else{const float Actual=UGameplayStatics::ApplyDamage(Target,HitDamage*AKTWStreetWar::DamageScale(C),C->GetController(),C,nullptr);if(Actual>0){++ConfirmedHits;if(UsedWeapon.IsValid()&&UsedWeapon->GetOwner()==C)UsedWeapon->Consume();AKTWStreetLife::Impact(Target);UKTWSession::Cue(this,TEXT("Strike"),.7f,Target->GetActorLocation());if(auto* Source=Cast<AKTWTestUnit>(C))if(Source->UnitRole==EKTWRole::Pipe)if(auto* U=Cast<AKTWTestUnit>(Target)){const double Now=GetWorld()->GetTimeSeconds();if(++Source->PipeHits%3==0&&Now>=U->StunImmuneUntil&&U->Health>0){U->StunnedUntil=Now+.4;U->StunImmuneUntil=Now+3;}}}}
  }
 }
 if(Elapsed>=(bGunShooting?.8f:.6f))Restore();
}
