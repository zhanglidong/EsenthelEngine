/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, TESSELATE
/******************************************************************************/
struct Data
{
   Vec2 tex:TEXCOORD;
#if TESSELATE
   Vec  pos:POS;
   VecH nrm:NORMAL;
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
   O.tex=vtx.tex();

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
):TARGET
{
   return Tex(Col, I.tex)*Color[0];
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
   O.tex=I[cp_id].tex;
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
   O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;

   SetDSPosNrm(O.pos, O.nrm, I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, B, hs_data, false, 0);
   pixel=Project(O.pos);
}
#endif
/******************************************************************************/
