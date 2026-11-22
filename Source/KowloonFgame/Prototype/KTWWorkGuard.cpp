#include "KTWWorkGuard.h"
#include "KTWPrototype.h"
#include "KTWMatch.h"
#include "KTWGeneration.h"
#include "KTWCombatAction.h"
#include "KTWGold.h"
#include "KTWTactics.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"

UKTWWorkGuard::UKTWWorkGuard(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.35f;}
bool UKTWWorkGuard::IsDetouring()const{return GetWorld()&&GetWorld()->GetTimeSeconds()<DetourUntil;}
bool UKTWWorkGuard::RequestPath(FVector Goal,bool Immediate){const double Now=GetWorld()->GetTimeSeconds();if(!Immediate&&Now-RequestedAt<.25&&FVector::Dist2D(Goal,RequestedGoal)<200)return false;RequestedAt=Now;RequestedGoal=Goal;return true;}
void UKTWWorkGuard::CancelRecovery(){DetourUntil=-1;LastMotionAt=GetWorld()->GetTimeSeconds();LastPosition=GetOwner()->GetActorLocation();}
void UKTWWorkGuard::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);auto* M=AKTWMatch::Find(GetWorld());auto* C=Cast<ACharacter>(GetOwner());if(!M||!M->CanAct()||!C)return;auto* U=Cast<AKTWTestUnit>(C);auto* B=Cast<AKTWBoss>(C);if((U&&U->Health<=0)||(B&&B->bRecovering))return;const int32 Team=U?U->Faction:B?B->Faction:0;auto* AI=Cast<AAIController>(C->GetController());if(!AI){C->SpawnDefaultController();return;}const double Now=GetWorld()->GetTimeSeconds();const FVector P=C->GetActorLocation();if(LastMotionAt<0||FVector::Dist2D(P,LastPosition)>28){LastMotionAt=Now;LastPosition=P;}if(IsDetouring())return;if(DetourUntil>0){DetourUntil=-1;FKTWTactics::Move(C,RecoveryGoal,Team,70,true);LastMotionAt=Now;return;}
 auto* A=C->FindComponentByClass<UKTWCombatAction>();auto* G=C->FindComponentByClass<UKTWGoldCarrier>();if((A&&A->bPlaying)||(G&&(G->Task==EKTWGoldTask::PickingUp||G->Task==EKTWGoldTask::Delivering||G->Task==EKTWGoldTask::Raiding||G->Task==EKTWGoldTask::Healing))){LastMotionAt=Now;return;}auto* PF=AI->GetPathFollowingComponent();auto Path=PF?PF->GetPath():nullptr;if((!bPathFailed&&(!Path.IsValid()||PF->GetStatus()!=EPathFollowingStatus::Moving))||FVector::Dist2D(P,bPathFailed?RequestedGoal:Path->GetEndLocation())<150){LastMotionAt=Now;return;}if(Now-LastMotionAt<2.5||Now-LastAttempt<2.5)return;LastAttempt=Now;RecoveryGoal=bPathFailed?RequestedGoal:Path->GetEndLocation();bPathFailed=false;AI->StopMovement();++Recoveries;
 auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());bool Detour=false;for(int I=0;NS&&I<6&&!Detour;++I){const float Angle=(I+Recoveries%2)*60.f;const FVector Candidate=P+FRotator(0,Angle,0).Vector()*220;FNavLocation N;if(!AKTWStreetDirector::IsRoadPosition(Candidate,-80)||FKTWTactics::Protected(Candidate,1-Team)||!NS->ProjectPointToNavigation(Candidate,N,FVector(80,80,200)))continue;auto Test=NS->FindPathToLocationSynchronously(GetWorld(),P,N.Location,C,FKTWTactics::Filter(Team));if(!Test||!Test->IsValid()||Test->IsPartial())continue;DetourUntil=Now+.8;AI->MoveToLocation(N.Location,35,false,true,true,false,FKTWTactics::Filter(Team),false);Detour=true;}
 if(!Detour)FKTWTactics::Move(C,RecoveryGoal,Team,65,true);LastMotionAt=Now;
}
