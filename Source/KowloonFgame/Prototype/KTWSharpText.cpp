#include "KTWSharpText.h"
#include "KTWPrototype.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/Paths.h"
namespace {
UFont* Face(bool Bold){
 static TStrongObjectPtr<UFont> Regular,Heading;auto& Ref=Bold?Heading:Regular;
 if(!Ref.IsValid()){
  Ref.Reset(NewObject<UFont>());Ref->FontCacheType=EFontCacheType::Runtime;
  auto& Font=Ref->GetMutableInternalCompositeFont();const FString Root=FPaths::ProjectContentDir()/TEXT("KTW/UIFont");
  Font.DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"),Root/(Bold?TEXT("Roboto-BoldCondensedItalic.ttf"):TEXT("Roboto-Regular.ttf")),EFontHinting::Auto,EFontLoadingPolicy::LazyLoad));
  Font.FallbackTypeface.Typeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"),Root/TEXT("DroidSansFallback.ttf"),EFontHinting::Auto,EFontLoadingPolicy::LazyLoad));
 }
 return Ref.Get();
}
FSlateFontInfo Info(float Pixels,bool Bold){return FSlateFontInfo(Face(Bold),FMath::Max(8.f,FMath::RoundToFloat(Pixels*.75f)),TEXT("Regular"));}
}
void KTWSharpText::Draw(AKTWPrototypeHUD* H,const FString& Text,float X,float Y,float Pixels,FLinearColor Color,bool Bold){if(!H||!H->GameCanvas()||Text.IsEmpty())return;FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(X),FMath::RoundToFloat(Y)),FText::FromString(Text),Info(Pixels,Bold),Color);Item.EnableShadow(FLinearColor(0,0,0,.35f),FVector2D(0,1));H->GameCanvas()->DrawItem(Item);}
FVector2D KTWSharpText::Measure(const FString& Text,float Pixels,bool Bold){if(!FSlateApplication::IsInitialized())return FVector2D(Text.Len()*Pixels*.6f,Pixels);return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text,Info(Pixels,Bold));}

