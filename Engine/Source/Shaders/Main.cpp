/******************************************************************************/
#include "!Header.h"
#include "Ambient Occlusion.h"
#include "Sky.h"
#include "Layered Clouds.h"
#include "Hdr.h"
#include "Water.h"
#include "Overlay.h"
#include "Simple.h"
#include "Fog.h"
#include "Fur.h"
/******************************************************************************
VecH4 Test_PS(NOPERSP Vec2 inTex:TEXCOORD,
              uniform Int  mode):TARGET
{
   if(mode) // half
   {
      min16float2 t=(min16float2)inTex;
      min16float2 r=1;
      LOOP for(Int i=0; i<256; i++)
      {
         r+=t*r;
      }
      return VecH4((VecH2)r, mode, 1);
   }else
   {
      Vec2 t=inTex;
      Vec2 r=1;
      LOOP for(Int i=0; i<256; i++)
      {
         r+=t*r;
      }
      return VecH4(r, mode, 1);
   }
}
#define TECHNIQUE5(name, vs, ps)   technique11 name{pass p0{SetVertexShader(CompileShader(vs_5_0, vs)); SetPixelShader(CompileShader(ps_5_0, ps));}}
TECHNIQUE5(Test , Draw_VS(), Test_PS(0));
TECHNIQUE5(Test1, Draw_VS(), Test_PS(1));

TECHNIQUE(Test , Draw_VS(), Test_PS(0));
TECHNIQUE(Test1, Draw_VS(), Test_PS(1));
/******************************************************************************/
VecH4 TexCubicFast(Vec2 inTex)
{
#if 0 // original
   Vec2  tex =inTex*ImgSize.zw-0.5,
         texi=Floor(tex);
   VecH2 texf=tex-texi;
         texi-=0.5; texi*=ImgSize.xy;

   VecH4 c00=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 0)), c10=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 0)), c20=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 0)), c30=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 0)),
         c01=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 1)), c11=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 1)), c21=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 1)), c31=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 1)),
         c02=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 2)), c12=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 2)), c22=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 2)), c32=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 2)),
         c03=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 3)), c13=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 3)), c23=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 3)), c33=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 3));

   VecH4 c0=Lerp4(c00, c10, c20, c30, texf.x),
         c1=Lerp4(c01, c11, c21, c31, texf.x),
         c2=Lerp4(c02, c12, c22, c32, texf.x),
         c3=Lerp4(c03, c13, c23, c33, texf.x);

   return Lerp4(c0, c1, c2, c3, texf.y);
#else // optimized
   inTex*=ImgSize.zw;
   Vec2 tc=Floor(inTex-0.5)+0.5;
   VecH2 f=inTex-tc, f2=f*f, f3=f2*f,
        w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
   #if 0
        w2=-1.5*f3+2*f2+0.5*f, w3=0.5*(f3-f2);
   #else
        w3=0.5*(f3-f2), w2=1.0-w0-w1-w3;
   #endif

   tc*=ImgSize.xy;
   Vec2 tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
#if 0 // 16 tex reads
   Vec2 tc2=tc+ImgSize.xy;

 /*Flt w[4][4]={(w0.x*w0.y), (w1.x*w0.y), (w2.x*w0.y), (w3.x*w0.y),
                (w0.x*w1.y), (w1.x*w1.y), (w2.x*w1.y), (w3.x*w1.y),
                (w0.x*w2.y), (w1.x*w2.y), (w2.x*w2.y), (w3.x*w2.y),
                (w0.x*w3.y), (w1.x*w3.y), (w2.x*w3.y), (w3.x*w3.y)};*/

   return TexPoint(Img, Vec2(tc0.x, tc0.y))*(w0.x*w0.y)
         +TexPoint(Img, Vec2(tc .x, tc0.y))*(w1.x*w0.y)
         +TexPoint(Img, Vec2(tc0.x, tc .y))*(w0.x*w1.y)
         +TexPoint(Img, Vec2(tc .x, tc .y))*(w1.x*w1.y)

         +TexPoint(Img, Vec2(tc2.x, tc0.y))*(w2.x*w0.y)
         +TexPoint(Img, Vec2(tc3.x, tc0.y))*(w3.x*w0.y)
         +TexPoint(Img, Vec2(tc2.x, tc .y))*(w2.x*w1.y)
         +TexPoint(Img, Vec2(tc3.x, tc .y))*(w3.x*w1.y)
 
         +TexPoint(Img, Vec2(tc0.x, tc2.y))*(w0.x*w2.y)
         +TexPoint(Img, Vec2(tc .x, tc2.y))*(w1.x*w2.y)
         +TexPoint(Img, Vec2(tc0.x, tc3.y))*(w0.x*w3.y)
         +TexPoint(Img, Vec2(tc .x, tc3.y))*(w1.x*w3.y)

         +TexPoint(Img, Vec2(tc2.x, tc2.y))*(w2.x*w2.y)
         +TexPoint(Img, Vec2(tc3.x, tc2.y))*(w3.x*w2.y)
         +TexPoint(Img, Vec2(tc2.x, tc3.y))*(w2.x*w3.y)
         +TexPoint(Img, Vec2(tc3.x, tc3.y))*(w3.x*w3.y);
#else // 5 tex reads, corners are ignored because they're insignificant, 
   VecH2 w12=w1+w2; Vec2 p=tc+(w2/w12)*ImgSize.xy;
   Half  wu=w12.x*w0.y, wd=w12.x*w3.y, wl=w12.y*w0.x, wr=w12.y*w3.x, wc=w12.x*w12.y;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return(Tex(Img, Vec2(  p.x, tc0.y))*wu // sample upper edge (2 texels), both weights are negative
         +Tex(Img, Vec2(  p.x, tc3.y))*wd // sample lower edge (2 texels), both weights are negative
         +Tex(Img, Vec2(tc0.x,   p.y))*wl // sample left  edge (2 texels), both weights are negative
         +Tex(Img, Vec2(tc3.x,   p.y))*wr // sample right edge (2 texels), both weights are negative
         +Tex(Img, Vec2(  p.x,   p.y))*wc // sample center     (4 texels), all  weights are positive
         )/(wu+wd+wl+wr+wc);
#endif
#endif
}
VecH TexCubicFastRGB(Vec2 inTex) // ignores alpha channel
{
   inTex*=ImgSize.zw;
   Vec2 tc=Floor(inTex-0.5)+0.5;
   VecH2 f=inTex-tc, f2=f*f, f3=f2*f,
        w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
        w3=0.5*(f3-f2), w2=1.0-w0-w1-w3;
   tc*=ImgSize.xy;
   Vec2  tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
   VecH2 w12=w1+w2; Vec2 p=tc+(w2/w12)*ImgSize.xy;
   Half  wu =w12.x*w0.y, wd=w12.x*w3.y, wl=w12.y*w0.x, wr=w12.y*w3.x, wc=w12.x*w12.y;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return(Tex(Img, Vec2(  p.x, tc0.y)).rgb*wu
         +Tex(Img, Vec2(  p.x, tc3.y)).rgb*wd
         +Tex(Img, Vec2(tc0.x,   p.y)).rgb*wl
         +Tex(Img, Vec2(tc3.x,   p.y)).rgb*wr
         +Tex(Img, Vec2(  p.x,   p.y)).rgb*wc)/(wu+wd+wl+wr+wc);
}
/******************************************************************************/
#define CUBIC_SAMPLES     3
#define CUBIC_RANGE       2
#define CUBIC_SHARPNESS   (2/2.5)
#define CUBIC_QUALITY     2 // 0..2, 0=3.748 fps, 1=3.242 fps, 2=3.075 fps (however when using CUBIC_SKIP_SAMPLE it's now much faster)
#if !CG
   #define CUBIC_SKIP_SAMPLE (1 && CUBIC_QUALITY==2) // because the actual range is 2.5 then it means we need to process 5x5 samples (and not 6x6), this optimization can work only if actual range <= 2.5, also we can enable this only for CUBIC_QUALITY==2 because only this mode operates on single 1x1 pixels and not 2x2 blocks)
#else
   #define CUBIC_SKIP_SAMPLE 1 // otherwise fails to compile
#endif

inline Flt Cubic(Flt x, uniform Flt blur, uniform Flt sharpen)
{
   Flt x2=x*x,
       x3=x*x*x;
   return (x<=1) ? ((12-9*blur-6*sharpen)/6*x3 + (-18+12*blur+6*sharpen)/6*x2 +                             (6-2*blur         )/6)
                 : ((-blur-6*sharpen    )/6*x3 + (6*blur+30*sharpen    )/6*x2 + (-12*blur-48*sharpen)/6*x + (8*blur+24*sharpen)/6);
}
inline Half Cubic(Half x, uniform Half blur, uniform Half sharpen)
{
   Half x2=x*x,
        x3=x*x*x;
   return (x<=1) ? ((12-9*blur-6*sharpen)/6*x3 + (-18+12*blur+6*sharpen)/6*x2 +                             (6-2*blur         )/6)
                 : ((-blur-6*sharpen    )/6*x3 + (6*blur+30*sharpen    )/6*x2 + (-12*blur-48*sharpen)/6*x + (8*blur+24*sharpen)/6);
}
Flt  CubicMed(Flt  x) {return Cubic(x, 0.0, 0.375);}
Half CubicMed(Half x) {return Cubic(x, 0.0, 0.375);}

/*VecH4 TexLerp(Flt x0, Flt x1, Flt y, Flt l, Flt r)
{
   Flt w=l+r;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, Vec2((x0*l + x1*r)/w, y))*Half(w);
}
VecH TexLerpRGB(Flt x0, Flt x1, Flt y, Flt l, Flt r) // ignores alpha channel
{
   Flt w=l+r;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, Vec2((x0*l + x1*r)/w, y)).rgb*Half(w);
}

VecH4 TexLerp(Vec2 t0, Vec2 t1, Flt lu, Flt ru, Flt lb, Flt rb)
{
#if 0 // slower
   return TexPoint(Img, Vec2(t0.x, t0.y))*lu
         +TexPoint(Img, Vec2(t1.x, t0.y))*ru
         +TexPoint(Img, Vec2(t0.x, t1.y))*lb
         +TexPoint(Img, Vec2(t1.x, t1.y))*rb;
#else
   Vec2 t=Vec2(t0.x, t0.y)*lu
         +Vec2(t1.x, t0.y)*ru
         +Vec2(t0.x, t1.y)*lb
         +Vec2(t1.x, t1.y)*rb;
   Flt w=lu+ru+lb+rb;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, t/w)*Half(w);
#endif
}
VecH TexLerpRGB(Vec2 t0, Vec2 t1, Flt lu, Flt ru, Flt lb, Flt rb) // ignores alpha channel
{
   Vec2 t=Vec2(t0.x, t0.y)*lu
         +Vec2(t1.x, t0.y)*ru
         +Vec2(t0.x, t1.y)*lb
         +Vec2(t1.x, t1.y)*rb;
   Flt w=lu+ru+lb+rb;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, t/w).rgb*Half(w);
}*/

