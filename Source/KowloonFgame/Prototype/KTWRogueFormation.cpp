#include "KTWRogue.h"
#include "KTWSession.h"

int32 AKTWRogueRun::EnemyBudget(int32 WaveIndex)const{
 int32 Budget=0;if(Waves.IsValidIndex(WaveIndex))for(int32 TypeIndex:Waves[WaveIndex].Soldiers)Budget+=SoldierTypes[TypeIndex].Cost;return Budget;
}

TArray<FVector> AKTWRogueRun::EnemyFormation(int32 WaveIndex)const{
 TArray<FVector> Result;if(!Waves.IsValidIndex(WaveIndex))return Result;
 const auto* Session=UKTWSession::Get(this);
 FRandomStream Layout(int32(HashCombineFast(uint32(Session?Session->RoundSeed:5990),uint32(WaveIndex+1)*7919u)));
 int FrontCount=0,PipeCount=0;for(int TypeIndex:Waves[WaveIndex].Soldiers){const auto& Soldier=SoldierTypes[TypeIndex];FrontCount+=!Soldier.Ranged&&Soldier.Visual!=4;PipeCount+=Soldier.Visual==2;}
 int Front=0,Ranged=0,Pipes=0,Flank=0;
 for(int32 TypeIndex:Waves[WaveIndex].Soldiers){
  const auto& Soldier=SoldierTypes[TypeIndex];FVector Point;
  if(Soldier.Visual==4){Point=FVector(820,0,0);}
  else if(Soldier.Ranged){
   const float Rows[]={-100,100,-300,300,-500,500};Point=FVector(WaveIndex==4&&Soldier.Visual==3?1450:1210,Rows[FMath::Min(Ranged++,5)],0);
  }else if(WaveIndex==0){Point=FVector(520+Front%2*150,(Front-1)*280,0);++Front;}
  else if(WaveIndex==1){const float Side=Front%2?1:-1;Point=FVector(460+(Front/2)*180,Side*(330+(Front/2)*115),0);++Front;}
  else if(WaveIndex==2){if(Soldier.Visual==2){Point=FVector(450,(Pipes++-(PipeCount-1)*.5f)*240,0);}else{Point=FVector(720,(Flank++?1:-1)*510,0);}}
  else if(WaveIndex==3){Point=FVector(440+Front%2*250,(Front-(FrontCount-1)*.5f)*180,0);++Front;}
  else{if(Soldier.Visual==2){Point=FVector(440,(Pipes++-(PipeCount-1)*.5f)*270,0);}else{const float Rows[]={-530,-350,350,530};Point=FVector(680,Rows[FMath::Min(Flank++,3)],0);}}
  Point.X+=Layout.FRandRange(-18,18);Point.Y+=Layout.FRandRange(-20,20);Result.Add(Point);
 }
 return Result;
}

bool AKTWRogueRun::PlaceSquad(const TArray<AKTWRogueUnit*>& Squad,const TArray<FVector>& Goals){
 if(Phase!=EKTWRoguePhase::Deployment||Squad.IsEmpty()||Squad.Num()!=Goals.Num())return false;
 // Validate every destination before moving anything, including swaps of occupied slots.
 for(int I=0;I<Squad.Num();++I){
  auto* Unit=Squad[I];if(!IsValid(Unit)||!Unit->Alive||Unit->Team!=0||!Positions.IsValidIndex(Unit->RosterSlot))return false;
  const float Radius=36*Unit->Stats.Scale;const FVector Point=Goals[I];
  if(Point.ContainsNaN()||Point.X<-1700+Radius||Point.X>-200-Radius||FMath::Abs(Point.Y)>800-Radius)return false;
  for(auto Other:Units)if(Other&&Other->Alive&&Other->Team==0&&!Squad.Contains(Other.Get())&&FVector::Dist2D(Point,Other->GetActorLocation())<(Radius+36*Other->Stats.Scale)*1.1f)return false;
  for(int J=0;J<I;++J)if(Squad[J]==Unit||FVector::Dist2D(Point,Goals[J])<(Radius+36*Squad[J]->Stats.Scale)*1.1f)return false;
 }
 for(int I=0;I<Squad.Num();++I){auto* Unit=Squad[I];FVector Point=Goals[I];Point.Z=88*Unit->Stats.Scale+4;Unit->SetActorLocation(Point);Positions[Unit->RosterSlot]=FVector(Point.X,Point.Y,0);}
 Notice=FString::Printf(TEXT("已调整 %d 名单位的位置。空格开始战斗。"),Squad.Num());return true;
}
