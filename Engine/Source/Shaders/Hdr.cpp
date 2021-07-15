/******************************************************************************/
#include "!Header.h"
#include "Hdr.h"

// shader expects linear gamma

#define BRIGHT    1 // if apply adjustment for scenes where half pixels are bright, and other half are dark, in that case prefer focus on brighter, to avoid making already bright pixels too bright
#define GEOMETRIC 0 // don't use geometric mean, because of cases when bright sky is mostly occluded by dark objects, then entire scene will get brighter, making the sky look too bright and un-realistic
/******************************************************************************/
// HDR
/******************************************************************************/
Flt HdrDS_PS(NOPERSP Vec2 uv:UV):TARGET
{
   Vec2 tex_min=uv-ImgSize.xy,
        tex_max=uv+ImgSize.xy;

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
Flt HdrUpdate_PS():TARGET // here use full precision
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

   lum=HdrBrightness/Max(lum, EPS_COL); // desired scale

   lum=Mid(lum, HdrMaxDark, HdrMaxBright);
   return Lerp(lum, TexPoint(ImgXF1, Vec2(0, 0)).x, Step); // lerp new with old
}
/******************************************************************************/
void Hdr_VS(VtxInput vtx,
   NOPERSP  out Vec2 uv   :UV ,
   NOINTERP out Half lum  :LUM,
   NOPERSP  out Vec4 pixel:POSITION)
{
   uv=vtx.uv();

   lum=ImgX[VecI2(0, 0)];
#if !LINEAR_GAMMA
   lum=LinearToSRGBFast(lum);
#endif

   pixel=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
}
VecH4 Hdr_PS(NOPERSP  Vec2 uv :UV ,
             NOINTERP Half lum:LUM,
             NOPERSP  PIXEL       ):TARGET
{
   VecH4 col=TexLod(Img, uv); // can't use 'TexPoint' because 'Img' can be supersampled

   /* full formula
   if(gamma)col.rgb=SRGBToLinearFast(col.rgb);
   col.rgb*=lum;
   if(gamma)col.rgb=LinearToSRGBFast(col.rgb);*/

   col.rgb*=lum;

#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
