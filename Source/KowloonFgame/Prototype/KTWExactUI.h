#pragma once
#include "CoreMinimal.h"
class AKTWPrototypeHUD;
namespace KTWExactUI {
 bool Panel(AKTWPrototypeHUD* H,float X,float Y,float W,float Height);
 bool Button(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,bool Active,bool Enabled=true);
 bool Role(AKTWPrototypeHUD* H,float X,float Y,float Size,int RoleID);
 bool Rule(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,int CardID);
 bool Logo(AKTWPrototypeHUD* H,float X,float Y,float W,float Height);
 bool Medal(AKTWPrototypeHUD* H,float X,float Y,float Size,bool Street=false);
}
