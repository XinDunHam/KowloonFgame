#include "KTWGold.h"
#include "KTWStreetWar.h"
#include "KTWRunnerVisuals.h"
#include "KTWSession.h"
#include "KTWPresentation.h"
#include "KTWTactics.h"
#include "KTWMatch.h"
#include "KTWGeneration.h"
#include "KTWCombatAction.h"
#include "AIController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Engine/DamageEvents.h"
#include "Containers/Ticker.h"
#include "PCGComponent.h"

AKTWGoldPile::AKTWGoldPile(){
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Marker=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceMarker"));Marker->SetupAttachment(RootComponent);
 Bars=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GoldBars"));Bars->SetupAttachment(RootComponent);
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));Bars->SetStaticMesh(Cube);Marker->SetStaticMesh(Cube);
 for(auto* C:{static_cast<UStaticMeshComponent*>(Bars.Get()),Marker.Get()}){C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);}
 Marker->SetRelativeScale3D(FVector(3.5f,3.5f,.05f));Marker->SetRelativeLocation(FVector(0,0,6));Marker->SetRelativeRotation(FRotator(0,45,0));
 if(auto* Gold=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/KTW/Materials/M_NeonGold.M_NeonGold"))){Bars->SetMaterial(0,Gold);Marker->SetMaterial(0,Gold);}
}
void AKTWGoldPile::UpdateVisuals(){Bars->ClearInstances();Marker->SetVisibility(SourceIndex>=0);
 const int32 Count=FMath::Min(12,FMath::DivideAndRoundUp(Amount,5));for(int32 I=0;I<Count;++I)Bars->AddInstance(FTransform(FRotator(0,(I/6)%2?90:0,0),FVector((I%3-1)*95,(I/3%2-.5f)*120,35+I/6*55),FVector(.85f,1.5f,.5f)));
}
AKTWGoldSystem::AKTWGoldSystem(){PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f;PrimaryActorTick.TickGroup=TG_PostUpdateWork;}
AKTWGoldSystem* AKTWGoldSystem::Find(const UWorld* W){if(W)for(TActorIterator<AKTWGoldSystem>I(W);I;++I)return *I;return nullptr;}
void AKTWGoldSystem::BeginPlay(){Super::BeginPlay();if(UKTWSession::Diagnostic()&&!FParse::Param(FCommandLine::Get(),TEXT("KTWRevisionTest"))){const auto Points=AKTWStreetDirector::GoldAnchors();for(int32 I=0;I<Points.Num();++I){auto* P=GetWorld()->SpawnActor<AKTWGoldPile>(Points[I],FRotator::ZeroRotator);if(!P)continue;P->Lane=I/2;P->SourceIndex=I;P->Capacity=P->Lane==1?45:30;Piles.Add(P);P->UpdateVisuals();}SourcesReady=true;}
 else {FTimerHandle Init;GetWorldTimerManager().SetTimer(Init,FTimerDelegate::CreateWeakLambda(this,[this](){for(TActorIterator<AKTWStreetDirector>I(GetWorld());I;++I)if(!SourcesReady&&!I->GoldSources->IsGenerating())I->GoldSources->GenerateLocal(true);}),.25f,false);}

 for(int32 F=0;F<2;++F)for(int32 L=0;L<3;++L){auto* V=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(V);V->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));V->SetWorldLocation(UKTWGoldCarrier::VaultPosition(F,L)+FVector(0,0,100));V->SetWorldScale3D(FVector(2.2f,2.2f,2));V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->SetCanEverAffectNavigation(false);V->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,F?TEXT("/Game/KTW/Materials/M_Red.M_Red"):TEXT("/Game/KTW/Materials/M_Blue.M_Blue")));V->RegisterComponent();}
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWPhase3Test"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWGoldSystem::RunPhase3Tests,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWGoldTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWGoldSystem::RunGoldTests,2.f,false);}
 if(FParse::Param(FCommandLine::Get(),TEXT("KTWGoldAutoTest"))){FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&AKTWGoldSystem::RunAutomaticTransportTest,2.f,false);}
#endif
}
void AKTWGoldSystem::CreatePCGSources(const TArray<FKTWPlacement>& Layout){
 if(AKTWStreetWar::Enabled(GetWorld())){SourcesReady=true;return;}
 if(SourcesReady||Layout.Num()!=6)return;
 // Fixed diagnostic fixtures keep their old lane-specific sources; real rounds use neutral PCG sources.
 if(UKTWSession::Diagnostic()&&!FParse::Param(FCommandLine::Get(),TEXT("KTWRevisionTest")))return;
 int32 Index=0;for(const auto& V:Layout){auto* P=GetWorld()->SpawnActor<AKTWGoldPile>(V.Transform.GetLocation(),FRotator::ZeroRotator);if(!P)continue;P->Lane=-1;P->SourceIndex=Index++;P->Capacity=(P->SourceIndex==2||P->SourceIndex==3)?45:30;Piles.Add(P);P->UpdateVisuals();}
 SourcesReady=Index==6;UE_LOG(LogTemp,Display,TEXT("KTW PCG GOLD sources=%d transverse-link=6"),Index);
}
int32 AKTWGoldSystem::GroundGold()const{int32 N=0;for(AKTWGoldPile* P:Piles)if(IsValid(P))N+=P->Amount;for(const auto& D:PendingDrops)N+=D.Amount;return N;}
int32 AKTWGoldSystem::CarriedGold(int32 Faction)const{int32 N=0;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(Faction<0||I->Faction==Faction)if(auto* C=I->FindComponentByClass<UKTWGoldCarrier>())N+=C->Carried;return N;}
bool AKTWGoldSystem::GoldConserved()const{auto* M=AKTWMatch::Find(GetWorld());if(!M)return GeneratedGold==0;return GeneratedGold==M->Accounts[0].Gold+M->Accounts[1].Gold+GroundGold()+CarriedGold();}
void AKTWGoldSystem::Tick(float D){Super::Tick(D);auto* M=AKTWMatch::Find(GetWorld());if(!M||!M->CanAct()||UGameplayStatics::IsGamePaused(GetWorld()))return;
 while(!AKTWStreetWar::Find(GetWorld())&&M->BattleSeconds>=NextRefresh&&NextRefresh<=720){for(AKTWGoldPile* P:Piles)if(IsValid(P)&&P->SourceIndex>=0){const int32 New=FMath::Min((P->SourceIndex==2||P->SourceIndex==3)?15:10,P->Capacity-P->Amount);P->Amount+=New;P->Unrewarded+=New;GeneratedGold+=New;P->UpdateVisuals();}NextRefresh+=30;}
 for(int32 I=PendingDrops.Num()-1;I>=0;--I)if(MaterializeDrop(PendingDrops[I]))PendingDrops.RemoveAt(I);
 Piles.RemoveAll([](const TObjectPtr<AKTWGoldPile>& P){return !IsValid(P);});
 M->ResolveVictory();
}
bool AKTWGoldSystem::Available(AKTWGoldPile* P,UKTWGoldCarrier* C)const{return IsValid(P)&&P->Amount>0&&C&&C->IsLiving()&&(!P->ReservedBy.IsValid()||!P->ReservedBy->IsLiving()||P->ReservedUntil<=GetWorld()->GetTimeSeconds()||P->ReservedBy.Get()==C);}
bool AKTWGoldSystem::Reserve(AKTWGoldPile* P,UKTWGoldCarrier* C,bool Started){if(!Available(P,C))return false;if(P->ReservedBy.Get()!=C||P->ReservedUntil<=GetWorld()->GetTimeSeconds()||Started){P->ReservedBy=C;P->ReservedUntil=GetWorld()->GetTimeSeconds()+2;}return true;}
void AKTWGoldSystem::Release(UKTWGoldCarrier* C){for(AKTWGoldPile* P:Piles)if(IsValid(P)&&P->ReservedBy.Get()==C){P->ReservedBy.Reset();P->ReservedUntil=0;}}
bool AKTWGoldSystem::IsSafe(FVector P,int32 Faction)const{for(TActorIterator<ACharacter>I(GetWorld());I;++I){bool Hostile=false;if(auto* U=Cast<AKTWTestUnit>(*I))Hostile=U->Health>0&&U->Faction!=Faction;if(auto* B=Cast<AKTWBoss>(*I))Hostile=!B->bRecovering&&B->Faction!=Faction;if(auto* Guard=Cast<AKTWSecurity>(*I))Hostile=Guard->Health>0;if(!Hostile||FVector::Dist2D(P,I->GetActorLocation())>500)continue;
 FCollisionQueryParams Q;Q.AddIgnoredActor(*I);FHitResult H;if(!GetWorld()->LineTraceSingleByChannel(H,P+FVector(0,0,90),I->GetActorLocation(),ECC_Visibility,Q))return false;
 }return true;}