VecH4 TexCubic(Vec2 inTex)
{
   Vec2  pixel =inTex*ImgSize.zw-0.5,
         pixeli=Floor(pixel),
         offset       [CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];
   VecH2 offset_weight[CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];

#if CUBIC_SKIP_SAMPLE
   pixeli+=(pixel-pixeli>=CUBIC_RANGE/CUBIC_SHARPNESS-(CUBIC_SAMPLES-1)); // if the left/top coordinate is completely out of range, then process the next pixel (the >= returns Vec2, so it modifies X and Y independently)
#endif

   UNROLL for(int i=0; i<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; i++)
   {
      offset       [i]=     pixeli+(i-(CUBIC_SAMPLES-1));
      offset_weight[i]=Sqr((pixel -offset[i])*CUBIC_SHARPNESS);
      offset       [i]=           (offset[i]+0.5)*ImgSize.xy;
   }
   VecH4 color =0;
   Half  weight=0;
#if CUBIC_QUALITY>=2
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; x++)
   {
      Half w=offset_weight[x].x+offset_weight[y].y; if(w<Sqr(CUBIC_RANGE))
      {
         w=CubicMed(Sqrt(w));
         color +=w*TexPoint(Img, Vec2(offset[x].x, offset[y].y)); // don't use "color+=w*Img.Load(VecI(offset[x].x without scale, offset[y].y without scale, 0)); because it is slower and doesn't support wrap/clamp
         weight+=w;
      }
   }
#else
   Flt weights[CUBIC_SAMPLES*2][CUBIC_SAMPLES*2]; // [y][x]
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x++)
   {
      Flt w=offset_weight[x].x+offset_weight[y].y;
          w=(w<Sqr(CUBIC_RANGE) ? CubicMed(Sqrt(w)) : 0);
      weights[y][x]=w;
      weight      +=w;
   }
   #if CUBIC_QUALITY>=1 // medium quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++ )
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerp(offset[x].x, offset[x+1].x, offset[y].y, weights[y][x], weights[y][x+1]);
   #else // low quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y+=2)
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerp(Vec2(offset[x].x, offset[y].y), Vec2(offset[x+1].x, offset[y+1].y), weights[y][x], weights[y][x+1], weights[y+1][x], weights[y+1][x+1]);
   #endif
#endif
   return color/weight;
}
VecH TexCubicRGB(Vec2 inTex) // ignores alpha channel
{
   Vec2  pixel =inTex*ImgSize.zw-0.5,
         pixeli=Floor(pixel),
         offset       [CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];
   VecH2 offset_weight[CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];

#if CUBIC_SKIP_SAMPLE
   pixeli+=(pixel-pixeli>=CUBIC_RANGE/CUBIC_SHARPNESS-(CUBIC_SAMPLES-1)); // if the left/top coordinate is completely out of range, then process the next pixel (the >= returns Vec2, so it modifies X and Y independently)
#endif

   UNROLL for(int i=0; i<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; i++)
   {
      offset       [i]=     pixeli+(i-(CUBIC_SAMPLES-1));
      offset_weight[i]=Sqr((pixel -offset[i])*CUBIC_SHARPNESS);
      offset       [i]=           (offset[i]+0.5)*ImgSize.xy;
   }
   VecH color =0;
   Half weight=0;
#if CUBIC_QUALITY>=2 // high quality
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; x++)
   {
      Half w=offset_weight[x].x+offset_weight[y].y; if(w<Sqr(CUBIC_RANGE))
      {
         w=CubicMed(Sqrt(w));
         color +=w*TexPoint(Img, Vec2(offset[x].x, offset[y].y)).rgb; // don't use "color+=w*Img.Load(VecI(offset[x].x without scale, offset[y].y without scale, 0)).rgb; because it is slower and doesn't support wrap/clamp
         weight+=w;
      }
   }
#else
   Flt weights[CUBIC_SAMPLES*2][CUBIC_SAMPLES*2]; // [y][x]
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x++)
   {
      Flt w=offset_weight[x].x+offset_weight[y].y;
          w=(w<Sqr(CUBIC_RANGE) ? CubicMed(Sqrt(w)) : 0);
      weights[y][x]=w;
      weight      +=w;
   }
   #if CUBIC_QUALITY>=1 // medium quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++ )
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerpRGB(offset[x].x, offset[x+1].x, offset[y].y, weights[y][x], weights[y][x+1]);
   #else // low quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y+=2)
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerpRGB(Vec2(offset[x].x, offset[y].y), Vec2(offset[x+1].x, offset[y+1].y), weights[y][x], weights[y][x+1], weights[y+1][x], weights[y+1][x+1]);
   #endif
#endif
   return color/weight;
}
/******************************************************************************/
// SHADERS
/******************************************************************************/
Vec4 Draw2DFlat_VS(VtxInput vtx):POSITION {return Vec4(vtx.pos2()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);}
Vec4 Draw3DFlat_VS(VtxInput vtx):POSITION {return Project(TransformPos(vtx.pos()));}

VecH4 DrawFlat_PS():TARGET {return Color[0];}

TECHNIQUE(Draw2DFlat, Draw2DFlat_VS(), DrawFlat_PS());
TECHNIQUE(Draw3DFlat, Draw3DFlat_VS(), DrawFlat_PS());
#if !DX // THERE IS A BUG ON NVIDIA GEFORCE DX10+ when trying to clear normal render target using SetCol "Bool clear_nrm=(_nrm && !NRM_CLEAR_START && ClearNrm());", with D.depth2DOn(true) entire RT is cleared instead of background pixels only, this was verified on Windows 10 GeForce 650m, drivers 381, TODO: check again in the future
TECHNIQUE(SetCol    , Draw_VS      (), DrawFlat_PS()); // this version fails on DX
#else // this version works OK on DX
void SetCol_VS(VtxInput vtx,
           out VecH4 outCol:COLOR   ,
           out Vec4  outVtx:POSITION)
{
   outCol=Color[0];
   outVtx=Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
VecH4 SetCol_PS(NOPERSP VecH4 inCol:COLOR):TARGET {return inCol;}
TECHNIQUE(SetCol, SetCol_VS(), SetCol_PS());
#endif
/******************************************************************************/
void Draw2DCol_VS(VtxInput vtx,
              out VecH4 outCol:COLOR   ,
              out Vec4  outVtx:POSITION)
{
   outCol=     vtx.color();
   outVtx=Vec4(vtx.pos2 ()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);
}
VecH4 Draw2DCol_PS(NOPERSP VecH4 inCol:COLOR):TARGET {return inCol;}

TECHNIQUE(Draw2DCol, Draw2DCol_VS(), Draw2DCol_PS());
/******************************************************************************/
void Draw3DCol_VS(VtxInput vtx,
              out VecH4 outCol:COLOR   ,
              out Vec4  outVtx:POSITION)
{
   outCol=vtx.color();
   outVtx=Project(TransformPos(vtx.pos()));
}
VecH4 Draw3DCol_PS(VecH4 inCol:COLOR):TARGET {return inCol;}

TECHNIQUE(Draw3DCol, Draw3DCol_VS(), Draw3DCol_PS());
/******************************************************************************/
VecH4 Draw2DTex_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return       Tex(Img, inTex);}
VecH4 Draw2DTexC_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return       Tex(Img, inTex)*Color[0]+Color[1];}
VecH4 Draw2DTexA_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(Tex(Img, inTex).rgb, Step);}

VecH4 DrawTexX_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(Tex(Img, inTex).xxx, 1);}
VecH4 DrawTexY_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(Tex(Img, inTex).yyy, 1);}
VecH4 DrawTexZ_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(Tex(Img, inTex).zzz, 1);}
VecH4 DrawTexW_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(Tex(Img, inTex).www, 1);}

// these functions are used in Editor for previewing material textures, so use slow high precision versions to visually match original
VecH4 DrawTexG_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return SRGBToLinear(Tex(Img, inTex)                  );}
VecH4 DrawTexCG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return SRGBToLinear(Tex(Img, inTex)*Color[0]+Color[1]);}

VecH4 DrawTexXG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).x).xxx, 1);}
VecH4 DrawTexYG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).y).xxx, 1);}
VecH4 DrawTexZG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).z).xxx, 1);}
VecH4 DrawTexWG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).w).xxx, 1);}

VecH4 DrawTexNrm_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH nrm; nrm.xy=Tex(Img, inTex).xy*2-1; // #MaterialTextureChannelOrder
             nrm.z =CalcZ(nrm.xy);
             nrm   =Normalize(nrm)*0.5+0.5;
          #if LINEAR_GAMMA
             nrm   =SRGBToLinear(nrm);
          #endif
   return VecH4(nrm, 1);
}

TECHNIQUE(Draw2DTex , Draw2DTex_VS(),  Draw2DTex_PS());
TECHNIQUE(Draw2DTexC, Draw2DTex_VS(), Draw2DTexC_PS());

TECHNIQUE(DrawTexX, Draw2DTex_VS(), DrawTexX_PS());
TECHNIQUE(DrawTexY, Draw2DTex_VS(), DrawTexY_PS());
TECHNIQUE(DrawTexZ, Draw2DTex_VS(), DrawTexZ_PS());
TECHNIQUE(DrawTexW, Draw2DTex_VS(), DrawTexW_PS());

TECHNIQUE(DrawTexXG, Draw2DTex_VS(), DrawTexXG_PS());
TECHNIQUE(DrawTexYG, Draw2DTex_VS(), DrawTexYG_PS());
TECHNIQUE(DrawTexZG, Draw2DTex_VS(), DrawTexZG_PS());
TECHNIQUE(DrawTexWG, Draw2DTex_VS(), DrawTexWG_PS());

