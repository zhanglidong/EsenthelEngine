/******************************************************************************/
#include "!Header.h"
#include "!Set HP.h"

BUFFER(ApplyLight)
   VecH NightShadeColor;
BUFFER_END
#include "!Set LP.h"

#define AO_ALL 1  // !! must be the same as 'D._amb_all' !! if apply Ambient Occlusion to all lights (not just Ambient), this was disabled in the past, however in LINEAR_GAMMA the darkening was too strong in low light, enabling this option solves that problem
/******************************************************************************/
Half CelShade(Half lum) {return TexLod(Img2, VecH2(lum, 0.5)).x;} // have to use linear filtering
/******************************************************************************/
VecH LitCol(VecH4 color, VecH4 lum, Half ao, VecH night_shade_col, Bool apply_ao)
{
   // treat glow as if it's a light source, this will have 2 effects: 1) pixels will have color even without any lights 2) this will disable night shade effects and retain original color (not covered by night shade), this is because 'night_shade_intensity' is multiplied by "Sat(1-max_lum)"
#if 0 // simply adding doesn't provide good results
   lum.rgb+=color.w;
#else // instead lerp to 1, to avoid glow pixels getting too bright, because if color is mostly red (255, 40, 20), but if too much light is applied, then it could become more white 10*(255, 40, 20)=(2550, 400, 200), and we want pixels to glow with exact color as on the texture
   #if 0 // slower
      lum.rgb=Lerp(lum.rgb, 1, color.w);
   #else // faster
      lum.rgb=lum.rgb*(1-color.w) + color.w;
   #endif
#endif
   Half max_lum=Max(lum.rgb);
   if(CEL_SHADE)
   {
      max_lum=CelShade(max_lum);
      lum.rgb=max_lum;
   }
   Half spec   =lum.w/Max(max_lum, HALF_MIN);
   VecH lit_col=(color.rgb+spec)*lum.rgb;
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
   return lit_col;
}
/******************************************************************************/
// Img=Col, ImgMS=ColMS, Img1=Lum, ImgMS1=LumMS, Img2=CelShade, ImgX=AO
VecH4 ApplyLight_PS(NOPERSP Vec2 inTex:TEXCOORD,
                    NOPERSP PIXEL):TARGET
{
   Half ao; VecH ambient; if(AO){ao=TexLod(ImgX, inTex).x; if(!AO_ALL)ambient=AmbColor*ao;} // use 'TexLod' because AO can be of different size and we need to use tex filtering
   VecI p=VecI(pixel.xy, 0);
   if(MULTI_SAMPLE==0)
   {
   #if !GL // does not work on SpirV -> GLSL
      VecH4 color=Img .Load(p),
            lum  =Img1.Load(p);
   #else
      VecH4 color=TexPoint(Img , inTex),
            lum  =TexPoint(Img1, inTex);
   #endif
      if(AO && !AO_ALL)lum.rgb+=ambient;
             color.rgb=LitCol(color, lum, ao, NightShadeColor, AO && !AO_ALL); if(AO && AO_ALL)color*=ao;
      return color;
   }else
   if(MULTI_SAMPLE==1) // 1 sample
   {
      VecH4  color=TexSample(ImgMS , pixel.xy, 0),
             lum  =TexSample(ImgMS1, pixel.xy, 0), // needed because Mesh Ambient is stored only in Multi Sampled Lum
             lum1s=Img1.Load(p);
             lum +=lum1s; 
      if(AO && !AO_ALL)lum.rgb+=ambient;
             color.rgb=LitCol(color, lum, ao, NightShadeColor, AO && !AO_ALL); if(AO && AO_ALL)color*=ao;
      return color;
   }else // n samples
   {
      VecH4 color_sum=0;
      Half  valid_samples=HALF_MIN;
      VecH  night_shade_col; if(NIGHT_SHADE && AO && !AO_ALL)night_shade_col=NightShadeColor*ao; // compute it once, and not inside 'LitCol'
      UNROLL for(Int i=0; i<MS_SAMPLES; i++)if(DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, i))) // valid sample
      {
         VecH4 color=TexSample(ImgMS , pixel.xy, i),
               lum  =TexSample(ImgMS1, pixel.xy, i);
         if(AO && !AO_ALL)lum.rgb+=ambient;
         color.rgb =LitCol(color, lum, ao, (NIGHT_SHADE && AO && !AO_ALL) ? night_shade_col : NightShadeColor, false); // we've already adjusted 'night_shade_col' by 'ao', so set 'apply_ao' as false
         color_sum+=color;
         valid_samples++;
      }
      if(AO && AO_ALL)color_sum*=ao/valid_samples;
      else            color_sum/=   valid_samples; // MS_SAMPLES
      return color_sum;
   }
}
/******************************************************************************/
