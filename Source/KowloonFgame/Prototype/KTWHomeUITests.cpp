#include "KTWSession.h"
#include "KTWPrototype.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "Engine/GameViewportClient.h"
void UKTWSession::RunHomeTests(){
#if !UE_BUILD_SHIPPING
 auto Fail=MakeShared<int>(0);auto Check=[Fail](bool OK,const TCHAR* Name){*Fail+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW HOME %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));};
 auto Has=[this](const TCHAR* Id){return Buttons.ContainsByPredicate([&](const FKTWMenuButton& B){return B.Id==Id&&B.Enabled;});};
 Check(InMenu&&CoverEntered&&Ready(),TEXT("home opens directly with ready scene"));Check(Has(TEXT("ModeStreet"))&&Has(TEXT("ModeRogue"))&&Has(TEXT("Play")),TEXT("both cards and start button have live hit areas"));
 auto Later=[this](float Delay,TFunction<void()> Fn){FTimerHandle T;GetWorld()->GetTimerManager().SetTimer(T,FTimerDelegate::CreateLambda(MoveTemp(Fn)),Delay,false);};
 auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 English=true;ModeChoice=1;DifficultyOpen=false;Later(.8f,[Shot](){Shot(TEXT("Home_English.png"));});
 Later(1.3f,[this,Check](){Action(TEXT("ModeStreet"));Check(ModeChoice==0,TEXT("street card changes actual mode"));Action(TEXT("DifficultyToggle"));Check(DifficultyOpen,TEXT("difficulty opens dropdown"));});
 Later(2.f,[Shot](){Shot(TEXT("Home_Difficulty.png"));});
 Later(2.5f,[this,Check,Has](){Check(Has(TEXT("Difficulty0"))&&Has(TEXT("Difficulty1"))&&Has(TEXT("Difficulty2")),TEXT("dropdown exposes all difficulty choices"));Action(TEXT("Difficulty2"));Check(Difficulty==2&&!DifficultyOpen,TEXT("difficulty selection stores hard and closes dropdown"));Action(TEXT("Language"));Check(!English,TEXT("language button switches Chinese"));});
 Later(3.3f,[Shot](){Shot(TEXT("Home_Chinese.png"));});
 Later(4.f,[this,Check,Has,Fail](){Check(Has(TEXT("Collection"))&&Has(TEXT("How"))&&Has(TEXT("Settings"))&&Has(TEXT("Quit")),TEXT("navigation features remain reachable"));Action(TEXT("Collection"));Check(Collection,TEXT("collection action opens existing collection"));Action(TEXT("CloseCollection"));Action(TEXT("How"));Check(Help,TEXT("guide action opens current guide"));Action(TEXT("Back"));Check(!Help&&InMenu,TEXT("guide returns to home"));Action(TEXT("ModeRogue"));Check(ModeChoice==1,TEXT("rule card changes actual mode"));UE_LOG(LogTemp,Display,TEXT("KTW HOME COMPLETE failures=%d"),*Fail);FPlatformMisc::RequestExitWithStatus(false,*Fail?1:0);});
#endif
}
