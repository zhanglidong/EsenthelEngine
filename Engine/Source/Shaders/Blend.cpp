/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
/******************************************************************************
SKIN, COLORS, REFLECT, TEXTURES
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec2  outTex  :TEXCOORD0,
   out VecH  outRfl  :TEXCOORD1,
   out VecH4 outColor:COLOR    ,
   out Vec4  outVtx  :POSITION
)
{
   if(TEXTURES)outTex   =vtx.tex();
               outColor =MaterialColor();
   if(COLORS  )outColor*=vtx.colorFast();

   Vec pos; VecH nrm;
   if(!SKIN)
   {
      if(true) // instance
      {
                    pos=TransformPos(vtx.pos(), vtx.instance());
         if(REFLECT)nrm=TransformDir(vtx.nrm(), vtx.instance());
      }else
      {
                    pos=TransformPos(vtx.pos());
         if(REFLECT)nrm=TransformDir(vtx.nrm());
      }
   }else
   {
      VecI      bone=vtx.bone();
                 pos=TransformPos(vtx.pos(), bone, vtx.weight());
      if(REFLECT)nrm=TransformDir(vtx.nrm(), bone, vtx.weight());
   }
   if(REFLECT)outRfl=Transform3(reflect((VecH)Normalize(pos), Normalize(nrm)), CamMatrix);

   outVtx=Project(pos);

   outColor.a*=(Half)Sat(Length(pos)*SkyFracMulAdd.x + SkyFracMulAdd.y);
}
/******************************************************************************/
VecH4 PS
(
   Vec2  inTex  :TEXCOORD0,
   VecH  inRfl  :TEXCOORD1,
   VecH4 inColor:COLOR
):TARGET
{
   VecH4 tex_nrm; // #MaterialTextureChannelOrder

   if(TEXTURES==1) inColor    *=Tex(Col, inTex);else                                                 // alpha in 'Col' texture
   if(TEXTURES==2){inColor.rgb*=Tex(Col, inTex).rgb; tex_nrm=Tex(Nrm, inTex); inColor.a*=tex_nrm.a;} // alpha in 'Nrm' texture

   inColor.rgb+=Highlight.rgb;

   // reflection
   if(REFLECT)inColor.rgb+=TexCube(Rfl, inRfl).rgb*((TEXTURES==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());

   return inColor;
}
/******************************************************************************/
