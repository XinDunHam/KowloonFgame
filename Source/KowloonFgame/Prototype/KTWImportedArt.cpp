#include "KTWImportedArt.h"
#include "Engine/StaticMesh.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "StaticMeshCompiler.h"
#include "HAL/FileManager.h"
#endif
namespace {TAutoConsoleVariable<int32> Imported(TEXT("ktw.ImportedStreetArt"),1,TEXT("Use curated imported Hong Kong meshes; 0 uses legacy art on the next regeneration."));}
UKTWImportedArtSet* FKTWImportedArt::Data(){static TWeakObjectPtr<UKTWImportedArtSet> Cache;if(!Cache.IsValid())Cache=LoadObject<UKTWImportedArtSet>(nullptr,TEXT("/Game/KTW/Data/DA_ImportedHongKong.DA_ImportedHongKong"));return Cache.Get();}
bool FKTWImportedArt::Enabled(){return Imported.GetValueOnGameThread()!=0&&!FParse::Param(FCommandLine::Get(),TEXT("KTWLegacyStreetArt"))&&Data()!=nullptr;}
namespace {
int ResolveKind(const FKTWPlacement& P,EKTWLayer Layer){if(Layer==EKTWLayer::Barriers)return P.Kind==0?300:301;
 if(P.Kind==2&&P.Transform.GetScale3D().Z>5&&P.Transform.GetScale3D().X<.2)return 202;
 if(P.Kind==1&&P.BuildingStyleId<0)return -1; // Retain car glazing when the pack has no vehicle equivalent.
 if(P.Kind==21&&P.Transform.GetScale3D().Z<3)return -1;
 if(P.Kind==42)return P.Transform.GetScale3D().Z<.1?42:-1;
 return P.Kind;}
}
bool FKTWImportedArt::Replaces(const FKTWPlacement& P,EKTWLayer Layer){return Enabled()&&(Layer==EKTWLayer::Facades||Layer==EKTWLayer::Barriers)&&Data()->Meshes.Contains(ResolveKind(P,Layer));}
FTransform FKTWImportedArt::Fit(const FTransform& Target,const UStaticMesh* Mesh){const FBox B=Mesh->GetBoundingBox();const FVector Size=B.GetSize(),Desired=Target.GetScale3D().GetAbs()*100;
 const FVector Scale(Desired.X/FMath::Max(1.,Size.X),Desired.Y/FMath::Max(1.,Size.Y),Size.Z<1?1:Desired.Z/Size.Z);
 return FTransform(Target.GetRotation(),Target.GetLocation()-Target.GetRotation().RotateVector(B.GetCenter()*Scale),Scale);
}
bool FKTWImportedArt::Configure(UInstancedStaticMeshComponent* C,int32 Kind){if(!Enabled())return false;auto* M=Data()->Meshes.Find(Kind);if(!M||!IsValid(M->Get()))return false;C->SetStaticMesh(M->Get());C->EmptyOverrideMaterials();C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->SetCullDistances(Kind==37?0:18000,Kind==37?0:24000);return true;}
void FKTWImportedArt::Render(AKTWStreetDirector* D,const TArray<FKTWPlacement>& Layout,EKTWLayer Layer){if(!Enabled()||!D->GetWorld())return;
 const FName Tag=Layer==EKTWLayer::Facades?TEXT("KTW_ImportedFacades"):TEXT("KTW_ImportedBarriers");TArray<AActor*> Old;for(TActorIterator<AActor>I(D->GetWorld());I;++I)if(I->GetOwner()==D&&I->ActorHasTag(Tag))Old.Add(*I);for(auto* A:Old)A->Destroy();
 auto* Holder=D->GetWorld()->SpawnActor<AActor>();if(!Holder)return;Holder->SetOwner(D);Holder->SetFlags(RF_Transient);Holder->Tags.Add(Tag);auto* Root=NewObject<USceneComponent>(Holder);Holder->AddInstanceComponent(Root);Holder->SetRootComponent(Root);Root->RegisterComponent();
 TMap<UStaticMesh*,UHierarchicalInstancedStaticMeshComponent*> Groups;int Count=0;
 UInstancedStaticMeshComponent* Collision=nullptr;
 for(const auto& V:Layout){if((Layer==EKTWLayer::Facades&&V.Kind>=44&&V.Kind<=57)||!Replaces(V,Layer))continue;auto* Mesh=Data()->Meshes.FindChecked(ResolveKind(V,Layer)).Get();auto** Found=Groups.Find(Mesh);UHierarchicalInstancedStaticMeshComponent* C=Found?*Found:nullptr;
  if(!C){C=NewObject<UHierarchicalInstancedStaticMeshComponent>(Holder);Holder->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->bAutoRebuildTreeOnInstanceChanges=false;
   const bool Building=V.Kind>=100&&V.Kind<120;C->SetCullDistances(V.Kind==37?0:Building?36000:16000,V.Kind==37?0:Building?46000:23000);C->SetCastShadow(Building||V.Transform.GetScale3D().Z>1);C->RegisterComponent();Groups.Add(Mesh,C);}
  if(V.Kind==37&&FMath::Max(V.Transform.GetScale3D().X,V.Transform.GetScale3D().Y)>6){const FVector Size=V.Transform.GetScale3D()*100;const bool X=Size.X>=Size.Y;const float Length=X?Size.X:Size.Y;const int N=FMath::CeilToInt(Length/350);for(int K=0;K<N;++K){FTransform T=V.Transform;FVector Scale=T.GetScale3D();(X?Scale.X:Scale.Y)=Length/N/100;T.SetScale3D(Scale);T.AddToTranslation(T.GetRotation().RotateVector((X?FVector::ForwardVector:FVector::RightVector)*(-Length/2+(K+.5f)*Length/N)));C->AddInstance(Fit(T,Mesh));++Count;}}else{C->AddInstance(Fit(V.Transform,Mesh));++Count;}
  if(V.Kind>=100&&V.Kind<120){if(!Collision){Collision=NewObject<UInstancedStaticMeshComponent>(Holder,TEXT("SimpleBuildingCollision"));Holder->AddInstanceComponent(Collision);Collision->SetupAttachment(Root);Collision->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Collision->SetHiddenInGame(true);Collision->SetVisibility(false);Collision->SetCollisionProfileName(TEXT("BlockAll"));Collision->SetCanEverAffectNavigation(false);Collision->RegisterComponent();}Collision->AddInstance(V.Transform);}
 }
 for(auto& Pair:Groups)Pair.Value->BuildTreeIfOutdated(true,true);
 UE_LOG(LogTemp,Display,TEXT("KTW IMPORTED ART layer=%d instances=%d meshGroups=%d"),int32(Layer),Count,Groups.Num());
}
#if WITH_EDITOR
int32 FKTWImportedArt::BuildContent(){
 auto Save=[](UObject* O){O->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(O->GetOutermost(),O,*FPackageName::LongPackageNameToFilename(O->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),A);};
 const FString Path=TEXT("/Game/KTW/Data/DA_ImportedHongKong");auto* Set=LoadObject<UKTWImportedArtSet>(nullptr,*(Path+TEXT(".DA_ImportedHongKong")));if(!Set){Set=NewObject<UKTWImportedArtSet>(CreatePackage(*Path),TEXT("DA_ImportedHongKong"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Set);}Set->Meshes.Empty();TSet<UStaticMesh*> Optimized;TSet<UTexture*> Textures;TSet<UMaterial*> Parents;
 struct FRow{int Kind;const TCHAR* Path;};const FRow Rows[]={
#include "KTWImportedArtSelection.inl"
 };
 FString Report=TEXT("kind,path,triangles,lods\n");
 for(const auto& Row:Rows){auto* Mesh=LoadObject<UStaticMesh>(nullptr,Row.Path);if(!Mesh)return 81;Set->Meshes.Add(Row.Kind,Mesh);if(Optimized.Contains(Mesh))continue;Optimized.Add(Mesh);FStaticMeshCompilingManager::Get().FinishAllCompilation();
  // Keep authored Nanite settings; add traditional distance LODs for fallback rendering.
  if(Mesh->GetNumSourceModels()==1&&Mesh->GetNumTriangles(0)>200){Mesh->SetNumSourceModels(3);Mesh->SetAutoComputeLODScreenSize(true);for(int L=1;L<3;++L){auto& Source=Mesh->GetSourceModel(L);Source.ReductionSettings.PercentTriangles=L==1?.5f:.2f;Source.ReductionSettings.BaseLODModel=0;}Mesh->Build(false);FStaticMeshCompilingManager::Get().FinishAllCompilation();if(!Save(Mesh))return 82;}
  for(const auto& Slot:Mesh->GetStaticMaterials())if(Slot.MaterialInterface){Parents.Add(Slot.MaterialInterface->GetMaterial());TArray<UTexture*> Used;Slot.MaterialInterface->GetUsedTextures(Used,EMaterialQualityLevel::High,false,ERHIFeatureLevel::SM6,true);for(auto* T:Used)if(T&&T->GetPathName().StartsWith(TEXT("/Game/Abandoned_Hong-Kong/")))Textures.Add(T);}
  Report+=FString::Printf(TEXT("%d,%s,%d,%d\n"),Row.Kind,Row.Path,Mesh->GetNumTriangles(0),Mesh->GetNumLODs());
 }
 TArray<FString> TextureFiles;IFileManager::Get().FindFilesRecursive(TextureFiles,*(FPaths::ProjectContentDir()/TEXT("Abandoned_Hong-Kong/Textures")),TEXT("*.uasset"),true,false);for(const auto& File:TextureFiles){FString Package;if(FPackageName::TryConvertFilenameToLongPackageName(File,Package))if(auto* T=LoadObject<UTexture>(nullptr,*(Package+TEXT(".")+FPaths::GetBaseFilename(File))))Textures.Add(T);}
 for(auto* T:Textures){T->MaxTextureSize=2048;T->NeverStream=false;T->PostEditChange();if(!Save(T))return 83;}
 for(auto* Parent:Parents)if(Parent){Parent->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);Parent->PostEditChange();if(!Save(Parent))return 85;}if(!Save(Set))return 84;FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectDir()/TEXT("Docs/ImportedArtSelected.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 UE_LOG(LogTemp,Display,TEXT("KTW IMPORTED CONTENT COMPLETE mappings=%d meshes=%d textures=%d"),Set->Meshes.Num(),Optimized.Num(),Textures.Num());return 0;
}
#endif
