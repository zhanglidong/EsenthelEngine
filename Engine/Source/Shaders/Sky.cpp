/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
/******************************************************************************/
inline VecH4 SkyColor(Vec inTex)
{
   Half hor=Pow(1-Sat(inTex.y), SkyHorExp);
   return Lerp(SkySkyCol, SkyHorCol, hor);
}

inline VecH4 SkyTex(Vec inTex, Vec inTexStar, VecH4 inCol, Half alpha, uniform Bool per_vertex, uniform Bool density, uniform Int textures, uniform Bool stars)
{
   if(density)alpha=Pow(SkyDnsExp, alpha)*SkyDnsMulAdd.x+SkyDnsMulAdd.y; // here 'alpha' means opacity of the sky which is used as the distance from start to end point, this function matches 'AccumulatedDensity'

   if(textures==2)return VecH4(Lerp(TexCube(Cub, inTex).rgb, TexCube(Cub1, inTex).rgb, SkyBoxBlend), alpha);else
   if(textures==1)return VecH4(     TexCube(Cub, inTex).rgb,                                         alpha);else
   {
      if(!per_vertex)
      {
         inTex=Normalize(inTex);
         inCol=SkyColor (inTex);

         Half cos      =Dot(SkySunPos, inTex),
              highlight=1+Sqr(cos)*((cos>0) ? SkySunHighlight.x : SkySunHighlight.y); // rayleigh, here 'Sqr' works better than 'Abs'
         inCol.rgb*=highlight;
      }

      if(stars)inCol.rgb=Lerp(TexCube(Cub, inTexStar).rgb, inCol.rgb, inCol.a);
      return VecH4(inCol.rgb, alpha);
   }
}
/******************************************************************************/
void Sky_VS(VtxInput vtx,
        out Vec4  outVtx     :POSITION ,
        out Vec   outPos     :TEXCOORD0,
        out Vec   outTex     :TEXCOORD1,
        out Vec   outTexStar :TEXCOORD2,
        out Vec   outTexCloud:TEXCOORD3,
        out VecH4 outCol     :COLOR0   ,
        out VecH4 outColCloud:COLOR1   ,
    uniform Bool  per_vertex           ,
    uniform Bool  stars                ,
    uniform Bool  clouds               )
{
                                outTex    =             vtx.pos();
   if(stars     )               outTexStar=Transform   (vtx.pos(), SkyStarOrn);
                 outVtx=Project(outPos    =TransformPos(vtx.pos()            ));
   if(per_vertex)outCol=                   SkyColor    (vtx.pos());

   if(clouds)
   {
      outTexCloud=vtx.pos()*Vec(LCScale, 1, LCScale);
      outColCloud=CL[0].color; outColCloud.a*=Sat(CloudAlpha(vtx.pos().y));
   }
}
/******************************************************************************/
VecH4 Sky_PS(PIXEL,
             Vec   inPos     :TEXCOORD0,
             Vec   inTex     :TEXCOORD1,
             Vec   inTexStar :TEXCOORD2,
             Vec   inTexCloud:TEXCOORD3,
             VecH4 inCol     :COLOR0   ,
             VecH4 inColCloud:COLOR1   ,
     uniform Bool  per_vertex          ,
     uniform Bool  flat                ,
     uniform Bool  density             ,
     uniform Int   textures            ,
     uniform Bool  stars               ,
     uniform Bool  clouds              ,
     uniform Bool  dither              ):TARGET
{
   Half alpha; if(flat)alpha=0;else // flat uses ALPHA_NONE
   {
      Flt frac=TexDepthPoint(PixelToScreen(pixel))/Normalize(inPos).z;
      alpha=Sat(frac*SkyFracMulAdd.x + SkyFracMulAdd.y);
   }
   VecH4 col=SkyTex(inTex, inTexStar, inCol, alpha, per_vertex, density, textures, stars);
   if(clouds)
   {
      Vec2  uv =Normalize(inTexCloud).xz;
      VecH4 tex=Tex(Img, uv*CL[0].scale + CL[0].position)*inColCloud;
      col.rgb=Lerp(col.rgb, tex.rgb, tex.a);
   }
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
VecH4 Sky1_PS(PIXEL,
              Vec  inPos     :TEXCOORD0,
              Vec  inTex     :TEXCOORD1,
              Vec  inTexStar :TEXCOORD2,
              Vec  inTexCloud:TEXCOORD3,
              Vec4 inCol     :COLOR    ,
      uniform Bool per_vertex          ,
      uniform Bool density             ,
      uniform Int  textures            ,
      uniform Bool stars               ,
      uniform Bool dither              ):TARGET
{
   Flt pos_scale=Normalize(inPos).z; Half alpha=0;
   UNROLL for(Int i=0; i<MS_SAMPLES; i++){Flt dist=TexDepthMS(pixel.xy, i)/pos_scale; alpha+=Sat(dist*SkyFracMulAdd.x + SkyFracMulAdd.y);}
   alpha/=MS_SAMPLES;
   VecH4 col=SkyTex(inTex, inTexStar, inCol, alpha, per_vertex, density, textures, stars);
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
VecH4 Sky2_PS(PIXEL,
              Vec  inPos     :TEXCOORD0     ,
              Vec  inTex     :TEXCOORD1     ,
              Vec  inTexStar :TEXCOORD2     ,
              Vec  inTexCloud:TEXCOORD3     ,
              Vec4 inCol     :COLOR         ,
              UInt index     :SV_SampleIndex,
      uniform Bool per_vertex               ,
      uniform Bool density                  ,
      uniform Int  textures                 ,
      uniform Bool stars                    ,
      uniform Bool dither                   ):TARGET
{
   Flt   pos_scale=Normalize(inPos).z;
   Half  alpha    =Sat(TexDepthMS(pixel.xy, index)/pos_scale*SkyFracMulAdd.x + SkyFracMulAdd.y);
   VecH4 col      =SkyTex(inTex, inTexStar, inCol, alpha, per_vertex, density, textures, stars);
   // skip dither for MS because it won't be noticeable
   return col;
}
// Textures Flat
TECHNIQUE    (SkyTF1   , Sky_VS(false, false, false), Sky_PS (false, true , false, 1, false, false, false));
TECHNIQUE    (SkyTF2   , Sky_VS(false, false, false), Sky_PS (false, true , false, 2, false, false, false));
TECHNIQUE    (SkyTF1C  , Sky_VS(false, false, true ), Sky_PS (false, true , false, 1, false, true , false));
TECHNIQUE    (SkyTF2C  , Sky_VS(false, false, true ), Sky_PS (false, true , false, 2, false, true , false));
TECHNIQUE    (SkyTF1D  , Sky_VS(false, false, false), Sky_PS (false, true , false, 1, false, false, true ));
TECHNIQUE    (SkyTF2D  , Sky_VS(false, false, false), Sky_PS (false, true , false, 2, false, false, true ));
TECHNIQUE    (SkyTF1CD , Sky_VS(false, false, true ), Sky_PS (false, true , false, 1, false, true , true ));
TECHNIQUE    (SkyTF2CD , Sky_VS(false, false, true ), Sky_PS (false, true , false, 2, false, true , true ));

// Textures
TECHNIQUE    (SkyT10   , Sky_VS(false, false, false), Sky_PS (false, false, false, 1, false, false, false));
TECHNIQUE    (SkyT20   , Sky_VS(false, false, false), Sky_PS (false, false, false, 2, false, false, false));
TECHNIQUE    (SkyT10D  , Sky_VS(false, false, false), Sky_PS (false, false, false, 1, false, false, true ));
TECHNIQUE    (SkyT20D  , Sky_VS(false, false, false), Sky_PS (false, false, false, 2, false, false, true ));
#if !CG // Multi Sample
TECHNIQUE    (SkyT11   , Sky_VS(false, false, false), Sky1_PS(false, false, 1, false, false));
TECHNIQUE    (SkyT21   , Sky_VS(false, false, false), Sky1_PS(false, false, 2, false, false));
TECHNIQUE_4_1(SkyT12   , Sky_VS(false, false, false), Sky2_PS(false, false, 1, false, false));
TECHNIQUE_4_1(SkyT22   , Sky_VS(false, false, false), Sky2_PS(false, false, 2, false, false));
TECHNIQUE    (SkyT11D  , Sky_VS(false, false, false), Sky1_PS(false, false, 1, false, true ));
TECHNIQUE    (SkyT21D  , Sky_VS(false, false, false), Sky1_PS(false, false, 2, false, true ));
TECHNIQUE_4_1(SkyT12D  , Sky_VS(false, false, false), Sky2_PS(false, false, 1, false, true ));
TECHNIQUE_4_1(SkyT22D  , Sky_VS(false, false, false), Sky2_PS(false, false, 2, false, true ));
#endif

// Atmospheric Flat
TECHNIQUE    (SkyAF    , Sky_VS(false, false, false), Sky_PS(false, true ,false, 0, false, false, false));
TECHNIQUE    (SkyAFV   , Sky_VS(true , false, false), Sky_PS(true , true ,false, 0, false, false, false));
TECHNIQUE    (SkyAFS   , Sky_VS(false, true , false), Sky_PS(false, true ,false, 0, true , false, false));
TECHNIQUE    (SkyAFVS  , Sky_VS(true , true , false), Sky_PS(true , true ,false, 0, true , false, false));
TECHNIQUE    (SkyAFC   , Sky_VS(false, false, true ), Sky_PS(false, true ,false, 0, false, true , false));
TECHNIQUE    (SkyAFVC  , Sky_VS(true , false, true ), Sky_PS(true , true ,false, 0, false, true , false));
TECHNIQUE    (SkyAFSC  , Sky_VS(false, true , true ), Sky_PS(false, true ,false, 0, true , true , false));
TECHNIQUE    (SkyAFVSC , Sky_VS(true , true , true ), Sky_PS(true , true ,false, 0, true , true , false));
TECHNIQUE    (SkyAFD   , Sky_VS(false, false, false), Sky_PS(false, true ,false, 0, false, false, true ));
TECHNIQUE    (SkyAFVD  , Sky_VS(true , false, false), Sky_PS(true , true ,false, 0, false, false, true ));
TECHNIQUE    (SkyAFSD  , Sky_VS(false, true , false), Sky_PS(false, true ,false, 0, true , false, true ));
TECHNIQUE    (SkyAFVSD , Sky_VS(true , true , false), Sky_PS(true , true ,false, 0, true , false, true ));
TECHNIQUE    (SkyAFCD  , Sky_VS(false, false, true ), Sky_PS(false, true ,false, 0, false, true , true ));
TECHNIQUE    (SkyAFVCD , Sky_VS(true , false, true ), Sky_PS(true , true ,false, 0, false, true , true ));
TECHNIQUE    (SkyAFSCD , Sky_VS(false, true , true ), Sky_PS(false, true ,false, 0, true , true , true ));
TECHNIQUE    (SkyAFVSCD, Sky_VS(true , true , true ), Sky_PS(true , true ,false, 0, true , true , true ));

// Atmospheric
TECHNIQUE    (SkyA0    , Sky_VS(false, false, false), Sky_PS(false, false, false, 0, false, false, false));
TECHNIQUE    (SkyAV0   , Sky_VS(true , false, false), Sky_PS(true , false, false, 0, false, false, false));
TECHNIQUE    (SkyAS0   , Sky_VS(false, true , false), Sky_PS(false, false, false, 0, true , false, false));
TECHNIQUE    (SkyAVS0  , Sky_VS(true , true , false), Sky_PS(true , false, false, 0, true , false, false));
TECHNIQUE    (SkyAP0   , Sky_VS(false, false, false), Sky_PS(false, false, true , 0, false, false, false));
TECHNIQUE    (SkyAVP0  , Sky_VS(true , false, false), Sky_PS(true , false, true , 0, false, false, false));
TECHNIQUE    (SkyASP0  , Sky_VS(false, true , false), Sky_PS(false, false, true , 0, true , false, false));
TECHNIQUE    (SkyAVSP0 , Sky_VS(true , true , false), Sky_PS(true , false, true , 0, true , false, false));
TECHNIQUE    (SkyA0D   , Sky_VS(false, false, false), Sky_PS(false, false, false, 0, false, false, true ));
TECHNIQUE    (SkyAV0D  , Sky_VS(true , false, false), Sky_PS(true , false, false, 0, false, false, true ));
TECHNIQUE    (SkyAS0D  , Sky_VS(false, true , false), Sky_PS(false, false, false, 0, true , false, true ));
TECHNIQUE    (SkyAVS0D , Sky_VS(true , true , false), Sky_PS(true , false, false, 0, true , false, true ));
TECHNIQUE    (SkyAP0D  , Sky_VS(false, false, false), Sky_PS(false, false, true , 0, false, false, true ));
TECHNIQUE    (SkyAVP0D , Sky_VS(true , false, false), Sky_PS(true , false, true , 0, false, false, true ));
TECHNIQUE    (SkyASP0D , Sky_VS(false, true , false), Sky_PS(false, false, true , 0, true , false, true ));
TECHNIQUE    (SkyAVSP0D, Sky_VS(true , true , false), Sky_PS(true , false, true , 0, true , false, true ));

#if !CG // Multi Sample
TECHNIQUE    (SkyA1    , Sky_VS(false, false, false), Sky1_PS(false, false, 0, false, false));
TECHNIQUE    (SkyAV1   , Sky_VS(true , false, false), Sky1_PS(true , false, 0, false, false));
TECHNIQUE    (SkyAS1   , Sky_VS(false, true , false), Sky1_PS(false, false, 0, true , false));
TECHNIQUE    (SkyAVS1  , Sky_VS(true , true , false), Sky1_PS(true , false, 0, true , false));
TECHNIQUE    (SkyAP1   , Sky_VS(false, false, false), Sky1_PS(false, true , 0, false, false));
TECHNIQUE    (SkyAVP1  , Sky_VS(true , false, false), Sky1_PS(true , true , 0, false, false));
TECHNIQUE    (SkyASP1  , Sky_VS(false, true , false), Sky1_PS(false, true , 0, true , false));
TECHNIQUE    (SkyAVSP1 , Sky_VS(true , true , false), Sky1_PS(true , true , 0, true , false));
TECHNIQUE    (SkyA1D   , Sky_VS(false, false, false), Sky1_PS(false, false, 0, false, true ));
TECHNIQUE    (SkyAV1D  , Sky_VS(true , false, false), Sky1_PS(true , false, 0, false, true ));
TECHNIQUE    (SkyAS1D  , Sky_VS(false, true , false), Sky1_PS(false, false, 0, true , true ));
TECHNIQUE    (SkyAVS1D , Sky_VS(true , true , false), Sky1_PS(true , false, 0, true , true ));
TECHNIQUE    (SkyAP1D  , Sky_VS(false, false, false), Sky1_PS(false, true , 0, false, true ));
TECHNIQUE    (SkyAVP1D , Sky_VS(true , false, false), Sky1_PS(true , true , 0, false, true ));
TECHNIQUE    (SkyASP1D , Sky_VS(false, true , false), Sky1_PS(false, true , 0, true , true ));
TECHNIQUE    (SkyAVSP1D, Sky_VS(true , true , false), Sky1_PS(true , true , 0, true , true ));

TECHNIQUE_4_1(SkyA2    , Sky_VS(false, false, false), Sky2_PS(false, false, 0, false, false));
TECHNIQUE_4_1(SkyAV2   , Sky_VS(true , false, false), Sky2_PS(true , false, 0, false, false));
TECHNIQUE_4_1(SkyAS2   , Sky_VS(false, true , false), Sky2_PS(false, false, 0, true , false));
TECHNIQUE_4_1(SkyAVS2  , Sky_VS(true , true , false), Sky2_PS(true , false, 0, true , false));
TECHNIQUE_4_1(SkyAP2   , Sky_VS(false, false, false), Sky2_PS(false, true , 0, false, false));
TECHNIQUE_4_1(SkyAVP2  , Sky_VS(true , false, false), Sky2_PS(true , true , 0, false, false));
TECHNIQUE_4_1(SkyASP2  , Sky_VS(false, true , false), Sky2_PS(false, true , 0, true , false));
TECHNIQUE_4_1(SkyAVSP2 , Sky_VS(true , true , false), Sky2_PS(true , true , 0, true , false));
TECHNIQUE_4_1(SkyA2D   , Sky_VS(false, false, false), Sky2_PS(false, false, 0, false, true ));
TECHNIQUE_4_1(SkyAV2D  , Sky_VS(true , false, false), Sky2_PS(true , false, 0, false, true ));
TECHNIQUE_4_1(SkyAS2D  , Sky_VS(false, true , false), Sky2_PS(false, false, 0, true , true ));
TECHNIQUE_4_1(SkyAVS2D , Sky_VS(true , true , false), Sky2_PS(true , false, 0, true , true ));
TECHNIQUE_4_1(SkyAP2D  , Sky_VS(false, false, false), Sky2_PS(false, true , 0, false, true ));
TECHNIQUE_4_1(SkyAVP2D , Sky_VS(true , false, false), Sky2_PS(true , true , 0, false, true ));
TECHNIQUE_4_1(SkyASP2D , Sky_VS(false, true , false), Sky2_PS(false, true , 0, true , true ));
TECHNIQUE_4_1(SkyAVSP2D, Sky_VS(true , true , false), Sky2_PS(true , true , 0, true , true ));
#endif
/******************************************************************************/