TECHNIQUE(DrawTexNrm, Draw2DTex_VS(), DrawTexNrm_PS());
TECHNIQUE(Draw      ,      Draw_VS(),  Draw2DTex_PS());
TECHNIQUE(DrawC     ,      Draw_VS(), Draw2DTexC_PS());
TECHNIQUE(DrawCG    ,      Draw_VS(), DrawTexCG_PS());
TECHNIQUE(DrawG     ,      Draw_VS(), DrawTexG_PS());
TECHNIQUE(DrawA     ,      Draw_VS(), Draw2DTexA_PS());

VecH4 DrawX_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(             Tex(ImgX, inTex)   .xxx, 1);}
VecH4 DrawXG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(ImgX, inTex).x).xxx, 1);}
VecH4 DrawXC_PS(NOPERSP Vec2 inTex:TEXCOORD,
                NOPERSP PIXEL,
        uniform Bool dither=false,
        uniform Bool gamma =false):TARGET
{
   VecH4 col=Tex(ImgX, inTex).x*Color[0]+Color[1];
   if(dither)ApplyDither(col.rgb, pixel.xy, LINEAR_GAMMA && !gamma); // don't perform gamma conversions inside dither if "gamma==true", because this means we have sRGB color which we're going to convert to linear below
   if(gamma )col.rgb=SRGBToLinearFast(col.rgb); // this is used for drawing sun rays, 'SRGBToLinearFast' works better here than 'SRGBToLinear' (gives high contrast, dark colors remain darker, while 'SRGBToLinear' highlights them more)
   return col;
}
TECHNIQUE(DrawX   , Draw_VS(), DrawX_PS ());
TECHNIQUE(DrawXG  , Draw_VS(), DrawXG_PS());
TECHNIQUE(DrawXC  , Draw_VS(), DrawXC_PS(false));
TECHNIQUE(DrawXCD , Draw_VS(), DrawXC_PS(true ));
TECHNIQUE(DrawXCG , Draw_VS(), DrawXC_PS(false, true));
TECHNIQUE(DrawXCDG, Draw_VS(), DrawXC_PS(true , true));

VecH4 DrawTexPoint_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return TexPoint(Img, inTex);}
VecH4 DrawTexPointC_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return TexPoint(Img, inTex)*Color[0]+Color[1];}
TECHNIQUE(DrawTexPoint , Draw2DTex_VS(), DrawTexPoint_PS ());
TECHNIQUE(DrawTexPointC, Draw2DTex_VS(), DrawTexPointC_PS());
/******************************************************************************/
void Draw2DTexCol_VS(VtxInput vtx,
                 out Vec2  outTex:TEXCOORD,
                 out VecH4 outCol:COLOR   ,
                 out Vec4  outVtx:POSITION)
{
   outTex=     vtx.tex  ();
   outCol=     vtx.color();
   outVtx=Vec4(vtx.pos2 ()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);
}
VecH4 Draw2DTexCol_PS(NOPERSP Vec2  inTex:TEXCOORD,
                      NOPERSP VecH4 inCol:COLOR   ):TARGET
{
   return Tex(Img, inTex)*inCol;
}
TECHNIQUE(Draw2DTexCol, Draw2DTexCol_VS(), Draw2DTexCol_PS());
/******************************************************************************/
void Draw3DTex_VS(VtxInput vtx,
              out Vec2  outTex:TEXCOORD,
              out VecH4 outFog:COLOR   ,
              out Vec4  outVtx:POSITION,
          uniform Bool  fog=false)
{
   Vec pos=TransformPos(vtx.pos());
          outTex=vtx.tex();
   if(fog)outFog=VecH4(FogColor, AccumulatedDensity(FogDensity, Length(pos)));
          outVtx=Project(pos);
}
void Draw2DDepthTex_VS(VtxInput vtx,
                   out Vec2  outTex:TEXCOORD,
                   out VecH4 outFog:COLOR   ,
                   out Vec4  outVtx:POSITION)
{
   outTex=vtx.tex();
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, DelinearizeDepth(vtx.posZ()), 1);
}
VecH4 Draw3DTex_PS(Vec2  inTex:TEXCOORD,
                   VecH4 inFog:COLOR   ,
           uniform Bool  alpha_test    ,
           uniform Bool  fog           ):TARGET
{
   VecH4 col=Tex(Img, inTex);
   if(alpha_test)clip(col.a-0.5);
   if(fog)col.rgb=Lerp(col.rgb, inFog.rgb, inFog.a);
   return col;
}
TECHNIQUE(Draw3DTex   , Draw3DTex_VS(false), Draw3DTex_PS(false, false));
TECHNIQUE(Draw3DTexAT , Draw3DTex_VS(false), Draw3DTex_PS(true , false));
TECHNIQUE(Draw3DTexF  , Draw3DTex_VS(true ), Draw3DTex_PS(false, true ));
TECHNIQUE(Draw3DTexATF, Draw3DTex_VS(true ), Draw3DTex_PS(true , true ));

TECHNIQUE(Draw2DDepthTex  , Draw2DDepthTex_VS(), Draw3DTex_PS(false, false));
TECHNIQUE(Draw2DDepthTexAT, Draw2DDepthTex_VS(), Draw3DTex_PS(true , false));
/******************************************************************************/
void Draw3DTexCol_VS(VtxInput vtx,
                 out Vec2  outTex:TEXCOORD,
                 out VecH4 outCol:COLOR   ,
                 out VecH4 outFog:COLOR1  ,
                 out Vec4  outVtx:POSITION,
             uniform Bool  fog=false)
{
   Vec pos=TransformPos(vtx.pos());
          outTex=vtx.tex  ();
          outCol=vtx.color();
   if(fog)outFog=VecH4(FogColor, AccumulatedDensity(FogDensity, Length(pos)));
          outVtx=Project(pos);
}
void Draw2DDepthTexCol_VS(VtxInput vtx,
                      out Vec2  outTex:TEXCOORD,
                      out VecH4 outCol:COLOR   ,
                      out VecH4 outFog:COLOR1  ,
                      out Vec4  outVtx:POSITION)
{
   outTex=vtx.tex  ();
   outCol=vtx.color();
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, DelinearizeDepth(vtx.posZ()), 1);
}
Vec4 Draw3DTexCol_PS(Vec2  inTex:TEXCOORD,
                     VecH4 inCol:COLOR   ,
                     VecH4 inFog:COLOR1  ,
             uniform Bool  alpha_test    ,
             uniform Bool  fog           ):TARGET
{
   VecH4 col=Tex(Img, inTex);
   if(alpha_test)clip(col.a-0.5);
   col*=inCol;
   if(fog)col.rgb=Lerp(col.rgb, inFog.rgb, inFog.a);
   return col;
}
TECHNIQUE(Draw3DTexCol   , Draw3DTexCol_VS(false), Draw3DTexCol_PS(false, false));
TECHNIQUE(Draw3DTexColAT , Draw3DTexCol_VS(false), Draw3DTexCol_PS(true , false));
TECHNIQUE(Draw3DTexColF  , Draw3DTexCol_VS(true ), Draw3DTexCol_PS(false, true ));
TECHNIQUE(Draw3DTexColATF, Draw3DTexCol_VS(true ), Draw3DTexCol_PS(true , true ));