AKTWGoldPile* AKTWGoldSystem::FindGold(UKTWGoldCarrier* C,float MaxDistance)const{auto* U=C?C->Unit():nullptr;if(!U)return nullptr;AKTWGoldPile* Best=nullptr;float BestD=MaxDistance;
 for(AKTWGoldPile* P:Piles){if(!Available(P,C)||(P->Lane>=0&&P->Lane!=U->Lane)||!IsSafe(P->GetActorLocation(),U->Faction))continue;const float D=FVector::Dist2D(U->GetActorLocation(),P->GetActorLocation());if(D<BestD){BestD=D;Best=P;}}return Best;}
bool AKTWGoldSystem::CommitPickup(UKTWGoldCarrier* C,AKTWGoldPile* P){auto* M=AKTWMatch::Find(GetWorld());auto* U=C?C->Unit():nullptr;
 if(!M||!M->CanAct()||UGameplayStatics::IsGamePaused(GetWorld())||!U||!C->IsLiving()||!Available(P,C)||P->ReservedBy.Get()!=C||C->Task!=EKTWGoldTask::PickingUp||C->ChannelProgress+.001f<C->ChannelRequired||C->ChannelFinishTime>AKTWStreetWar::Limit(GetWorld())||C->DamageFrame==GFrameCounter||FVector::Dist2D(U->GetActorLocation(),P->GetActorLocation())>120)return false;
 const int32 Amount=FMath::Min(P->Amount,U->Stats.Capacity-C->Carried);if(Amount<=0)return false;const int32 NewXP=FMath::Min(Amount,P->Unrewarded);
 P->Amount-=Amount;P->Unrewarded-=NewXP;C->SetCarried(C->Carried+Amount,C->Unrewarded+NewXP);C->LastTransferAt=GetWorld()->GetTimeSeconds();C->LastTransferAmount=Amount;C->bLastBanked=false;UKTWSession::Cue(this,TEXT("Pickup"),.65f,U->GetActorLocation());Release(C);P->UpdateVisuals();if(P->Amount==0&&P->SourceIndex<0){Piles.Remove(P);P->Destroy();}return true;
}
bool AKTWGoldSystem::CommitDelivery(UKTWGoldCarrier* C){auto* M=AKTWMatch::Find(GetWorld());auto* U=C?C->Unit():nullptr;
 if(!M||!M->CanAct()||UGameplayStatics::IsGamePaused(GetWorld())||!U||!C->IsLiving()||C->Carried<=0||C->Task!=EKTWGoldTask::Delivering||C->ChannelProgress+.001f<1||C->ChannelFinishTime>AKTWStreetWar::Limit(GetWorld())||C->DamageFrame==GFrameCounter||FVector::Dist2D(U->GetActorLocation(),AKTWStreetDirector::BasePosition(U->Faction))>400)return false;
 const int32 Amount=C->Carried;M->Accounts[U->Faction].Gold+=Amount;C->DeliveredTotal+=Amount;C->TransportExperience+=FMath::Min(6.f,C->Unrewarded/10.f);U->GainExperience(FMath::Min(6.f,C->Unrewarded/10.f));C->SetCarried(0,0);C->LastTransferAt=GetWorld()->GetTimeSeconds();C->LastTransferAmount=Amount;C->bLastBanked=true;UKTWSession::Cue(this,TEXT("Deposit"),.85f,U->GetActorLocation());M->GoldChanged(U->Faction);return true;
}
bool AKTWGoldSystem::ProjectDrop(FVector From,int32 Faction,FVector& Point)const{auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!NS)return false;FNavLocation N;
 auto Project=[&](FVector P){P.Z=100;if(AKTWStreetDirector::IsRoadPosition(P,-80)&&NS->ProjectPointToNavigation(P,N,FVector(120,120,250))&&AKTWStreetDirector::IsRoadPosition(N.Location,-60)){Point=N.Location;return true;}return false;};
 if(Project(From))return true;TArray<FVector> Candidates;for(int32 L=0;L<3;++L){auto R=AKTWStreetDirector::Route(L);for(int32 I=0;I<2;++I)Candidates.Add(FMath::ClosestPointOnSegment(FVector(From.X,From.Y,0),R[I],R[I+1]));}auto J=AKTWStreetDirector::Junctions();Candidates.Add(FMath::ClosestPointOnSegment(FVector(From.X,From.Y,0),J[1],J[3]));Candidates.Sort([&](const FVector& A,const FVector& B){return FVector::DistSquared2D(From,A)<FVector::DistSquared2D(From,B);});
 for(FVector P:Candidates)if(Project(P)){UE_LOG(LogTemp,Warning,TEXT("KTW gold drop projected from blocked point to legal road."));return true;}return Project(AKTWStreetDirector::BasePosition(Faction));
}
bool AKTWGoldSystem::MaterializeDrop(const FKTWPendingGoldDrop& D){FVector P;if(!ProjectDrop(D.Position,D.Faction,P))return false;
 for(AKTWGoldPile* Existing:Piles)if(IsValid(Existing)&&Existing->SourceIndex<0&&FVector::Dist2D(Existing->GetActorLocation(),P)<180){Existing->Amount+=D.Amount;Existing->Unrewarded+=D.Unrewarded;Existing->UpdateVisuals();return true;}
 auto* Pile=GetWorld()->SpawnActor<AKTWGoldPile>(P,FRotator::ZeroRotator);if(!Pile)return false;Pile->Amount=D.Amount;Pile->Unrewarded=D.Unrewarded;Pile->Lane=D.Lane;Pile->UpdateVisuals();Piles.Add(Pile);return true;
}
void AKTWGoldSystem::Drop(UKTWGoldCarrier* C){if(!C)return;Release(C);if(C->Carried<=0)return;auto* U=C->Unit();if(!U)return;
 FKTWPendingGoldDrop D;D.Amount=C->Carried;D.Unrewarded=C->Unrewarded;D.Position=U->GetActorLocation();D.Lane=U->Lane;D.Faction=U->Faction;
 // Escrow is counted as ground gold until navigation/spawn is available; never discard paid-for transport value.
 PendingDrops.Add(D);C->SetCarried(0,0);if(MaterializeDrop(D))PendingDrops.RemoveAt(PendingDrops.Num()-1);
}

