/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, ALPHA_TEST
/******************************************************************************/
struct Data
{
#if ALPHA_TEST
   Vec2 uv:UV;
#endif
#if ALPHA_TEST==ALPHA_TEST_DITHER
   NOINTERP VecU2 face_id:FACE_ID;
#endif
   VecH nrm:NORMAL; // !! not Normalized !!
   Vec  pos:POS;
};
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Data O,
   out Vec4 vpos:POSITION
)
{
#if ALPHA_TEST
   O.uv=vtx.uv();
#endif
#if ALPHA_TEST==ALPHA_TEST_DITHER
   O.face_id=vtx.faceID();
#endif

   if(!SKIN)
   {
                   O.nrm=TransformDir(vtx.nrm());
      vpos=Project(O.pos=TransformPos(vtx.pos()));
   }else
   {
      VecU bone=vtx.bone();
                   O.nrm=TransformDir(vtx.nrm(), bone, vtx.weight());
      vpos=Project(O.pos=TransformPos(vtx.pos(), bone, vtx.weight()));
   }
}
/******************************************************************************/
VecH4 PS
(
   Data I,
   PIXEL
):TARGET
{
#if ALPHA_TEST==ALPHA_TEST_YES
   MaterialAlphaTest(RTex(Col, I.uv).a);
#elif ALPHA_TEST==ALPHA_TEST_DITHER
   MaterialAlphaTestDither(RTex(Col, I.uv).a, pixel.xy, I.face_id);
#endif

   Half alpha=Sat((Half(I.pos.z-TexDepthPix(pixel.xy))-BehindBias)/0.3);

   VecH4  col   =Lerp(Color[0], Color[1], Abs(Normalize(I.nrm).z));
          col.a*=alpha;
   return col;
}
/******************************************************************************/
