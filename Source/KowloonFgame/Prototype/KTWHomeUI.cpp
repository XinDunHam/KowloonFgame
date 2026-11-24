#include "KTWSession.h"
#include "KTWPrototype.h"
#include "KTWReferenceUI.h"
#include "KTWSharpText.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

void UKTWSession::DrawHome(AKTWPrototypeHUD* H){
 auto* C=H->GameCanvas();if(!C)return;const float Scale=FMath::Min(C->SizeX/1672.f,C->SizeY/941.f),OX=(C->SizeX-1672*Scale)*.5f,OY=(C->SizeY-941*Scale)*.5f;
 const FLinearColor Ink(.009f,.025f,.035f,1),Paper(.91f,.92f,.89f,1),Cyan(.02f,.88f,.95f,1),Blue(.022f,.065f,.15f,1),Red(.12f,.025f,.055f,1);
 auto Rect=[&](float X,float Y,float W,float V,FLinearColor Col){H->DrawRect(Col,OX+X*Scale,OY+Y*Scale,W*Scale,V*Scale);};
 auto Line=[&](float X,float Y,float A,float B,FLinearColor Col,float Width=1){H->DrawLine(OX+X*Scale,OY+Y*Scale,OX+A*Scale,OY+B*Scale,Col,Width*Scale);};
 auto Text=[&](FString T,float X,float Y,float Size,FLinearColor Col,bool Bold=false){KTWSharpText::Draw(H,T,OX+X*Scale,OY+Y*Scale,Size*Scale,Col,Bold);};
 auto L=[&](const TCHAR* EN,const TCHAR* CN){return FString(English?EN:CN);};
 auto Add=[&](FString Id,float X,float Y,float W,float V,bool Enabled=true,bool Front=false){FKTWMenuButton B;B.Id=Id;B.Rect=FVector4(OX+X*Scale,OY+Y*Scale,W*Scale,V*Scale);B.Enabled=Enabled;if(Front)Buttons.Insert(B,0);else Buttons.Add(B);};
 H->DrawRect(FLinearColor::Black,0,0,C->SizeX,C->SizeY);static TStrongObjectPtr<UTexture2D> Art(LoadObject<UTexture2D>(nullptr,TEXT("/Game/KTW/UI/Home/T_MenuReference.T_MenuReference")));
 if(Art.IsValid())H->DrawTexture(Art.Get(),OX,OY,1672*Scale,941*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);

#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWHomeTest"))&&Ready()){static bool Scheduled=false;if(!Scheduled){Scheduled=true;FTimerHandle T;GetWorld()->GetTimerManager().SetTimer(T,this,&UKTWSession::RunHomeTests,.5f,false);}}
#endif
 Buttons.Empty();
 // Keep the supplied illustration and icon silhouettes; all mutable wording uses native font glyphs.
 Rect(326,126,770,65,Ink);Line(340,162,426,162,Cyan,2);Line(1008,162,1092,162,Cyan,2);Text(L(TEXT("CHOOSE YOUR MODE"),TEXT("选择游玩模式")),454,140,38,Paper,true);
 const TCHAR* NavEN[]={TEXT("PLAY"),TEXT("COLLECTION"),TEXT("HOW TO PLAY"),TEXT("QUIT")};const TCHAR* NavCN[]={TEXT("开始游戏"),TEXT("成就收藏"),TEXT("游玩指南"),TEXT("退出游戏")};const TCHAR* IDs[]={TEXT("SelectMode"),TEXT("Collection"),TEXT("How"),TEXT("Quit")};const float NY[]={235,391,543,695};
 for(int I=0;I<4;++I){Rect(145,NY[I],150,43,I==0?FLinearColor(.62f,.67f,.68f,1):Ink);Text(English?NavEN[I]:NavCN[I],154,NY[I]+7,I?22:26,I==0?Ink:Paper,true);Add(IDs[I],20,NY[I]-24,275,94);}
 for(int I=0;I<2;++I){const float Y=I?432:223;const auto Col=I?Red:Blue;const bool Selected=ModeChoice==I;
  // Remove the example's permanently selected second card before applying actual session state.
  Line(353,Y-3,1090,Y-3,Ink,10);Line(353,Y+181,1090,Y+181,Ink,10);Line(353,Y-3,353,Y+181,Ink,10);Line(1090,Y-3,1090,Y+181,Ink,10);Rect(333,Y+72,42,48,Ink);
  for(int X=590;X<1080;X+=2){const float A=(X-590)/490.f;const auto Left=I?FLinearColor(.10f,.020f,.043f,1):FLinearColor(.014f,.052f,.13f,1);const auto Right=I?FLinearColor(.065f,.013f,.03f,1):FLinearColor(.012f,.023f,.075f,1);Rect(X,Y+5,2,168,FMath::Lerp(Left,Right,A));}Text(I?TEXT("02"):TEXT("01"),916,Y-6,150,FLinearColor(.76f,.80f,.89f,.13f),true);
  Text(I?L(TEXT("RULE RUN"),TEXT("法则挑战")):L(TEXT("STREET WAR"),TEXT("街区争夺")),615,Y+48,43,Paper,true);
  Text(I?L(TEXT("7 ROUNDS · FORMATION & RULES"),TEXT("7轮挑战 · 布阵与24种法则")):L(TEXT("6-MINUTE REAL-TIME COMMAND"),TEXT("6分钟即时指挥 · 黄金与基地争夺")),615,Y+120,21,Paper);
  const auto Edge=Selected?Cyan:FLinearColor(.35f,.46f,.55f,1);Line(358,Y,1084,Y,Edge,Selected?3:1);Line(358,Y+177,1070,Y+177,Edge,Selected?3:1);Line(358,Y,358,Y+177,Edge,Selected?3:1);Line(1084,Y,1084,Y+163,Edge,Selected?3:1);Line(1070,Y+177,1084,Y+163,Edge,Selected?3:1);
  if(Selected){Line(339,Y+88,353,Y+74,Cyan,2);Line(353,Y+74,367,Y+88,Cyan,2);Line(367,Y+88,353,Y+102,Cyan,2);Line(353,Y+102,339,Y+88,Cyan,2);}
  Add(I?TEXT("ModeRogue"):TEXT("ModeStreet"),355,Y,735,180);
 }
 Rect(365,625,728,44,Ink);Text(ModeChoice?L(TEXT("Recruit. Arrange your formation. Rewrite the rules."),TEXT("招募帮众，调整阵型，用法则改变战局。")):L(TEXT("Command your crew. Seize the gold. Protect your base."),TEXT("指挥帮众，抢夺黄金，守护基地。")),385,635,21,Paper);
 Rect(380,700,153,39,Ink);Text(L(TEXT("DIFFICULTY"),TEXT("游戏难度")),390,708,20,Paper);
 Rect(586,702,174,35,Ink);const TCHAR* LevelsEN[]={TEXT("EASY"),TEXT("NORMAL"),TEXT("HARD")};const TCHAR* LevelsCN[]={TEXT("简单"),TEXT("普通"),TEXT("困难")};Text(English?LevelsEN[FMath::Clamp(Difficulty,0,2)]:LevelsCN[FMath::Clamp(Difficulty,0,2)],600,707,24,Paper,true);Add(TEXT("DifficultyToggle"),568,694,252,48);
 const bool Enabled=Ready()&&!DifficultyOpen;KTWReferenceUI::MenuButton(H,OX+367*Scale,OY+778*Scale,570*Scale,94*Scale,true,Enabled);for(int Y=0;Y<54;++Y){const float Shade=.69f+.08f*(Y/54.f);Rect(433-Y*.85f,795+Y,475,1,FLinearColor(Shade,Shade+.015f,Shade+.01f,1));}const FString StartTitle=Ready()?L(TEXT("START GAME  →"),TEXT("开始游戏  →")):L(TEXT("LOADING STREETS…"),TEXT("正在准备街区…"));Text(StartTitle,652-KTWSharpText::Measure(StartTitle,34*Scale,true).X/(2*Scale),801,34,Ink,true);Add(TEXT("Play"),367,778,570,94,Enabled);
 Rect(1272,37,139,36,Ink);Text(L(TEXT("COLLECTION"),TEXT("成就收藏")),1284,44,20,Paper,true);Add(TEXT("Collection"),1207,26,225,56);
 Rect(1482,33,158,38,Ink);Text(English?TEXT("中文 / EN ✓"):TEXT("中文 ✓ / EN"),1490,40,22,Paper,true);Add(TEXT("Language"),1475,27,172,57);
 Rect(45,846,210,38,Ink);Text(L(TEXT("SETTINGS"),TEXT("设置")),65,854,18,Paper);Add(TEXT("Settings"),40,840,220,45);
 if(DifficultyOpen)for(int I=0;I<3;++I){const float Y=744+I*44;Rect(570,Y,250,43,Ink);Line(570,Y+43,820,Y+43,Cyan);Text(English?LevelsEN[I]:LevelsCN[I],597,Y+10,22,Difficulty==I?Cyan:Paper,true);Add(FString::Printf(TEXT("Difficulty%d"),I),570,Y,250,43,true,true);}
}
