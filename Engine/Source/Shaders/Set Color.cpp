/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, ALPHA_TEST, TESSELATE
/******************************************************************************/
struct Data
{
#if TESSELATE
   Vec  pos:POS;
   VecH nrm:NORMAL;
#endif

#if ALPHA_TEST
   Vec2 tex:TEXCOORD;
#endif

#if ALPHA_TEST==ALPHA_TEST_DITHER
   NOINTERP VecU2 face_id:FACE_ID;
#endif
};
/******************************************************************************/
// VS
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Data O,
   out Vec4 pixel:POSITION
)
{
#if ALPHA_TEST
   O.tex=vtx.tex();
#endif

#if ALPHA_TEST==ALPHA_TEST_DITHER
   O.face_id=vtx.faceID();
#endif

   Vec pos;
   if(!SKIN)
   {
   #if TESSELATE
      O.nrm=Normalize(TransformDir(vtx.nrm()));
   #endif
        pos=          TransformPos(vtx.pos());
   }else
   {
      VecU bone=vtx.bone();
   #if TESSELATE
      O.nrm=Normalize(TransformDir(vtx.nrm(), bone, vtx.weight()));
   #endif
        pos=          TransformPos(vtx.pos(), bone, vtx.weight());
   }
#if TESSELATE
   O.pos=pos;
#endif
   pixel=Project(pos);
}
/******************************************************************************/
// PS
/******************************************************************************/
VecH4 PS
(
   Data I
#if ALPHA_TEST==ALPHA_TEST_DITHER
      , PIXEL
#endif
):TARGET
{
#if ALPHA_TEST==ALPHA_TEST_YES
   MaterialAlphaTest(Tex(Col, I.tex).a);
#elif ALPHA_TEST==ALPHA_TEST_DITHER
   MaterialAlphaTestDither(Tex(Col, I.tex).a, pixel.xy, I.face_id);
#endif

   return Highlight;
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if TESSELATE
HSData HSConstant(InputPatch<Data,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm);}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
Data HS
(
   InputPatch<Data,3> I,
   UInt cp_id:SV_OutputControlPointID
)
{
   Data O;
   O.pos=I[cp_id].pos;
   O.nrm=I[cp_id].nrm;
#if ALPHA_TEST
   O.tex=I[cp_id].tex;
#endif
#if ALPHA_TEST==ALPHA_TEST_DITHER
   O.face_id=I[cp_id].face_id;
#endif
   return O;
}
/******************************************************************************/
[domain("tri")]
void DS
(
   HSData hs_data, const OutputPatch<Data,3> I, Vec B:SV_DomainLocation,

   out Data O,
   out Vec4 pixel:POSITION
)
{
#if ALPHA_TEST
   O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;
#endif

#if ALPHA_TEST==ALPHA_TEST_DITHER
   O.face_id=I[0].face_id;
#endif

   SetDSPosNrm(O.pos, O.nrm, I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, B, hs_data, false, 0);
   pixel=Project(O.pos);
}
#endif
/******************************************************************************/
