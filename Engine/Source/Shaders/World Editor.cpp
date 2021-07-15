/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
// COLOR
/******************************************************************************/
void Color_VS
(
   VtxInput vtx,

#if VTX_COL
   out VecH4 outCol:COLOR    ,
#endif
   out VecH  outNrm:TEXCOORD0,
   out Vec   outPos:TEXCOORD1,
   out Vec4  pixel :POSITION
)
{  
#if VTX_COL
   outCol=vtx.colorFast();
#endif

   outNrm=Normalize(TransformDir(vtx.nrm()));
   outPos=          TransformPos(vtx.pos()) ;
   pixel =          Project     ( outPos  ) ;
}
/******************************************************************************/
#ifndef COL_VALUE
#define COL_VALUE 0,0,0
#endif
void Color_PS
(
#if VTX_COL
   VecH4 inCol:COLOR    ,
#endif
   VecH  inNrm:TEXCOORD0,
   Vec   inPos:TEXCOORD1,

   out DeferredSolidOutput output
)
{
#if VTX_COL
   output.color      (VecH(COL_VALUE)*inCol.rgb+Highlight.rgb);
#else
   output.color      (VecH(COL_VALUE)+Highlight.rgb);
#endif
   output.glow       (0);
   output.normal     (Normalize(inNrm));
   output.translucent(0);
   output.rough      (1);
   output.reflect    (0);
   output.motionZero ( );
}
/******************************************************************************/
// CIRCLE / SQUARE / GRID
/******************************************************************************/
#include "!Set Prec Struct.h"

BUFFER(WorldEditor)
   Flt  XZImageUse,
        XZPattern;
   Flt  XZRange,
        XZSoft,
        XZAngle,
        XZPatternScale;
   Vec2 XZPos;
   VecH XZCol;
BUFFER_END

#include "!Set Prec Image.h"

Image XZImage;

#include "!Set Prec Default.h"
/******************************************************************************/
struct VS_PS
{
   Vec  pos  :TEXCOORD0,
        nrm  :TEXCOORD1;
   Vec2 pos2D:TEXCOORD2;
   Vec4 vtx  :POSITION ;
};
struct VS_PS_NOVTX
{
   Vec  pos  :TEXCOORD0,
        nrm  :TEXCOORD1;
   Vec2 pos2D:TEXCOORD2;
};
/******************************************************************************/
VS_PS FX_VS
(
   VtxInput vtx
)
{
   VS_PS O;
   O.pos=          TransformPos(vtx.pos()) ; O.pos2D=Transform(O.pos, CamMatrix).xz;
   O.nrm=Normalize(TransformDir(vtx.nrm()));
   O.vtx=          Project     (  O.pos  ) ;
   return O;
}
/******************************************************************************/
Vec4 Circle_PS
(
   VS_PS I
):TARGET
{
   Vec2 cos_sin; CosSin(cos_sin.x, cos_sin.y, XZAngle);
   Vec2 d=I.pos2D-XZPos; d=Rotate(d, cos_sin); d/=XZRange;

   Flt b;
   b=Length(d);
   b=Sat   ((b-(1-XZSoft))/XZSoft);
   b=BlendSmoothCube(b);

   if(LINEAR_GAMMA)b=SRGBToLinearFast(b);
   Vec col=XZCol*b;
   if(XZImageUse)col*=Tex(XZImage, (XZPattern ? I.pos2D*XZPatternScale : d*0.5+0.5)*Vec2(1,-1)).r; // XZImage is sRGB
   return Vec4(col, 0);
}
/******************************************************************************/
Vec4 Square_PS
(
   VS_PS I
):TARGET
{
   Vec2 cos_sin; CosSin(cos_sin.x, cos_sin.y, XZAngle);
   Vec2 d=I.pos2D-XZPos; d=Rotate(d, cos_sin); d/=XZRange;

   Flt b;
   b=Max(Abs(d));
   b=Sat((b-(1-XZSoft))/XZSoft);
   b=BlendSmoothCube(b);

   if(LINEAR_GAMMA)b=SRGBToLinearFast(b);
   Vec col=XZCol*b;
   if(XZImageUse)col*=Tex(XZImage, (XZPattern ? I.pos2D*XZPatternScale : d*0.5+0.5)*Vec2(1, -1)).r; // XZImage is sRGB
   return Vec4(col, 0);
}
/******************************************************************************/
Vec4 Grid_PS
(
   VS_PS_NOVTX I
):TARGET
{
   Vec2 pos  =I.pos2D/XZRange;
   Vec2 xz   =Sat((Abs(Frac(pos)-0.5)-0.5)/XZSoft+1);
   Flt  alpha=Max(xz);
   Flt  dd=Max(Vec4(Abs(ddx(pos)), Abs(ddy(pos)))); alpha*=LerpRS(0.2, 0.1, dd);

   if(LINEAR_GAMMA)alpha=SRGBToLinearFast(alpha);
   Vec col=XZCol*alpha;
   return Vec4(col, 0);
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************
HSData HSConstant(InputPatch<VS_PS_NOVTX,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm);}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
VS_PS_NOVTX HS
(
   InputPatch<VS_PS_NOVTX,3> I,
   UInt cp_id:SV_OutputControlPointID
)
{
   VS_PS_NOVTX O;
   O.pos  =I[cp_id].pos;
   O.nrm  =I[cp_id].nrm;
   O.pos2D=I[cp_id].pos2D;
   return O;
}

[domain("tri")]
VS_PS DS
(
   HSData hs_data, const OutputPatch<VS_PS_NOVTX,3> I, Vec B:SV_DomainLocation
)
{
   VS_PS O;

   O.pos2D=I[0].pos2D*B.z + I[1].pos2D*B.x + I[2].pos2D*B.y;

   SetDSPosNrm(O.pos, O.nrm, I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, B, hs_data, false, 0);
   O.vtx=Project(O.pos);

   return O;
}
/******************************************************************************/
