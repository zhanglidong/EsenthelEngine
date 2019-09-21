/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
/******************************************************************************
SKIN, COLORS, LAYOUT, REFLECT
/******************************************************************************/
void VS
(
   VtxInput vtx,

#if LAYOUT
   out Vec2  outTex  :TEXCOORD  ,
#endif
#if REFLECT
   out VecH  outRfl  :REFLECTION,
#endif
   out VecH4 outColor:COLOR     ,
   out Vec4  outVtx  :POSITION
)
{
#if LAYOUT
   outTex=vtx.tex();
#endif
             outColor =Material.color;
   if(COLORS)outColor*=vtx.colorFast();

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
      VecU      bone=vtx.bone();
                 pos=TransformPos(vtx.pos(), bone, vtx.weight());
      if(REFLECT)nrm=TransformDir(vtx.nrm(), bone, vtx.weight());
   }
#if REFLECT
   outRfl=Transform3(reflect((VecH)Normalize(pos), Normalize(nrm)), CamMatrix);
#endif

   outVtx=Project(pos);

   outColor.a*=(Half)Sat(Length(pos)*SkyFracMulAdd.x + SkyFracMulAdd.y);
}
/******************************************************************************/
VecH4 PS
(
#if LAYOUT
   Vec2  inTex  :TEXCOORD  ,
#endif
#if REFLECT
   VecH  inRfl  :REFLECTION,
#endif
   VecH4 inColor:COLOR
):TARGET
{
   VecH4 ext; // #MaterialTextureLayout

#if LAYOUT
   if(LAYOUT==1)                      inColor    *=Tex(Col, inTex);else                    // alpha in 'Col' texture
   if(LAYOUT==2){ext=Tex(Ext, inTex); inColor.rgb*=Tex(Col, inTex).rgb; inColor.a*=ext.a;} // alpha in 'Ext' texture
#endif

   inColor.rgb+=Highlight.rgb;

#if REFLECT // reflection
   inColor.rgb+=TexCube(Rfl, inRfl).rgb*((LAYOUT==2) ? Material.reflect*ext.y : Material.reflect);
#endif

   return inColor;
}
/******************************************************************************/
