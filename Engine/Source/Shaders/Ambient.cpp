/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, ALPHA_TEST, LIGHT_MAP
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec2 outTex:TEXCOORD,
   out Vec4 outVtx:POSITION,

   CLIP_DIST
)
{
   if(ALPHA_TEST || LIGHT_MAP)outTex=vtx.tex();

   Vec      pos;
   if(!SKIN)pos=TransformPos(vtx.pos());
   else     pos=TransformPos(vtx.pos(), vtx.bone(), vtx.weight());
 CLIP_PLANE(pos); outVtx=Project(pos);
}
/******************************************************************************/
VecH4 PS
(
   Vec2 inTex:TEXCOORD
):TARGET
{
   if(ALPHA_TEST==1)clip(Tex(Col, inTex).a + MaterialAlpha()-1);else
   if(ALPHA_TEST==2)clip(Tex(Nrm, inTex).a + MaterialAlpha()-1); // #MaterialTextureChannelOrder

   return VecH4(LIGHT_MAP ? Tex(Lum, inTex).rgb*MaterialAmbient() : MaterialAmbient(), 0);
}
/******************************************************************************/
