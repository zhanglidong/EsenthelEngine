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
