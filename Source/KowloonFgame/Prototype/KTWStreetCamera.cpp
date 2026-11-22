#include "KTWPrototype.h"
#include "KTWGeneration.h"
#include "KTWSession.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"

float AKTWCameraPawn::StreetEyeHeight(FVector Point) const
{
 float HeadHeight=1150,Nearest=3500;
 for(TActorIterator<ACharacter> It(GetWorld());It;++It){
  auto* U=Cast<AKTWTestUnit>(*It);auto* B=Cast<AKTWBoss>(*It);
  if((!U&&!B)||It->IsHidden()||(U&&U->Health<=0)||(B&&B->bRecovering))continue;
  const float Distance=FVector::Dist2D(Point,It->GetActorLocation());
  if(Distance>=Nearest||!It->GetMesh()->DoesSocketExist(TEXT("head")))continue;
  Nearest=Distance;
  const float RawHeadHeight=float(It->GetMesh()->GetSocketLocation(TEXT("head")).Z-It->GetActorLocation().Z+It->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
  HeadHeight=RawHeadHeight>400?FMath::Clamp(RawHeadHeight,400.f,1800.f):1150.f;
 }
 return HeadHeight+160;
}

bool AKTWCameraPawn::FindStreetLanding(FVector Requested,FVector& Eye,float& Yaw) const
{
 Requested.Z=0;
 const auto Nodes=AKTWStreetDirector::Junctions();
 const FIntPoint Edges[]={{0,1},{1,4},{0,2},{2,4},{0,3},{3,4},{1,2},{2,3}};
 float Best=MAX_flt;bool Found=false;
 auto Try=[&](FVector Road,FVector Along){
  Road.Z=0;if(!AKTWStreetDirector::IsRoadPosition(Road,-120))return;
  FCollisionQueryParams Query;Query.AddIgnoredActor(this);FHitResult Floor;
  // Trace only the road slab: a building roof must never become the landing floor.
  if(!GetWorld()->LineTraceSingleByObjectType(Floor,Road+FVector(0,0,100),Road-FVector(0,0,200),FCollisionObjectQueryParams(ECC_WorldStatic),Query))return;
  const FVector Candidate=Floor.ImpactPoint+FVector(0,0,StreetEyeHeight(Road));
  FHitResult Obstacle;
  if(GetWorld()->SweepSingleByObjectType(Obstacle,Floor.ImpactPoint+FVector(0,0,90),Candidate,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(65),Query))return;
  if(GetWorld()->OverlapAnyTestByObjectType(Candidate,FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(70),Query))return;
  const float Score=FVector::DistSquared2D(Requested,Road);if(Score>=Best)return;
  Best=Score;Eye=Candidate;Found=true;
  if(FVector::DotProduct(Along,-Road)<0)Along=-Along;
  Yaw=Along.Rotation().Yaw;
 };
 float NearestEdge=MAX_flt;FVector StreetAlong=FVector(1,0,0);
 for(const auto Edge:Edges){const FVector A=Nodes[Edge.X],B=Nodes[Edge.Y];const float Distance=FVector::DistSquared2D(Requested,FMath::ClosestPointOnSegment(Requested,A,B));if(Distance<NearestEdge){NearestEdge=Distance;StreetAlong=(B-A).GetSafeNormal2D();}}
 Try(Requested,StreetAlong);
 for(const auto Edge:Edges){
  const FVector A=Nodes[Edge.X],B=Nodes[Edge.Y],Along=(B-A).GetSafeNormal2D();
  const FVector Closest=FMath::ClosestPointOnSegment(Requested,A,B);
  // Preserve the mouse position on clear road; otherwise use nearby road center lines.

  for(float Offset:{0.f,-220.f,220.f,-500.f,500.f})Try(FMath::ClosestPointOnSegment(Closest+Along*Offset,A,B),Along);
 }
 return Found;
}

void AKTWCameraPawn::ApplyLookDelta(float X,float Y)
{
 OrbitYaw=FRotator::NormalizeAxis(OrbitYaw+X*.50f);
 OrbitPitch=FMath::Clamp(OrbitPitch+Y*.50f,-65.f,45.f);
}

void AKTWCameraPawn::SetLookCaptured(bool On)
{
 auto* PC=Cast<APlayerController>(GetController());if(!PC||On==bLookCaptured)return;
 bLookCaptured=On;
 if(On){
  float X=0,Y=0;PC->GetMousePosition(X,Y);LookRestore=FVector2D(X,Y);
  PC->bShowMouseCursor=false;
  FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Mode);
 }else{
  FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
  PC->bShowMouseCursor=true;PC->SetMouseLocation(FMath::RoundToInt(LookRestore.X),FMath::RoundToInt(LookRestore.Y));
 }
}

bool AKTWCameraPawn::UpdateLookGesture(float D)
{
 auto* PC=Cast<APlayerController>(GetController());if(!PC)return false;
 if(!bStreetView){SetLookCaptured(false);bRightPending=false;return false;}
 if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton)){
  float X=0,Y=0;int32 W=0,H=0;PC->GetViewportSize(W,H);
  if(PC->GetMousePosition(X,Y)&&Y>125&&X<W-280&&!(X<335&&Y<450)&&Y<H-85){bRightPending=true;bRightDragged=false;RightPressedAt=GetWorld()->GetTimeSeconds();RightPress=FVector2D(X,Y);}
 }
 if(bRightPending&&PC->IsInputKeyDown(EKeys::RightMouseButton)&&!bRightDragged){
  float X=0,Y=0;PC->GetMousePosition(X,Y);
  if(FVector2D::Distance(RightPress,FVector2D(X,Y))>6||GetWorld()->GetTimeSeconds()-RightPressedAt>.18){bRightDragged=true;SetLookCaptured(true);LookRestore=RightPress;}
 }
 const bool Middle=PC->IsInputKeyDown(EKeys::MiddleMouseButton);
 const bool Looking=Middle||(bRightPending&&bRightDragged&&PC->IsInputKeyDown(EKeys::RightMouseButton));
 SetLookCaptured(Looking);
 if(Looking){float X=0,Y=0;PC->GetInputMouseDelta(X,Y);ApplyLookDelta(X,Y);}
 if(PC->WasInputKeyJustReleased(EKeys::RightMouseButton)){
  const bool Click=bRightPending&&!bRightDragged;bRightPending=false;bRightDragged=false;return Click;
 }
 return false;
}
