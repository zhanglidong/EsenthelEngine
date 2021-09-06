/******************************************************************************/
#include "!Header.h"
#include "Hdr.h"
/******************************************************************************/
#define BRIGHT    1 // if apply adjustment for scenes where half pixels are bright, and other half are dark, in that case prefer focus on brighter, to avoid making already bright pixels too bright
#define GEOMETRIC 0 // don't use geometric mean, because of cases when bright sky is mostly occluded by dark objects, then entire scene will get brighter, making the sky look too bright and un-realistic

#define MAX_LUM 16 // max value of linear color, keeping this low preserves contrast better
/******************************************************************************
// AMD LPM
#define Quart _Quart // "ffx_a.h" has its own 'Quart'
#define A_GPU  1
#define A_HLSL 1
#define A_HALF 1
#include "FidelityFX/ffx_a.h"

BUFFER(AMD_LPT)
   uint4 AMD_LPT_constant[24];
BUFFER_END
AU4 LpmFilterCtl(AU1 i) {return AMD_LPT_constant[i];}

#define LPM_NO_SETUP 1
#define AD1_(a) ((AD1)(a))
#define AF1_(a) ((AF1)(a))
#define AL1_(a) ((AL1)(a))
#define AU1_(a) ((AU1)(a))
#include "FidelityFX/ffx_lpm.h"
#undef Quart
/******************************************************************************/
// HDR
/******************************************************************************/
Flt HdrDS_PS(NOPERSP Vec2 uv:UV):TARGET
{
   Vec2 tex_min=uv-ImgSize.xy,
        tex_max=uv+ImgSize.xy;

#if STEP==0
   // use linear filtering because we're downsampling, for the first step use half precision for high performance, because there's a lot of data
   VecH sum=TexLod(Img, Vec2(tex_min.x, tex_min.y)).rgb
           +TexLod(Img, Vec2(tex_max.x, tex_min.y)).rgb
           +TexLod(Img, Vec2(tex_min.x, tex_max.y)).rgb
           +TexLod(Img, Vec2(tex_max.x, tex_max.y)).rgb;

#if !LINEAR_GAMMA // convert from sRGB to linear
   sum=SRGBToLinearFast(sum)/4; // SRGBToLinearFast(sum/4)*4
#endif

   Flt lum=Dot(sum, HdrWeight);

// adjustment
#if BRIGHT
   lum=Sqr(lum);
#endif
#if GEOMETRIC
   lum=log2(Max(lum, HALF_MIN)); // NaN
#endif

   return lum;
#else
   // use linear filtering because we're downsampling, here use full precision for more accuracy
   return Avg(TexLod(ImgXF, Vec2(tex_min.x, tex_min.y)).x,
              TexLod(ImgXF, Vec2(tex_max.x, tex_min.y)).x,
              TexLod(ImgXF, Vec2(tex_min.x, tex_max.y)).x,
              TexLod(ImgXF, Vec2(tex_max.x, tex_max.y)).x);
#endif
}
/******************************************************************************/
Flt HdrUpdate_PS():TARGET // here use full precision
{
   Flt lum=ImgXF[VecI2(0, 0)].x; // new luminance

   // adjustment restore
#if GEOMETRIC
   lum=exp2(lum); // we've applied 'log2' above, so revert it back
#endif
#if BRIGHT
   lum=Sqrt(lum); // we've applied 'Sqr' above, so revert it back
#endif

   lum=Pow(lum, HdrExp); //lum=Sqrt(lum); // if further from the target brightness, apply the smaller scale. When using a smaller 'HdrExp' then scale will be stretched towards "1" (meaning smaller changes), using exp=0.5 gives Sqrt(lum)

   lum=HdrBrightness/Max(lum, EPS_COL); // desired scale

   lum=Mid(lum, HdrMaxDark, HdrMaxBright);
   return Lerp(lum, ImgXF1[VecI2(0, 0)].x, Step); // lerp new with old
}
/******************************************************************************/
void DrawLine(inout VecH col, VecH line_col, Vec2 screen, Flt y)
{
   col=Lerp(col, line_col, Sat(Half(1-Abs(screen.y-y)*512)));
}
/******************************************************************************/
Half TonemapReinhard(Half x) {return x/(1+x);} // x=0..Inf
VecH TonemapReinhard(VecH x) {return x/(1+x);} // x=0..Inf

