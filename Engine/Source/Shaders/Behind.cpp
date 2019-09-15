/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, ALPHA
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec4 outVtx:POSITION,
#if ALPHA
   out Vec2 outTex:TEXCOORD,
#endif
   out VecH outNrm:NORMAL, // !! not Normalized !!
   out Vec  outPos:POS
)
{
#if ALPHA
   outTex=vtx.tex();
#endif

   if(!SKIN)
   {
                     outNrm=TransformDir(vtx.nrm());
      outVtx=Project(outPos=TransformPos(vtx.pos()));
   }else
   {
      VecU bone=vtx.bone();
                     outNrm=TransformDir(vtx.nrm(), bone, vtx.weight());
      outVtx=Project(outPos=TransformPos(vtx.pos(), bone, vtx.weight()));
   }
}
/******************************************************************************/
VecH4 PS
(
   PIXEL,
#if ALPHA
   Vec2 inTex:TEXCOORD,
#endif
   VecH inNrm:NORMAL,
   Vec  inPos:POS
):TARGET
{
#if ALPHA // alpha-test
   if(ALPHA==1)clip(Tex(Col, inTex).a + MaterialAlpha()-1);else // alpha in 'Col' texture
   if(ALPHA==2)clip(Tex(Nrm, inTex).a + MaterialAlpha()-1);     // alpha in 'Nrm' texture, #MaterialTextureChannelOrder
#endif

   Half alpha=Sat((Half(inPos.z-TexDepthPoint(PixelToScreen(pixel)))-BehindBias)/0.3);

   VecH4  col   =Lerp(Color[0], Color[1], Abs(Normalize(inNrm).z));
          col.a*=alpha;
   return col;
}
/******************************************************************************/
