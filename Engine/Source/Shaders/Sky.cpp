/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
#include "Layered Clouds.h"
/******************************************************************************/
VecH4 SkyColor(Vec inTex)
{
   Half hor=Pow(1-Sat(inTex.y), SkyHorExp);
   return Lerp(SkySkyCol, SkyHorCol, hor);
}
/******************************************************************************/
// MULTI_SAMPLE, FLAT, DENSITY, TEXTURES, STARS, DITHER, PER_VERTEX, CLOUD
void Sky_VS
(
   VtxInput vtx,
   out Vec4 outVtx:POSITION

#if !FLAT
 , out Vec outPos:TEXCOORD0
#endif

#if TEXTURES || !PER_VERTEX
 , out Vec outTex:TEXCOORD1
#endif

#if STARS && TEXTURES==0
 , out Vec outTexStar:TEXCOORD2
#endif

#if PER_VERTEX && TEXTURES==0
 , out VecH4 outCol:COLOR0
#endif

#if CLOUD
 , out Vec   outTexCloud:TEXCOORD3
 , out VecH4 outColCloud:COLOR1
#endif
)
{
   Vec pos=TransformPos(vtx.pos()); outVtx=Project(pos);
#if !FLAT
   outPos=pos;
#endif

#if TEXTURES || !PER_VERTEX
   outTex=vtx.pos();
#endif

#if STARS && TEXTURES==0
   outTexStar=Transform(vtx.pos(), SkyStarOrn);
#endif

#if PER_VERTEX && TEXTURES==0
   outCol=SkyColor(vtx.pos());
#endif

#if CLOUD
   outTexCloud=vtx.pos()*Vec(LCScale, 1, LCScale);
   outColCloud=CL[0].color; outColCloud.a*=Sat(CloudAlpha(vtx.pos().y));
#endif
}
/******************************************************************************/
VecH4 Sky_PS
(
   PIXEL

#if !FLAT
 , Vec inPos:TEXCOORD0
#endif

#if TEXTURES || !PER_VERTEX
 , Vec inTex:TEXCOORD1
#endif

#if STARS && TEXTURES==0
 , Vec inTexStar:TEXCOORD2
#endif

#if PER_VERTEX && TEXTURES==0
 , VecH4 inCol:COLOR0
#endif

#if CLOUD
 , Vec   inTexCloud:TEXCOORD3
 , VecH4 inColCloud:COLOR1
#endif

#if MULTI_SAMPLE==2 && !FLAT
 , UInt index:SV_SampleIndex
#endif
):TARGET
{
   Half alpha;
#if FLAT
   alpha=0; // flat uses ALPHA_NONE
#else
   #if MULTI_SAMPLE==0
      Flt frac=TexDepthPoint(PixelToUV(pixel))/Normalize(inPos).z;
      alpha=Sat(frac*SkyFracMulAdd.x + SkyFracMulAdd.y);
   #elif MULTI_SAMPLE==1
      Flt pos_scale=Normalize(inPos).z;
      alpha=0; UNROLL for(Int i=0; i<MS_SAMPLES; i++){Flt dist=TexDepthMS(pixel.xy, i)/pos_scale; alpha+=Sat(dist*SkyFracMulAdd.x + SkyFracMulAdd.y);}
      alpha/=MS_SAMPLES;
   #elif MULTI_SAMPLE==2
      Flt pos_scale=Normalize(inPos).z;
      alpha=Sat(TexDepthMS(pixel.xy, index)/pos_scale*SkyFracMulAdd.x + SkyFracMulAdd.y);
   #endif

   if(DENSITY)alpha=Pow(SkyDnsExp, alpha)*SkyDnsMulAdd.x+SkyDnsMulAdd.y; // here 'alpha' means opacity of the sky which is used as the distance from start to end point, this function matches 'AccumulatedDensity'
#endif

   VecH4 col;

#if   TEXTURES==2
   col.rgb=Lerp(TexCubeLod(Cub, inTex).rgb, TexCubeLod(Cub1, inTex).rgb, SkyBoxBlend);
#elif TEXTURES==1
   col.rgb=TexCubeLod(Cub, inTex).rgb; // use 'TexCubeLod' in case Image is for PBR Environment Map which has blurred mip-maps
#else
   #if PER_VERTEX
      col=inCol;
   #else
      inTex=Normalize(inTex);
        col=SkyColor (inTex);

      Half cos      =Dot(SkySunPos, inTex),
           highlight=1+Sqr(cos)*((cos>0) ? SkySunHighlight.x : SkySunHighlight.y); // rayleigh, here 'Sqr' works better than 'Abs'
      col.rgb*=highlight;
   #endif

   #if STARS
      col.rgb=Lerp(TexCubeLod(Cub, inTexStar).rgb, col.rgb, col.a);
   #endif
#endif

   col.a=alpha;

#if CLOUD
   Vec2  cloud_tex=Normalize(inTexCloud).xz;
   VecH4 cloud_col=Tex(Img, cloud_tex*CL[0].scale + CL[0].position)*inColCloud;
   col.rgb=Lerp(col.rgb, cloud_col.rgb, cloud_col.a);
#endif

   if(DITHER && MULTI_SAMPLE!=2)ApplyDither(col.rgb, pixel.xy); // skip dither for MS because it won't be noticeable

   return col;
}
/******************************************************************************/
