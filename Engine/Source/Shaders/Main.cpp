/******************************************************************************/
#include "!Header.h"
#include "Ambient Occlusion.h"
#include "Sky.h"
#include "Layered Clouds.h"
#include "Hdr.h"
#include "Fog.h"
#include "Fur.h"
#include "Overlay.h"
#include "Light Apply.h"
/******************************************************************************/
#ifdef MODE
VecH4 Test_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
#if MODE // half
   VecH4 t=inTex.xyyx;
   VecH4 r=VecH4(0.1, 0.2, 0.3, 0.4);
   LOOP for(Int i=0; i<256; i++)
   {
      r+=t*r;
   }
   return VecH4(r.xy+r.yx, MODE, 1);
#else
   Vec4 t=inTex.xyyx;
   Vec4 r=Vec4(0.1, 0.2, 0.3, 0.4);
   LOOP for(Int i=0; i<256; i++)
   {
      r+=t*r;
   }
   return VecH4(r.xy+r.yx, MODE, 1);
#endif
}
#endif
/******************************************************************************/
// SHADERS
/******************************************************************************/
NOPERSP Vec4 Draw2DFlat_VS(VtxInput vtx):POSITION {return Vec4(vtx.pos2()*Coords.xy+Coords.zw, Z_FRONT, 1);}
        Vec4 Draw3DFlat_VS(VtxInput vtx):POSITION {return Project(TransformPos(vtx.pos()));}

VecH4 DrawFlat_PS():TARGET {return Color[0];}
/******************************************************************************/
void Draw2DCol_VS(VtxInput vtx,
      NOPERSP out VecH4 outCol:COLOR   ,
      NOPERSP out Vec4  outVtx:POSITION)
{
   outCol=     vtx.color();
   outVtx=Vec4(vtx.pos2 ()*Coords.xy+Coords.zw, Z_FRONT, 1);
}
VecH4 Draw2DCol_PS(NOPERSP VecH4 inCol:COLOR):TARGET {return inCol;}
/******************************************************************************/
void Draw3DCol_VS(VtxInput vtx,
              out VecH4 outCol:COLOR   ,
              out Vec4  outVtx:POSITION)
{
   outCol=vtx.color();
   outVtx=Project(TransformPos(vtx.pos()));
}
VecH4 Draw3DCol_PS(VecH4 inCol:COLOR):TARGET {return inCol;}
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

VecH4 DrawTexXIG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(1-Tex(Img, inTex).x).xxx, 1);}
VecH4 DrawTexYIG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(1-Tex(Img, inTex).y).xxx, 1);}
VecH4 DrawTexZIG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(1-Tex(Img, inTex).z).xxx, 1);}
VecH4 DrawTexWIG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(1-Tex(Img, inTex).w).xxx, 1);}

VecH4 DrawTexXSG_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).x *0.5+0.5).xxx, 1);}
VecH4 DrawTexXYSG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(Img, inTex).xy*0.5+0.5),  0, 1);}
VecH4 DrawTexSG_PS  (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return       SRGBToLinear(Tex(Img, inTex)   *0.5+0.5);}

VecH4 DrawTexNrm_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH nrm; nrm.xy=Tex(Img, inTex).BASE_CHANNEL_NORMAL; // #MaterialTextureLayout
             nrm.z =CalcZ(nrm.xy);
             nrm   =Normalize(nrm)*0.5+0.5;
          #if LINEAR_GAMMA
             nrm   =SRGBToLinear(nrm);
          #endif
   return VecH4(nrm, 1);
}
VecH4 DrawTexDetNrm_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH nrm; nrm.xy=Tex(Img, inTex).DETAIL_CHANNEL_NORMAL*2-1; // #MaterialTextureLayoutDetail
             nrm.z =CalcZ(nrm.xy);
             nrm   =Normalize(nrm)*0.5+0.5;
          #if LINEAR_GAMMA
             nrm   =SRGBToLinear(nrm);
          #endif
   return VecH4(nrm, 1);
}

