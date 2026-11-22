#include "KTWHoloHUD.h"
#include "KTWExactUI.h"
#include "KTWReferenceUI.h"
#include "KTWRogue.h"
#include "KTWStreetWar.h"
#include "KTWGold.h"
#include "KTWMatch.h"
#include "KTWSession.h"
#include "KTWGeneration.h"
#include "KTWPresentation.h"
#include "KTWCommander.h"
#include "KTWDynamics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Kismet/GameplayStatics.h"

namespace {const FLinearColor Cyan(.08f,.72f,.78f),Amber(.80f,.67f,.45f),Red(.8f,.32f,.29f);const FColor Muted(178,176,162),White(239,224,195);FLinearColor Team(int32 F){return F?Red:Cyan;}const TCHAR* RoleName(EKTWRole R){const TCHAR* N[]={TEXT("RUNNER"),TEXT("KNIFE"),TEXT("PIPE"),TEXT("THROWER")};return N[int32(R)];}}
void AKTWHoloHUD::LaneIcon(float X,float Y,float Size,int32 Lane,bool Active){
 auto Point=[&](float U,float V){return FVector2D(X+U*Size,Y+V*Size);};auto Edge=[&](FVector2D A,FVector2D B,FLinearColor C,float Width){DrawLine(A.X,A.Y,B.X,B.Y,C,Width);};
 const auto BL=Point(.12,.88),TL=Point(.12,.12),TR=Point(.88,.12),BR=Point(.88,.88);const FLinearColor Gray(.22,.28,.32);const float Stroke=FMath::Max(1.f,Size*.065f);
 DrawRect(FLinearColor(.035,.045,.055),X,Y,Size,Size);for(auto E:TArray<TPair<FVector2D,FVector2D>>{{BL,TL},{TL,TR},{BL,BR},{BR,TR},{BL,TR},{TL,BR}})Edge(E.Key,E.Value,Gray,Stroke);
 const auto C=Active?Cyan:FLinearColor(.48,.62,.67);const float Width=FMath::Max(1.5f,Size*.10f);if(Lane==0){Edge(BL,TL,C,Width);Edge(TL,TR,C,Width);}else if(Lane==1)Edge(BL,TR,C,Width);else{Edge(BL,BR,C,Width);Edge(BR,TR,C,Width);}DrawRect(C,BL.X-1.5,BL.Y-1.5,3,3);DrawRect(C,TR.X-1.5,TR.Y-1.5,3,3);
}
void AKTWHoloHUD::RoleIcon(float X,float Y,float Size,int32 RoleID,FLinearColor C){
 if(KTWExactUI::Role(this,X,Y,Size,RoleID))return;
 auto Line=[&](float A,float B,float D,float E,float Width=2.f){DrawLine(X+A*Size,Y+B*Size,X+D*Size,Y+E*Size,C,FMath::Max(1.f,Width*Size/28));};
 DrawRect(FLinearColor(.02,.09,.12,.9),X-3,Y-3,Size+6,Size+6);
 if(RoleID==0){Ring(FVector2D(X+.56*Size,Y+.17*Size),Size*.105,1,C);Line(.52,.31,.43,.55,3);Line(.48,.4,.23,.43);Line(.48,.4,.74,.49);Line(.43,.55,.64,.69,3);Line(.64,.69,.48,.91,3);Line(.43,.55,.28,.78,3);Line(.28,.78,.08,.78,3);Line(.02,.26,.26,.26,1);Line(.04,.57,.21,.57,1);}
 else if(RoleID==1){Line(.2,.87,.4,.65,4);Line(.28,.56,.5,.76,2);Line(.4,.65,.81,.13,3);Line(.81,.13,.69,.53,2);Line(.69,.53,.47,.7,2);Line(.4,.65,.69,.53,2);}
 else if(RoleID==2){Line(.23,.9,.23,.29,5);Line(.23,.29,.38,.14,5);Line(.38,.14,.78,.14,5);Line(.13,.7,.33,.7,1);Line(.13,.79,.33,.79,1);Line(.13,.87,.33,.87,1);Line(.67,.06,.67,.22,1);}
 else if(RoleID==3){Line(.1,.88,.27,.58,4);Line(.27,.58,.48,.61,3);Line(.48,.61,.61,.47,3);Line(.18,.7,.45,.77,2);Ring(FVector2D(X+.75*Size,Y+.21*Size),Size*.12,1,C);Line(.45,.3,.54,.17,1);Line(.54,.17,.63,.12,1);Line(.68,.43,.79,.39,1);Line(.79,.39,.88,.31,1);}
 else if(RoleID==4){Line(.13,.35,.27,.72,3);Line(.27,.72,.77,.72,3);Line(.77,.72,.9,.35,3);Line(.13,.35,.38,.52,3);Line(.38,.52,.51,.2,3);Line(.51,.2,.65,.52,3);Line(.65,.52,.9,.35,3);Line(.27,.87,.77,.87,3);}
 else if(RoleID==5){Line(.12,.28,.88,.28,5);Line(.12,.42,.83,.42,4);Line(.88,.28,.88,.42,3);Line(.26,.43,.17,.82,5);Line(.17,.82,.36,.82,3);Line(.36,.82,.45,.45,3);Line(.45,.58,.62,.58,2);Line(.62,.58,.65,.43,2);}
}
void AKTWHoloHUD::Panel(float X,float Y,float W,float H,FLinearColor A,bool Grid){
 if(KTWExactUI::Panel(this,X,Y,W,H))return;
 DrawRect(FLinearColor(0,0,0,.22f),X+3,Y+5,W,H);
 DrawRect(FLinearColor(.012f,.018f,.022f,.96f),X,Y,W,H);
 DrawRect(FLinearColor(.18f,.16f,.12f,.45f),X,Y,W,27);
 DrawLine(X,Y,X+W,Y,FLinearColor(.62f,.53f,.38f,.8f));
 DrawLine(X,Y+H,X+W,Y+H,FLinearColor(.38f,.33f,.25f,.8f));
 DrawLine(X+10,Y,X+FMath::Min(W*.24f,56.f),Y,A,2);
 DrawLine(X+W-15,Y+H,X+W,Y+H-15,FLinearColor(.72f,.65f,.5f),2);for(int32 I=0;I<4;++I)DrawLine(X+W-70+I*12,Y+4,X+W-65+I*12,Y+7,FLinearColor(.32f,.29f,.23f),1);
 if(Grid)DrawLine(X+W-9,Y+9,X+W-9,Y+18,A,1);
}
void AKTWHoloHUD::Meter(float X,float Y,float W,float F,FLinearColor C,float H){DrawRect(FLinearColor(.055f,.11f,.15f,.9f),X,Y,W,H);F=FMath::Clamp(F,0.f,1.f);DrawRect(C,X,Y,W*F,H);if(F>0)DrawRect(FLinearColor(.75f,.95f,1,.8f),X+W*F-1,Y,2,H);}
void AKTWHoloHUD::Ring(FVector2D P,float R,float Fraction,FLinearColor C){for(int32 I=0;I<32;++I){const float A=(I/32.f*2*PI)-PI/2,B=((I+1)/32.f*2*PI)-PI/2;DrawLine(P.X+FMath::Cos(A)*R,P.Y+FMath::Sin(A)*R,P.X+FMath::Cos(B)*R,P.Y+FMath::Sin(B)*R,I/32.f<Fraction?C:FLinearColor(.12f,.2f,.24f,.65f),2);}}
bool AKTWHoloHUD::VisiblePoint(FVector P,FVector2D& S)const{auto* PC=GetOwningPlayerController();if(!PC||!PC->ProjectWorldLocationToScreen(P,S)||S.X<-60||S.Y<-40||S.X>Canvas->SizeX+60||S.Y>Canvas->SizeY+40)return false;auto* Cam=Cast<AKTWCameraPawn>(PC->GetPawn());if(Cam&&Cam->bStreetView&&PC->PlayerCameraManager){FHitResult H;if(GetWorld()->LineTraceSingleByObjectType(H,PC->PlayerCameraManager->GetCameraLocation(),P,FCollisionObjectQueryParams(ECC_WorldStatic))&&FVector::Dist(H.ImpactPoint,P)>80)return false;}return true;}
void AKTWHoloHUD::DrawWorld(){auto* PC=Cast<AKTWPrototypeController>(GetOwningPlayerController());auto* Cam=PC?Cast<AKTWCameraPawn>(PC->GetPawn()):nullptr;const float Now=GetWorld()->GetTimeSeconds();
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Health>0&&AKTWStreetWar::Visible(GetWorld(),*I)){const bool Selected=PC&&PC->SelectedUnits.Contains(*I);const FVector Head=I->GetMesh()->GetSocketLocation(TEXT("head"))+FVector(0,0,110);FVector2D P;if(!VisiblePoint(Head,P))continue;auto* C=I->FindComponentByClass<UKTWGoldCarrier>();const float W=Selected?92:68;DrawRect(FLinearColor(.003f,.008f,.015f,.85f),P.X-W*.5f-3,P.Y-21,W+6,37);DrawText(FString::Printf(TEXT("%s / L%d"),RoleName(I->UnitRole),I->Level),Team(I->Faction).ToFColor(true),P.X-W*.5f,P.Y-19,nullptr,.68f);Meter(P.X-W*.5f,P.Y-3,W,I->Health/I->Stats.MaxHealth,Team(I->Faction),4);
  if(C){DrawText(FString::Printf(TEXT("G %d / %d"),C->Carried,I->Stats.Capacity),C->Carried?FColor(255,208,90):Muted,P.X-W*.5f,P.Y+4,nullptr,.65f);if(C->Task==EKTWGoldTask::PickingUp||C->Task==EKTWGoldTask::Delivering||C->Task==EKTWGoldTask::Raiding){const float F=C->ChannelFraction();Ring(P+FVector2D(0,-50),18,F,Amber);DrawText(FString::Printf(TEXT("%d%%"),FMath::RoundToInt(F*100)),FColor(255,220,130),P.X-10,P.Y-55,nullptr,.65f);const TCHAR* N=C->Task==EKTWGoldTask::PickingUp?TEXT("COLLECT"):C->Task==EKTWGoldTask::Delivering?TEXT("BANK"):TEXT("RAID");DrawText(N,FColor(255,220,130),P.X-23,P.Y-78,nullptr,.65f);}
   const float Age=Now-C->LastTransferAt;if(Age>=0&&Age<1.1f&&C->LastTransferAmount>0){FLinearColor Color=C->bLastBanked?Cyan:Amber;Color.A=1-Age/1.1f;DrawText(FString::Printf(TEXT("+%d %s"),C->LastTransferAmount,C->bLastBanked?TEXT("BANKED"):TEXT("GOLD")),Color,P.X-35,P.Y-50-Age*38,nullptr,.95f);}}
  if(Selected){FVector2D Foot;if(VisiblePoint(I->GetActorLocation(),Foot)){Ring(Foot,17,1,Cyan);DrawLine(Foot.X-23,Foot.Y,Foot.X-18,Foot.Y,Cyan,2);DrawLine(Foot.X+18,Foot.Y,Foot.X+23,Foot.Y,Cyan,2);}}
 }
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I){if(!AKTWStreetWar::Visible(GetWorld(),*I))continue;FVector2D P;const auto V=I->bRecovering?AKTWStreetDirector::BasePosition(I->Faction)+FVector(0,0,300):I->GetMesh()->GetSocketLocation(TEXT("head"))+FVector(0,0,150);if(!VisiblePoint(V,P))continue;Panel(P.X-55,P.Y-27,110,34,Team(I->Faction));DrawText(I->bRecovering?FString::Printf(TEXT("BOSS / %.0fs"),I->RecoveryRemaining):TEXT("BOSS / COMMANDER"),White,P.X-50,P.Y-22,nullptr,.7f);Meter(P.X-50,P.Y-3,100,I->Health/I->MaxHealth,Team(I->Faction),5);}
 for(TActorIterator<AKTWGoldPile>I(GetWorld());I;++I){FVector2D P;if(!VisiblePoint(I->GetActorLocation()+FVector(0,0,100),P))continue;DrawText(FString::Printf(TEXT("GOLD %d"),I->Amount),I->Amount>0?FColor(255,210,86):Muted,P.X-23,P.Y,nullptr,.75f);}
 if(auto* L=AKTWStreetLife::Find(GetWorld()))for(AKTWStreetWeapon* W:L->GroundWeapons)if(IsValid(W)&&!W->Equipped){FVector2D P;if(!VisiblePoint(W->GetActorLocation()+FVector(0,0,300),P))continue;const TCHAR* Names[]={TEXT("KNIFE +20%"),TEXT("PIPE +25%"),TEXT("STOOL +35%"),TEXT("MED +60HP"),TEXT("MOLOTOV x1")};DrawRect(FLinearColor(.006f,.025f,.04f,.85f),P.X-43,P.Y-16,86,18);DrawText(Names[FMath::Clamp(W->Kind,0,4)],W->Kind==3?FColor(110,255,170):FColor(90,220,255),P.X-40,P.Y-14,nullptr,.7f);}
 if(PC){if(PC->bSelecting){float X,Y;PC->GetMousePosition(X,Y);const float A=FMath::Min(X,float(PC->SelectionStart.X)),B=FMath::Min(Y,float(PC->SelectionStart.Y)),W=FMath::Abs(X-PC->SelectionStart.X),H=FMath::Abs(Y-PC->SelectionStart.Y);DrawRect(FLinearColor(.05f,.65f,.9f,.09f),A,B,W,H);DrawLine(A,B,A+W,B,Cyan);DrawLine(A,B,A,B+H,Cyan);DrawLine(A+W,B,A+W,B+H,Cyan);DrawLine(A,B+H,A+W,B+H,Cyan);}
  if(Now-PC->LastCommandTime<1.15f){FVector2D P;if(VisiblePoint(PC->LastCommandPoint+FVector(0,0,40),P))DrawText(TEXT("ORDER RECEIVED"),FColor(95,225,255),P.X-42,P.Y-26,nullptr,.8f);}}
 if(!Cam||!Cam->bStreetView){auto Label=[&](const TCHAR* T,FVector P){FVector2D S;if(VisiblePoint(P,S))DrawText(T,Muted,S.X-35,S.Y,nullptr,.7f);};Label(TEXT("UPPER / BACK ALLEY"),FVector(-1500,-6000,30));Label(TEXT("LOWER / MARKET"),FVector(0,6000,30));Label(TEXT("GOLD / LINK ALLEY"),FVector(-2300,-2300,30));}
}
bool AKTWHoloHUD::HandleHudClick(float X,float Y){return KTWReferenceUI::StreetClick(this,X,Y);}
void AKTWHoloHUD::DrawHUD(){AHUD::DrawHUD();if(!Canvas)return;if(auto* S=UKTWSession::Get(this))if(S->Draw(this))return;if(auto* R=AKTWRogueRun::Find(GetWorld())){R->Draw(this);return;}KTWReferenceUI::Street(this);}
