/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#include "!Set SP.h"
struct SunClass
{
   Vec2 pos2;
   VecH pos, color;
};

BUFFER(Sun)
   SunClass Sun;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
Half SunRaysMask_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                    NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET
{
#if MASK // for this version we have to use linear depth filtering, because RT is of different size, and otherwise too much artifacts/flickering is generated
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      Half m=(Length2(GetPosLinear(inTex, inPosXY))>=Sqr(Viewport.range));
   #else // need safer
      Flt  z=TexDepthRawLinear(inTex);
      Half m=(DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), inPosXY))>=Sqr(Viewport.range));
   #endif
      return m*TexLod(ImgX, inTex).x; // use linear filtering because 'ImgX' can be of different size
#else // can use point filtering here
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      return Length2(GetPosPoint(inTex, inPosXY))>=Sqr(Viewport.range);
   #else // need safer
      Flt z=TexDepthRawPoint(inTex);
      return DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), inPosXY))>=Sqr(Viewport.range);
   #endif
#endif
}
/******************************************************************************/
/*Vec4 SunRaysSoft_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   const Int samples=6;
         Flt color  =TexLod(ImgX, inTex).x; // use linear filtering because 'ImgX' can be of different size
   for(Int i=0; i<samples; i++)
   {
      Vec2 t=inTex+BlendOfs6[i]*ImgSize.xy;
      color+=TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
   }
   return Vec4(color/(samples+1)*Color[0].rgb, 0);
}
TECHNIQUE(SunRaysSoft, Draw_VS(), SunRaysSoft_PS());*/
/******************************************************************************/
VecH4 SunRays_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL                 ):TARGET
{
   VecH  pos  =Normalize(Vec(inPosXY, 1));
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
   #if 0
      if(sun_pos.x>1)
      {
         Flt frac=(1-inTex.x)/(sun_pos.x-inTex.x);
         sun_pos.x =1;
         sun_pos.y-=(1-frac)*(sun_pos.y-inTex.y);
       //power    *=frac;
      }else
      if(sun_pos.x<0)
      {
         Flt frac=(inTex.x)/(inTex.x-sun_pos.x);
         sun_pos.x =0;
         sun_pos.y-=(1-frac)*(sun_pos.y-inTex.y);
       //power    *=frac;
      }

      if(sun_pos.y>1)
      {
         Flt frac=(1-inTex.y)/(sun_pos.y-inTex.y);
         sun_pos.y =1;
         sun_pos.x-=(1-frac)*(sun_pos.x-inTex.x);
       //power    *=frac;
      }else
      if(sun_pos.y<0)
      {
         Flt frac=(inTex.y)/(inTex.y-sun_pos.y);
         sun_pos.y =0;
         sun_pos.x-=(1-frac)*(sun_pos.x-inTex.x);
       //power    *=frac;
      }
   #else
    //Flt frac=Max(Max(Vec2Zero, Abs(sun_pos-0.5)-0.5)/Abs(sun_pos-inTex)); // Max(Max(0, Abs(sun_pos.x-0.5)-0.5)/Abs(sun_pos.x-inTex.x), Max(0, Abs(sun_pos.y-0.5)-0.5)/Abs(sun_pos.y-inTex.y));
      Flt frac=Max(Max(Vec2Zero, Abs(sun_pos-Viewport.center)-Viewport.size/2)/Abs(sun_pos-inTex)); // Max(Max(0, Abs(sun_pos.x-0.5)-0.5)/Abs(sun_pos.x-inTex.x), Max(0, Abs(sun_pos.y-0.5)-0.5)/Abs(sun_pos.y-inTex.y));
      sun_pos-=(  frac)*(sun_pos-inTex);
    //power  *=(1-frac);
   #endif

   #if JITTER
      inTex+=(sun_pos-inTex)*(DitherValue(pixel.xy)*(3.0/steps)); // a good value is 2.5 or 3.0 (3.0 was slightly better)
   #endif

      UNROLL for(Int i=0; i<steps; i++)
      {
         Vec2 t=Lerp(inTex, sun_pos, i/Flt(steps)); // /(steps) worked better than /(steps-1)
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