VecH4 DrawX_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(             Tex(ImgX, inTex)   .xxx, 1);}
VecH4 DrawXG_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return VecH4(SRGBToLinear(Tex(ImgX, inTex).x).xxx, 1);}

#if defined DITHER && defined GAMMA
VecH4 DrawXC_PS(NOPERSP Vec2 inTex:TEXCOORD,
                NOPERSP PIXEL              ):TARGET
{
   VecH4 col=Tex(ImgX, inTex).x*Color[0]+Color[1];
#if DITHER
   ApplyDither(col.rgb, pixel.xy, LINEAR_GAMMA && !GAMMA); // don't perform gamma conversions inside dither, because this means we have sRGB color which we're going to convert to linear below
#endif
#if GAMMA
   col.rgb=SRGBToLinearFast(col.rgb); // this is used for drawing sun rays, 'SRGBToLinearFast' works better here than 'SRGBToLinear' (gives high contrast, dark colors remain darker, while 'SRGBToLinear' highlights them more)
#endif
   return col;
}
#endif

VecH4 DrawTexPoint_PS (NOPERSP Vec2 inTex:TEXCOORD):TARGET {return TexPoint(Img, inTex);}
VecH4 DrawTexPointC_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return TexPoint(Img, inTex)*Color[0]+Color[1];}
/******************************************************************************/
void Draw2DTexCol_VS(VtxInput vtx,
         NOPERSP out Vec2  outTex:TEXCOORD,
         NOPERSP out VecH4 outCol:COLOR   ,
         NOPERSP out Vec4  outVtx:POSITION)
{
   outTex=     vtx.tex  ();
   outCol=     vtx.color();
   outVtx=Vec4(vtx.pos2 ()*Coords.xy+Coords.zw, Z_FRONT, 1);
}
VecH4 Draw2DTexCol_PS(NOPERSP Vec2  inTex:TEXCOORD,
                      NOPERSP VecH4 inCol:COLOR   ):TARGET
{
   return Tex(Img, inTex)*inCol;
}
/******************************************************************************/
void Draw2DDepthTex_VS(VtxInput vtx,
           NOPERSP out Vec2  outTex:TEXCOORD,
                #if COLORS
           NOPERSP out VecH4 outCol:COLOR   ,
                #endif
           NOPERSP out Vec4  outVtx:POSITION)
{
   outTex=vtx.tex();
#if COLORS
   outCol=vtx.color();
#endif
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, DelinearizeDepth(vtx.posZ()), 1);
}
VecH4 Draw2DDepthTex_PS(NOPERSP Vec2  inTex:TEXCOORD
                   #if COLORS
                      , NOPERSP VecH4 inCol:COLOR
                   #endif
                       ):TARGET
{
   VecH4 col=Tex(Img, inTex);
#if ALPHA_TEST
   clip(col.a-0.5);
#endif
#if COLORS
   col*=inCol;
#endif
   return col;
}
/******************************************************************************/
void Draw3DTex_VS(VtxInput vtx,
              out Vec2  outTex:TEXCOORD,
           #if COLORS
              out VecH4 outCol:COLOR   ,
           #endif
           #if FOG
              out VecH4 outFog:COLOR1  ,
           #endif
              out Vec4  outVtx:POSITION)
{
   Vec pos=TransformPos(vtx.pos());
   outTex=vtx.tex();
#if COLORS
   outCol=vtx.color();
#endif
#if FOG
   outFog=VecH4(FogColor, AccumulatedDensity(FogDensity, Length(pos)));
#endif
   outVtx=Project(pos);
}
VecH4 Draw3DTex_PS(Vec2  inTex:TEXCOORD
                #if COLORS
                 , VecH4 inCol:COLOR
                #endif
                #if FOG
                 , VecH4 inFog:COLOR1  
                #endif
                  ):TARGET
{
   VecH4 col=Tex(Img, inTex);
#if ALPHA_TEST
   clip(col.a-0.5);
#endif
#if COLORS
   col*=inCol;
#endif
#if FOG
   col.rgb=Lerp(col.rgb, inFog.rgb, inFog.a);
#endif
   return col;
}
/******************************************************************************/
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
/******************************************************************************/
void DrawMask_VS(VtxInput vtx,
     NOPERSP out Vec2 outTexC:TEXCOORD0,
     NOPERSP out Vec2 outTexM:TEXCOORD1,
     NOPERSP out Vec4 outVtx :POSITION )
{
   outTexC=vtx.tex ();
   outTexM=vtx.tex1();
   outVtx =Vec4(vtx.pos2()*Coords.xy+Coords.zw, Z_FRONT, 1);
}
VecH4 DrawMask_PS(NOPERSP Vec2 inTexC:TEXCOORD0, NOPERSP Vec2 inTexM:TEXCOORD1):TARGET
{
#if POINT
   VecH4 c=TexPoint(Img, inTexC);
#else
   VecH4 c=Tex(Img, inTexC);
#endif

#if ALPHA
   c.a*=Tex(Img1, inTexM).a;
#else
   c*=Tex(Img1, inTexM);
#endif

   return c*Color[0]+Color[1];
}
/******************************************************************************/
void DrawCubeFace_VS(VtxInput vtx,
         NOPERSP out Vec  outTex:TEXCOORD,
         NOPERSP out Vec4 outVtx:POSITION)
{
   outTex=Vec (vtx.tex(), vtx.size());
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, Z_FRONT, 1);
}
VecH4 DrawCubeFace_PS(NOPERSP Vec inTex:TEXCOORD):TARGET {return TexCube(Cub, inTex)*Color[0]+Color[1];}
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
/******************************************************************************/
VecH4 Dither_PS(NOPERSP Vec2 inTex:TEXCOORD,
                NOPERSP PIXEL):TARGET
{
   VecH4 col=VecH4(TexLod(Img, inTex).rgb, 1); // force full alpha so back buffer effects can work ok, can't use 'TexPoint' because 'Img' can be of different size
   ApplyDither(col.rgb, pixel.xy);
   return col;
}
/******************************************************************************/
// EDGE DETECT
/******************************************************************************/
VecH4 EdgeDetect_PS(NOPERSP Vec2 inTex  :TEXCOORD ,
                    NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET // use VecH4 because we may want to apply this directly onto RGBA destination
{
   Flt z =TexDepthPoint   (inTex),
       zl=TexDepthPointOfs(inTex, VecI2(-1, 0)),
       zr=TexDepthPointOfs(inTex, VecI2( 1, 0)),
       zd=TexDepthPointOfs(inTex, VecI2( 0,-1)),
       zu=TexDepthPointOfs(inTex, VecI2( 0, 1)), soft=0.1+z/50;

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
/******************************************************************************/
// COMBINE
/******************************************************************************/
Half SetAlphaFromDepth_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return DEPTH_FOREGROUND(TexDepthRawPoint(inTex));
}
#if 1 // this is needed
Half SetAlphaFromDepthMS_PS(NOPERSP Vec2 inTex:TEXCOORD, NOPERSP PIXEL, UInt index:SV_SampleIndex):TARGET
{
   return DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, index));
}
#else
Half SetAlphaFromDepthMS_PS(NOPERSP Vec2 inTex:TEXCOORD, NOPERSP PIXEL):TARGET
{
   Half   alpha=0; UNROLL for(Int i=0; i<MS_SAMPLES; i++)alpha+=DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, i));
   return alpha/MS_SAMPLES;
}
#endif

