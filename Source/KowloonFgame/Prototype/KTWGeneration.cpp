#include "KTWGeneration.h"
#include "KTWImportedArt.h"
#include "KTWSession.h"
#include "KTWPresentation.h"
#include "KTWGold.h"
#include "KTWDynamics.h"
#include "Algo/Reverse.h"
#include "PCGComponent.h"
#include "PCGContext.h"
#include "PCGGraphExecutionStateInterface.h"
#include "PCGPin.h"
#include "PCGPoint.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Components/PointLightComponent.h"
#include "PCGGraph.h"
#include "UObject/ConstructorHelpers.h"

namespace {
FKTWPlacement Place(FVector P,FVector Scale,int32 Kind=0,int32 Lane=-1,float Yaw=0) {
 FKTWPlacement V; V.Transform=FTransform(FRotator(0,Yaw,0),P,Scale); V.Kind=Kind; V.Lane=Lane; return V;
}
// Decorative courtyards remain inside the existing non-playable district plinths.
bool AlleyLot(FVector P){const int X=FMath::RoundToInt((P.X+6750)/500),Y=FMath::RoundToInt((P.Y+6750)/500);return FMath::Abs(P.X)<6200&&FMath::Abs(P.Y)<6200&&(X%4==1||Y%5==2)&&!AKTWStreetDirector::IsScaffoldLot(P);}
void StreetMarketMeshes(AActor* Holder,USceneComponent* Root,const TArray<FKTWPlacement>& Layout){
 TMap<int,UInstancedStaticMeshComponent*> Groups;TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> Lamps;TArray<TArray<FKTWPlacement>> Signals;
 auto Component=[&](int Kind){auto** Existing=Groups.Find(Kind);if(Existing)return *Existing;auto* C=NewObject<UInstancedStaticMeshComponent>(Holder,*FString::Printf(TEXT("StreetDecoration%d"),Kind));Holder->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);
  const bool Round=Kind==46||(Kind>=48&&Kind<=50)||Kind==54||Kind==55;
  C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Round?TEXT("/Engine/BasicShapes/Sphere.Sphere"):TEXT("/Engine/BasicShapes/Cube.Cube")));
  const TCHAR* Mat=Kind==44||Kind==45?TEXT("M_SignalBody"):Kind==46?TEXT("M_SignalOff"):Kind==48?TEXT("M_SignalRed"):Kind==49?TEXT("M_SignalAmber"):Kind==50?TEXT("M_SignalGreen"):Kind==52?TEXT("M_FacadeWarm"):Kind==53||Kind==55?TEXT("M_FacadeGreen"):Kind==54?TEXT("M_FacadeWarm"):Kind==51||Kind==56?TEXT("M_FacadeWarm"):Kind==57?TEXT("M_Sign0"):TEXT("M_Aircon");
  if(auto* M=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/KTW/Materials/%s.%s"),Mat,Mat)))C->SetMaterial(0,M);FKTWImportedArt::Configure(C,Kind);C->RegisterComponent();Groups.Add(Kind,C);return C;};
 for(const auto& V:Layout)if(V.Kind>=44&&V.Kind<=57){auto* C=Component(V.Kind);C->AddInstance(FKTWImportedArt::Enabled()&&FKTWImportedArt::Data()->Meshes.Contains(V.Kind)?FKTWImportedArt::Fit(V.Transform,C->GetStaticMesh()):V.Transform);}
 for(int Kind=48;Kind<=50;++Kind){Lamps.Add(Component(Kind));TArray<FKTWPlacement> Points;for(const auto& V:Layout)if(V.Kind==Kind)Points.Add(V);Signals.Add(Points);}
 auto Update=[Weak=TWeakObjectPtr<AActor>(Holder),Lamps,Signals](){if(!Weak.IsValid())return;const float Clock=FMath::Fmod(Weak->GetWorld()->GetTimeSeconds(),40.f);const int Phase=Clock<16?0:Clock<20?1:Clock<36?2:3;
  for(int Color=0;Color<3;++Color)if(auto* C=Lamps[Color].Get())for(int Index=0;Index<Signals[Color].Num();++Index){const auto& V=Signals[Color][Index];const int Wanted=V.Lane==0?(Phase==0?50:Phase==1?49:48):(Phase==2?50:Phase==3?49:48);FTransform T=V.Transform;if(V.Kind!=Wanted)T.SetScale3D(FVector(.001));C->UpdateInstanceTransform(Index,T,false,true);}
 };
 Update();FTimerHandle Timer;Holder->GetWorldTimerManager().SetTimer(Timer,FTimerDelegate::CreateWeakLambda(Holder,Update),.5f,true);
}
void SyncPavementLights(AKTWStreetDirector* Director,const TArray<FKTWPlacement>& Layout){
  auto* World=Director->GetWorld();if(!World->IsGameWorld())return;
  TArray<AActor*> Old;for(TActorIterator<AActor>I(World);I;++I)if(I->GetOwner()==Director&&I->ActorHasTag(TEXT("KTW_PavementLighting")))Old.Add(*I);for(auto* A:Old)A->Destroy();
  auto* Holder=World->SpawnActor<AActor>();if(!Holder)return;Holder->SetOwner(Director);Holder->Tags.Add(TEXT("KTW_PavementLighting"));auto* Root=NewObject<USceneComponent>(Holder);Holder->AddInstanceComponent(Root);Holder->SetRootComponent(Root);Root->RegisterComponent();
  auto* Bulbs=NewObject<UInstancedStaticMeshComponent>(Holder,TEXT("StreetLampGlow"));Holder->AddInstanceComponent(Bulbs);Bulbs->SetupAttachment(Root);Bulbs->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Bulbs->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/KTW/Materials/M_NeonGold.M_NeonGold")));Bulbs->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bulbs->SetCanEverAffectNavigation(false);Bulbs->SetCastShadow(false);Bulbs->RegisterComponent();
  int Count=0;for(const auto& P:Layout)if(P.Kind==22&&Count++<24){auto* Light=NewObject<UPointLightComponent>(Holder);Holder->AddInstanceComponent(Light);Light->SetupAttachment(Root);Light->SetMobility(EComponentMobility::Movable);Light->SetRelativeLocation(P.Transform.GetLocation()-FVector(0,0,65));Light->SetLightColor(FLinearColor(1,.82f,.58f));Light->SetIntensity(0);Light->SetAttenuationRadius(2600);Light->SetCastShadows(false);Light->ComponentTags.Add(TEXT("KTW_StreetLamp"));Light->RegisterComponent();Bulbs->AddInstance(FTransform(FRotator::ZeroRotator,P.Transform.GetLocation()-FVector(0,0,45),FVector(.65,.65,.25)));}
  auto Update=[Holder,Bulbs](){auto* W=AKTWWeather::Find(Holder->GetWorld());const bool Dark=W&&(W->Preset==1||W->Preset==3);const bool Rain=W&&W->Preset==2;TArray<UPointLightComponent*> Lights;Holder->GetComponents(Lights);for(auto* L:Lights)if(L->ComponentHasTag(TEXT("KTW_StreetLamp"))){L->SetIntensity(Dark?24000.f:Rain?11000.f:0.f);L->SetVisibility(Dark||Rain);}Bulbs->SetVisibility(Dark||Rain);};Update();FTimerHandle LightTimer;World->GetTimerManager().SetTimer(LightTimer,FTimerDelegate::CreateWeakLambda(Holder,Update),.5f,true);
  StreetMarketMeshes(Holder,Root,Layout);
  UE_LOG(LogTemp,Display,TEXT("KTW PAVEMENT LIGHTS active=%d"),FMath::Min(Count,24));
}
class FKTWLayoutElement : public IPCGElement {
public:
 virtual bool CanExecuteOnlyOnMainThread(FPCGContext*) const override { return true; }
 virtual bool IsCacheable(const UPCGSettings*) const override { return false; }
protected:
 virtual bool ExecuteInternal(FPCGContext* C) const override {
  const auto* Settings=C->GetInputSettings<UKTWPCGSettings>();
  auto* Source=C->ExecutionSource.Get();
  auto* Director=Source ? Source->GetExecutionState().GetTypedTarget<AKTWStreetDirector>() : nullptr;
  if(!Director || !Settings) return true;
  auto* Data=NewObject<UPCGPointData>();
  auto* Kind=Data->Metadata->CreateAttribute<int32>(FName("WeaponType"),-1,false,true);
  auto* Style=Data->Metadata->CreateAttribute<int32>(FName("BuildingStyle"),-1,false,true);
  auto* Shop=Data->Metadata->CreateAttribute<int32>(FName("ShopType"),-1,false,true);
  auto* Lane=Data->Metadata->CreateAttribute<int32>(FName("LaneId"),-1,false,true);
  auto* Mesh=Data->Metadata->CreateAttribute<FSoftObjectPath>(FName("Mesh"),FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")),false,true);
  auto* Material=Data->Metadata->CreateAttribute<FSoftObjectPath>(FName("Material"),FSoftObjectPath(),false,true);
  auto& Points=Data->GetMutablePoints();
  const auto Layout=Director->BuildLayer(Settings->Layer);
  if(Settings->Layer==EKTWLayer::Facades||Settings->Layer==EKTWLayer::Barriers)FKTWImportedArt::Render(Director,Layout,Settings->Layer);
  if(Settings->Layer==EKTWLayer::Facades)SyncPavementLights(Director,Layout);
  if(Settings->Layer==EKTWLayer::Atmosphere&&Director->GetWorld()->IsGameWorld())if(auto* Weather=AKTWWeather::Find(Director->GetWorld()))Weather->ApplySeed(Director->MapSeed);
  if(Settings->Layer==EKTWLayer::GoldSources&&Director->GetWorld()->IsGameWorld())if(auto* Gold=AKTWGoldSystem::Find(Director->GetWorld()))Gold->CreatePCGSources(Layout);
  for(const auto& V:Layout) {
   if(FKTWImportedArt::Replaces(V,Settings->Layer))continue;
   if(Settings->Layer==EKTWLayer::Facades&&V.Kind>=44&&V.Kind<=57)continue; // Runtime decorations are nonblocking, signals animate independently.
   if(Settings->Layer==EKTWLayer::Weapons&&AKTWStreetLife::Find(Director->GetWorld()))continue; // Gameplay pickups replace decorative cubes.
   FPCGPoint& P=Points.Emplace_GetRef(); P.Transform=V.Transform*Director->GetActorTransform(); P.Density=1; P.BoundsMin=FVector(-50); P.BoundsMax=FVector(50);
   P.Seed=GetTypeHash(P.Transform.GetLocation()); P.MetadataEntry=Data->Metadata->AddEntry();
   Shop->SetValue(P.MetadataEntry,V.Kind==25?0:V.Kind==26?1:V.Kind==29?2:V.Kind==30?3:-1);Style->SetValue(P.MetadataEntry,V.BuildingStyleId);Kind->SetValue(P.MetadataEntry,V.Kind); Lane->SetValue(P.MetadataEntry,V.Lane);
   Mesh->SetValue(P.MetadataEntry,FSoftObjectPath(Settings->Layer==EKTWLayer::Facades&&(V.Kind==9||V.Kind==20||V.Kind==42)?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube")));
   FString Mat=TEXT("M_Facade");
   if(Settings->Layer==EKTWLayer::Facades) {const TCHAR* Palette[]={TEXT("M_Facade"),TEXT("M_Window"),TEXT("M_Aircon"),TEXT("M_NeonRed"),TEXT("M_Roof"),TEXT("M_NeonBlue"),TEXT("M_NeonGold"),TEXT("M_FacadeWarm"),TEXT("M_FacadeGreen"),TEXT("M_Aircon"),TEXT("M_Aircon"),TEXT("M_Sign0"),TEXT("M_Sign1"),TEXT("M_Sign2"),TEXT("M_Sign3"),TEXT("M_Sign4"),TEXT("M_Sign5"),TEXT("M_Awning"),TEXT("M_Shutter"),TEXT("M_Aircon"),TEXT("M_FacadeWarm"),TEXT("M_Aircon"),TEXT("M_NeonGold"),TEXT("M_FacadeWarm"),TEXT("M_Window"),TEXT("M_Sign6"),TEXT("M_Sign7"),TEXT("M_NeonRed"),TEXT("M_NeonBlue"),TEXT("M_Sign0"),TEXT("M_Sign4")};Mat=V.Kind==37?TEXT("M_UrbanPaving"):V.Kind==38?TEXT("M_UrbanPaint"):V.Kind==39?TEXT("M_Street"):V.Kind==40?TEXT("M_FacadeGreen"):V.Kind==41?TEXT("M_FacadeWarm"):V.Kind==42?TEXT("M_Aircon"):V.Kind==43?TEXT("M_NeonGold"):V.Kind>=31&&V.Kind<=36?FString::Printf(TEXT("M_Urban%d"),V.Kind-31):FString(Palette[FMath::Clamp(V.Kind,0,30)]);}
   else Mat=Settings->Layer==EKTWLayer::Barriers?(V.Kind==0?TEXT("M_Barrier"):TEXT("M_Bamboo")):TEXT("M_Weapon");
   Material->SetValue(P.MetadataEntry,FSoftObjectPath(FString::Printf(TEXT("/Game/KTW/Materials/%s.%s"),*Mat,*Mat)));
  }
  auto& Out=C->OutputData.TaggedData.Emplace_GetRef(); Out.Data=Data; Out.Pin=PCGPinConstants::DefaultOutputLabel;
  return true;
 }
};
}
TArray<FPCGPinProperties> UKTWPCGSettings::OutputPinProperties() const { return {FPCGPinProperties(PCGPinConstants::DefaultOutputLabel,EPCGDataType::Point)}; }
FPCGElementPtr UKTWPCGSettings::CreateElement() const { return MakeShared<FKTWLayoutElement>(); }
AKTWStreetDirector::AKTWStreetDirector() {
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Boundary=CreateDefaultSubobject<USplineComponent>(TEXT("ClosedFacadeBoundary")); Boundary->SetupAttachment(RootComponent);
 Foundation=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FixedStreetSkeleton")); Foundation->SetupAttachment(RootComponent);
 RoadMarkings=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NonBlockingMarkers")); RoadMarkings->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Foundation->SetStaticMesh(Cube.Object); RoadMarkings->SetStaticMesh(Cube.Object);
 Foundation->SetCollisionProfileName(TEXT("BlockAll")); RoadMarkings->SetCollisionEnabled(ECollisionEnabled::NoCollision); RoadMarkings->SetCanEverAffectNavigation(false);
 Facades=CreateDefaultSubobject<UPCGComponent>(TEXT("PCG_Facades"));
 Barriers=CreateDefaultSubobject<UPCGComponent>(TEXT("PCG_Barriers"));
 Weapons=CreateDefaultSubobject<UPCGComponent>(TEXT("PCG_Weapons"));
 GoldSources=CreateDefaultSubobject<UPCGComponent>(TEXT("PCG_GoldSources"));
 Atmosphere=CreateDefaultSubobject<UPCGComponent>(TEXT("PCG_Atmosphere"));
 auto Visual=[&](const TCHAR* Name){auto* C=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);C->SetupAttachment(RootComponent);C->SetStaticMesh(Cube.Object);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);return C;};
 UpperRoad=Visual(TEXT("UpperRoadSurface"));MainRoad=Visual(TEXT("MainRoadSurface"));LowerRoad=Visual(TEXT("LowerRoadSurface"));LinkAlley=Visual(TEXT("LinkAlleySurface"));
 for(auto* C:{Facades.Get(),Barriers.Get(),Weapons.Get(),GoldSources.Get(),Atmosphere.Get()}) C->bIsComponentPartitioned=false;
}