TECHNIQUE(Draw2DDepthTexCol  , Draw2DDepthTexCol_VS(), Draw3DTexCol_PS(false, false));
TECHNIQUE(Draw2DDepthTexColAT, Draw2DDepthTexCol_VS(), Draw3DTexCol_PS(true , false));
/******************************************************************************/
VecH4 DrawTexCubicFast_PS(NOPERSP Vec2 inTex:TEXCOORD,
                          NOPERSP PIXEL,
                  uniform Bool color,
                  uniform Bool dither):TARGET
{
   VecH4 col=TexCubicFast(inTex);
   if(color)col=col*Color[0]+Color[1];
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
VecH4 DrawTexCubicFastRGB_PS(NOPERSP Vec2 inTex:TEXCOORD,
                             NOPERSP PIXEL,
                     uniform Bool dither=false):TARGET
{
   VecH4 col=VecH4(TexCubicFastRGB(inTex), 1);
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
TECHNIQUE(DrawTexCubicFast    , Draw2DTex_VS(), DrawTexCubicFast_PS(false, false));
TECHNIQUE(DrawTexCubicFastC   , Draw2DTex_VS(), DrawTexCubicFast_PS(true , false));
TECHNIQUE(DrawTexCubicFast1   ,      Draw_VS(), DrawTexCubicFast_PS(false, false));
TECHNIQUE(DrawTexCubicFastD   ,      Draw_VS(), DrawTexCubicFast_PS(false, true ));
TECHNIQUE(DrawTexCubicFastRGB ,      Draw_VS(), DrawTexCubicFastRGB_PS());
TECHNIQUE(DrawTexCubicFastRGBD,      Draw_VS(), DrawTexCubicFastRGB_PS(true));
/******************************************************************************/
VecH4 DrawTexCubic_PS(NOPERSP Vec2 inTex:TEXCOORD,
                      NOPERSP PIXEL              ,
                      uniform Bool color         ,
                      uniform Bool dither        ):TARGET
{
   VecH4 col=TexCubic(inTex);
   if(color)col=col*Color[0]+Color[1];
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
VecH4 DrawTexCubicRGB_PS(NOPERSP Vec2 inTex:TEXCOORD,
                         NOPERSP PIXEL              ,
                         uniform Bool dither=false  ):TARGET
{
   VecH4 col=VecH4(TexCubicRGB(inTex), 1);
   if(dither)ApplyDither(col.rgb, pixel.xy);
   return col;
}
TECHNIQUE(DrawTexCubic    , Draw2DTex_VS(), DrawTexCubic_PS(false, false));
TECHNIQUE(DrawTexCubicC   , Draw2DTex_VS(), DrawTexCubic_PS(true , false));
TECHNIQUE(DrawTexCubic1   ,      Draw_VS(), DrawTexCubic_PS(false, false));
TECHNIQUE(DrawTexCubicD   ,      Draw_VS(), DrawTexCubic_PS(false, true ));
TECHNIQUE(DrawTexCubicRGB ,      Draw_VS(), DrawTexCubicRGB_PS());
TECHNIQUE(DrawTexCubicRGBD,      Draw_VS(), DrawTexCubicRGB_PS(true));
/******************************************************************************/
#if !CG
VecH4 DrawMs1_PS(NOPERSP PIXEL):TARGET {return TexSample(ImgMS, pixel.xy, 0);}
VecH4 DrawMsN_PS(NOPERSP PIXEL):TARGET
{
                                   VecH4 color =TexSample(ImgMS, pixel.xy, 0);
   UNROLL for(Int i=1; i<MS_SAMPLES; i++)color+=TexSample(ImgMS, pixel.xy, i);
   return color/MS_SAMPLES;
}
VecH4 DrawMsM_PS(NOPERSP PIXEL,
                    UInt index:SV_SampleIndex):TARGET
{
   return TexSample(ImgMS, pixel.xy, index);
}
TECHNIQUE    (DrawMs1, DrawPixel_VS(), DrawMs1_PS());
TECHNIQUE    (DrawMsN, DrawPixel_VS(), DrawMsN_PS());
TECHNIQUE_4_1(DrawMsM, DrawPixel_VS(), DrawMsM_PS());
#endif
/******************************************************************************/
void DrawMask_VS(VtxInput vtx,
             out Vec2 outTexC:TEXCOORD0,
             out Vec2 outTexM:TEXCOORD1,
             out Vec4 outVtx :POSITION )
{
   outTexC=vtx.tex ();
   outTexM=vtx.tex1();
   outVtx =Vec4(vtx.pos2()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);
}
VecH4 DrawMask_PS(NOPERSP Vec2 inTexC:TEXCOORD0,
                  NOPERSP Vec2 inTexM:TEXCOORD1):TARGET
{
   VecH4  col   =Tex(Img , inTexC)*Color[0]+Color[1];
          col.a*=Tex(Img1, inTexM).a;
   return col;
}
TECHNIQUE(DrawMask, DrawMask_VS(), DrawMask_PS());
/******************************************************************************/
void DrawCubeFace_VS(VtxInput vtx,
                 out Vec  outTex:TEXCOORD,
                 out Vec4 outVtx:POSITION)
{
   outTex=Vec (vtx.tex(), vtx.size());
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);
}
VecH4 DrawCubeFace_PS(NOPERSP Vec inTex:TEXCOORD):TARGET {return TexCube(Cub, inTex)*Color[0]+Color[1];}

TECHNIQUE(DrawCubeFace, DrawCubeFace_VS(), DrawCubeFace_PS());
/******************************************************************************/
// FONT
/******************************************************************************/
#include "!Set HP.h"
BUFFER(Font)
   Half FontShadow,
        FontContrast=1,
        FontShade;
   Flt  FontDepth;
   VecH FontLum;
BUFFER_END
#include "!Set LP.h"

void Font_VS(VtxInput vtx,
         out Vec2 outTex  :TEXCOORD0,
         out Half outShade:TEXCOORD1,
         out Vec4 outVtx  :POSITION ,
     uniform Bool custom_depth)
{
                   outTex  =     vtx.tex ();
                   outShade=     vtx.size();
   if(custom_depth)outVtx  =Vec4(vtx.pos2()*Coords.xy+Coords.zw, DelinearizeDepth(FontDepth), 1);
   else            outVtx  =Vec4(vtx.pos2()*Coords.xy+Coords.zw,               REVERSE_DEPTH, 1);
}
VecH4 Font_PS
(
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Half inShade:TEXCOORD1,
   uniform Bool linear_gamma
):TARGET
{
   // c=color, s=shadow, a=alpha

   // final=dest *(1-s) + 0*s; -> background after applying shadow
   // final=final*(1-a) + c*a; -> background after applying shadow after applying color

   // final=(dest*(1-s) + 0*s)*(1-a) + c*a;

   // final=dest*(1-s)*(1-a) + c*a;

#if !CG
   VecH2 as=ImgXY.Sample(SamplerFont, inTex).rg; // #FontImageLayout
#else
   VecH2 as=Tex(ImgXY, inTex).rg; // #FontImageLayout
#endif
   Half  a =Sat(as.x*FontContrast), // font opacity, "Min(as.x*FontContrast, 1)", scale up by 'FontContrast' to improve quality when font is very small
         s =    as.y*FontShadow   ; // font shadow

   if(linear_gamma)
   {
      //a=  Sqr(  a); // good for bright text
      //a=1-Sqr(1-a); // good for dark   text
      //Half lum=Min(Max(Color[0].rgb)*4.672, 1); // calculate text brightness, multiply by "1/SRGBToLinear(0.5)" will give better results for grey text color, 'FontShade' is ignored for performance reasons
      a=Lerp(1-Sqr(1-a), Sqr(a), FontLum.x); // TODO: could this be done somehow per RGB channel? similar to Sub-Pixel
      s=     1-Sqr(1-s);
   }

   // Half final_alpha=1-(1-s)*(1-a);
   // 1-(1-s)*(1-a)
   // 1-(1-a-s+sa)
   // 1-1+a+s-sa
   // a + s - s*a
   Half final_alpha=a+s-s*a;

#if 1 // use for ALPHA_BLEND (this option is better because we don't need to set blend state specifically for drawing fonts)
   return VecH4(Color[0].rgb*(Lerp(FontShade, 1, Sat(inShade))*a/(final_alpha+HALF_MIN)), Color[0].a*final_alpha); // NaN, division by 'final_alpha' is required because of the hardware ALPHA_BLEND formula, without it we would get dark borders around the font
#else // use for ALPHA_MERGE
   return VecH4(Color[0].rgb*(Lerp(FontShade, 1, Sat(inShade))*a*Color[0].a), Color[0].a*final_alpha);
#endif
}
TECHNIQUE(Font  , Font_VS(false), Font_PS(false));
TECHNIQUE(FontD , Font_VS(true ), Font_PS(false));
TECHNIQUE(FontG , Font_VS(false), Font_PS(true ));
TECHNIQUE(FontDG, Font_VS(true ), Font_PS(true ));
/******************************************************************************/
// FONT SUB-PIXEL
/******************************************************************************/
void FontSP_VS(VtxInput vtx,
           out Vec2 outTex:TEXCOORD,
           out Vec4 outVtx:POSITION)
{
   outTex=     vtx.tex ();
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, REVERSE_DEPTH, 1);
}
VecH4 FontSP_PS
(
   NOPERSP Vec2 inTex:TEXCOORD,
   uniform Bool linear_gamma
):TARGET
{
   VecH4 c=Tex(Img, inTex);
   if(linear_gamma)
   {
    //c.rgb=  Sqr(  c.rgb); // good for bright text
    //c.rgb=1-Sqr(1-c.rgb); // good for dark   text
      c.rgb=Lerp(1-Sqr(1-c.rgb), Sqr(c.rgb), FontLum); // auto
   }
   return c*Color[0].a;
}
TECHNIQUE(FontSP , FontSP_VS(), FontSP_PS(false));
TECHNIQUE(FontSPG, FontSP_VS(), FontSP_PS(true ));
/******************************************************************************/
void Simple_VS(VtxInput vtx,
           out Vec2  outTex:TEXCOORD,
           out VecH4 outCol:COLOR   ,
           out Vec4  outVtx:POSITION)
{
   outTex=vtx.tex();
   outCol=vtx.colorFast();
   outVtx=Project(TransformPos(vtx.pos()));
}
Vec4 Simple_PS(Vec2  inTex:TEXCOORD,
               VecH4 inCol:COLOR   ):TARGET
{
   return Tex(Img, inTex)*inCol;
}
TECHNIQUE(Simple, Simple_VS(), Simple_PS());
/******************************************************************************/
inline VecH TexYUV(Vec2 inTex,
           uniform Bool gamma)
{
 /*Half y=Tex(ImgX , inTex).x,
        u=Tex(ImgX1, inTex).x,
        v=Tex(ImgX2, inTex).x;

   Half r=1.1643*(y-0.0625)                   + 1.5958*(v-0.5),
        g=1.1643*(y-0.0625) - 0.39173*(u-0.5) - 0.8129*(v-0.5),
        b=1.1643*(y-0.0625) + 2.017  *(u-0.5)                 ;*/

   // keep 'Tex' in case images have LOD's (however unlikely)
   Half y=Tex(ImgX , inTex).x*1.1643-0.07276875,
        u=Tex(ImgX1, inTex).x       -0.5,
        v=Tex(ImgX2, inTex).x       -0.5;

   VecH rgb=VecH(y             + 1.5958*v,
                 y - 0.39173*u - 0.8129*v,
                 y + 2.017  *u          );
   if(gamma)rgb=SRGBToLinear(rgb);
   return   rgb;
}
VecH4 YUV_PS (NOPERSP Vec2 inTex:TEXCOORD, uniform Bool gamma):TARGET {return VecH4(TexYUV(inTex, gamma),                                     1);}
VecH4 YUVA_PS(NOPERSP Vec2 inTex:TEXCOORD, uniform Bool gamma):TARGET {return VecH4(TexYUV(inTex, gamma), Tex(ImgX3, inTex).x*1.1643-0.07276875);} // need to MulAdd because alpha image assumes to come from another YUV video

TECHNIQUE(YUV  , Draw2DTex_VS(), YUV_PS (false));
TECHNIQUE(YUVG , Draw2DTex_VS(), YUV_PS (true ));
TECHNIQUE(YUVA , Draw2DTex_VS(), YUVA_PS(false));
TECHNIQUE(YUVAG, Draw2DTex_VS(), YUVA_PS(true ));
/******************************************************************************/
// BLUR
/******************************************************************************/
#define WEIGHT4_0 0.250000000
#define WEIGHT4_1 0.213388354
#define WEIGHT4_2 0.124999993
#define WEIGHT4_3 0.036611654
// WEIGHT4_0 + WEIGHT4_1*2 + WEIGHT4_2*2 + WEIGHT4_3*2 = 1

#define WEIGHT5_0 0.200000014
#define WEIGHT5_1 0.180901707
#define WEIGHT5_2 0.130901704
#define WEIGHT5_3 0.069098287
#define WEIGHT5_4 0.019098295
// WEIGHT5_0 + WEIGHT5_1*2 + WEIGHT5_2*2 + WEIGHT5_3*2 + WEIGHT5_4*2 = 1

#define WEIGHT6_0 0.166666668
#define WEIGHT6_1 0.155502122
#define WEIGHT6_2 0.125000001
#define WEIGHT6_3 0.083333329
#define WEIGHT6_4 0.041666662
#define WEIGHT6_5 0.011164551
// WEIGHT6_0 + WEIGHT6_1*2 + WEIGHT6_2*2 + WEIGHT6_3*2 + WEIGHT6_4*2 + WEIGHT6_5*2 = 1

// !!
// !! If changing number of samples then change also SHADER_BLUR_RANGE !!
// !!

#define TEST_BLUR 0
#if     TEST_BLUR
   Flt Test, Samples, Mode;
   Flt Weight(Int i, Int s)
   {
      return Mode ? Gaussian(Mode*i/Flt(s+1)) : BlendSmoothSin(i/Flt(s+1));
   }
#endif

// can use 'RTSize' instead of 'ImgSize' since there's no scale

VecH4 BlurX_PS(NOPERSP Vec2 inTex:TEXCOORD,
               uniform Int  samples       ):TARGET
{
#if TEST_BLUR
   if(Test){Int s=Round(Samples); Vec4 color=0; Flt weight=0; for(Int i=-s; i<=s; i++){Flt w=Weight(Abs(i), s); weight+=w; color.rgb+=w*TexPoint(Img, inTex+RTSize.xy*Vec2(i, 0)).rgb;} color.rgb/=weight; return color;}
#endif
   // use linear filtering because texcoords aren't rounded
   if(samples==4) // -3 .. 3
      return VecH4(TexLod(Img, inTex+RTSize.xy*Vec2( 0+WEIGHT4_1/(WEIGHT4_0/2+WEIGHT4_1), 0)).rgb*(WEIGHT4_0/2+WEIGHT4_1) // 0th sample is divided by 2 because it's obtained here and line below to preserve symmetry
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-0-WEIGHT4_1/(WEIGHT4_0/2+WEIGHT4_1), 0)).rgb*(WEIGHT4_0/2+WEIGHT4_1)
                  +TexLod(Img, inTex+RTSize.xy*Vec2( 2+WEIGHT4_3/(WEIGHT4_2  +WEIGHT4_3), 0)).rgb*(WEIGHT4_2  +WEIGHT4_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-2-WEIGHT4_3/(WEIGHT4_2  +WEIGHT4_3), 0)).rgb*(WEIGHT4_2  +WEIGHT4_3), 0);
   if(samples==5) // -4 .. 4
      return VecH4(TexLod(Img, inTex                                                      ).rgb* WEIGHT5_0
                  +TexLod(Img, inTex+RTSize.xy*Vec2( 1+WEIGHT5_2/(WEIGHT5_1+WEIGHT5_2), 0)).rgb*(WEIGHT5_1+WEIGHT5_2)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-1-WEIGHT5_2/(WEIGHT5_1+WEIGHT5_2), 0)).rgb*(WEIGHT5_1+WEIGHT5_2)
                  +TexLod(Img, inTex+RTSize.xy*Vec2( 3+WEIGHT5_4/(WEIGHT5_3+WEIGHT5_4), 0)).rgb*(WEIGHT5_3+WEIGHT5_4)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-3-WEIGHT5_4/(WEIGHT5_3+WEIGHT5_4), 0)).rgb*(WEIGHT5_3+WEIGHT5_4), 0);
   if(samples==6) // -5 .. 5
      return VecH4(TexLod(Img, inTex+RTSize.xy*Vec2( 0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).rgb*(WEIGHT6_0/2+WEIGHT6_1) // 0th sample is divided by 2 because it's obtained here and line below to preserve symmetry
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).rgb*(WEIGHT6_0/2+WEIGHT6_1)
                  +TexLod(Img, inTex+RTSize.xy*Vec2( 2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).rgb*(WEIGHT6_2  +WEIGHT6_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).rgb*(WEIGHT6_2  +WEIGHT6_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2( 4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).rgb*(WEIGHT6_4  +WEIGHT6_5)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(-4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).rgb*(WEIGHT6_4  +WEIGHT6_5), 0);
}
VecH4 BlurY_PS(NOPERSP Vec2 inTex:TEXCOORD,
               uniform Int  samples       ):TARGET
{
#if TEST_BLUR
   if(Test){Int s=Round(Samples); Vec4 color=0; Flt weight=0; for(Int i=-s; i<=s; i++){Flt w=Weight(Abs(i), s); weight+=w; color.rgb+=w*TexPoint(Img, inTex+RTSize.xy*Vec2(0, i)).rgb;} color.rgb/=weight; return color;}
#endif
   // use linear filtering because texcoords aren't rounded
   if(samples==4) // -3 .. 3
      return VecH4(TexLod(Img, inTex+RTSize.xy*Vec2(0,  0+WEIGHT4_1/(WEIGHT4_0/2+WEIGHT4_1))).rgb*(WEIGHT4_0/2+WEIGHT4_1) // 0th sample is divided by 2 because it's obtained here and line below to preserve symmetry
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -0-WEIGHT4_1/(WEIGHT4_0/2+WEIGHT4_1))).rgb*(WEIGHT4_0/2+WEIGHT4_1)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0,  2+WEIGHT4_3/(WEIGHT4_2  +WEIGHT4_3))).rgb*(WEIGHT4_2  +WEIGHT4_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -2-WEIGHT4_3/(WEIGHT4_2  +WEIGHT4_3))).rgb*(WEIGHT4_2  +WEIGHT4_3), 0);
   if(samples==5) // -4 .. 4
      return VecH4(TexLod(Img, inTex                                                      ).rgb* WEIGHT5_0
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0,  1+WEIGHT5_2/(WEIGHT5_1+WEIGHT5_2))).rgb*(WEIGHT5_1+WEIGHT5_2)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -1-WEIGHT5_2/(WEIGHT5_1+WEIGHT5_2))).rgb*(WEIGHT5_1+WEIGHT5_2)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0,  3+WEIGHT5_4/(WEIGHT5_3+WEIGHT5_4))).rgb*(WEIGHT5_3+WEIGHT5_4)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -3-WEIGHT5_4/(WEIGHT5_3+WEIGHT5_4))).rgb*(WEIGHT5_3+WEIGHT5_4), 0);
   if(samples==6) // -5 .. 5
      return VecH4(TexLod(Img, inTex+RTSize.xy*Vec2(0,  0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).rgb*(WEIGHT6_0/2+WEIGHT6_1) // 0th sample is divided by 2 because it's obtained here and line below to preserve symmetry
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).rgb*(WEIGHT6_0/2+WEIGHT6_1)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0,  2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).rgb*(WEIGHT6_2  +WEIGHT6_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).rgb*(WEIGHT6_2  +WEIGHT6_3)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0,  4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).rgb*(WEIGHT6_4  +WEIGHT6_5)
                  +TexLod(Img, inTex+RTSize.xy*Vec2(0, -4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).rgb*(WEIGHT6_4  +WEIGHT6_5), 0);
}