Half SetAlphaFromDepthAndCol_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return Max(Max(TexLod(Img, inTex).rgb), DEPTH_FOREGROUND(TexDepthRawPoint(inTex))); // treat luminance as opacity
}
Half SetAlphaFromDepthAndColMS_PS(NOPERSP Vec2 inTex:TEXCOORD, NOPERSP PIXEL, UInt index:SV_SampleIndex):TARGET
{
   return Max(Max(TexLod(Img, inTex).rgb), DEPTH_FOREGROUND(TexDepthMSRaw(pixel.xy, index))); // treat luminance as opacity
}

VecH4 CombineAlpha_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 col;
   col.rgb=TexLod(Img , inTex).rgb; // use linear filtering because 'Img'  can be of different size
   col.w  =TexLod(ImgX, inTex).x  ; // use linear filtering because 'ImgX' can be of different size
   return col;
}
VecH4 ReplaceAlpha_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return VecH4(0, 0, 0, TexLod(ImgX, inTex).x); // use linear filtering because 'ImgX' can be of different size
}
/******************************************************************************/
// MULTI SAMPLE
/******************************************************************************/
// !! Warning: if MS looks like it has artifacts/jitter it's because multi-sampled shadows are not blurred for performance reasons        !!
// !! Warning: if MS looks like it has outlines         it's because multi-sample detection tests only colors, ignoring normals and depth !!
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
/******************************************************************************/
// DEPTH
/******************************************************************************/
void ResolveDepth_PS(NOPERSP PIXEL,
                         out Flt depth:DEPTH)
{
   // return the smallest of all samples
                                         depth=                 TexSample(DepthMS, pixel.xy, 0).x;
   UNROLL for(Int i=1; i<MS_SAMPLES; i++)depth=DEPTH_MIN(depth, TexSample(DepthMS, pixel.xy, i).x); // have to use minimum of depth samples to avoid shadow artifacts, by picking the samples that are closer to the camera, similar effect to what we do with view space bias (if Max is used, then shadow acne can occur for local lights)
}

