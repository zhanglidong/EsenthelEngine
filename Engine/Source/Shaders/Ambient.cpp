/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#define PARAMS             \
   uniform Bool skin      ,\
   uniform Int  alpha_test,\
   uniform Bool light_map
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec2 outTex:TEXCOORD,
   out Vec4 outVtx:POSITION,

   IF_IS_CLIP

   PARAMS
)
{
   if(alpha_test || light_map)outTex=vtx.tex();

   Vec      pos;
   if(!skin)pos=TransformPos(vtx.pos());
   else     pos=TransformPos(vtx.pos(), vtx.bone(), vtx.weight());
       CLIP(pos); outVtx=Project(pos);
}
/******************************************************************************/
VecH4 PS
(
   Vec2 inTex:TEXCOORD,

   PARAMS
):TARGET
{
   if(alpha_test==1)clip(Tex(Col, inTex).a + MaterialAlpha()-1);else
   if(alpha_test==2)clip(Tex(Nrm, inTex).a + MaterialAlpha()-1); // #MaterialTextureChannelOrder

   return VecH4(light_map ? Tex(Lum, inTex).rgb*MaterialAmbient() : MaterialAmbient(), 0);
}
/******************************************************************************/
CUSTOM_TECHNIQUE
/******************************************************************************/
