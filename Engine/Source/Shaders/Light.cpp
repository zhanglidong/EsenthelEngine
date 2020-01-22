/******************************************************************************

   These shaders use 'discard' a few times to skip processing if light is known to be zero.

   However using 'discard' is not free:
      if no  pixels are discarded, then performance will be slower than without 'discard'
      if all pixels are discarded, then performance will be higher than without 'discard'
      benefit depends on amount of discarded pixels and the GPU

/******************************************************************************/
#include "!Header.h"
#include "Water.h"

#include "!Set Prec Struct.h"
BUFFER(LightMap)
   Flt LightMapScale=1;
BUFFER_END
#include "!Set Prec Default.h"

#define SHADOW_PERCEPTUAL (SHADOW && 0) // disable because in tests it looked a little worse, and also it requires 1 extra MUL instruction
/******************************************************************************/
void Geom_VS // for 3D Geom
(
           VtxInput vtx,
#if !GL_ES
   out NOPERSP Vec2 outTex  :TEXCOORD0,
   out NOPERSP Vec2 outPosXY:TEXCOORD1,
#endif
   out         Vec4 outPos  :POSITION
)
{
   outPos=Project(TransformPos(vtx.pos()));

#if CLAMP_DEPTH // simulate "D.depthClip(false)", needed for GL ES which doesn't support it, Warning: this introduces a bit too much clipping at the viewport end, because the neighboring vertexes remain the same, and only the vertex behind the viewport gets repositioned, the line between them won't cover entire original area (however it's small)
   #if REVERSE_DEPTH
      outPos.z=Max(outPos.z, 0);
   #else
      outPos.z=Min(outPos.z, outPos.w);
   #endif
#endif

#if !GL_ES
   outTex  =ProjectedPosToUV   (outPos);
   outPosXY=          UVToPosXY(outTex);
#endif
}
/******************************************************************************/
// Img=Nrm (this also used for Water Apply shader), ImgMS=NrmMS, Img1=Col, ImgMS1=ColMS, ImgXY=Ext, ImgXYMS=ExtMS, Img2=ConeLight.Lightmap, ImgX=shadow
/******************************************************************************/
VecH LightDir_PS
(
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1
#if MULTI_SAMPLE
 , NOPERSP PIXEL // 2D
 ,         UInt index  :SV_SampleIndex
#endif
 , out VecH outSpec:TARGET1
):TARGET
{
   // shadow (start with shadows because they're IMAGE_R8 and have small bandwidth)
#if MULTI_SAMPLE
   Half shadow; if(SHADOW)shadow=TexSample(ImgXMS, pixel.xy, index).x;
#else
   Half shadow; if(SHADOW)shadow=TexPoint(ImgX, inTex).x;
#endif
   if(SHADOW && shadow<=EPS_LUM)discard;

   // normal
#if MULTI_SAMPLE
   Vec4 nrm=GetNormalMS(pixel.xy, index);
#else
   Vec4 nrm=GetNormal(inTex);
#endif

   // light
   Vec light_dir=LightDir.dir;
   LightParams lp; lp.set(nrm.xyz, light_dir);
   Half lum=lp.NdotL; if(SHADOW)lum*=shadow; if(lum<=EPS_LUM)
   {
      if(!WATER && nrm.w && -lum>EPS_LUM){outSpec=0; return LightDir.color.rgb*(lum*-TRANSLUCENT_VAL);} // #RTOutput translucent
      discard; // !! have to skip when "NdotL<=0" to don't apply negative values to RT !!
   }
   if(SHADOW_PERCEPTUAL)lum*=shadow;

   // ext+col
#if WATER
   VecH2 ext      ={WaterMaterial.smooth, WaterMaterial.reflect}; // #RTOutput Water doesn't have EXT #WaterExt
   Half  unlit_col= WaterMaterial.reflect; // this is used for calculation of final reflectivity, just copy from water reflectivity #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext      =GetExtMS (pixel.xy, index);
   VecH  unlit_col=TexSample(ImgMS1, pixel.xy, index).rgb;
#else
   VecH2 ext      =GetExt  (inTex);
   VecH  unlit_col=TexPoint(Img1, inTex).rgb;
#endif

   // light #1
   Vec eye_dir=Normalize(Vec(inPosXY, 1));
   lp.set(nrm.xyz, light_dir, eye_dir);

   VecH    lum_rgb=LightDir.color.rgb*lum;
   outSpec=lum_rgb*lp.specular(ext.x, ext.y, ReflectCol(ext.y, unlit_col), true, LightDir.radius_frac); // specular #RTOutput
   return  lum_rgb*lp.diffuse (ext.x                                                                 ); // diffuse  #RTOutput
}
/******************************************************************************/
VecH LightPoint_PS
(
#if GL_ES // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1
#if MULTI_SAMPLE
 ,         PIXEL // 3D
 ,         UInt index  :SV_SampleIndex
#endif
#endif
 , out VecH outSpec:TARGET1
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToUV   (pixel);
   Vec2 inPosXY=   UVToPosXY(inTex);
#endif

   // shadow (start with shadows because they're IMAGE_R8 and have small bandwidth)
#if MULTI_SAMPLE
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW && shadow<=EPS_LUM)discard;

   // distance
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightPoint.pos-pos; Flt inv_dist2=1/Length2(delta);
   Half lum  =LightPointDist(inv_dist2); if(SHADOW)lum*=shadow; if(lum<=EPS_LUM)discard;

   // normal
