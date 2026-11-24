#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "KTWSession.generated.h"
class AKTWPrototypeHUD;
class AKTWPrototypeController;
class USoundBase;
class UAudioComponent;
USTRUCT() struct FKTWMenuButton {GENERATED_BODY() FString Id;FString Text;FVector4 Rect;bool Enabled=true;};
/** Local front end and per-round settings. No economy or unit attributes are stored here. */
UCLASS() class KOWLOONFGAME_API UKTWSession:public UGameInstance {
 GENERATED_BODY()
public:
 UKTWSession();
 UPROPERTY(Transient) bool English=false;
 UPROPERTY(Transient) bool Collection=false;
 UPROPERTY(Transient) TSet<FString> Achievements;
 FString Localize(FString Text)const;void LoadProfile();void SaveProfile();void Earn(const FString& Id);
 UPROPERTY(Transient) FString ProfileOverride;
 void RunUIProfileTest();
 static void Paper(AKTWPrototypeHUD* H,float X,float Y,float W,float Height,bool Light);
 virtual void Init()override;
 static UKTWSession* Get(const UObject* Context);
 static bool Diagnostic();
 UPROPERTY(VisibleAnywhere) int32 Difficulty=1;
 UPROPERTY(VisibleAnywhere) int32 ModeChoice=0; // 0 street war, 1 standalone physics rule run
 UPROPERTY(VisibleAnywhere) int32 RoundSeed=5990;
 UPROPERTY(VisibleAnywhere) int32 RoundNumber=0;
 UPROPERTY(VisibleAnywhere) bool InMenu=true;
 UPROPERTY(VisibleAnywhere) bool PausedMenu=false;
 UPROPERTY(VisibleAnywhere) bool Help=false;
 UPROPERTY(VisibleAnywhere) bool Settings=false;
 UPROPERTY(VisibleAnywhere) bool SoundEnabled=true;
 UPROPERTY(VisibleAnywhere) int32 Quality=2;
 UPROPERTY(VisibleAnywhere) int32 HelpPage=0;
 UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Sounds;
 UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
 TArray<FKTWMenuButton> Buttons;
 double NextAudio=0;
 bool PreviousPause=false;
 FString Notice;
 float DecisionSeconds()const{return Difficulty==0?6.f:Difficulty==2?1.5f:3.f;}
 UPROPERTY() TArray<TObjectPtr<class UTexture2D>> CoverTextures;
 bool CoverEntered=false;double CoverPressedAt=-1;
 bool Handle(AKTWPrototypeController* PC);
 bool Draw(AKTWPrototypeHUD* HUD);
 void DrawOverlay(AKTWPrototypeHUD* HUD);
 void Action(const FString& Id);
 void NewRound(bool Menu=false);void DrawGuide(class AKTWPrototypeHUD* H);void DrawHome(AKTWPrototypeHUD* H);void RunHomeTests();
 UPROPERTY() bool DifficultyOpen=false;
 bool Ready()const;
 void Play(FName Cue,float Volume=1,FVector Point=FVector::ZeroVector);
 static void Cue(const UObject* Context,FName Name,float Volume=1,FVector Point=FVector::ZeroVector);
 void RunFrontendTest();
 void RunModeTest();
 int32 ModeTestStage=0,ModeTestFailures=0;
};
