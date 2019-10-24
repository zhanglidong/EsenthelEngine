/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
/******************************************************************************
SKIN, COLORS, LAYOUT, BUMP_MODE, REFLECT
/******************************************************************************/
#define HEIGHTMAP 0
#define TESSELATE 0
#define SET_POS   (REFLECT)
#define SET_TEX   (LAYOUT || BUMP_MODE>SBUMP_FLAT)
/******************************************************************************/
struct VS_PS
{
#if SET_POS
   Vec pos:POS;
#endif

#if SET_TEX
   Vec2 tex:TEXCOORD;
#endif

   VecH4 color:COLOR;

#if   BUMP_MODE> SBUMP_FLAT
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE==SBUMP_FLAT
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif
};
void VS
(
   VtxInput vtx,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
#if SET_TEX
   O.tex=vtx.tex();
#endif
             O.color =Material.color;
   if(COLORS)O.color*=vtx.colorFast();

   Vec  pos=vtx.pos();
   VecH nrm, tan; if(BUMP_MODE>=SBUMP_FLAT)nrm=vtx.nrm(); if(BUMP_MODE>SBUMP_FLAT)tan=vtx.tan(nrm, HEIGHTMAP);
   if(!SKIN)
   {
      if(true) // instance
      {
         pos=TransformPos(pos, vtx.instance());
      #if   BUMP_MODE> SBUMP_FLAT
         nrm=TransformDir(nrm, vtx.instance());
         tan=TransformDir(tan, vtx.instance());
      #elif BUMP_MODE==SBUMP_FLAT
         nrm=TransformDir(nrm, vtx.instance());
      #endif
      }else
      {
         pos=TransformPos(pos);
      #if   BUMP_MODE> SBUMP_FLAT
         nrm=TransformDir(nrm);
         tan=TransformDir(tan);
      #elif BUMP_MODE==SBUMP_FLAT
         nrm=TransformDir(nrm);
      #endif
      }
   }else
   {
      VecU bone    =vtx.bone  ();
      VecH weight_h=vtx.weight();
      pos=TransformPos(pos, bone, vtx.weight());
   #if   BUMP_MODE> SBUMP_FLAT
      nrm=TransformDir(nrm, bone, weight_h);
      tan=TransformDir(tan, bone, weight_h);
   #elif BUMP_MODE==SBUMP_FLAT
      nrm=TransformDir(nrm, bone, weight_h);
   #endif
   }

   // normalize (have to do all at the same time, so all have the same lengths)
   if(BUMP_MODE>SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || TESSELATE) // needed for tesselation
   {
                              nrm=Normalize(nrm);
      if(BUMP_MODE>SBUMP_FLAT)tan=Normalize(tan);
   }

#if   BUMP_MODE> SBUMP_FLAT
   O.mtrx[0]=tan;
   O.mtrx[2]=nrm;
   O.mtrx[1]=vtx.bin(nrm, tan, HEIGHTMAP);
#elif BUMP_MODE==SBUMP_FLAT
   O.nrm=nrm;
#endif

#if SET_POS
   O.pos=pos;
#endif
   O_vtx=Project(pos);

   O.color.a*=(Half)Sat(Length(pos)*SkyFracMulAdd.x + SkyFracMulAdd.y);
}
/******************************************************************************/
VecH4 PS
(
   VS_PS I,
   out Half outAlpha:TARGET2 // #RTOutput.Blend
):TARGET
{
   Half smooth      =Material.smooth,
        reflectivity=Material.reflect;

   // #MaterialTextureLayout
#if LAYOUT
   if(LAYOUT==1)                            I.color    *=Tex(Col, I.tex);else                                                        // alpha in 'Col' texture
   if(LAYOUT==2){VecH4 ext=Tex(Ext, I.tex); I.color.rgb*=Tex(Col, I.tex).rgb; I.color.a*=ext.a; smooth*=ext.x; reflectivity*=ext.y;} // alpha in 'Ext' texture
#endif

   // normal
   VecH nrm;
   #if   BUMP_MODE==SBUMP_ZERO
      nrm=0;
   #elif BUMP_MODE==SBUMP_FLAT
      nrm=Normalize(I.Nrm());
   #else
      nrm.xy=Tex(Nrm, I.tex).xy*Material.normal;
      nrm.z =CalcZ(nrm.xy);
      nrm   =Normalize(Transform(nrm, I.mtrx));
   #endif

/*#if PER_PIXEL && FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   BackFlip(nrm, front);
#endif*/

   I.color.rgb+=Highlight.rgb;

#if REFLECT // reflection
   Vec eye_dir=Normalize(I.pos);
   I.color.rgb=PBR(I.color.rgb, I.color.rgb, nrm, smooth, reflectivity, eye_dir, 0);
#endif

   outAlpha=I.color.a;
   return   I.color;
}
/******************************************************************************/
