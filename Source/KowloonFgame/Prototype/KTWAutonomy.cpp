#include "KTWPrototype.h"
#include "KTWGold.h"
#include "KTWTactics.h"
#include "KTWGeneration.h"
#include "KTWSession.h"
#include "KTWWorkGuard.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"

void AKTWTestUnit::BeginLocalWork(){WorkAnchor=OrderAnchor;bHasLocalWork=true;bLaneDeparturePending=false;Order=EKTWUnitOrder::Advance;bAutoPatrol=true;CombatAnchor=GetActorLocation();if(auto* C=FindComponentByClass<UKTWGoldCarrier>())C->OnMoveArrived();}
bool AKTWTestUnit::LocalWorkStep(){if(!bHasLocalWork)return false;const double Now=GetWorld()->GetTimeSeconds();
 if(Now>=NextWorkPatrol||FVector::Dist2D(GetActorLocation(),WorkAnchor)>800){const float A=WorkPatrolStep++*90;const FVector P=WorkAnchor+FRotator(0,A,0).Vector()*230;FVector N;WorkPatrolGoal=FKTWTactics::ProjectOrder(this,P,Faction,N)?N:WorkAnchor;NextWorkPatrol=Now+3.5;}
 FKTWTactics::Move(this,WorkPatrolGoal,Faction,60);return true;
}
void AKTWBoss::ChooseAutomaticWork(){if(bPlayerOrdered||bHoldPosition||bLowHealthRetreat||Task!=EKTWBossTask::Sit||UKTWSession::Diagnostic())return;const double Now=GetWorld()->GetTimeSeconds();if(Now<NextWorkDecision||FVector::Dist2D(GetActorLocation(),TaskAnchor)>170)return;NextWorkDecision=Now+4;AKTWTestUnit* Partner=nullptr;float Best=-MAX_flt;for(TActorIterator<AKTWTestUnit>I(GetWorld());I;++I)if(I->Faction==Faction&&I->Health>0){const float D=FVector::Dist2D(GetActorLocation(),I->GetActorLocation());if(D>2600)continue;auto* G=I->FindComponentByClass<UKTWGoldCarrier>();const float Score=(G&&G->Carried>0?5000:!I->bRunner?1800:500)-D;if(Score>Best){Best=Score;Partner=*I;}}if(Partner){auto* G=Partner->FindComponentByClass<UKTWGoldCarrier>();IssueTask(G&&G->Carried>0?EKTWBossTask::Escort:EKTWBossTask::Follow,Partner->GetActorLocation(),Partner);return;}if(auto* G=AKTWGoldSystem::Find(GetWorld())){AKTWGoldPile* Pile=nullptr;float D=MAX_flt;for(AKTWGoldPile* Item:G->Piles)if(IsValid(Item)&&Item->SourceIndex>=0&&G->IsSafe(Item->GetActorLocation(),Faction)){const float Range=FVector::DistSquared2D(GetActorLocation(),Item->GetActorLocation());if(Range<D){D=Range;Pile=Item;}}if(Pile){const FVector Offset=FRotator(0,WorkPatrolStep++*90,0).Vector()*210;FVector P;if(FKTWTactics::ProjectOrder(this,Pile->GetActorLocation()+Offset,Faction,P))TaskAnchor=P;else TaskAnchor=Pile->GetActorLocation();PublishDestination(TaskAnchor,true);}}
}
