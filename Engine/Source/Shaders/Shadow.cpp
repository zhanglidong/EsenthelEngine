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
#ifndef GATHER
#define GATHER 0
#endif
/******************************************************************************/
void Geom_VS // for 3D Geom
(
           VtxInput vtx,
#if !GL_ES
   out NOPERSP Vec2 outTex  :TEXCOORD0,
 //out NOPERSP Vec2 outPosXY:TEXCOORD1,
#endif
   out         Vec4 outPos  :POSITION
)
{
   outPos=Project(TransformPos(vtx.pos()));

#if GL_ES // simulate D.depthClip(false), needed for GL ES which doesn't support it, Warning: this introduces a bit too much clipping at the viewport end, because the neighboring vertexes remain the same, and only the vertex behind the viewport gets repositioned, the line between them won't cover entire original area (however it's small)
   #if REVERSE_DEPTH
      outPos.z=Max(outPos.z, 0);
   #else
      outPos.z=Min(outPos.z, outPos.w);
   #endif
#endif

#if !GL_ES
   outTex  =ProjectedPosToUV   (outPos);
 //outPosXY=          UVToPosXY(outTex);
#endif
}
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
#if GL_ES && GATHER // GL ES with GATHER support
#undef TexDepthRawLinear
Flt    TexDepthRawLinear(Vec2 uv) // because GL ES 3 can't do 'TexDepthRawLinear'
{
   Vec2 pixel =uv*RTSize.zw+0.5,
        pixeli=Floor(pixel),
        f     =pixel-pixeli; Flt fx1=1-f.x;
        uv    =pixeli*RTSize.xy; // adjust 'uv' to make sure pixels will be selected based on 'pixeli' (because of precision issues)
   Vec4 t=TexDepthGather(uv);

   return Lerp(t.w*fx1 + t.z*f.x,
               t.x*fx1 + t.y*f.x, f.y);
}
#undef TexDepthLinear
Flt    TexDepthLinear(Vec2 uv) {return LinearizeDepth(TexDepthRawLinear(uv));} // because GL ES 3 can't do 'TexDepthLinear'
#endif
void Process(inout Half color, inout Half weight, Half c, Flt z, Flt base_z, Vec2 dw_mad)
{
   if(1)
   {
      z=LinearizeDepth(z);
      Half w=DepthWeight(base_z-z, dw_mad);
      color +=w*c;
      weight+=w;
   }else
   {
      color +=c;
      weight+=1;
   }
}
Half ShdBlur_PS
(
#if GL_ES && GEOM // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex:TEXCOORD
#endif
):TARGET
{
#if GL_ES && GEOM // doesn't support NOPERSP
   Vec2 inTex=PixelToUV(pixel);
#endif

   Half weight, color;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z);
   /*if(E && GATHER)
   {
      weight=0;
      color =0;
      UNROLL for(Int y=0; y<2; y++)
      UNROLL for(Int x=0; x<2; x++)
      {
         VecH4 c=TexGatherOfs(ImgX, inTex, VecI2(-1+x*2, -1+y*2));
         if(0)
         {
            Vec4 d=TexDepthGatherOfs(inTex, VecI2(-1+x*2, -1+y*2)); // FIXME here are different AO and Depth res
            if(x==0 && y==0){Process(color, weight, c.x, d.x, z, dw_mad); Process(color, weight, c.y, d.y, z, dw_mad); Process(color, weight, c.z, d.z, z, dw_mad); Process(color, weight, c.w, d.w, z, dw_mad);}else
            if(x==1 && y==0){Process(color, weight, c.x, d.x, z, dw_mad); Process(color, weight, c.w, d.w, z, dw_mad);}else
            if(x==0 && y==1){Process(color, weight, c.w, d.w, z, dw_mad); Process(color, weight, c.z, d.z, z, dw_mad);}else
            if(x==1 && y==1){Process(color, weight, c.w, d.w, z, dw_mad);}
         }else
         {
            if(x==0 && y==0){color+=Sum(c) ; weight+=4;}else
            if(x==1 && y==0){color+=c.x+c.w; weight+=2;}else
            if(x==0 && y==1){color+=c.w+c.z; weight+=2;}else
            if(x==1 && y==1){color+=c.w    ; weight+=1;}
         }
      }
   }else*/
   {
      weight=0.25;
      color =TexPoint(ImgX, inTex).x*weight;
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
         Process(color, weight, TexLod(ImgX, t).x, TexDepthRawLinear(t), z, dw_mad); // use linear filtering because texcoords aren't rounded
      }
   }
   return color/weight;
}
Half ShdBlurX_PS
(
#if GL_ES && GEOM // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex:TEXCOORD
#endif
):TARGET
{
#if GL_ES && GEOM // doesn't support NOPERSP
   Vec2 inTex=PixelToUV(pixel);
#endif

   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.y=inTex.y;
   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.x=RTSize.x*(2*i+((i>0) ? -0.5 : 0.5))+inTex.x;
      Process(color, weight, TexLod(ImgX, t).x, TexDepthRawLinear(t), z, dw_mad); // use linear filtering because texcoords aren't rounded
   }
   return color/weight;
}
Half ShdBlurY_PS
(
#if GL_ES && GEOM // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex:TEXCOORD
#endif
):TARGET
{
#if GL_ES && GEOM // doesn't support NOPERSP
   Vec2 inTex=PixelToUV(pixel);
#endif

   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.x=inTex.x;
   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.y=RTSize.y*(2*i+((i>0) ? -0.5 : 0.5))+inTex.y;
      Process(color, weight, TexLod(ImgX, t).x, TexDepthRawLinear(t), z, dw_mad); // use linear filtering because texcoords aren't rounded
   }
   return color/weight;
}
/******************************************************************************/
