#pragma once
#include "KTWPrototype.h"
#include "KTWHoloHUD.generated.h"

/** Canvas HUD with smoothed presentation; economy and commands retain their authoritative owners. */
UCLASS() class KOWLOONFGAME_API AKTWHoloHUD:public AKTWPrototypeHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 bool HandleHudClick(float X,float Y);
 void RoleIcon(float X,float Y,float Size,int32 RoleID,FLinearColor Color);
private:
 bool bInitialized=false;
 float GoldVisual[2]={0,0};
 float CashPulse[2]={0,0};
 int32 PreviousCash[2]={0,0},CashDelta[2]={0,0};
 FString LastNotice;
 float NoticeAge=10;
 TMap<int32,float> Hover;
 void Panel(float X,float Y,float W,float H,FLinearColor Accent,bool Grid=false);
 void Meter(float X,float Y,float W,float Fraction,FLinearColor Color,float Height=5);
 void Ring(FVector2D P,float Radius,float Fraction,FLinearColor Color);
 bool VisiblePoint(FVector World,FVector2D& Screen)const;
 void DrawWorld();
 void LaneIcon(float X,float Y,float Size,int32 Lane,bool Active);
};