TECHNIQUE(BlurX , Draw_VS(), BlurX_PS(4));
TECHNIQUE(BlurXH, Draw_VS(), BlurX_PS(6));
TECHNIQUE(BlurY , Draw_VS(), BlurY_PS(4));
TECHNIQUE(BlurYH, Draw_VS(), BlurY_PS(6));

Half BlurX_X_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   // use linear filtering because texcoords aren't rounded
   return TexLod(ImgX, inTex+RTSize.xy*Vec2( 0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).x*(WEIGHT6_0/2+WEIGHT6_1)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(-0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).x*(WEIGHT6_0/2+WEIGHT6_1)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2( 2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).x*(WEIGHT6_2  +WEIGHT6_3)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(-2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).x*(WEIGHT6_2  +WEIGHT6_3)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2( 4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).x*(WEIGHT6_4  +WEIGHT6_5)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(-4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).x*(WEIGHT6_4  +WEIGHT6_5);
}
Half BlurY_X_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   // use linear filtering because texcoords aren't rounded
   return TexLod(ImgX, inTex+RTSize.xy*Vec2(0,  0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).x*(WEIGHT6_0/2+WEIGHT6_1)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(0, -0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).x*(WEIGHT6_0/2+WEIGHT6_1)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(0,  2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).x*(WEIGHT6_2  +WEIGHT6_3)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(0, -2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).x*(WEIGHT6_2  +WEIGHT6_3)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(0,  4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).x*(WEIGHT6_4  +WEIGHT6_5)
         +TexLod(ImgX, inTex+RTSize.xy*Vec2(0, -4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).x*(WEIGHT6_4  +WEIGHT6_5);
}

TECHNIQUE(BlurX_X, Draw_VS(), BlurX_X_PS());
TECHNIQUE(BlurY_X, Draw_VS(), BlurY_X_PS());
/******************************************************************************/
// MAX
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
VecH4 MaxX_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH color; // color=0; for(Int i=0; i<11; i++)color=Max(color, TexPoint(Img, inTex+RTSize.xy*Vec2(i-5, 0)).rgb*(BlendWeight[i]/BlendWeight[5])); original slower version
   // use linear filtering because texcoords aren't rounded
   color=           TexLod(Img, inTex+RTSize.xy*Vec2( 0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).rgb ;
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(-0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1), 0)).rgb);
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2( 2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).rgb*((WEIGHT6_2+WEIGHT6_3)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(-2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3), 0)).rgb*((WEIGHT6_2+WEIGHT6_3)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2( 4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).rgb*((WEIGHT6_4+WEIGHT6_5)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(-4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5), 0)).rgb*((WEIGHT6_4+WEIGHT6_5)/(WEIGHT6_0+WEIGHT6_1)));
   return VecH4(color, 0);
}
VecH4 MaxY_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH color; // color=0; for(Int i=0; i<11; i++)color=Max(color, TexPoint(Img, inTex+RTSize.xy*Vec2(0, i-5)).rgb*(BlendWeight[i]/BlendWeight[5])); original slower version
   // use linear filtering because texcoords aren't rounded
   color=           TexLod(Img, inTex+RTSize.xy*Vec2(0,  0+WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).rgb ;
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(0, -0-WEIGHT6_1/(WEIGHT6_0/2+WEIGHT6_1))).rgb);
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(0,  2+WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).rgb*((WEIGHT6_2+WEIGHT6_3)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(0, -2-WEIGHT6_3/(WEIGHT6_2  +WEIGHT6_3))).rgb*((WEIGHT6_2+WEIGHT6_3)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(0,  4+WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).rgb*((WEIGHT6_4+WEIGHT6_5)/(WEIGHT6_0+WEIGHT6_1)));
   color=Max(color, TexLod(Img, inTex+RTSize.xy*Vec2(0, -4-WEIGHT6_5/(WEIGHT6_4  +WEIGHT6_5))).rgb*((WEIGHT6_4+WEIGHT6_5)/(WEIGHT6_0+WEIGHT6_1)));
   return VecH4(color, 0);
}
TECHNIQUE(MaxX, Draw_VS(), MaxX_PS());
TECHNIQUE(MaxY, Draw_VS(), MaxY_PS());
/******************************************************************************/
VecH4 Outline_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 uniform Bool clip_do       ,
                 uniform Bool down_sample=false):TARGET
{
   if(down_sample)inTex=(Floor(inTex*ImgSize.zw)+0.5)*ImgSize.xy; // we're rendering to a smaller RT, so inTex is located in the center between multiple texels, we need to align it so it's located at the center of the nearest one

   VecH4 col=TexLod(Img, inTex); // use linear filtering because 'Img' can be of different size
   Vec2  t0=inTex+ImgSize.xy*(down_sample ? 2.5 : 0.5), // 2.5 scale was the min value that worked OK with 2.0 density
         t1=inTex-ImgSize.xy*(down_sample ? 2.5 : 0.5);
   // use linear filtering because texcoords aren't rounded
#if 0
   if(Dist2(col.rgb, TexLod(Img, Vec2(t0.x, t0.y)).rgb)
     +Dist2(col.rgb, TexLod(Img, Vec2(t0.x, t1.y)).rgb)
     +Dist2(col.rgb, TexLod(Img, Vec2(t1.x, t0.y)).rgb)
     +Dist2(col.rgb, TexLod(Img, Vec2(t1.x, t1.y)).rgb)<=EPS_COL)col.a=0; // if all neighbors are the same then make this pixel transparent
#else // faster approximate
   if(Length2(col.rgb*4-TexLod(Img, Vec2(t0.x, t0.y)).rgb
                       -TexLod(Img, Vec2(t0.x, t1.y)).rgb
                       -TexLod(Img, Vec2(t1.x, t0.y)).rgb
                       -TexLod(Img, Vec2(t1.x, t1.y)).rgb)<=EPS_COL)col.a=0; // if all neighbors are the same then make this pixel transparent
#endif
   /* old code used for super sampling
	{
      Flt pos=TexDepthPoint(inTex);
      if(Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2(-1,  0)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2(-1,  0))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 1,  0)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 1,  0))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 0, -1)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 0, -1))>=pos)
        +Dist2(col, TexLod(Img, inTex+ImgSize.xy*Vec2( 0,  1)))*(TexDepthPoint(inTex+ImgSize.xy*Vec2( 0,  1))>=pos)<=EPS_COL)col.a=0;
	}*/
   if(clip_do)clip(col.a-EPS);
   return col;
}
VecH4 OutlineApply_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 color=TexLod(Img, inTex); // use linear filtering because 'Img' can be of different size
   for(Int i=0; i<6; i++)
   {
      Vec2  t=inTex+BlendOfs6[i]*ImgSize.xy;
      VecH4 c=TexLod(Img, t); // use linear filtering because texcoords aren't rounded
      if(c.a>color.a)color=c;
   }
   return color;
}
TECHNIQUE(Outline     , Draw_VS(),      Outline_PS(false));
TECHNIQUE(OutlineDS   , Draw_VS(),      Outline_PS(false, true));
TECHNIQUE(OutlineClip , Draw_VS(),      Outline_PS(true ));
TECHNIQUE(OutlineApply, Draw_VS(), OutlineApply_PS());
/******************************************************************************/
VecH4 EdgeDetect_PS(NOPERSP Vec2 inTex  :TEXCOORD ,
                    NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET // use VecH4 because we may want to apply this directly onto RGBA destination
{
   Flt z =TexDepthPoint(inTex),
       zl=TexDepthPoint(inTex+ImgSize.xy*Vec2(-1, 0)),
       zr=TexDepthPoint(inTex+ImgSize.xy*Vec2( 1, 0)),
       zd=TexDepthPoint(inTex+ImgSize.xy*Vec2( 0,-1)),
       zu=TexDepthPoint(inTex+ImgSize.xy*Vec2( 0, 1)), soft=0.1+z/50;

   Vec pos=GetPos(Min(z, Min(zl, zr, zd, zu)), inPosXY);

   Half px   =Abs(zr-((z-zl)+z))/soft-0.5, //cx=Sat(Max(Abs(zl-z), Abs(zr-z))/soft-0.5), cx, cy doesn't work well in lower screen resolutions and with flat terrain
        py   =Abs(zu-((z-zd)+z))/soft-0.5, //cy=Sat(Max(Abs(zu-z), Abs(zd-z))/soft-0.5),
        alpha=Sat(Length(pos)*SkyFracMulAdd.x + SkyFracMulAdd.y),
        edge =Sat(px+py);

   return 1-edge*alpha;
}
VecH4 EdgeDetectApply_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET // use VecH4 because we apply this directly onto RGBA destination
{
   const Int  samples=4;
         Half color  =TexPoint(ImgX, inTex).x;
   for(Int i=0; i<samples; i++)
   {
      Vec2 t=inTex+BlendOfs4[i]*ImgSize.xy;
      color+=TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
   }
   return color/(samples+1); // Sqr could be used on the result, to make darkening much stronger
}
TECHNIQUE(EdgeDetect     , DrawPosXY_VS(),      EdgeDetect_PS());
TECHNIQUE(EdgeDetectApply, Draw_VS     (), EdgeDetectApply_PS());
/******************************************************************************/
VecH4 Dither_PS(NOPERSP Vec2 inTex:TEXCOORD,
                NOPERSP PIXEL):TARGET
{
   VecH4 col=VecH4(TexLod(Img, inTex).rgb, 1); // force full alpha so back buffer effects can work ok, can't use 'TexPoint' because 'Img' can be of different size
   ApplyDither(col.rgb, pixel.xy);
   return col;
}
TECHNIQUE(Dither, Draw_VS(), Dither_PS());
/******************************************************************************/
Half CombineSSAlpha_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return DEPTH_FOREGROUND(TexDepthRawPoint(inTex));
}
VecH4 Combine_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 NOPERSP PIXEL,
                 uniform Int  sample        ):TARGET
{
   VecH4 col=TexLod(Img, inTex); // use linear filtering because 'Img' can be of different size
#if !CG
   if(sample==2) // multi sample
   {
      col.w =0; UNROLL for(Int i=0; i<MS_SAMPLES; i++)col.w+=DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, i));
      col.w/=MS_SAMPLES;
      // here col.rgb is not premultiplied by alpha (it is at full scale), which means that it will not work as smooth when doing the blended support below

      // if any of the neighbor pixels are transparent then assume that there's no blended graphics in the area, and then return just the solid pixel to keep AA
      // use linear filtering because 'Img' can be of different size
      if(Max(TexLod(Img, inTex+ImgSize.xy*Vec2( 0, SQRT2)).rgb)<=0.15
      || Max(TexLod(Img, inTex+ImgSize.xy*Vec2(-1,    -1)).rgb)<=0.15
      || Max(TexLod(Img, inTex+ImgSize.xy*Vec2( 1,    -1)).rgb)<=0.15)return col; // there can be bloom around solid pixels, so allow some tolerance
   }else