Half _TonemapReinhardML(Half x) {return (1+x/Sqr(MAX_LUM))/(1+x);} // Max Lum version x=0..MAX_LUM - internal without "x*"
VecH _TonemapReinhardML(VecH x) {return (1+x/Sqr(MAX_LUM))/(1+x);} // Max Lum version x=0..MAX_LUM - internal without "x*"

Half TonemapReinhardML(Half x) {return x*_TonemapReinhardML(x);} // Max Lum version x=0..MAX_LUM - full formula
VecH TonemapReinhardML(VecH x) {return x*_TonemapReinhardML(x);} // Max Lum version x=0..MAX_LUM - full formula

VecH TonemapReinhardLum  (VecH x) {Half lum=LinearLumOfLinearColor(x); return x/(1+lum)                ;} // x*(TonemapReinhard  (lum)/lum)
VecH TonemapReinhardMLLum(VecH x) {Half lum=LinearLumOfLinearColor(x); return x*_TonemapReinhardML(lum);} // x*(TonemapReinhardML(lum)/lum)

// Jodie - https://www.shadertoy.com/view/4dBcD1
VecH TonemapReinhardJodie(VecH x) // preserves saturation
{
   VecH d=TonemapReinhard   (x);
   VecH s=TonemapReinhardLum(x);
   return Lerp(s, d, d);
}
VecH TonemapReinhardJodieML(VecH x) // preserves saturation
{
   VecH d=TonemapReinhardML   (x);
   VecH s=TonemapReinhardMLLum(x);
   return Lerp(s, d, d);
}
void DarkenDarks(inout VecH x, Flt exp=1.6)
{  // parameters set to match ACES
   Half start=1.0/16;
   VecH step =Sat(x/start);
   x=Lerp(Pow(step, exp)*start, x, Sqr(step));
}
VecH TonemapReinhardJodieToe(VecH x, Flt exp) // preserves saturation and darkens darks
{
   x=TonemapReinhardJodie(x);
   DarkenDarks(x, exp); // better to do this after 'TonemapReinhardJodie' than before, curve looks smoother
   return x;
}
VecH TonemapReinhardJodieDDHalf(VecH x) {return TonemapReinhardJodieToe(x, 1.27);} // 1.27 is perceptually in the middle of 1.0 and 1.6
VecH TonemapReinhardJodieDDFull(VecH x) {return TonemapReinhardJodieToe(x, 1.6 );} // 1.6 matches ACES

// robobo1221 - https://www.shadertoy.com/view/4dBcD1 and https://www.shadertoy.com/view/ldlcWX
Half TonemapRobo(Half x) {return x/Sqrt(1+x*x);}
VecH TonemapRobo(VecH x) {return x/Sqrt(1+x*x);}

