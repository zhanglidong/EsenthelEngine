/******************************************************************************/
#include "!Header.h"

#define TAA_WEIGHT (1.0/8) // FIXME should this be converted to INT? because it introduces some error due to values being 1.0/255

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif
/******************************************************************************/
// ImgX=Weight, Img=Cur, Img1=Old, Img2=Old1, ImgXY=Vel
BUFFER(TAA)
   Vec2 TAAOffset;
BUFFER_END
/******************************************************************************/
void TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
           //NOPERSP Vec2 inPosXY:TEXCOORD1,
           //NOPERSP PIXEL                 ,
                 out Half  outWeight:TARGET0,
                 out VecH4 outNext  :TARGET1,
                 out VecH4 outOld   :TARGET2
               #if DUAL_HISTORY
               , out VecH4 outOld1  :TARGET3
               #endif
            )
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
      CubicFastSampler cs; cs.set(old_tex);
      VecH4 cur =Max(VecH4(0,0,0,0), TexCubicFast(Img , inTex+TAAOffset)); // use Max(0) because of cubic sharpening potentially giving negative values
      VecH4 old =Max(VecH4(0,0,0,0),       cs.tex(Img1)                 ); // use Max(0) because of cubic sharpening potentially giving negative values
   #if DUAL_HISTORY
      VecH4 old1=Max(VecH4(0,0,0,0),       cs.tex(Img2)                 ); // use Max(0) because of cubic sharpening potentially giving negative values
   #endif
#else
      VecH4 cur =TexLod(Img , inTex+TAAOffset);
      VecH4 old =TexLod(Img1, old_tex        );
   #if DUAL_HISTORY
      VecH4 old1=TexLod(Img2, old_tex        );
   #endif
#endif

   Half old_weight=TexLod(ImgX, old_tex).x;

   // if 'old_tex' is outside viewport then ignore it
   if(any(old_tex!=UVClamp(old_tex)))old_weight=0; // if(any(old_tex<ImgClamp.xy || old_tex>ImgClamp.zw))old_weight=0;

   //if(any(vel))old_weight=0; // FIXME TAA improve

#if !DUAL_HISTORY
   Half cur_weight=TAA_WEIGHT;
   outWeight=old_weight+cur_weight;
   outOld=outNext=old*(old_weight/outWeight) + cur*(cur_weight/outWeight);
#else
   Half cur_weight=TAA_WEIGHT/2;
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT
   {
      outWeight=old_weight+cur_weight;
      outOld1=outOld=outNext=old*(old_weight/outWeight) + cur*(cur_weight/outWeight);
   }else // fill 2nd history RT
   {
      outWeight=old_weight+cur_weight;
      outOld=old;
      Half old_weight1=old_weight-0.5, total=old_weight1+cur_weight;
      outOld1=old1*(old_weight1/total) + cur*(cur_weight/total);

      outNext=Lerp(outOld, outOld1, total*2);

      if(total>=0.5 - cur_weight/2) // filled all RT's
      {
         outOld=outOld1; // move Old1 to Old
         outWeight=0.5; // mark Old1 as empty
      }
   }
#endif
}
/******************************************************************************/