namespace {
float SegmentDistance(FVector P,FVector A,FVector B){P.Z=A.Z=B.Z=0;const FVector AB=B-A;return FVector::Dist2D(P,A+AB*FMath::Clamp(FVector::DotProduct(P-A,AB)/AB.SizeSquared(),0.,1.));}
void SegmentInstances(UInstancedStaticMeshComponent* C,FVector A,FVector B,float Width,float Z){const FVector D=B-A;const FTransform Target(FRotator(0,D.Rotation().Yaw,0),(A+B)*.5+FVector(0,0,Z),FVector(D.Size2D()/100,Width/100,.025));if(C->GetFName()==TEXT("UrbanPaving")&&FKTWImportedArt::Enabled()){const int N=FMath::CeilToInt(D.Size2D()/350);for(int K=0;K<N;++K){const FTransform T(Target.GetRotation(),A+D*((K+.5f)/N)+FVector(0,0,Z),FVector(D.Size2D()/N/100,Width/100,.025));C->AddInstance(FKTWImportedArt::Fit(T,C->GetStaticMesh()));}}else C->AddInstance(Target);}
}
FVector AKTWStreetDirector::BasePosition(int32 F){return F==0?FVector(-6000,6000,110):FVector(6000,-6000,110);}
TArray<FVector> AKTWStreetDirector::Junctions(){return {FVector(-6000,6000,0),FVector(-6000,-6000,0),FVector(0,0,0),FVector(6000,6000,0),FVector(6000,-6000,0)};}
TArray<FVector> AKTWStreetDirector::Route(int32 Lane,int32 Faction){auto N=Junctions();TArray<FVector> R={N[0],N[Lane==0?1:(Lane==1?2:3)],N[4]};if(Faction==1)Algo::Reverse(R);return R;}
TArray<FVector> AKTWStreetDirector::GoldAnchors(){return {FVector(-6000,0,20),FVector(0,-6000,20),FVector(-2600,2600,20),FVector(2600,-2600,20),FVector(0,6000,20),FVector(6000,0,20)};}
bool AKTWStreetDirector::IsRoadPosition(FVector P,float Extra){
 auto N=Junctions();const int32 Edges[][2]={{0,1},{1,4},{0,2},{2,4},{0,3},{3,4},{1,2},{2,3}};
 const float Widths[]={600,600,900,900,750,750,350,350};
 for(int32 I=0;I<8;++I)if(SegmentDistance(P,N[Edges[I][0]],N[Edges[I][1]])<Widths[I]+Extra)return true;
 return FVector::Dist2D(P,N[0])<1050+Extra||FVector::Dist2D(P,N[4])<1050+Extra;
}
TArray<FVector> AKTWStreetDirector::BuildingLots(){TArray<FVector> R;for(int32 X=-6750;X<=6750;X+=500)for(int32 Y=-6750;Y<=6750;Y+=500){FVector P(X,Y,0);if(!IsRoadPosition(P,355))R.Add(P);}return R;}
bool AKTWStreetDirector::IsScaffoldLot(FVector P){return (GetTypeHash(FIntPoint(FMath::RoundToInt(P.X),FMath::RoundToInt(P.Y)))%9)==0&&FMath::Abs(P.X)<5500&&FMath::Abs(P.Y)<5500;}
void AKTWStreetDirector::BuildFoundation(){
 Boundary->ClearSplinePoints(false);
 for(FVector P:{FVector(-7300,-7300,0),FVector(7300,-7300,0),FVector(7300,7300,0),FVector(-7300,7300,0)})Boundary->AddSplinePoint(P,ESplineCoordinateSpace::Local,false);
 for(int32 I=0;I<4;++I)Boundary->SetSplinePointType(I,ESplinePointType::Linear,false);Boundary->SetClosedLoop(true,true);
 for(auto* C:{Foundation.Get(),RoadMarkings.Get(),UpperRoad.Get(),MainRoad.Get(),LowerRoad.Get(),LinkAlley.Get()})C->ClearInstances();
 Foundation->AddInstance(FTransform(FVector(0,0,-50)));Foundation->UpdateInstanceTransform(0,FTransform(FRotator::ZeroRotator,FVector(0,0,-50),FVector(360,360,1)));
 // Visible curbs stay at street level; retain the original navigation volumes separately.
 UInstancedStaticMeshComponent* Blockers=nullptr;TArray<UInstancedStaticMeshComponent*> Existing;GetComponents(Existing);for(auto* C:Existing)if(C->GetFName()==TEXT("DistrictNavigationBlockers"))Blockers=C;
 if(!Blockers){Blockers=NewObject<UInstancedStaticMeshComponent>(this,TEXT("DistrictNavigationBlockers"));AddInstanceComponent(Blockers);Blockers->SetupAttachment(RootComponent);Blockers->SetStaticMesh(Foundation->GetStaticMesh());Blockers->SetCollisionProfileName(TEXT("BlockAll"));Blockers->SetCanEverAffectNavigation(true);Blockers->SetVisibility(false);Blockers->SetHiddenInGame(true);Blockers->RegisterComponent();}
 Blockers->ClearInstances();
 for(FVector P:BuildingLots()){Foundation->AddInstance(FTransform(FRotator::ZeroRotator,P,FVector(5,5,.16)));Blockers->AddInstance(FTransform(FRotator::ZeroRotator,P+FVector(0,0,110),FVector(5,5,2.2)));}

 // Surface-only street details: neither curbs nor paint obstruct units or navigation.
 auto UrbanMesh=[&](const FName Name,const TCHAR* Mat){UInstancedStaticMeshComponent* C=nullptr;TArray<UInstancedStaticMeshComponent*> Cs;GetComponents(Cs);for(auto* Existing:Cs)if(Existing->GetFName()==Name)C=Existing;
  if(!C){C=NewObject<UInstancedStaticMeshComponent>(this,Name);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(Foundation->GetStaticMesh());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->RegisterComponent();}
  C->ClearInstances();if(auto* M=LoadObject<UMaterialInterface>(nullptr,Mat))C->SetMaterial(0,M);return C;};
 auto* Paving=UrbanMesh(TEXT("UrbanPaving"),TEXT("/Game/KTW/Materials/M_UrbanPaving.M_UrbanPaving"));
 auto* Paint=UrbanMesh(TEXT("UrbanPaint"),TEXT("/Game/KTW/Materials/M_UrbanPaint.M_UrbanPaint"));
 FKTWImportedArt::Configure(Paving,37);
 for(int Lane=0;Lane<3;++Lane){const float Full=Lane==0?1200:Lane==1?1800:1500,Walk=Lane==0?150:Lane==1?300:250;auto Points=Route(Lane);
  for(int Edge=0;Edge<2;++Edge){const FVector A=Points[Edge],B=Points[Edge+1],Dir=(B-A).GetSafeNormal(),Normal(-Dir.Y,Dir.X,0);const float Length=FVector::Dist2D(A,B);
   for(float Side:{-1.f,1.f})SegmentInstances(Paving,A+Dir*1100+Normal*Side*(Full-Walk)/2,B-Dir*1100+Normal*Side*(Full-Walk)/2,Walk,8);
   for(float End:{1300.f,Length-1300})for(float Offset=-Full/2+Walk+50;Offset<Full/2-Walk;Offset+=90)
    Paint->AddInstance(FTransform(FRotator(0,Dir.Rotation().Yaw,0),A+Dir*End+Normal*Offset+FVector(0,0,10),FVector(3.2,.42,.025)));
  }
 }
 UInstancedStaticMeshComponent* Surfaces[]={UpperRoad,MainRoad,LowerRoad};
 for(int32 L=0;L<3;++L){auto R=Route(L);for(int32 I=0;I<2;++I){SegmentInstances(Surfaces[L],R[I],R[I+1],L==0?1200:(L==1?1800:1500),2+L*.2f);
  FVector D=(R[I+1]-R[I]).GetSafeNormal();for(float T=600;T<(R[I+1]-R[I]).Size2D()-400;T+=700){FVector P=R[I]+D*T;RoadMarkings->AddInstance(FTransform(FRotator(0,D.Rotation().Yaw,0),P+FVector(0,0,5),FVector(2.6,.16,.025)));}
 }}
 auto N=Junctions();SegmentInstances(LinkAlley,N[1],N[3],700,3.5f);
 FVector D=(N[3]-N[1]).GetSafeNormal();for(float T=500;T<(N[3]-N[1]).Size2D();T+=500)RoadMarkings->AddInstance(FTransform(FRotator(0,45,0),N[1]+D*T+FVector(0,0,6),FVector(1.8,.13,.025)));
 
 for(FVector P:{N[0],N[4]})RoadMarkings->AddInstance(FTransform(FRotator::ZeroRotator,P+FVector(0,0,7),FVector(10,10,.08)));
}
void AKTWStreetDirector::AssignGraphs() {
 const TCHAR* Paths[]={TEXT("/Game/KTW/PCG/PCG_TongLau_V4.PCG_TongLau_V4"),TEXT("/Game/KTW/PCG/PCG_LaneBarriers_V4.PCG_LaneBarriers_V4"),TEXT("/Game/KTW/PCG/PCG_StreetWeapons_V4.PCG_StreetWeapons_V4"),TEXT("/Game/KTW/PCG/PCG_GoldSources_V48.PCG_GoldSources_V48"),TEXT("/Game/KTW/PCG/PCG_Weather_V49.PCG_Weather_V49")};
 UPCGComponent* Cs[]={Facades,Barriers,Weapons,GoldSources,Atmosphere};
 for(int32 I=0;I<5;++I) if(auto* G=LoadObject<UPCGGraph>(nullptr,Paths[I])) Cs[I]->SetGraph(G);
}
void AKTWStreetDirector::OnConstruction(const FTransform& T) { Super::OnConstruction(T); BuildFoundation(); }
void AKTWStreetDirector::BeginPlay() { Super::BeginPlay();if(auto* S=UKTWSession::Get(this))if(!UKTWSession::Diagnostic())MapSeed=S->RoundSeed;Regenerate();
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWGroundTest"))){FTimerHandle Verify;GetWorldTimerManager().SetTimer(Verify,FTimerDelegate::CreateWeakLambda(this,[this](){
 int32 Fail=0;auto Check=[&](bool OK,const TCHAR* Name){Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW GROUND %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 bool Low=true;for(int32 I=1;I<Foundation->GetInstanceCount();++I){FTransform T;Foundation->GetInstanceTransform(I,T);Low&=T.GetLocation().Z+50*T.GetScale3D().Z<=8.01f;}Check(Low,TEXT("visible district bases no higher than 8cm"));
 UInstancedStaticMeshComponent* Blockers=nullptr;TArray<UInstancedStaticMeshComponent*> All;GetComponents(All);for(auto* C:All)if(C->GetFName()==TEXT("DistrictNavigationBlockers"))Blockers=C;
 Check(Blockers&&Blockers->GetInstanceCount()==BuildingLots().Num()&&!Blockers->IsVisible()&&Blockers->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&Blockers->CanEverAffectNavigation(),TEXT("invisible navigation blockers retained for every lot"));
 bool AtGround=true;for(int32 Seed=0;Seed<10;++Seed)for(const auto& V:BuildLayer(EKTWLayer::Facades,Seed))if(V.Kind>=100&&V.Kind<120&&FMath::Abs(V.Transform.GetLocation().X)<7000&&FMath::Abs(V.Transform.GetLocation().Y)<7000)AtGround&=FMath::Abs(V.Transform.GetLocation().Z-V.Transform.GetScale3D().Z*50)<.1f;Check(AtGround,TEXT("central building feet remain at ground across ten seeds"));
 const FVector P=BuildingLots()[0];FHitResult Hit;FCollisionQueryParams Q;const bool HitBuilding=GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,500),P-FVector(0,0,50),ECC_Visibility,Q);Check(HitBuilding,TEXT("district remains physically blocking"));
 UE_LOG(LogTemp,Display,TEXT("KTW GROUND COMPLETE failures=%d"),Fail);FPlatformMisc::RequestExitWithStatus(false,Fail?1:0);
 }),4.f,false);}
