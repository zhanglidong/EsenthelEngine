/******************************************************************************/
#include "!Header.h"
/******************************************************************************
CLAMP, ALPHA, DUAL_HISTORY, GATHER

ALPHA=0
   ImgX=Weight
ALPHA=1
   ImgXY2=AlphaWeight, ImgX=CurAlpha
   
DUAL_HISTORY=1
   Img2=Old1

Img=Cur, Img1=Old, ImgXY=CurVel, ImgXY1=OldVel
/******************************************************************************/
#define TAA_WEIGHT (1.0/8) // FIXME should this be converted to INT? because it introduces some error due to values being 1.0/255

#define VEL_EPS 0.0006h

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif

#if ALPHA
   #undef DUAL_HISTORY // #TAADualAlpha
#endif

#define DUAL_ADJUST_OLD 0 // disable because didn't make any significant difference
/******************************************************************************/
BUFFER(TAA)
   Vec2 TAAOffset,
        TAAOffsetPrev;
   Half TAAAspectRatio;
BUFFER_END
/******************************************************************************/
void Test(VecH2 vel, VecH2 test_vel, in out Half old_weight)
{
   VecH2 delta_vel=vel-test_vel;
   delta_vel.x*=TAAAspectRatio; // 'delta_vel' is in UV 0..1 for both XY so mul X by aspect ratio
   if(Length2(delta_vel)>Sqr(VEL_EPS))old_weight=0;
}
void TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
          //NOPERSP Vec2 inPosXY:TEXCOORD1,
          //NOPERSP PIXEL                 ,
             #if ALPHA
                out VecH2 outWeight   :TARGET0,
             #else
                out Half  outWeight   :TARGET0,
             #endif
                out VecH4 outNext     :TARGET1,
                out VecH4 outOld      :TARGET2
             #if ALPHA && TAA_SEPARATE_ALPHA
              , out Half  outNextAlpha:TARGET3 // #TAADualAlpha
             #endif
             #if DUAL_HISTORY
              , out VecH4 outOld1     :TARGET3
             #endif
            )
{
#if   VEL_RT_MODE==VEL_RT_VECH2
   VEL_RT_TYPE vel=TexPoint(ImgXY, inTex).xy; //FIXME TAA, offset or gather?
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
      cs.set(cur_tex); if(CLAMP)cs.UVClamp(ImgClamp.xy, ImgClamp.zw);
      VecH4 cur      =Max(VecH4(0,0,0,0), cs.tex (Img )); // use Max(0) because of cubic sharpening potentially giving negative values
   #if ALPHA
      Half  cur_alpha=Sat(                cs.texX(ImgX)); // use Sat    because of cubic sharpening potentially giving negative values
   #endif
      cs.set(old_tex); if(CLAMP)cs.UVClamp(ImgClamp.xy, ImgClamp.zw);
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

#if ALPHA
   VecH2 tex=TexLod(ImgXY2, old_tex).xy;
#else
   Half  tex=TexLod(ImgX  , old_tex).x;
#endif

#if !ALPHA
   Half old_weight=tex.x;
#else
   Half old_weight=tex.y, old_alpha=tex.x;
#endif

   // if 'old_tex' is outside viewport then ignore it
   if(any(old_tex!=UVClamp(old_tex)))old_weight=0; // if(any(old_tex<ImgClamp.xy || old_tex>ImgClamp.zw))old_weight=0;

   // if old velocity is different then ignore old
#if 1
   Vec2 old_tex_vel=old_tex
                   +TAAOffsetPrev  // we're using 'old_tex_vel' to access texture from a previous frame that was not offseted
                   -TAAOffset    ; // we're comparing results to 'vel' accessed with 'inTex' instead of "inTex+TAAOffset"
   #if GATHER
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
         if(x>0 || y>0)Test(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(x, y)).xy, old_weight);
      old_tex_vel-=ImgSize.xy*0.5;
      VecH4 r=ImgXY1.GatherRed  (SamplerPoint, old_tex_vel);
      VecH4 g=ImgXY1.GatherGreen(SamplerPoint, old_tex_vel);
      Test(vel, VecH2(r.x, g.x), old_weight);
      Test(vel, VecH2(r.y, g.y), old_weight);
      Test(vel, VecH2(r.z, g.z), old_weight);
      Test(vel, VecH2(r.w, g.w), old_weight);
   #else
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
         Test(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(x, y)).xy, old_weight);
   #endif
#endif

#if !DUAL_HISTORY
      Half cur_weight=TAA_WEIGHT, total_weight=old_weight+cur_weight;
      old_weight/=total_weight;
      cur_weight/=total_weight;
      outOld=outNext=old*old_weight + cur*cur_weight;
   #if !ALPHA
      outWeight=total_weight;
   #else
      outWeight.y=total_weight;
      outWeight.x=old_alpha*old_weight + cur_alpha*cur_weight; // !! store alpha in X so it can be used for '_alpha' RT !!
      #if TAA_SEPARATE_ALPHA
         outNextAlpha=outWeight.x;
      #endif
   #endif
#else
   // #TAADualAlpha
   Half cur_weight=TAA_WEIGHT/2, total_weight=old_weight+cur_weight;
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT
   {
      outWeight=total_weight;
      outOld1=outOld=outNext=old*(old_weight/total_weight) + cur*(cur_weight/total_weight);
   }else // fill 2nd history RT
   {
      outWeight=total_weight;
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
