/******************************************************************************/
#include "!Header.h"

#include "!Set Prec Struct.h"
BUFFER(BoneHighlight)
   Flt BoneHighlight;
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************
SKIN, BUMP_MODE
/******************************************************************************/
struct VS_PS
{
#if BUMP_MODE>=SBUMP_FLAT
   VecH nrm:NORMAL;
#endif
   VecH col:COLOR;
};
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
#if BUMP_MODE>=SBUMP_FLAT
   O.nrm=TransformDir(vtx.nrm());
#endif

   Flt b; if(!SKIN)b=0;else
   {
      VecI bone=vtx._bone; // use real bone regardless if skinning is enabled
      b=vtx.weight().x*Sat(1-Abs(bone.x-BoneHighlight))
       +vtx.weight().y*Sat(1-Abs(bone.y-BoneHighlight))
       +vtx.weight().z*Sat(1-Abs(bone.z-BoneHighlight));
   }
   O.col=((b>EPS) ? VecH(b, 0, 1-b) : VecH(1, 1, 1));

   O_vtx=Project(TransformPos(vtx.pos()));
}
/******************************************************************************/
VecH4 PS(VS_PS I, IS_FRONT):TARGET
{
   if(LINEAR_GAMMA)I.col.rgb=SRGBToLinear(I.col.rgb);
   I.col.rgb+=Highlight.rgb;

   // perform lighting
#if BUMP_MODE>=SBUMP_FLAT
   VecH nrm=Normalize(I.nrm); BackFlip(nrm, front);
   Half d  =Sat(Dot(nrm, LightDir.dir));
   if(LINEAR_GAMMA)d=Sqr(d);
   VecH lum=LightDir.color.rgb*d + AmbientNSColor;
   I.col.rgb*=lum;
#endif

   return VecH4(I.col.rgb, 0);
}
/******************************************************************************/
