/******************************************************************************/
#include "!Header.h"
#include "Hdr.h"

// shader expects linear gamma

#define BRIGHT    0 // if apply adjustment for scenes where half pixels are bright, and other half are dark, in that case prefer focus on brighter, to avoid making already bright pixels too bright
#define GEOMETRIC 0 // don't use geometric mean, because of cases when bright sky is mostly occluded by dark objects, then entire scene will get brighter, making the sky look too bright and un-realistic
/******************************************************************************/
// HDR
/******************************************************************************/
Flt HdrDS_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Vec2 tex_min=inTex-ImgSize.xy,
        tex_max=inTex+ImgSize.xy;

#if STEP==0
   // use linear filtering because we're downsampling, for the first step use half precision for high performance, because there's a lot of data
   VecH sum=TexLod(Img, Vec2(tex_min.x, tex_min.y)).rgb
           +TexLod(Img, Vec2(tex_max.x, tex_min.y)).rgb
           +TexLod(Img, Vec2(tex_min.x, tex_max.y)).rgb
           +TexLod(Img, Vec2(tex_max.x, tex_max.y)).rgb;

#if !LINEAR_GAMMA // convert from sRGB to linear
   sum=SRGBToLinearFast(sum)/4; // SRGBToLinearFast(sum/4)*4
#endif

   Flt lum=Dot(sum, HdrWeight);

// adjustment
#if BRIGHT
   lum=Sqr(lum);
#endif
#if GEOMETRIC
   lum=log2(Max(lum, HALF_MIN)); // NaN
#endif

   return lum;
#else
   // use linear filtering because we're downsampling, here use full precision for more accuracy
   return Avg(TexLod(ImgXF, Vec2(tex_min.x, tex_min.y)).x,
              TexLod(ImgXF, Vec2(tex_max.x, tex_min.y)).x,
              TexLod(ImgXF, Vec2(tex_min.x, tex_max.y)).x,
              TexLod(ImgXF, Vec2(tex_max.x, tex_max.y)).x);
#endif
}
/******************************************************************************/
Flt HdrUpdate_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET // here use full precision
{
   Flt lum=TexPoint(ImgXF, Vec2(0, 0)).x; // new luminance

   // adjustment restore
#if GEOMETRIC
   lum=exp2(lum); // we've applied 'log2' above, so revert it back
#endif
#if BRIGHT
   lum=Sqrt(lum); // we've applied 'Sqr' above, so revert it back
#endif

   lum=Pow(lum, HdrExp); //lum=Sqrt(lum); // if further from the target brightness, apply the smaller scale. When using a smaller 'HdrExp' then scale will be stretched towards "1" (meaning smaller changes), using exp=0.5 gives Sqrt(lum)

#if !CG
   lum=HdrBrightness/Max(lum, EPS_COL); // desired scale
#else // above fails to compile on CG
   lum=HdrBrightness/Max(lum, (Flt)EPS_COL); // desired scale
#endif

   lum=Mid(lum, HdrMaxDark, HdrMaxBright);
   return Lerp(lum, TexPoint(ImgXF1, Vec2(0, 0)).x, Step); // lerp new with old
}
/******************************************************************************/
VecH4 Hdr_PS(NOPERSP Vec2 inTex:TEXCOORD,
             NOPERSP PIXEL              ):TARGET
{
   VecH4 col=TexLod  (Img , inTex); // can't use 'TexPoint' because 'Img' can be supersampled
   Half  lum=TexPoint(ImgX, Vec2(0, 0)).x;

   /* full formula
   if(gamma)col.rgb=SRGBToLinearFast(col.rgb);
   col.rgb*=lum;
   if(gamma)col.rgb=LinearToSRGBFast(col.rgb); */

#if !LINEAR_GAMMA
   lum=LinearToSRGBFast(lum);
#endif
   col.rgb*=lum;

#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
