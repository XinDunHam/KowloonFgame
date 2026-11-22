#include "KTWPrototype.h"
#include "KTWImportedArt.h"
#include "KTWDynamics.h"
#include "Engine/StaticMesh.h"
#include "KTWGold.h"
#include "KTWMatch.h"
#include "KTWGeneration.h"
#include "KTWCommander.h"
#include "KTWWorkGuard.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerInput.h"
#include "TimerManager.h"

void AKTWCameraPawn::RunContinuityTest(){
#if !UE_BUILD_SHIPPING
 auto Fail=MakeShared<int32>(0);auto Check=[Fail](bool OK,const TCHAR* N){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW CONTINUITY %s: %s"),N,OK?TEXT("PASS"):TEXT("FAIL"));};auto* M=AKTWMatch::Find(GetWorld());auto* G=AKTWGoldSystem::Find(GetWorld());auto* PC=Cast<AKTWPrototypeController>(GetController());Check(M&&G&&PC&&G->Piles.Num()==6,TEXT("real map and production systems ready"));if(!M||!G||!PC||G->Piles.Num()!=6){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 for(TActorIterator<AKTWCommander>I(GetWorld());I;++I)I->SetActorTickEnabled(false);for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();M->StartBattle();M->Tick(20);G->Tick(0);M->SetActorTickEnabled(false);G->SetActorTickEnabled(false);
 auto Spawn=[this](EKTWRole UnitType,FVector P){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));U->UnitRole=UnitType;U->bEnableCombat=false;U->bAutoPatrol=false;U->Order=EKTWUnitOrder::Defend;U->FinishSpawning(FTransform(P));return U;};
 auto* Runner=Spawn(EKTWRole::Runner,G->Piles[2]->GetActorLocation()+FVector(-250,-250,100));Check(Runner->IssueOrder(EKTWUnitOrder::Move,G->Piles[2]->GetActorLocation()),TEXT("courier accepts move to gold rather than a pickup-specific command"));auto* Fighter=Spawn(EKTWRole::Knife,G->Piles[4]->GetActorLocation()+FVector(-230,-230,100));Check(Fighter->IssueOrder(EKTWUnitOrder::Move,G->Piles[4]->GetActorLocation()),TEXT("fighter accepts move to local work area"));
 auto* Hold=Spawn(EKTWRole::Pipe,FVector(-6000,500,100));Hold->IssueOrder(EKTWUnitOrder::Defend,Hold->GetActorLocation());auto* Stalled=Spawn(EKTWRole::Knife,FVector(-6000,3500,100));Stalled->IssueOrder(EKTWUnitOrder::Move,FVector(-6000,1500,20));Stalled->GetCharacterMovement()->MaxWalkSpeed=0;
 TArray<TWeakObjectPtr<AKTWBoss>> Bosses;Bosses.SetNum(2);for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){Bosses[I->Faction]=*I;I->SetActorLocation(FVector(I->Faction?160:-160,I->Faction?-160:160,100));I->IssueTask(EKTWBossTask::Sit,FVector::ZeroVector,nullptr);I->bHoldPosition=true;}
 Check(Bosses[0].IsValid()&&Bosses[1].IsValid(),TEXT("both faction bosses participate in duel"));
 SetStreetView(false,false);SetActorLocation(FVector(0,-800,26000));Camera->SetRelativeRotation(FRotator(-90,-90,0));PC->PlayerCameraManager->UpdateCamera(.016f);const FVector PointerPoint(1800,1800,20);FVector2D Screen;FVector MouseGround;int32 VX=0,VY=0;PC->GetViewportSize(VX,VY);UE_LOG(LogTemp,Display,TEXT("KTW VIEWPORT width=%d height=%d local=%d fov=%.2f"),VX,VY,PC->GetLocalPlayer()!=nullptr,PC->PlayerCameraManager->GetFOVAngle());const bool Projection=PC->ProjectWorldLocationToScreen(PointerPoint,Screen)&&CursorWorldAt(Screen,MouseGround);UE_LOG(LogTemp,Display,TEXT("KTW POINTER projection=%d screen=%s output=%s actual=%s eye=%s rotation=%s"),Projection,*Screen.ToString(),*MouseGround.ToString(),*PointerPoint.ToString(),*PC->PlayerCameraManager->GetCameraLocation().ToString(),*PC->PlayerCameraManager->GetCameraRotation().ToString());if(VX>0&&VY>0)Check(Projection&&FVector::Dist2D(PointerPoint,MouseGround)<45,TEXT("rendered cursor round trip stays within one ground pixel"));else UE_LOG(LogTemp,Display,TEXT("KTW CONTINUITY cursor projection requires rendered viewport; separate RHI run verifies it"));EnterStreetAt(PointerPoint);Check(AKTWStreetDirector::IsRoadPosition(FreePosition,-120)&&!FollowTarget.IsValid(),TEXT("building pointer snaps to nearby road without selected troop snapping"));
 Check(!bStreetTransition&&GetActorLocation().Equals(FreePosition,1)&&FreePosition.Z<2000,TEXT("entry immediately reaches head height without rooftop cruise"));
 bool Landings=true;int Tested=0;for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)for(const FVector Lot:I->BuildingLots()){FVector Eye;float Yaw=0;const bool OK=FindStreetLanding(Lot,Eye,Yaw);Landings&=OK&&AKTWStreetDirector::IsRoadPosition(Eye,-120)&&Eye.Z<2000; ++Tested;}
 Check(Tested>10&&Landings,TEXT("all building lot pointers resolve to clear road at head height"));
 const float BeforeYaw=OrbitYaw;ApplyLookDelta(80,10000);Check(FMath::Abs(FRotator::NormalizeAxis(OrbitYaw-BeforeYaw)-40.f)<.01&&OrbitPitch==45,TEXT("look input uses direct bounded yaw and pitch"));ApplyLookDelta(0,-10000);Check(OrbitPitch==-65,TEXT("look down clamp avoids disorienting flips"));OrbitPitch=-12;
 if(VX>1000&&VY>700&&PC->PlayerInput){
  auto Step=[this,PC](FKey Key,EInputEvent Event){PC->PlayerInput->InputKey(FInputKeyParams(Key,Event,Event==IE_Released?0.:1.));TArray<UInputComponent*> Empty;PC->PlayerInput->ProcessInputStack(Empty,.016f,false);return UpdateLookGesture(.016f);};
  PC->PlayerInput->FlushPressedKeys();PC->SetMouseLocation(800,550);
  Check(!Step(EKeys::RightMouseButton,IE_Pressed)&&bRightPending&&!bLookCaptured,TEXT("right press defers command until click or drag is known"));
  Check(Step(EKeys::RightMouseButton,IE_Released)&&!bLookCaptured&&PC->bShowMouseCursor,TEXT("short right click emits command and retains visible pointer"));
  PC->SetMouseLocation(800,550);Step(EKeys::RightMouseButton,IE_Pressed);RightPressedAt-=.3;
  TArray<UInputComponent*> Empty;PC->PlayerInput->ProcessInputStack(Empty,.016f,false);UpdateLookGesture(.016f);
  Check(bLookCaptured&&!PC->bShowMouseCursor,TEXT("right hold captures pointer for unbounded rotation"));
  Check(!Step(EKeys::RightMouseButton,IE_Released)&&!bLookCaptured&&PC->bShowMouseCursor,TEXT("right drag release restores pointer without issuing order"));
  Step(EKeys::MiddleMouseButton,IE_Pressed);Check(bLookCaptured,TEXT("middle look remains compatible"));Step(EKeys::MiddleMouseButton,IE_Released);
  SetLookCaptured(true);SetStreetView(false,false);Check(!bLookCaptured&&PC->bShowMouseCursor,TEXT("leaving street always releases look capture"));EnterStreetAt(PointerPoint);
  PC->PlayerInput->FlushPressedKeys();
 }
 SetActorLocation(FreePosition);const FVector Before=FreePosition;TranslateFree(FVector::UpVector,600);Check(FreePosition.Z>Before.Z+500,TEXT("free view independently rises above street"));SetActorLocation(FreePosition);TranslateFree(FVector(1,1,0).GetSafeNormal(),350);Check(FVector::Dist2D(Before,FreePosition)>300,TEXT("free view translates without orbiting an anchor"));

 // Exercise production key polling and physical view motion, not only configured constants.
 if(PC->PlayerInput){
  auto Key=[PC](FKey K,EInputEvent E){PC->PlayerInput->InputKey(FInputKeyParams(K,E,E==IE_Released?0.:1.));TArray<UInputComponent*> Stack;PC->PlayerInput->ProcessInputStack(Stack,.016f,false);};
  EnterStreetAt(FVector::ZeroVector);SetActorLocation(FVector(0,0,1800));FreePosition=GetActorLocation();OrbitYaw=0;OrbitPitch=30;
  const FVector Start=FreePosition;Key(EKeys::W,IE_Pressed);Tick(.1f);const float Normal=FVector::Dist(Start,FreePosition);
  Check(FreePosition.Z>Start.Z+150&&FVector::Dist2D(Start,FreePosition)>250,TEXT("W follows upward view and moves faster through real input polling"));
  SetActorLocation(Start);FreePosition=Start;Key(EKeys::LeftShift,IE_Pressed);Tick(.1f);
  Check(FVector::Dist(Start,FreePosition)>Normal*1.8f,TEXT("shift accelerates actual free flight"));Key(EKeys::W,IE_Released);Key(EKeys::LeftShift,IE_Released);
 }
 if(PC->PlayerInput){
  auto Key=[PC](FKey K,EInputEvent E){PC->PlayerInput->InputKey(FInputKeyParams(K,E,E==IE_Released?0.:1.));TArray<UInputComponent*> Stack;PC->PlayerInput->ProcessInputStack(Stack,.016f,false);};
  EnterStreetAt(FVector::ZeroVector);FreePosition=FVector(0,0,1800);SetActorLocation(FreePosition);OrbitPitch=0;OrbitYaw=45;
  const FVector Start=FreePosition;Key(EKeys::MouseScrollUp,IE_Pressed);Tick(.016f);
  Check(FVector::Dist(Start,FreePosition)>1400,TEXT("wheel input moves the street target by fifteen metres without frame-time scaling"));Key(EKeys::MouseScrollUp,IE_Released);
  SetLookCaptured(true);FollowTarget=Runner;bHasCursorAnchor=true;bRightPending=true;
  Key(EKeys::F2,IE_Pressed);Tick(.016f);
  Check(!bStreetView&&GetActorLocation().Equals(FVector(0,0,26000),1)&&FocusGoal.IsNearlyZero()&&TopHeight==26000,TEXT("F2 input instantly restores the original centered full map"));
  Check(!bLookCaptured&&!bRightPending&&!FollowTarget.IsValid()&&!bHasCursorAnchor&&PC->bShowMouseCursor,TEXT("F2 clears follow drag capture and stale cursor state"));
  Check(Camera->GetRelativeRotation().Equals(FRotator(-90,-90,0),.1)&&Camera->FieldOfView==65,TEXT("F2 restores overhead rotation and field of view in the same frame"));Key(EKeys::F2,IE_Released);
  Key(EKeys::MouseScrollUp,IE_Pressed);Tick(.016f);
  Check(TopHeight<21000&&TopHeight>19000,TEXT("overhead wheel uses responsive proportional zoom"));Key(EKeys::MouseScrollUp,IE_Released);
  EnterStreetAt(FVector::ZeroVector);SetActorLocation(FVector(0,0,1800));FreePosition=GetActorLocation();OrbitPitch=0;OrbitYaw=45;
  const FVector MultiStart=FreePosition;PC->PlayerInput->InputKey(FInputKeyParams(EKeys::MouseWheelAxis,2.0,.016f,1));TArray<UInputComponent*> Stack;PC->PlayerInput->ProcessInputStack(Stack,.016f,false);Tick(.016f);
  Check(FMath::Abs(FVector::Dist(MultiStart,FreePosition)-3000)<2,TEXT("analog wheel preserves multiple notches in one frame"));
  PC->PlayerInput->InputKey(FInputKeyParams(EKeys::MouseWheelAxis,0.0,.016f,1));PC->PlayerInput->ProcessInputStack(Stack,.016f,false);

 }
 EnterStreetAt(FVector(600,-600,0));OrbitPitch=25;SetActorLocation(FVector(600,-600,4000));FreePosition=FVector(600,-600,4300);Tick(.1f);
 Check(bStreetView,TEXT("upward flight below threshold remains street view"));
 SetActorLocation(FVector(600,-600,6400));FreePosition=FVector(600,-600,7100);OrbitPitch=-20;Tick(.1f);
 Check(bStreetView,TEXT("looking down above threshold does not auto switch"));
 SetActorLocation(FVector(600,-600,7000));FreePosition=GetActorLocation();OrbitPitch=25;Tick(.1f);
 Check(bStreetView,TEXT("stationary high camera does not auto switch"));
 SetActorLocation(FVector(600,-600,6400));FreePosition=FVector(600,-600,7100);OrbitPitch=25;SetLookCaptured(true);Tick(.1f);
 Check(!bStreetView&&GetActorLocation().Z>=6500,TEXT("actual upward travel past threshold restores tactical view"));
 Check(FVector::Dist2D(FocusGoal,FVector(600,-600,0))<1&&FMath::Abs(TopHeight-GetActorLocation().Z)<1,TEXT("automatic tactical entry preserves current horizontal position and height"));
 Check(!bLookCaptured&&PC->bShowMouseCursor&&!bHasCursorAnchor,TEXT("automatic tactical entry releases pointer and stale street anchor"));
 Tick(.016f);Check(!bStreetView,TEXT("automatic mode switch does not bounce back to street"));
 EnterStreetAt(PointerPoint);
 for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I){bool Styles[4]={};bool Tall=true;for(int Seed=1;Seed<=8;++Seed){for(const auto& P:I->BuildLayer(EKTWLayer::Facades,Seed)){if(P.BuildingStyleId>=0&&P.BuildingStyleId<4)Styles[P.BuildingStyleId]=true;if(P.Kind==4&&FMath::IsNearlyEqual(P.Transform.GetScale3D().Z,.36, .001))Tall&=P.Transform.GetLocation().Z>=2160;}}bool All=true;for(bool S:Styles)All&=S;Check(All,TEXT("seeded PCG generates four distinct Tong Lau styles"));Check(Tall,TEXT("roof heights reflect six to eleven full floors"));}

 for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I){
  bool Separated=true,Backdrop=true,Varied=true;
  for(int Seed=1;Seed<=12;++Seed){const auto Layout=I->BuildLayer(EKTWLayer::Facades,Seed);TArray<FBox2D> Bounds;bool Materials[6]={};int Distant=0;
   for(const auto& V:Layout)if((V.Kind>=31&&V.Kind<=36)||(V.Kind>=100&&V.Kind<=107)){Materials[(V.Kind>=100?V.Kind-100:V.Kind-31)%6]=true;const auto P=V.Transform.GetLocation(),Scale=V.Transform.GetScale3D();
    if(FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y))>8500&&Scale.Z>10)++Distant;
    if(V.BuildingStyleId>=0&&FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y))>7000&&Scale.Z>10){const FVector Half=V.Transform.Rotator().RotateVector(Scale*50).GetAbs();FBox2D Box(FVector2D(P.X-Half.X,P.Y-Half.Y),FVector2D(P.X+Half.X,P.Y+Half.Y));for(const auto& Other:Bounds)if(!Other.GetCenter().Equals(Box.GetCenter(),1)&&Other.Intersect(Box))Separated=false;Bounds.Add(Box);}
   }
   Backdrop&=Distant>=100;for(bool Available:Materials)Varied&=Available;
  }
  if(FKTWImportedArt::Enabled()){auto* Art=FKTWImportedArt::Data();int Houses=0,Groups=0;bool OriginalMaterials=true;for(TActorIterator<AActor>A(GetWorld());A;++A)if(A->ActorHasTag(TEXT("KTW_ImportedFacades"))){TArray<UInstancedStaticMeshComponent*> Cs;A->GetComponents(Cs);for(auto* C:Cs)if(C->GetFName()!=TEXT("SimpleBuildingCollision")) {++Groups;OriginalMaterials&=C->GetStaticMesh()!=nullptr&&C->OverrideMaterials.IsEmpty();if(C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName().Contains(TEXT("/Buildings/SM_Buildings")))Houses+=C->GetInstanceCount();}}
   Check(Art&&Art->Meshes.Num()>=30&&Houses>100,TEXT("curated imported models actually render more than one hundred buildings"));
   Check(OriginalMaterials&&Groups<40,TEXT("imported art preserves authored materials and batches into fewer than forty mesh groups"));
  }
  bool ClearStreets=true;for(int Seed=1;Seed<=12;++Seed){const auto Layout=I->BuildLayer(EKTWLayer::Facades,Seed);TArray<FBox2D> Roads;for(const auto& V:Layout)if(V.Kind==39){const auto P=V.Transform.GetLocation(),H=V.Transform.GetRotation().RotateVector(V.Transform.GetScale3D()*50).GetAbs()+FVector(290,290,0);Roads.Add(FBox2D(FVector2D(P.X-H.X,P.Y-H.Y),FVector2D(P.X+H.X,P.Y+H.Y)));}for(const auto& V:Layout)if((V.Kind>=100&&V.Kind<=107)&&FMath::Max(FMath::Abs(V.Transform.GetLocation().X),FMath::Abs(V.Transform.GetLocation().Y))>8500){const auto P=V.Transform.GetLocation(),H=V.Transform.GetRotation().RotateVector(V.Transform.GetScale3D()*50).GetAbs();const FBox2D B(FVector2D(P.X-H.X,P.Y-H.Y),FVector2D(P.X+H.X,P.Y+H.Y));for(const auto& Road:Roads)ClearStreets&=!B.Intersect(Road);}}
  Check(ClearStreets,TEXT("background buildings keep all ring roads sidewalks and connector junctions clear on twelve seeds"));
  int LampCount=0,GlowCount=0;bool CorrectLampState=true;auto* Weather=AKTWWeather::Find(GetWorld());const bool Dark=Weather&&(Weather->Preset==1||Weather->Preset==3),Rain=Weather&&Weather->Preset==2;for(TActorIterator<AActor>A(GetWorld());A;++A)if(A->ActorHasTag(TEXT("KTW_PavementLighting"))){TArray<UPointLightComponent*> Lights;A->GetComponents(Lights);for(auto* L:Lights)if(L->ComponentHasTag(TEXT("KTW_StreetLamp"))){++LampCount;CorrectLampState&=FMath::IsNearlyEqual(L->Intensity,Dark?24000.f:Rain?11000.f:0.f)&&L->IsVisible()==(Dark||Rain)&&!L->CastShadows;}TArray<UInstancedStaticMeshComponent*> Cs;A->GetComponents(Cs);for(auto* C:Cs)if(C->GetFName()==TEXT("StreetLampGlow")){GlowCount+=C->GetInstanceCount();CorrectLampState&=C->IsVisible()==(Dark||Rain);}}
  Check(LampCount==24&&GlowCount==24&&CorrectLampState,TEXT("twenty four warm street lamps and glowing bulbs match weather without shadow cost"));
  bool PavingVisible=true;int PavingGroups=0;for(TActorIterator<AActor>A(GetWorld());A;++A){TArray<UInstancedStaticMeshComponent*> Cs;A->GetComponents(Cs);for(auto* C:Cs)if(C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName().Contains(TEXT("SM_Tile_005"))){++PavingGroups;PavingVisible&=C->InstanceEndCullDistance==0;}}Check(!FKTWImportedArt::Enabled()||(PavingGroups>=2&&PavingVisible),TEXT("both core and imported sidewalks remain visible at maximum tactical camera distance"));
  Check(Separated,TEXT("urban exterior footprints remain separated on twelve seeds including corners"));
  Check(Backdrop,TEXT("three background city rings create at least one hundred distant buildings per seed"));
  Check(Varied,TEXT("all six wall or imported building variants are used on every tested seed"));
  int Alleys=0,Streets=0,Parked=0;for(const auto& V:I->BuildLayer(EKTWLayer::Facades,12345)){const auto P=V.Transform.GetLocation();if(V.Kind==37&&FMath::Abs(P.X)<6200&&FMath::Abs(P.Y)<6200)++Alleys;if(V.Kind==39)++Streets;if(V.Kind==41)++Parked;}
  Check(Alleys>50,TEXT("decorative side alleys replace at least fifty interior building lots"));
  Check(Streets==36&&Parked>0,TEXT("outer city has connected street rings intersection pads and parked vehicles"));
  bool MarketSafe=true,MarketCounts=true;for(int Seed=1;Seed<=12;++Seed){int Heads=0,Stalls=0;for(const auto& V:I->BuildLayer(EKTWLayer::Facades,Seed)){if(V.Kind==45)++Heads;if(V.Kind==51){++Stalls;const auto P=V.Transform.GetLocation();MarketSafe&=FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y))>=6700&&FMath::Min(FMath::Abs(P.X),FMath::Abs(P.Y))<4000;}}MarketCounts&=Heads==12&&Stalls==8;}
  Check(MarketCounts,TEXT("twelve signal heads and eight roadside stalls appear on all twelve seeds"));
  Check(MarketSafe,TEXT("market stalls stay at perimeter street edges away from central gold and base corners"));
  int Colored=0;bool Nonblocking=true;for(TActorIterator<AActor>A(GetWorld());A;++A)if(A->ActorHasTag(TEXT("KTW_PavementLighting"))){TArray<UInstancedStaticMeshComponent*> Cs;A->GetComponents(Cs);for(auto* C:Cs)if(C->GetName().StartsWith(TEXT("StreetDecoration"))){Nonblocking&=C->GetStaticMesh()!=nullptr&&C->GetMaterial(0)!=nullptr&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->CanEverAffectNavigation();if(C->GetName()==TEXT("StreetDecoration48")||C->GetName()==TEXT("StreetDecoration49")||C->GetName()==TEXT("StreetDecoration50"))for(int K=0;K<C->GetInstanceCount();++K){FTransform T;C->GetInstanceTransform(K,T);if(T.GetScale3D().X>.1)++Colored;}}}
  Check(Nonblocking&&Colored==12,TEXT("street props never block navigation and exactly one colored lens is active per signal"));
  bool Clear=true,Seeded=true,Shops=true,Counts=true;FVector Previous=FVector::ZeroVector;bool Changed=false;
  for(int Seed=1;Seed<=12;++Seed){const auto A=I->BuildLayer(EKTWLayer::Facades,Seed),B=I->BuildLayer(EKTWLayer::Facades,Seed);Seeded&=A.Num()==B.Num();int Lamps=0,Boards=0;bool Types[4]={};FVector First=FVector::ZeroVector;
   for(int Index=0;Index<A.Num();++Index){const auto& V=A[Index];if(B.IsValidIndex(Index))Seeded&=V.Kind==B[Index].Kind&&V.Transform.Equals(B[Index].Transform);
    if(V.Kind==22){if(Lamps==0)First=V.Transform.GetLocation();++Lamps;}if(V.Kind==23)++Boards;
    if(V.Kind>=21&&V.Kind<=23){const FVector P=V.Transform.GetLocation(),Extent=V.Transform.GetScale3D()*50;const FRotator R=V.Transform.Rotator();for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})Clear&=!AKTWStreetDirector::IsRoadPosition(P+R.RotateVector(FVector(X*Extent.X,Y*Extent.Y,0)),80);}
    if(V.Kind==25)Types[0]=true;if(V.Kind==26)Types[1]=true;if(V.Kind==29)Types[2]=true;if(V.Kind==30)Types[3]=true;
   }for(bool T:Types)Shops&=T;Counts&=Lamps==24&&Boards>=36;if(Seed>1)Changed|=!First.Equals(Previous,1);Previous=First;
  }
  int ActiveLights=0,Holders=0;for(TActorIterator<AActor>A(GetWorld());A;++A)if(A->ActorHasTag(TEXT("KTW_PavementLighting"))){++Holders;TArray<UPointLightComponent*> Lights;A->GetComponents(Lights);for(auto* L:Lights)if(L->IsRegistered()&&L->ComponentHasTag(TEXT("KTW_StreetLamp")))++ActiveLights;}
  Check(Holders==1&&ActiveLights==24,TEXT("PCG creates one lighting owner with twenty four actual point lights"));
  Check(Clear,TEXT("street furniture bounds preserve complete road corridors and base clearances"));
  Check(Seeded&&Changed,TEXT("street decoration repeats for same seed and changes for new rounds"));
  Check(Shops,TEXT("all tested seeds include bar restaurant teahouse and pawn facades"));
  Check(Counts,TEXT("all tested seeds generate twenty four lamps and twelve benches"));
 }
 TWeakObjectPtr<AKTWTestUnit> WR=Runner,WF=Fighter,WH=Hold,WS=Stalled;const double ArrivalDeadline=GetWorld()->GetTimeSeconds()+12;auto First=MakeShared<FTimerHandle>();GetWorldTimerManager().SetTimer(*First,FTimerDelegate::CreateLambda([this,WR,WF,WH,WS,Bosses,Check,Fail,First,ArrivalDeadline](){
  auto* R=WR.IsValid()?WR->FindComponentByClass<UKTWGoldCarrier>():nullptr;auto* F=WF.IsValid()?WF->FindComponentByClass<UKTWGoldCarrier>():nullptr;if(GetWorld()->GetTimeSeconds()<ArrivalDeadline&&(!R||!F||!WR->bHasLocalWork||!WF->bHasLocalWork||R->Carried<=0||F->Carried<=0))return;
  if(WR.IsValid())UE_LOG(LogTemp,Display,TEXT("KTW ARRIVAL runner distance=%.1f goal=%s speed=%.1f velocity=%s recoveries=%d"),FVector::Dist2D(WR->GetActorLocation(),WR->CommandDestination()),*WR->CommandDestination().ToString(),WR->GetCharacterMovement()->MaxWalkSpeed,*WR->GetVelocity().ToString(),WR->FindComponentByClass<UKTWWorkGuard>()->Recoveries);
  if(WR.IsValid()&&WF.IsValid())UE_LOG(LogTemp,Display,TEXT("KTW WORK runner gold=%d order=%d local=%d task=%d pos=%s; fighter gold=%d order=%d local=%d task=%d pos=%s"),R?R->Carried:-1,int32(WR->Order),WR->bHasLocalWork,R?int32(R->Task):-1,*WR->GetActorLocation().ToString(),F?F->Carried:-1,int32(WF->Order),WF->bHasLocalWork,F?int32(F->Task):-1,*WF->GetActorLocation().ToString());Check(R&&R->Carried>0&&WR->bHasLocalWork&&WR->Order==EKTWUnitOrder::Advance,TEXT("arrived runner automatically collects real gold"));Check(F&&WF->bHasLocalWork&&F->Carried>0,TEXT("arrived fighter collects safe gold without an implicit escort lock"));Check(WH.IsValid()&&WH->Order==EKTWUnitOrder::Defend&&!WH->bHasLocalWork,TEXT("explicit hold is preserved instead of meaningless forced motion"));Check(Bosses[0].IsValid()&&Bosses[1].IsValid()&&Bosses[0]->Health<320&&Bosses[1]->Health<320,TEXT("boss versus boss resolves actual reciprocal damage"));Check(WS.IsValid()&&WS->FindComponentByClass<UKTWWorkGuard>()->Recoveries>0,TEXT("stalled path triggers watchdog and safe replanning"));if(WS.IsValid())WS->GetCharacterMovement()->MaxWalkSpeed=WS->Stats.Speed;
  if(Bosses[1].IsValid())Bosses[1]->Destroy();if(Bosses[0].IsValid()){auto* B=Bosses[0].Get();B->IssueTask(EKTWBossTask::Sit,B->GetActorLocation(),nullptr);B->bHoldPosition=false;}
  // The carrier must interrupt an actual load for nearby combat and resume its job afterward.
  TWeakObjectPtr<AKTWTestUnit> Threat;
  if(WF.IsValid()){
   WF->bEnableCombat=true;const FVector P=WF->GetActorLocation()+FVector(70,70,0);
   auto* E=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P));
   E->Faction=1;E->UnitRole=EKTWRole::Pipe;E->bEnableCombat=false;E->bAutoPatrol=false;E->Order=EKTWUnitOrder::Defend;E->FinishSpawning(FTransform(P));E->Stats.MaxHealth=2000;E->Health=2000;Threat=E;
  }
  FTimerHandle End;GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateLambda([this,WF,Threat,WS,Bosses,Check,Fail](){Check(WS.IsValid()&&FVector::Dist2D(WS->GetActorLocation(),FVector(-6000,3500,100))>250,TEXT("recovered unit physically resumes movement"));Check(Bosses[0].IsValid()&&(Bosses[0]->Task==EKTWBossTask::Follow||Bosses[0]->Task==EKTWBossTask::Escort||FVector::Dist2D(Bosses[0]->GetActorLocation(),FVector::ZeroVector)>200),TEXT("idle boss selects escort follow or gold protection work"));auto* G=AKTWGoldSystem::Find(GetWorld());Check(G&&G->GoldConserved(),TEXT("automatic retasking and recovery conserve gold"));Check(Threat.IsValid()&&Threat->Health<2000&&WF.IsValid()&&WF->FindComponentByClass<UKTWGoldCarrier>()->Carried>0,TEXT("fighter carrying real gold damages a nearby enemy instead of remaining cargo locked"));
   if(Threat.IsValid())Threat->Destroy();
   FTimerHandle Resume;GetWorldTimerManager().SetTimer(Resume,FTimerDelegate::CreateLambda([this,WF,Check,Fail](){
    auto* C=WF.IsValid()?WF->FindComponentByClass<UKTWGoldCarrier>():nullptr;
    Check(C&&C->ControlsMovement()&&C->Task!=EKTWGoldTask::Idle&&WF->bHasLocalWork&&WF->Order==EKTWUnitOrder::Advance,TEXT("after combat the carrier resumes gold work and retains the player mission area"));
    auto* G=AKTWGoldSystem::Find(GetWorld());Check(G&&G->GoldConserved(),TEXT("combat interruption and resumption preserve the gold ledger"));
    const double BankDeadline=GetWorld()->GetTimeSeconds()+30;auto Bank=MakeShared<FTimerHandle>();GetWorldTimerManager().SetTimer(*Bank,FTimerDelegate::CreateLambda([this,WF,Check,Fail,Bank,BankDeadline](){
     auto* C=WF.IsValid()?WF->FindComponentByClass<UKTWGoldCarrier>():nullptr;
     if(C&&C->DeliveredTotal<=0&&GetWorld()->GetTimeSeconds()<BankDeadline)return;
     // Clear only after consuming captures: clearing this timer destroys its delegate.
     if(WF.IsValid())UE_LOG(LogTemp,Display,TEXT("KTW BANK delivered=%d carried=%d task=%d order=%d local=%d pos=%s"),C?C->DeliveredTotal:-1,C?C->Carried:-1,C?int32(C->Task):-1,int32(WF->Order),WF->bHasLocalWork,*WF->GetActorLocation().ToString());
     Check(C&&C->DeliveredTotal>0&&WF->bHasLocalWork&&WF->Order==EKTWUnitOrder::Advance,TEXT("fighter actually banks collected gold after combat and keeps its local mission"));
     auto* G=AKTWGoldSystem::Find(GetWorld());Check(G&&G->GoldConserved(),TEXT("banking and local mission return conserve gold"));
     const int32 Failures=*Fail;FTimerHandle CompletedTimer=*Bank;
     UE_LOG(LogTemp,Display,TEXT("KTW CONTINUITY COMPLETE failures=%d"),Failures);
     GetWorldTimerManager().ClearTimer(CompletedTimer);
     FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
    }),.25f,true);
   }),1.5f,false);
  }),4.f,false);
  FTimerHandle CompletedFirst=*First;GetWorldTimerManager().ClearTimer(CompletedFirst);
 }),.25f,true,4.5f);
#endif
}
