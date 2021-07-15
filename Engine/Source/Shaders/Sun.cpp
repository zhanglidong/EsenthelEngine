/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#include "!Set Prec Struct.h"
struct SunClass
{
   Vec2 pos2;
   VecH pos, color;
};

BUFFER(Sun)
   SunClass Sun;
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
Half SunRaysMask_PS
(
#if MASK
   NOPERSP Vec2 uv:UV,
#endif
   NOPERSP Vec2 posXY:POS_XY,
   NOPERSP PIXEL
):TARGET
{
#if MASK // for this version we have to use linear depth filtering, because RT is of different size, and otherwise too much artifacts/flickering is generated
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      Half m=(Length2(GetPosLinear(uv, posXY))>=Sqr(Viewport.range));
   #else // need safer
      Flt  z=TexDepthRawLinear(uv);
      Half m=(DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), posXY))>=Sqr(Viewport.range));
   #endif
      return m*TexLod(ImgX, uv).x; // use linear filtering because 'ImgX' can be of different size
#else // can use point filtering here
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      return Length2(GetPosPix(pixel.xy, posXY))>=Sqr(Viewport.range);
   #else // need safer
      Flt z=Depth[pixel.xy];
      return DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), posXY))>=Sqr(Viewport.range);
   #endif
#endif
}
/******************************************************************************/
/*Vec4 SunRaysSoft_PS(NOPERSP Vec2 uv:UV):TARGET
{
   const Int samples=6;
         Flt color  =TexLod(ImgX, uv).x; // use linear filtering because 'ImgX' can be of different size
   for(Int i=0; i<samples; i++)
   {
      Vec2 t=uv+BlendOfs6[i]*ImgSize.xy;
      color+=TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
   }
   return Vec4(color/(samples+1)*Color[0].rgb, 0);
}
TECHNIQUE(SunRaysSoft, Draw_VS(), SunRaysSoft_PS());*/
/******************************************************************************/
VecH4 SunRays_PS(NOPERSP Vec2 uv   :UV,
                 NOPERSP Vec2 posXY:POS_XY,
                 NOPERSP PIXEL            ):TARGET
{
   VecH  pos  =Normalize(Vec(posXY, 1));
   Half  cos  =Dot(Sun.pos, pos),
         power=(LINEAR_GAMMA ? cos : (cos>0) ? Sqr(cos) : 0);
   VecH4 col  =0;

#if 0 // can't use 'clip' because we always have to set output (to 0 if no rays)
   clip(power-EPS_COL);
#else
   BRANCH if(power>EPS_COL)
#endif
   {
      const Int  steps  =40;
            Flt  light  =0; // use HP because we're iterating lot of samples
            Vec2 sun_pos=Sun.pos2;

   // limit sun position
   Vec2 delta=uv-sun_pos; // towards viewport
   Flt  frac=ViewportClamp(sun_pos, delta); // returns 0 if already inside viewport
   sun_pos+=frac*delta; // move towards viewport
 //power  *=(1-frac);

   #if JITTER
      uv+=(sun_pos-uv)*(Noise1D(pixel.xy)*(3.0/steps)); // a good value is 2.5 or 3.0 (3.0 was slightly better)
   #endif

      UNROLL for(Int i=0; i<steps; i++)
      {
         Vec2 t=Lerp(uv, sun_pos, i/Flt(steps)); // /(steps) worked better than /(steps-1)
         if(MASK)light+=TexLod(ImgX, t).x; // pos and clouds combined together, use linear filtering because texcoords aren't rounded
         else    light+=DEPTH_BACKGROUND(TexDepthRawPoint(t)); // use simpler version here unlike in 'SunRaysPre_PS' because this one is called for each step for each pixel
      }
      col.rgb=Half(light*power/steps)*Sun.color;
   #if DITHER
      ApplyDither(col.rgb, pixel.xy, false); // here we're always in sRGB gamma
   #endif
   #if GAMMA
      col.rgb=SRGBToLinearFast(col.rgb); // 'SRGBToLinearFast' works better here than 'SRGBToLinear' (gives high contrast, dark colors remain darker, while 'SRGBToLinear' highlights them more)
   #endif
   }
   return col;
}
/******************************************************************************/