#endif
 }
void AKTWStreetDirector::Regenerate() {
 if(IsGenerating()) return;
 BuildFoundation();
 if(!ValidateLayout()) {
  bool Valid=false;
  for(int32 Retry=0;Retry<3&&!Valid;++Retry){MapSeed=GetTypeHash(MapSeed+Retry+1)&0x7fffffff;Valid=ValidateLayout();}
  if(!Valid){MapSeed=5990;Valid=ValidateLayout();}
  if(!Valid){UE_LOG(LogTemp,Error,TEXT("KTW: invalid director transform; generation aborted. Keep unit scale and zero rotation."));return;}
 }
 AssignGraphs();
 for(auto* C:{Facades.Get(),Barriers.Get(),Weapons.Get(),GoldSources.Get(),Atmosphere.Get()}) { C->CleanupLocalImmediate(true); C->GenerateLocal(true); }
}
void AKTWStreetDirector::NextSeed() { if(!IsGenerating()){++MapSeed;Regenerate();} }
bool AKTWStreetDirector::IsGenerating() const { return Facades->IsGenerating()||Barriers->IsGenerating()||Weapons->IsGenerating()||GoldSources->IsGenerating()||Atmosphere->IsGenerating(); }

TArray<FKTWPlacement> AKTWStreetDirector::BuildLayer(EKTWLayer Layer,int32 Override)const {
 TArray<FKTWPlacement> R;const int32 S=Override==INDEX_NONE?MapSeed:Override;FRandomStream Rand(int32(HashCombineFast(uint32(S),uint32(Layer)+7919)));
 int32 ExteriorIndex=0;
 auto Building=[&](FVector P,float Yaw,float Width,float Depth,bool Interior){
  const int32 Shop=Interior?-1:((ExteriorIndex%3==0)?((ExteriorIndex/3+(S&3))%4):-1);if(!Interior)++ExteriorIndex;
  const int32 Style=Rand.RandRange(0,3),Floors=Rand.RandRange(6,11);const float FloorH=360,H=Floors*FloorH;const int32 Wall=31+Rand.RandRange(0,5);const FRotator Facing(0,Yaw,0);
  if(FKTWImportedArt::Enabled()){
   const int Variant=Rand.RandRange(0,7);R.Add(Place(P+FVector(0,0,H/2),FVector(Width/100,Depth/100,H/100),100+Variant,-1,Yaw));R.Last().BuildingStyleId=Variant%4;
   if(Shop>=0)R.Add(Place(P+Facing.RotateVector(FVector(0,Depth/2+35,460)),FVector(Width/100,.18,1.3),Shop==0?25:Shop==1?26:Shop==2?29:30,-1,Yaw));
   for(int F=2;F<Floors;F+=3)R.Add(Place(P+Facing.RotateVector(FVector(Width*.28,Depth/2+25,F*360+80)),FVector(.75,.5,.6),2,-1,Yaw));return;
  }
  auto Add=[&](FVector Local,FVector Scale,int32 Kind){R.Add(Place(P+Facing.RotateVector(Local),Scale,Kind,-1,Yaw));R.Last().BuildingStyleId=Style;};
  if(Style==3){Add(FVector(0,0,H/2),FVector((Width-140)/100,Depth/100,H/100),Wall);Add(FVector(0,0,H/2),FVector(Width/100,(Depth-140)/100,H/100),Wall);for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})Add(FVector(X*(Width/2-70),Y*(Depth/2-70),H/2),FVector(1.4,1.4,H/100),20);}
  else Add(FVector(0,0,H/2),FVector(Width/100,Depth/100,H/100),Wall);
  Add(FVector(0,0,H+18),FVector((Width+12)/100,(Depth+12)/100,.36),4);
  for(float SideSign:{-1.f,1.f}){Add(FVector(SideSign*(Width/2-12),0,H+95),FVector(.18,Depth/100,1.5),Wall);Add(FVector(0,SideSign*(Depth/2-12),H+95),FVector(Width/100,.18,1.5),Wall);}
  Add(FVector(-100,90,H+155),FVector(1.25,1.25,2.2),9);Add(FVector(100,-100,H+60),FVector(1.1,.8,.9),2);
  if(!Interior||Rand.FRand()<.2f)Add(FVector(0,0,H+215),FVector(3.4,1.7,.06),11+Rand.RandRange(0,5));
  TArray<FVector> Faces;if(!Interior)Faces.Add(FRotator(0,Yaw+90,0).Vector());else for(FVector V:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)})if(IsRoadPosition(P+V*600,180)||AlleyLot(P+V*500))Faces.Add(V);
  for(FVector N:Faces){const FVector T(-N.Y,N.X,0);const float Rot=T.Rotation().Yaw;auto Face=[&](float Along,float Out,float Z,FVector Scale,int32 Kind){Along*=Width/500;Scale.X*=Width/500;R.Add(Place(P+N*Out+T*Along+FVector(0,0,Z),Scale,Kind,-1,Rot));R.Last().BuildingStyleId=Style;};
   const int32 OrdinarySign=11+Rand.RandRange(0,5);if(Shop<0){Face(0,Depth/2+8,145,FVector(2.4,.1,2.5),18);Face(0,Depth/2+32,305,FVector(4.1,.65,.22),17);Face(0,Depth/2+16,410,FVector(3.8,.12,1.3),OrdinarySign);}
   if(Shop>=0){
    // Ground-floor businesses are facade modules, not extra obstacles across the road.
    const int SignKind=Shop==0?25:Shop==1?26:Shop==2?29:30;
    Face(0,Depth/2+28,340,FVector(3.9,.12,4.8),24);
    Face(0,Depth/2+55,690,FVector(4.8,.18,1.6),SignKind);
    if(Shop==0){
     for(float A:{-212.f,212.f})Face(A,Depth/2+42,325,FVector(.12,.12,5.8),27);
     Face(0,Depth/2+46,620,FVector(4.4,.16,.12),28);
     Face(0,Depth/2+85,800,FVector(4.9,1.2,.18),4);
     for(float A:{-90.f,90.f})Face(A,Depth/2+40,140,FVector(.7,.12,.12),6);
    }else if(Shop==1){
     for(float A:{-215.f,215.f})Face(A,Depth/2+52,320,FVector(.35,.35,6.4),27);
     Face(0,Depth/2+72,595,FVector(4.9,.9,.25),17);
     for(float A:{-165.f,165.f}){Face(A,Depth/2+70,470,FVector(.55,.55,.85),6);Face(A,Depth/2+70,410,FVector(.06,.06,.4),27);}
    }else if(Shop==2){
     for(float Z:{520.f,570.f})Face(0,Depth/2+65,Z,FVector(4.9,1.0,.13),8);
     for(float A:{-130.f,0.f,130.f})Face(A,Depth/2+38,255,FVector(.07,.12,3.6),10);
     Face(0,Depth/2+34,95,FVector(4.6,.18,.8),8);
    }else{
     Face(0,Depth/2+42,290,FVector(2.8,.1,3.4),18);
     for(float A:{-140.f,-70.f,0.f,70.f,140.f})Face(A,Depth/2+50,290,FVector(.045,.08,3.5),6);
     Face(175,Depth/2+60,1050,FVector(1.25,.18,3.4),30);
    }
   }
   // Arcaded shops, iron balconies, narrow shop-houses and rounded corner tenements.
   if(Shop<0&&Style==0)for(float A:{-205.f,205.f}){Face(A,Depth/2+48,170,FVector(.28,.28,3.4),Wall);Face(A,Depth/2+48,330,FVector(.65,.4,.25),4);}
   for(int32 F=Shop>=0?2:1;F<Floors;++F){const float Z=F*FloorH;for(float A:{-130.f,130.f}){Face(A,Depth/2+8,Z+185,FVector(.9,.13,1.65),1);if(Style==2){Face(A-54,Depth/2+12,Z+185,FVector(.075,.14,1.85),4);Face(A+54,Depth/2+12,Z+185,FVector(.075,.14,1.85),4);}}
    Face(0,Depth/2+12,Z+25,FVector(Width/100,.22,.13),Style==2?4:10);
    if(Style==0||Style==1){Face(0,Depth/2+36,Z+55,FVector(3.7,.65,.13),10);Face(0,Depth/2+67,Z+140,FVector(3.7,.055,.055),19);for(float A:{-170.f,-85.f,0.f,85.f,170.f})Face(A,Depth/2+67,Z+100,FVector(.04,.04,.85),19);if(Style==1&&F%2==0)for(float A:{-100.f,50.f})Face(A,Depth/2+55,Z+190,FVector(.4,.03,.55),A<0?7:8);}
    if(Rand.FRand()<.45f)Face(160,Depth/2+32,Z+105,FVector(.65,.6,.5),2);
   }
   // Vertical rainwater pipes and restrained projecting neon shop signs.
   for(float A:{-220.f,220.f})Face(A,Depth/2+22,H/2,FVector(.055,.055,H/100),2);
   if(Shop<0&&(Style==2||Rand.FRand()<.5f))Face(Style==2?165:0,Depth/2+52,850,FVector(1.5,.25,3.9),11+Rand.RandRange(0,5));
  }
 };

 if(Layer==EKTWLayer::Facades&&bFacadesEnabled){
  // Straight street fronts avoid rotating and overlapping buildings around spline corners.
  for(int Side=0;Side<4;++Side)for(int Slot=0;Slot<19;++Slot){
   const float Along=-6480+Slot*720;
   const FVector P=Side==0?FVector(Along,-7300,0):Side==1?FVector(7300,Along,0):Side==2?FVector(-Along,7300,0):FVector(-7300,-Along,0);
   Building(P,Side*90.f,Rand.FRandRange(470,590),Rand.FRandRange(440,540),false);
  }
  // Removing selected complete lots opens wider, connected visual side alleys.
  for(FVector P:BuildingLots())if(!IsScaffoldLot(P)){
   if(!AlleyLot(P)){Building(P,0,370,370,true);continue;}
   R.Add(Place(P+FVector(0,0,11),FVector(4.95,4.95,.025),37));
   // Drain grilles, service doors, crates and refuse bins give the gaps a purpose.
   for(float T:{-170.f,-85.f,0.f,85.f,170.f})R.Add(Place(P+FVector(T,-180,14),FVector(.045,.65,.02),42));
   if(Rand.FRand()<.28f){
    R.Add(Place(P+FVector(170,140,48),FVector(.55,.55,.7),40));
    R.Add(Place(P+FVector(170,140,86),FVector(.62,.62,.06),42));
    R.Add(Place(P+FVector(100,160,33),FVector(.45,.5,.4),8));
   }
  }
  auto City=[&](FVector P,FVector Scale,int Kind,float Yaw=0){R.Add(Place(P,Scale,Kind,-1,Yaw));};
  // A hierarchy of perimeter streets with paving, road markings and parked traffic.
  for(int Ring=0;Ring<4;++Ring){const float Radius=8400+Ring*2700;
   for(int Side=0;Side<4;++Side){const FRotator Rot(0,Side*90.f,0);auto Street=[&](float Along,float Out,float Z){return Rot.RotateVector(FVector(Along,-Radius+Out,Z));};
    City(Street(0,0,3),FVector((Radius*2-900)/100,9,.025),39,Side*90);
    City(Rot.RotateVector(FVector(Radius,-Radius,3)),FVector(9,9,.025),39,Side*90);
    for(float Sign:{-1.f,1.f})City(Street(0,Sign*530,8),FVector((Radius*2-900)/100,1.6,.025),37,Side*90);
    for(float T=-Radius+1500;T<Radius-1200;T+=650)City(Street(T,0,6),FVector(2,.12,.025),38,Side*90);
    for(float End:{-Radius+1000,Radius-1000})for(float Across=-350;Across<=350;Across+=90)City(Street(End,Across,7),FVector(2.8,.4,.025),38,Side*90);
    for(float T=-Radius+1900;T<Radius-1500;T+=2400){
     City(Street(T,550,350),FVector(.1,.1,7),42);City(Street(T,490,705),FVector(.9,1.3,.15),43,Side*90);
     City(Street(T+200,550,80),FVector(.5,.5,1.6),40);
     if((FMath::RoundToInt(T)+Side+Ring)%3==0){
      City(Street(T,270,80),FVector(4.2,1.8,1),41,Side*90);City(Street(T-20,270,150),FVector(2.3,1.6,.65),1,Side*90);
      for(float X:{-140.f,140.f})for(float Y:{-90.f,90.f}){City(Street(T+X,270+Y,48),FVector(.6,.6,.2),42);R.Last().Transform.SetRotation(FRotator(0,Side*90,90).Quaternion());}
     }
    }
   }
  }
  for(int Side=0;Side<4;++Side){const FRotator Rot(0,Side*90.f,0);
   City(Rot.RotateVector(FVector(0,-12450,3)),FVector(9,81,.025),39,Side*90);
   for(float X:{-530.f,530.f})City(Rot.RotateVector(FVector(X,-12450,8)),FVector(1.6,81,.025),37,Side*90);
   for(float Y=-15800;Y<-8500;Y+=650)City(Rot.RotateVector(FVector(0,Y,6)),FVector(.12,2,.025),38,Side*90);
  }
  // All street reservations are established before placing background buildings.
  TArray<FBox2D> StreetReservations;
  for(const auto& V:R)if(V.Kind==39){const FVector Half=V.Transform.GetRotation().RotateVector(V.Transform.GetScale3D()*50).GetAbs()+FVector(310,310,0);const auto P=V.Transform.GetLocation();StreetReservations.Add(FBox2D(FVector2D(P.X-Half.X,P.Y-Half.Y),FVector2D(P.X+Half.X,P.Y+Half.Y)));}
  // Background blocks have podiums, roof plant, service pipes and varied facades.
  for(int Ring=0;Ring<3;++Ring){const float Radius=9750+Ring*2700;
   for(int Side=0;Side<4;++Side)for(float Along=-14500;Along<=14500;Along+=1450){
    if(FMath::Abs(Along)<1300||FMath::Abs(Along)>Radius-1750)continue; // Aligned blocks and clear arterial intersections.
    const float W=Rand.FRandRange(850,1150),D=Rand.FRandRange(700,950),H=Rand.FRandRange(2600,6200)+Ring*450;
    const FVector P=Side<2?FVector(Along,Side==0?-Radius:Radius,0):FVector(Side==2?-Radius:Radius,Along,0);
    const float Yaw=Side<2?0:90;const FRotator Rot(0,Yaw,0);const int Wall=31+Rand.RandRange(0,5),Family=Rand.RandRange(0,2);
    const FVector Half=Rot.RotateVector(FVector(W/2+80,D/2+80,0)).GetAbs();const FBox2D Lot(FVector2D(P.X-Half.X,P.Y-Half.Y),FVector2D(P.X+Half.X,P.Y+Half.Y));bool OverlapsStreet=false;for(const auto& Reserved:StreetReservations)if(Lot.Intersect(Reserved)){OverlapsStreet=true;break;}if(OverlapsStreet)continue;
    City(P+FVector(0,0,8),FVector((W+80)/100,(D+80)/100,.025),37,Yaw);
    if(FKTWImportedArt::Enabled()){City(P+FVector(0,0,H/2),FVector(W/100,D/100,H/100),100+Rand.RandRange(0,7),Yaw);continue;}
    City(P+FVector(0,0,210),FVector((W+70)/100,(D+70)/100,4.2),Wall,Yaw);
    City(P+FVector(0,0,H/2),FVector(W/100,D/100,H/100),Wall,Yaw);
    City(P+FVector(0,0,H+70),FVector((W+15)/100,(D+15)/100,1.4),Wall,Yaw);
    City(P+FVector(80,70,H+220),FVector(2.2,2.2,3),9);
    City(P+FVector(-100,-100,H+85),FVector(2,1.4,1.4),2);
    for(int Floor=1;Floor<H/360;++Floor)for(float Face:{-1.f,1.f}){
     for(float A:{-.3f,0.f,.3f})City(P+Rot.RotateVector(FVector(A*W,Face*(D/2+8),Floor*360+150)),FVector(Family==1?1.45:1.05,.12,1.55),1,Yaw);
     if(Family!=1){City(P+Rot.RotateVector(FVector(0,Face*(D/2+38),Floor*360+35)),FVector((W-70)/100,.65,.12),10,Yaw);City(P+Rot.RotateVector(FVector(0,Face*(D/2+70),Floor*360+125)),FVector((W-70)/100,.055,.055),19,Yaw);}
     if(Floor%3==0)City(P+Rot.RotateVector(FVector(W*.33,Face*(D/2+35),Floor*360+80)),FVector(.8,.65,.6),2,Yaw);
    }
    for(float Sign:{-1.f,1.f})for(int Floor=1;Floor<H/360;++Floor){
     for(float A:{-.28f,.28f})City(P+Rot.RotateVector(FVector(Sign*(W/2+8),A*D,Floor*360+150)),FVector(1.1,.12,1.55),1,Yaw+90);
     if(Floor%3==1)City(P+Rot.RotateVector(FVector(Sign*(W/2+30),0,Floor*360+85)),FVector(.8,.65,.6),2,Yaw+90);
    }
    for(float Face:{-1.f,1.f}){
     City(P+Rot.RotateVector(FVector(-W*.42,Face*(D/2+16),H/2)),FVector(.08,.08,H/100),2,Yaw);
     City(P+Rot.RotateVector(FVector(0,Face*(D/2+30),340)),FVector((W-80)/100,.1,1.1),11+Rand.RandRange(0,5),Yaw);
     for(float A:{-.3f,0.f,.3f})City(P+Rot.RotateVector(FVector(A*W,Face*(D/2+8),160)),FVector(1.65,.12,2.6),18,Yaw);
    }
   }
  }
  // Roadside market props and signal heads are rendered without collision or nav influence.
  FRandomStream Market(int32(HashCombineFast(uint32(S),65017)));
  auto Signal=[&](FVector P,float Yaw,int Direction){const FRotator Rot(0,Yaw,0);auto Part=[&](FVector Local,FVector Scale,int Kind){R.Add(Place(P+Rot.RotateVector(Local),Scale,Kind,Direction,Yaw));};
   Part(FVector(0,0,325),FVector(.16,.16,6.5),44);Part(FVector(0,0,15),FVector(.44,.44,.3),44);
   Part(FVector(0,0,665),FVector(.65,.5,1.7),45);
   for(int I=0;I<3;++I){const FVector L(0,29,715-I*50);Part(L,FVector(.34,.12,.34),46);Part(L+FVector(0,5,0),FVector(.29,.13,.29),48+I);}
  };
  for(int Side=0;Side<4;++Side){const FRotator Rot(0,Side*90.f,0);for(float Along:{-4600.f,4600.f})Signal(Rot.RotateVector(FVector(Along,-6630,0)),Side*90.f,Side%2);
   for(int Slot=0;Slot<2;++Slot){const float Choices[]={-3500,-1750,1750,3500};const float Along=Choices[Slot*2+Market.RandRange(0,1)];const FVector P=Rot.RotateVector(FVector(Along,-6750,0));auto Part=[&](FVector Local,FVector Scale,int Kind){City(P+Rot.RotateVector(Local),Scale,Kind,Side*90.f);};
    Part(FVector(0,0,70),FVector(2.1,1.1,1.3),51);Part(FVector(0,0,143),FVector(2.3,1.25,.14),56);
    for(float X:{-105.f,105.f})for(float Y:{-55.f,55.f})Part(FVector(X,Y,165),FVector(.065,.065,3.2),44);
    const int Fabric=(Slot+Side)%2?52:53;
    for(float Sign:{-1.f,1.f}){Part(FVector(0,Sign*35,322),FVector(2.5,.8,.08),Fabric);R.Last().Transform.SetRotation(FRotator(0,Side*90,Sign*14).Quaternion());}
    Part(FVector(0,66,270),FVector(1.5,.06,.55),57);
    for(int I=0;I<3;++I){Part(FVector(-70+I*70,0,157),FVector(.58,.8,.16),56);for(float X:{-14.f,14.f})Part(FVector(-70+I*70+X,15,173),FVector(.22,.22,.22),I%2?54:55);}
    Part(FVector(145,-25,25),FVector(.5,.65,.5),56);
   }
  }
  for(int Side=0;Side<4;++Side){const float A=Side*90.f;Signal(FRotator(0,A,0).RotateVector(FVector(1250,0,0)),A+90,Side%2);}
  // Pavement dressing stays outside the full-width road corridors and base corners.
  FRandomStream Dress(int32(HashCombineFast(uint32(S),41317)));
  for(int Side=0;Side<4;++Side)for(int Slot=0;Slot<6;++Slot){
   const float Along=-4200+Slot*1680+Dress.FRandRange(-120,120);
   const FVector P=Side<2?FVector(Along,Side==0?-6940:6940,0):FVector(Side==2?-6940:6940,Along,0);
   const float Yaw=Side<2?0:90;const FRotator Rot(0,Yaw,0);
   auto Part=[&](FVector Local,FVector Scale,int Kind){R.Add(Place(P+Rot.RotateVector(Local),Scale,Kind,-1,Yaw));};
   Part(FVector(0,0,35),FVector(.75,.75,.7),21);
   Part(FVector(0,0,850),FVector(.16,.16,16.5),21);
   Part(FVector(0,0,1710),FVector(1.8,1.25,.24),21);
   Part(FVector(0,0,1685),FVector(1.4,.85,.22),22);
   if((Slot+Side)%2==0){
    const float Shift=Dress.FRandRange(390,460);
    // Narrow benches face the road; model pieces never extend into the traffic lanes.
    for(float X:{-230.f,230.f})Part(FVector(Shift+X,0,140),FVector(.22,1.1,2.8),21);
    for(float Y:{-45.f,0.f,45.f})Part(FVector(Shift,Y,285),FVector(6.8,.34,.22),23);
    const float Back=Side==0||Side==3?-55:55;
    for(float Z:{400.f,510.f})Part(FVector(Shift,Back,Z),FVector(6.8,.18,.7),23);
    for(float X:{-290.f,290.f})Part(FVector(Shift+X,Back,400),FVector(.12,.14,2.5),21);
   }
  }

 }else if(Layer==EKTWLayer::Barriers&&bBarriersEnabled){
  for(FVector P:BuildingLots())if(IsScaffoldLot(P)){
   R.Add(Place(P+FVector(0,0,360),FVector(Rand.FRandRange(3.6f,4.2f),3.5,Rand.FRandRange(2.4f,3.5f)),0));
   for(float X:{-220.f,220.f})for(float Y:{-220.f,220.f})R.Add(Place(P+FVector(X,Y,650+Rand.FRandRange(-30,30)),FVector(.15,.15,Rand.FRandRange(8.4f,9.2f)),1));
   for(float Z:{350.f,650.f,950.f}){for(float Y:{-220.f,220.f})R.Add(Place(P+FVector(0,Y,Z),FVector(4.5,.15,.15),1));for(float X:{-220.f,220.f})R.Add(Place(P+FVector(X,0,Z),FVector(.15,4.5,.15),1));}
  }
 }else if(Layer==EKTWLayer::Weapons&&bWeaponsEnabled){
  for(int32 L=0;L<3;++L){auto RoutePoints=Route(L);float Roll=Rand.FRand();int32 Kind=Roll<.35f?0:(Roll<.7f?1:2);float T=Rand.FRandRange(.28f,.62f);
   FVector P=FMath::Lerp(RoutePoints[0],RoutePoints[1],T);FVector D=(RoutePoints[1]-RoutePoints[0]).GetSafeNormal();P+=FVector(-D.Y,D.X,0)*Rand.FRandRange(-120,120);
   for(int32 I=0;I<(WeaponsPerLane>0?2:0);++I){FVector Q=I==0?P:FVector(P.Y,P.X,0);const FVector HeldSize=Kind==0?FVector(.45f,.035f,.025f):(Kind==1?FVector(.8f,.06f,.06f):FVector(.12f,.12f,.2f));const FVector Scale=HeldSize*StreetWeaponVisualScale;Q.Z=Scale.Z*50+5;R.Add(Place(Q,Scale,Kind,L,D.Rotation().Yaw));}
  }
 }else if(Layer==EKTWLayer::GoldSources){
  const float Outer=Rand.FRandRange(3200,3900),Inner=Rand.FRandRange(650,1050),Middle=Rand.FRandRange(1850,2350);
  for(float T:{-Outer,Outer,-Inner,Inner,-Middle,Middle})R.Add(Place(FVector(T,T,20),FVector::OneVector,0,-1));
 }else if(Layer==EKTWLayer::Atmosphere){R.Add(Place(FVector::ZeroVector,FVector::OneVector,Rand.RandRange(0,4),-1));
 }
 return R;
}
bool AKTWStreetDirector::ValidateLayout(){
 bool OK=GetActorScale3D().Equals(FVector::OneVector)&&GetActorRotation().IsNearlyZero()&&GetActorLocation().IsNearlyZero();
 for(FVector P:BuildingLots())OK&=!IsRoadPosition(P,355);
 for(auto V:BuildLayer(EKTWLayer::Barriers))OK&=!IsRoadPosition(V.Transform.GetLocation(),80);
 for(auto V:BuildLayer(EKTWLayer::Weapons))OK&=IsRoadPosition(V.Transform.GetLocation(),-140)&&V.Lane>=0&&V.Lane<3;
 for(auto V:BuildLayer(EKTWLayer::GoldSources))OK&=IsRoadPosition(V.Transform.GetLocation(),-180)&&V.Transform.GetLocation().Size2D()<6000;
 ValidationReport=OK?TEXT("V4 PASS: 3 routes + open central link; district-only buildings"):TEXT("V4 FAIL: clearance or transform");return OK;
}
