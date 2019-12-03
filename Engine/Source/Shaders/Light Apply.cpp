/******************************************************************************/
#include "!Header.h"
#include "Light Apply.h"

#define AO_ALL 1  // !! must be the same as 'D.aoAll()' !! if apply Ambient Occlusion to all lights (not just Ambient), this was disabled in the past, however in LINEAR_GAMMA the darkening was too strong in low light, enabling this option solves that problem

// MULTI_SAMPLE, AO, CEL_SHADE, NIGHT_SHADE, GLOW, REFLECT
// Img=Nrm, ImgMS=Nrm, Img1=Col, ImgMS1=Col, Img2=Lum, ImgMS2=Lum, Img3=Spec, ImgMS3=Spec, ImgXY=Ext, ImgXYMS=Ext, ImgX=AO, Img4=CelShade
/******************************************************************************/
Half CelShade(Half lum) {return TexLod(Img4, VecH2(lum, 0.5)).x;} // have to use linear filtering
/******************************************************************************/
VecH LitCol(VecH4 color, Vec nrm, VecH2 ext, VecH lum, VecH spec, Half ao, VecH night_shade_col, Bool apply_ao, Vec eye_dir)
{
#if GLOW
      // treat glow as if it's a light source, this will have 2 effects: 1) pixels will have color even without any lights 2) this will disable night shade effects and retain original color (not covered by night shade), this is because 'night_shade_intensity' is multiplied by "Sat(1-max_lum)"
   #if 0 // simply adding doesn't provide good results
      lum+=color.w;
   #else // instead lerp to 1, to avoid glow pixels getting too bright, because if color is mostly red (255, 40, 20), but if too much light is applied, then it could become more white 10*(255, 40, 20)=(2550, 400, 200), and we want pixels to glow with exact color as on the texture
      #if 0 // slower
         lum=Lerp(lum, 1, color.w);
      #else // faster
         lum=lum*(1-color.w) + color.w;
      #endif
   #endif
#endif
   Half max_lum=Max(lum);
   if(CEL_SHADE)
   {
      max_lum=CelShade(max_lum);
      lum=max_lum;
   }
   VecH lit_col=color.rgb*lum;
   if(NIGHT_SHADE)
   {
   #if LINEAR_GAMMA
      Half night_shade_intensity=Sat(1-max_lum)                     // only for low light
                                *LinearLumOfLinearColor(color.rgb); // set based on unlit color luminance
   #else
      Half night_shade_intensity=Sat(1-max_lum)                 // only for low light
                                *SRGBLumOfSRGBColor(color.rgb); // set based on unlit color luminance
   #endif
      if(apply_ao)night_shade_intensity*=ao;
         lit_col+=night_shade_intensity*night_shade_col;
   }
   Half smooth=ext.x, reflectivity=ext.y; // #RTOutput
#if REFLECT
   lit_col=PBR(color.rgb, lit_col, nrm, smooth, reflectivity, eye_dir, spec);
#else
   Half inv_metal=ReflectToInvMetal(reflectivity);
   lit_col=lit_col*Diffuse(inv_metal) + spec;
#endif
   return lit_col;
}
/******************************************************************************/
VecH4 ApplyLight_PS(NOPERSP Vec2 inTex  :TEXCOORD ,
                    NOPERSP Vec2 inPosXY:TEXCOORD1,
                    NOPERSP PIXEL):TARGET
{
   Half ao; VecH ambient; if(AO){ao=TexLod(ImgX, inTex).x; if(!AO_ALL)ambient=AmbientColor*ao;} // use 'TexLod' because AO can be of different size and we need to use tex filtering
   VecI p=VecI(pixel.xy, 0);
   Vec  eye_dir=Normalize(Vec(inPosXY, 1));
   if(MULTI_SAMPLE==0)
   {
   #if !GL // does not work on SpirV -> GLSL
      VecH4 color=Img1.Load(p);
      VecH  lum  =Img2.Load(p).rgb;
      VecH  spec =Img3.Load(p).rgb;
   #else
      VecH4 color=TexPoint(Img1, inTex);
      VecH  lum  =TexPoint(Img2, inTex).rgb;
      VecH  spec =TexPoint(Img3, inTex).rgb;
   #endif
      Vec   nrm=GetNormal(inTex).xyz;
      VecH2 ext=GetExt   (inTex);
      if(AO && !AO_ALL)lum+=ambient;
      color.rgb=LitCol(color, nrm, ext, lum, spec, ao, NightShadeColor, AO && !AO_ALL, eye_dir);
      if(AO && AO_ALL)color*=ao;
      return color;
   }else
   if(MULTI_SAMPLE==1) // 1 sample
   {
      VecH4  color=TexSample  (ImgMS1, pixel.xy, 0);
      VecH   lum  =Img2.Load(p).rgb; // Lum1S
      VecH   spec =Img3.Load(p).rgb; // Spec1S
      Vec    nrm  =GetNormalMS(        pixel.xy, 0).xyz;
      VecH2  ext  =GetExtMS   (        pixel.xy, 0);
             lum +=TexSample  (ImgMS2, pixel.xy, 0).rgb; // LumMS, needed because Mesh Ambient is stored only in Multi Sampled Lum
      if(AO && !AO_ALL)lum+=ambient;
      color.rgb=LitCol(color, nrm, ext, lum, spec, ao, NightShadeColor, AO && !AO_ALL, eye_dir);
      if(AO && AO_ALL)color*=ao;
      return color;
   }else // n samples
   {
      VecH4 color_sum=0;
      Half  valid_samples=HALF_MIN;
      VecH  night_shade_col; if(NIGHT_SHADE && AO && !AO_ALL)night_shade_col=NightShadeColor*ao; // compute it once, and not inside 'LitCol'
      UNROLL for(Int i=0; i<MS_SAMPLES; i++)if(DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, i))) // valid sample
      {
         VecH4 color=TexSample  (ImgMS1, pixel.xy, i);
         VecH  lum  =TexSample  (ImgMS2, pixel.xy, i).rgb;
         VecH  spec =TexSample  (ImgMS3, pixel.xy, i).rgb;
         Vec   nrm  =GetNormalMS(        pixel.xy, i).xyz;
         VecH2 ext  =GetExtMS   (        pixel.xy, i);
         if(AO && !AO_ALL)lum+=ambient;
         color.rgb =LitCol(color, nrm, ext, lum, spec, ao, (NIGHT_SHADE && AO && !AO_ALL) ? night_shade_col : NightShadeColor, false, eye_dir); // we've already adjusted 'night_shade_col' by 'ao', so set 'apply_ao' as false
         color_sum+=color;
         valid_samples++;
      }
      if(AO && AO_ALL)color_sum*=ao/valid_samples;
      else            color_sum/=   valid_samples; // MS_SAMPLES
      return color_sum;
   }
}
/******************************************************************************/
