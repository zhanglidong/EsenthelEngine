/******************************************************************************/
#include "!Header.h"

#define TAA_WEIGHT (1.0/8)

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif
/******************************************************************************/
// Img=Old, Img1=New, ImgXY=Vel, ImgX=Alpha
BUFFER(TAA)
   Vec2 TAAOffset;
BUFFER_END
/******************************************************************************/
VecH4 TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
           //NOPERSP Vec2 inPosXY:TEXCOORD1,
           //NOPERSP PIXEL                 ,
                 out Half outWeight:TARGET1):TARGET
{
   #if   VEL_RT_MODE==VEL_RT_VECH2
     VEL_RT_TYPE vel=TexPoint(ImgXY, inTex).xy; //FIXME TAA
   //VEL_RT_TYPE vel=TexLod  (ImgXY, inTex+TAAOffset).xy;
   #elif VEL_RT_MODE==VEL_RT_VEC2
     VEL_RT_TYPE vel=TexPoint(ImgXYF, inTex).xy; //FIXME TAA
   //VEL_RT_TYPE vel=TexLod  (ImgXYF, inTex+TAAOffset).xy;
   #endif

#if !SIGNED_VEL_RT // convert 0..1 -> -1..1 (*2-1)
   vel=vel*2-1;
#endif

   Vec2 old_tex=inTex+vel;

#if CUBIC
   VecH4 old=Max(VecH4(0,0,0,0), TexCubicFast(Img , old_tex        )); // use Max(0) because of cubic sharpening potentially giving negative values
   VecH4 cur=Max(VecH4(0,0,0,0), TexCubicFast(Img1, inTex+TAAOffset)); // use Max(0) because of cubic sharpening potentially giving negative values
#else
   VecH4 old=TexLod(Img , old_tex        );
   VecH4 cur=TexLod(Img1, inTex+TAAOffset);
#endif

   Half old_weight=TexPoint(ImgX, inTex).x,
        cur_weight=TAA_WEIGHT;

   // if 'old_tex' is outside viewport then ignore it
   if(any(old_tex!=UVClamp(old_tex)))old_weight=0; // if(any(old_tex<ImgClamp.xy || old_tex>ImgClamp.zw))old_weight=0;

   //if(any(vel))old_weight=0; // FIXME TAA improve

   outWeight=old_weight+cur_weight;
   old_weight/=outWeight;
   cur_weight/=outWeight;
   VecH4 col=old*old_weight + cur*cur_weight;
   return col;
}
/******************************************************************************/
