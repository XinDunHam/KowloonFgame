#pragma once
#include "KTWPrototype.h"

// Original procedural city artwork; normalized coordinates keep it resolution independent.
namespace KTWMistTheme {
inline void Backdrop(AKTWPrototypeHUD* H,float W,float V,float Time){
 for(int32 I=0;I<100;++I){const float T=I/99.f;H->DrawRect(FLinearColor(.025f+.12f*(1-T),.055f+.24f*(1-T),.07f+.25f*(1-T),1),0,V*T,W,V/99+1);}
 for(int32 Layer=0;Layer<3;++Layer)for(int32 I=0;I<22;++I){const float X=(I/22.f)*W,Width=W/22.f*.82f,Height=V*(.13f+FMath::Frac(FMath::Sin(I*31.7f+Layer*19.3f)*43758.f)*.33f),Base=V*(.69f+Layer*.13f);const float K=.085f-Layer*.029f;H->DrawRect(FLinearColor(K,K*1.8f,K*2,1),X,Base-Height,Width,Height+V*.25f);H->DrawRect(FLinearColor(K*.8f,K*1.7f,K*1.8f,1),X+Width*.15f,Base-Height-8,Width*.64f,12);for(int32 R=0;R<7;++R)for(int32 C=0;C<3;++C)if((I+R+C+Layer)%4==0)H->DrawRect(FLinearColor(.33f,.55f,.53f,.25f),X+Width*(.18f+C*.26f),Base-Height+16+R*18,3,6);}
 // Mist ribbons drift slowly between the skyline and the foreground.
 for(int32 I=0;I<32;++I){const float Y=V*(.38f+I*.018f)+FMath::Sin(Time*.14f+I*.6f)*V*.012f;H->DrawRect(FLinearColor(.25f,.42f,.4f,.018f),0,Y,W*.55f,V*.025f);}
 auto Figure=[&](float X,float Y,float Scale,float Lean){auto L=[&](float A,float B,float C,float D,float Thick){H->DrawLine(X+A*Scale,Y+B*Scale,X+C*Scale,Y+D*Scale,FLinearColor(.008f,.022f,.029f,1),Thick*Scale);};L(0,-86,Lean,-35,32);L(Lean,-35,-24,17,19);L(Lean,-35,37,22,18);L(0,-77,-30,-44,14);L(-30,-44,-49,-68,12);L(2,-72,35,-53,15);L(35,-53,55,-78,10);L(52,-81,81,-123,5);H->DrawRect(FLinearColor(.008f,.022f,.029f,1),X-10*Scale,Y-119*Scale,21*Scale,26*Scale);};
 Figure(W*.16f,V*.95f,V*.0026f,-12);Figure(W*.43f,V*1.02f,V*.0031f,8);
 for(int32 I=0;I<38;++I){const float X=FMath::Frac(I*.618f)*W*.6f,Y=FMath::Frac(I*.317f+Time*.009f)*V;H->DrawLine(X,Y,X+2,Y-8,FLinearColor(.66f,.87f,.82f,.18f),1);}
 // Dark right-hand scrim reserves a quiet, readable area for controls.
 for(int32 I=0;I<45;++I)H->DrawRect(FLinearColor(.006f,.018f,.025f,.025f+I*.0017f),W*(.46f+I*.012f),0,W,V);
}
}
