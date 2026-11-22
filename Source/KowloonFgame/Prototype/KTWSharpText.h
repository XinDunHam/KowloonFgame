#pragma once
#include "CoreMinimal.h"
class AKTWPrototypeHUD;
namespace KTWSharpText {
 void Draw(AKTWPrototypeHUD* H,const FString& Text,float X,float Y,float Pixels,FLinearColor Color,bool Bold=false);
 FVector2D Measure(const FString& Text,float Pixels,bool Bold=false);
}