#endif
   if(sample==1)col.w=TexLod(Img1, inTex).x; // super sample, use linear filtering because 'Img' can be of different size
   else         col.w=DEPTH_FOREGROUND(TexDepthRawPoint(inTex)); // single sample

   // support blended graphics (pixels with colors but without depth)
      col.w=Max(col); // treat luminance as opacity
   if(col.w>0)col.rgb/=col.w;
   return col;
}
TECHNIQUE(CombineSSAlpha, Draw_VS(), CombineSSAlpha_PS( ));
TECHNIQUE(Combine       , Draw_VS(),        Combine_PS(0));
TECHNIQUE(CombineSS     , Draw_VS(),        Combine_PS(1));
#if !CG
TECHNIQUE(CombineMS     , Draw_VS(),        Combine_PS(2));
#endif
/******************************************************************************/
#if !CG
void ResolveDepth_PS(NOPERSP PIXEL,
                         out Flt depth:DEPTH)
{
   // return the smallest of all samples
                                         depth=                 TexSample(DepthMS, pixel.xy, 0).x;
   UNROLL for(Int i=1; i<MS_SAMPLES; i++)depth=DEPTH_MIN(depth, TexSample(DepthMS, pixel.xy, i).x); // have to use minimum of depth samples to avoid shadow artifacts, by picking the samples that are closer to the camera, similar effect to what we do with view space bias (if Max is used, then shadow acne can occur for local lights)
}
TECHNIQUE(ResolveDepth, DrawPixel_VS(), ResolveDepth_PS());

