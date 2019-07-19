/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
VecH4 YUV_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
 /*Half y=TexLod(ImgX , inTex).x,
        u=TexLod(ImgX1, inTex).x,
        v=TexLod(ImgX2, inTex).x;

   Half r=1.1643*(y-0.0625)                   + 1.5958*(v-0.5),
        g=1.1643*(y-0.0625) - 0.39173*(u-0.5) - 0.8129*(v-0.5),
        b=1.1643*(y-0.0625) + 2.017  *(u-0.5)                 ;*/

   Half y=TexLod(ImgX , inTex).x*1.1643-0.07276875,
        u=TexLod(ImgX1, inTex).x       -0.5,
        v=TexLod(ImgX2, inTex).x       -0.5;

   VecH4 col;
   col.r=y             + 1.5958*v;
   col.g=y - 0.39173*u - 0.8129*v;
   col.b=y + 2.017  *u           ;
   col.a=(ALPHA ? TexLod(ImgX3, inTex).x*1.1643-0.07276875 : 1);} // need to MulAdd because alpha image assumes to come from another YUV video

   if(GAMMA)col.rgb=SRGBToLinear(col.rgb);
   return   col;
}
/******************************************************************************/
