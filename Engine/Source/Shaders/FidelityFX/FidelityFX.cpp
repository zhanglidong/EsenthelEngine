/******************************************************************************/
#include "../!Header.h"
#define Quart _Quart
/******************************************************************************/
#define A_GPU 1
#define A_HLSL 1
#define A_HALF 1
#include "ffx_a.h"
/******************************************************************************/
VecH4 GetColor(VecH4 c)
{
#if GAMMA
   c.rgb=LinearToSRGBFast(c.rgb);
#endif
   return c;
}

VecH4 FsrEasuRH(Vec2 p) {return GetColor(TexGatherR(Img, p));}
VecH4 FsrEasuGH(Vec2 p) {return GetColor(TexGatherG(Img, p));}
VecH4 FsrEasuBH(Vec2 p) {return GetColor(TexGatherB(Img, p));}

VecH4 FsrRcasLoadH(ASW2 p) {return GetColor(Img[p]);}
Vec4  FsrRcasLoadF(ASU2 p) {return GetColor(Img[p]);}

void FsrRcasInputH(inout AH1 r, inout AH1 g, inout AH1 b) {}
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
/******************************************************************************/
#define FSR_EASU_H 1
#define FSR_RCAS_H 1
#define FSR_RCAS_F 1
#if ALPHA
   #define FSR_RCAS_PASSTHROUGH_ALPHA 1
#endif
#include "ffx_fsr1.h"
/******************************************************************************/
struct EASU
{
   AU4 c0, c1, c2, c3;
};
BUFFER(EASU)
   EASU Easu;
BUFFER_END

VecH4 EASU_PS(NOPERSP PIXEL):TARGET
{
   VecI2 pos=pixel.xy;
   VecH4 c; FsrEasuH(c.rgb, pos, Easu.c0, Easu.c1, Easu.c2, Easu.c3);
#if ALPHA
   Vec2 uv=(Vec2(pos) * AF2_AU2(Easu.c0.xy) + AF2_AU2(Easu.c0.zw)) * AF2_AU2(Easu.c1.xy) + Vec2(0.5, -0.5) * AF2_AU2(Easu.c1.zw);
   c.a=Tex(Img, uv).a;
#else
   c.a=1;
#endif
#if DITHER
   ApplyDither(c.rgb, pixel.xy, false); // here 'c' is already in gamma space
#endif
#if GAMMA
   c.rgb=SRGBToLinearFast(c.rgb);
#endif
   return c;
}
/******************************************************************************/
struct RCAS
{
   AU4 c0;
};
BUFFER(RCAS)
   RCAS Rcas;
BUFFER_END

VecH4 RCAS_PS(NOPERSP PIXEL):TARGET
{
   VecH4 c;
   // TODO: FIXME: should use 'FsrRcasH' however DX shader compiler is crashing
#if ALPHA
   FsrRcasF(c.r, c.g, c.b, c.a, pixel.xy, Rcas.c0);
#else
   FsrRcasF(c.r, c.g, c.b, pixel.xy, Rcas.c0); c.a=1;
#endif
#if DITHER
   ApplyDither(c.rgb, pixel.xy, false); // here 'c' is already in gamma space
#endif
#if GAMMA
   c.rgb=SRGBToLinearFast(c.rgb);
#endif
   return c;
}
/******************************************************************************
void Filter(int2 pos)
{
#if UPSCALE
   if(0) // linear filtering
   {
      Vec2 pp = (Vec2(pos) * AF2_AU2(Easu.c0.xy) + AF2_AU2(Easu.c0.zw)) * AF2_AU2(Easu.c1.xy) + Vec2(0.5, -0.5) * AF2_AU2(Easu.c1.zw);
      RWImg[pos]=TexLod(Img, pp);
   }else
   {
      VecH4 c; FsrEasuH(c.rgb, pos, Easu.c0, Easu.c1, Easu.c2, Easu.c3); c.a=1; RWImg[pos]=c;
   }
#else
   VecH4 c; FsrRcasH(c.r, c.g, c.b, pos, Rcas.c0); c.a=1; RWImg[pos]=c;
#endif
}

[numthreads(64, 1, 1)]
void CS(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
   AU2 gxy = ARmp8x8(LocalThreadId.x) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
   Filter(gxy); gxy.x += 8u;
   Filter(gxy); gxy.y += 8u;
   Filter(gxy); gxy.x -= 8u;
   Filter(gxy);
}
/******************************************************************************/
