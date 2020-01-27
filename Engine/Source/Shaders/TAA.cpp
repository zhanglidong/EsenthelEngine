/******************************************************************************/
#include "!Header.h"

#define TAA_WEIGHT (1.0/8) // FIXME should this be converted to INT? because it introduces some error due to values being 1.0/255

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif

#if ALPHA
   #undef DUAL_HISTORY // #TAADualAlpha
#endif

#define DUAL_ADJUST_OLD 0 // disable because didn't make any significant difference
/******************************************************************************
ALPHA=0
   ImgX=Weight
ALPHA=1
   ImgXY1=AlphaWeight, ImgX=CurAlpha
   
DUAL_HISTORY=1
   Img2=Old1

Img=Cur, Img1=Old, ImgXY=Vel */
BUFFER(TAA)
   Vec2 TAAOffset;
BUFFER_END
/******************************************************************************/
void TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
          //NOPERSP Vec2 inPosXY:TEXCOORD1,
          //NOPERSP PIXEL                 ,
               #if !ALPHA
                 out Half  outWeight     :TARGET0,
               #else
                 out VecH2 outAlphaWeight:TARGET0,
               #endif
                 out VecH4 outNext       :TARGET1,
                 out VecH4 outOld        :TARGET2
               #if ALPHA && TAA_SEPARATE_ALPHA
               , out Half  outNextAlpha  :TARGET3 // #TAADualAlpha
               #endif
               #if DUAL_HISTORY
               , out VecH4 outOld1       :TARGET3
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

   Vec2 cur_tex=inTex+TAAOffset,
        old_tex=inTex+vel;

#if CUBIC
      CubicFastSampler cs;
      cs.set(cur_tex);
      VecH4 cur      =Max(VecH4(0,0,0,0), cs.tex (Img )); // use Max(0) because of cubic sharpening potentially giving negative values
   #if ALPHA
      Half  cur_alpha=Sat(                cs.texX(ImgX)); // use Sat    because of cubic sharpening potentially giving negative values
   #endif
      cs.set(old_tex);
      VecH4 old      =Max(VecH4(0,0,0,0), cs.tex (Img1)); // use Max(0) because of cubic sharpening potentially giving negative values
   #if DUAL_HISTORY
      VecH4 old1     =Max(VecH4(0,0,0,0), cs.tex (Img2)); // use Max(0) because of cubic sharpening potentially giving negative values
   #endif
#else
      VecH4 cur      =TexLod(Img , cur_tex);
   #if ALPHA
      Half  cur_alpha=TexLod(ImgX, cur_tex);
   #endif
      VecH4 old      =TexLod(Img1, old_tex);
   #if DUAL_HISTORY
      VecH4 old1     =TexLod(Img2, old_tex);
   #endif
#endif

#if !ALPHA
   Half old_weight=TexLod(ImgX, old_tex).x;
#else
   VecH2 tex=TexLod(ImgXY1, old_tex).xy;
   Half  old_alpha=tex.x, old_weight=tex.y;
#endif

   // if 'old_tex' is outside viewport then ignore it
   if(any(old_tex!=UVClamp(old_tex)))old_weight=0; // if(any(old_tex<ImgClamp.xy || old_tex>ImgClamp.zw))old_weight=0;

   //if(any(vel))old_weight=0; // FIXME TAA improve

#if !DUAL_HISTORY
      Half cur_weight=TAA_WEIGHT, total_weight=old_weight+cur_weight;
      old_weight/=total_weight;
      cur_weight/=total_weight;
      outOld=outNext=old*old_weight + cur*cur_weight;
   #if !ALPHA
      outWeight=total_weight;
   #else
      outAlphaWeight=VecH2(old_alpha*old_weight + cur_alpha*cur_weight, total_weight); // !! store alpha in X so it can be used for '_alpha' RT !!
      #if TAA_SEPARATE_ALPHA
         outNextAlpha=outAlphaWeight.x;
      #endif
   #endif
#else
   // #TAADualAlpha
   Half cur_weight=TAA_WEIGHT/2;
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT
   {
      outWeight=old_weight+cur_weight;
      outOld1=outOld=outNext=old*(old_weight/outWeight) + cur*(cur_weight/outWeight);
   }else // fill 2nd history RT
   {
      outWeight=old_weight+cur_weight;
      if(DUAL_ADJUST_OLD)
      {
            Half ow=1, cw=TAA_WEIGHT, tw=ow+cw;
            outOld=old*(ow/tw) + cur*(cw/tw);
      }else outOld=old;

      Half old_weight1=old_weight-0.5, total=old_weight1+cur_weight;
      outOld1=old1*(old_weight1/total) + cur*(cur_weight/total);

      outNext=Lerp(outOld, outOld1, DUAL_ADJUST_OLD ? Sqr(total*2) : total*2);

      if(total>=0.5 - cur_weight/2) // filled all RT's
      {
         outOld=outOld1; // move Old1 to Old
         outWeight=0.5; // mark Old1 as empty
      }
   }
#endif
}
/******************************************************************************/