UKTWGoldCarrier::UKTWGoldCarrier(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.05f;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;}
AKTWTestUnit* UKTWGoldCarrier::Unit()const{return Cast<AKTWTestUnit>(GetOwner());}
bool UKTWGoldCarrier::IsLiving()const{auto* U=Unit();return IsValid(U)&&U->Health>0;}
void UKTWGoldCarrier::SetCarried(int32 N,int32 X){Carried=N;Unrewarded=X;if(auto* U=Unit()){U->bCarryingGold=N>0;U->SetAura(U->bAura);
 if(N>0&&!CargoVisual){CargoVisual=NewObject<UStaticMeshComponent>(U,TEXT("CarriedGoldBag"));U->AddInstanceComponent(CargoVisual);CargoVisual->SetupAttachment(U->GetMesh(),TEXT("spine_03"));CargoVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));CargoVisual->SetRelativeLocation(FVector(-15,0,0));CargoVisual->SetRelativeScale3D(FVector(.2f,.26f,.15f));CargoVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);CargoVisual->SetCanEverAffectNavigation(false);if(auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/KTW/Materials/M_NeonGold.M_NeonGold")))CargoVisual->SetMaterial(0,Mat);FKTWRunnerVisuals::ConfigureCargo(U,CargoVisual);CargoVisual->RegisterComponent();}
 if(CargoVisual)CargoVisual->SetVisibility(N>0);
 }}
