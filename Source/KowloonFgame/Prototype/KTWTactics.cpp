#include "KTWTactics.h"
#include "KTWStreetWar.h"
#include "KTWGeneration.h"
#include "KTWPrototype.h"
#include "KTWWorkGuard.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
bool FKTWTactics::Protected(FVector P,int32 F){const FVector D=P-AKTWStreetDirector::BasePosition(F);return FMath::Abs(D.X)<480&&FMath::Abs(D.Y)<480;}
bool FKTWTactics::Hostile(const ACharacter* Source,const AActor* Target){if(!IsValid(Source)||!IsValid(Target)||Source==Target)return false;int32 Team=-1;if(auto* U=Cast<AKTWTestUnit>(Source))Team=U->Faction;if(auto* B=Cast<AKTWBoss>(Source))Team=B->Faction;if(auto* U=Cast<AKTWTestUnit>(Target))return U->Health>0&&U->Faction!=Team;if(auto* B=Cast<AKTWBoss>(Target))return !B->bRecovering&&B->Faction!=Team;if(auto* G=Cast<AKTWSecurity>(Target))return G->Health>0;if(auto* O=Cast<AKTWFieldObject>(Target))return O->Kind==EKTWFieldKind::Base&&O->Health>0&&O->Faction!=Team;return false;}
bool FKTWTactics::AttackPoint(ACharacter* U,AActor* Target,int32 Faction,FVector& Point){if(!Hostile(U,Target))return false;const FVector Center=Target->GetActorLocation();if(!Cast<AKTWFieldObject>(Target)&&ProjectOrder(U,Center,Faction,Point))return true;float Best=MAX_flt;bool Found=false;for(int I=0;I<8;++I){const FVector Offset=FRotator(0,I*45.f,0).Vector()*850;FVector P;if(ProjectOrder(U,Center+Offset,Faction,P)){const float D=FVector::DistSquared2D(U->GetActorLocation(),P);if(D<Best){Best=D;Point=P;Found=true;}}}return Found;}
bool FKTWTactics::CanDamage(AActor* Source,AActor* Target){
 if(!Source||!Target)return true;
 if(auto* Object=Cast<AKTWFieldObject>(Target))return Object->Kind==EKTWFieldKind::Base&&Object->Health>0&&((Cast<AKTWTestUnit>(Source)&&Cast<AKTWTestUnit>(Source)->Faction!=Object->Faction)||(Cast<AKTWBoss>(Source)&&Cast<AKTWBoss>(Source)->Faction!=Object->Faction));
 if(Cast<AKTWSecurity>(Source)||Cast<AKTWSecurity>(Target))return true;
 auto Team=[](AActor* A){if(auto* U=Cast<AKTWTestUnit>(A))return U->Faction;if(auto* B=Cast<AKTWBoss>(A))return B->Faction;return -1;};
 const int32 A=Team(Source),B=Team(Target);if(A<0||B<0)return true;
 auto* War=AKTWStreetWar::Find(Source->GetWorld());return A!=B&&((War&&War->Late)||(!Protected(Source->GetActorLocation(),A)&&!Protected(Target->GetActorLocation(),B)));
}

bool FKTWTactics::CanSee(const ACharacter* Observer,const AActor* Target,float Radius){
 if(!IsValid(Observer)||!IsValid(Target)||FVector::DistSquared2D(Observer->GetActorLocation(),Target->GetActorLocation())>FMath::Square(Radius))return false;
 if(AKTWStreetWar::Concealed(Observer))return false;if(AKTWStreetWar::Concealed(Target))return false;
 FCollisionQueryParams Q;Q.AddIgnoredActor(Observer);Q.AddIgnoredActor(Target);FHitResult Hit;
 return !Observer->GetWorld()->LineTraceSingleByChannel(Hit,Observer->GetActorLocation(),Target->GetActorLocation(),ECC_Visibility,Q);
}
bool FKTWTactics::ProjectOrder(ACharacter* U,FVector P,int32 Faction,FVector& Out){
 if(!U||P.ContainsNaN()||!AKTWStreetDirector::IsRoadPosition(P)||Protected(P,1-Faction))return false;
 P.Z=100;auto* NS=FNavigationSystem::GetCurrent<UNavigationSystemV1>(U->GetWorld());FNavLocation N;
 if(!NS||!NS->ProjectPointToNavigation(P,N,FVector(100,100,250))||FVector::Dist2D(P,N.Location)>100)return false;
 if(FVector::Dist2D(U->GetActorLocation(),N.Location)>80){auto* Path=NS->FindPathToLocationSynchronously(U->GetWorld(),U->GetActorLocation(),N.Location,U,Filter(Faction));if(!Path||!Path->IsValid()||Path->IsPartial())return false;}
 Out=N.Location;return true;
}
void FKTWTactics::Move(ACharacter* U,FVector P,int32 Faction,float Acceptance,bool Immediate){
 auto* AI=U?Cast<AAIController>(U->GetController()):nullptr;if(!AI)return;
 if(FVector::Dist2D(U->GetActorLocation(),P)<=Acceptance){AI->StopMovement();return;}
 if(auto* Guard=U->FindComponentByClass<UKTWWorkGuard>()){if(Immediate)Guard->CancelRecovery();else if(Guard->IsDetouring())return;}
 if(!Immediate){auto* PF=AI->GetPathFollowingComponent();auto Path=PF?PF->GetPath():nullptr;if(Path.IsValid()&&PF->GetStatus()==EPathFollowingStatus::Moving&&FVector::Dist2D(Path->GetEndLocation(),P)<60)return;}
 auto* Guard=U->FindComponentByClass<UKTWWorkGuard>();if(Guard&&!Guard->RequestPath(P,Immediate))return;const auto Result=AI->MoveToLocation(P,Acceptance,false,true,true,false,Filter(Faction),false);if(Guard)Guard->PathFailed(Result==EPathFollowingRequestResult::Failed);
}
FVector FKTWTactics::EscortPoint(const ACharacter* Escort,const ACharacter* Target,float Distance){
 if(!Escort||!Target)return FVector::ZeroVector;
 FVector Direction=Target->GetVelocity().GetSafeNormal2D();if(Direction.IsNearlyZero())Direction=(Target->GetActorLocation()-Escort->GetActorLocation()).GetSafeNormal2D();
 if(Direction.IsNearlyZero())Direction=FVector(1,0,0);
 const FVector Side(-Direction.Y,Direction.X,0);return Target->GetActorLocation()-Direction*Distance+Side*(int32(Escort->GetUniqueID()%3)-1)*80;
}

ACharacter* FKTWTactics::FindEnemy(const ACharacter* U,int32 Team,float Radius){if(!U)return nullptr;ACharacter* Result=nullptr;float Best=Radius;for(TActorIterator<ACharacter>I(U->GetWorld());I;++I){bool Hostile=false;if(auto* V=Cast<AKTWTestUnit>(*I))Hostile=V->Faction!=Team&&V->Health>0;if(auto* B=Cast<AKTWBoss>(*I))Hostile=B->Faction!=Team&&!B->bRecovering;if(auto* G=Cast<AKTWSecurity>(*I))Hostile=G->Health>0;const float D=FVector::Dist2D(U->GetActorLocation(),I->GetActorLocation());if(Hostile&&D<Best&&CanDamage(const_cast<ACharacter*>(U),*I)&&CanSee(U,*I,Radius)){Best=D;Result=*I;}}return Result;}