// 'Depth'   can't be used because it's            1-sample
// 'DepthMs' can't be used because it's always multi-sampled (all samples are different)
void DetectMSCol_PS(NOPERSP PIXEL)
{
   VecH cols[4]={TexSample(ImgMS, pixel.xy, 0).rgb, TexSample(ImgMS, pixel.xy, 1).rgb, TexSample(ImgMS, pixel.xy, 2).rgb, TexSample(ImgMS, pixel.xy, 3).rgb}; // load 4-multi-samples of texel
#if 0 // generates too many MS pixels
   if(all((cols[0]==cols[1]) && (cols[0]==cols[2]) && (cols[0]==cols[3])))discard;
#else
/* this Eps was calculated using formula below, it's meant to disable MS for neighbor byte colors (such as 0,1) but enable for others (0,2), so calculate at borderline of 1.5:
   Vec c0=0.0f/255, c1=1.5f/255, cols[]={c0, c0, c1, c1};
   Flt eps=Dist2(cols[0], cols[1])
          +Dist2(cols[0], cols[2])
          +Dist2(cols[0], cols[3]); */
   if(Dist2(cols[0], cols[1])
     +Dist2(cols[0], cols[2])
     +Dist2(cols[0], cols[3])<=0.000207612466)discard;
#endif
}
TECHNIQUE(DetectMSCol, DrawPixel_VS(), DetectMSCol_PS());
/*void DetectMSNrm_PS(NOPERSP PIXEL)
{
   Vec2 nrms[4]={TexSample(ImgMS, pixel.xy, 0).xy, TexSample(ImgMS, pixel.xy, 1).xy, TexSample(ImgMS, pixel.xy, 2).xy, TexSample(ImgMS, pixel.xy, 3).xy}; // load 4-multi-samples of texel
#if 0 // generates too many MS pixels
   if(all((nrms[0]==nrms[1]) && (nrms[0]==nrms[2]) && (nrms[0]==nrms[3])))discard;
#else
   if(Dist2(nrms[0], nrms[1])
     +Dist2(nrms[0], nrms[2])
     +Dist2(nrms[0], nrms[3])<=Half(some eps))discard;
#endif
}
TECHNIQUE(DetectMSNrm, DrawPixel_VS(), DetectMSNrm_PS());*/
#endif

void SetDepth_PS(NOPERSP Vec2 inTex:TEXCOORD,
                     out Flt  depth:DEPTH   )
{
   depth=TexLod(Depth, inTex).x; // use linear filtering because this can be used for different size RT
}
TECHNIQUE(SetDepth, Draw_VS(), SetDepth_PS());

/*void RebuildDepth_PS(NOPERSP Vec2 inTex:TEXCOORD,
                         out Flt  depth:DEPTH   ,
                     uniform Bool perspective   )
{
   depth=DelinearizeDepth(TexLod(Depth, inTex).x, perspective); // use linear filtering because this can be used for different size RT
}
TECHNIQUE(RebuildDepth , Draw_VS(), RebuildDepth_PS(false));
TECHNIQUE(RebuildDepthP, Draw_VS(), RebuildDepth_PS(true ));*/

Flt LinearizeDepth_PS(NOPERSP Vec2 inTex:TEXCOORD,
                      uniform Bool perspective   ):TARGET
{
   return LinearizeDepth(TexLod(Depth, inTex).x, perspective); // use linear filtering because this can be used for different size RT
}
TECHNIQUE(LinearizeDepth0 , Draw_VS(), LinearizeDepth_PS(false));
TECHNIQUE(LinearizeDepthP0, Draw_VS(), LinearizeDepth_PS(true ));
#if !CG
Flt LinearizeDepth1_PS(NOPERSP PIXEL,
                       uniform Bool perspective):TARGET
{
   return LinearizeDepth(TexSample(DepthMS, pixel.xy, 0).x, perspective);
}
Flt LinearizeDepth2_PS(NOPERSP PIXEL,
                               UInt index:SV_SampleIndex,
                       uniform Bool perspective         ):TARGET
{
   return LinearizeDepth(TexSample(DepthMS, pixel.xy, index).x, perspective);
}
TECHNIQUE    (LinearizeDepth1 , DrawPixel_VS(), LinearizeDepth1_PS(false));
TECHNIQUE    (LinearizeDepthP1, DrawPixel_VS(), LinearizeDepth1_PS(true ));
TECHNIQUE_4_1(LinearizeDepth2 , DrawPixel_VS(), LinearizeDepth2_PS(false));
TECHNIQUE_4_1(LinearizeDepthP2, DrawPixel_VS(), LinearizeDepth2_PS(true ));
#endif

Vec4 DrawDepth_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Flt frac=TexDepthPoint(inTex)/Viewport.range; // can't filter depth, because if Depth image is smaller, then we will get borders around objects
   Vec rgb=HsbToRgb(Vec(frac*2.57, 1, 1)); // the scale is set so the full range equals to blue color, to imitate sky color
   if(LINEAR_GAMMA)rgb=SRGBToLinear(rgb);
   return Vec4(rgb, 1);
}
TECHNIQUE(DrawDepth, Draw_VS(), DrawDepth_PS());
/******************************************************************************/
// SKY
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
#if !CG
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
#endif
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
// FOG
/******************************************************************************/
VecH4 Fog_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
             NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET
{
   Vec  pos=GetPosPoint(inTex, inPosXY);
   Half dns=AccumulatedDensity(FogDensity, Length(pos));

   return VecH4(FogColor, dns);
}
#if !CG
VecH4 FogN_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
              NOPERSP Vec2 inPosXY:TEXCOORD1,
              NOPERSP PIXEL                 ):TARGET
{
   Half valid=HALF_MIN, dns=0;
   UNROLL for(Int i=0; i<MS_SAMPLES; i++)
   {
      Flt depth=TexDepthMSRaw(pixel.xy, i); if(DEPTH_FOREGROUND(depth))
      {
         Vec pos =GetPos(LinearizeDepth(depth), inPosXY);
             dns+=AccumulatedDensity(FogDensity, Length(pos));
         valid++;
      }
   }
   return VecH4(FogColor, dns/valid);
}
VecH4 FogM_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
              NOPERSP Vec2 inPosXY:TEXCOORD1,
              NOPERSP PIXEL                 ,
                      UInt index  :SV_SampleIndex):TARGET
{
   Vec pos=GetPosMS(pixel.xy, index, inPosXY);
   return VecH4(FogColor, AccumulatedDensity(FogDensity, Length(pos)));
}
#endif
TECHNIQUE    (Fog , DrawPosXY_VS(), Fog_PS ());
#if !CG // multi sample
TECHNIQUE    (FogN, DrawPosXY_VS(), FogN_PS());
TECHNIQUE_4_1(FogM, DrawPosXY_VS(), FogM_PS());
#endif
/******************************************************************************/
// SUN
/******************************************************************************/
#include "!Set HP.h"
struct SunClass
{
   Vec2 pos2;
   VecH pos, color;
};

BUFFER(Sun)
   SunClass Sun;
BUFFER_END
#include "!Set LP.h"

Half SunRaysMask_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                    NOPERSP Vec2 inPosXY:TEXCOORD1,
                    uniform Bool mask             ):TARGET
{
   if(mask) // for this version we have to use linear depth filtering, because RT is of different size, and otherwise too much artifacts/flickering is generated
   {
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      Half m=(Length2(GetPosLinear(inTex, inPosXY))>=Sqr(Viewport.range));
   #else // need safer
      Flt  z=TexDepthRawLinear(inTex);
      Half m=(DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), inPosXY))>=Sqr(Viewport.range));
   #endif
      return m*TexLod(ImgX, inTex).x; // use linear filtering because 'ImgX' can be of different size
   }else // can use point filtering here
   {
   #if REVERSE_DEPTH // we can use the simple version for REVERSE_DEPTH
      return Length2(GetPosPoint(inTex, inPosXY))>=Sqr(Viewport.range);
   #else // need safer
      Flt z=TexDepthRawPoint(inTex);
      return DEPTH_BACKGROUND(z) || Length2(GetPos(LinearizeDepth(z), inPosXY))>=Sqr(Viewport.range);
   #endif
   }
}
TECHNIQUE(SunRaysMask , DrawPosXY_VS(), SunRaysMask_PS(false));
TECHNIQUE(SunRaysMask1, DrawPosXY_VS(), SunRaysMask_PS(true ));
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
VecH4 SunRays_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL                 ,
                 uniform Bool high             ,
                 uniform Bool dither           ,
                 uniform Bool jitter           ,
                 uniform Bool gamma            ):TARGET
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
    //Flt frac=Max(Max(Vec2(0,0), Abs(sun_pos-0.5)-0.5)/Abs(sun_pos-inTex)); // Max(Max(0, Abs(sun_pos.x-0.5)-0.5)/Abs(sun_pos.x-inTex.x), Max(0, Abs(sun_pos.y-0.5)-0.5)/Abs(sun_pos.y-inTex.y));
      Flt frac=Max(Max(Vec2(0,0), Abs(sun_pos-Viewport.center)-Viewport.size/2)/Abs(sun_pos-inTex)); // Max(Max(0, Abs(sun_pos.x-0.5)-0.5)/Abs(sun_pos.x-inTex.x), Max(0, Abs(sun_pos.y-0.5)-0.5)/Abs(sun_pos.y-inTex.y));
      sun_pos-=(  frac)*(sun_pos-inTex);
    //power  *=(1-frac);
   #endif

      if(jitter)inTex+=(sun_pos-inTex)*(DitherValue(pixel.xy)*(3.0/steps)); // a good value is 2.5 or 3.0 (3.0 was slightly better)

      UNROLL for(Int i=0; i<steps; i++)
      {
         Vec2 t=Lerp(inTex, sun_pos, i/Flt(steps)); // /(steps) worked better than /(steps-1)
         if(high)light+=TexLod(ImgX, t).x; // pos and clouds combined together, use linear filtering because texcoords aren't rounded
         else    light+=DEPTH_BACKGROUND(TexDepthRawPoint(t)); // use simpler version here unlike in 'SunRaysPre_PS' because this one is called for each step for each pixel
      }
      col.rgb=Half(light*power/steps)*Sun.color;
      if(dither)ApplyDither(col.rgb, pixel.xy, false); // here we're always in sRGB gamma
      if(gamma )col.rgb=SRGBToLinearFast(col.rgb); // 'SRGBToLinearFast' works better here than 'SRGBToLinear' (gives high contrast, dark colors remain darker, while 'SRGBToLinear' highlights them more)
   }
   return col;
}
TECHNIQUE(SunRays    , DrawPosXY_VS(), SunRays_PS(false, false, false, false));
TECHNIQUE(SunRaysH   , DrawPosXY_VS(), SunRays_PS(true , false, false, false));
TECHNIQUE(SunRaysD   , DrawPosXY_VS(), SunRays_PS(false, true , false, false));
TECHNIQUE(SunRaysHD  , DrawPosXY_VS(), SunRays_PS(true , true , false, false));
TECHNIQUE(SunRaysJ   , DrawPosXY_VS(), SunRays_PS(false, false, true , false));
TECHNIQUE(SunRaysHJ  , DrawPosXY_VS(), SunRays_PS(true , false, true , false));
TECHNIQUE(SunRaysDJ  , DrawPosXY_VS(), SunRays_PS(false, true , true , false));
TECHNIQUE(SunRaysHDJ , DrawPosXY_VS(), SunRays_PS(true , true , true , false));
TECHNIQUE(SunRaysG   , DrawPosXY_VS(), SunRays_PS(false, false, false, true ));
TECHNIQUE(SunRaysHG  , DrawPosXY_VS(), SunRays_PS(true , false, false, true ));
TECHNIQUE(SunRaysDG  , DrawPosXY_VS(), SunRays_PS(false, true , false, true ));
TECHNIQUE(SunRaysHDG , DrawPosXY_VS(), SunRays_PS(true , true , false, true ));
TECHNIQUE(SunRaysJG  , DrawPosXY_VS(), SunRays_PS(false, false, true , true ));
TECHNIQUE(SunRaysHJG , DrawPosXY_VS(), SunRays_PS(true , false, true , true ));
TECHNIQUE(SunRaysDJG , DrawPosXY_VS(), SunRays_PS(false, true , true , true ));
TECHNIQUE(SunRaysHDJG, DrawPosXY_VS(), SunRays_PS(true , true , true , true ));
/******************************************************************************/
// SHADOW MAP
/******************************************************************************/
Half ShdDir_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1,
               NOPERSP PIXEL                 ,
               uniform Int  num              ,
               uniform Bool cloud            ):TARGET
{
   return ShadowDirValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true, num, cloud);
}
#if !CG
Half ShdDirM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                NOPERSP PIXEL                      ,
                        UInt index  :SV_SampleIndex,
                uniform Int  num                   ,
                uniform Bool cloud                 ):TARGET
{
   return ShadowDirValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true, num, cloud);
}
#endif
TECHNIQUE(ShdDir1, DrawPosXY_VS(), ShdDir_PS(1, false));   TECHNIQUE(ShdDir1C, DrawPosXY_VS(), ShdDir_PS(1, true));
TECHNIQUE(ShdDir2, DrawPosXY_VS(), ShdDir_PS(2, false));   TECHNIQUE(ShdDir2C, DrawPosXY_VS(), ShdDir_PS(2, true));
TECHNIQUE(ShdDir3, DrawPosXY_VS(), ShdDir_PS(3, false));   TECHNIQUE(ShdDir3C, DrawPosXY_VS(), ShdDir_PS(3, true));
TECHNIQUE(ShdDir4, DrawPosXY_VS(), ShdDir_PS(4, false));   TECHNIQUE(ShdDir4C, DrawPosXY_VS(), ShdDir_PS(4, true));
TECHNIQUE(ShdDir5, DrawPosXY_VS(), ShdDir_PS(5, false));   TECHNIQUE(ShdDir5C, DrawPosXY_VS(), ShdDir_PS(5, true));
TECHNIQUE(ShdDir6, DrawPosXY_VS(), ShdDir_PS(6, false));   TECHNIQUE(ShdDir6C, DrawPosXY_VS(), ShdDir_PS(6, true));

