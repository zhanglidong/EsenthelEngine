/******************************************************************************/
#include "!Header.h"
#include "Layered Clouds.h"
/******************************************************************************/
void LayeredClouds_VS
(
   VtxInput vtx,

   out Vec  outPos:TEXCOORD0,
   out Vec4 outTex:TEXCOORD1, // xyz=3d tex coord, w=alpha
   out Vec4 outVtx:POSITION 
)
{
   Vec pos=vtx.pos();

   outTex.xyz=pos.xyz*Vec(LCScale, 1, LCScale);
   outTex.w  =CloudAlpha(outTex.y);
      pos.y  =pos.y*LCScaleY+(-LCScaleY+1); // (pos.y-1)*LCScaleY+1
   outVtx=Project(outPos=TransformPos(pos));
}
/******************************************************************************/
VecH4 LayeredClouds_PS(Vec   inPos :TEXCOORD0,
                       Vec4  inTex :TEXCOORD1,
                       PIXEL                 ,
                   out VecH4 outMask:COLOR1  ,
               uniform Int   num             ,
               uniform Bool  blend           ):TARGET
{
   Half a=Sat(inTex.w);
   if(blend)
   {
      Flt range=TexDepthPoint(PixelToScreen(pixel))/Normalize(inPos).z; // 0..Viewport.range
          a   *=Sat(range*LCRange.x+LCRange.y);
   }
   Vec2  uv=Normalize(inTex.xyz).xz;
   VecH4 color;
   if(num>=4){VecH4 tex=Tex(Img3, uv*CL[3].scale + CL[3].position)*CL[3].color; if(num==4)color=tex;else color=Lerp(color, tex, tex.a);}
   if(num>=3){VecH4 tex=Tex(Img2, uv*CL[2].scale + CL[2].position)*CL[2].color; if(num==3)color=tex;else color=Lerp(color, tex, tex.a);}
   if(num>=2){VecH4 tex=Tex(Img1, uv*CL[1].scale + CL[1].position)*CL[1].color; if(num==2)color=tex;else color=Lerp(color, tex, tex.a);}
   if(num>=1){VecH4 tex=Tex(Img , uv*CL[0].scale + CL[0].position)*CL[0].color; if(num==1)color=tex;else color=Lerp(color, tex, tex.a);}

   color.a*=a;
   outMask.rgb=0;
   outMask.a  =color.a;
   return color;
}
/******************************************************************************/
// TECHNIQUES
/******************************************************************************/
TECHNIQUE(Clouds1 , LayeredClouds_VS(), LayeredClouds_PS(1, false));
TECHNIQUE(Clouds2 , LayeredClouds_VS(), LayeredClouds_PS(2, false));
TECHNIQUE(Clouds3 , LayeredClouds_VS(), LayeredClouds_PS(3, false));
TECHNIQUE(Clouds4 , LayeredClouds_VS(), LayeredClouds_PS(4, false));
TECHNIQUE(Clouds1B, LayeredClouds_VS(), LayeredClouds_PS(1, true ));
TECHNIQUE(Clouds2B, LayeredClouds_VS(), LayeredClouds_PS(2, true ));
TECHNIQUE(Clouds3B, LayeredClouds_VS(), LayeredClouds_PS(3, true ));
TECHNIQUE(Clouds4B, LayeredClouds_VS(), LayeredClouds_PS(4, true ));
/******************************************************************************/
