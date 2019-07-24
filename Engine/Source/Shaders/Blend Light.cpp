/******************************************************************************/
#include "!Header.h"
#include "Fog.h"
#include "Sky.h"
/******************************************************************************/
#define ALPHA_CLIP 0.5
/******************************************************************************/
#define PARAMS              \
   uniform Bool skin       ,\
   uniform Bool color      ,\
   uniform Int  textures   ,\
   uniform Int  bump_mode  ,\
   uniform Bool alpha_test ,\
   uniform Bool alpha      ,\
   uniform Bool light_map  ,\
   uniform Bool rflct      ,\
   uniform Int  fx         ,\
   uniform Bool per_pixel  ,\
   uniform Int  shadow_maps
// when adding "Bool tesselate" here, then remove "Bool tesselate=false;" below
#define use_vel alpha_test
/******************************************************************************/
struct VS_PS
{
   Vec      pos    :TEXCOORD0;
   Vec2     tex    :TEXCOORD1;
   MatrixH3 mtrx   :TEXCOORD2; // !! may not be Normalized !!
   VecH     rfl    :TEXCOORD5;
   Vec      vel    :TEXCOORD6;
   VecH4    col    :COLOR0   ;
   VecH     col_add:COLOR1   ;
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
   Bool heightmap=false, tesselate=false;

   Vec  pos=vtx.pos();
   VecH nrm, tan; if(bump_mode>=SBUMP_FLAT)nrm=vtx.nrm(); if(bump_mode>SBUMP_FLAT)tan=vtx.tan(nrm, heightmap);

   if(textures   )O.tex =vtx.tex(heightmap);
                  O.col =MaterialColor();
   if(color      )O.col*=vtx.colorFast();
   if(fx==FX_LEAF)
   {
      if(bump_mode> SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm, tan);else
      if(bump_mode==SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm     );else
                               BendLeaf(vtx.hlp(), pos          );
   }
   if(fx==FX_LEAFS)
   {
      if(bump_mode> SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm, tan);else
      if(bump_mode==SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm     );else
                               BendLeafs(vtx.hlp(), vtx.size(), pos          );
   }

   if(!skin)
   {
      if(true) // instance
      {
                    O.pos=TransformPos(pos, vtx.instance());

         if(bump_mode>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm, vtx.instance());
         if(bump_mode> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan, vtx.instance());
         if(fx==FX_GRASS)
         {
              BendGrass(pos, O.pos, vtx.instance());
            O.col.a*=1-GrassFadeOut(vtx.instance());
         }
         if(use_vel){O.vel=ObjVel[vtx.instance()]; UpdateVelocities_VS(O.vel, pos, O.pos, vtx.instance());} // #PER_INSTANCE_VEL
      }else
      {
                    O.pos=TransformPos(pos);

         if(bump_mode>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm);
         if(bump_mode> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan);
         if(fx==FX_GRASS)
         {
            BendGrass(pos, O.pos);
            O.col.a*=1-GrassFadeOut();
         }
         if(use_vel){O.vel=ObjVel[0]; UpdateVelocities_VS(O.vel, pos, O.pos);}
      }
   }else
   {
      VecI bone=vtx.bone();
                                   O.pos=TransformPos(pos, bone, vtx.weight());
      if(bump_mode>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm, bone, vtx.weight());
      if(bump_mode> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan, bone, vtx.weight());
      if(use_vel){O.vel=GetBoneVel(bone, vtx.weight()); UpdateVelocities_VS(O.vel, pos, O.pos);}
   }

   // normalize (have to do all at the same time, so all have the same lengths)
   if(bump_mode>SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || rflct && !(per_pixel && bump_mode>SBUMP_FLAT) // per-vertex reflection
   || !per_pixel && bump_mode>=SBUMP_FLAT // per-vertex lighting
   || tesselate) // needed for tesselation
   {
                              O.mtrx[2]=Normalize(O.mtrx[2]);
      if(bump_mode>SBUMP_FLAT)O.mtrx[0]=Normalize(O.mtrx[0]);
   }

   if(bump_mode>SBUMP_FLAT)O.mtrx[1]=vtx.bin(O.mtrx[2], O.mtrx[0], heightmap);

   if(rflct && !(per_pixel && bump_mode>SBUMP_FLAT))O.rfl=Transform3(reflect((VecH)Normalize(O.pos), O.mtrx[2]), CamMatrix);

   Flt d=Length(O.pos);

