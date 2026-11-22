#include "KTWAI.h"
#include "KTWPrototype.h"
#include "KTWTactics.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
void AKTWBossAIController::OnPossess(APawn* InPawn){
 Super::OnPossess(InPawn);
 if(auto* Tree=LoadObject<UBehaviorTree>(nullptr,TEXT("/Game/KTW/AI/BT_Boss_V4.BT_Boss_V4"))){
  RunBehaviorTree(Tree);
  if(auto* BB=GetBlackboardComponent()){BB->SetValueAsVector(TEXT("Goal"),InPawn->GetActorLocation());BB->SetValueAsBool(TEXT("Recovering"),false);BB->SetValueAsBool(TEXT("Retreating"),false);}
 }
}
UBTTask_KTWMoveOrder::UBTTask_KTWMoveOrder(){NodeName=TEXT("Execute latest valid tactical destination");}
EBTNodeResult::Type UBTTask_KTWMoveOrder::ExecuteTask(UBehaviorTreeComponent& C,uint8* Memory){
 auto* AI=C.GetAIOwner();auto* BB=C.GetBlackboardComponent();if(!AI||!BB||!AI->GetPawn())return EBTNodeResult::Failed;
 if(BB->GetValueAsBool(TEXT("Recovering"))){AI->StopMovement();return EBTNodeResult::Succeeded;}
 FVector Goal=BB->GetValueAsVector(TEXT("Goal"));
 if(auto* B=Cast<AKTWBoss>(AI->GetPawn()))FKTWTactics::Move(B,Goal,B->Faction,70);
 return EBTNodeResult::Succeeded;
}
UBTDecorator_KTWBossState::UBTDecorator_KTWBossState(){NodeName=TEXT("Boss recovery / retreat state");}
bool UBTDecorator_KTWBossState::CalculateRawConditionValue(UBehaviorTreeComponent& C,uint8*)const{
 auto* BB=C.GetBlackboardComponent();return BB&&BB->GetValueAsBool(bRecoveryBranch?TEXT("Recovering"):TEXT("Retreating"));
}

