/******************************************************************************/
#include "!Header.h"
#include "Fog.h"
#include "Sky.h"
/******************************************************************************/
//SKIN, COLORS, LAYOUT, BUMP_MODE, ALPHA_TEST, ALPHA, REFLECT, LIGHT_MAP, FX, PER_PIXEL, SHADOW_MAPS, TESSELATE
#define USE_VEL     ALPHA_TEST
#define HEIGHTMAP   0
#define SET_POS     (USE_VEL || SHADOW_MAPS || (REFLECT && PER_PIXEL && BUMP_MODE>SBUMP_FLAT))
#define SET_TEX     (LAYOUT || LIGHT_MAP || BUMP_MODE>SBUMP_FLAT)
#define VTX_REFLECT (REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT))
#define ALPHA_CLIP  0.5
/******************************************************************************/
struct VS_PS
{
#if SET_POS
   Vec pos:POS;
#endif

#if SET_TEX
   Vec2 tex:TEXCOORD;
#endif

   VecH4 col    :COLOR;
   VecH  col_add:COLOR_ADD;

#if   BUMP_MODE> SBUMP_FLAT && PER_PIXEL
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE==SBUMP_FLAT && PER_PIXEL
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif

#if USE_VEL
   Vec vel:VELOCITY;
#endif

#if VTX_REFLECT
   VecH rfl:REFLECTION;
#endif

#if !PER_PIXEL && BUMP_MODE>=SBUMP_FLAT && SHADOW_MAPS
   VecH lum:LUM;
#endif
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
   VecH nrm, tan; if(BUMP_MODE>=SBUMP_FLAT)nrm=vtx.nrm(); if(BUMP_MODE>SBUMP_FLAT)tan=vtx.tan(nrm, HEIGHTMAP);

#if SET_TEX
   O.tex=vtx.tex(HEIGHTMAP);
   if(HEIGHTMAP)O.tex*=Material.tex_scale;
#endif

             O.col =Material.color;
   if(COLORS)O.col*=vtx.colorFast();

   if(FX==FX_LEAF)
   {
      if(BUMP_MODE> SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm, tan);else
      if(BUMP_MODE==SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm     );else
                               BendLeaf(vtx.hlp(), pos          );
   }
   if(FX==FX_LEAFS)
   {
      if(BUMP_MODE> SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm, tan);else
      if(BUMP_MODE==SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm     );else
                               BendLeafs(vtx.hlp(), vtx.size(), pos          );
   }

   Vec local_pos; if(USE_VEL || FX==FX_GRASS)local_pos=pos;
   if(!SKIN)
   {
      if(true) // instance
      {
         pos=TransformPos(pos, vtx.instance());
      #if USE_VEL
         O.vel=GetObjVel(local_pos, pos, vtx.instance());
      #endif

      #if   BUMP_MODE> SBUMP_FLAT
         nrm=TransformDir(nrm, vtx.instance());
         tan=TransformDir(tan, vtx.instance());
      #elif BUMP_MODE==SBUMP_FLAT
         nrm=TransformDir(nrm, vtx.instance());
      #endif

         if(FX==FX_GRASS)
         {
            BendGrass(local_pos, pos, vtx.instance());
              O.col.a*=1-GrassFadeOut(vtx.instance());
         }
      }else
      {
         pos=TransformPos(pos);
      #if USE_VEL
         O.vel=GetObjVel(local_pos, pos);
      #endif

      #if   BUMP_MODE> SBUMP_FLAT
         nrm=TransformDir(nrm);
         tan=TransformDir(tan);
      #elif BUMP_MODE==SBUMP_FLAT
         nrm=TransformDir(nrm);
      #endif

         if(FX==FX_GRASS)
         {
            BendGrass(local_pos, pos);
            O.col.a*=1-GrassFadeOut();
         }
      }
   }else
   {
      VecU bone    =vtx.bone  ();
      VecH weight_h=vtx.weight();
      pos=TransformPos(pos, bone, vtx.weight());
   #if USE_VEL
      O.vel=GetBoneVel(local_pos, pos, bone, weight_h);
   #endif

   #if   BUMP_MODE> SBUMP_FLAT
      nrm=TransformDir(nrm, bone, weight_h);
      tan=TransformDir(tan, bone, weight_h);
   #elif BUMP_MODE==SBUMP_FLAT
      nrm=TransformDir(nrm, bone, weight_h);
   #endif
   }

   // normalize (have to do all at the same time, so all have the same lengths)
   if(BUMP_MODE>SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || VTX_REFLECT // per-vertex reflection
   || !PER_PIXEL && BUMP_MODE>=SBUMP_FLAT // per-vertex lighting
   || TESSELATE) // needed for tesselation
   {
                              nrm=Normalize(nrm);
      if(BUMP_MODE>SBUMP_FLAT)tan=Normalize(tan);
   }

   Flt dist=Length(pos);

#if VTX_REFLECT
   O.rfl=Transform3(reflect(VecH(pos/dist), nrm), CamMatrix);
#endif

#if   BUMP_MODE> SBUMP_FLAT && PER_PIXEL
   O.mtrx[0]=tan;
   O.mtrx[2]=nrm;
   O.mtrx[1]=vtx.bin(nrm, tan, HEIGHTMAP);
#elif BUMP_MODE==SBUMP_FLAT && PER_PIXEL
   O.nrm=nrm;
#endif

