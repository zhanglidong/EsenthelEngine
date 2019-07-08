/******************************************************************************/
#include "!Header.h"
#include "Hdr.h"

// shader expects linear gamma

#define BRIGHT    0 // if apply adjustment for scenes where half pixels are bright, and other half are dark, in that case prefer focus on brighter, to avoid making already bright pixels too bright
#define GEOMETRIC 0 // don't use geometric mean, because of cases when bright sky is mostly occluded by dark objects, then entire scene will get brighter, making the sky look too bright and un-realistic
/******************************************************************************/
// HDR
/******************************************************************************/
Vec4 HdrDS_PS(NOPERSP Vec2 inTex:TEXCOORD,
              uniform Int  step          ):COLOR
{
   Vec2 tex_min=inTex-ImgSize.xy,
        tex_max=inTex+ImgSize.xy;
   if(step==0)
   {
      // use linear filtering because we're downsampling
      Vec sum=TexLod(Col, Vec2(tex_min.x, tex_min.y)).rgb
             +TexLod(Col, Vec2(tex_max.x, tex_min.y)).rgb
             +TexLod(Col, Vec2(tex_min.x, tex_max.y)).rgb
             +TexLod(Col, Vec2(tex_max.x, tex_max.y)).rgb;

   #if !LINEAR_GAMMA // convert from sRGB to linear
      sum=SRGBToLinearFast(sum)/4; // SRGBToLinearFast(sum/4)*4
   #endif

      Flt lum=Dot(sum, HdrWeight);

   // adjustment
   #if BRIGHT
      lum=Sqr(lum);
   #endif
   #if GEOMETRIC
      lum=log2(Max(lum, EPS)); // NaN
   #endif

      return lum;
   }else
   {
      // use linear filtering because we're downsampling
      return Avg(TexLod(Col, Vec2(tex_min.x, tex_min.y)).x,
                 TexLod(Col, Vec2(tex_max.x, tex_min.y)).x,
                 TexLod(Col, Vec2(tex_min.x, tex_max.y)).x,
                 TexLod(Col, Vec2(tex_max.x, tex_max.y)).x);
   }
}
/******************************************************************************/
Vec4 HdrUpdate_PS(NOPERSP Vec2 inTex:TEXCOORD):COLOR
{
   Flt lum=TexPoint(Col, Vec2(0, 0)).x;

   // adjustment restore
#if GEOMETRIC
   lum=exp2(lum); // we've applied 'log2' above, so revert it back
#endif
#if BRIGHT
   lum=Sqrt(lum); // we've applied 'Sqr' above, so revert it back
#endif

   lum=Pow(lum, HdrExp); //lum=Sqrt(lum); // if further from the target brightness, apply the smaller scale. When using a smaller 'HdrExp' then scale will be stretched towards "1" (meaning smaller changes), using exp=0.5 gives Sqrt(lum)

   lum=HdrBrightness/Max(lum, EPS_COL); // desired scale

   lum=Mid(lum, HdrMaxDark, HdrMaxBright);
   return Lerp(lum, TexPoint(Lum, Vec2(0, 0)).x, Step);
}
/******************************************************************************/
VecH4 Hdr_PS(NOPERSP Vec2 inTex:TEXCOORD,
             NOPERSP PIXEL              ,
             uniform Bool dither        ):COLOR
{
   VecH4 col=TexLod  (Col, inTex); // can't use 'TexPoint' because 'Col' can be supersampled
   Half  lum=TexPoint(Lum, Vec2(0, 0)).x;

   /* full formula
   if(gamma)col.rgb=SRGBToLinearFast(col.rgb);
   col.rgb*=lum;
   if(gamma)col.rgb=LinearToSRGBFast(col.rgb); */

#if !LINEAR_GAMMA
   lum=LinearToSRGBFast(lum);
#endif
   col.rgb*=lum;

   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
/******************************************************************************/
// TECHNIQUES
/******************************************************************************/
TECHNIQUE(HdrDS0, Draw_VS(), HdrDS_PS(0));
TECHNIQUE(HdrDS1, Draw_VS(), HdrDS_PS(1));

TECHNIQUE(HdrUpdate, Draw_VS(), HdrUpdate_PS());
TECHNIQUE(Hdr      , Draw_VS(),       Hdr_PS(false));
TECHNIQUE(HdrD     , Draw_VS(),       Hdr_PS(true ));
/******************************************************************************/
