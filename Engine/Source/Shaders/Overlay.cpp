/******************************************************************************/
#include "!Header.h"
#include "Overlay.h"
/******************************************************************************
SKIN, NORMALS, ALPHA
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec     outTex   :TEXCOORD, // xy=uv, z=alpha
#if NORMALS
   out Matrix3 outMatrix:MATRIX,
#endif
   out Vec4    outVtx   :POSITION
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
   #endif

      outVtx=Project(TransformPos(vtx.pos()));
   }else
   {
      VecU bone=vtx.bone();

   #if NORMALS
      outMatrix[1]=Normalize(TransformDir(OverlayParams.mtrx[1], bone, vtx.weight()));
      outMatrix[2]=Normalize(TransformDir(OverlayParams.mtrx[2], bone, vtx.weight()));
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
     Vec      inTex   :TEXCOORD
#if NORMALS
 ,   Matrix3  inMatrix:MATRIX
 , out VecH4 outNrm   :TARGET1
#endif
):TARGET
{
   VecH4 col  =Tex(Col, inTex.xy);
#if ALPHA
         col.a=Tex(Ext, inTex.xy).a; // #MaterialTextureLayout
#endif
   col  *=Material.color;
   col.a*=Sat((Half)inTex.z)*OverlayAlpha();

#if NORMALS
           VecH nrm;
                nrm.xy =Tex(Nrm, inTex.xy).xy*Material.normal; // #MaterialTextureLayout
    //if(DETAIL)nrm.xy+=det.xy;
                nrm.z  =CalcZ(nrm.xy);
                nrm    =Transform(nrm, inMatrix);

   #if SIGNED_NRM_RT
      outNrm.xyz=nrm;
   #else
      outNrm.xyz=nrm*0.5+0.5;
   #endif

   outNrm.w=col.a;
#endif

   return col;
}
/******************************************************************************/