VecH TonemapPersson(VecH x) {return exp2(-x)*-SQRT2+SQRT2;} // (1-exp2(-x))*SQRT2 - Emil Persson
/******************************************************************************
AMD Tonemapper
AMD Cauldron code
Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */
Half ColToneB(Half hdrMax, Half contrast, Half shoulder, Half midIn, Half midOut) // General tonemapping operator, build 'b' term.
{
   return -((-Pow(midIn, contrast) + (midOut*(Pow(hdrMax, contrast*shoulder)*Pow(midIn, contrast) -
              Pow(hdrMax, contrast)*Pow(midIn, contrast*shoulder)*midOut)) /
             (Pow(hdrMax, contrast*shoulder)*midOut - Pow(midIn, contrast*shoulder)*midOut)) /
             (Pow(midIn, contrast*shoulder)*midOut));
}
Half ColToneC(Half hdrMax, Half contrast, Half shoulder, Half midIn, Half midOut) // General tonemapping operator, build 'c' term.
{
    return (Pow(hdrMax, contrast*shoulder)*Pow(midIn, contrast) - Pow(hdrMax, contrast)*Pow(midIn, contrast*shoulder)*midOut) /
           (Pow(hdrMax, contrast*shoulder)*midOut - Pow(midIn, contrast*shoulder)*midOut);
}
Half ColTone(Half x, VecH4 p) // General tonemapping operator, p := {contrast,shoulder,b,c}.
{ 
   Half   z= Pow(x, p.r); 
   return z/(Pow(z, p.g)*p.b + p.a); 
}
VecH TonemapAMD_Cauldron(VecH col)
{
   const Half hdrMax  =MAX_LUM; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
   const Half shoulder=1; // Likely don't need to mess with this factor, unless matching existing tonemapper is not working well..
   const Half contrast=1+1.0/16; // good values are 1+1.0/16=darks closest to original, 1+1.0/8=slightly higher contrast (darks darker, brights brighter)
   const Half midIn   =0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
   const Half midOut  =0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

   Half b=ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
   Half c=ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

   Half peak =Max(Max(col), HALF_MIN);
   VecH ratio=col/peak; // always 0..1
   peak=ColTone(peak, VecH4(contrast, shoulder, b, c)); // should be 0..1

   // ratio
   if(1) // better quality
   {
    //Half crossTalk      = 4; // controls amount of channel crossTalk
      Half      saturation= 1; // full tonal range saturation control, "1" works better (saturation looks closer to original) than "contrast" from original source code
      Half crossSaturation=16; // crossTalk saturation, using "16" or "contrast*16" as in original source code made no visual difference, so keep 16 as it might be faster

      // wrap crossTalk in transform
      ratio=Pow (ratio, saturation/crossSaturation); // ratio 0..1
      ratio=Lerp(ratio, 1, Quart(peak)); // Pow(peak, crossTalk), ratio 0..1
      ratio=Pow (ratio, crossSaturation); // ratio 0..1
   }else // faster but lower saturation for high values
      ratio=Lerp(ratio, 1, Quart(peak)); // ratio 0..1

   return peak*ratio;
}
/******************************************************************************
VecH TonemapAMD_LPM(VecH col) currently disabled on the CPU side
{
   LpmFilter(col.r, col.g, col.b, false, LPM_CONFIG_709_709);
   return col;
}
/******************************************************************************
Desaturates
VecH _TonemapHable(VecH x) // http://filmicworlds.com/blog/filmic-tonemapping-operators/
{
   Half A=0.15;
   Half B=0.50;
   Half C=0.10;
   Half D=0.20;
   Half E=0.02;
   Half F=0.30;
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
VecH TonemapHable(VecH col)
{
#if MAX_LUM // col=0..MAX_LUM
   #if   MAX_LUM==2
      Half scale=1.74936676;
   #elif MAX_LUM==4
      Half scale=2.55470848;
   #elif MAX_LUM==6
      Half scale=2.82278252;
   #elif MAX_LUM==8
      Half scale=2.95678377;
   #elif MAX_LUM==12
      Half scale=3.09075975;
   #elif MAX_LUM==16
      Half scale=3.15773392;
   #elif MAX_LUM==24
      Half scale=3.22470760;
   #elif MAX_LUM==32
      Half scale=3.25820065;
   #endif
   return _TonemapHable(scale*col)/_TonemapHable(scale*MAX_LUM);
#else // no limit = 0..Inf, this version is very similar to 'TonemapReinhard'
   return _TonemapHable(3.35864878*col)/0.93333333333; // max value of _TonemapHable is 0.93333333333 calculated based on _TonemapHable(65536*256).x
#endif
}
/* scale calculated using:
Vec TonemapHableNoLimit(Vec col, Flt scale)
{
   return _TonemapHable(scale*col)/0.93333333333;
}
Vec TonemapHableMaxLum(Vec col, Flt scale)
{
   return _TonemapHable(scale*col)/_TonemapHable(scale*MAX_LUM);
}
Flt scale=1, min=0, max=16;
REP(65536)
{
   scale=Avg(min, max);
   Flt x=1.0/256, h=TonemapHable(x, scale).x;
   if(h<x)min=scale;
   if(h>x)max=scale;
}
/******************************************************************************
VecH _TonemapUchimura(VecH x, Half P, Half a, Half m, Half l, Half c, Half b) // Uchimura 2017, "HDR theory and practice"
{  // Math: https://www.desmos.com/calculator/gslcdxvipg
   // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
   Half l0 = ((P - m) * l) / a;
   Half L0 = m - m / a;
   Half L1 = m + (1 - m) / a;
   Half S0 = m + l0;
   Half S1 = m + a * l0;
   Half C2 = (a * P) / (P - S1);
   Half CP = -C2 / P;

   VecH w0 = 1 - smoothstep(0, m, x);
   VecH w2 = step(m + l0, x);
   VecH w1 = 1 - w0 - w2;

   VecH T = m * Pow(x / m, c) + b;
   VecH S = P - (P - S1) * exp(CP * (x - S0));
   VecH L = m + a * (x - m);

   return T * w0 + L * w1 + S * w2;
}
VecH TonemapUchimura(VecH x, Half black=1) // 'black' can also be 1.33
{
   const Half P=1;     // max display brightness
   const Half a=1;     // contrast
   const Half m=0.22;  // linear section start
   const Half l=0.4;   // linear section length
   const Half c=black; // black
   const Half b=0;     // pedestal
   return _TonemapUchimura(x, P, a, m, l, c, b);
}
/******************************************************************************/
// TOE
/******************************************************************************/
VecH TonemapACESNarkowicz(VecH x) // Krzysztof Narkowicz "ACES Filmic Tone Mapping Curve" - https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
{
   const Half a=2.51;
   const Half b=0.03;
   const Half c=2.43;
   const Half d=0.59;
   const Half e=0.14;
   return (x*(a*x+b))/(x*(c*x+d)+e);
}
VecH ACESFilmRec2020(VecH x) // https://knarkowicz.wordpress.com/2016/08/31/hdr-display-first-steps/
{
   Half a=15.8;
   Half b=2.12;
   Half c=1.2;
   Half d=5.92;
   Half e=1.9;
   return (x*(a*x+b))/(x*(c*x+d)+e);
}
/******************************************************************************/
#define MUL 2 // to match 'TonemapACESNarkowicz'
static const MatrixH3 ACESInputMat = // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
{
   {0.59719*MUL, 0.35458*MUL, 0.04823*MUL},
   {0.07600*MUL, 0.90834*MUL, 0.01566*MUL},
   {0.02840*MUL, 0.13383*MUL, 0.83777*MUL},
};
static const MatrixH3 ACESOutputMat = // ODT_SAT => XYZ => D60_2_D65 => sRGB
{
   { 1.60475, -0.53108, -0.07367},
   {-0.10208,  1.10813, -0.00605},
   {-0.00327, -0.07276,  1.07602},
};
VecH RRTAndODTFit(VecH v)
{
   VecH a = v * (v + 0.0245786) - 0.000090537;
   VecH b = v * (0.983729 * v + 0.4329510) + 0.238081;
   return a/b;
}
VecH TonemapACESHill(VecH color) // Stephen Hill "self_shadow"
{
   color=mul(ACESInputMat, color);
   color=RRTAndODTFit(color); // Apply RRT and ODT
   color=mul(ACESOutputMat, color);
   color=Sat(color);
   return color;
}
/******************************************************************************/
VecH TonemapACESLottes(VecH x) // Timothy Lottes "Advanced Techniques and Optimization of HDR Color Pipelines" - https://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf
{
   const Half a     =1.6;
   const Half d     =0.977;
   const Half hdrMax=MAX_LUM;
   const Half midIn =0.18;
   const Half midOut=0.267;

   // can be precomputed
   const Half b = (-Pow(midIn, a) + Pow(hdrMax, a) * midOut) / ((Pow(hdrMax, a * d) - Pow(midIn, a * d)) * midOut);
   const Half c = (Pow(hdrMax, a * d) * Pow(midIn, a) - Pow(hdrMax, a) * Pow(midIn, a * d) * midOut) / ((Pow(hdrMax, a * d) - Pow(midIn, a * d)) * midOut);

   return Pow(x, a)/(Pow(x, a*d)*b+c);
}
/******************************************************************************
VecH TonemapUnreal(VecH x) // Unreal 3, Documentation: "Color Grading", adapted to be close to TonemapACES with similar range
{
   x=x/(x+0.155)*1.019;
   return SRGBToLinear(x);
}
/******************************************************************************
VecH ToneMapHejl(VecH col) // too dark
{
   VecH4  vh=VecH4(col, MAX_LUM);
   VecH4  va=(1.435*vh)+0.05;
   VecH4  vf=((vh*va+0.004)/((vh*(va+0.55)+0.0491)))-0.0821;
   return vf.xyz/vf.w;
}
/******************************************************************************/
VecH ToneMapHejlBurgessDawson(VecH col) // Jim Hejl + Richard Burgess-Dawson "Filmic" - http://filmicworlds.com/blog/filmic-tonemapping-operators/
{
   col=Max(0, col-0.004);
   col=(col*(6.2*col+0.5))/(col*(6.2*col+1.7)+0.06);
   return SRGBToLinear(col);
}
/******************************************************************************
VecH ToneMapRomBinDaHouse(VecH color)
{
   color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	return color;
}
/******************************************************************************/
void AdaptEye_VS(VtxInput vtx,
   NOPERSP  out Vec2 uv  :UV ,
   NOINTERP out Half lum :LUM,
   NOPERSP  out Vec4 vpos:POSITION)
{
   uv=vtx.uv();
   lum=ImgX[VecI2(0, 0)];
   vpos=vtx.pos4();
}
VecH4 AdaptEye_PS(NOPERSP  Vec2 uv :UV ,
                  NOINTERP Half lum:LUM,
                  NOPERSP  PIXEL       ):TARGET
{
   VecH4 col=TexLod(Img, uv); // can't use 'TexPoint' because 'Img' can be supersampled
   col.rgb*=lum;
#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
VecH4 ToneMap_PS(NOPERSP Vec2 uv:UV,
                 NOPERSP PIXEL     ):TARGET
{
   VecH4 col=TexLod(Img, uv); // can't use 'TexPoint' because 'Img' can be supersampled
#if !ALPHA
   col.a=1; // force full alpha so back buffer effects can work ok
#endif

#if TONE_MAP
   if(TONE_MAP==STONE_MAP_ROBO                    )col.rgb=TonemapRobo               (col.rgb);
   if(TONE_MAP==STONE_MAP_AMD_CAULDRON            )col.rgb=TonemapAMD_Cauldron       (col.rgb);
   if(TONE_MAP==STONE_MAP_REINHARD_JODIE          )col.rgb=TonemapReinhardJodie      (col.rgb);
   if(TONE_MAP==STONE_MAP_REINHARD_JODIE_DARK_HALF)col.rgb=TonemapReinhardJodieDDHalf(col.rgb);
   if(TONE_MAP==STONE_MAP_REINHARD_JODIE_DARK     )col.rgb=TonemapReinhardJodieDDFull(col.rgb);
   if(TONE_MAP==STONE_MAP_ACES_HILL               )col.rgb=TonemapACESHill           (col.rgb);
   if(TONE_MAP==STONE_MAP_ACES_NARKOWICZ          )col.rgb=TonemapACESNarkowicz      (col.rgb);
   if(TONE_MAP==STONE_MAP_ACES_LOTTES             )col.rgb=TonemapACESLottes         (col.rgb);
   if(TONE_MAP==STONE_MAP_HEJL_BURGESS_DAWSON     )col.rgb=ToneMapHejlBurgessDawson  (col.rgb);
#endif

#if 0 // Debug Drawing
   Vec2 pos=Vec2(uv.x*AspectRatio, 1-uv.y);
   //if(AL)pos*=2;
   //pos*=1.0/16;
   if(CT || SH)DrawLine(col.rgb, VecH(1,1,1), pos, pos.x);
   if(CT)
   {
      DrawLine(col.rgb, VecH(0.5,0,0), pos, TonemapRobo(pos.x));
      DrawLine(col.rgb, VecH(0,0.5,0), pos, TonemapAMD_Cauldron(pos.x));
    //DrawLine(col.rgb, VecH(0,0,0.5), pos, TonemapHable(pos.x));
      DrawLine(col.rgb, VecH(0,0,0.5), pos, TonemapReinhardJodie(pos.x));
    //DrawLine(col.rgb, VecH(0.5,0.5,0), pos, TonemapReinhardJodieToe(pos.x));
    //DrawLine(col.rgb, VecH(0,0.5,0.5), pos, TonemapAMD_LPM(pos.x));
    //DrawLine(col.rgb, VecH(0,0.5,0.5), pos, TonemapUchimura(pos.x));
    //DrawLine(col.rgb, VecH(0.5,0,0.5), pos, TonemapPersson(pos.x));
    //DrawLine(col.rgb, VecH(1,1,1), pos, TonemapUchimura(pos.x, 1.33));
    //DrawLine(col.rgb, VecH(0.5,0.5,0), pos, TonemapReinhard(pos.x));/**/
   }
   if(SH)
   {
      DrawLine(col.rgb, VecH(1,0,0), pos, TonemapACESNarkowicz(pos.x));
      DrawLine(col.rgb, VecH(0,1,0), pos, TonemapACESLottes(pos.x));
    //DrawLine(col.rgb, VecH(0,0,1), pos, TonemapUnreal(pos.x));
      DrawLine(col.rgb, VecH(0,1,1), pos, ToneMapHejlBurgessDawson(pos.x));
    //DrawLine(col.rgb, VecH(1,0,1), pos, TonemapUchimura(pos.x, 1.33));
    //DrawLine(col.rgb, VecH(1,1,0), pos, ToneMapHejl(pos.x));
    //DrawLine(col.rgb, VecH(1,0,1), pos, ToneMapRomBinDaHouse(pos.x));
      DrawLine(col.rgb, VecH(1,1,1), pos, TonemapACESHill(pos.x));
   }
#endif

#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