#if MULTI_SAMPLE
   Vec4 nrm=GetNormalMS(pixel.xy, index);
#else
   Vec4 nrm=GetNormal(inTex);
#endif

   // light
   Vec light_dir=delta*Sqrt(inv_dist2); // Normalize(delta);
   LightParams lp; lp.set(nrm.xyz, light_dir);
   lum*=lp.NdotL; if(lum<=EPS_LUM)
   {
      if(!WATER && nrm.w && -lum>EPS_LUM){outSpec=0; return LightPoint.color.rgb*(lum*-TRANSLUCENT_VAL);} // #RTOutput translucent
      discard; // !! have to skip when "NdotL<=0" to don't apply negative values to RT !!
   }
   if(SHADOW_PERCEPTUAL)lum*=shadow;

   // ext+col
#if WATER
   VecH2 ext      ={WaterMaterial.smooth, WaterMaterial.reflect}; // #RTOutput Water doesn't have EXT #WaterExt
   Half  unlit_col= WaterMaterial.reflect; // this is used for calculation of final reflectivity, just copy from water reflectivity #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext      =GetExtMS (pixel.xy, index);
   VecH  unlit_col=TexSample(ImgMS1, pixel.xy, index).rgb;
#else
   VecH2 ext      =GetExt  (inTex);
   VecH  unlit_col=TexPoint(Img1, inTex).rgb;
#endif

   // light #1
   Vec eye_dir=Normalize(pos);
   lp.set(nrm.xyz, light_dir, eye_dir);

   VecH    lum_rgb=LightPoint.color.rgb*lum;
   outSpec=lum_rgb*lp.specular(ext.x, ext.y, ReflectCol(ext.y, unlit_col), true); // specular #RTOutput
   return  lum_rgb*lp.diffuse (ext.x                                           ); // diffuse  #RTOutput
}
/******************************************************************************/
VecH LightLinear_PS
(
#if GL_ES // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1
#if MULTI_SAMPLE
 ,         PIXEL // 3D
 ,         UInt index  :SV_SampleIndex
#endif
#endif
 , out VecH outSpec:TARGET1
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToUV   (pixel);
   Vec2 inPosXY=   UVToPosXY(inTex);
#endif

   // shadow (start with shadows because they're IMAGE_R8 and have small bandwidth)
#if MULTI_SAMPLE
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW && shadow<=EPS_LUM)discard;

   // distance
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightLinear.pos-pos; Flt dist=Length(delta);
   Half lum  =LightLinearDist(dist); if(SHADOW)lum*=shadow; if(lum<=EPS_LUM)discard;

   // normal
#if MULTI_SAMPLE
   Vec4 nrm=GetNormalMS(pixel.xy, index);
#else
   Vec4 nrm=GetNormal(inTex);
#endif

   // light
   Vec light_dir=delta/dist; // Normalize(delta);
   LightParams lp; lp.set(nrm.xyz, light_dir);
   lum*=lp.NdotL; if(lum<=EPS_LUM)
   {
      if(!WATER && nrm.w && -lum>EPS_LUM){outSpec=0; return LightLinear.color.rgb*(lum*-TRANSLUCENT_VAL);} // #RTOutput translucent
      discard; // !! have to skip when "NdotL<=0" to don't apply negative values to RT !!
   }
   if(SHADOW_PERCEPTUAL)lum*=shadow;

   // ext+col