TECHNIQUE_4_1(ShdDir1M, DrawPosXY_VS(), ShdDirM_PS(1, false));   TECHNIQUE_4_1(ShdDir1CM, DrawPosXY_VS(), ShdDirM_PS(1, true));
TECHNIQUE_4_1(ShdDir2M, DrawPosXY_VS(), ShdDirM_PS(2, false));   TECHNIQUE_4_1(ShdDir2CM, DrawPosXY_VS(), ShdDirM_PS(2, true));
TECHNIQUE_4_1(ShdDir3M, DrawPosXY_VS(), ShdDirM_PS(3, false));   TECHNIQUE_4_1(ShdDir3CM, DrawPosXY_VS(), ShdDirM_PS(3, true));
TECHNIQUE_4_1(ShdDir4M, DrawPosXY_VS(), ShdDirM_PS(4, false));   TECHNIQUE_4_1(ShdDir4CM, DrawPosXY_VS(), ShdDirM_PS(4, true));
TECHNIQUE_4_1(ShdDir5M, DrawPosXY_VS(), ShdDirM_PS(5, false));   TECHNIQUE_4_1(ShdDir5CM, DrawPosXY_VS(), ShdDirM_PS(5, true));
TECHNIQUE_4_1(ShdDir6M, DrawPosXY_VS(), ShdDirM_PS(6, false));   TECHNIQUE_4_1(ShdDir6CM, DrawPosXY_VS(), ShdDirM_PS(6, true));
/******************************************************************************/
Half ShdPoint_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL):TARGET
{
   return ShadowPointValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
}
#if !CG
Half ShdPointM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                  NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                  NOPERSP PIXEL,
                          UInt index  :SV_SampleIndex):TARGET
{
   return ShadowPointValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
}
#endif
TECHNIQUE    (ShdPoint , DrawPosXY_VS(), ShdPoint_PS ());
TECHNIQUE_4_1(ShdPointM, DrawPosXY_VS(), ShdPointM_PS());
/******************************************************************************/
Half ShdCone_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                NOPERSP Vec2 inPosXY:TEXCOORD1,
                NOPERSP PIXEL):TARGET
{
   return ShadowConeValue(GetPosPoint(inTex, inPosXY), ShadowJitter(pixel.xy), true);
}
#if !CG
Half ShdConeM_PS(NOPERSP Vec2 inTex  :TEXCOORD0     ,
                 NOPERSP Vec2 inPosXY:TEXCOORD1     ,
                 NOPERSP PIXEL,
                         UInt index  :SV_SampleIndex):TARGET
{
   return ShadowConeValue(GetPosMS(pixel.xy, index, inPosXY), ShadowJitter(pixel.xy), true);
}
#endif
TECHNIQUE    (ShdCone , DrawPosXY_VS(), ShdCone_PS());
TECHNIQUE_4_1(ShdConeM, DrawPosXY_VS(), ShdConeM_PS());
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
Half ShdBlur_PS(NOPERSP Vec2 inTex:TEXCOORD,
                uniform Int  samples       ):TARGET
{
   Half weight=0.25,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z);
   UNROLL for(Int i=0; i<samples; i++)
   {
      Vec2 t;
      if(samples== 4)t=RTSize.xy*BlendOfs4 [i]+inTex;
    //if(samples== 5)t=RTSize.xy*BlendOfs5 [i]+inTex;
      if(samples== 6)t=RTSize.xy*BlendOfs6 [i]+inTex;
      if(samples== 8)t=RTSize.xy*BlendOfs8 [i]+inTex;
    //if(samples== 9)t=RTSize.xy*BlendOfs9 [i]+inTex;
      if(samples==12)t=RTSize.xy*BlendOfs12[i]+inTex;
    //if(samples==13)t=RTSize.xy*BlendOfs13[i]+inTex;
      // use linear filtering because texcoords are not rounded
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurX_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 uniform Int  range         ):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.y=inTex.y;
   UNROLL for(Int i=-range; i<=range; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.x=RTSize.x*(2*i+((i>0) ? -0.5 : 0.5))+inTex.x;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
Half ShdBlurY_PS(NOPERSP Vec2 inTex:TEXCOORD,
                 uniform Int  range         ):TARGET
{
   Half weight=0.5,
        color =TexPoint(ImgX, inTex).x*weight;
   Flt  z     =TexDepthPoint(inTex);
   Vec2 dw_mad=DepthWeightMAD(z), t; t.x=inTex.x;
   UNROLL for(Int i=-range; i<=range; i++)if(i)
   {
      // use linear filtering because texcoords are not rounded
      t.y=RTSize.y*(2*i+((i>0) ? -0.5 : 0.5))+inTex.y;
      Half w=DepthWeight(z-TexDepthLinear(t), dw_mad);
      color +=w*TexLod(ImgX, t).x; // use linear filtering because texcoords aren't rounded
      weight+=w;
   }
   return color/weight;
}
  TECHNIQUE(ShdBlur4 , Draw_VS(), ShdBlur_PS(4));
//TECHNIQUE(ShdBlur5 , Draw_VS(), ShdBlur_PS(5));
  TECHNIQUE(ShdBlur6 , Draw_VS(), ShdBlur_PS(6));
  TECHNIQUE(ShdBlur8 , Draw_VS(), ShdBlur_PS(8));
//TECHNIQUE(ShdBlur9 , Draw_VS(), ShdBlur_PS(9));
  TECHNIQUE(ShdBlur12, Draw_VS(), ShdBlur_PS(12));
//TECHNIQUE(ShdBlur13, Draw_VS(), ShdBlur_PS(13));
//TECHNIQUE(ShdBlurX1, Draw_VS(), ShdBlurX_PS(1));
//TECHNIQUE(ShdBlurY1, Draw_VS(), ShdBlurY_PS(1));
  TECHNIQUE(ShdBlurX2, Draw_VS(), ShdBlurX_PS(2));
  TECHNIQUE(ShdBlurY2, Draw_VS(), ShdBlurY_PS(2));
/******************************************************************************/
VecH4 PaletteDraw_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 particle=TexLod(Img, inTex); // use linear filtering in case in the future we support downsized palette intensities (for faster fill-rate)
   clip(Length2(particle)-Sqr(Half(EPS_COL))); // 'clip' is faster than "BRANCH if(Length2(particle)>Sqr(EPS_COL))" (branch is however slightly faster when entire majority of pixels have some effect, however in most cases majority of pixels doesn't have anything so stick with 'clip')

   // have to use linear filtering because this is palette image
   VecH4 c0=TexLod(Img1, VecH2(particle.x, 0.5/4)),
         c1=TexLod(Img1, VecH2(particle.y, 1.5/4)),
         c2=TexLod(Img1, VecH2(particle.z, 2.5/4)),
         c3=TexLod(Img1, VecH2(particle.w, 3.5/4));
   Half  a =Max(c0.a, c1.a, c2.a, c3.a);

   return VecH4((c0.rgb*c0.a
                +c1.rgb*c1.a
                +c2.rgb*c2.a
                +c3.rgb*c3.a)/(a+HALF_MIN), a); // NaN
}
TECHNIQUE(PaletteDraw, Draw_VS(), PaletteDraw_PS());
/******************************************************************************/
#if GL // #WebSRGB
VecH4 WebLToS_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return LinearToSRGB(TexLod(Img, inTex));}
TECHNIQUE(WebLToS, Draw_VS(), WebLToS_PS());
#endif
/******************************************************************************/