void UKTWGoldCarrier::ResetChannel(){ChannelStartedAt=-1;ChannelFinishTime=0;ChannelProgress=0;ChannelRequired=0;if(Task==EKTWGoldTask::PickingUp||Task==EKTWGoldTask::Delivering)Task=EKTWGoldTask::Idle;if(auto* A=GetOwner()->FindComponentByClass<UKTWCombatAction>())A->CancelInteraction();}
bool UKTWGoldCarrier::UnderThreat()const{return GetWorld()->GetTimeSeconds()-LastDamage<3;}
void UKTWGoldCarrier::Interrupt(){LastDamage=GetWorld()->GetTimeSeconds();DamageFrame=GFrameCounter;NextAttempt=LastDamage+.2;ResetChannel();if(System.IsValid())System->Release(this);}
void UKTWGoldCarrier::Defeated(){Interrupt();if(auto* S=AKTWGoldSystem::Find(GetWorld()))S->Drop(this);TargetPile.Reset();bDeferredOrder=false;bSavedTask=false;bControlsMovement=false;}
void UKTWGoldCarrier::EndPlay(const EEndPlayReason::Type Reason){if(Reason==EEndPlayReason::Destroyed)if(auto* S=AKTWGoldSystem::Find(GetWorld()))S->Drop(this);Super::EndPlay(Reason);}
void UKTWGoldCarrier::MoveTo(FVector P,float Acceptance){if(GetWorld()->GetTimeSeconds()<NextMove)return;NextMove=GetWorld()->GetTimeSeconds()+.2;if(auto* U=Unit())if(auto* AI=Cast<AAIController>(U->GetController()))AI->MoveToLocation(P,Acceptance,false,true,true,false,FKTWTactics::Filter(U->Faction),false);}
bool UKTWGoldCarrier::RequestPickup(AKTWGoldPile* P){auto* S=AKTWGoldSystem::Find(GetWorld());auto* M=AKTWMatch::Find(GetWorld());auto* U=Unit();if(!M||!S||!S->Available(P,this)||!M->CanAct()||M->BattleSeconds>=AKTWStreetWar::Limit(GetWorld())||UGameplayStatics::IsGamePaused(GetWorld())||!U||!IsLiving()||!IsValid(P)||P->Amount<=0||Carried>=U->Stats.Capacity||(Carried>0&&(Carried*5>=U->Stats.Capacity*4||FVector::Dist2D(U->GetActorLocation(),P->GetActorLocation())>400)))return false;
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!NS)return false;if(FVector::Dist2D(U->GetActorLocation(),P->GetActorLocation())>80){auto* Path=NS->FindPathToLocationSynchronously(GetWorld(),U->GetActorLocation(),P->GetActorLocation()+FVector(0,0,80),nullptr,FKTWTactics::Filter(U->Faction));if(!Path||!Path->IsValid()||Path->IsPartial())return false;}
 if(!bSavedTask&&(U->Order==EKTWUnitOrder::Defend||U->Order==EKTWUnitOrder::Escort||U->Order==EKTWUnitOrder::Retreat)){bSavedTask=true;SavedOrder=U->Order;SavedPoint=U->OrderAnchor;SavedTarget=U->OrderTarget;}
 if(S)S->Release(this);ResetChannel();TargetPile=P;System=S;bManualPickup=true;bReturning=false;bHealing=false;bDeferredOrder=false;U->bLaneDeparturePending=false;U->Order=EKTWUnitOrder::Pickup;U->bAutoPatrol=false;Task=EKTWGoldTask::Seeking;bControlsMovement=true;return true;
}
bool UKTWGoldCarrier::DeferOrder(EKTWUnitOrder O,FVector P,AActor* Target){if(Carried<=0)return false;if(O==EKTWUnitOrder::Defend||O==EKTWUnitOrder::Escort){bDeferredOrder=true;DeferredOrder=O;DeferredPoint=P;DeferredTarget=Target;bReturning=true;bManualPickup=false;if(auto* U=Unit()){U->Order=EKTWUnitOrder::Advance;U->bAutoPatrol=true;}ResetChannel();if(System.IsValid())System->Release(this);TargetPile.Reset();return true;}return false;}
void UKTWGoldCarrier::OnAcceptedOrder(EKTWUnitOrder O){ResetChannel();bRaid=false;bRaidAfterDelivery=false;RaidEmptySince=-1;if(System.IsValid())System->Release(this);TargetPile.Reset();bManualPickup=false;bHealing=false;bDeferredOrder=false;bControlsMovement=false;if(O!=EKTWUnitOrder::Move||Carried<=0)bSavedTask=false;NoSafeGoldSince=-1;if(O==EKTWUnitOrder::Move)bReturning=false;else bReturning=Carried>0;}
void UKTWGoldCarrier::ResumeAfterDelivery(){auto* U=Unit();if(!U)return;bReturning=false;bManualPickup=false;TargetPile.Reset();ResetChannel();
 const bool Restore=bDeferredOrder||bSavedTask;const EKTWUnitOrder O=bDeferredOrder?DeferredOrder:SavedOrder;const FVector P=bDeferredOrder?DeferredPoint:SavedPoint;AActor* T=bDeferredOrder?DeferredTarget.Get():SavedTarget.Get();bDeferredOrder=false;bSavedTask=false;
 if(bRaidAfterDelivery){bRaidAfterDelivery=false;RequestRaid();return;}
 if(U->Order==EKTWUnitOrder::Pickup||Restore){U->Order=EKTWUnitOrder::Advance;U->bAutoPatrol=true;}if(Restore)U->IssueOrder(O,P,T);
}
const TCHAR* UKTWGoldCarrier::TaskLabel()const{switch(Task){case EKTWGoldTask::Raiding:return TEXT("RAIDING VAULT");case EKTWGoldTask::Seeking:return TEXT("FIND GOLD");case EKTWGoldTask::PickingUp:return TEXT("PICKING");case EKTWGoldTask::Returning:return TEXT("RETURN GOLD");case EKTWGoldTask::Delivering:return TEXT("DELIVERING");case EKTWGoldTask::Healing:return TEXT("HEALING");case EKTWGoldTask::Waiting:return TEXT("WAIT GOLD");default:return TEXT("TACTICAL");}}
void UKTWGoldCarrier::TickComponent(float D,ELevelTick Type,FActorComponentTickFunction* Fn){Super::TickComponent(D,Type,Fn);auto* M=AKTWMatch::Find(GetWorld());auto* U=Unit();if(!M||!U)return;if(!System.IsValid())System=AKTWGoldSystem::Find(GetWorld());auto* S=System.Get();if(!S)return;S->AddTickPrerequisiteComponent(this);
 if(AKTWStreetWar::Busy(U)){ResetChannel();S->Release(this);bControlsMovement=false;return;}
 if(!M->CanAct()||!IsLiving()){ResetChannel();S->Release(this);bControlsMovement=false;return;}if(UGameplayStatics::IsGamePaused(GetWorld()))return;
 if((U->bLaneDeparturePending&&U->Order==EKTWUnitOrder::Advance)||U->Order==EKTWUnitOrder::PickupGear||(AKTWStreetLife::Find(GetWorld())&&AKTWStreetLife::Find(GetWorld())->Busy(U))){bControlsMovement=false;return;}
 if(auto* A=U->FindComponentByClass<UKTWCombatAction>())if(A->bPlaying&&Task!=EKTWGoldTask::PickingUp&&Task!=EKTWGoldTask::Delivering){bControlsMovement=false;return;}
 const double Now=GetWorld()->GetTimeSeconds();const FVector Here=U->GetActorLocation(),Home=AKTWStreetDirector::BasePosition(U->Faction);const bool AtHome=FVector::Dist2D(Here,Home)<=400;
 if(AtHome&&Now-LastDamage>=5)U->Health=FMath::Min(U->Stats.MaxHealth,U->Health+U->Stats.MaxHealth*.04f*D);
 if(Now<U->StunnedUntil){ResetChannel();S->Release(this);if(auto* AI=Cast<AAIController>(U->GetController()))AI->StopMovement();bControlsMovement=true;return;}
 if(DamageFrame==GFrameCounter||Now<NextAttempt){bControlsMovement=false;return;}
 if(U->Order==EKTWUnitOrder::Attack){ResetChannel();S->Release(this);bControlsMovement=false;Task=EKTWGoldTask::Idle;return;}
 // Movement orders reach their destination before automatic jobs; fighters defend cargo too.
 if(!U->bRunner&&U->Order!=EKTWUnitOrder::Move&&U->Order!=EKTWUnitOrder::Retreat&&
    ((U->bEnableCombat&&FKTWTactics::FindEnemy(U,U->Faction,1400))||(Carried==0&&!S->IsSafe(Here,U->Faction)))){
  ResetChannel();S->Release(this);bControlsMovement=false;Task=EKTWGoldTask::Idle;return;
 }
 if(UnderThreat()&&U->Order!=EKTWUnitOrder::Move){
  ResetChannel();S->Release(this);TargetPile.Reset();bManualPickup=false;
  if(Carried>0||U->bRunner){bReturning=Carried>0;Task=EKTWGoldTask::Returning;bControlsMovement=true;MoveTo(Home,180);return;}
  bControlsMovement=false;Task=EKTWGoldTask::Idle;return;
 }
 if(bHealing){if(U->Health<U->Stats.MaxHealth*.8f){Task=EKTWGoldTask::Healing;bControlsMovement=true;return;}bHealing=false;ResumeAfterDelivery();}
 if(Carried>0&&U->Order==EKTWUnitOrder::Move){Task=EKTWGoldTask::Idle;bControlsMovement=false;return;}
 if(Carried>0&&!bManualPickup&&!S->IsSafe(Here,U->Faction))bReturning=true;
 if(Carried>0&&(bReturning||U->Order==EKTWUnitOrder::Retreat||Carried*5>=U->Stats.Capacity*4)){
  bReturning=true;bControlsMovement=true;S->Release(this);TargetPile.Reset();
  if(!AtHome){ResetChannel();Task=EKTWGoldTask::Returning;MoveTo(Home,180);return;}
  if(auto* AI=Cast<AAIController>(U->GetController()))AI->StopMovement();if(Task!=EKTWGoldTask::Delivering){ResetChannel();Task=EKTWGoldTask::Delivering;ChannelRequired=1;if(auto* A=U->FindComponentByClass<UKTWCombatAction>())A->PlayPickup(1);}
  AdvanceChannel(1,M->BattleSeconds);if(ChannelFinishTime<=AKTWStreetWar::Limit(GetWorld())&&ChannelProgress>=1&&S->CommitDelivery(this)){ResetChannel();if(U->Health<U->Stats.MaxHealth*.8f){bHealing=true;Task=EKTWGoldTask::Healing;}else ResumeAfterDelivery();}return;
 }
 if(bRaid&&Carried==0){
  const FVector Vault=VaultPosition(1-U->Faction,U->Lane);bControlsMovement=true;
  bool Guarded=false;for(TActorIterator<ACharacter> G(GetWorld());G;++G){bool Alive=false;if(auto* V=Cast<AKTWTestUnit>(*G))Alive=V->Faction!=U->Faction&&V->Health>0;if(auto* B=Cast<AKTWBoss>(*G))Alive=B->Faction!=U->Faction&&!B->bRecovering;if(Alive&&FVector::Dist2D(G->GetActorLocation(),Vault)<=400){Guarded=true;break;}}
  if(Guarded&&U->bRunner){ResetChannel();Task=EKTWGoldTask::Waiting;bControlsMovement=true;FVector Away=(Here-Vault).GetSafeNormal2D();if(Away.IsNearlyZero())Away=(Home-Vault).GetSafeNormal2D();MoveTo(Vault+Away*650,100);return;}
  if(FVector::Dist2D(Here,Vault)>140){ResetChannel();Task=EKTWGoldTask::Seeking;MoveTo(Vault,75);return;}
  if(Guarded){ResetChannel();Task=EKTWGoldTask::Waiting;bControlsMovement=false;return;}
  if(M->Accounts[1-U->Faction].Gold<=0){ResetChannel();Task=EKTWGoldTask::Waiting;if(RaidEmptySince<0)RaidEmptySince=Now;if(Now-RaidEmptySince>3){bRaid=false;ResumeAfterDelivery();}return;}
  RaidEmptySince=-1;if(auto* AI=Cast<AAIController>(U->GetController()))AI->StopMovement();
  Task=EKTWGoldTask::Raiding;AdvanceChannel(4,M->BattleSeconds);
  if(ChannelFinishTime<=AKTWStreetWar::Limit(GetWorld())&&ChannelProgress>=4&&DamageFrame!=GFrameCounter){const int32 Amount=FMath::Min(M->Accounts[1-U->Faction].Gold,U->Stats.Capacity);M->Accounts[1-U->Faction].Gold-=Amount;M->GoldChanged(1-U->Faction);SetCarried(Amount,0);bRaid=false;bReturning=true;ResetChannel();}
  return;
 }
 if(Carried==0&&(U->Order==EKTWUnitOrder::Move||U->Order==EKTWUnitOrder::Defend||U->Order==EKTWUnitOrder::Escort||U->Order==EKTWUnitOrder::Retreat||U->Order==EKTWUnitOrder::Attack)){bControlsMovement=false;Task=EKTWGoldTask::Idle;return;}
 // A local mission survives banking: return to its area before seeking another load.
 if(!U->bRunner&&Carried==0&&U->bHasLocalWork&&U->Order==EKTWUnitOrder::Advance&&FVector::Dist2D(Here,U->WorkAnchor)>2400){ResetChannel();S->Release(this);TargetPile.Reset();bControlsMovement=false;Task=EKTWGoldTask::Idle;return;}
 if(!bManualPickup){const float Radius=U->bRunner?(Carried>0?600.f:30000.f):(U->bHasLocalWork?2200.f:1800.f);auto* P=TargetPile.Get();if(!S->Available(P,this)||!S->IsSafe(P->GetActorLocation(),U->Faction)||FVector::Dist2D(Here,P->GetActorLocation())>Radius){ResetChannel();S->Release(this);TargetPile=S->FindGold(this,Radius);}}
 auto* P=TargetPile.Get();if(!IsValid(P)||P->Amount<=0){ResetChannel();S->Release(this);TargetPile.Reset();
  if(bManualPickup){bManualPickup=false;if(Carried>0){bReturning=true;return;}ResumeAfterDelivery();}
  if(Carried>0){if(NoSafeGoldSince<0)NoSafeGoldSince=Now;if(Now-NoSafeGoldSince>=5)bReturning=true;bControlsMovement=true;Task=EKTWGoldTask::Waiting;return;}
  if(U->bRunner&&U->Order==EKTWUnitOrder::Advance&&U->bAutoPatrol&&M->BattleSeconds>=60&&M->Accounts[1-U->Faction].Gold>=500&&RequestRaid())return;
  if(U->bRunner&&U->Order==EKTWUnitOrder::Advance&&U->bAutoPatrol){bControlsMovement=true;Task=EKTWGoldTask::Waiting;FVector Goal=Home;float Best=MAX_flt;for(AKTWGoldPile* Source:S->Piles)if(IsValid(Source)&&Source->SourceIndex>=0&&(Source->Lane<0||Source->Lane==U->Lane)&&S->IsSafe(Source->GetActorLocation(),U->Faction)){const float Dist=FVector::Dist2D(Here,Source->GetActorLocation());if(Dist<Best){Best=Dist;Goal=Source->GetActorLocation();}}MoveTo(Goal,180);return;}
  if(!U->bHasLocalWork&&U->Order==EKTWUnitOrder::Advance&&M->BattleSeconds>=60&&M->Accounts[1-U->Faction].Gold>=60&&RequestRaid())return;
  bControlsMovement=false;Task=EKTWGoldTask::Idle;return;
 }
 NoSafeGoldSince=-1;bControlsMovement=true;const float Distance=FVector::Dist2D(Here,P->GetActorLocation());
 if(Distance>120){ResetChannel();Task=EKTWGoldTask::Seeking;if(Distance<260)S->Reserve(P,this);MoveTo(P->GetActorLocation(),65);return;}
 if(!S->Reserve(P,this,true)){TargetPile.Reset();ResetChannel();Task=EKTWGoldTask::Waiting;return;}
 if(auto* AI=Cast<AAIController>(U->GetController()))AI->StopMovement();if(Task!=EKTWGoldTask::PickingUp){ResetChannel();Task=EKTWGoldTask::PickingUp;ChannelRequired=PickupSeconds();U->SetActorRotation(FRotator(0,(P->GetActorLocation()-Here).Rotation().Yaw,0));if(auto* A=U->FindComponentByClass<UKTWCombatAction>())A->PlayPickup(ChannelRequired);}
 AdvanceChannel(ChannelRequired,M->BattleSeconds);if(ChannelFinishTime<=AKTWStreetWar::Limit(GetWorld())&&ChannelProgress>=ChannelRequired&&S->CommitPickup(this,P)){ResetChannel();TargetPile.Reset();if(bManualPickup||Carried*5>=U->Stats.Capacity*4)bReturning=true;Task=EKTWGoldTask::Returning;}
}

