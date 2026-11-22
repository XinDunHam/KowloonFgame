#include "KTWRogue.h"
#include "KTWPrototype.h"
#include "KTWHoloHUD.h"
#include "KTWSession.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void AKTWRogueRun::Action(const FString& Id){auto* S=UKTWSession::Get(this);if(Id==TEXT("Start")){StartBattle();return;}if(Id==TEXT("Auto")){AutoDeploy();return;}if(Id==TEXT("Draft")&&Phase==EKTWRoguePhase::Result&&Won){DraftRules();return;}if(Id==TEXT("Retry")){Retry();return;}if(Id==TEXT("Next")){NextWave();return;}if(Id==TEXT("Refresh")){RefreshShop(true);return;}if(Id==TEXT("Restart")){if(S)S->NewRound();return;}if(Id==TEXT("Menu")){if(S)S->Action(TEXT("Menu"));return;}if(Id==TEXT("Speed")){Speed=Speed==1?2:Speed==2?.5f:1;UGameplayStatics::SetGlobalTimeDilation(GetWorld(),Speed);return;}if(Id.StartsWith(TEXT("Buy"))){const int I=FCString::Atoi(*Id.Mid(3));Buy(I);return;}if(Id.StartsWith(TEXT("Card"))&&Phase==EKTWRoguePhase::Draft){const int I=FCString::Atoi(*Id.Mid(4));if(Choices.Contains(I)&&!ActiveCards.Contains(I)){ActiveCards.Add(I);if(S)S->Earn(TEXT("law"));RebuildRules();Phase=EKTWRoguePhase::Shop;RefreshShop(false);}return;}}
void AKTWRogueRun::UpdateCamera(AKTWCameraPawn* Cam,float D){auto* PC=Cast<APlayerController>(Cam->GetController());if(!PC)return;const FRotator Rot(CameraPitch,CameraYaw,0);const FVector Forward=FRotator(0,CameraYaw,0).Vector(),Right=FRotator(0,CameraYaw+90,0).Vector();if(PC->IsInputKeyDown(EKeys::W))CameraFocus+=Forward*D*1000;if(PC->IsInputKeyDown(EKeys::S))CameraFocus-=Forward*D*1000;if(PC->IsInputKeyDown(EKeys::D))CameraFocus+=Right*D*1000;if(PC->IsInputKeyDown(EKeys::A))CameraFocus-=Right*D*1000;CameraFocus.X=FMath::Clamp(CameraFocus.X,-1800.,1800.);CameraFocus.Y=FMath::Clamp(CameraFocus.Y,-1000.,1000.);if(PC->IsInputKeyDown(EKeys::MiddleMouseButton)){float X,Y;PC->GetInputMouseDelta(X,Y);CameraYaw+=X*.3f;CameraPitch=FMath::Clamp(CameraPitch-Y*.25f,-82.f,-30.f);}if(PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))CameraDistance*=.88f;if(PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))CameraDistance*=1.14f;CameraDistance=FMath::Clamp(CameraDistance,2300.f,6500.f);if(PC->WasInputKeyJustPressed(EKeys::Home)){CameraFocus=FVector::ZeroVector;CameraDistance=4600;CameraYaw=-90;CameraPitch=-58;}Cam->SetActorLocation(CameraFocus-Rot.Vector()*CameraDistance);Cam->Camera->SetRelativeRotation(Rot);Cam->Camera->FieldOfView=58;}
void AKTWRogueRun::CommandSelected(FVector Point,AKTWRogueUnit* Enemy){
 TArray<AKTWRogueUnit*> Squad;float Spacing=110;
 for(auto Weak:Selected)if(Weak.IsValid()&&Weak->Alive&&Weak->Team==0){Squad.Add(Weak.Get());Spacing=FMath::Max(Spacing,80*Weak->Stats.Scale);}
 if(Squad.IsEmpty())return;
 TArray<FVector> Goals;const int Columns=FMath::Min(3,Squad.Num()),Rows=FMath::DivideAndRoundUp(Squad.Num(),Columns);
 for(int I=0;I<Squad.Num();++I){FVector Goal=Point;if(Squad.Num()>1)Goal+=FVector((I%Columns-(Columns-1)*.5f)*Spacing,(I/Columns-(Rows-1)*.5f)*Spacing,0);if(Phase==EKTWRoguePhase::Battle){Goal.X=FMath::Clamp(Goal.X,-1700.,1700.);Goal.Y=FMath::Clamp(Goal.Y,-800.,800.);}Goals.Add(Goal);}
 if(Phase==EKTWRoguePhase::Deployment){if(!PlaceSquad(Squad,Goals))Notice=TEXT("请放在蓝方区域内，并与其他单位留出空间；本次位置没有改变。");return;}
 if(Phase==EKTWRoguePhase::Battle)for(int I=0;I<Squad.Num();++I){if(Enemy&&Enemy->Alive&&Enemy->Team!=0)Squad[I]->OrderAttack(Enemy);else Squad[I]->OrderMove(Goals[I]);}
}
AKTWRogueUnit* AKTWRogueRun::PickUnit(APlayerController* PC,FVector2D Cursor)const{
 AKTWRogueUnit* Picked=nullptr;float Best=22;
 for(auto Unit:Units)if(Unit&&Unit->Alive){
  FVector2D Bottom,Top;const float Height=88*Unit->Stats.Scale*Unit->SizeFactor;
  if(!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation()-FVector(0,0,Height*.9f),Bottom)||!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation()+FVector(0,0,Height),Top))continue;
  const FVector2D Segment=Top-Bottom;const float Fraction=FMath::Clamp(FVector2D::DotProduct(Cursor-Bottom,Segment)/FMath::Max(1.,Segment.SizeSquared()),0.,1.);
  const float Distance=FVector2D::Distance(Cursor,Bottom+Segment*Fraction);if(Distance<Best){Best=Distance;Picked=Unit;}
 }
 return Picked;
}
void AKTWRogueRun::ClickWorld(APlayerController* PC,FVector2D Cursor,bool Primary,bool Add){
 if(Phase!=EKTWRoguePhase::Deployment&&Phase!=EKTWRoguePhase::Battle)return;
 auto* Picked=PickUnit(PC,Cursor);
 if(Picked&&Picked->Team==0){if(!Add)Selected.Empty();Selected.AddUnique(Picked);Notice=FString::Printf(TEXT("已选中 %s。%s"),*Picked->Stats.Name,Phase==EKTWRoguePhase::Deployment?TEXT("点击蓝方地面调整位置。"):TEXT("右键地面移动，右键敌人集火。"));return;}
 if(Primary&&Phase==EKTWRoguePhase::Battle){if(!Add)Selected.Empty();return;}
 if(Picked&&Phase==EKTWRoguePhase::Deployment){Notice=TEXT("敌方阵容可提前观察，只能调整己方单位。");return;}
 FVector Origin,Direction;if(!PC->DeprojectScreenPositionToWorld(Cursor.X,Cursor.Y,Origin,Direction)||Direction.Z>=-.01)return;
 const FVector Point=Origin+Direction*(-Origin.Z/Direction.Z);if(FMath::Abs(Point.X)>1800||FMath::Abs(Point.Y)>900)return;
 CommandSelected(Point,Picked&&Picked->Team==1?Picked:nullptr);
}
void AKTWRogueRun::Handle(AKTWPrototypeController* PC){
 float X=0,Y=0;PC->GetMousePosition(X,Y);const FVector2D Cursor(X,Y);int32 W,H;PC->GetViewportSize(W,H);
 if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)&&Phase==EKTWRoguePhase::Deployment)StartBattle();
 if(PC->WasInputKeyJustPressed(EKeys::Tab)&&Phase==EKTWRoguePhase::Battle)Action(TEXT("Speed"));
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))for(const auto& Button:Buttons)if(X>=Button.Rect.X&&Y>=Button.Rect.Y&&X<Button.Rect.X+Button.Rect.Z&&Y<Button.Rect.Y+Button.Rect.W){if(Button.Enabled)Action(Button.Id);return;}
 if(Phase!=EKTWRoguePhase::Deployment&&Phase!=EKTWRoguePhase::Battle)return;
 const float Scale=FMath::Clamp(FMath::Min(W/1600.f,H/900.f),.65f,1.4f);const bool InWorld=X>16+250*Scale&&X<W-270*Scale&&Y>105&&Y<H-95;
 const bool Add=PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift);
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&InWorld){ClickWorld(PC,Cursor,true,Add);return;}
 if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton)&&InWorld){SelectionStart=Cursor;Selecting=true;}
 if(PC->WasInputKeyJustReleased(EKeys::RightMouseButton)&&Selecting){
  Selecting=false;if(!InWorld)return;
  if(FVector2D::Distance(Cursor,SelectionStart)>12){if(!Add)Selected.Empty();FBox2D Box(ForceInit);Box+=SelectionStart;Box+=Cursor;for(auto Unit:Units)if(Unit&&Unit->Alive&&Unit->Team==0){FVector2D Point;if(PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(),Point)&&Box.IsInside(Point))Selected.AddUnique(Unit);}return;}
  ClickWorld(PC,Cursor,false,Add);
 }
}
void AKTWRogueRun::Draw(AKTWPrototypeHUD* H){auto* Canvas=H->GameCanvas();if(!Canvas)return;Buttons.Empty();const float W=Canvas->SizeX,V=Canvas->SizeY,S=FMath::Clamp(FMath::Min(W/1600.f,V/900.f),.65f,1.4f);const FColor White(228,233,228),Muted(153,178,176),Gold(240,204,125);const FLinearColor Cyan(.15f,.75f,.78f),Amber(.9f,.62f,.2f);
 auto Panel=[&](float X,float Y,float Width,float Height){H->DrawRect(FLinearColor(.012f,.025f,.03f,.94f),X,Y,Width,Height);H->DrawLine(X,Y,X+Width,Y,FLinearColor(.48f,.54f,.48f),1);H->DrawRect(Cyan,X,Y,3,Height);};
 auto Text=[&](FString T,float X,float Y,float Size=1.f,FColor Color=FColor(228,233,228)){H->DrawText(T,Color,X,Y,nullptr,Size*1.25f*S);};
 auto Button=[&](FString Id,FString Label,float X,float Y,float Width,bool Enabled=true,int Icon=-1){float MX,MY;H->GetOwningPlayerController()->GetMousePosition(MX,MY);const bool Hover=MX>=X&&MX<X+Width&&MY>=Y&&MY<Y+44*S;UKTWSession::Paper(H,X,Y,Width,44*S,Hover&&Enabled);H->DrawRect(Enabled?Cyan:FLinearColor(.15f,.17f,.16f),X,Y,3,44*S);if(Icon>=0)if(auto* SharedHUD=Cast<AKTWHoloHUD>(H))SharedHUD->RoleIcon(X+12*S,Y+9*S,26*S,Icon,Enabled?Cyan:FLinearColor(.2f,.3f,.3f));Text(Label,X+(Icon>=0?50:12)*S,Y+13*S,.78f,Enabled?(Hover?FColor(30,27,23):White):Muted);Buttons.Add({Id,FVector4(X,Y,Width,44*S),Enabled});};
 auto Wrap=[&](const FString& T,float X,float Y,float Width){const auto* Profile=UKTWSession::Get(this);const FString Value=Profile?Profile->Localize(T):T;FString Line;for(TCHAR Character:Value){float TW=0,TH=0;const FString Candidate=Line+FString::Chr(Character);H->GetTextSize(Candidate,TW,TH,nullptr,FMath::Max(.72f*1.25f*S*1.12f,.82f));if(Character==TCHAR('\n')||TW>Width){Text(Line,X,Y,.72f,Muted);Y+=22*S;Line.Empty();if(Character==TCHAR('\n'))continue;}Line.AppendChar(Character);}if(!Line.IsEmpty())Text(Line,X,Y,.72f,Muted);};

 Panel(16,14,W-32,78*S);Text(TEXT("KOWLOON / RULE RUN"),32,25,.8f,Gold);Text(FString::Printf(TEXT("WAVE %d / %d  -  %s"),Wave+1,Waves.Num(),Waves.IsValidIndex(Wave)?*Waves[Wave].Title:TEXT("CONFIG ERROR")),W*.30f,25,.9f);Text(Phase==EKTWRoguePhase::Battle?FString::Printf(TEXT("%.0fs / %.0fs   %.1fx"),Elapsed,Deadline,Speed):Phase==EKTWRoguePhase::Deployment?TEXT("DEPLOYMENT / PLAN YOUR FORMATION"):TEXT("BUILD YOUR NEXT ADVANTAGE"),W*.30f,53,.72f,Muted);
 Panel(16,110,250*S,440*S);Text(FString::Printf(TEXT("GOLD  %d"),Coins),30,128,1.25f,Gold);Text(FString::Printf(TEXT("LEGION LIVES  %d / %d"),Lives,StartingLives),30,168,.85f);Text(FString::Printf(TEXT("ROSTER  %d / %d"),Roster.Num(),MaxRoster),30,196,.8f,Muted);Text(TEXT("ACTIVE PHYSICS RULES"),30,239,.78f,Gold);if(ActiveCards.IsEmpty())Text(TEXT("No rules yet. Win a wave."),30,274,.67f,Muted);for(int I=0;I<ActiveCards.Num();++I){Text(CardTypes[ActiveCards[I]].Name,30,274+I*43*S,.76f);Text(TEXT("ACTIVE FOR THIS RUN"),30,292+I*43*S,.55f,Muted);}Text(FString::Printf(TEXT("ALLIES %d  /  ENEMIES %d"),Living(0),Living(1)),30,500*S,.73f);
 const float RX=W-270*S;Panel(RX,110,254*S,500*S);Text(Phase==EKTWRoguePhase::Deployment?TEXT("招募兵种 / 本局保留"):TEXT("战术指挥"),RX+12,128,.76f,Gold);
 if(Phase==EKTWRoguePhase::Deployment){
  Text(TEXT("点击单位，再点地面布阵"),RX+12,160,.65f,Muted);
  const int Types[]={0,1,2,3,5};
  for(int I=0;I<5;++I){const int Type=Types[I];const auto& Soldier=SoldierTypes[Type];const float Y=186+I*62*S;Button(FString::Printf(TEXT("Buy%d"),Type),FString::Printf(TEXT("%s / %d 金币"),*Soldier.Name,Soldier.Cost),RX+12,Y,228*S,Coins>=Soldier.Cost&&Roster.Num()<MaxRoster,Soldier.Visual);Text(FString::Printf(TEXT("HP %.0f / HIT %.0f / RNG %.0fm"),Soldier.HP,Soldier.Damage,Soldier.Range/100),RX+20,Y+46*S,.53f,Muted);}
  Button(TEXT("Auto"),TEXT("一键恢复默认阵型"),RX+12,510*S,228*S);
 }else{
  Wrap(TEXT("LMB / RMB ally: select. RMB drag: squad. RMB enemy: focus attack. RMB ground: move and hold. SHIFT adds allies."),RX+12,175,226*S);
  Wrap(TEXT("Middle drag: orbit. WASD: pan. Wheel: zoom. HOME: reset camera. TAB: speed. ESC: pause/menu."),RX+12,300*S,226*S);Button(TEXT("Speed"),FString::Printf(TEXT("SIMULATION / %.1fx"),Speed),RX+12,465*S,228*S);
 }
 if(Waves.IsValidIndex(Wave)){Text(FString::Printf(TEXT("敌方阵型：%s"),*Waves[Wave].Formation),RX+12,572*S,.65f,Gold);Text(FString::Printf(TEXT("%d 人 / 战力预算 %d"),Waves[Wave].Soldiers.Num(),EnemyBudget(Wave)),RX+12,596*S,.58f,Muted);}
 if(!Selected.IsEmpty()&&Selected[0].IsValid()){const auto* Unit=Selected[0].Get();Text(FString::Printf(TEXT("已选 %d 人 / %s"),Selected.Num(),*Unit->Stats.Name),32,575*S,.73f,Gold);Text(Phase==EKTWRoguePhase::Deployment?TEXT("点击蓝方地面调整站位"):TEXT("右键地面或敌人下达命令"),32,600*S,.65f,Muted);}
 auto* PC=H->GetOwningPlayerController();for(auto U:Units)if(U&&U->Alive){FVector2D P;if(PC->ProjectWorldLocationToScreen(U->GetActorLocation()+FVector(0,0,100*U->Stats.Scale*U->SizeFactor),P)){H->DrawRect(FLinearColor(.01f,.02f,.025f,.9f),P.X-24,P.Y-7,48,6);H->DrawRect(U->Team?FLinearColor(.95f,.25f,.16f):Cyan,P.X-23,P.Y-6,46*FMath::Clamp(U->Health/U->MaxHealth,0.f,1.f),4);if(Selected.Contains(U)){H->DrawLine(P.X-27,P.Y+2,P.X+27,P.Y+2,Cyan,2);Text(TEXT("SELECTED"),P.X-24,P.Y+5,.45f);}}}
 if(Selecting){float X,Y;PC->GetMousePosition(X,Y);H->DrawLine(SelectionStart.X,SelectionStart.Y,X,SelectionStart.Y,Cyan);H->DrawLine(X,SelectionStart.Y,X,Y,Cyan);H->DrawLine(X,Y,SelectionStart.X,Y,Cyan);H->DrawLine(SelectionStart.X,Y,SelectionStart.X,SelectionStart.Y,Cyan);}
 Panel(16,V-82,W-32,65);Text(Notice,32,V-68,.75f,Muted);Text(TEXT("左键选兵 / 战前点击地面布阵   |   右键框选与战中指挥   |   中键旋转   |   掉落场外淘汰"),32,V-43,.6f,Gold);if(Phase==EKTWRoguePhase::Deployment)Button(TEXT("Start"),TEXT("空格 / 开始本关战斗"),W*.5f-150*S,V-143*S,300*S);
 if(Phase==EKTWRoguePhase::Battle&&Elapsed>=CollapseAt){Panel(W*.5f-245,105,490,47);Text(TEXT("FIELD COLLAPSING / RETURN TO THE CENTRE"),W*.5f-232,119,.8f,FColor(255,130,95));}
 const bool Modal=Phase==EKTWRoguePhase::Result||Phase==EKTWRoguePhase::Draft||Phase==EKTWRoguePhase::Shop||Phase==EKTWRoguePhase::Victory||Phase==EKTWRoguePhase::GameOver;if(!Modal)return;Buttons.Empty();
 H->DrawRect(FLinearColor(0,0,0,.72f),0,100,W,V-100);const float X=W*.19f,Y=V*.24f,Width=W*.62f;Panel(X,Y,Width,V*.56f);
 if(Phase==EKTWRoguePhase::Draft){Text(TEXT("CHOOSE ONE LAW / IT CHANGES THE BATTLE"),X+24,Y+24,1.1f,Gold);const float CardW=(Width-80)/3;for(int I=0;I<Choices.Num();++I){const auto& C=CardTypes[Choices[I]];const float CX=X+20+I*(CardW+20);Panel(CX,Y+90,CardW,220*S);Text(C.Name,CX+12,Y+108,.8f,Gold);Wrap(C.Description,CX+12,Y+151,CardW-24);Button(FString::Printf(TEXT("Card%d"),Choices[I]),TEXT("TAKE THIS RULE"),CX+12,Y+262*S,CardW-24);}}
 else if(Phase==EKTWRoguePhase::Shop){Text(FString::Printf(TEXT("BARRACKS / GOLD %d / ROSTER %d"),Coins,Roster.Num()),X+24,Y+24,1.1f,Gold);for(int I=0;I<Shop.Num();++I){const auto& U=SoldierTypes[Shop[I]];Button(FString::Printf(TEXT("Buy%d"),Shop[I]),FString::Printf(TEXT("%s / %d GOLD"),*U.Name,U.Cost),X+24,Y+84+I*53*S,Width-48,Coins>=U.Cost&&Roster.Num()<MaxRoster,U.Visual);}Button(TEXT("Refresh"),FString::Printf(TEXT("REFRESH / %d GOLD"),RefreshCost),X+24,Y+325*S,Width*.43f,Coins>=RefreshCost);Button(TEXT("Next"),TEXT("DEPLOY NEXT WAVE"),X+Width*.51f,Y+325*S,Width*.45f);}
 else{const FString Title=Phase==EKTWRoguePhase::Victory?TEXT("FIVE WAVES CLEARED"):Phase==EKTWRoguePhase::GameOver?TEXT("THE RUN IS OVER"):Won?TEXT("WAVE VICTORY"):TEXT("LEGION LIFE LOST");Text(Title,X+30,Y+32,1.6f,Gold);Wrap(Notice,X+30,Y+106,Width-60);Text(FString::Printf(TEXT("GOLD %d   /   LIVES %d   /   ROSTER %d"),Coins,Lives,Roster.Num()),X+30,Y+188,.95f);if(Phase==EKTWRoguePhase::Result)Button(Won?TEXT("Draft"):TEXT("Retry"),Won?TEXT("CHOOSE A PHYSICS RULE"):TEXT("RETRY / RESTORE YOUR ROSTER"),X+30,Y+260*S,Width-60);else Button(TEXT("Restart"),TEXT("START ANOTHER RULE RUN"),X+30,Y+245*S,Width-60);Button(TEXT("Menu"),TEXT("MODE SELECT / MAIN MENU"),X+30,Y+325*S,Width-60);}
}
