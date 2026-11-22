#include "KTWExactUI.h"
#include "KTWPrototype.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
namespace {
UTexture2D* Texture(const FString& Name){static TMap<FString,TStrongObjectPtr<UTexture2D>> Cache;if(auto* Existing=Cache.Find(Name))return Existing->Get();const FString Path=TEXT("/Game/KTW/UI/Exact/T_")+Name+TEXT(".T_")+Name;auto* Value=LoadObject<UTexture2D>(nullptr,*Path);if(Value)Cache.Add(Name,TStrongObjectPtr<UTexture2D>(Value));return Value;}
bool Slice(AKTWPrototypeHUD* H,const FString& Name,float X,float Y,float W,float Height,float U=0,float V=0,float UW=1,float VH=1,FLinearColor Tint=FLinearColor::White){auto* T=Texture(Name);if(!T)return false;H->DrawTexture(T,X,Y,W,Height,U,V,UW,VH,Tint,BLEND_Translucent);return true;}
}
bool KTWExactUI::Panel(AKTWPrototypeHUD* H,float X,float Y,float W,float Height){
 // Sample the supplied smoke plate, not the fixed example text or its black ownership holes.
 if(!Slice(H,TEXT("04_intermission_shop__sidebar_plate"),X,Y,W,Height,.08f,.87f,.84f,.10f))return false;
 H->DrawRect(FLinearColor(.015f,.03f,.04f,.45f),X,Y,W,Height);H->DrawLine(X,Y,X+W,Y,FLinearColor(.6f,.7f,.73f),1);H->DrawLine(X,Y+Height,X+W-12,Y+Height,FLinearColor(.28f,.44f,.49f),1);H->DrawLine(X+W-12,Y+Height,X+W,Y+Height-12,FLinearColor(.1f,.8f,.88f),2);return true;
}
bool KTWExactUI::Button(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,bool Active,bool Enabled){
 auto* PC=H->GetOwningPlayerController();const bool Pressed=Active&&PC&&PC->IsInputKeyDown(EKeys::LeftMouseButton);
 FString State=!Enabled?TEXT("disabled"):Pressed?TEXT("pressed"):Active?TEXT("hover"):TEXT("default");const FString Name=TEXT("04_intermission_shop__buy_gunner_")+State;
 if(!Texture(Name))return false;
 // Nine-slice around baked BUY lettering: the label remains localized and live.
 const float Side=FMath::Min(Height*.65f,W*.22f);const auto Tint=Enabled?(Active?FLinearColor::White:FLinearColor(.22f,.31f,.36f,1)):FLinearColor(.12f,.15f,.17f,1);
 Slice(H,Name,X,Y,Side,Height,0,0,.22f,1,Tint);Slice(H,Name,X+W-Side,Y,Side,Height,.78f,0,.22f,1,Tint);
 Slice(H,Name,X+Side,Y,W-2*Side,Height,.28f,.45f,.06f,.10f,Tint);
 Slice(H,Name,X+Side,Y,W-2*Side,Height*.16f,.22f,0,.56f,.16f,Tint);Slice(H,Name,X+Side,Y+Height*.84f,W-2*Side,Height*.16f,.22f,.84f,.56f,.16f,Tint);return true;
}
bool KTWExactUI::Role(AKTWPrototypeHUD* H,float X,float Y,float Size,int ID){const TCHAR* Names[]={TEXT("runner_offer"),TEXT("blade_offer"),TEXT(""),TEXT("thrower_offer"),TEXT(""),TEXT("gunner_offer")};if(ID<0||ID>5||ID==4)return false;if(ID==2)return Slice(H,TEXT("02_rule_run_battle__select_baton"),X,Y,Size,Size,.02f,.10f,.30f,.80f);return Slice(H,FString(TEXT("04_intermission_shop__"))+Names[ID],X,Y,Size,Size,.17f,ID==5?.15f:.09f,.65f,.34f);}
bool KTWExactUI::Rule(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,int ID){const TCHAR* Name=ID==6?TEXT("enemy_lightweight_card"):ID==1?TEXT("double_impact_card"):ID==7?TEXT("precision_impact_card"):nullptr;if(Name)return Slice(H,FString(TEXT("03_rule_choice__"))+Name,X,Y,W,Height,.18f,.05f,.67f,.39f);return Slice(H,TEXT("02_rule_run_battle__rules_panel"),X,Y,W,Height,0,0,.32f,.95f);}
bool KTWExactUI::Logo(AKTWPrototypeHUD* H,float X,float Y,float W,float Height){return Slice(H,TEXT("04_intermission_shop__logo"),X,Y,W,Height);}
bool KTWExactUI::Medal(AKTWPrototypeHUD* H,float X,float Y,float Size,bool Street){return Slice(H,Street?TEXT("06_street_war_result_van__victory_emblem"):TEXT("05_rule_run_result__victory_emblem"),X,Y,Size,Size);}

