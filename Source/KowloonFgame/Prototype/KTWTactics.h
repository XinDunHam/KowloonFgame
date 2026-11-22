#pragma once
#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "KTWTactics.generated.h"
UCLASS() class KOWLOONFGAME_API UKTWBlueSafeArea:public UNavArea{GENERATED_BODY()};
UCLASS() class KOWLOONFGAME_API UKTWRedSafeArea:public UNavArea{GENERATED_BODY()};
UCLASS() class KOWLOONFGAME_API UKTWBlueFilter:public UNavigationQueryFilter{GENERATED_BODY() public:UKTWBlueFilter(){AddExcludedArea(UKTWRedSafeArea::StaticClass());}};
UCLASS() class KOWLOONFGAME_API UKTWRedFilter:public UNavigationQueryFilter{GENERATED_BODY() public:UKTWRedFilter(){AddExcludedArea(UKTWBlueSafeArea::StaticClass());}};
struct FKTWTactics{
 static TSubclassOf<UNavigationQueryFilter> Filter(int32 F){return F==0?UKTWBlueFilter::StaticClass():UKTWRedFilter::StaticClass();}
 static bool Protected(FVector P,int32 F);
 static bool CanDamage(AActor* Source,AActor* Target);
 static bool Hostile(const class ACharacter* Source,const AActor* Target);
 static bool AttackPoint(class ACharacter* Unit,AActor* Target,int32 Faction,FVector& Point);
 static class ACharacter* FindEnemy(const class ACharacter* Observer,int32 Faction,float Radius=1400);
 static bool CanSee(const class ACharacter* Observer,const AActor* Target,float Radius=1800);
 static bool ProjectOrder(class ACharacter* Unit,FVector Requested,int32 Faction,FVector& Point);
 static void Move(class ACharacter* Unit,FVector Point,int32 Faction,float Acceptance=70,bool Immediate=false);
 static FVector EscortPoint(const class ACharacter* Escort,const class ACharacter* Target,float Distance=300);

};
