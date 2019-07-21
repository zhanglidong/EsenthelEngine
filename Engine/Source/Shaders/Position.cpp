/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
struct VS_PS
{
   Vec  pos:TEXCOORD0,
        nrm:TEXCOORD1;
   Vec2 tex:TEXCOORD2;
};
/******************************************************************************/
// VS
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
   Vec  pos=vtx.pos();
   VecH nrm; if(TESSELATE)nrm=vtx.nrm();

   if(FX==FX_LEAF)
   {
      if(TESSELATE)BendLeaf(vtx.hlp(), pos, nrm);
      else         BendLeaf(vtx.hlp(), pos);
   }
   if(FX==FX_LEAFS)
   {
      if(TESSELATE)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm);
      else         BendLeafs(vtx.hlp(), vtx.size(), pos);
   }

   if(!SKIN)
   {
      if(true) // instance
      {
                      O.pos=          TransformPos(pos, vtx.instance());
         if(TESSELATE)O.nrm=Normalize(TransformDir(nrm, vtx.instance()));
         if(FX==FX_GRASS)BendGrass(pos, O.pos, vtx.instance());
      }else
      {
                      O.pos=          TransformPos(pos);
         if(TESSELATE)O.nrm=Normalize(TransformDir(nrm));
         if(FX==FX_GRASS)BendGrass(pos, O.pos);
      }
   }else
   {
      VecI bone=vtx.bone();
                   O.pos=          TransformPos(pos, bone, vtx.weight());
      if(TESSELATE)O.nrm=Normalize(TransformDir(nrm, bone, vtx.weight()));
   }

   if(TEXTURES)O.tex=vtx.tex();
               O_vtx=Project(O.pos);
}
/******************************************************************************/
// PS
/******************************************************************************/
void PS(VS_PS I)
{
   if(TEXTURES==1)clip(Tex(Col, I.tex).a+(TEST_BLEND ? (MaterialAlpha()*0.5-1) : (MaterialAlpha()-1)));else
   if(TEXTURES==2)clip(Tex(Nrm, I.tex).a+(TEST_BLEND ? (MaterialAlpha()*0.5-1) : (MaterialAlpha()-1))); // #MaterialTextureChannelOrder
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if !CG
HSData HSConstant(InputPatch<VS_PS,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, true);}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
VS_PS HS(InputPatch<VS_PS,3> I, UInt cp_id:SV_OutputControlPointID)
{
   VS_PS O;
               O.pos=I[cp_id].pos;
               O.nrm=I[cp_id].nrm;
   if(TEXTURES)O.tex=I[cp_id].tex;
   return O;
}
/******************************************************************************/
[domain("tri")]
void DS
(
   HSData hs_data, const OutputPatch<VS_PS,3> I, Vec B:SV_DomainLocation,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
   if(TEXTURES)O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;

   SetDSPosNrm(O.pos, O.nrm, I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, B, hs_data, false, 0);
   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
