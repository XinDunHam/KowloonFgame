#pragma once
#include "CoreMinimal.h"
class AKTWPrototypeHUD;class AKTWRogueRun;class AKTWHoloHUD;
namespace KTWReferenceUI {
 void Rogue(AKTWRogueRun* Run,AKTWPrototypeHUD* HUD);
 void Street(AKTWHoloHUD* HUD);
 bool StreetClick(AKTWHoloHUD* HUD,float X,float Y);
 bool WorldCursor(int W,int H,FVector2D Point,bool Rogue=false,bool Deploy=false,bool Street=false);
 bool RogueAction(AKTWRogueRun* Run,const FString& Id);
 void MenuButton(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,bool Active,bool Enabled=true);
 void Audit(AKTWPrototypeHUD* H);
}
