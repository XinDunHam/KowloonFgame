#include "KTWDynamics.h"
#include "KTWPrototype.h"
#include "KTWSession.h"
#include "KTWMatch.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Materials/MaterialInterface.h"

void AKTWWeather::ResetDrop(int32 Index,bool Initial){auto* PC=GetWorld()->GetFirstPlayerController();const FVector Eye=PC&&PC->PlayerCameraManager?PC->PlayerCameraManager->GetCameraLocation():FVector::ZeroVector;const bool Near=Index>=320;FVector V(Near?Eye.X+Random.FRandRange(-2200,2200):Random.FRandRange(-7200,7200),Near?Eye.Y+Random.FRandRange(-2200,2200):Random.FRandRange(-7200,7200),7000);V.X=FMath::Clamp(V.X,-8000.,8000.);V.Y=FMath::Clamp(V.Y,-8000.,8000.);FHitResult H;const bool Hit=GetWorld()->LineTraceSingleByObjectType(H,V,FVector(V.X,V.Y,-100),FCollisionObjectQueryParams(ECC_WorldStatic));DropFloors[Index]=Hit?H.ImpactPoint.Z:0;V.Z=Initial?Random.FRandRange(DropFloors[Index]+80,7000):7000;Drops[Index]=V;}
void AKTWWeather::BuildPrecipitation(){Drops.Empty();DropFloors.Empty();Particles->ClearInstances();const int32 N=Preset>=2?560:0;Drops.SetNum(N);DropFloors.SetNum(N);for(int32 I=0;I<N;++I){ResetDrop(I,true);const float S=Preset==4?.12f:.018f;Particles->AddInstance(FTransform(FRotator(0,0,Preset==4?0:-7),Drops[I],Preset==4?FVector(S):FVector(S,S,2.2f)));}Lightning->ClearInstances();Splashes->ClearInstances();SplashTimes.Init(-100,32);SplashLocations.Init(FVector::ZeroVector,32);for(int32 I=0;I<32;++I)Splashes->AddInstance(FTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector::ZeroVector));if(StormLight)StormLight->SetIntensity(0);}
void AKTWWeather::TriggerLightning(){if(Preset!=3)return;Lightning->ClearInstances();FVector End(Random.FRandRange(-3800,3800),Random.FRandRange(-3800,3800),900);if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* Cam=Cast<AKTWCameraPawn>(PC->GetPawn()))if(Cam->bStreetView){const FVector Forward=FRotator(0,Cam->OrbitYaw,0).Vector(),Side(-Forward.Y,Forward.X,0);End=Cam->FocusGoal+Forward*Random.FRandRange(1800,4000)+Side*Random.FRandRange(-900,900);End.X=FMath::Clamp(End.X,-6500.,6500.);End.Y=FMath::Clamp(End.Y,-6500.,6500.);End.Z=900;}FVector P=End+FVector(Random.FRandRange(-900,900),Random.FRandRange(-900,900),6500);
 auto Segment=[&](FVector A,FVector B,float Radius){const FVector V=B-A;Lightning->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,V.GetSafeNormal()),(A+B)*.5,FVector(Radius,Radius,V.Size()/100)));};
 for(int32 I=0;I<12;++I){const FVector Next=FMath::Lerp(End+FVector(0,0,6500),End,(I+1)/12.f)+FVector(I==11?0:Random.FRandRange(-220,220),I==11?0:Random.FRandRange(-220,220),0);Segment(P,Next,.10f);if(I==3||I==6){FVector B=P;for(int32 J=0;J<4;++J){FVector C=B+FVector(Random.FRandRange(180,360),Random.FRandRange(-320,-160),-280);Segment(B,C,.055f);B=C;}}P=Next;}
 const float Now=GetWorld()->GetTimeSeconds();FlashUntil=Now+.22f;StormLight->SetWorldLocation(End+FVector(0,0,1800));StormLight->SetIntensity(28000);++LightningEvents;float Delay=.2f;if(auto* PC=GetWorld()->GetFirstPlayerController())if(PC->PlayerCameraManager)Delay=FMath::Clamp(FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(),End)/34300.f,.12f,1.2f);
 if(!ThunderAttenuation){ThunderAttenuation=NewObject<USoundAttenuation>(this);auto& A=ThunderAttenuation->Attenuation;A.bAttenuate=true;A.bSpatialize=true;A.AttenuationShape=EAttenuationShape::Sphere;A.AttenuationShapeExtents=FVector(10000);A.FalloffDistance=24000;}
 FTimerHandle T;TWeakObjectPtr<AKTWWeather> Self=this;GetWorldTimerManager().SetTimer(T,FTimerDelegate::CreateLambda([Self,End](){if(!Self.IsValid()||Self->Preset!=3||FParse::Param(FCommandLine::Get(),TEXT("nosound")))return;auto* S=UKTWSession::Get(Self.Get());if(S&&!S->SoundEnabled)return;if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/KTW/Audio/S_Thunder.S_Thunder")))UGameplayStatics::PlaySoundAtLocation(Self.Get(),Sound,End,FRotator::ZeroRotator,.3f,1,0,Self->ThunderAttenuation);}),Delay,false);
}
void AKTWWeather::Tick(float D){Super::Tick(D);const float Now=GetWorld()->GetTimeSeconds();if(WeatherAudio)if(auto* S=UKTWSession::Get(this))WeatherAudio->SetPaused(!S->SoundEnabled);if(Preset==3&&Now>=NextFlash){TriggerLightning();NextFlash=Now+Random.FRandRange(8,16);}if(Preset==3){for(TActorIterator<ADirectionalLight>I(GetWorld());I;++I)I->GetLightComponent()->SetIntensity(Now<FlashUntil?6.f:AmbientSun);if(Now>=FlashUntil){Lightning->ClearInstances();StormLight->SetIntensity(0);}}
 FVector Eye=FVector::ZeroVector;if(auto* PC=GetWorld()->GetFirstPlayerController())if(PC->PlayerCameraManager)Eye=PC->PlayerCameraManager->GetCameraLocation();
 for(int32 I=0;I<Drops.Num();++I){auto& V=Drops[I];V.Z-=D*(Preset==4?260:3300);V.X+=D*(Preset==4?65:180);if(V.Z<=DropFloors[I]+10){if(Preset!=4){const int32 S=NextSplash++%32;SplashTimes[S]=Now;SplashLocations[S]=FVector(V.X,V.Y,DropFloors[I]+8);}ResetDrop(I,false);}else if(I>=320&&FVector::Dist2D(V,Eye)>3400)ResetDrop(I,true);const float S=Preset==4?.12f:.018f;Particles->UpdateInstanceTransform(I,FTransform(FRotator(Preset==4?Now*35:0,0,Preset==4?Now*25:-7),V,Preset==4?FVector(S):FVector(S,S,2.2f)),false,false);}
 if(Drops.Num())Particles->MarkRenderStateDirty();for(int32 I=0;I<SplashTimes.Num();++I){const float T=Now-SplashTimes[I],S=T>=0&&T<.28f?(1-T/.28f)*.35f:0;Splashes->UpdateInstanceTransform(I,FTransform(FRotator::ZeroRotator,SplashLocations[I],FVector(S,S,.012f*S)),false,false);}if(SplashTimes.Num())Splashes->MarkRenderStateDirty();
}
// Weather is entirely in world space; no screen-space rain/snow drawing remains.
void AKTWWeather::Draw(AKTWPrototypeHUD*){}