   // sky
   O.col.a*=(Half)Sat(dist*SkyFracMulAdd.x + SkyFracMulAdd.y);

   // fog
   Half fog_rev=    VisibleOpacity(FogDensity, dist); // fog_rev=1-fog
   O.col.rgb*=                              fog_rev ; //       *=1-fog
   O.col_add =Lerp(FogColor, Highlight.rgb, fog_rev); //         1-fog

   //  per-vertex light
   #if !PER_PIXEL && BUMP_MODE>=SBUMP_FLAT
   {
      Half d  =Sat(Dot(nrm, LightDir.dir));
      VecH lum=LightDir.color.rgb*d;
   #if SHADOW_MAPS
      O.lum=lum;
   #else
      O.col.rgb*=lum+AmbNSColor;
   #endif
   }
   #endif

   O_vtx=Project(pos);
#if SET_POS
   O.pos=pos;
#endif
}
/******************************************************************************/
// PS
/******************************************************************************/
void PS
(
   VS_PS I,
 //PIXEL,

#if PER_PIXEL && FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   IS_FRONT,
#endif

  out VecH4 outCol:TARGET0
#if USE_VEL
, out VecH4 outVel:TARGET1 // #BlendRT
#endif
) // #RTOutput
{
   VecH  nrm;
   Half  specular=MaterialSpecular();
   VecH4 tex_nrm; // #MaterialTextureLayout

#if   LAYOUT==0
   if(PER_PIXEL && BUMP_MODE>=SBUMP_FLAT)nrm=Normalize(I.Nrm());
#elif LAYOUT==1
   VecH4 tex_col=Tex(Col, I.tex); if(ALPHA_TEST)clip(tex_col.a-ALPHA_CLIP);
   if(ALPHA)I.col*=tex_col;else I.col.rgb*=tex_col.rgb;
   if(PER_PIXEL && BUMP_MODE>=SBUMP_FLAT)nrm=Normalize(I.Nrm());
#elif LAYOUT==2
   tex_nrm=Tex(Nrm, I.tex); if(ALPHA_TEST)clip(tex_nrm.a-ALPHA_CLIP); // FIXME
            specular *=tex_nrm.z;
   if(ALPHA)I.col.a  *=tex_nrm.a;
            I.col.rgb*=Tex(Col, I.tex).rgb;

   #if PER_PIXEL
      #if   BUMP_MODE==SBUMP_FLAT
         nrm=Normalize(I.Nrm());
      #elif BUMP_MODE> SBUMP_FLAT
         nrm.xy=tex_nrm.xy*Material.normal;
         nrm.z =CalcZ(nrm.xy);
         nrm   =Normalize(Transform(nrm, I.mtrx));
      #endif
   #endif
#endif

   // reflection
   #if REFLECT
   {
   #if VTX_REFLECT
      Vec rfl=I.rfl;
   #else
      Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
   #endif
      I.col.rgb+=TexCube(Rfl, rfl).rgb * ((LAYOUT==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
   }
   #endif

   // calculate lighting
#if BUMP_MODE>=SBUMP_FLAT && (PER_PIXEL || SHADOW_MAPS)
 //VecH total_specular=0;

   VecH total_lum=AmbNSColor;
 /*if(AmbMaterial)
   {
      if(LIGHT_MAP)total_lum+=Material.ambient*Tex(Lum, I.tex).rgb;
      else         total_lum+=Material.ambient;
   }*/

#if PER_PIXEL && FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   BackFlip(nrm, front);
#endif

   // directional light
   {
      // shadow
      Half shd;
   #if SHADOW_MAPS
      shd=Sat(ShadowDirValue(I.pos, 0, false, SHADOW_MAPS, false)); // for improved performance don't use shadow jittering because it's not much needed for blended objects
   #endif

      // diffuse
   #if PER_PIXEL
      VecH light_dir=LightDir.dir;
      Half lum      =LightDiffuse(nrm, light_dir); if(SHADOW_MAPS)lum*=shd;

      // specular
   /* don't use specular at all
      BRANCH if(lum*specular>EPS_LUM)
      {
         Vec eye_dir=Normalize    (-I.pos);
         Flt spec   =LightSpecular( nrm, specular, light_dir, eye_dir); if(SHADOW_MAPS)spec*=shd;
         total_specular+=LightDir.color.rgb*spec;
      }*/total_lum     +=LightDir.color.rgb*lum ;
   #else
      if(SHADOW_MAPS)I.lum*=shd;
      total_lum+=I.lum;
   #endif
   }

   // perform lighting
   I.col.rgb=I.col.rgb*total_lum;// + total_specular;
#endif
   I.col.rgb+=I.col_add; // add after lighting because this could have fog
   outCol=I.col;

#if USE_VEL
   outVel.xyz=GetVelocity_PS(I.vel, I.pos); outVel.w=I.col.a; // alpha needed because of blending
#endif
}
/******************************************************************************/
