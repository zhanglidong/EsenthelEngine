/******************************************************************************/
#include "!Header.h"
BUFFER(LightMap)
   Flt LightMapScale=1;
BUFFER_END
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

#if CLAMP_DEPTH // simulate D.depthClip(false), needed for GL ES which doesn't support it, Warning: this introduces a bit too much clipping at the viewport end, because the neighboring vertexes remain the same, and only the vertex behind the viewport gets repositioned, the line between them won't cover entire original area (however it's small)
   #if REVERSE_DEPTH
      outPos.z=Max(outPos.z, 0);
   #else
      outPos.z=Min(outPos.z, outPos.w);
   #endif
#endif

#if !GL_ES
   outTex  =PosToScreen  (outPos);
   outPosXY=ScreenToPosXY(outTex);
#endif
}
/******************************************************************************/
// Img=Nrm (this also used for Water Apply shader), ImgMS=NrmMS, ImgXY=Ext, ImgXYMS=ExtMS, Img1=ConeLight.Lightmap, ImgX=shadow
/******************************************************************************/
VecH4 LightDir_PS
(
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1
#if MULTI_SAMPLE
 , NOPERSP PIXEL // 2D
 ,         UInt index  :SV_SampleIndex
#endif
):TARGET
{
   // shadow
#if MULTI_SAMPLE
   Half shd; if(SHADOW)shd=TexSample(ImgXMS, pixel.xy, index).x;
#else
   Half shd; if(SHADOW)shd=TexPoint(ImgX, inTex).x;
#endif
   if(SHADOW)clip(shd-EPS_LUM);

   // normal
#if MULTI_SAMPLE
   VecH nrm=GetNormalMS(pixel.xy, index);
#else
   VecH nrm=GetNormal(inTex);
#endif

   // light
   LightParams lp; lp.set(nrm, LightDir.dir);
   Half lum=lp.NdotL; if(SHADOW)lum*=shd; clip(lum-EPS_LUM); // !! have to skip when <= NdotL to don't apply negative values to RT !!

   // ext
#if WATER
   VecH2 ext={WATER_SMOOTH, WATER_REFLECT}; // #RTOutput Water doesn't have EXT #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext=GetExtMS(pixel.xy, index);
#else
   VecH2 ext=GetExt(inTex);
#endif

   // light #1
   VecH eye_dir=Normalize(Vec(inPosXY, 1));
   lp.set(nrm, LightDir.dir, eye_dir);

   // specular
   Half specular=lp.specular(ext.x, ext.y)*lum; // #RTOutput

   // diffuse !! after specular because it adjusts 'lum' !!
   lum*=lp.diffuse(ext.x); // #RTOutput

   return VecH4(LightDir.color.rgb*lum, LightDir.color.a*specular);
}
/******************************************************************************/
VecH4 LightPoint_PS
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
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToScreen(pixel);
   Vec2 inPosXY=ScreenToPosXY(inTex);
#endif

   // shadow
#if MULTI_SAMPLE
   Half shd; if(SHADOW)shd=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shd; if(SHADOW)shd=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW)clip(shd-EPS_LUM);

   // distance
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightPoint.pos-pos; Flt inv_dist2=1/Length2(delta);
   Half lum  =LightPointDist(inv_dist2); if(SHADOW)lum*=shd; clip(lum-EPS_LUM);

   // normal
#if MULTI_SAMPLE
   VecH nrm=GetNormalMS(pixel.xy, index);
#else
   VecH nrm=GetNormal(inTex);
#endif

   // light
   VecH light_dir=delta*Sqrt(inv_dist2); // Normalize(delta);
   LightParams lp; lp.set(nrm, light_dir);
   lum*=lp.NdotL; clip(lum-EPS_LUM); // !! have to skip when <= NdotL to don't apply negative values to RT !!

   // ext
#if WATER
   VecH2 ext={WATER_SMOOTH, WATER_REFLECT}; // #RTOutput Water doesn't have EXT #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext=GetExtMS(pixel.xy, index);
#else
   VecH2 ext=GetExt(inTex);
#endif

   // light #1
   VecH eye_dir=Normalize(pos);
   lp.set(nrm, light_dir, eye_dir);

   // specular
   Half specular=lp.specular(ext.x, ext.y)*lum; // #RTOutput

   // diffuse !! after specular because it adjusts 'lum' !!
   lum*=lp.diffuse(ext.x); // #RTOutput

   return VecH4(LightPoint.color.rgb*lum, LightPoint.color.a*specular);
}
/******************************************************************************/
VecH4 LightLinear_PS
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
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToScreen(pixel);
   Vec2 inPosXY=ScreenToPosXY(inTex);
