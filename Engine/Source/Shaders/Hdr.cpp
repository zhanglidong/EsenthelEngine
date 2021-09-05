/******************************************************************************/
#include "!Header.h"
#include "Hdr.h"

// shader expects linear gamma

#define BRIGHT    1 // if apply adjustment for scenes where half pixels are bright, and other half are dark, in that case prefer focus on brighter, to avoid making already bright pixels too bright
#define GEOMETRIC 0 // don't use geometric mean, because of cases when bright sky is mostly occluded by dark objects, then entire scene will get brighter, making the sky look too bright and un-realistic

#define MAX_LUM 16 // max value of linear color, keeping this low preserves contrast better
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
Half TonemapReinhard(Half x) {return x/(1+x);}
VecH TonemapReinhard(VecH x) {return x/(1+x);}

Half _TonemapReinhardML(Half x) {return (1+x/Sqr(MAX_LUM))/(1+x);} // Max Lum version - internal without "x*"
VecH _TonemapReinhardML(VecH x) {return (1+x/Sqr(MAX_LUM))/(1+x);} // Max Lum version - internal without "x*"

Half TonemapReinhardML(Half x) {return x*_TonemapReinhardML(x);} // Max Lum version - full formula
VecH TonemapReinhardML(VecH x) {return x*_TonemapReinhardML(x);} // Max Lum version - full formula

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

// https://www.shadertoy.com/view/4dBcD1 and https://www.shadertoy.com/view/ldlcWX
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
VecH TonemapAMD(VecH col)
{
   const Half hdrMax  =MAX_LUM; // How much HDR range before clipping. HDR modes likely need this pushed up to say 25.0.
   const Half shoulder=1; // Likely don't need to mess with this factor, unless matching existing tonemapper is not working well..
   const Half contrast=1+1.0/16;
   const Half midIn   =0.18; // most games will have a {0.0 to 1.0} range for LDR so midIn should be 0.18.
   const Half midOut  =0.18; // Use for LDR. For HDR10 10:10:10:2 use maybe 0.18/25.0 to start. For scRGB, I forget what a good starting point is, need to re-calculate.

   Half b=ColToneB(hdrMax, contrast, shoulder, midIn, midOut);
   Half c=ColToneC(hdrMax, contrast, shoulder, midIn, midOut);

   Half peak =Max(Max(col), HALF_MIN);
   VecH ratio=col/peak;
   peak=ColTone(peak, VecH4(contrast, shoulder, b, c));

   // ratio
   if(0)
   {
      Half crosstalk = 4; // controls amount of channel crosstalk
      Half saturation = 1; // full tonal range saturation control
      Half crossSaturation = 1*16; // crosstalk saturation use 1 for faster performance

      // wrap crosstalk in transform
      ratio = Pow(ratio, saturation / crossSaturation);
      ratio = Lerp(ratio, 1, Pow(peak, crosstalk));
      ratio = Pow(ratio, crossSaturation);
   }else // faster but slightly lower color accuracy for high values
      ratio=Lerp(ratio, 1, Quart(peak));

   return peak*ratio;
}
/******************************************************************************/
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
#if   MAX_LUM==2
   Half scale=1.28622985;
#elif MAX_LUM==4
   Half scale=1.86127877;
#elif MAX_LUM==6
   Half scale=2.18674517;
#elif MAX_LUM==8
   Half scale=2.39616013;
#elif MAX_LUM==12
   Half scale=2.64986658;
#elif MAX_LUM==16
   Half scale=2.79797029;
#elif MAX_LUM==24
   Half scale=2.96359253;
#elif MAX_LUM==32
   Half scale=3.05397701;
#endif
   return _TonemapHable(scale*col)/_TonemapHable(MAX_LUM);
}
/* scale calculated using:
Vec TonemapHable(Vec col, Flt scale)
{
   return _TonemapHable(scale*col)/_TonemapHable(MAX_LUM);
}
Flt scale=1, min=0, max=16;
REP(65536)
{
   scale=Avg(min, max);
   Flt x=0.01, h=TonemapHable(x, scale).x;
   if(h<x)min=scale;
   if(h>x)max=scale;
}
/******************************************************************************/
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
   const Half P = 1;     // max display brightness
   const Half a = 1;     // contrast
   const Half m = 0.22;  // linear section start
   const Half l = 0.4;   // linear section length
   const Half c = black; // black
   const Half b = 0;     // pedestal
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
/******************************************************************************/
VecH TonemapLottes(VecH x) // Timothy Lottes "Advanced Techniques and Optimization of HDR Color Pipelines" - https://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf
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
static const MatrixH3 ACESInputMat = // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
{
   {0.59719, 0.35458, 0.04823},
   {0.07600, 0.90834, 0.01566},
   {0.02840, 0.13383, 0.83777},
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
VecH TonemapUnreal(VecH x) // Unreal 3, Documentation: "Color Grading", adapted to be close to TonemapACES with similar range
{
   x=x/(x+0.155)*1.019;
   return SRGBToLinear(x);
}
/******************************************************************************/
VecH ToneMapHejl(VecH col)
{
   VecH4  vh=VecH4(col, MAX_LUM);
   VecH4  va=(1.435*vh)+0.05;
   VecH4  vf=((vh*va+0.004)/((vh*(va+0.55)+0.0491)))-0.0821;
   return vf.xyz/vf.w;
}
/******************************************************************************/
VecH ToneMapFilmic(VecH col) // Jim Hejl + Richard Burgess-Dawson - http://filmicworlds.com/blog/filmic-tonemapping-operators/
{
   col=Max(0, col-0.004);
   col=(col*(6.2*col+0.5)) / (col*(6.2*col+1.7)+0.06);
   return SRGBToLinear(col);
}
/******************************************************************************
VecH ToneMapRomBinDaHouse(VecH color)
{
   color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	return color;
}
/******************************************************************************/
void Hdr_VS(VtxInput vtx,
   NOPERSP  out Vec2 uv  :UV ,
#if ADAPT_EYE
   NOINTERP out Half lum :LUM,
#endif
   NOPERSP  out Vec4 vpos:POSITION)
{
   uv=vtx.uv();

#if ADAPT_EYE
   lum=ImgX[VecI2(0, 0)];
#endif

   vpos=vtx.pos4();
}
VecH4 Hdr_PS(NOPERSP  Vec2 uv :UV ,
#if ADAPT_EYE
             NOINTERP Half lum:LUM,
#endif
             NOPERSP  PIXEL       ):TARGET
{
   VecH4 col=TexLod(Img, uv); // can't use 'TexPoint' because 'Img' can be supersampled

#if ADAPT_EYE
   col.rgb*=lum;
#endif


#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
