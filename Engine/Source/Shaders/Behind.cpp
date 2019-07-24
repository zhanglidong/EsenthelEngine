/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#define PARAMS           \
   uniform Bool skin    ,\
   uniform Int  textures
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec4 outVtx:POSITION ,
   out Vec2 outTex:TEXCOORD0,
   out VecH outNrm:TEXCOORD1, // !! not Normalized !!
   out Vec  outPos:TEXCOORD2
)
{
   outTex=vtx.tex();

   if(!skin)
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
   Vec2 inTex:TEXCOORD0,
   VecH inNrm:TEXCOORD1,
   Vec  inPos:TEXCOORD2
):TARGET
{
   // perform alpha testing
   if(textures==1)clip(Tex(Col, inTex).a + MaterialAlpha()-1);else // alpha in 'Col' texture
   if(textures==2)clip(Tex(Nrm, inTex).a + MaterialAlpha()-1);     // alpha in 'Nrm' texture, #MaterialTextureChannelOrder

   Half alpha=Sat((Half(inPos.z-TexDepthPoint(PixelToScreen(pixel)))-BehindBias)/0.3);

   VecH4  col   =Lerp(Color[0], Color[1], Abs(Normalize(inNrm).z));
          col.a*=alpha;
   return col;
}
/******************************************************************************/
