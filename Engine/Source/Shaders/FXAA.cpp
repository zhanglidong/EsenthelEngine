/******************************************************************************/
#include "!Header.h"
/******************************************************************************

   FXAA unlike SMAA is kept outside of Main shader, because it's rarely used.

/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale

#define FxaaTex Image

#if !CG
   #define FxaaInt2 VecI2
   #define FxaaTexTop(t, p      ) t.SampleLevel(SamplerLinearClamp, p, 0)
   #define FxaaTexOff(t, p, o, r) t.SampleLevel(SamplerLinearClamp, p, 0, o)
#else
   #define FxaaInt2 Vec2
   #define FxaaTexTop(t, p      ) TexLod(t, p)
   #define FxaaTexOff(t, p, o, r) TexLod(t, p+o*RTSize.xy)
#endif

#include "FXAA_config.h"
#include "FXAA.h"

VecH4 FXAA_PS(NOPERSP Vec2 pos:TEXCOORD,
              uniform Bool gamma):TARGET
{
   return FxaaPixelShader(pos, 0, Img, Img, Img, RTSize.xy, 0, 0, 0, 0.475, 0.15, 0.0833, 8.0, 0.125, 0.05, Vec4(1.0, -1.0, 0.25, -0.25), gamma);
}
TECHNIQUE(FXAA , Draw_VS(), FXAA_PS(false));
TECHNIQUE(FXAAG, Draw_VS(), FXAA_PS(true ));
/******************************************************************************/
