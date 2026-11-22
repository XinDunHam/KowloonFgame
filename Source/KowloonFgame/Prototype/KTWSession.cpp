#include "KTWSession.h"
#include "KTWStreetWar.h"
#include "KTWMistTheme.h"
#include "KTWPrototype.h"
#include "KTWRogue.h"
#include "KTWGeneration.h"
#include "KTWMatch.h"
#include "KTWGold.h"
#include "KTWDynamics.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "TimerManager.h"
#include "PCGComponent.h"
#include "HAL/PlatformTime.h"

UKTWSession* UKTWSession::Get(const UObject* C){return C&&C->GetWorld()?Cast<UKTWSession>(C->GetWorld()->GetGameInstance()):nullptr;}
bool UKTWSession::Diagnostic(){const FString C=FCommandLine::Get();if(C.Contains(TEXT("KTWModeTest"))||C.Contains(TEXT("KTWModeCapture"))||C.Contains(TEXT("KTWOrdersLootTest"))||C.Contains(TEXT("KTWMenuCapture"))||C.Contains(TEXT("KTWFrontendTest"))||C.Contains(TEXT("KTWPresentationCapture"))||C.Contains(TEXT("KTWRevisionTest"))||C.Contains(TEXT("KTWSeasonTest"))||C.Contains(TEXT("KTWImmersionTest"))||C.Contains(TEXT("KTWContinuityTest")))return false;return C.Contains(TEXT("KTW"))&&(C.Contains(TEXT("Test"))||C.Contains(TEXT("Capture"))||C.Contains(TEXT("KTWAutomation"))||C.Contains(TEXT("KTWPackageSmoke")));}
void UKTWSession::Init(){Super::Init();LoadProfile();if(FParse::Param(FCommandLine::Get(),TEXT("KTWEnglish")))English=true;InMenu=!FParse::Param(FCommandLine::Get(),TEXT("KTWOrdersLootTest"))&&!Diagnostic()&&!FParse::Param(FCommandLine::Get(),TEXT("KTWPresentationCapture"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWRevisionTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWSeasonTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWImmersionTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("KTWContinuityTest"));if(!Diagnostic())RoundSeed=int32(FDateTime::UtcNow().GetTicks()&0x7fffffff);if(FParse::Param(FCommandLine::Get(),TEXT("KTWContinuityTest")))RoundSeed=678844112;if(!Diagnostic()){Quality=1;if(auto* G=UGameUserSettings::GetGameUserSettings()){G->SetOverallScalabilityLevel(Quality);G->ApplyNonResolutionSettings();}}if(FParse::Param(FCommandLine::Get(),TEXT("KTWRogueDeployTest"))||FParse::Param(FCommandLine::Get(),TEXT("KTWRogueTest"))||FParse::Param(FCommandLine::Get(),TEXT("KTWRogueCapture"))||FParse::Param(FCommandLine::Get(),TEXT("KTWRogueSoak"))){ModeChoice=1;InMenu=false;}if(FParse::Param(FCommandLine::Get(),TEXT("KTWRogueSoak"))){int Value=1;if(FParse::Value(FCommandLine::Get(),TEXT("KTWRogueDifficulty="),Value))Difficulty=FMath::Clamp(Value,0,2);}if(FParse::Param(FCommandLine::Get(),TEXT("KTWModeCapture"))){CoverEntered=true;ModeChoice=1;}if(FParse::Param(FCommandLine::Get(),TEXT("KTWModeTest")))CoverEntered=true;}
bool UKTWSession::Ready()const{if(!GetWorld())return false;if(ModeChoice==1&&!InMenu)return AKTWRogueRun::Find(GetWorld())!=nullptr;for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)return I->Facades->GetGraph()&&I->Barriers->GetGraph()&&I->Weapons->GetGraph()&&I->GoldSources->GetGraph()&&I->Atmosphere->GetGraph()&&!I->IsGenerating();return false;}
void UKTWSession::NewRound(bool Menu){
 const int32 Previous=RoundSeed;RoundSeed=int32(HashCombineFast(uint32(Previous),uint32(FPlatformTime::Cycles64()))&0x7fffffff);if(RoundSeed==Previous)++RoundSeed;++RoundNumber;
 if(!Menu)Earn(ModeChoice?TEXT("rule_start"):TEXT("street_start"));Collection=false;InMenu=Menu;PausedMenu=Help=Settings=false;UGameplayStatics::SetGamePaused(GetWorld(),false);UGameplayStatics::SetGlobalTimeDilation(GetWorld(),1);UGameplayStatics::OpenLevel(GetWorld(),FName(!Menu&&ModeChoice==1?TEXT("/Game/KTW/Maps/L_RuleArena"):TEXT("/Game/KTW/Maps/L_Kowloon_ReferenceV4")));
}
void UKTWSession::Cue(const UObject* C,FName Name,float Volume,FVector P){if(auto* S=Get(C))S->Play(Name,Volume,P);}
void UKTWSession::Play(FName Name,float Volume,FVector P){
 if(!SoundEnabled||!GetWorld()||FParse::Param(FCommandLine::Get(),TEXT("nosound")))return;
 const double Now=FPlatformTime::Seconds();if(Now<NextAudio)return;NextAudio=Now+.035;
 USoundBase* Sound=Sounds.FindRef(Name);if(!Sound){const FString Path=FString::Printf(TEXT("/Game/KTW/Audio/S_%s.S_%s"),*Name.ToString(),*Name.ToString());Sound=LoadObject<USoundBase>(nullptr,*Path);if(Sound)Sounds.Add(Name,Sound);}if(!Sound)return;
 if(!P.IsNearlyZero()){auto* PC=GetWorld()->GetFirstPlayerController();if(PC&&PC->GetPawn()){const float D=FVector::Dist2D(PC->GetPawn()->GetActorLocation(),P);Volume*=FMath::Clamp(1-D/13000.f,.08f,1.f);}}
 UGameplayStatics::PlaySound2D(GetWorld(),Sound,Volume*.4f);
}
void UKTWSession::Action(const FString& Id){
 Play(TEXT("Click"));
 if(Id==TEXT("Language")){English=!English;SaveProfile();return;}if(Id==TEXT("Collection")){if(Collection){Action(TEXT("CloseCollection"));return;}Collection=true;PreviousPause=UGameplayStatics::IsGamePaused(GetWorld());if(!InMenu)UGameplayStatics::SetGamePaused(GetWorld(),true);return;}if(Id==TEXT("CloseCollection")){Collection=false;if(!InMenu)UGameplayStatics::SetGamePaused(GetWorld(),PreviousPause);return;}
 if(Id==TEXT("Play")){if(Ready())NewRound();else Notice=TEXT("Building streets. Please wait...");}
 else if(Id==TEXT("ModeStreet"))ModeChoice=0;
 else if(Id==TEXT("ModeRogue"))ModeChoice=1;
 else if(Id.StartsWith(TEXT("Difficulty")))Difficulty=FCString::Atoi(*Id.Right(1));
 else if(Id==TEXT("How")){Help=true;Settings=false;HelpPage=0;PreviousPause=UGameplayStatics::IsGamePaused(GetWorld());if(!InMenu)UGameplayStatics::SetGamePaused(GetWorld(),true);}
 else if(Id==TEXT("Next"))HelpPage=(HelpPage+1)%3;
 else if(Id==TEXT("Prev"))HelpPage=(HelpPage+2)%3;
 else if(Id==TEXT("Back")){Help=Settings=false;if(!InMenu&&!PausedMenu)UGameplayStatics::SetGamePaused(GetWorld(),PreviousPause);}
 else if(Id==TEXT("Settings")){Settings=true;Help=false;}
 else if(Id==TEXT("Sound")){SoundEnabled=!SoundEnabled;if(IsValid(Ambience))Ambience->SetPaused(!SoundEnabled);if(auto* W=AKTWWeather::Find(GetWorld()))if(IsValid(W->WeatherAudio))W->WeatherAudio->SetPaused(!SoundEnabled);}
 else if(Id.StartsWith(TEXT("Quality"))){Quality=FCString::Atoi(*Id.Right(1));if(auto* G=UGameUserSettings::GetGameUserSettings()){G->SetOverallScalabilityLevel(Quality);G->ApplyNonResolutionSettings();}}
 else if(Id==TEXT("Resume")){PausedMenu=false;UGameplayStatics::SetGamePaused(GetWorld(),false);}
 else if(Id==TEXT("Menu"))NewRound(true);
 else if(Id==TEXT("Quit"))if(auto* PC=GetWorld()->GetFirstPlayerController())PC->ConsoleCommand(TEXT("quit"));
}
bool UKTWSession::Handle(AKTWPrototypeController* PC){
 if(Diagnostic())return false;
 if(InMenu&&!CoverEntered&&!Help&&!Settings){if(CoverPressedAt>=0){if(FPlatformTime::Seconds()-CoverPressedAt>=.25){CoverEntered=true;CoverPressedAt=-1;}return true;}if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)||PC->WasInputKeyJustPressed(EKeys::Enter)||PC->WasInputKeyJustPressed(EKeys::SpaceBar)){CoverPressedAt=FPlatformTime::Seconds();Play(TEXT("Click"));}return true;}
 if(PC->WasInputKeyJustPressed(EKeys::F1))Action(TEXT("How"));
 if(PC->WasInputKeyJustPressed(EKeys::Escape)||PC->WasInputKeyJustPressed(EKeys::P)){
  if(Collection)Action(TEXT("CloseCollection"));else if(Help||Settings)Action(TEXT("Back"));else if(!InMenu){PausedMenu=!PausedMenu;UGameplayStatics::SetGamePaused(GetWorld(),PausedMenu);}
 }
 if(!InMenu&&!PausedMenu&&!Help&&!Settings&&!Collection&&PC->WasInputKeyJustPressed(EKeys::N)){NewRound();return true;}
 const auto* M=AKTWMatch::Find(GetWorld());const bool Overlay=Collection||InMenu||PausedMenu||Help||Settings||(M&&M->Phase==EKTWMatchPhase::Finished);
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)){float X,Y;if(PC->GetMousePosition(X,Y))for(const auto& B:Buttons)if(X>=B.Rect.X&&Y>=B.Rect.Y&&X<B.Rect.X+B.Rect.Z&&Y<B.Rect.Y+B.Rect.W){if(B.Enabled)Action(B.Id);return true;}}
 if(Overlay&&PC->WasInputKeyJustPressed(EKeys::Enter))Action(Collection?TEXT("CloseCollection"):Help?TEXT("Next"):PausedMenu?TEXT("Resume"):TEXT("Play"));
 if(Overlay)return true;
 int32 W,H;PC->GetViewportSize(W,H);float X,Y;if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&PC->GetMousePosition(X,Y)&&X>W-210&&Y>H-240&&Y<H-80){if(auto* Camera=Cast<AKTWCameraPawn>(PC->GetPawn())){FVector P=Camera->GetActorLocation();P.X=(X-(W-210))/190*14000-7000;P.Y=(Y-(H-240))/160*14000-7000;Camera->PanTo(P);}return true;}
 return false;
}
bool UKTWSession::Draw(AKTWPrototypeHUD* H){
 if(Diagnostic()&&!FParse::Param(FCommandLine::Get(),TEXT("KTWFrontendTest")))return false;
 auto* C=H->GameCanvas();if(!C)return false;Buttons.Reset();if(FParse::Param(FCommandLine::Get(),TEXT("KTWCollectionCapture")))Collection=true;if(!InMenu&&!PausedMenu&&!Help&&!Settings&&!Collection)return false;
 const float W=C->SizeX,V=C->SizeY,S=FMath::Clamp(FMath::Min(W/1400.f,V/850.f),.48f,1.5f),CX=W*.57f,Y=V*.22f,BW=440*S;
 if(CoverTextures.IsEmpty()){const TCHAR* Names[]={TEXT("T_Cover"),TEXT("T_Press0"),TEXT("T_Press1"),TEXT("T_Press2"),TEXT("T_Press3"),TEXT("T_Press4"),TEXT("T_Press5")};for(const auto* Name:Names)CoverTextures.Add(LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/KTW/UI/Cover/"))+Name+TEXT(".")+Name)));}
 const float ArtScale=FMath::Min(W/1672.f,V/941.f),AW=1672*ArtScale,AH=941*ArtScale,AX=(W-AW)*.5f,AY=(V-AH)*.5f;
 H->DrawRect(FLinearColor(.004f,.008f,.012f,1),0,0,W,V);
 if(CoverTextures.IsValidIndex(0)&&CoverTextures[0])H->DrawTexture(CoverTextures[0],AX,AY,AW,AH,0,0,1,1,FLinearColor::White,BLEND_Translucent);
 else KTWMistTheme::Backdrop(H,W,V,GetWorld()->GetTimeSeconds());
 if(InMenu&&!CoverEntered&&!Help&&!Settings){const int32 Frame=CoverPressedAt<0?0:FMath::Clamp(int32((FPlatformTime::Seconds()-CoverPressedAt)*24),0,5);if(CoverTextures.IsValidIndex(Frame+1)&&CoverTextures[Frame+1])H->DrawTexture(CoverTextures[Frame+1],AX+136*ArtScale,AY+818*ArtScale,1400*ArtScale,80*ArtScale,0,0,1,1,FLinearColor::White,BLEND_Translucent);return true;}
 if(Help||Settings||PausedMenu)H->DrawRect(FLinearColor(.012f,.018f,.021f,.95f),W*.53f,V*.12f,W*.44f,V*.79f);
 else H->DrawRect(FLinearColor(.012f,.018f,.021f,.94f),W*.055f,V*.49f,W*.49f,V*.45f);
 auto Button=[&](const FString& Id,const FString& Text,float X,float Z,float Width,bool Enabled=true){float MX=-1,MY=-1;H->GetOwningPlayerController()->GetMousePosition(MX,MY);const float Height=48*S;const bool Hover=MX>=X&&MX<X+Width&&MY>=Z&&MY<Z+Height;const bool Light=Enabled&&(Hover||(Id==TEXT("ModeStreet")&&ModeChoice==0)||(Id==TEXT("ModeRogue")&&ModeChoice==1)||(Id.StartsWith(TEXT("Difficulty"))&&FCString::Atoi(*Id.Right(1))==Difficulty));Paper(H,X,Z,Width,Height,Light);H->DrawRect(FLinearColor(.72f,.65f,.5f),X,Z,3*S,Height);H->DrawLine(X+8*S,Z,X+Width-8*S,Z,FLinearColor(.55f,.49f,.38f),1);H->DrawLine(X+Width-14*S,Z+Height,X+Width,Z+Height-14*S,FLinearColor(.1f,.75f,.79f),2);H->DrawText(Text,Enabled?(Light?FColor(30,27,23):FColor(239,223,193)):FColor(100,110,120),X+15*S,Z+14*S,nullptr,1.05f*S);FKTWMenuButton B;B.Id=Id;B.Text=Text;B.Rect=FVector4(X,Z,Width,Height);B.Enabled=Enabled;Buttons.Add(B);};
 Button(TEXT("Language"),TEXT("中文 / EN"),W-300*S,20*S,130*S);Button(TEXT("Collection"),English?TEXT("COLLECTION"):TEXT("成就收藏"),W-160*S,20*S,145*S);
 if(Collection){H->DrawRect(FLinearColor(.012f,.012f,.014f,.98f),W*.08f,V*.11f,W*.84f,V*.83f);H->DrawText(English?TEXT("COLLECTION / YOUR LEGACY"):TEXT("成就收藏 / 你的街区足迹"),FColor(225,204,157),W*.12f,V*.15f,nullptr,1.6f*S);const TCHAR* Ids[]={TEXT("street_start"),TEXT("rule_start"),TEXT("recruit"),TEXT("gunner"),TEXT("first_kill"),TEXT("first_wave"),TEXT("law"),TEXT("champion")};const TCHAR* CN[]={TEXT("初入街区"),TEXT("法则新兵"),TEXT("招兵买马"),TEXT("火力支援"),TEXT("初战告捷"),TEXT("突破防线"),TEXT("改写法则"),TEXT("天台之王")};const TCHAR* EN[]={TEXT("Street arrival"),TEXT("Rule recruit"),TEXT("Growing legion"),TEXT("Fire support"),TEXT("First takedown"),TEXT("Breakthrough"),TEXT("Rewrite the law"),TEXT("Rooftop king")};const TCHAR* DescCN[]={TEXT("开始一次街区争夺"),TEXT("开始一次法则挑战"),TEXT("在法则挑战中购买一名单位"),TEXT("招募一名枪手"),TEXT("在法则挑战中击败敌人"),TEXT("赢得法则挑战的一关"),TEXT("获得第一张物理法则"),TEXT("完成五关法则挑战")};const TCHAR* DescEN[]={TEXT("Start Street War"),TEXT("Start Rule Run"),TEXT("Recruit a soldier in Rule Run"),TEXT("Recruit a gunner"),TEXT("Defeat an enemy in Rule Run"),TEXT("Win a Rule Run wave"),TEXT("Choose your first physics law"),TEXT("Clear all five waves")};for(int I=0;I<8;++I){float X=W*.12f+(I%2)*W*.4f,Z=V*.24f+(I/2)*V*.14f;bool Won=Achievements.Contains(Ids[I]);Paper(H,X,Z,W*.36f,V*.115f,Won);H->DrawText(English?EN[I]:CN[I],Won?FColor(30,27,23):FColor(190,172,138),X+18,Z+12,nullptr,1.15f*S);H->DrawText(English?DescEN[I]:DescCN[I],Won?FColor(45,40,32):FColor(145,141,131),X+18,Z+42,nullptr,.86f*S);H->DrawText(Won?(English?TEXT("UNLOCKED"):TEXT("已收藏")):(English?TEXT("LOCKED"):TEXT("待解锁")),Won?FColor(95,64,20):FColor(140,119,87),X+18,Z+67,nullptr,.85f*S);}Button(TEXT("CloseCollection"),English?TEXT("BACK"):TEXT("返回"),W*.4f,V*.84f,W*.2f);return true;}
 if(Help&&ModeChoice==1){const TCHAR* Titles[]={TEXT("01 / RULE RUN / DEPLOY"),TEXT("02 / THE PHYSICS LAWS"),TEXT("03 / KEEP YOUR LEGION")};const TCHAR* Pages[]={TEXT("Start with 3 chargers, 200 gold and 3 lives.\nRecruit more soldiers before the first battle.\nLMB or RMB selects allies; RMB drag: squads.\nLMB blue floor places selected soldiers.\nKeep space between bodies when deploying.\nSPACE starts the wave. Enemies are visible.\nFive waves: wave 3 elite / wave 5 boss."),TEXT("Soldiers walk upright, strike and shoot.\nHeavy units resist knockback. Falling is fatal.\nAfter a victory choose 1 of 3 physics laws.\nGravity, ice and knockback affect BOTH teams.\nCombine laws with your roster and formation.\nRMB can move units or focus a chosen enemy.\nMiddle drag orbits, wheel zooms, HOME resets.\nTAB changes speed. ESC pauses.\n45s collapse / 65s surviving-strength judgement."),TEXT("A purchased soldier stays for this run.\nDead soldiers return at full health next wave.\nLosing costs one life; retry the same wave.\nThree lost lives ends the run.\nWinning grants gold and a new law, then shop.\nRefresh costs 20 gold. Army limit: 20.\nClear wave 5 to win. N begins a fresh run.\nReturn to main menu to select STREET WAR.")};H->DrawText(Titles[HelpPage],FColor(226,209,175),CX,Y,nullptr,1.35f*S);H->DrawText(Pages[HelpPage],FColor(215,225,232),CX,Y+56*S,nullptr,.95f*S);Button(TEXT("Prev"),TEXT("PREVIOUS"),CX,Y+344*S,BW*.48f);Button(TEXT("Next"),TEXT("NEXT"),CX+BW*.52f,Y+344*S,BW*.48f);Button(TEXT("Back"),TEXT("BACK"),CX,Y+404*S,BW);}
 else if(Help){const TCHAR* Titles[]={TEXT("01 / RECRUIT & DEPLOY"),TEXT("02 / COMMAND YOUR GANG"),TEXT("03 / GOLD, RAIDS & VICTORY")};const TCHAR* Text[]={TEXT("Start with 600 CASH and a free boss.\nZ upper / X middle / C lower lane.\n[2] Runner 80: fast gold courier\n[3] Knife 120: counters Thrower\n[4] Pipe 180: counters Knife\n[5] Thrower 220: counters Pipe\n[6] Box 130: public odds and guarantee\nPlace units near your base. SPACE starts."),TEXT("Overhead: RMB unit / drag selects. SHIFT adds.\nRight-click ground: move, then resume work.\nRight-click enemy: focus attack.\nStreet: LMB selects; RMB ally escorts.\nALT + right-click: force ground move.\nF hold / G auto-follow / B return / V raid\n1 boss / H escort courier\nTAB street / overhead. HOME frames selection.\nStreet RMB drag: look / short click: command\nWASD fly / PgUp PgDn height\nT follows / wheel: fast zoom\nF2: instant initial full map\nLook up + climb above 65m: tactical view"),TEXT("Start: three random gold bags.\nGold convoys: 1:00, 2:10, 3:20.\n30s warning. Clear guards, RMB to open.\nPick up: Runner 0.22s / others 0.42s.\nCarry home; a 1s delivery banks it.\n500 GOLD held 15s / destroy enemy van wins.\n6min limit. 4min final phase, 10s grace.\nThen boss defeat loses / all NPC damage x1.5.\nDiamond convoy at 4:30; bring diamond home.\nRMB vans/hideouts to interact; move to exit.\nRaid enemy vault: clear guards, channel 4s.\nDamage / guards interrupt a raid.\nCash is never stolen or spent as gold.\nXP 60 / 160 promotes a unit.\nP or ESC pauses. N starts a new round.")};H->DrawText(Titles[HelpPage],FColor(226,209,175),CX,Y,nullptr,1.35f*S);H->DrawText(Text[HelpPage],FColor(215,225,232),CX,Y+56*S,nullptr,.95f*S);Button(TEXT("Prev"),TEXT("PREVIOUS"),CX,Y+344*S,BW*.48f);Button(TEXT("Next"),TEXT("NEXT"),CX+BW*.52f,Y+344*S,BW*.48f);Button(TEXT("Back"),TEXT("BACK"),CX,Y+404*S,BW);}
 else if(Settings){H->DrawText(TEXT("SETTINGS"),FColor(226,209,175),CX,Y,nullptr,1.5f*S);Button(TEXT("Sound"),SoundEnabled?TEXT("SOUND / ON"):TEXT("SOUND / OFF"),CX,Y+65*S,BW);for(int32 I=0;I<3;++I)Button(FString::Printf(TEXT("Quality%d"),I),FString::Printf(TEXT("%s%s"),Quality==I?TEXT("* "):TEXT(""),I==0?TEXT("LOW"):I==1?TEXT("MEDIUM"):TEXT("HIGH")),CX+I*BW/3,Y+132*S,BW/3-6*S);Button(TEXT("Back"),TEXT("BACK"),CX,Y+210*S,BW);}
 else if(PausedMenu){H->DrawText(TEXT("PAUSED"),FColor(226,209,175),CX,Y,nullptr,1.7f*S);Button(TEXT("Resume"),TEXT("RESUME"),CX,Y+65*S,BW);Button(TEXT("How"),TEXT("HOW TO PLAY"),CX,Y+125*S,BW);Button(TEXT("Settings"),TEXT("SETTINGS"),CX,Y+185*S,BW);Button(TEXT("Menu"),TEXT("RETURN TO MAIN MENU"),CX,Y+245*S,BW);}
 else{const float BX=W*.075f,BWidth=W*.45f;H->DrawText(TEXT("SELECT YOUR BATTLE"),FColor(239,223,193),BX,V*.515f,nullptr,.85f*S);
  Button(TEXT("ModeStreet"),TEXT("STREET WAR / 街区争夺"),BX,V*.55f,BWidth*.49f);Button(TEXT("ModeRogue"),TEXT("RULE RUN / 法则挑战"),BX+BWidth*.51f,V*.55f,BWidth*.49f);H->DrawRect(FLinearColor(.1f,.8f,.8f),BX+(ModeChoice?BWidth*.51f:0),V*.55f+48*S-3,BWidth*.49f,3);
  H->DrawText(ModeChoice?TEXT("5 WAVES / DRAFT PHYSICS LAWS / BUILD YOUR LEGION"):TEXT("6 MINUTES / COMMAND YOUR GANG / GOLD & BASE SIEGE"),FColor(204,207,189),BX,V*.62f,nullptr,.69f*S);
  for(int32 I=0;I<3;++I)Button(FString::Printf(TEXT("Difficulty%d"),I),FString::Printf(TEXT("%s%s"),Difficulty==I?TEXT("* "):TEXT(""),I==0?TEXT("EASY"):I==1?TEXT("NORMAL"):TEXT("HARD")),BX+I*BWidth/3,V*.67f,BWidth/3-6*S);
  Button(TEXT("Play"),Ready()?(ModeChoice?TEXT("START RULE RUN / 开始挑战"):TEXT("ENTER THE STREETS / 开始争夺")):TEXT("BUILDING STREETS..."),BX,V*.745f,BWidth,Ready());const TCHAR* Ids[]={TEXT("How"),TEXT("Settings"),TEXT("Quit")};const TCHAR* Labels[]={TEXT("HOW TO PLAY"),TEXT("SETTINGS"),TEXT("QUIT")};for(int32 I=0;I<3;++I)Button(Ids[I],Labels[I],BX+I*BWidth/3,V*.82f,BWidth/3-6*S);H->DrawText(TEXT("TWO MODES / ONE CITY / YOUR RULES"),FColor(197,184,161),BX,V*.9f,nullptr,.65f*S);}

 return true;
}
void UKTWSession::DrawOverlay(AKTWPrototypeHUD* H){
 auto* C=H->GameCanvas();if(!C||Diagnostic()||C->SizeY<650)return;if(auto* W=AKTWWeather::Find(GetWorld()))H->DrawText(FString::Printf(TEXT("WEATHER / %s"),W->Name(W->Preset)),FColor(145,200,220),20,280,nullptr,.85f);const float X=C->SizeX-210,Y=C->SizeY-240;
 H->DrawRect(FLinearColor(.015f,.03f,.045f,.92f),X-5,Y-20,200,184);H->DrawText(TEXT("TACTICAL MAP / click to pan"),FColor(150,180,190),X,Y-16,nullptr,.65f);
 auto P=[&](FVector V){return FVector2D(X+(V.X+7000)/14000*190,Y+(V.Y+7000)/14000*160);};auto Nodes=AKTWStreetDirector::Junctions();for(auto Edge:TArray<FIntPoint>{{0,1},{1,4},{0,2},{2,4},{0,3},{3,4},{1,3}}){const auto A=P(Nodes[Edge.X]),B=P(Nodes[Edge.Y]);H->DrawLine(A.X,A.Y,B.X,B.Y,FLinearColor(.2f,.35f,.4f),2);}
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Health>0){auto V=P(I->GetActorLocation());H->DrawRect(I->Faction?FLinearColor(.95f,.24f,.17f):FLinearColor(.18f,.65f,1),V.X-2,V.Y-2,4,4);}
 auto* M=AKTWMatch::Find(GetWorld());if(M){const FString Hint=M->Phase==EKTWMatchPhase::Preparation?(M->Population(0)?TEXT("READY / SPACE TO START"):TEXT("RECRUIT / PRESS 2 - 5")):M->Phase==EKTWMatchPhase::Battle?TEXT("BANK GOLD / PROTECT COURIERS"):TEXT("MATCH COMPLETE / N NEW ROUND");H->DrawText(Hint,FColor(226,209,175),20,235,nullptr,.9f);H->DrawText(FString::Printf(TEXT("+30 CASH / 10s     %s AI     Seed %d     F1 Help / ESC Menu"),Difficulty==0?TEXT("EASY"):Difficulty==2?TEXT("HARD"):TEXT("NORMAL"),RoundSeed),FColor(155,183,198),20,258,nullptr,.75f);}
}
void UKTWSession::RunFrontendTest(){
#if !UE_BUILD_SHIPPING
 static int32 Stage=0,PreviousSeed=0,Failures=0;
 auto Check=[&](bool OK,const TCHAR* N){Failures+=!OK;UE_LOG(LogTemp,Display,TEXT("KTW FRONTEND %s: %s"),N,OK?TEXT("PASS"):TEXT("FAIL"));};
 Check(Ready(),TEXT("all five assigned PCG graphs completed"));
 for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)Check(I->MapSeed==RoundSeed&&I->ValidateLayout(),TEXT("session seed and route constraints"));
 Check(AKTWMatch::Find(GetWorld())->Phase==EKTWMatchPhase::Preparation,TEXT("menu never starts economy"));
 if(Stage==0){Check(InMenu,TEXT("starts at main menu"));Action(TEXT("Difficulty0"));Check(Difficulty==0&&DecisionSeconds()==6,TEXT("easy reaction"));Action(TEXT("Difficulty2"));Check(Difficulty==2&&DecisionSeconds()==1.5f,TEXT("hard reaction"));Action(TEXT("How"));Action(TEXT("Next"));Check(Help&&HelpPage==1,TEXT("tutorial pages navigate"));Action(TEXT("Back"));Check(!Help&&InMenu,TEXT("tutorial returns to menu"));PreviousSeed=RoundSeed;Stage=1;Action(TEXT("Play"));return;}
 Check(!InMenu&&RoundNumber==Stage,TEXT("new round exits menu and reloads map"));Check(RoundSeed!=PreviousSeed,TEXT("fresh seed on consecutive rounds"));Check(Difficulty==2,TEXT("difficulty survives level reload"));
 if(Stage==1){PreviousSeed=RoundSeed;++Stage;NewRound();return;}
 UE_LOG(LogTemp,Display,TEXT("KTW FRONTEND COMPLETE failures=%d rounds=%d"),Failures,RoundNumber);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
#endif
}
