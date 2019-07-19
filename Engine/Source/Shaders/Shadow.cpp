/******************************************************************************/
#include "!Header.h"
#ifndef MAP_NUM
#define MAP_NUM 0
#endif
#ifndef CLOUD
#define CLOUD 0
#endif
#ifndef SAMPLES
#define SAMPLES 0
#endif
#ifndef RANGE
#define RANGE 0
#endif
/******************************************************************************/
// SHADOW SET
/******************************************************************************/
Half ShdDir_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1,
               NOPERSP PIXEL                 
            #if MULTI_SAMPLE
                     , UInt index  :SV_SampleIndex
            #endif
              ):TARGET
{
#if MULTI_SAMPLE
   return ShadowDirValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true, MAP_NUM, CLOUD);
#else
   return ShadowDirValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true, MAP_NUM, CLOUD);
#endif
}
/******************************************************************************/
Half ShdPoint_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL                 
              #if MULTI_SAMPLE
                       , UInt index  :SV_SampleIndex
              #endif
                ):TARGET
{
#if MULTI_SAMPLE
   return ShadowPointValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
#else
   return ShadowPointValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
#endif
}
/******************************************************************************/
Half ShdCone_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                NOPERSP Vec2 inPosXY:TEXCOORD1,
                NOPERSP PIXEL                 
             #if MULTI_SAMPLE
                      , UInt index  :SV_SampleIndex
             #endif
               ):TARGET
{
#if MULTI_SAMPLE
   return ShadowConeValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
#else
   return ShadowConeValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
#endif
}
/******************************************************************************/
// SHADOW BLUR
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
Half ShdBlur_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Half weight=0.25,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z);
   UNROLL for(Int i=0; i<SAMPLES; i++)
   {
      Vec2 t;
      if(SAMPLES== 4)t=RTSize.xy*BlendOfs4 [i]+inTex;
    //if(SAMPLES== 5)t=RTSize.xy*BlendOfs5 [i]+inTex;
      if(SAMPLES== 6)t=RTSize.xy*BlendOfs6 [i]+inTex;
      if(SAMPLES== 8)t=RTSize.xy*BlendOfs8 [i]+inTex;
    //if(SAMPLES== 9)t=RTSize.xy*BlendOfs9 [i]+inTex;
      if(SAMPLES==12)t=RTSize.xy*BlendOfs12[i]+inTex;
    //if(SAMPLES==13)t=RTSize.xy*BlendOfs13[i]+inTex;
      // use linear filtering because texcoords are not rounded
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurX_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.y=inTex.y;
   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.x=RTSize.x*(2*i+((i>0) ? -0.5 : 0.5))+inTex.x;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurY_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.x=inTex.x;
   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.y=RTSize.y*(2*i+((i>0) ? -0.5 : 0.5))+inTex.y;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
/******************************************************************************/