void AKTWGoldSystem::TestCheck(bool OK,const TCHAR* Name){UE_LOG(LogTemp,Display,TEXT("KTW GOLD %s: %s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));if(!OK)++TestFailures;}
void AKTWGoldSystem::RunAutomaticTransportTest(){
#if !UE_BUILD_SHIPPING
 auto* M=AKTWMatch::Find(GetWorld());if(!M){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 TestCheck(M->Purchase(0,EKTWRole::Runner,0)&&M->Purchase(1,EKTWRole::Runner,0),TEXT("automatic couriers recruited using cash"));M->StartBattle();for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->bEnableCombat=false;
 const double Deadline=GetWorld()->GetTimeSeconds()+60;FTimerHandle Driver;
 GetWorldTimerManager().SetTimer(Driver,FTimerDelegate::CreateWeakLambda(this,[this,Deadline](){auto* M=AKTWMatch::Find(GetWorld());if(!M)return;
  if(M->Accounts[0].Gold>=10&&M->Accounts[1].Gold>=10){TestCheck(GoldConserved()&&M->CashConserved(),TEXT("both factions automatically travel pick return and bank gold"));UE_LOG(LogTemp,Display,TEXT("KTW GOLD AUTO COMPLETE Blue=%d Red=%d time=%.1fs failures=%d"),M->Accounts[0].Gold,M->Accounts[1].Gold,M->BattleSeconds,TestFailures);FPlatformMisc::RequestExitWithStatus(false,TestFailures?1:0);}
  else if(GetWorld()->GetTimeSeconds()>Deadline){for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I){auto* C=I->FindComponentByClass<UKTWGoldCarrier>();UE_LOG(LogTemp,Error,TEXT("AUTO courier %d at %s task=%s carried=%d"),I->Faction,*I->GetActorLocation().ToString(),C?C->TaskLabel():TEXT("NONE"),C?C->Carried:0);}TestCheck(false,TEXT("automatic transport timeout"));FPlatformMisc::RequestExitWithStatus(false,1);}
 }),1.f,true);
#endif
}
void AKTWGoldSystem::RunGoldTests(){
#if !UE_BUILD_SHIPPING
 auto* M=AKTWMatch::Find(GetWorld());if(!M){FPlatformMisc::RequestExitWithStatus(false,1);return;}M->SetActorTickEnabled(false);SetActorTickEnabled(false);
 TestCheck(Piles.Num()==6&&GeneratedGold==0&&GoldConserved(),TEXT("six empty sources in preparation"));Tick(30);TestCheck(GeneratedGold==0,TEXT("preparation cannot generate gold"));
 M->StartBattle();M->Tick(19.9f);Tick(0);TestCheck(GeneratedGold==0,TEXT("no spawn before twenty seconds"));M->Tick(.1001f);Tick(0);
 TestCheck(GeneratedGold==70&&Piles[0]->Amount==10&&Piles[2]->Amount==15&&Piles[4]->Amount==10,TEXT("first wave 20 30 20 by lane"));M->Tick(30);Tick(0);TestCheck(GeneratedGold==140,TEXT("thirty second refresh"));M->Tick(90);Tick(0);TestCheck(GeneratedGold==210&&Piles[0]->Amount==30&&Piles[2]->Amount==45,TEXT("per source cap skips overflow"));
 struct FProbe{int32 Stage=0;double Until=0,Deadline=0;TWeakObjectPtr<AKTWTestUnit> Runner,Enemy,Knife,Pipe,Thrower;TWeakObjectPtr<AKTWGoldPile> CapacityPile;int32 Cash=0;};auto P=MakeShared<FProbe>();P->Deadline=GetWorld()->GetTimeSeconds()+35;
 auto Spawn=[&](EKTWRole Type,int32 Team,int32 Lane,FVector Point){auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(Point),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);U->Faction=Team;U->UnitRole=Type;U->Lane=Lane;U->bEnableCombat=false;U->bAutoPatrol=false;U->Order=EKTWUnitOrder::Defend;U->FinishSpawning(FTransform(Point));return U;};
 P->Runner=Spawn(EKTWRole::Runner,0,0,Piles[0]->GetActorLocation()+FVector(0,0,90));P->Enemy=Spawn(EKTWRole::Runner,1,0,Piles[0]->GetActorLocation()+FVector(90,0,90));
 auto* C=P->Runner->FindComponentByClass<UKTWGoldCarrier>();auto* E=P->Enemy->FindComponentByClass<UKTWGoldCarrier>();
 TestCheck(Reserve(Piles[0],C)&&!Reserve(Piles[0],E),TEXT("opposing carriers cannot share a reservation"));Piles[0]->ReservedUntil=GetWorld()->GetTimeSeconds()-.01;TestCheck(Reserve(Piles[0],E),TEXT("expired reservation can be claimed by another carrier"));Release(E);TestCheck(C->RequestPickup(Piles[0]),TEXT("manual pickup request accepted"));P->Until=GetWorld()->GetTimeSeconds()+.1;
 FTimerHandle Driver;GetWorldTimerManager().SetTimer(Driver,FTimerDelegate::CreateWeakLambda(this,[this,P,Spawn]()mutable{
  auto* Match=AKTWMatch::Find(GetWorld());if(!Match)return;const double Now=GetWorld()->GetTimeSeconds();if(Now>P->Deadline){TestCheck(false,TEXT("gold test timeout"));FPlatformMisc::RequestExitWithStatus(false,1);return;}if(Now<P->Until)return;
  auto* Runner=P->Runner.Get();auto* Enemy=P->Enemy.Get();auto* RC=Runner?Runner->FindComponentByClass<UKTWGoldCarrier>():nullptr;auto* EC=Enemy?Enemy->FindComponentByClass<UKTWGoldCarrier>():nullptr;
  switch(P->Stage){
   case 0:TestCheck(RC&&RC->Carried==0&&Piles[0]->Amount==30,TEXT("runner has a real pickup windup"));P->Stage=1;P->Until=Now+.35;break;
   case 1:{TestCheck(RC&&RC->Carried==30&&Piles[0]->Amount==0&&GoldConserved(),TEXT("pickup transfers once with provenance"));TestCheck(RC&&FMath::IsNearlyEqual(Runner->GetCharacterMovement()->MaxWalkSpeed,Runner->Stats.Speed*.85f,.1f),TEXT("carried gold slows movement"));TestCheck(Runner->IssueOrder(EKTWUnitOrder::Defend,Runner->GetActorLocation())&&RC->bDeferredOrder,TEXT("carrying defense command deferred until delivery"));
    FDamageEvent D;Runner->TakeDamage(1000,D,nullptr,Enemy);TestCheck(RC->Carried==0&&GroundGold()==210&&GoldConserved(),TEXT("death drops the entire load without creating gold"));Drop(RC);TestCheck(GroundGold()==210,TEXT("duplicate death does not duplicate drops"));
    AKTWGoldPile* DropPile=nullptr;for(AKTWGoldPile* G:Piles)if(G->SourceIndex<0)DropPile=G;TestCheck(DropPile&&DropPile->Unrewarded==30,TEXT("drop retains unawarded transport provenance"));if(!DropPile){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    Enemy->SetActorLocation(DropPile->GetActorLocation()+FVector(80,0,90),false,nullptr,ETeleportType::TeleportPhysics);TestCheck(EC->RequestPickup(DropPile),TEXT("enemy can recover dropped gold"));P->Stage=2;P->Until=Now+.6;break;
   }
   case 2:{TestCheck(EC&&EC->Carried==30&&GoldConserved(),TEXT("opposing pickup is a transfer only"));TestCheck(Enemy->IssueOrder(EKTWUnitOrder::Move,Enemy->GetActorLocation()+FVector(0,-350,0)),TEXT("carrying direct move is accepted"));P->Stage=20;P->Until=Now+.9;break;}
   case 20:{TestCheck(EC->Task==EKTWGoldTask::Returning&&Enemy->Order==EKTWUnitOrder::Advance,TEXT("partial load resumes delivery immediately after a manual move"));Enemy->SetActorLocation(AKTWStreetDirector::BasePosition(1)+FVector(200,0,100),false,nullptr,ETeleportType::TeleportPhysics);P->Cash=Match->Accounts[1].Cash;P->Stage=3;P->Until=Now+.5;break;}
   case 3:TestCheck(Match->Accounts[1].Gold==0&&EC->Carried==30,TEXT("delivery cannot credit before one second"));P->Stage=4;P->Until=Now+.8;break;
   case 4:{TestCheck(Match->Accounts[1].Gold==30&&EC->Carried==0&&FMath::IsNearlyEqual(EC->TransportExperience,3.f)&&GoldConserved(),TEXT("delivery banks gold once and records transport experience"));TestCheck(Match->Accounts[1].Cash==P->Cash&&Match->CashConserved(),TEXT("gold delivery never changes cash"));
    TestCheck(Enemy->Order==EKTWUnitOrder::Defend,TEXT("manual pickup restores the original persistent defense task"));
    P->Knife=Spawn(EKTWRole::Knife,0,1,Piles[2]->GetActorLocation()+FVector(0,0,90));auto* K=P->Knife->FindComponentByClass<UKTWGoldCarrier>();TestCheck(K->RequestPickup(Piles[2]),TEXT("combat unit can manually collect gold"));P->Stage=5;P->Until=Now+.25;break;
   }
   case 5:{auto* K=P->Knife.Get();if(!K){FPlatformMisc::RequestExitWithStatus(false,1);return;}FDamageEvent D;K->TakeDamage(1,D,nullptr,nullptr);TestCheck(K->FindComponentByClass<UKTWGoldCarrier>()->Carried==0,TEXT("damage interrupts pickup before commit"));P->Stage=6;P->Until=Now+.4;break;}
   case 6:TestCheck(P->Knife->FindComponentByClass<UKTWGoldCarrier>()->Carried==0&&!P->Knife->FindComponentByClass<UKTWGoldCarrier>()->ControlsMovement()&&Piles[2]->Amount==45,TEXT("interrupted pickup cancels transfer and releases combat control"));P->Knife->FindComponentByClass<UKTWGoldCarrier>()->LastDamage=-100;P->Knife->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(Piles[2]);P->Stage=7;P->Until=Now+.8;break;
   case 7:{auto* K=P->Knife->FindComponentByClass<UKTWGoldCarrier>();TestCheck(K->Carried==20&&Piles[2]->Amount==25,TEXT("knife capacity and partial stack are exact"));P->Knife->SetActorLocation(AKTWStreetDirector::BasePosition(0)+FVector(200,0,100),false,nullptr,ETeleportType::TeleportPhysics);P->Stage=8;P->Until=Now+.8;break;}
   case 8:{auto* K=P->Knife.Get();FDamageEvent D;K->TakeDamage(1000,D,nullptr,nullptr);TestCheck(Match->Accounts[0].Gold==0&&K->FindComponentByClass<UKTWGoldCarrier>()->Carried==0&&GoldConserved(),TEXT("death before delivery commit credits no base gold"));
    P->Pipe=Spawn(EKTWRole::Pipe,0,1,Piles[3]->GetActorLocation()+FVector(0,0,90));P->Thrower=Spawn(EKTWRole::Thrower,0,2,Piles[4]->GetActorLocation()+FVector(0,0,90));P->Pipe->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(Piles[3]);P->Thrower->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(Piles[4]);P->Stage=9;P->Until=Now+.9;break;
   }
   case 9:{auto* PC=P->Pipe->FindComponentByClass<UKTWGoldCarrier>();auto* TC=P->Thrower->FindComponentByClass<UKTWGoldCarrier>();TestCheck(PC->Carried==30&&TC->Carried==20&&GoldConserved(),TEXT("pipe and thrower capacity limits"));
    P->Pipe->SetActorLocation(FVector(-3000,-4500,100),false,nullptr,ETeleportType::TeleportPhysics);FDamageEvent D;P->Pipe->TakeDamage(1000,D,nullptr,nullptr);bool Legal=P->Pipe->Health<=0&&PC->Carried==0;for(AKTWGoldPile* G:Piles)if(G->SourceIndex<0)Legal&=AKTWStreetDirector::IsRoadPosition(G->GetActorLocation());TestCheck(Legal&&GoldConserved(),TEXT("blocked drop location falls back to a legal road"));
    P->Thrower->SetActorLocation(AKTWStreetDirector::BasePosition(0)+FVector(-200,0,100),false,nullptr,ETeleportType::TeleportPhysics);P->Stage=10;P->Until=Now+.35;break;
   }
   case 10:{auto* TC=P->Thrower->FindComponentByClass<UKTWGoldCarrier>();TestCheck(TC->Task==EKTWGoldTask::Delivering&&TC->ChannelProgress>0,TEXT("pause begins during an active delivery channel"));
    const int32 Before=GeneratedGold;const float Progress=TC->ChannelProgress;const float Clock=Match->BattleSeconds;UGameplayStatics::SetGamePaused(GetWorld(),true);const TWeakObjectPtr<AKTWGoldSystem> Weak=this;const TWeakObjectPtr<UKTWGoldCarrier> Cargo=TC;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Cargo,Before,Progress,Clock](float){if(auto* S=Weak.Get()){auto* M=AKTWMatch::Find(S->GetWorld());S->TestCheck(M&&M->BattleSeconds==Clock&&S->GeneratedGold==Before&&Cargo.IsValid()&&Cargo->ChannelProgress==Progress,TEXT("real pause freezes resources and channels"));UGameplayStatics::SetGamePaused(S->GetWorld(),false);}return false;}),.3f);P->Stage=11;P->Until=Now+.2;break;
   }
   case 11:{
    // Build a >60 stack using existing source balances, never create test gold out of thin air.
    auto* Stack=GetWorld()->SpawnActor<AKTWGoldPile>(Piles[0]->GetActorLocation(),FRotator::ZeroRotator);Stack->Lane=0;Stack->Amount=70;Stack->Unrewarded=70;for(int32 I:{1,5}){Piles[I]->Amount-=30;Piles[I]->Unrewarded-=30;Piles[I]->UpdateVisuals();}Piles[2]->Amount-=10;Piles[2]->Unrewarded-=10;Piles[2]->UpdateVisuals();Stack->UpdateVisuals();Piles.Add(Stack);P->CapacityPile=Stack;
    P->Runner=Spawn(EKTWRole::Runner,0,0,Stack->GetActorLocation()+FVector(0,0,90));TestCheck(GoldConserved()&&P->Runner->FindComponentByClass<UKTWGoldCarrier>()->RequestPickup(Stack),TEXT("merged stack preserves global gold"));P->Stage=12;P->Until=Now+.6;break;
   }
   case 12:{auto* C=P->Runner->FindComponentByClass<UKTWGoldCarrier>();TestCheck(C->Carried==60&&P->CapacityPile.IsValid()&&P->CapacityPile->Amount==10&&!C->RequestPickup(P->CapacityPile.Get())&&GoldConserved(),TEXT("runner capacity sixty leaves exact remainder and rejects overfill"));
    Match->Tick(30);Tick(0);TestCheck(Piles[0]->Amount==10&&GeneratedGold==280&&GoldConserved(),TEXT("depleted sources refill independently of dropped stacks"));const int32 Before=GeneratedGold;Match->FinishBattle();Match->Tick(30);Tick(30);TestCheck(GeneratedGold==Before&&GoldConserved(),TEXT("finished match freezes gold generation and transfers"));UE_LOG(LogTemp,Display,TEXT("KTW GOLD COMPLETE failures=%d"),TestFailures);FPlatformMisc::RequestExitWithStatus(false,TestFailures?1:0);break;}
  }
 }),.05f,true);
#endif
}

