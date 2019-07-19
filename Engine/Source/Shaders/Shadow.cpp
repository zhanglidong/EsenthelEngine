/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
Half ShdDir_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1,
               NOPERSP PIXEL                 ,
               uniform Int  num              ,
               uniform Bool cloud            ):TARGET
{
   return ShadowDirValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true, num, cloud);
}
#if !CG
Half ShdDirM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                NOPERSP PIXEL                      ,
                        UInt index  :SV_SampleIndex,
                uniform Int  num                   ,
                uniform Bool cloud                 ):TARGET
{
   return ShadowDirValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true, num, cloud);
}
#endif
TECHNIQUE(ShdDir1, DrawPosXY_VS(), ShdDir_PS(1, false));   TECHNIQUE(ShdDir1C, DrawPosXY_VS(), ShdDir_PS(1, true));
TECHNIQUE(ShdDir2, DrawPosXY_VS(), ShdDir_PS(2, false));   TECHNIQUE(ShdDir2C, DrawPosXY_VS(), ShdDir_PS(2, true));
TECHNIQUE(ShdDir3, DrawPosXY_VS(), ShdDir_PS(3, false));   TECHNIQUE(ShdDir3C, DrawPosXY_VS(), ShdDir_PS(3, true));
TECHNIQUE(ShdDir4, DrawPosXY_VS(), ShdDir_PS(4, false));   TECHNIQUE(ShdDir4C, DrawPosXY_VS(), ShdDir_PS(4, true));
TECHNIQUE(ShdDir5, DrawPosXY_VS(), ShdDir_PS(5, false));   TECHNIQUE(ShdDir5C, DrawPosXY_VS(), ShdDir_PS(5, true));
TECHNIQUE(ShdDir6, DrawPosXY_VS(), ShdDir_PS(6, false));   TECHNIQUE(ShdDir6C, DrawPosXY_VS(), ShdDir_PS(6, true));

TECHNIQUE_4_1(ShdDir1M, DrawPosXY_VS(), ShdDirM_PS(1, false));   TECHNIQUE_4_1(ShdDir1CM, DrawPosXY_VS(), ShdDirM_PS(1, true));
TECHNIQUE_4_1(ShdDir2M, DrawPosXY_VS(), ShdDirM_PS(2, false));   TECHNIQUE_4_1(ShdDir2CM, DrawPosXY_VS(), ShdDirM_PS(2, true));
TECHNIQUE_4_1(ShdDir3M, DrawPosXY_VS(), ShdDirM_PS(3, false));   TECHNIQUE_4_1(ShdDir3CM, DrawPosXY_VS(), ShdDirM_PS(3, true));
TECHNIQUE_4_1(ShdDir4M, DrawPosXY_VS(), ShdDirM_PS(4, false));   TECHNIQUE_4_1(ShdDir4CM, DrawPosXY_VS(), ShdDirM_PS(4, true));
TECHNIQUE_4_1(ShdDir5M, DrawPosXY_VS(), ShdDirM_PS(5, false));   TECHNIQUE_4_1(ShdDir5CM, DrawPosXY_VS(), ShdDirM_PS(5, true));
TECHNIQUE_4_1(ShdDir6M, DrawPosXY_VS(), ShdDirM_PS(6, false));   TECHNIQUE_4_1(ShdDir6CM, DrawPosXY_VS(), ShdDirM_PS(6, true));
/******************************************************************************/
Half ShdPoint_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL):TARGET
{
   return ShadowPointValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
}
#if !CG
Half ShdPointM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                  NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                  NOPERSP PIXEL,
                          UInt index  :SV_SampleIndex):TARGET
{
   return ShadowPointValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
}
#endif
TECHNIQUE    (ShdPoint , DrawPosXY_VS(), ShdPoint_PS ());
TECHNIQUE_4_1(ShdPointM, DrawPosXY_VS(), ShdPointM_PS());
/******************************************************************************/
Half ShdCone_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                NOPERSP Vec2 inPosXY:TEXCOORD1,
                NOPERSP PIXEL):TARGET
{
   return ShadowConeValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
}
#if !CG
Half ShdConeM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                 NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                 NOPERSP PIXEL,
                         UInt index  :SV_SampleIndex):TARGET
{
   return ShadowConeValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
}
#endif
TECHNIQUE    (ShdCone , DrawPosXY_VS(), ShdCone_PS());
TECHNIQUE_4_1(ShdConeM, DrawPosXY_VS(), ShdConeM_PS());
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
Half ShdBlur_PS(NOPERSP Vec2 inTex:TEXCOORD,
                uniform Int  samples       ):TARGET
{
   Half weight=0.25,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z);
   UNROLL for(Int i=0; i<samples; i++)
   {
      Vec2 t;
      if(samples== 4)t=RTSize.xy*BlendOfs4 [i]+inTex;
    //if(samples== 5)t=RTSize.xy*BlendOfs5 [i]+inTex;
      if(samples== 6)t=RTSize.xy*BlendOfs6 [i]+inTex;
      if(samples== 8)t=RTSize.xy*BlendOfs8 [i]+inTex;
    //if(samples== 9)t=RTSize.xy*BlendOfs9 [i]+inTex;
      if(samples==12)t=RTSize.xy*BlendOfs12[i]+inTex;
    //if(samples==13)t=RTSize.xy*BlendOfs13[i]+inTex;
      // use linear filtering because texcoords are not rounded
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurX_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 uniform Int  range         ):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.y=inTex.y;
   UNROLL for(Int i=-range; i<=range; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.x=RTSize.x*(2*i+((i>0) ? -0.5 : 0.5))+inTex.x;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurY_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 uniform Int  range         ):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.x=inTex.x;
   UNROLL for(Int i=-range; i<=range; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.y=RTSize.y*(2*i+((i>0) ? -0.5 : 0.5))+inTex.y;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
  TECHNIQUE(ShdBlur4 , Draw_VS(), ShdBlur_PS(4));
//TECHNIQUE(ShdBlur5 , Draw_VS(), ShdBlur_PS(5));
  TECHNIQUE(ShdBlur6 , Draw_VS(), ShdBlur_PS(6));
  TECHNIQUE(ShdBlur8 , Draw_VS(), ShdBlur_PS(8));
//TECHNIQUE(ShdBlur9 , Draw_VS(), ShdBlur_PS(9));
  TECHNIQUE(ShdBlur12, Draw_VS(), ShdBlur_PS(12));
//TECHNIQUE(ShdBlur13, Draw_VS(), ShdBlur_PS(13));
//TECHNIQUE(ShdBlurX1, Draw_VS(), ShdBlurX_PS(1));
//TECHNIQUE(ShdBlurY1, Draw_VS(), ShdBlurY_PS(1));
  TECHNIQUE(ShdBlurX2, Draw_VS(), ShdBlurX_PS(2));
  TECHNIQUE(ShdBlurY2, Draw_VS(), ShdBlurY_PS(2));
/******************************************************************************/
