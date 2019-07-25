/******************************************************************************/
#include "!Header.h"
#include "Layered Clouds.h"
// NUM, BLEND
/******************************************************************************/
void LayeredClouds_VS
(
   VtxInput vtx,

   out Vec  outPos  :TEXCOORD0,
   out Vec  outTex  :TEXCOORD1, // 3d tex coord
   out Half outAlpha:TEXCOORD2,
   out Vec4 outVtx  :POSITION 
)
{
   Vec pos=vtx.pos();

   outTex  =pos.xyz*Vec(LCScale, 1, LCScale);
   outAlpha=CloudAlpha(outTex.y);
      pos.y=pos.y*LCScaleY+(-LCScaleY+1); // (pos.y-1)*LCScaleY+1
   outVtx  =Project(outPos=TransformPos(pos));
}
/******************************************************************************/
VecH4 LayeredClouds_PS(Vec   inPos  :TEXCOORD0,
                       Vec   inTex  :TEXCOORD1,
                       Half  inAlpha:TEXCOORD2,
                       PIXEL                  ,
                   out VecH4 outMask :TARGET1 ):TARGET
{
   Half a=Sat(inAlpha);
   if(BLEND)
   {
      Flt range=TexDepthPoint(PixelToScreen(pixel))/Normalize(inPos).z; // 0..Viewport.range
          a   *=Sat(range*LCRange.x+LCRange.y);
   }
   Vec2  uv=Normalize(inTex).xz;
   VecH4 color;
   if(NUM>=4){VecH4 tex=Tex(Img3, uv*CL[3].scale + CL[3].position)*CL[3].color; if(NUM==4)color=tex;else color=Lerp(color, tex, tex.a);}
   if(NUM>=3){VecH4 tex=Tex(Img2, uv*CL[2].scale + CL[2].position)*CL[2].color; if(NUM==3)color=tex;else color=Lerp(color, tex, tex.a);}
   if(NUM>=2){VecH4 tex=Tex(Img1, uv*CL[1].scale + CL[1].position)*CL[1].color; if(NUM==2)color=tex;else color=Lerp(color, tex, tex.a);}
   if(NUM>=1){VecH4 tex=Tex(Img , uv*CL[0].scale + CL[0].position)*CL[0].color; if(NUM==1)color=tex;else color=Lerp(color, tex, tex.a);}

   color.a*=a;
   outMask.rgb=0;
   outMask.a  =color.a;
   return color;
}
/******************************************************************************/