FVector UKTWGoldCarrier::VaultPosition(int32 Faction,int32 Lane){const FVector B=AKTWStreetDirector::BasePosition(Faction);return B+(AKTWStreetDirector::Route(Lane,Faction)[1]-B).GetSafeNormal2D()*1100;}
bool UKTWGoldCarrier::RequestRaid(){
 auto* M=AKTWMatch::Find(GetWorld());auto* U=Unit();if(!M||!M->CanAct()||M->BattleSeconds>=AKTWStreetWar::Limit(GetWorld())||!IsLiving()||UGameplayStatics::IsGamePaused(GetWorld()))return false;
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());const FVector P=VaultPosition(1-U->Faction,U->Lane);
 auto* Path=NS?NS->FindPathToLocationSynchronously(GetWorld(),U->GetActorLocation(),P+FVector(0,0,80),nullptr,FKTWTactics::Filter(U->Faction)):nullptr;
 if(FVector::Dist2D(U->GetActorLocation(),P)>80&&(!Path||!Path->IsValid()||Path->IsPartial()))return false;
 if(Carried>0){bRaidAfterDelivery=true;bReturning=true;bRaid=false;ResetChannel();if(System.IsValid())System->Release(this);TargetPile.Reset();return true;}
 OnAcceptedOrder(EKTWUnitOrder::Advance);U->bLaneDeparturePending=false;U->Order=EKTWUnitOrder::Advance;U->bAutoPatrol=true;bRaid=true;bControlsMovement=true;Task=EKTWGoldTask::Seeking;return true;
}