void SetDepth_PS(NOPERSP Vec2 inTex:TEXCOORD,
                     out Flt  depth:DEPTH   )
{
   depth=TexLod(Depth, inTex).x; // use linear filtering because this can be used for different size RT
}

VecH4 DrawDepth_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Flt frac=TexDepthPoint(inTex)/Viewport.range; // can't filter depth, because if Depth image is smaller, then we will get borders around objects
   VecH rgb=HsbToRgb(Vec(frac*2.57, 1, 1)); // the scale is set so the full range equals to blue color, to imitate sky color
   if(LINEAR_GAMMA)rgb=SRGBToLinear(rgb);
   return VecH4(rgb, 1);
}

#ifdef PERSPECTIVE
Flt LinearizeDepth0_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return LinearizeDepth(TexLod(Depth, inTex).x, PERSPECTIVE); // use linear filtering because this can be used for different size RT
}
Flt LinearizeDepth1_PS(NOPERSP PIXEL):TARGET
{
   return LinearizeDepth(TexSample(DepthMS, pixel.xy, 0).x, PERSPECTIVE);
}
Flt LinearizeDepth2_PS(NOPERSP PIXEL,
                               UInt index:SV_SampleIndex):TARGET
{
   return LinearizeDepth(TexSample(DepthMS, pixel.xy, index).x, PERSPECTIVE);
}
/*void RebuildDepth_PS(NOPERSP Vec2 inTex:TEXCOORD,
                         out Flt  depth:DEPTH   )
{
   depth=DelinearizeDepth(TexLod(Depth, inTex).x, PERSPECTIVE); // use linear filtering because this can be used for different size RT
}*/
#endif
/******************************************************************************/
// PALETTE
/******************************************************************************/
VecH4 PaletteDraw_PS(NOPERSP Vec2  inTex  :TEXCOORD,
                         out Half outAlpha:TARGET2 // #RTOutput.Blend
                    ):TARGET
{
   VecH4 particle=TexLod(Img, inTex); // use linear filtering in case in the future we support downsized palette intensities (for faster fill-rate)
   clip(Length2(particle)-Sqr(Half(EPS_COL))); // 'clip' is faster than "BRANCH if(Length2(particle)>Sqr(EPS_COL))" (branch is however slightly faster when entire majority of pixels have some effect, however in most cases majority of pixels doesn't have anything so stick with 'clip')

   // have to use linear filtering because this is palette image
   VecH4 c0=TexLod(Img1, VecH2(particle.x, 0.5/4)),
         c1=TexLod(Img1, VecH2(particle.y, 1.5/4)),
         c2=TexLod(Img1, VecH2(particle.z, 2.5/4)),
         c3=TexLod(Img1, VecH2(particle.w, 3.5/4));
   Half  a =Max(c0.a, c1.a, c2.a, c3.a);

   outAlpha=a;

   return VecH4((c0.rgb*c0.a
                +c1.rgb*c1.a
                +c2.rgb*c2.a
                +c3.rgb*c3.a)/(a+HALF_MIN), a); // NaN
}
/******************************************************************************/
void ClearDeferred_VS(VtxInput vtx,
          NOPERSP out Vec2  outTex                :TEXCOORD,
          NOPERSP out Vec   projected_prev_pos_xyw:PREV_POS,
          NOPERSP out Vec4  outVtx                :POSITION)
{
   outTex=vtx.tex();

   Vec view_pos=Vec(UVToPosXY(vtx.tex()), 1); // no need to normalize
   Vec prev_pos=Transform3(view_pos, ViewToViewPrev); // view_pos/ViewMatrix*ViewMatrixPrev, use 'Transform3' to rotate only (angular velocity) and skip movement (linear velocity)
   projected_prev_pos_xyw=ProjectPrevXYW(prev_pos);

   outVtx=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
}
void ClearDeferred_PS(NOPERSP Vec2 inTex                 :TEXCOORD,
                      NOPERSP Vec  projected_prev_pos_xyw:PREV_POS,
           out DeferredSolidOutput output) // #RTOutput
{
   output.color      (0);
   output.glow       (0);
   output.normal     (VecH(0, 0, -1)); // set -1 because of AO #NRM_CLEAR
   output.translucent(0);
   output.rough      (1);
   output.reflect    (0);
   output.velocityUV (projected_prev_pos_xyw, inTex);
}
/******************************************************************************/
void ClearLight_PS(out VecH lum :TARGET0,
                   out VecH spec:TARGET1)
{
   lum =Color[0].rgb;
   spec=0;
}
/******************************************************************************/
// COLOR LUT - HDR, DITHER, IN_GAMMA, OUT_GAMMA
/******************************************************************************/
VecH4 ColorLUT_PS(NOPERSP Vec2 inTex:TEXCOORD,
                  NOPERSP PIXEL              ):TARGET
{
   VecH4 col=TexLod(Img, inTex);

   // now 'col' is Linear

#if HDR // LUT supports only 0..1 ranges, so calculate how much color we have on top of that range, perform this in linear space before gamma conversion to avoid loss of precision
   VecH add=col.rgb-Sat(col.rgb); // this is linear
#endif

#if IN_GAMMA
   col.rgb=LinearToSRGB(col.rgb); // make sure we have sRGB
#endif

   // now 'col' is sRGB

   col.rgb=Tex3DLod(Vol, col.rgb*ImgSize.x+ImgSize.y).rgb;

#if DITHER
   ApplyDither(col.rgb, pixel.xy, false); // don't perform gamma conversions inside dither, because at this stage, color is in sRGB
#endif

#if OUT_GAMMA
   col.rgb=SRGBToLinear(col.rgb);
#endif

   // now 'col' is Linear

#if HDR
   col.rgb+=add; // add leftovers
#endif

   return col;
}
/******************************************************************************/
// DUMMY - used only to obtain info about ConstantBuffers/ShaderParams
/******************************************************************************/
Flt Params0_PS():TARGET {return Highlight.x+FurVel[0].x+FurStep.x+Material.color.a+MultiMaterial0.color.a+MultiMaterial1.color.a+MultiMaterial2.color.a+MultiMaterial3.color.a+TexLod(FurCol, 0).x+TexLod(FurLight, 0).x;}
Flt Params1_PS():TARGET {return CamMatrix[0].x+AmbientContrast+HdrBrightness+LocalFogColor.x+OverlayOpaqueFrac()+BehindBias+Step+TransformPosPrev(0);}
Flt Params2_PS():TARGET {return NightShadeColor.x;}
/******************************************************************************/
#if GL // #WebSRGB
VecH4 WebLToS_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET {return LinearToSRGB(TexLod(Img, inTex));}
#endif
/******************************************************************************/
