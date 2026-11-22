#include "KTWRogue.h"
#include "KTWExactUI.h"
#include "KTWReferenceUI.h"
#include "KTWPrototype.h"
#include "KTWHoloHUD.h"
#include "KTWSession.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AKTWRogueRun::Action(const FString& Id){if(KTWReferenceUI::RogueAction(this,Id))return;auto* S=UKTWSession::Get(this);if(Id==TEXT("RefreshRules")){RefreshRules();return;}if(Id==TEXT("Start")){StartBattle();return;}if(Id==TEXT("Auto")){AutoDeploy();return;}if(Id==TEXT("Draft")&&Phase==EKTWRoguePhase::Result&&Won){DraftRules();return;}if(Id==TEXT("Retry")){Retry();return;}if(Id==TEXT("Next")){NextWave();return;}if(Id==TEXT("Refresh")){RefreshShop(true);return;}if(Id==TEXT("Restart")){if(S)S->NewRound();return;}if(Id==TEXT("Menu")){if(S)S->Action(TEXT("Menu"));return;}if(Id==TEXT("Speed")){Speed=Speed==1?2:Speed==2?.5f:1;UGameplayStatics::SetGlobalTimeDilation(GetWorld(),Speed);return;}if(Id.StartsWith(TEXT("Buy"))){const int I=FCString::Atoi(*Id.Mid(3));Buy(I);return;}if(Id.StartsWith(TEXT("Card"))&&Phase==EKTWRoguePhase::Draft){const int I=FCString::Atoi(*Id.Mid(4));if(Choices.Contains(I)&&!ActiveCards.Contains(I)){ActiveCards.Add(I);if(S)S->Earn(TEXT("law"));RebuildRules();Phase=EKTWRoguePhase::Shop;RefreshShop(false);}return;}}
void AKTWRogueRun::UpdateCamera(AKTWCameraPawn* Cam,float D){auto* PC=Cast<APlayerController>(Cam->GetController());if(!PC)return;const FRotator Rot(CameraPitch,CameraYaw,0);const FVector Forward=FRotator(0,CameraYaw,0).Vector(),Right=FRotator(0,CameraYaw+90,0).Vector();if(PC->IsInputKeyDown(EKeys::W))CameraFocus+=Forward*D*1000;if(PC->IsInputKeyDown(EKeys::S))CameraFocus-=Forward*D*1000;if(PC->IsInputKeyDown(EKeys::D))CameraFocus+=Right*D*1000;if(PC->IsInputKeyDown(EKeys::A))CameraFocus-=Right*D*1000;CameraFocus.X=FMath::Clamp(CameraFocus.X,-1800.,1800.);CameraFocus.Y=FMath::Clamp(CameraFocus.Y,-1000.,1000.);if(PC->IsInputKeyDown(EKeys::MiddleMouseButton)){float X,Y;PC->GetInputMouseDelta(X,Y);CameraYaw+=X*.3f;CameraPitch=FMath::Clamp(CameraPitch-Y*.25f,-82.f,-30.f);}if(PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))CameraDistance*=.88f;if(PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))CameraDistance*=1.14f;CameraDistance=FMath::Clamp(CameraDistance,2300.f,6500.f);if(PC->WasInputKeyJustPressed(EKeys::Home)){CameraFocus=FVector(-350,0,0);CameraDistance=4600;CameraYaw=-90;CameraPitch=-58;}Cam->SetActorLocation(CameraFocus-Rot.Vector()*CameraDistance);Cam->Camera->SetRelativeRotation(Rot);Cam->Camera->FieldOfView=58;}
void AKTWRogueRun::CommandSelected(FVector Point,AKTWRogueUnit* Enemy){
 TArray<AKTWRogueUnit*> Squad;float Spacing=110;
 for(auto Weak:Selected)if(Weak.IsValid()&&Weak->Alive&&Weak->Team==0){Squad.Add(Weak.Get());Spacing=FMath::Max(Spacing,80*Weak->Stats.Scale);}
 if(Squad.IsEmpty())return;
 TArray<FVector> Goals;const int Columns=FMath::Min(3,Squad.Num()),Rows=FMath::DivideAndRoundUp(Squad.Num(),Columns);
 for(int I=0;I<Squad.Num();++I){FVector Goal=Point;if(Squad.Num()>1)Goal+=FVector((I%Columns-(Columns-1)*.5f)*Spacing,(I/Columns-(Rows-1)*.5f)*Spacing,0);if(Phase==EKTWRoguePhase::Battle){Goal.X=FMath::Clamp(Goal.X,-1700.,1700.);Goal.Y=FMath::Clamp(Goal.Y,-800.,800.);}Goals.Add(Goal);}
 if(Phase==EKTWRoguePhase::Deployment){if(!PlaceSquad(Squad,Goals))Notice=TEXT("请放在蓝方区域内，并与其他单位留出空间；本次位置没有改变。");return;}
 if(Phase==EKTWRoguePhase::Battle)for(int I=0;I<Squad.Num();++I){if(Enemy&&Enemy->Alive&&Enemy->Team!=0)Squad[I]->OrderAttack(Enemy);else Squad[I]->OrderMove(Goals[I]);}
}
AKTWRogueUnit* AKTWRogueRun::PickUnit(APlayerController* PC,FVector2D Cursor)const{
 AKTWRogueUnit* Picked=nullptr;float Best=22;
 for(auto Unit:Units)if(Unit&&Unit->Alive){
  FVector2D Bottom,Top;const float Height=88*Unit->Stats.Scale*Unit->SizeFactor;
  if(!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation()-FVector(0,0,Height*.9f),Bottom)||!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation()+FVector(0,0,Height),Top))continue;
  const FVector2D Segment=Top-Bottom;const float Fraction=FMath::Clamp(FVector2D::DotProduct(Cursor-Bottom,Segment)/FMath::Max(1.,Segment.SizeSquared()),0.,1.);
  const float Distance=FVector2D::Distance(Cursor,Bottom+Segment*Fraction);if(Distance<Best){Best=Distance;Picked=Unit;}
 }
 return Picked;
}
void AKTWRogueRun::ClickWorld(APlayerController* PC,FVector2D Cursor,bool Primary,bool Add){
 if(Phase!=EKTWRoguePhase::Deployment&&Phase!=EKTWRoguePhase::Battle)return;
 auto* Picked=PickUnit(PC,Cursor);
 if(Picked&&Picked->Team==0){if(!Add)Selected.Empty();Selected.AddUnique(Picked);Notice=FString::Printf(TEXT("已选中 %s。%s"),*Picked->Stats.Name,Phase==EKTWRoguePhase::Deployment?TEXT("点击蓝方地面调整位置。"):TEXT("右键地面移动，右键敌人集火。"));return;}
 if(Primary&&Phase==EKTWRoguePhase::Battle){if(!Add)Selected.Empty();return;}
 if(Picked&&Phase==EKTWRoguePhase::Deployment){Notice=TEXT("敌方阵容可提前观察，只能调整己方单位。");return;}
 FVector Origin,Direction;if(!PC->DeprojectScreenPositionToWorld(Cursor.X,Cursor.Y,Origin,Direction)||Direction.Z>=-.01)return;
 const FVector Point=Origin+Direction*(-Origin.Z/Direction.Z);if(FMath::Abs(Point.X)>1800||FMath::Abs(Point.Y)>900)return;
 CommandSelected(Point,Picked&&Picked->Team==1?Picked:nullptr);
}
void AKTWRogueRun::Handle(AKTWPrototypeController* PC){
 float X=0,Y=0;PC->GetMousePosition(X,Y);const FVector2D Cursor(X,Y);int32 W,H;PC->GetViewportSize(W,H);
 if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)&&Phase==EKTWRoguePhase::Deployment)StartBattle();
 if(PC->WasInputKeyJustPressed(EKeys::Tab)&&Phase==EKTWRoguePhase::Battle)Action(TEXT("Speed"));
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))for(const auto& Button:Buttons)if(X>=Button.Rect.X&&Y>=Button.Rect.Y&&X<Button.Rect.X+Button.Rect.Z&&Y<Button.Rect.Y+Button.Rect.W){if(Button.Enabled)Action(Button.Id);return;}
 if(Phase!=EKTWRoguePhase::Deployment&&Phase!=EKTWRoguePhase::Battle)return;
 const float Scale=FMath::Clamp(FMath::Min(W/1600.f,H/900.f),.65f,1.4f);const bool InWorld=KTWReferenceUI::WorldCursor(W,H,Cursor,true,Phase==EKTWRoguePhase::Deployment);
 const bool Add=PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift);
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&InWorld){ClickWorld(PC,Cursor,true,Add);return;}
 if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton)&&InWorld){SelectionStart=Cursor;Selecting=true;}
 if(PC->WasInputKeyJustReleased(EKeys::RightMouseButton)&&Selecting){
  Selecting=false;if(!InWorld)return;
  if(FVector2D::Distance(Cursor,SelectionStart)>12){if(!Add)Selected.Empty();FBox2D Box(ForceInit);Box+=SelectionStart;Box+=Cursor;for(auto Unit:Units)if(Unit&&Unit->Alive&&Unit->Team==0){FVector2D Point;if(PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(),Point)&&Box.IsInside(Point))Selected.AddUnique(Unit);}return;}
  ClickWorld(PC,Cursor,false,Add);
 }
}
void AKTWRogueRun::Draw(AKTWPrototypeHUD* H){KTWReferenceUI::Rogue(this,H);}