#if WATER
   VecH2 ext      ={WaterMaterial.smooth, WaterMaterial.reflect}; // #RTOutput Water doesn't have EXT #WaterExt
   Half  unlit_col= WaterMaterial.reflect; // this is used for calculation of final reflectivity, just copy from water reflectivity #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext      =GetExtMS (pixel.xy, index);
   VecH  unlit_col=TexSample(ImgMS1, pixel.xy, index).rgb;
#else
   VecH2 ext      =GetExt  (inTex);
   VecH  unlit_col=TexPoint(Img1, inTex).rgb;
#endif

   // light #1
   Vec eye_dir=Normalize(pos);
   lp.set(nrm.xyz, light_dir, eye_dir);

   VecH    lum_rgb=LightLinear.color.rgb*lum;
   outSpec=lum_rgb*lp.specular(ext.x, ext.y, ReflectCol(ext.y, unlit_col), true); // specular #RTOutput
   return  lum_rgb*lp.diffuse (ext.x                                           ); // diffuse  #RTOutput
}
/******************************************************************************/
VecH LightCone_PS
(
#if GL_ES // doesn't support NOPERSP
   PIXEL // 3D
#else
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1
#if MULTI_SAMPLE
 ,         PIXEL // 3D
 ,         UInt index  :SV_SampleIndex
#endif
#endif
 , out VecH outSpec:TARGET1
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToUV   (pixel);
   Vec2 inPosXY=   UVToPosXY(inTex);
#endif

   // shadow (start with shadows because they're IMAGE_R8 and have small bandwidth)
#if MULTI_SAMPLE
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shadow; if(SHADOW)shadow=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW && shadow<=EPS_LUM)discard;

   // distance & angle
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightCone.pos-pos,
        dir  =TransformTP(delta, LightCone.mtrx); dir.xy/=dir.z; clip(Vec(1-Abs(dir.xy), dir.z));
   Flt  dist =Length(delta);
   Half lum  =LightConeAngle(dir.xy)*LightConeDist(dist); if(SHADOW)lum*=shadow; if(lum<=EPS_LUM)discard;

   // normal
#if MULTI_SAMPLE
   Vec4 nrm=GetNormalMS(pixel.xy, index);
#else
   Vec4 nrm=GetNormal(inTex);
#endif

   // light
   Vec light_dir=delta/dist; // Normalize(delta);
   LightParams lp; lp.set(nrm.xyz, light_dir);
   lum*=lp.NdotL; if(lum<=EPS_LUM)
   {
      if(!WATER && nrm.w && -lum>EPS_LUM) // #RTOutput translucent
      {
         outSpec=0;
         lum*=-TRANSLUCENT_VAL;
         VecH lum_rgb=LightCone.color.rgb*lum;
      #if IMAGE
         lum_rgb*=Tex(Img2, dir.xy*(LightMapScale*0.5)+0.5).rgb;
      #endif
         return lum_rgb;
      }
      discard; // !! have to skip when "NdotL<=0" to don't apply negative values to RT !!
   }
   if(SHADOW_PERCEPTUAL)lum*=shadow;

   // ext+col
#if WATER
   VecH2 ext      ={WaterMaterial.smooth, WaterMaterial.reflect}; // #RTOutput Water doesn't have EXT #WaterExt
   Half  unlit_col= WaterMaterial.reflect; // this is used for calculation of final reflectivity, just copy from water reflectivity #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext      =GetExtMS (pixel.xy, index);
   VecH  unlit_col=TexSample(ImgMS1, pixel.xy, index).rgb;
#else
   VecH2 ext      =GetExt  (inTex);
   VecH  unlit_col=TexPoint(Img1, inTex).rgb;
#endif

   // light #1
   Vec eye_dir=Normalize(pos);
   lp.set(nrm.xyz, light_dir, eye_dir);

   VecH lum_rgb=LightCone.color.rgb*lum;
#if IMAGE
   lum_rgb*=Tex(Img2, dir.xy*(LightMapScale*0.5)+0.5).rgb;
#endif

   outSpec=lum_rgb*lp.specular(ext.x, ext.y, ReflectCol(ext.y, unlit_col), true); // specular #RTOutput
   return  lum_rgb*lp.diffuse (ext.x                                           ); // diffuse  #RTOutput
}
/******************************************************************************/
