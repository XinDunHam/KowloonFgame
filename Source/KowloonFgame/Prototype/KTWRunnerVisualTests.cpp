#include "KTWRunnerVisuals.h"
#include "KTWPrototype.h"
#include "KTWGold.h"
#include "KTWAI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "HAL/IConsoleManager.h"

void FKTWRunnerVisuals::RunIntegrationTest(UWorld* World)
{
#if !UE_BUILD_SHIPPING
 int32 Failures=0;auto Check=[&](bool OK,const TCHAR* Name){Failures+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW RUNNER %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto Spawn=[World](EKTWRole Role,int32 Faction,float X){auto* U=World->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(FVector(X,0,120)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->UnitRole=Role;U->Faction=Faction;U->bEnableCombat=false;U->bAutoPatrol=false;U->FinishSpawning(FTransform(FVector(X,0,120)));return U;};
 auto* Blue=Spawn(EKTWRole::Runner,0,-500);auto* Red=Spawn(EKTWRole::Runner,1,500);auto* Knife=Spawn(EKTWRole::Knife,0,1500);
 Check(IsActive(Blue)&&IsActive(Red),TEXT("spawned red and blue Runners use prototype"));
 auto* Config=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));
 Check(Config&&Knife->GetMesh()->GetSkeletalMeshAsset()==Config->SoldierMesh.LoadSynchronous(),TEXT("other roles retain existing soldier mesh"));
 auto* Main=Blue->GetMesh();auto* Mesh=Main->GetSkeletalMeshAsset();auto* Instance=Main->GetAnimInstance();
 Check(Instance!=nullptr,TEXT("existing animation blueprint instantiated"));
 Check(Main->GetPhysicsAsset()!=nullptr,TEXT("existing compatible physics asset retained for defeat ragdoll"));
 Check(Main->GetBoneIndex(TEXT("head"))!=INDEX_NONE&&Main->GetBoneIndex(TEXT("hand_r"))!=INDEX_NONE,TEXT("label and equipment bones available"));
 const int32 Identity=Main->GetMaterialIndex(TEXT("M_Identity"));
 Check(Identity!=INDEX_NONE&&Main->GetMaterial(Identity)!=Red->GetMesh()->GetMaterial(Identity),TEXT("factions have distinct identity materials"));
 bool SkinOK=true;for(int32 I=0;I<Main->GetNumMaterials();++I)if(I!=Identity)SkinOK&=Main->GetMaterial(I)==Red->GetMesh()->GetMaterial(I)&&Main->GetMaterial(I)==Mesh->GetMaterials()[I].MaterialInterface;
 Check(SkinOK,TEXT("skin clothing and accessories keep authored materials"));
 auto Count=[&](){TArray<USkeletalMeshComponent*> Cs;Blue->GetComponents(Cs);int32 N=0;for(auto* C:Cs)if(C->ComponentHasTag(TEXT("KTWRunnerModule"))&&C->IsVisible()){++N;Check(C->LeaderPoseComponent.Get()==Main,TEXT("module follows main animation pose"));}return N;};
 Check(Count()==0,TEXT("L1 has no upgrade modules"));
 auto* Carrier=Blue->FindComponentByClass<UKTWGoldCarrier>();Carrier->Carried=17;Blue->bCarryingGold=true;Blue->Order=EKTWUnitOrder::Defend;
 const auto ActorId=Blue->GetUniqueID();const auto* Controller=Blue->GetController();const FVector Scale=Blue->GetActorScale3D(),Destination=Blue->CommandDestination();const float HalfHeight=Blue->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();Blue->Health-=13;
 Blue->GainExperience(60);Check(Blue->Level==2&&Count()==3,TEXT("60 XP switches to L2 modules"));
 Check(FMath::IsNearlyEqual(Blue->Stats.MaxHealth-Blue->Health,13),TEXT("existing promotion health rule preserved"));
 Blue->GainExperience(100);Check(Blue->Level==3&&Count()==4,TEXT("160 cumulative XP replaces strap with L3 harness"));
 const float HP=Blue->Health,XP=Blue->Experience;Refresh(Blue);Refresh(Blue);
 Check(Count()==4&&Blue->GetUniqueID()==ActorId&&Blue->GetController()==Controller&&Blue->GetActorScale3D().Equals(Scale)&&Blue->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()==HalfHeight,TEXT("refresh is idempotent and preserves actor controller and capsule"));
 Check(Blue->Health==HP&&Blue->Experience==XP&&Carrier->Carried==17&&Blue->Order==EKTWUnitOrder::Defend&&Blue->CommandDestination().Equals(Destination),TEXT("appearance refresh preserves health XP cargo and orders"));
 Check(Main->GetSkeletalMeshAsset()==Mesh&&Main->GetAnimInstance()==Instance,TEXT("promotion does not replace body or restart animation blueprint"));
 auto* Cargo=NewObject<UStaticMeshComponent>(Blue);Cargo->SetupAttachment(Main,TEXT("spine_03"));Check(ConfigureCargo(Blue,Cargo)&&Cargo->GetStaticMesh()!=nullptr&&Cargo->GetRelativeScale3D().Equals(FVector::OneVector),TEXT("real cargo bag uses centimeter scale and reference-bone attachment"));
 auto* Toggle=IConsoleManager::Get().FindConsoleVariable(TEXT("ktw.RunnerPrototype"));Toggle->Set(0);auto* Fallback=Spawn(EKTWRole::Runner,0,2500);Check(!IsActive(Fallback)&&Fallback->GetMesh()->GetSkeletalMeshAsset()==Config->SoldierMesh.LoadSynchronous(),TEXT("prototype can be disabled for new spawns"));Toggle->Set(1);
 Carrier->Carried=0;Blue->bCarryingGold=false;
 UE_LOG(LogTemp,Display,TEXT("KTW RUNNER COMPLETE failures=%d"),Failures);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
#endif
}

