#include "KTWRunnerVisuals.h"
#include "KTWPrototype.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"

namespace {
TAutoConsoleVariable<int32> RunnerEnabled(TEXT("ktw.RunnerPrototype"),1,TEXT("Use the Runner prototype for newly spawned Runners; 0 restores the existing soldier mesh."));
const TCHAR* Root=TEXT("/Game/KTW/Characters/RunnerPrototype/");
template<class T> T* Asset(const TCHAR* Name){const FString Path=FString(Root)+Name+TEXT(".")+Name;return LoadObject<T>(nullptr,*Path);}
const FName ModuleTag(TEXT("KTWRunnerModule"));
}
USkeletalMesh* FKTWRunnerVisuals::ResolveMesh(const AKTWTestUnit* Unit)
{
 return Unit&&Unit->UnitRole==EKTWRole::Runner&&RunnerEnabled.GetValueOnGameThread()!=0?Asset<USkeletalMesh>(TEXT("SK_KTW_Runner")):nullptr;
}
bool FKTWRunnerVisuals::IsActive(const AKTWTestUnit* Unit)
{
 return Unit&&Unit->UnitRole==EKTWRole::Runner&&Unit->GetMesh()&&Unit->GetMesh()->GetSkeletalMeshAsset()&&Unit->GetMesh()->GetSkeletalMeshAsset()->GetPathName()==TEXT("/Game/KTW/Characters/RunnerPrototype/SK_KTW_Runner.SK_KTW_Runner");
}
void FKTWRunnerVisuals::Refresh(AKTWTestUnit* Unit)
{
 if(!IsActive(Unit))return;
 auto* Main=Unit->GetMesh();
 const int32 IdentitySlot=Main->GetMaterialIndex(TEXT("M_Identity"));
 if(IdentitySlot!=INDEX_NONE)if(auto* Material=Asset<UMaterialInterface>(Unit->Faction==0?TEXT("M_RunnerIdentity_Blue"):TEXT("M_RunnerIdentity_Red")))Main->SetMaterial(IdentitySlot,Material);
 struct FModule {const TCHAR* Name;int32 Minimum;int32 Maximum;};
 const FModule Modules[]={{TEXT("SKM_KTW_Runner_WristWrap_l"),2,3},{TEXT("SKM_KTW_Runner_WristWrap_r"),2,3},{TEXT("SKM_KTW_Runner_ReinforcedStrap"),2,2},{TEXT("SKM_KTW_Runner_TransportHarness"),3,3},{TEXT("SKM_KTW_Runner_WaistPouch"),3,3}};
 TArray<USkeletalMeshComponent*> Existing;Unit->GetComponents(Existing);
 for(const auto& Entry:Modules){
  USkeletalMeshComponent* Component=nullptr;
  for(auto* Candidate:Existing)if(Candidate->ComponentHasTag(ModuleTag)&&Candidate->GetFName()==FName(Entry.Name)){Component=Candidate;break;}
  const bool Visible=Unit->Level>=Entry.Minimum&&Unit->Level<=Entry.Maximum;
  if(!Component&&Visible){
   auto* Mesh=Asset<USkeletalMesh>(Entry.Name);if(!Mesh)continue;
   Component=NewObject<USkeletalMeshComponent>(Unit,FName(Entry.Name));Unit->AddInstanceComponent(Component);Component->ComponentTags.Add(ModuleTag);
   Component->SetupAttachment(Main);Component->SetSkeletalMeshAsset(Mesh);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetCanEverAffectNavigation(false);
   Component->SetLeaderPoseComponent(Main);Component->bUseAttachParentBound=true;Component->RegisterComponent();
  }
  if(Component)Component->SetVisibility(Visible);
 }
}
bool FKTWRunnerVisuals::ConfigureCargo(AKTWTestUnit* Unit,UStaticMeshComponent* Cargo)
{
 if(!Cargo||!IsActive(Unit))return false;
 auto* Bag=Asset<UStaticMesh>(TEXT("SM_KTW_GoldBag"));if(!Bag)return false;
 Cargo->SetStaticMesh(Bag);Cargo->EmptyOverrideMaterials();
 // The prop was authored in reference mesh coordinates at (0,-16.5,129) cm.
 // Convert this anchor to spine_03 local space using the actual skeleton, not a guessed bone axis.
 const auto& Ref=Unit->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();const int32 Bone=Ref.FindBoneIndex(TEXT("spine_03"));
 if(Bone==INDEX_NONE)return false;
 FTransform BoneCS=Ref.GetRefBonePose()[Bone];
 for(int32 Parent=Ref.GetParentIndex(Bone);Parent!=INDEX_NONE;Parent=Ref.GetParentIndex(Parent))BoneCS=BoneCS*Ref.GetRefBonePose()[Parent];
 Cargo->SetRelativeTransform(FTransform(FQuat::Identity,FVector(0,-16.5,129)).GetRelativeTransform(BoneCS));
 return true;
}
