/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, ALPHA_TEST, LIGHT_MAP
/******************************************************************************/
void VS
(
   VtxInput vtx,

#if ALPHA_TEST || LIGHT_MAP
   out Vec2 outTex:TEXCOORD,
#endif
   out Vec4 outVtx:POSITION,

   CLIP_DIST
)
{
#if ALPHA_TEST || LIGHT_MAP
   outTex=vtx.tex();
#endif

   Vec      pos;
   if(!SKIN)pos=TransformPos(vtx.pos());
   else     pos=TransformPos(vtx.pos(), vtx.bone(), vtx.weight());
 CLIP_PLANE(pos); outVtx=Project(pos);
}
/******************************************************************************/
VecH4 PS
(
#if ALPHA_TEST || LIGHT_MAP
   Vec2 inTex:TEXCOORD
#endif
):TARGET
{
#if ALPHA_TEST
   if(ALPHA_TEST==1)clip(Tex(Col, inTex).a + Material.color.a-1);else
   if(ALPHA_TEST==2)clip(Tex(Ext, inTex).a + Material.color.a-1); // #MaterialTextureChannelOrder
#endif

#if LIGHT_MAP
   return VecH4(Tex(Lum, inTex).rgb*Material.ambient, 0);
#else
   return VecH4(Material.ambient, 0);
#endif
}
/******************************************************************************/
