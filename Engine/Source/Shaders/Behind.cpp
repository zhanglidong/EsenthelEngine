/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, TEXTURES
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec4 outVtx:POSITION ,
#if TEXTURES
   out Vec2 outTex:TEXCOORD0,
#endif
   out VecH outNrm:TEXCOORD1, // !! not Normalized !!
   out Vec  outPos:TEXCOORD2
)
{
#if TEXTURES
   outTex=vtx.tex();
#endif

   if(!SKIN)
   {
                     outNrm=TransformDir(vtx.nrm());
      outVtx=Project(outPos=TransformPos(vtx.pos()));
   }else
   {
      VecI bone=vtx.bone();
                     outNrm=TransformDir(vtx.nrm(), bone, vtx.weight());
      outVtx=Project(outPos=TransformPos(vtx.pos(), bone, vtx.weight()));
   }
}
/******************************************************************************/
VecH4 PS
(
   PIXEL,
#if TEXTURES
   Vec2 inTex:TEXCOORD0,
#endif
   VecH inNrm:TEXCOORD1,
   Vec  inPos:TEXCOORD2
):TARGET
{
#if TEXTURES
   // perform alpha testing
   if(TEXTURES==1)clip(Tex(Col, inTex).a + MaterialAlpha()-1);else // alpha in 'Col' texture
   if(TEXTURES==2)clip(Tex(Nrm, inTex).a + MaterialAlpha()-1);     // alpha in 'Nrm' texture, #MaterialTextureChannelOrder
#endif

   Half alpha=Sat((Half(inPos.z-TexDepthPoint(PixelToScreen(pixel)))-BehindBias)/0.3);

   VecH4  col   =Lerp(Color[0], Color[1], Abs(Normalize(inNrm).z));
          col.a*=alpha;
   return col;
}
/******************************************************************************/
