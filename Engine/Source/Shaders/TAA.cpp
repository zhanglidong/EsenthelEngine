/******************************************************************************/
#include "!Header.h"

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif
/******************************************************************************/
// Img=Old, Img1=New, Img2=Vel, ImgX=Alpha
BUFFER(TAA)
   Vec2 TAAOffset;
BUFFER_END
/******************************************************************************/
VecH4 TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
{
#if CUBIC
   VecH4 cur=Max(VecH4(0,0,0,0), TexCubicFast(Img1, inTex+TAAOffset)); // use Max(0) because of cubic sharpening potentially giving negative values
#else
   VecH4 cur=TexLod(Img1, inTex+TAAOffset);
#endif

}
/******************************************************************************/
