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
void VS
(
   VtxInput vtx,

   out Data O,
   out Vec4 pixel:POSITION
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
      pixel=Project(O.pos=TransformPos(vtx.pos()));
   }else
   {
      VecU bone=vtx.bone();
                    O.nrm=TransformDir(vtx.nrm(), bone, vtx.weight());
      pixel=Project(O.pos=TransformPos(vtx.pos(), bone, vtx.weight()));
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
   MaterialAlphaTest(Tex(Col, I.uv).a);
#elif ALPHA_TEST==ALPHA_TEST_DITHER
   MaterialAlphaTestDither(Tex(Col, I.uv).a, pixel.xy, I.face_id);
#endif

   Half alpha=Sat((Half(I.pos.z-TexDepthPoint(PixelToUV(pixel)))-BehindBias)/0.3);

   VecH4  col   =Lerp(Color[0], Color[1], Abs(Normalize(I.nrm).z));
          col.a*=alpha;
   return col;
}
/******************************************************************************/
