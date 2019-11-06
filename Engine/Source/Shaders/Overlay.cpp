/******************************************************************************/
#include "!Header.h"
#include "Overlay.h"
/******************************************************************************
SKIN, NORMALS, LAYOUT
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec      outTex   :TEXCOORD, // xy=uv, z=alpha
#if NORMALS
   out MatrixH3 outMatrix:MATRIX,
#else
   out VecH     outNrm   :NORMAL,
#endif
   out Vec4     outVtx   :POSITION
)
{
   Matrix3 m;
   m[0]=OverlayParams.mtrx[0]/Length2(OverlayParams.mtrx[0]);
   m[1]=OverlayParams.mtrx[1]/Length2(OverlayParams.mtrx[1]);
   m[2]=OverlayParams.mtrx[2]/Length2(OverlayParams.mtrx[2]);
   outTex   =TransformTP(vtx.pos()-OverlayParams.mtrx[3], m);
   outTex.xy=outTex.xy*0.5+0.5;
   outTex.z =1 - (Abs(outTex.z)-OverlayOpaqueFrac())/(1-OverlayOpaqueFrac()); // Abs(outTex.z)/-(1-OverlayOpaqueFrac()) + (OverlayOpaqueFrac()/(1-OverlayOpaqueFrac())+1)

   if(!SKIN)
   {
   #if NORMALS
      outMatrix[1]=Normalize(TransformDir(OverlayParams.mtrx[1]));
      outMatrix[2]=Normalize(TransformDir(OverlayParams.mtrx[2]));
   #else
      outNrm=Normalize(TransformDir(OverlayParams.mtrx[2]));
   #endif

      outVtx=Project(TransformPos(vtx.pos()));
   }else
   {
      VecU bone=vtx.bone();

   #if NORMALS
      outMatrix[1]=Normalize(TransformDir(OverlayParams.mtrx[1], bone, vtx.weight()));
      outMatrix[2]=Normalize(TransformDir(OverlayParams.mtrx[2], bone, vtx.weight()));
   #else
      outNrm=Normalize(TransformDir(OverlayParams.mtrx[2], bone, vtx.weight()));
   #endif

      outVtx=Project(TransformPos(vtx.pos(), bone, vtx.weight()));
   }
#if NORMALS
   outMatrix[0]=Cross(outMatrix[1], outMatrix[2]);
#endif
}
/******************************************************************************/
VecH4 PS
(
    Vec      inTex   :TEXCOORD,
#if NORMALS
    MatrixH3 inMatrix:MATRIX  ,
#else
    VecH     inNrm   :NORMAL  ,
#endif
out VecH4   outNrm   :TARGET1 ,
out VecH4   outExt   :TARGET2
):TARGET // #RTOutput
{
   Half  smooth =Material.smooth ,
         reflect=Material.reflect;
   VecH4 col    =Tex(Col, inTex.xy);
#if LAYOUT==2 // #MaterialTextureLayout
   VecH4 ext    =Tex(Ext, inTex.xy);
   smooth *=ext.x;
   reflect*=ext.y;
   col.a   =ext.a;
#endif
   col  *=Material.color;
   col.a*=Sat(inTex.z)*OverlayAlpha();

   VecH nrm;
#if NORMALS
                nrm.xy =Tex(Nrm, inTex.xy).xy*Material.normal; // #MaterialTextureLayout
    //if(DETAIL)nrm.xy+=det.xy;
                nrm.z  =CalcZ(nrm.xy);
                nrm    =Normalize(Transform(nrm, inMatrix));

#else
   nrm=Normalize(inNrm);
#endif

   // Nrm
#if SIGNED_NRM_RT
   outNrm.xyz=nrm;
#else
   outNrm.xyz=nrm*0.5+0.5;
#endif
   outNrm.w=col.a; // alpha needed because of blending

   // Ext
   outExt.x=smooth;
   outExt.y=reflect;
   outExt.z=0;
   outExt.w=col.a; // alpha needed because of blending

   return col;
}
/******************************************************************************/