#endif

   // shadow
#if MULTI_SAMPLE
   Half shd; if(SHADOW)shd=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shd; if(SHADOW)shd=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW)clip(shd-EPS_LUM);

   // distance
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightLinear.pos-pos; Flt dist=Length(delta);
   Half lum  =LightLinearDist(dist); if(SHADOW)lum*=shd; clip(lum-EPS_LUM);

   // normal
#if MULTI_SAMPLE
   VecH nrm=GetNormalMS(pixel.xy, index);
#else
   VecH nrm=GetNormal(inTex);
#endif

   // light
   VecH light_dir=delta/dist; // Normalize(delta);
   LightParams lp; lp.set(nrm, light_dir);
   lum*=lp.NdotL; clip(lum-EPS_LUM); // !! have to skip when <= NdotL to don't apply negative values to RT !!

   // ext
#if WATER
   VecH2 ext={WATER_SMOOTH, WATER_REFLECT}; // #RTOutput Water doesn't have EXT #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext=GetExtMS(pixel.xy, index);
#else
   VecH2 ext=GetExt(inTex);
#endif

   // light #1
   VecH eye_dir=Normalize(pos);
   lp.set(nrm, light_dir, eye_dir);

   // specular
   Half specular=lp.specular(ext.x, ext.y)*lum; // #RTOutput

   // diffuse !! after specular because it adjusts 'lum' !!
   lum*=lp.diffuse(ext.x); // #RTOutput

   return VecH4(LightLinear.color.rgb*lum, LightLinear.color.a*specular);
}
/******************************************************************************/
VecH4 LightCone_PS
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
):TARGET
{
#if GL_ES // doesn't support NOPERSP
   Vec2 inTex  =PixelToScreen(pixel);
   Vec2 inPosXY=ScreenToPosXY(inTex);
#endif

   // shadow
#if MULTI_SAMPLE
   Half shd; if(SHADOW)shd=ShadowFinal(TexSample(ImgXMS, pixel.xy, index).x);
#else
   Half shd; if(SHADOW)shd=ShadowFinal(TexPoint(ImgX, inTex).x);
#endif
   if(SHADOW)clip(shd-EPS_LUM);

   // distance & angle
#if MULTI_SAMPLE
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
#else
   Vec pos=GetPosPoint(inTex, inPosXY);
#endif
   Vec  delta=LightCone.pos-pos,
        dir  =TransformTP(delta, LightCone.mtrx); dir.xy/=dir.z; clip(Vec(1-Abs(dir.xy), dir.z));
   Flt  dist =Length(delta);
   Half lum  =LightConeAngle(dir.xy)*LightConeDist(dist); if(SHADOW)lum*=shd; clip(lum-EPS_LUM);

   // normal
#if MULTI_SAMPLE
   VecH nrm=GetNormalMS(pixel.xy, index);
#else
   VecH nrm=GetNormal(inTex);
#endif

   // light
   VecH light_dir=delta/dist; // Normalize(delta);
   LightParams lp; lp.set(nrm, light_dir);
   lum*=lp.NdotL; clip(lum-EPS_LUM); // !! have to skip when <= NdotL to don't apply negative values to RT !!

   // ext
#if WATER
   VecH2 ext={WATER_SMOOTH, WATER_REFLECT}; // #RTOutput Water doesn't have EXT #WaterExt
#elif MULTI_SAMPLE
   VecH2 ext=GetExtMS(pixel.xy, index);
#else
   VecH2 ext=GetExt(inTex);
#endif

   // light #1
   VecH eye_dir=Normalize(pos);
   lp.set(nrm, light_dir, eye_dir);

   // specular
   Half specular=lp.specular(ext.x, ext.y)*lum; // #RTOutput

   // diffuse !! after specular because it adjusts 'lum' !!
   lum*=lp.diffuse(ext.x); // #RTOutput

#if IMAGE
   VecH map_col=Tex(Img1, dir.xy*(LightMapScale*0.5)+0.5).rgb;
   return VecH4(LightCone.color.rgb*lum*map_col, LightCone.color.a*specular);
#else
   return VecH4(LightCone.color.rgb*lum, LightCone.color.a*specular);
#endif
}
/******************************************************************************/
