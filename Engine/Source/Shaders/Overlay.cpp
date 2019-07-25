/******************************************************************************/
#include "!Header.h"
#include "Overlay.h"
/******************************************************************************
SKIN, NORMALS
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out Vec     outTex   :TEXCOORD0, // xy=uv, z=alpha
#if NORMALS
   out Matrix3 outMatrix:TEXCOORD1,
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
      VecI bone=vtx.bone();

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
     Vec      inTex   :TEXCOORD0
#if NORMALS
 ,   Matrix3  inMatrix:TEXCOORD1
 , out VecH4 outNrm   :TARGET1
#endif
):TARGET
{
   VecH4 col  =Tex(Col, inTex.xy);
   Half  alpha=Sat((Half)inTex.z)*OverlayAlpha();

#if NORMALS
      VecH4 tex     =Tex(Nrm, inTex.xy); // #MaterialTextureChannelOrder
    //Half  specular=tex.z*MaterialSpecular();

           VecH nrm;
                nrm.xy =(tex.xy*2-1)*MaterialRough();
    //if(detail)nrm.xy+=det.xy;
                nrm.z  =CalcZ(nrm.xy);
                nrm    =Transform(nrm, inMatrix);

      col.a=tex.w;

   #if SIGNED_NRM_RT
      outNrm.xyz=nrm;
   #else
      outNrm.xyz=nrm*0.5+0.5;
   #endif
#endif

   col.a*=alpha;
   col  *=MaterialColor();

#if NORMALS
   outNrm.w=col.a;
#endif

   return col;
}
/******************************************************************************/
