/******************************************************************************/
#include "!Header.h"
#include "Fur.h"
#define FACTOR (-0.7) // prevents complete darkness at the bottom layers, gives ambient=0.3, it will match the 'size' version
/******************************************************************************/
VecH GetBoneFurVel(VecU bone, VecH weight) {return weight.x*FurVel[bone.x] + weight.y*FurVel[bone.y] + weight.z*FurVel[bone.z];}
/******************************************************************************/
// SKIN, SIZE, DIFFUSE
/******************************************************************************/
void Base_VS
(
   VtxInput vtx,

   out Vec2 outTex:TEXCOORD,
   out VecH outNrm:NORMAL  , // !! not Normalized !!
   out Vec  outPos:POS     ,
   out Vec  outVel:VELOCITY,
#if SIZE
   out Half outLen:LENGTH  ,
#endif
   out Vec4 outVtx:POSITION,

   CLIP_DIST
)
{
   outTex=vtx.tex();

   if(!SKIN)
   {
      if(true) // instance
      {
         outPos=TransformPos(vtx.pos(),         vtx.instance());
         outNrm=TransformDir(vtx.nrm(),         vtx.instance());
         outVel=GetObjVel   (vtx.pos(), outPos, vtx.instance());
      }else
      {
         outPos=TransformPos(vtx.pos());
         outNrm=TransformDir(vtx.nrm());
         outVel=GetObjVel   (vtx.pos(), outPos);
      }
   }else
   {
      VecU bone=vtx.bone();
      outPos=TransformPos(vtx.pos(),         bone, vtx.weight());
      outNrm=TransformDir(vtx.nrm(),         bone, vtx.weight());
      outVel=GetBoneVel  (vtx.pos(), outPos, bone, vtx.weight());
   }
#if SIZE
   outLen=vtx.size();
#endif
   CLIP_PLANE(outPos); outVtx=Project(outPos);
}
/******************************************************************************/
void Base_PS
(
   Vec2 inTex:TEXCOORD,
   VecH inNrm:NORMAL  ,
   Vec  inPos:POS     ,
   Vec  inVel:VELOCITY,
#if SIZE
   Half inLen:LENGTH  ,
#endif

   out DeferredSolidOutput output
)
{
   Half fur=Tex(FurCol, inTex*Material.det_scale).r;
#if SIZE
   VecH col=Sat(inLen*-fur+1); // inLen*-fur+step+1 : fur*FACTOR+step+1, here step=0
#else
   VecH col=Sat(fur*FACTOR+1); // inLen*-fur+step+1 : fur*FACTOR+step+1, here step=0
#endif
   if(DIFFUSE)col*=Tex(Col, inTex).rgb;
   col=col*Material.color.rgb+Highlight.rgb;

   inNrm=Normalize(inNrm);

   output.color      (col);
   output.glow       (0);
   output.normal     (inNrm);
   output.translucent(0);
   output.smooth     (Material.smooth);
   output.velocity   (inVel, inPos);
   output.reflect    (Material.reflect);
}
/******************************************************************************/
void Soft_VS
(
   VtxInput vtx,

   out Vec2 outTex    :TEXCOORD,
   out Vec  outOrigPos:ORIG_POS,
#if SIZE
   out Half outLen    :LENGTH  ,
#endif
   out Vec4 outVtx    :POSITION
)
{
   Vec  pos=vtx.pos();
   VecH nrm=vtx.nrm();

   outTex=vtx.tex();

   if(!SKIN)
   {
      pos=TransformPos(pos); nrm+=FurVel[0]; nrm=Normalize(nrm);
      nrm=TransformDir(nrm);
   }else
   {
      VecU bone=vtx.bone();
      pos =TransformPos (pos, bone, vtx.weight());
      nrm+=GetBoneFurVel(     bone, vtx.weight()); nrm=Normalize(nrm);
      nrm =TransformDir (nrm, bone, vtx.weight());
   }
   outOrigPos=ProjectXYW(pos); // set in 'outOrigPos' the original position without expansion
#if SIZE
   outLen=vtx.size();
#endif
   pos+=nrm*(SIZE ? vtx.size()*Material.det_power*FurStep.x : Material.det_power*FurStep.x);
   outVtx=Project(pos);
}
/******************************************************************************/
VecH4 Soft_PS
(
   Vec2 inTex    :TEXCOORD,
   Vec  inOrigPos:ORIG_POS
#if SIZE
 , Half inLen    :LENGTH
#endif
, out Half outAlpha:TARGET2 // #RTOutput.Blend
):TARGET
{
   Half fur=Tex(FurCol, inTex*Material.det_scale).r;

   VecH4 color;
#if SIZE
   color.rgb=Sat(inLen*-fur+FurStep.y   ); // inLen*-fur+step+1 : fur*FACTOR+step+1
   color.a  =Sat(inLen*(1-FurStep.x/fur)); // alternative: Sat(1-FurStep.x/(fur*inLen))
#else
   color.rgb=Sat(fur*FACTOR+FurStep.y); // inLen*-fur+step+1 : fur*FACTOR+step+1
   color.a  =Sat(1-FurStep.x/fur     ); // alternative: Sat(1-FurStep.x/(fur*inLen))
#endif

   outAlpha=color.a;
   
   if(DIFFUSE)color.rgb*=Tex(Col, inTex).rgb;
              color.rgb =(color.rgb*Material.color.rgb+Highlight.rgb)*TexPoint(FurLight, ProjectedPosXYWToScreen(inOrigPos)).rgb; // we need to access the un-expanded pixel and not current pixel
   return     color;
}
/******************************************************************************/