   // sky
   O.col.a*=(Half)Sat(d*SkyFracMulAdd.x + SkyFracMulAdd.y);

   // fog
   Half fog_rev=       VisibleOpacity(FogDensity, d); // fog_rev=1-fog
   O.col.rgb*=                              fog_rev ; //       *=1-fog
   O.col_add =Lerp(FogColor, Highlight.rgb, fog_rev); //         1-fog

   //  per-vertex light
   if(!per_pixel && bump_mode>=SBUMP_FLAT)
   {
      Half d  =Sat(Dot(O.mtrx[2], Light_dir.dir));
      VecH lum=Light_dir.color.rgb*d + AmbNSColor;
      O.col.rgb*=lum;
   }

   O_vtx=Project(O.pos);
}
/******************************************************************************/
// PS
/******************************************************************************/
void PS
(
   VS_PS I,
 //PIXEL,
   IS_FRONT,

out VecH4 outCol:TARGET0,
out VecH4 outVel:TARGET1  // #BlendRT
)
{
   VecH  nrm;
   Half  specular=MaterialSpecular();
   VecH4 tex_nrm; // #MaterialTextureChannelOrder

   if(textures==0)
   {
      if(per_pixel && bump_mode>=SBUMP_FLAT)nrm=Normalize(I.mtrx[2]);
   }else
   if(textures==1)
   {
      VecH4 tex_col=Tex(Col, I.tex); if(alpha_test)clip(tex_col.a-ALPHA_CLIP);
      if(alpha)I.col*=tex_col;else I.col.rgb*=tex_col.rgb;
      if(per_pixel && bump_mode>=SBUMP_FLAT)nrm=Normalize(I.mtrx[2]);
   }else
   if(textures==2)
   {
      tex_nrm=Tex(Nrm, I.tex); if(alpha_test)clip(tex_nrm.a-ALPHA_CLIP);
               specular *=tex_nrm.z;
      if(alpha)I.col.a  *=tex_nrm.a;
               I.col.rgb*=Tex(Col, I.tex).rgb;

      if(per_pixel)
      {
         if(bump_mode==SBUMP_FLAT)nrm=Normalize(I.mtrx[2]);else
         if(bump_mode> SBUMP_FLAT)
         {
            nrm.xy=(tex_nrm.xy*2-1)*MaterialRough();
            nrm.z =CalcZ(nrm.xy);
            nrm   =Normalize(Transform(nrm, I.mtrx));
         }
      }
   }

   // reflection
   if(rflct)
   {
      if(per_pixel && bump_mode>SBUMP_FLAT)I.rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
      I.col.rgb+=TexCube(Rfl, I.rfl).rgb * ((textures==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
   }

   // calculate lighting
   if(per_pixel && bump_mode>=SBUMP_FLAT)
   {
    //VecH total_specular=0;

      VecH total_lum=AmbNSColor;
    //if(light_map)total_lum+=AmbMaterial*MaterialAmbient()*Tex(Lum, I.tex).rgb;
    //else         total_lum+=AmbMaterial*MaterialAmbient();

      if(fx!=FX_GRASS && fx!=FX_LEAF && fx!=FX_LEAFS)BackFlip(nrm, front);

      // directional light
      {
         // shadow
         Half shd; if(shadow_maps)shd=Sat(ShadowDirValue(I.pos, 0, false, shadow_maps, false)); // for improved performance don't use shadow jittering because it's not much needed for blended objects

         // diffuse
         VecH light_dir=Light_dir.dir;
         Half lum      =LightDiffuse(nrm, light_dir); if(shadow_maps)lum*=shd;

         // specular
      /* don't use specular at all
         BRANCH if(lum*specular>EPS_LUM)
         {
            Vec eye_dir=Normalize    (-I.pos);
            Flt spec   =LightSpecular( nrm, specular, light_dir, eye_dir); if(shadow_maps)spec*=shd;
            total_specular+=Light_dir.color.rgb*spec;
         }*/total_lum     +=Light_dir.color.rgb*lum ;
      }

      // perform lighting
      I.col.rgb=I.col.rgb*total_lum;// + total_specular;
   }
   I.col.rgb+=I.col_add; // add after lighting because this could have fog
   outCol=I.col;
   if(use_vel){UpdateVelocities_PS(I.vel, I.pos); outVel.xyz=I.vel; outVel.w=I.col.a;} // alpha needed because of blending
}
/******************************************************************************/
