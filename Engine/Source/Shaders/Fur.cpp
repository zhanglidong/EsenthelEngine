/******************************************************************************/
#include "!Header.h"
#include "Fur.h"
/******************************************************************************/
#define USE_VEL 1
#define FACTOR (-0.7) // prevents complete darkness at the bottom layers, gives ambient=0.3, it will match the 'size' version
/******************************************************************************/
VecH GetBoneFurVel(VecU bone, VecH weight) {return weight.x*FurVel[bone.x] + weight.y*FurVel[bone.y] + weight.z*FurVel[bone.z];}
/******************************************************************************/
// SKIN, SIZE, DIFFUSE
/******************************************************************************/
struct Base_VS_PS
{
   Vec2 tex:TEXCOORD;
   VecH nrm:NORMAL  ; // !! not Normalized !!
#if USE_VEL
   Vec projected_prev_pos_xyw:PREV_POS;
#endif
#if SIZE
   Half len:LENGTH;
#endif
};
void Base_VS
(
   VtxInput vtx,

   out Base_VS_PS O,
   out Vec4 outVtx:POSITION,

   CLIP_DIST
)
{
   Vec view_pos, view_pos_prev;
   O.tex=vtx.tex();

   if(!SKIN)
   {
      if(true) // instance
      {
                    view_pos     =TransformPos    (vtx.pos(), vtx.instance());
         if(USE_VEL)view_pos_prev=TransformPosPrev(vtx.pos(), vtx.instance());
                       O.nrm     =TransformDir    (vtx.nrm(), vtx.instance());
      }else
      {
                    view_pos     =TransformPos    (vtx.pos());
         if(USE_VEL)view_pos_prev=TransformPosPrev(vtx.pos());
                       O.nrm     =TransformDir    (vtx.nrm());
      }
   }else
   {
      VecU bone=vtx.bone();
                 view_pos     =TransformPos    (vtx.pos(), bone, vtx.weight());
      if(USE_VEL)view_pos_prev=TransformPosPrev(vtx.pos(), bone, vtx.weight());
                    O.nrm     =TransformDir    (vtx.nrm(), bone, vtx.weight());
   }
#if SIZE
   O.len=vtx.size();
#endif
   CLIP_PLANE(view_pos); outVtx=Project(view_pos);
#if USE_VEL
   O.projected_prev_pos_xyw=ProjectPrevXYW(view_pos_prev);
#endif
}
/******************************************************************************/
void Base_PS
(
   Base_VS_PS I,
#if USE_VEL
   PIXEL,
#endif
   out DeferredSolidOutput output
)
{
   Half fur=Tex(FurCol, I.tex*Material.det_scale).r;
#if SIZE
   VecH col=Sat(I.len*-fur+1); // I.len*-fur+step+1 : fur*FACTOR+step+1, here step=0
#else
   VecH col=Sat(fur*FACTOR+1); // I.len*-fur+step+1 : fur*FACTOR+step+1, here step=0
#endif
   if(DIFFUSE)col*=Tex(Col, I.tex).rgb;
   col=col*Material.color.rgb+Highlight.rgb;

   I.nrm=Normalize(I.nrm);

   output.color       (col);
   output.glow        (0);
   output.normal      (I.nrm);
   output.translucent (0);
   output.smooth      (Material.smooth);
   output.reflect     (Material.reflect);
#if USE_VEL
   output.velocity    (I.projected_prev_pos_xyw, pixel);
#else
   output.velocityZero();
#endif
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
              color.rgb =(color.rgb*Material.color.rgb+Highlight.rgb)*TexPoint(FurLight, ProjectedPosXYWToUV(inOrigPos)).rgb; // we need to access the un-expanded pixel and not current pixel
   return     color;
}
/******************************************************************************/
