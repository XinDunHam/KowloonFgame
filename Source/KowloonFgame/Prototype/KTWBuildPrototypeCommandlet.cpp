#include "KTWBuildPrototypeCommandlet.h"
#include "KTWGeneration.h"
#include "KTWImportedArt.h"
#include "KTWPrototype.h"
#include "KTWAI.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Factories/SoundFactory.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshCompiler.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Sound/SoundWave.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "MeshSelectors/PCGMeshSelectorByAttribute.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Engine/DirectionalLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerStart.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavModifierVolume.h"
#include "KTWTactics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "Components/BrushComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "HAL/PlatformProcess.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#endif
UKTWBuildPrototypeCommandlet::UKTWBuildPrototypeCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UKTWBuildPrototypeCommandlet::Main(const FString& Params) {
#if WITH_EDITOR
 auto SaveAsset=[](UObject* O) {
  O->MarkPackageDirty(); FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;
  return UPackage::SavePackage(O->GetOutermost(),O,*FPackageName::LongPackageNameToFilename(O->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),A);
 };
 if(FParse::Param(*Params,TEXT("OnlyRogueMap"))){
  UWorld* Arena=UEditorLoadingAndSavingUtils::NewBlankMap(false);if(!Arena)return 84;Arena->GetWorldSettings()->DefaultGameMode=AKTWPrototypeGameMode::StaticClass();Arena->GetWorldSettings()->bForceNoPrecomputedLighting=true;
  auto* Start=Arena->SpawnActor<APlayerStart>();Start->SetActorLocation(FVector(0,-2400,2800));auto* Sun=Arena->SpawnActor<ADirectionalLight>();Sun->SetActorRotation(FRotator(-45,-35,0));Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(3);Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight=true;Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->ForwardShadingPriority=1;
  auto* Fill=Arena->SpawnActor<ADirectionalLight>();Fill->SetActorRotation(FRotator(-35,140,0));Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);Fill->GetLightComponent()->SetIntensity(.8f);Fill->GetLightComponent()->SetCastShadows(false);auto* Sky=Arena->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetIntensity(1);Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);Arena->SpawnActor<ASkyAtmosphere>();
  if(!UEditorLoadingAndSavingUtils::SaveMap(Arena,TEXT("/Game/KTW/Maps/L_RuleArena")))return 85;UE_LOG(LogTemp,Display,TEXT("KTW ROGUE MAP COMPLETE"));return 0;
 }
 if(FParse::Param(*Params,TEXT("OnlySecurityAnimations"))){
  auto* Skeleton=LoadObject<USkeleton>(nullptr,TEXT("/Game/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton.UE4_Mannequin_Skeleton"));if(!Skeleton)return 81;
  const TCHAR* Name=TEXT("A_SecurityShoot");const FString Path=TEXT("/Game/KTW/Animations/A_SecurityShoot");auto* Anim=LoadObject<UAnimSequence>(nullptr,*(Path+TEXT(".")+Name));const bool NewAsset=!Anim;auto* P=Anim?Anim->GetOutermost():CreatePackage(*Path);P->FullyLoad();if(!Anim)Anim=NewObject<UAnimSequence>(P,Name,RF_Public|RF_Standalone);Anim->SetSkeleton(Skeleton);
  auto& C=Anim->GetController();C.InitializeModel();C.OpenBracket(FText::FromString(TEXT("Security pistol aim recoil recover")),false);C.RemoveAllBoneTracks(false);C.SetFrameRate(FFrameRate(30,1),false);C.SetNumberOfFrames(FFrameNumber(24),false);
  const auto& Ref=Skeleton->GetReferenceSkeleton();const auto& Pose=Ref.GetRefBonePose();TArray<FTransform> RefCS;RefCS.SetNum(Ref.GetNum());for(int32 B=0;B<Ref.GetNum();++B){const int32 Parent=Ref.GetParentIndex(B);RefCS[B]=Parent>=0?Pose[B]*RefCS[Parent]:Pose[B];}
  const FVector Forward=(RefCS[Ref.FindBoneIndex(TEXT("ball_l"))].GetLocation()-RefCS[Ref.FindBoneIndex(TEXT("foot_l"))].GetLocation()).GetSafeNormal2D();const FVector Side=FVector::CrossProduct(Forward,FVector::UpVector).GetSafeNormal();TArray<TArray<FQuat>> CS;CS.SetNum(25);for(auto& Frame:CS)Frame.SetNum(Ref.GetNum());
  for(int32 B=0;B<Ref.GetNum();++B){const FName N=Ref.GetBoneName(B);TArray<FVector3f> Positions,Scales;TArray<FQuat4f> Rotations;
   for(int32 K=0;K<=24;++K){const float Aim=K<4?K/4.f:K>18?(24-K)/6.f:1.f,Recoil=FMath::Max(0.f,1.f-FMath::Abs(K-8)/3.f);const int32 Parent=Ref.GetParentIndex(B);const FQuat ParentCS=Parent>=0?CS[K][Parent]:FQuat::Identity;FQuat Desired=ParentCS*Pose[B].GetRotation();FName Child=NAME_None;FVector Direction;
    if(N==TEXT("upperarm_r")){Child=TEXT("lowerarm_r");Direction=Forward*(Aim-.08f*Recoil)-FVector::UpVector*(1.05f-.85f*Aim-.15f*Recoil)+Side*.12f;}
    if(N==TEXT("lowerarm_r")){Child=TEXT("hand_r");Direction=Forward*Aim+FVector::UpVector*(-1.f+1.08f*Aim+.22f*Recoil);}
    if(N==TEXT("hand_r")){Child=TEXT("middle_01_r");Direction=Forward*Aim+FVector::UpVector*(-1.f+Aim+.18f*Recoil);}
    if(N==TEXT("upperarm_l")){Child=TEXT("lowerarm_l");Direction=-FVector::UpVector-Side*.12f;}
    if(N==TEXT("lowerarm_l")){Child=TEXT("hand_l");Direction=-FVector::UpVector+Forward*.15f;}
    if(Child!=NAME_None){const int32 Next=Ref.FindBoneIndex(Child);if(Next>=0){const FVector Original=(RefCS[Next].GetLocation()-RefCS[B].GetLocation()).GetSafeNormal();Desired=FQuat::FindBetweenNormals(Original,Direction.GetSafeNormal())*RefCS[B].GetRotation();}}
    if(N==TEXT("spine_03"))Desired=FQuat(Side,FMath::DegreesToRadians(-4.f*Recoil))*Desired;
    CS[K][B]=Desired;Positions.Add(FVector3f(Pose[B].GetTranslation()));Scales.Add(FVector3f(Pose[B].GetScale3D()));Rotations.Add(FQuat4f(ParentCS.Inverse()*Desired));
   }C.AddBoneCurve(N,false);if(!C.SetBoneTrackKeys(N,Positions,Rotations,Scales,false))return 82;
  }C.NotifyPopulated();C.CloseBracket(false);Anim->MarkPackageDirty();if(NewAsset)FAssetRegistryModule::AssetCreated(Anim);if(!SaveAsset(Anim))return 83;UE_LOG(LogTemp,Display,TEXT("KTW SECURITY ANIMATION COMPLETE frames=25 duration=0.8"));return 0;
 }
 if(FParse::Param(*Params,TEXT("OnlyImportedContent")))return FKTWImportedArt::BuildContent();
 if(FParse::Param(*Params,TEXT("OnlyAbandonedAudit"))){
  TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*(FPaths::ProjectContentDir()/TEXT("Abandoned_Hong-Kong/StaticMeshes")),TEXT("*.uasset"),true,false);Files.Sort();
  FString Report=TEXT("path\tx\ty\tz\tcx\tcy\tcz\ttriangles\tlods\tmaterials\n");int Loaded=0;
  for(int Index=0;Index<Files.Num();++Index){FString Package;if(!FPackageName::TryConvertFilenameToLongPackageName(Files[Index],Package))return 74;const FString Name=FPaths::GetBaseFilename(Files[Index]);auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name));if(!Mesh){UE_LOG(LogTemp,Error,TEXT("KTW ASSET LOAD FAILED %s"),*Package);return 75;}
   FStaticMeshCompilingManager::Get().FinishAllCompilation();const auto B=Mesh->GetBoundingBox();const auto Size=B.GetSize(),Center=B.GetCenter();
   Report+=FString::Printf(TEXT("%s.%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%d\t%d\t%d\n"),*Package,*Name,Size.X,Size.Y,Size.Z,Center.X,Center.Y,Center.Z,Mesh->GetNumTriangles(0),Mesh->GetNumLODs(),Mesh->GetStaticMaterials().Num());++Loaded;
   if(Index%40==39){UE_LOG(LogTemp,Display,TEXT("KTW ASSET AUDIT %d / %d"),Loaded,Files.Num());CollectGarbage(RF_NoFlags);}
  }
  if(!FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectDir()/TEXT("Docs/AbandonedMeshAudit.tsv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return 76;
  UE_LOG(LogTemp,Display,TEXT("KTW ABANDONED AUDIT COMPLETE loaded=%d"),Loaded);return Loaded==834?0:77;
 }
 if(FParse::Param(*Params,TEXT("OnlySignals"))){
  const TCHAR* Names[]={TEXT("M_SignalBody"),TEXT("M_SignalOff"),TEXT("M_SignalRed"),TEXT("M_SignalAmber"),TEXT("M_SignalGreen")};
  const FLinearColor Colors[]={FLinearColor(.006f,.008f,.01f),FLinearColor(.025f,.03f,.033f),FLinearColor(.28f,.001f,.001f),FLinearColor(.32f,.09f,.001f),FLinearColor(.001f,.20f,.008f)};
  for(int I=0;I<5;++I){const FString Path=FString(TEXT("/Game/KTW/Materials/"))+Names[I];auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Names[I]));if(!M){auto* P=CreatePackage(*Path);M=NewObject<UMaterial>(P,Names[I],RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(M);}
   auto* Color=NewObject<UMaterialExpressionConstant3Vector>(M);Color->Constant=Colors[I]*.01f;M->GetExpressionCollection().AddExpression(Color);M->SetShadingModel(MSM_Unlit);M->GetEditorOnlyData()->EmissiveColor.Expression=Color;M->GetEditorOnlyData()->BaseColor.Expression=Color;M->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);M->PostEditChange();if(!SaveAsset(M))return 73;
  }UE_LOG(LogTemp,Display,TEXT("KTW SIGNAL MATERIALS PASS five calibrated traffic signal materials"));return 0;
 }
 if(FParse::Param(*Params,TEXT("OnlyAudio"))){const TCHAR* Names[]={TEXT("Click"),TEXT("Buy"),TEXT("Pickup"),TEXT("Deposit"),TEXT("Level"),TEXT("Win"),TEXT("Lose"),TEXT("Strike"),TEXT("Down"),TEXT("Ambience"),TEXT("Rain"),TEXT("Thunder"),TEXT("Wind")};for(const TCHAR* Name:Names){const FString AssetName=FString(TEXT("S_"))+Name,Path=FString(TEXT("/Game/KTW/Audio/"))+AssetName;if(LoadObject<USoundWave>(nullptr,*(Path+TEXT(".")+AssetName)))continue;auto* P=CreatePackage(*Path);P->FullyLoad();TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectDir()/TEXT("SourceAssets/Audio")/(FString(Name)+TEXT(".wav")))))return 40;auto* Factory=NewObject<USoundFactory>();Factory->SuppressImportDialogs();const uint8* Data=Bytes.GetData();auto* Wave=Cast<USoundWave>(Factory->FactoryCreateBinary(USoundWave::StaticClass(),P,*AssetName,RF_Public|RF_Standalone,nullptr,TEXT("wav"),Data,Data+Bytes.Num(),GWarn));if(!Wave)return 41;Wave->bLooping=FString(Name)==TEXT("Ambience")||FString(Name)==TEXT("Rain")||FString(Name)==TEXT("Wind");FAssetRegistryModule::AssetCreated(Wave);if(!SaveAsset(Wave))return 42;}UE_LOG(LogTemp,Display,TEXT("KTW AUDIO BUILD PASS thirteen original synthesized sound assets"));return 0;}
 if(FParse::Param(*Params,TEXT("OnlyGameplay49"))){
  auto Make=[&](const TCHAR* Name,FLinearColor Color,bool Wet=false){FString Path=FString(TEXT("/Game/KTW/Materials/"))+Name;auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));if(!M){auto* P=CreatePackage(*Path);M=NewObject<UMaterial>(P,Name,RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(M);}auto* C=NewObject<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;if(!Wet){M->SetShadingModel(MSM_Unlit);M->GetEditorOnlyData()->EmissiveColor.Expression=C;}else{M->SetShadingModel(MSM_DefaultLit);M->BlendMode=BLEND_Translucent;M->TranslucencyLightingMode=TLM_SurfacePerPixelLighting;auto* O=NewObject<UMaterialExpressionConstant>(M);O->R=.35f;M->GetExpressionCollection().AddExpression(O);M->GetEditorOnlyData()->Opacity.Expression=O;}auto* R=NewObject<UMaterialExpressionConstant>(M);R->R=Wet?.08f:.8f;M->GetExpressionCollection().AddExpression(R);M->GetEditorOnlyData()->Roughness.Expression=R;M->TwoSided=true;M->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);M->PostEditChange();return SaveAsset(M);};
  if(!Make(TEXT("M_MedGreen"),FLinearColor(.01f,.7f,.2f))||!Make(TEXT("M_SnowWhite"),FLinearColor(.7f,.8f,.9f))||!Make(TEXT("M_RainBlue"),FLinearColor(.08f,.3f,.5f))||!Make(TEXT("M_WetStreet"),FLinearColor(.02f,.025f,.035f),true))return 61;
  for(int32 Team=0;Team<2;++Team){const FString Path=Team?TEXT("/Game/KTW/Materials/M_Red.M_Red"):TEXT("/Game/KTW/Materials/M_Blue.M_Blue");if(auto* M=LoadObject<UMaterial>(nullptr,*Path)){auto* E=NewObject<UMaterialExpressionConstant3Vector>(M);E->Constant=Team?FLinearColor(.13f,.012f,.005f):FLinearColor(.005f,.045f,.13f);M->GetExpressionCollection().AddExpression(E);M->GetEditorOnlyData()->EmissiveColor.Expression=E;M->SetMaterialUsage(MATUSAGE_SkeletalMesh);M->PostEditChange();if(!SaveAsset(M))return 69;}}
  FString Path=TEXT("/Game/KTW/PCG/PCG_Weather_V49");if(!LoadObject<UPCGGraph>(nullptr,*(Path+TEXT(".PCG_Weather_V49")))){auto* P=CreatePackage(*Path);auto* G=NewObject<UPCGGraph>(P,TEXT("PCG_Weather_V49"),RF_Public|RF_Standalone);UKTWPCGSettings* Settings=nullptr;auto* N=G->AddNodeOfType(Settings);Settings->Layer=EKTWLayer::Atmosphere;G->AddLabeledEdge(N,PCGPinConstants::DefaultOutputLabel,G->GetOutputNode(),PCGPinConstants::DefaultOutputLabel);FAssetRegistryModule::AssetCreated(G);if(!SaveAsset(G))return 62;}
  auto* BP=LoadObject<UAnimBlueprint>(nullptr,TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP"));if(!BP)return 63;TArray<UEdGraph*> Graphs;BP->GetAllGraphs(Graphs);bool Ready=false;
  for(auto* Graph:Graphs){UAnimGraphNode_Root* Root=nullptr;for(UEdGraphNode* Node:Graph->Nodes){if(auto* Slot=Cast<UAnimGraphNode_Slot>(Node))if(Slot->Node.SlotName==TEXT("DefaultSlot"))Ready=true;if(auto* R=Cast<UAnimGraphNode_Root>(Node))Root=R;}if(Ready)break;if(!Root)continue;UEdGraphPin* Result=nullptr;for(auto* P:Root->Pins)if(P->Direction==EGPD_Input&&P->LinkedTo.Num()){Result=P;break;}if(!Result)continue;auto* Upstream=Result->LinkedTo[0];auto* Slot=NewObject<UAnimGraphNode_Slot>(Graph);Graph->AddNode(Slot,true,false);Slot->CreateNewGuid();Slot->PostPlacedNewNode();Slot->Node.SlotName=TEXT("DefaultSlot");Slot->Node.bAlwaysUpdateSourcePose=true;Slot->AllocateDefaultPins();Slot->NodePosX=Root->NodePosX-180;Slot->NodePosY=Root->NodePosY;UEdGraphPin* Input=nullptr;UEdGraphPin* Output=nullptr;for(auto* P:Slot->Pins){if(P->Direction==EGPD_Input&&!Input)Input=P;if(P->Direction==EGPD_Output&&!Output)Output=P;}if(!Input||!Output)return 64;Result->BreakAllPinLinks();Ready=Graph->GetSchema()->TryCreateConnection(Upstream,Input)&&Graph->GetSchema()->TryCreateConnection(Output,Result);break;}
  if(!Ready)return 65;
  // Cache locomotion once and layer strikes above the spine; feet continue their real walk cycle.
  for(auto* Graph:Graphs){UAnimGraphNode_Root* Root=nullptr;UAnimGraphNode_Slot* Slot=nullptr;bool Layered=false;for(UEdGraphNode* N:Graph->Nodes){if(auto* R=Cast<UAnimGraphNode_Root>(N))Root=R;if(auto* S=Cast<UAnimGraphNode_Slot>(N))if(S->Node.SlotName==TEXT("DefaultSlot"))Slot=S;if(Cast<UAnimGraphNode_LayeredBoneBlend>(N))Layered=true;}if(!Root||!Slot||Layered)continue;
   auto Pin=[](UEdGraphNode* N,EEdGraphPinDirection D,const FString& Name=FString())->UEdGraphPin*{for(auto* P:N->Pins)if(P->Direction==D&&(Name.IsEmpty()||P->PinName.ToString()==Name))return P;return nullptr;};
   auto* SI=Pin(Slot,EGPD_Input,TEXT("Source"));auto* SO=Pin(Slot,EGPD_Output);auto* RI=Pin(Root,EGPD_Input);if(!SI||SI->LinkedTo.IsEmpty()||!SO||!RI)return 70;auto* Upstream=SI->LinkedTo[0];
   auto Prepare=[&](UEdGraphNode* N,int X,int Y){Graph->AddNode(N,true,false);N->CreateNewGuid();N->PostPlacedNewNode();N->NodePosX=X;N->NodePosY=Y;};
   auto* Cache=NewObject<UAnimGraphNode_SaveCachedPose>(Graph);Prepare(Cache,Root->NodePosX-900,Root->NodePosY);Cache->CacheName=TEXT("KTWLocomotion");Cache->Node.CachePoseName=TEXT("KTWLocomotion");Cache->AllocateDefaultPins();
   auto Use=[&](int Y){auto* N=NewObject<UAnimGraphNode_UseCachedPose>(Graph);Prepare(N,Root->NodePosX-650,Y);N->SaveCachedPoseNode=Cache;N->AllocateDefaultPins();N->GetNodeTitle(ENodeTitleType::FullTitle);return N;};auto* Base=Use(Root->NodePosY);auto* Upper=Use(Root->NodePosY+200);
   auto* Blend=NewObject<UAnimGraphNode_LayeredBoneBlend>(Graph);Prepare(Blend,Root->NodePosX-240,Root->NodePosY);Blend->Node.BlendPoses.SetNum(1);Blend->Node.BlendWeights.SetNum(1);Blend->Node.BlendWeights[0]=1;Blend->Node.LayerSetup.SetNum(1);FBranchFilter Filter;Filter.BoneName=TEXT("spine_01");Filter.BlendDepth=1;Blend->Node.LayerSetup[0].BranchFilters.Add(Filter);Blend->Node.bMeshSpaceRotationBlend=true;Blend->AllocateDefaultPins();
   SI->BreakAllPinLinks();RI->BreakAllPinLinks();const auto* Schema=Graph->GetSchema();auto Connect=[&](UEdGraphPin* A,UEdGraphPin* B){return A&&B&&Schema->TryCreateConnection(A,B);};if(!Connect(Upstream,Pin(Cache,EGPD_Input))||!Connect(Pin(Upper,EGPD_Output),SI)||!Connect(Pin(Base,EGPD_Output),Pin(Blend,EGPD_Input,TEXT("BasePose")))||!Connect(SO,Pin(Blend,EGPD_Input,TEXT("BlendPoses_0")))||!Connect(Pin(Blend,EGPD_Output),RI))return 71;
  }
  FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error||!SaveAsset(BP))return 66;auto* V=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));if(!V)return 67;V->bActionMontages=true;if(!SaveAsset(V))return 68;UE_LOG(LogTemp,Display,TEXT("KTW GAMEPLAY49 ASSETS PASS weather graph, loot materials and blended action slot"));return 0;
 }
 if(FParse::Param(*Params,TEXT("OnlyUrban"))||FParse::Param(*Params,TEXT("OnlyStreetArt"))||FParse::Param(*Params,TEXT("OnlyShopSigns"))){
  auto Texture=[&](const FString& Name)->UTexture2D*{const FString Path=TEXT("/Game/KTW/StreetArt/T_")+Name;auto* P=CreatePackage(*Path);P->FullyLoad();TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectDir()/TEXT("SourceAssets/StreetArt")/(Name+TEXT(".png")))))return nullptr;auto* F=NewObject<UTextureFactory>();UTextureFactory::SuppressImportOverwriteDialog();const uint8* Data=Bytes.GetData();auto* T=Cast<UTexture2D>(F->FactoryCreateBinary(UTexture2D::StaticClass(),P,*(TEXT("T_")+Name),RF_Public|RF_Standalone,nullptr,TEXT("png"),Data,Data+Bytes.Num(),GWarn));if(T){T->SRGB=true;T->PostEditChange();FAssetRegistryModule::AssetCreated(T);SaveAsset(T);}return T;};
  auto TexturedMaterial=[&](const FString& Name,UTexture2D* Image,bool Neon=false,bool WorldRoad=false){if(!Image)return false;const FString Path=TEXT("/Game/KTW/Materials/")+Name;auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));if(!M){auto* P=CreatePackage(*Path);M=NewObject<UMaterial>(P,*Name,RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(M);}auto* T=NewObject<UMaterialExpressionTextureSample>(M);T->Texture=Image;T->SamplerType=SAMPLERTYPE_Color;M->GetExpressionCollection().AddExpression(T);M->GetEditorOnlyData()->BaseColor.Expression=T;if(Neon){M->SetShadingModel(MSM_Unlit);M->GetEditorOnlyData()->EmissiveColor.Expression=T;}
   if(WorldRoad){auto* W=NewObject<UMaterialExpressionWorldPosition>(M);auto* XY=NewObject<UMaterialExpressionComponentMask>(M);XY->Input.Expression=W;XY->R=XY->G=true;auto* UV=NewObject<UMaterialExpressionMultiply>(M);UV->A.Expression=XY;UV->ConstB=.001f;T->Coordinates.Expression=UV;M->GetExpressionCollection().AddExpression(W);M->GetExpressionCollection().AddExpression(XY);M->GetExpressionCollection().AddExpression(UV);}auto* Rough=NewObject<UMaterialExpressionConstant>(M);Rough->R=.92f;M->GetExpressionCollection().AddExpression(Rough);M->GetEditorOnlyData()->Roughness.Expression=Rough;M->TwoSided=true;M->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);M->PostEditChange();return SaveAsset(M);};
  if(FParse::Param(*Params,TEXT("OnlyUrban"))){for(int I=0;I<6;++I){const FString N=FString::Printf(TEXT("Urban%d"),I);if(!TexturedMaterial(TEXT("M_")+N,Texture(N)))return 71;}for(const TCHAR* N:{TEXT("UrbanPaving"),TEXT("UrbanPaint")})if(!TexturedMaterial(FString(TEXT("M_"))+N,Texture(N),false,true))return 72;UE_LOG(LogTemp,Display,TEXT("KTW URBAN ASSETS PASS six facades and two pavement materials"));return 0;}
  if(!FParse::Param(*Params,TEXT("OnlyShopSigns"))){
  UTexture2D* Road=Texture(TEXT("Road"));for(const TCHAR* N:{TEXT("M_Street"),TEXT("M_UpperRoad"),TEXT("M_MainRoad"),TEXT("M_LowerRoad"),TEXT("M_LinkAlley")})if(!TexturedMaterial(N,Road,true,true))return 51;
  for(const TCHAR* N:{TEXT("Facade"),TEXT("FacadeWarm"),TEXT("FacadeGreen"),TEXT("Roof"),TEXT("Awning"),TEXT("Shutter")})if(!TexturedMaterial(FString(TEXT("M_"))+N,Texture(N)))return 52;
  }
  for(int32 I=FParse::Param(*Params,TEXT("OnlyShopSigns"))?6:0;I<8;++I){const FString Name=FString::Printf(TEXT("Sign%d"),I);if(!TexturedMaterial(TEXT("M_")+Name,Texture(Name),true))return 53;}
  if(FParse::Param(*Params,TEXT("OnlyShopSigns"))){UE_LOG(LogTemp,Display,TEXT("KTW SHOP SIGNS PASS two original commercial signs"));return 0;}
  const TCHAR* GoldName=TEXT("PCG_GoldSources_V48");const FString Path=FString(TEXT("/Game/KTW/PCG/"))+GoldName;
  auto* Graph=LoadObject<UPCGGraph>(nullptr,*(Path+TEXT(".")+GoldName));if(!Graph){auto* P=CreatePackage(*Path);Graph=NewObject<UPCGGraph>(P,GoldName,RF_Public|RF_Standalone);UKTWPCGSettings* Settings=nullptr;auto* Node=Graph->AddNodeOfType(Settings);Settings->Layer=EKTWLayer::GoldSources;Graph->AddLabeledEdge(Node,PCGPinConstants::DefaultOutputLabel,Graph->GetOutputNode(),PCGPinConstants::DefaultOutputLabel);FAssetRegistryModule::AssetCreated(Graph);if(!SaveAsset(Graph))return 54;}
  UE_LOG(LogTemp,Display,TEXT("KTW STREET ART BUILD PASS: Chinese signs, tiled streets, facades and fourth gold PCG graph"));return 0;
 }
 auto Material=[&](const TCHAR* Name,FLinearColor Color) {
  FString Path=FString(TEXT("/Game/KTW/Materials/"))+Name;
  if(auto* Existing=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name))){Existing->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);if(FString(Name)==TEXT("M_Blue")||FString(Name)==TEXT("M_Red"))Existing->SetMaterialUsage(MATUSAGE_SkeletalMesh);SaveAsset(Existing);return Existing;}
  UPackage* P=CreatePackage(*Path);P->FullyLoad();
  auto* M=NewObject<UMaterial>(P,Name,RF_Public|RF_Standalone);
  auto* C=NewObject<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;
  M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;
  if(FString(Name).Contains(TEXT("Neon")))M->GetEditorOnlyData()->EmissiveColor.Expression=C;
  M->TwoSided=true;M->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);if(FString(Name)==TEXT("M_Blue")||FString(Name)==TEXT("M_Red"))M->SetMaterialUsage(MATUSAGE_SkeletalMesh);M->PostEditChange();FAssetRegistryModule::AssetCreated(M);SaveAsset(M);return M;
 };
 auto* Road=Material(TEXT("M_Street"),FLinearColor(.07f,.085f,.10f));
 auto* Mark=Material(TEXT("M_Markers"),FLinearColor(.95f,.55f,.10f));
 auto* Facade=Material(TEXT("M_Facade"),FLinearColor(.42f,.35f,.26f));
 auto* Barrier=Material(TEXT("M_Barrier"),FLinearColor(.10f,.29f,.25f));
 auto* Weapon=Material(TEXT("M_Weapon"),FLinearColor(.75f,.30f,.07f));
 Material(TEXT("M_Blue"),FLinearColor(.03f,.25f,.85f));
 Material(TEXT("M_Red"),FLinearColor(.85f,.08f,.035f));

 Material(TEXT("M_Window"),FLinearColor(.10f,.17f,.20f));Material(TEXT("M_Aircon"),FLinearColor(.40f,.43f,.40f));
 Material(TEXT("M_Roof"),FLinearColor(.09f,.12f,.12f));Material(TEXT("M_Bamboo"),FLinearColor(.50f,.34f,.15f));
 Material(TEXT("M_NeonRed"),FLinearColor(2.f,.06f,.04f));Material(TEXT("M_NeonBlue"),FLinearColor(.02f,.65f,2.f));Material(TEXT("M_NeonGold"),FLinearColor(2.f,.9f,.08f));
 auto* Upper=Material(TEXT("M_UpperRoad"),FLinearColor(.13f,.22f,.26f));
 auto* Main=Material(TEXT("M_MainRoad"),FLinearColor(.23f,.26f,.27f));
 auto* Lower=Material(TEXT("M_LowerRoad"),FLinearColor(.23f,.24f,.18f));
 auto* Link=Material(TEXT("M_LinkAlley"),FLinearColor(.42f,.29f,.13f));
 // Persist real skeletal assets and an editable replacement contract.
 const TCHAR* VisualPath=TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4");
 auto* Visual=LoadObject<UKTWCharacterVisuals>(nullptr,TEXT("/Game/KTW/Characters/DA_CharacterVisuals_V4.DA_CharacterVisuals_V4"));
 if(!Visual){auto* P=CreatePackage(VisualPath);P->FullyLoad();Visual=NewObject<UKTWCharacterVisuals>(P,TEXT("DA_CharacterVisuals_V4"),RF_Public|RF_Standalone);
  Visual->BossMesh=TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")));
  Visual->SoldierMesh=Visual->BossMesh;Visual->AnimationClass=TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C")));
  FAssetRegistryModule::AssetCreated(Visual);if(!SaveAsset(Visual))return 10;
 }
 if(!Visual->BossMesh.LoadSynchronous()||!Visual->AnimationClass.LoadSynchronous())return 11;
 // Deliberate migration for the requested readability / pace revision; no map scale or combat distance changes.
 Visual->OverviewScaleMultiplier=7.5f;Visual->MovementSpeedMultiplier=1.75f;if(!SaveAsset(Visual))return 15;
 auto* Skeleton=Visual->BossMesh.LoadSynchronous()->GetSkeleton();
 auto MakeAction=[&](const TCHAR* Name,bool Throw,bool Pickup=false){
  FString Path=FString(TEXT("/Game/KTW/Animations/"))+Name;
  auto* Anim=LoadObject<UAnimSequence>(nullptr,*(Path+TEXT(".")+Name));const bool NewAsset=Anim==nullptr;if(Anim&&!(Pickup&&FParse::Param(*Params,TEXT("UpdatePickup"))))return true;
  auto* P=Anim?Anim->GetOutermost():CreatePackage(*Path);P->FullyLoad();if(!Anim)Anim=NewObject<UAnimSequence>(P,Name,RF_Public|RF_Standalone);Anim->SetSkeleton(Skeleton);
  auto& C=Anim->GetController();C.InitializeModel();C.OpenBracket(FText::FromString(TEXT("Author basic combat action")),false);C.RemoveAllBoneTracks(false);C.SetFrameRate(FFrameRate(30,1),false);C.SetNumberOfFrames(FFrameNumber(18),false);
  const auto& Ref=Skeleton->GetReferenceSkeleton();const auto& Pose=Ref.GetRefBonePose();
  TArray<FTransform> RefCS;RefCS.SetNum(Ref.GetNum());for(int32 B=0;B<Ref.GetNum();++B){const int32 Parent=Ref.GetParentIndex(B);RefCS[B]=Parent>=0?Pose[B]*RefCS[Parent]:Pose[B];}
  const int32 Foot=Ref.FindBoneIndex(TEXT("foot_l")),Ball=Ref.FindBoneIndex(TEXT("ball_l"));const FVector Forward=Foot>=0&&Ball>=0?(RefCS[Ball].GetLocation()-RefCS[Foot].GetLocation()).GetSafeNormal2D():FVector::YAxisVector;const FVector Side=FVector::CrossProduct(Forward,FVector::UpVector).GetSafeNormal();const FVector BendAxis=FVector::CrossProduct(FVector::UpVector,Forward).GetSafeNormal();
  TArray<TArray<FQuat>> PickupCS;PickupCS.SetNum(19);for(auto& Frame:PickupCS)Frame.SetNum(Ref.GetNum());
  for(int32 Bone=0;Bone<Ref.GetNum();++Bone){const FName BoneName=Ref.GetBoneName(Bone);TArray<FVector3f> Positions,Scales;TArray<FQuat4f> Rotations;
   for(int32 K=0;K<=18;++K){const float T=K/18.f;const float Wind=FMath::Sin(FMath::Clamp(T/.3f,0.f,1.f)*PI*.5f)*(T<.3f?1.f:FMath::Max(0.f,1.f-(T-.3f)/.7f));FRotator Add=FRotator::ZeroRotator;
    // Lower both arms from the reference pose, with an unmistakable wind-up, strike and recovery.
    if(BoneName==TEXT("upperarm_l"))Add=FRotator(0,0,-55);
    if(BoneName==TEXT("upperarm_r"))Add=Throw?FRotator(-80*Wind,35*Wind,55-65*Wind):FRotator(0,-80*Wind,55-35*Wind);
    if(BoneName==TEXT("lowerarm_r"))Add=FRotator(0,Throw?-70*Wind:-35*Wind,0);
    if(BoneName==TEXT("spine_03"))Add=FRotator(-8*Wind,Throw?-15*Wind:25*Wind,0);
    const FTransform& B=Pose[Bone];FQuat Local=B.GetRotation()*Add.Quaternion();
    if(Pickup){const int32 Parent=Ref.GetParentIndex(Bone);const FQuat ParentCS=Parent>=0?PickupCS[K][Parent]:FQuat::Identity;FQuat Desired=ParentCS*B.GetRotation();
     if(BoneName==TEXT("spine_01")||BoneName==TEXT("spine_02"))Desired=FQuat(BendAxis,FMath::DegreesToRadians(27.f*Wind))*Desired;
     const bool Upper=BoneName==TEXT("upperarm_l")||BoneName==TEXT("upperarm_r");const bool Lower=BoneName==TEXT("lowerarm_l")||BoneName==TEXT("lowerarm_r");
     if(Upper||Lower){const bool Left=BoneName.ToString().EndsWith(TEXT("_l"));const FName Child=Upper?(Left?FName(TEXT("lowerarm_l")):FName(TEXT("lowerarm_r"))):(Left?FName(TEXT("hand_l")):FName(TEXT("hand_r")));const int32 Next=Ref.FindBoneIndex(Child);
      if(Next>=0){const FVector Original=(RefCS[Next].GetLocation()-RefCS[Bone].GetLocation()).GetSafeNormal();const float Sign=FVector::DotProduct(RefCS[Bone].GetLocation(),Side)>=0?1.f:-1.f;const FVector Reach=(Forward*(Lower?.45f:.25f)*Wind-FVector::UpVector+Side*Sign*.1f).GetSafeNormal();Desired=FQuat::FindBetweenNormals(Original,Reach)*RefCS[Bone].GetRotation();}
     }PickupCS[K][Bone]=Desired;Local=ParentCS.Inverse()*Desired;
    }
    Positions.Add(FVector3f(B.GetTranslation()));Scales.Add(FVector3f(B.GetScale3D()));Rotations.Add(FQuat4f(Local));
   }
   C.AddBoneCurve(BoneName,false);if(!C.SetBoneTrackKeys(BoneName,Positions,Rotations,Scales,false))return false;
  }C.NotifyPopulated();C.CloseBracket(false);Anim->MarkPackageDirty();if(NewAsset)FAssetRegistryModule::AssetCreated(Anim);return SaveAsset(Anim);
 };
 if(!MakeAction(TEXT("A_StreetStrike"),false)||!MakeAction(TEXT("A_StreetThrow"),true)||!MakeAction(TEXT("A_StreetPickup"),false,true))return 16;
 if(FParse::Param(*Params,TEXT("UpdateBalance")))if(auto* B=LoadObject<UKTWUnitBalance>(nullptr,TEXT("/Game/KTW/Data/DA_UnitBalance_V4.DA_UnitBalance_V4"))){B->Units=UKTWUnitBalance::Defaults();if(!SaveAsset(B))return 14;}
 const TCHAR* BalancePath=TEXT("/Game/KTW/Data/DA_UnitBalance_V4");
 if(!LoadObject<UKTWUnitBalance>(nullptr,TEXT("/Game/KTW/Data/DA_UnitBalance_V4.DA_UnitBalance_V4"))){auto* P=CreatePackage(BalancePath);auto* Balance=NewObject<UKTWUnitBalance>(P,TEXT("DA_UnitBalance_V4"),RF_Public|RF_Standalone);Balance->Units=UKTWUnitBalance::Defaults();FAssetRegistryModule::AssetCreated(Balance);if(!SaveAsset(Balance))return 14;}
 // Reuse hand-edited AI assets on subsequent runs.
 auto* Blackboard=LoadObject<UBlackboardData>(nullptr,TEXT("/Game/KTW/AI/BB_Boss_V4.BB_Boss_V4"));
 if(!Blackboard){auto* P=CreatePackage(TEXT("/Game/KTW/AI/BB_Boss_V4"));Blackboard=NewObject<UBlackboardData>(P,TEXT("BB_Boss_V4"),RF_Public|RF_Standalone);
  auto Key=[&](FName Name,UBlackboardKeyType* Type){FBlackboardEntry E;E.EntryName=Name;E.KeyType=Type;Blackboard->Keys.Add(E);};
  Key(TEXT("Goal"),NewObject<UBlackboardKeyType_Vector>(Blackboard));Key(TEXT("Recovering"),NewObject<UBlackboardKeyType_Bool>(Blackboard));Key(TEXT("Retreating"),NewObject<UBlackboardKeyType_Bool>(Blackboard));Key(TEXT("TaskType"),NewObject<UBlackboardKeyType_Int>(Blackboard));Key(TEXT("FollowTarget"),NewObject<UBlackboardKeyType_Object>(Blackboard));
  FAssetRegistryModule::AssetCreated(Blackboard);if(!SaveAsset(Blackboard))return 12;
 }
 auto* Tree=LoadObject<UBehaviorTree>(nullptr,TEXT("/Game/KTW/AI/BT_Boss_V4.BT_Boss_V4"));
 if(!Tree){auto* P=CreatePackage(TEXT("/Game/KTW/AI/BT_Boss_V4"));Tree=NewObject<UBehaviorTree>(P,TEXT("BT_Boss_V4"),RF_Public|RF_Standalone);Tree->BlackboardAsset=Blackboard;
  auto* Root=NewObject<UBTComposite_Selector>(Tree,TEXT("BossPriority"));Tree->RootNode=Root;
  for(int32 Branch=0;Branch<3;++Branch){auto* Sequence=NewObject<UBTComposite_Sequence>(Tree);FBTCompositeChild Child;Child.ChildComposite=Sequence;
   if(Branch<2){auto* State=NewObject<UBTDecorator_KTWBossState>(Tree);State->bRecoveryBranch=Branch==0;Child.Decorators.Add(State);}
   if(Branch>0){FBTCompositeChild Move;Move.ChildTask=NewObject<UBTTask_KTWMoveOrder>(Tree);Sequence->Children.Add(Move);}
   FBTCompositeChild Wait;auto* W=NewObject<UBTTask_Wait>(Tree);W->WaitTime=.2f;W->RandomDeviation=0;Wait.ChildTask=W;Sequence->Children.Add(Wait);Root->Children.Add(Child);
  }
  FAssetRegistryModule::AssetCreated(Tree);if(!SaveAsset(Tree))return 13;
 }
 const TCHAR* Names[]={TEXT("PCG_TongLau_V4"),TEXT("PCG_LaneBarriers_V4"),TEXT("PCG_StreetWeapons_V4")};
 UMaterial* Mats[]={Facade,Barrier,Weapon};
 for(int32 I=0;I<3;++I) {
  const FString Path=FString(TEXT("/Game/KTW/PCG/"))+Names[I];
  if(LoadObject<UPCGGraph>(nullptr,*(Path+TEXT(".")+Names[I])))continue;
  UPackage* P=CreatePackage(*Path);P->FullyLoad();
  auto* G=NewObject<UPCGGraph>(P,Names[I],RF_Public|RF_Standalone);
  UKTWPCGSettings* Source=nullptr;auto* N=G->AddNodeOfType(Source);Source->Layer=EKTWLayer(I);N->SetNodePosition(0,0);
  UPCGStaticMeshSpawnerSettings* Spawn=nullptr;auto* SN=G->AddNodeOfType(Spawn);SN->SetNodePosition(350,0);
  Spawn->SetMeshSelectorType(UPCGMeshSelectorByAttribute::StaticClass());Spawn->bSynchronousLoad=true;
  auto* Selector=CastChecked<UPCGMeshSelectorByAttribute>(Spawn->MeshSelectorParameters);Selector->AttributeName=TEXT("Mesh");
  Selector->TemplateDescriptor.OverrideMaterials.Add(Mats[I]);Selector->bUseAttributeMaterialOverrides=true;Selector->MaterialOverrideAttributes.Add(TEXT("Material"));
  Selector->TemplateDescriptor.BodyInstance.SetCollisionProfileName(I==2?TEXT("NoCollision"):TEXT("BlockAll"));
  G->AddLabeledEdge(N,PCGPinConstants::DefaultOutputLabel,SN,PCGPinConstants::DefaultInputLabel);
  G->AddLabeledEdge(SN,PCGPinConstants::DefaultOutputLabel,G->GetOutputNode(),PCGPinConstants::DefaultOutputLabel);
  FAssetRegistryModule::AssetCreated(G);if(!SaveAsset(G))return 2;
 }
 UWorld* W=UEditorLoadingAndSavingUtils::NewBlankMap(false);if(!W)return 3;
 W->GetWorldSettings()->DefaultGameMode=AKTWPrototypeGameMode::StaticClass();
 auto* Director=W->SpawnActor<AKTWStreetDirector>();Director->SetActorLabel(TEXT("KTW_StreetDirector"));
 Director->StreetWeaponVisualScale=Visual->OverviewScaleMultiplier*Visual->SoldierScale;
 Director->Foundation->SetMaterial(0,Road);Director->RoadMarkings->SetMaterial(0,Mark);
 Director->UpperRoad->SetMaterial(0,Upper);Director->MainRoad->SetMaterial(0,Main);Director->LowerRoad->SetMaterial(0,Lower);Director->LinkAlley->SetMaterial(0,Link);
 Director->BuildFoundation();Director->AssignGraphs();
 int32 Failures=0;int32 Counts[3]={0,0,0};
 for(int32 Seed=0;Seed<100;++Seed) {
  Director->MapSeed=Seed;if(!Director->ValidateLayout())++Failures;
  auto A=Director->BuildLayer(EKTWLayer::Weapons);auto B=Director->BuildLayer(EKTWLayer::Weapons);
  if(A.Num()!=6||A.Num()!=B.Num())++Failures;
  for(int32 I=0;I<A.Num();++I) {
   if(!A[I].Transform.Equals(B[I].Transform)||A[I].Kind!=B[I].Kind)++Failures;
   if(I%2==0){++Counts[A[I].Kind];if(!FMath::IsNearlyEqual(A[I].Transform.GetLocation().X,A[I+1].Transform.GetLocation().Y)||A[I].Kind!=A[I+1].Kind)++Failures;}
  }
 }
 Director->MapSeed=5990;Director->ValidateLayout();
 UE_LOG(LogTemp,Display,TEXT("KTW SEED TEST: 100 seeds; failures=%d; weapon pair kinds=%d/%d/%d"),Failures,Counts[0],Counts[1],Counts[2]);
 if(Failures)return 4;
 auto* Light=W->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-55,-30,0));Light->SetActorLabel(TEXT("KTW_Sun"));
 Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);Light->GetLightComponent()->SetIntensity(5);
 auto* Sky=W->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetIntensity(1);
 W->SpawnActor<APlayerStart>(FVector(0,0,15500),FRotator::ZeroRotator);
 auto* Nav=W->SpawnActor<ANavMeshBoundsVolume>();Nav->SetActorLabel(TEXT("KTW_NavBounds"));
 auto* Builder=NewObject<UCubeBuilder>();Builder->X=14500;Builder->Y=14500;Builder->Z=1800;UActorFactory::CreateBrushForVolumeActor(Nav,Builder);Nav->SetActorLocation(FVector(0,0,600));Nav->GetBrushComponent()->SetCanEverAffectNavigation(false);
 for(int32 F=0;F<2;++F){auto* Zone=W->SpawnActor<ANavModifierVolume>();auto* Shape=NewObject<UCubeBuilder>();Shape->X=960;Shape->Y=960;Shape->Z=1000;UActorFactory::CreateBrushForVolumeActor(Zone,Shape);Zone->SetActorLocation(AKTWStreetDirector::BasePosition(F)+FVector(0,0,400));Zone->SetAreaClass(F==0?UKTWBlueSafeArea::StaticClass():UKTWRedSafeArea::StaticClass());Zone->SetActorLabel(F?TEXT("KTW_RedSafeZone"):TEXT("KTW_BlueSafeZone"));}
 Director->Regenerate();
 auto* PCG=W->GetSubsystem<UPCGSubsystem>();
 for(int32 I=0;I<6000&&Director->IsGenerating();++I){if(PCG)PCG->Tick(.016f);FPlatformProcess::Sleep(.005f);}
 if(Director->IsGenerating()){UE_LOG(LogTemp,Error,TEXT("PCG generation timeout"));return 5;}
 FNavigationSystem::AddNavigationSystemToWorld(*W,FNavigationSystemRunMode::EditorMode);
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);if(!NS)return 6;
 FlushAsyncLoading();
 NS->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock|ENavigationBuildLock::InitialLock|ENavigationBuildLock::NoUpdateInEditor,UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
 NS->OnNavigationBoundsUpdated(Nav);NS->Tick(.1f);FNavigationSystem::Build(*W);
 for(int32 I=0;I<100;++I){W->Tick(LEVELTICK_All,.05f);FPlatformProcess::Sleep(.01f);}

 int32 PathFailures=0;int32 PathCount=0;
 auto CheckPath=[&](FVector Start,FVector End,const FString& Name){
  Start.Z=End.Z=100;FNavLocation A,B;bool OK=NS->ProjectPointToNavigation(Start,A,FVector(120,120,180))&&NS->ProjectPointToNavigation(End,B,FVector(120,120,180));
  UNavigationPath* Path=OK?NS->FindPathToLocationSynchronously(W,A.Location,B.Location):nullptr;OK=Path&&Path->IsValid()&&!Path->IsPartial();
  if(OK){const float Direct=FVector::Dist2D(Start,End);float Length=0;for(int32 I=1;I<Path->PathPoints.Num();++I)Length+=FVector::Dist2D(Path->PathPoints[I-1],Path->PathPoints[I]);
   // Central arm pairs must connect locally, never by a long trip via outer roads.
   if(Name.StartsWith(TEXT("center"))&&Length>Direct*1.6f+300)OK=false;
  }
  ++PathCount;if(!OK)++PathFailures;UE_LOG(LogTemp,Display,TEXT("KTW NAV %s: %s"),*Name,OK?TEXT("PASS"):TEXT("FAIL"));
 };
 auto Nodes=AKTWStreetDirector::Junctions();
 for(int32 A=0;A<Nodes.Num();++A)for(int32 B=0;B<Nodes.Num();++B)if(A!=B)CheckPath(Nodes[A],Nodes[B],FString::Printf(TEXT("node%d->%d"),A,B));
 const TArray<FVector> Arms={FVector(-1300,-1300,0),FVector(1300,1300,0),FVector(-1300,1300,0),FVector(1300,-1300,0)};
 for(int32 A=0;A<4;++A)for(int32 B=0;B<4;++B)if(A!=B)CheckPath(Arms[A],Arms[B],FString::Printf(TEXT("center%d->%d"),A,B));
 for(int32 F=0;F<2;++F)for(int32 L=0;L<3;++L){auto R=AKTWStreetDirector::Route(L,F);for(int32 I=0;I<2;++I)CheckPath(R[I],R[I+1],FString::Printf(TEXT("faction%d route%d segment%d"),F,L,I));}
 // Sample the whole usable road surface, rather than just five junction centres.
 int32 SurfaceChecks=0,SurfaceFailures=0;
 auto CheckSurface=[&](FVector P){if(!AKTWStreetDirector::IsRoadPosition(P,-110))return;P.Z=100;FNavLocation N;
  bool OK=NS->ProjectPointToNavigation(P,N,FVector(55,55,200))&&FVector::Dist2D(P,N.Location)<=70;
  for(int32 F=0;F<2&&OK;++F){const FVector Base=AKTWStreetDirector::BasePosition(F);if(FVector::Dist2D(Base,N.Location)<70)continue;auto* Path=NS->FindPathToLocationSynchronously(W,Base+FVector(0,0,100),N.Location);OK=Path&&Path->IsValid()&&!Path->IsPartial();}
  ++SurfaceChecks;if(!OK){++SurfaceFailures;if(SurfaceFailures<=20)UE_LOG(LogTemp,Error,TEXT("KTW ROAD SURFACE FAIL %s"),*P.ToString());}
 };
 for(int32 X=-6900;X<=6900;X+=150)for(int32 Y=-6900;Y<=6900;Y+=150)CheckSurface(FVector(X,Y,100));
 for(FVector P:Nodes)for(float Radius:{150.f,300.f,500.f,700.f})for(int32 A=0;A<24;++A){const float Angle=A*2*PI/24;CheckSurface(P+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0));}
 UE_LOG(LogTemp,Display,TEXT("KTW ROAD SURFACE: samples=%d both-faction paths=%d failures=%d"),SurfaceChecks,SurfaceChecks*2,SurfaceFailures);
 if(SurfaceFailures)return 17;
 if(PathFailures)return 7;
 int32 InstanceCount=0;TArray<UInstancedStaticMeshComponent*> ISMs;Director->GetComponents(ISMs);for(auto* C:ISMs)InstanceCount+=C->GetInstanceCount();
 UE_LOG(LogTemp,Display,TEXT("KTW GENERATED INSTANCES: %d"),InstanceCount);const int32 Expected=Director->Foundation->GetInstanceCount()+Director->RoadMarkings->GetInstanceCount()+Director->UpperRoad->GetInstanceCount()+Director->MainRoad->GetInstanceCount()+Director->LowerRoad->GetInstanceCount()+Director->LinkAlley->GetInstanceCount()+Director->BuildLayer(EKTWLayer::Facades).Num()+Director->BuildLayer(EKTWLayer::Barriers).Num()+Director->BuildLayer(EKTWLayer::Weapons).Num();if(InstanceCount!=Expected)return 9;
 if(!UEditorLoadingAndSavingUtils::SaveMap(W,TEXT("/Game/KTW/Maps/L_Kowloon_ReferenceV4")))return 8;
 UE_LOG(LogTemp,Display,TEXT("KTW V4 BUILD SUCCESS: 3 PCG graphs, 100 layout seeds, 44 nav paths including all central turns; skeletal visuals and boss BT saved."));return 0;
#else
 return 1;
#endif
}






