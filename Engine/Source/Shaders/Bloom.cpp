/******************************************************************************/
#include "!Header.h"
#include "!Set SP.h"
BUFFER(Bloom)
   VecH BloomParams; // x=original, y=scale, z=cut
BUFFER_END
#include "!Set LP.h"

#ifndef GLOW
   #define GLOW 0
#endif
#ifndef CLAMP
   #define CLAMP 0
#endif
#ifndef HALF_RES
   #define HALF_RES 0
#endif
#ifndef SATURATE
   #define SATURATE 0
#endif
#ifndef GAMMA
   #define GAMMA 0
#endif
#ifndef DITHER
   #define DITHER 0
#endif
/******************************************************************************/
void BloomDS_VS(VtxInput vtx,
    NOPERSP out Vec2 outTex:TEXCOORD,
    NOPERSP out Vec4 outVtx:POSITION)
{
   outTex=vtx.tex (); if(GLOW)outTex-=ImgSize.xy*Vec2(HALF_RES ? 0.5 : 1.5, HALF_RES ? 0.5 : 1.5);
   outVtx=vtx.pos4();
}
inline VecH BloomColor(VecH color, Bool gamma)
{
   if(gamma)color=LinearToSRGBFast(color);
   if(SATURATE)
   {
      return color*BloomParams.y+BloomParams.z;
   }else
   {
      Half col_lum=Max(color), lum=col_lum*BloomParams.y+BloomParams.z;
      return (lum>0) ? color*(lum/col_lum) : VecH(0, 0, 0);
   }
}
VecH4 BloomDS_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET // "Max(0, " of the result is not needed because we're rendering to 1 byte per channel RT
{
   if(GLOW)
   {
      const Int  res=(HALF_RES ? 2 : 4);
      const Bool gamma_per_pixel=false; // !! must be the same as in 'RendererClass::bloom' !!

      VecH  color=0;
      VecH4 glow =0;
      UNROLL for(Int y=0; y<res; y++)
      UNROLL for(Int x=0; x<res; x++)
      {
         VecH4 c=TexLod(Img, UVClamp(inTex+ImgSize.xy*Vec2(x, y), CLAMP)); // can't use 'TexPoint' because 'Img' can be supersampled
         if(GAMMA && gamma_per_pixel)c.rgb=LinearToSRGBFast(c.rgb);
         color   +=c.rgb;
         glow.rgb+=c.rgb*c.a;
         glow.a   =Max(glow.a, c.a);
      }
      if(GAMMA && !gamma_per_pixel)glow.rgb =(2*glow.a)*LinearToSRGBFast(glow.rgb/Max(Max(glow.rgb), HALF_MIN));
      else                         glow.rgb*= 2*glow.a                           /Max(Max(glow.rgb), HALF_MIN) ; // NaN (increase by 2 because normally it's too small)
      return VecH4(Max(BloomColor(color, GAMMA && !gamma_per_pixel), glow.rgb), 0);
   }else
   {
      if(HALF_RES)
      {
         return VecH4(BloomColor(TexLod(Img, UVClamp(inTex, CLAMP)).rgb, GAMMA), 0);
      }else
      {
         Vec2 tex_min=UVClamp(inTex-ImgSize.xy, CLAMP),
              tex_max=UVClamp(inTex+ImgSize.xy, CLAMP);
         return VecH4(BloomColor(TexLod(Img, Vec2(tex_min.x, tex_min.y)).rgb
                                +TexLod(Img, Vec2(tex_max.x, tex_min.y)).rgb
                                +TexLod(Img, Vec2(tex_min.x, tex_max.y)).rgb
                                +TexLod(Img, Vec2(tex_max.x, tex_max.y)).rgb, GAMMA), 0);
      }
   }
}
/******************************************************************************/
VecH4 Bloom_PS(NOPERSP Vec2 inTex:TEXCOORD,
               NOPERSP PIXEL              ):TARGET
{
   // final=src*original + Sat((src-cut)*scale)
   VecH4 col;
   col.rgb=TexLod(Img, inTex).rgb; // original, can't use 'TexPoint' because 'Img' can be supersampled
   if(GAMMA)col.rgb=LinearToSRGBFast(col.rgb);
   col.rgb=col.rgb*BloomParams.x + TexLod(Img1, inTex).rgb; // bloom, can't use 'TexPoint' because 'Img1' can be smaller
   if(DITHER)ApplyDither(col.rgb, pixel.xy, false); // here we always have sRGB gamma
   if(GAMMA)col.rgb=SRGBToLinearFast(col.rgb);
#if ALPHA
   col.a=TexLod(ImgX, inTex).r; // can't use 'TexPoint' because 'ImgX' can be supersampled
#else
   col.a=1; // force full alpha so back buffer effects can work ok
#endif
   return col;
}
/******************************************************************************/
