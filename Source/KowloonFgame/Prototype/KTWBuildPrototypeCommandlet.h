#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "KTWBuildPrototypeCommandlet.generated.h"
UCLASS()
class KOWLOONFGAME_API UKTWBuildPrototypeCommandlet : public UCommandlet
{
 GENERATED_BODY()
public:
 UKTWBuildPrototypeCommandlet();
 virtual int32 Main(const FString& Params) override;
};