void UKTWGoldCarrier::AdvanceChannel(float Duration,float MatchTime){ChannelRequired=Duration;if(ChannelStartedAt<0){ChannelStartedAt=GetWorld()->GetTimeSeconds();ChannelFinishTime=MatchTime+Duration;}ChannelProgress=FMath::Min(Duration,float(GetWorld()->GetTimeSeconds()-ChannelStartedAt));}
void AKTWGoldSystem::RunPhase3Tests(){
#if !UE_BUILD_SHIPPING
 auto* M=AKTWMatch::Find(GetWorld());if(!M){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 SetActorTickEnabled(false);M->SetActorTickEnabled(false);
 for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)I->Destroy();
 for(TActorIterator<AKTWBoss>I(GetWorld());I;++I)I->Destroy();
 M->StartBattle();M->Accounts[1].Gold=100;GeneratedGold=100;
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());for(int32 F=0;F<2;++F){auto* Own=NS->FindPathToLocationSynchronously(GetWorld(),FVector(0,0,100),AKTWStreetDirector::BasePosition(F)+FVector(0,0,100),nullptr,FKTWTactics::Filter(F));auto* Enemy=NS->FindPathToLocationSynchronously(GetWorld(),FVector(0,0,100),AKTWStreetDirector::BasePosition(1-F)+FVector(0,0,100),nullptr,FKTWTactics::Filter(F));TestCheck(Own&&Own->IsValid()&&!Own->IsPartial()&&(!Enemy||!Enemy->IsValid()||Enemy->IsPartial()),TEXT("faction navigation allows own base and excludes enemy safe zone"));}

 auto* U=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(UKTWGoldCarrier::VaultPosition(1,0)+FVector(0,0,90)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 U->Faction=0;U->Lane=0;U->UnitRole=EKTWRole::Runner;U->bEnableCombat=false;U->bAutoPatrol=false;U->FinishSpawning(FTransform(UKTWGoldCarrier::VaultPosition(1,0)+FVector(0,0,90)));
 auto* C=U->FindComponentByClass<UKTWGoldCarrier>();TestCheck(C->RequestRaid(),TEXT("reachable enemy vault accepts raid"));
 const TWeakObjectPtr<AKTWTestUnit> WU=U;const TWeakObjectPtr<UKTWGoldCarrier> WC=C;
 FTimerHandle Hurt;GetWorldTimerManager().SetTimer(Hurt,FTimerDelegate::CreateLambda([this,WU,WC](){if(WU.IsValid()){FDamageEvent D;WU->TakeDamage(1,D,nullptr,nullptr);TestCheck(WC->ChannelProgress==0&&WC->Carried==0,TEXT("damage cancels raid channel without partial transfer"));WC->LastDamage=-100;}}),1.f,false);
 auto Guard=MakeShared<TWeakObjectPtr<AKTWTestUnit>>();FTimerHandle GuardSpawn;GetWorldTimerManager().SetTimer(GuardSpawn,FTimerDelegate::CreateLambda([this,Guard](){const FVector P=UKTWGoldCarrier::VaultPosition(1,0)+FVector(200,0,90);auto* G=GetWorld()->SpawnActorDeferred<AKTWTestUnit>(AKTWTestUnit::StaticClass(),FTransform(P),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);G->Faction=1;G->bEnableCombat=false;G->bAutoPatrol=false;G->Order=EKTWUnitOrder::Defend;G->FinishSpawning(FTransform(P));*Guard=G;}),2.f,false);
 FTimerHandle ClearGuard;GetWorldTimerManager().SetTimer(ClearGuard,FTimerDelegate::CreateLambda([this,Guard,WC](){TestCheck(WC.IsValid()&&WC->Carried==0&&WC->ChannelProgress==0,TEXT("defender inside four hundred cancels raid"));if(Guard->IsValid())Guard->Get()->Destroy();}),3.3f,false);
 FTimerHandle Early;GetWorldTimerManager().SetTimer(Early,FTimerDelegate::CreateLambda([this,WC](){auto* M=AKTWMatch::Find(GetWorld());TestCheck(WC.IsValid()&&WC->Carried==0&&M->Accounts[1].Gold==100,TEXT("raid cannot pay before four seconds"));}),3.f,false);
 FTimerHandle Done;GetWorldTimerManager().SetTimer(Done,FTimerDelegate::CreateLambda([this,WU,WC](){
 auto* M=AKTWMatch::Find(GetWorld());if(WC.IsValid()&&WU.IsValid())UE_LOG(LogTemp,Display,TEXT("KTW RAID DIAGNOSTIC cargo=%d task=%d progress=%.2f pos=%s"),WC->Carried,int32(WC->Task),WC->ChannelProgress,*WU->GetActorLocation().ToString());TestCheck(WC.IsValid()&&WC->Carried==60&&M->Accounts[1].Gold==40&&GoldConserved(),TEXT("raid transfers exact capacity and conserves gold"));
 TestCheck(WC.IsValid()&&WC->Unrewarded==0&&M->Accounts[0].Cash==600,TEXT("stolen gold grants neither cash nor repeat experience"));
 if(WU.IsValid()){WU->SetActorLocation(AKTWStreetDirector::BasePosition(0)+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);}
 FTimerHandle Deposit;GetWorldTimerManager().SetTimer(Deposit,FTimerDelegate::CreateLambda([this,WC](){
 auto* M=AKTWMatch::Find(GetWorld());TestCheck(WC.IsValid()&&WC->Carried==0&&M->Accounts[0].Gold==60&&M->Accounts[1].Gold==40&&GoldConserved(),TEXT("stolen cargo scores only after home delivery"));
 auto Reset=[&](){M->Phase=EKTWMatchPhase::Battle;M->Winner=-2;M->BattleSeconds=0;M->Accounts[0].Gold=M->Accounts[1].Gold=0;M->HoldStarted[0]=M->HoldStarted[1]=-1;};
 Reset();M->Accounts[0].Gold=500;M->GoldChanged(0);M->BattleSeconds=14.9f;M->ResolveVictory();TestCheck(M->CanAct(),TEXT("target requires full fifteen second hold"));
 M->Accounts[0].Gold=499;M->GoldChanged(0);M->Accounts[0].Gold=500;M->GoldChanged(0);M->BattleSeconds=15;M->ResolveVictory();TestCheck(M->CanAct()&&M->HoldRemaining(0)>14,TEXT("raid below threshold resets hold"));
 M->BattleSeconds=30;M->ResolveVictory();TestCheck(M->Winner==0&&M->Phase==EKTWMatchPhase::Finished,TEXT("completed hold declares blue victory"));
 Reset();M->Accounts[0].Gold=M->Accounts[1].Gold=500;M->GoldChanged(0);M->GoldChanged(1);M->BattleSeconds=15;M->ResolveVictory();TestCheck(M->Winner==-1,TEXT("simultaneous equal holds draw"));
 Reset();M->Accounts[1].Gold=100;M->BattleSeconds=720;M->ResolveVictory();TestCheck(M->Winner==1,TEXT("deadline compares banked gold"));
 Reset();M->BattleSeconds=720;M->ResolveVictory();TestCheck(M->Winner==-1,TEXT("equal deadline banks draw"));
 TestCheck(!M->Purchase(0,EKTWRole::Runner,0),TEXT("finished match rejects recruit"));
 UE_LOG(LogTemp,Display,TEXT("KTW PHASE3 COMPLETE failures=%d"),TestFailures);FPlatformMisc::RequestExitWithStatus(false,TestFailures?1:0);
 }),1.5f,false);
 }),15.f,false);
#endif
}

float UKTWGoldCarrier::PickupSeconds()const{return Unit()&&Unit()->bRunner?.22f:.42f;}
float UKTWGoldCarrier::ChannelFraction()const{if(ChannelRequired<=0||ChannelStartedAt<0||(Task!=EKTWGoldTask::PickingUp&&Task!=EKTWGoldTask::Delivering&&Task!=EKTWGoldTask::Raiding))return 0;return FMath::Clamp(float(GetWorld()->GetTimeSeconds()-ChannelStartedAt)/ChannelRequired,0.f,1.f);}
float UKTWGoldCarrier::ChannelSecondsLeft()const{return ChannelRequired>0?ChannelRequired*(1-ChannelFraction()):0;}
