/******************************************************************************/
#include "!Header.h"
#include "Fog.h"
#include "Sky.h"
/******************************************************************************/
// SKIN, COLORS, LAYOUT, BUMP_MODE, ALPHA_TEST, ALPHA, REFLECT, LIGHT_MAP, FX, PER_PIXEL, SHADOW_MAPS, TESSELATE
#define NO_AMBIENT  0 // this could be set to 1 for Secondary Passes, if we would use this then we could remove 'FirstPass'
#define HAS_AMBIENT (!NO_AMBIENT)

#define HEIGHTMAP      0
#define LIGHT          1
#define SHADOW         (SHADOW_MAPS>0)
#define VTX_LIGHT      (LIGHT && !PER_PIXEL)
#define AMBIENT_IN_VTX (VTX_LIGHT && !SHADOW && !LIGHT_MAP) // if stored per-vertex (in either 'vtx.col' or 'vtx.lum')
#define LIGHT_IN_COL   (VTX_LIGHT && !DETAIL && (NO_AMBIENT || !SHADOW) && !REFLECT) // can't mix light with vtx.col when REFLECT because for reflections we need unlit color
#define FOG_IN_COL     (!REFLECT) // can't mix fog with vtx.col when REFLECT because for reflections we need unlit color
#define USE_VEL        ALPHA_TEST
#define SET_POS        ((LIGHT && PER_PIXEL) || USE_VEL || SHADOW || REFLECT || TESSELATE)
#define SET_TEX        (LAYOUT || DETAIL || LIGHT_MAP || BUMP_MODE>SBUMP_FLAT)
#define SET_LUM        (VTX_LIGHT && !LIGHT_IN_COL)
#define SET_FOG        (!FOG_IN_COL)
#define VTX_REFLECT    (REFLECT && BUMP_MODE<=SBUMP_FLAT)
#define PIXEL_NORMAL   ((PER_PIXEL && LIGHT) || REFLECT) // if calculate normal in the pixel shader
#define GRASS_FADE     (FX==FX_GRASS_2D || FX==FX_GRASS_3D)
#define ALPHA_CLIP     0.5
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
#if SET_FOG
   Half  fog_rev:FOG;
#endif

#if   BUMP_MODE> SBUMP_FLAT && PIXEL_NORMAL
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE>=SBUMP_FLAT && (PIXEL_NORMAL || TESSELATE)
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif

#if USE_VEL
   Vec vel:VELOCITY;
#endif

#if VTX_REFLECT
   Vec reflect_dir:REFLECTION;
#endif

#if SET_LUM
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

   Vec local_pos; if(USE_VEL || FX==FX_GRASS_2D || FX==FX_GRASS_3D)local_pos=pos;
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

         if(FX==FX_GRASS_2D || FX==FX_GRASS_3D)BendGrass(local_pos, pos, vtx.instance());
         if(GRASS_FADE                        )  O.col.a*=1-GrassFadeOut(vtx.instance());
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

         if(FX==FX_GRASS_2D || FX==FX_GRASS_3D)BendGrass(local_pos, pos);
         if(GRASS_FADE                        )O.col.a*=1-GrassFadeOut();
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
   || VTX_LIGHT   // per-vertex lighting
   || TESSELATE)  // needed for tesselation
   {
                              nrm=Normalize(nrm);
      if(BUMP_MODE>SBUMP_FLAT)tan=Normalize(tan);
   }

#if   BUMP_MODE> SBUMP_FLAT && PIXEL_NORMAL
   O.mtrx[0]=tan;
   O.mtrx[2]=nrm;
   O.mtrx[1]=vtx.bin(nrm, tan, HEIGHTMAP);
#elif BUMP_MODE>=SBUMP_FLAT && (PIXEL_NORMAL || TESSELATE)
   O.nrm=nrm;
#endif

   Flt dist=Length(pos);

#if VTX_REFLECT
   O.reflect_dir=ReflectDir(pos/dist, nrm);
#endif

   // sky
   O.col.a*=Sat(dist*SkyFracMulAdd.x + SkyFracMulAdd.y);

   // fog
   Half fog_rev=    VisibleOpacity(FogDensity, dist); // fog_rev=1-fog
   O.col_add =Lerp(FogColor, Highlight.rgb, fog_rev); //         1-fog
#if FOG_IN_COL
   O.col.rgb*=                              fog_rev ; //       *=1-fog
#else
   O.fog_rev =fog_rev;
#endif

   //  per-vertex light
   #if VTX_LIGHT
   {
      VecH total_lum;

      // AMBIENT
      if(HAS_AMBIENT && AMBIENT_IN_VTX)
      {
         total_lum=AmbientNSColor;
         /*if(MATERIALS<=1 && FirstPass)*/total_lum+=Material.ambient;
      }else total_lum=0;

      // LIGHTS
      total_lum+=LightDir.color.rgb*Sat(Dot(nrm, LightDir.dir));

      // STORE
      #if LIGHT_IN_COL
         O.col.rgb*=total_lum;
      #else
         O.lum=total_lum;
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

#if PIXEL_NORMAL && FX!=FX_GRASS_2D
   IS_FRONT,
#endif

  out VecH4 outCol  :TARGET0
#if USE_VEL
, out VecH4 outVel  :TARGET1
#endif
, out Half  outAlpha:TARGET2 // #RTOutput.Blend
) // #RTOutput
{
   Half smooth, reflectivity;

   // #MaterialTextureLayout
#if   LAYOUT==0
   smooth      =Material.smooth;
   reflectivity=Material.reflect;
#elif LAYOUT==1
   VecH4 tex_col=Tex(Col, I.tex); if(ALPHA_TEST)clip(tex_col.a-ALPHA_CLIP);
   if(ALPHA)I.col*=tex_col;else I.col.rgb*=tex_col.rgb;
   smooth      =Material.smooth;
   reflectivity=Material.reflect;
#elif LAYOUT==2
   VecH4 tex_ext=Tex(Ext, I.tex); if(ALPHA_TEST)clip(tex_ext.a-ALPHA_CLIP);
            I.col.rgb*=Tex(Col, I.tex).rgb;
   if(ALPHA)I.col.a  *=tex_ext.a;
   smooth      =Material.smooth *tex_ext.x;
   reflectivity=Material.reflect*tex_ext.y;
#endif

   // normal
#if PIXEL_NORMAL
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
#endif

#if PIXEL_NORMAL && FX!=FX_GRASS_2D
   BackFlip(nrm, front);
#endif

#if (LIGHT && PER_PIXEL) || REFLECT
   Vec eye_dir=Normalize(I.pos);
#endif

   Bool translucent=(FX==FX_GRASS_2D || FX==FX_GRASS_3D || FX==FX_LEAF || FX==FX_LEAFS);

   // lighting
   VecH ambient;
   if(HAS_AMBIENT && !AMBIENT_IN_VTX)
   {
      ambient=AmbientNSColor;
      /*if(MATERIALS<=1 && FirstPass)*/
      {
      #if LIGHT_MAP
         ambient+=Material.ambient*Tex(Lum, I.tex).rgb;
      #else
         ambient+=Material.ambient;
      #endif
      }
   }else ambient=0;

   VecH total_lum,
        total_specular=0;

   #if VTX_LIGHT // per-vertex
   {
      Half shadow;
   #if SHADOW
      shadow=Sat(ShadowDirValue(I.pos, 0, false, SHADOW_MAPS, false)); // for improved performance don't use shadow jittering because it's not much needed for blended objects
   #else
      shadow=1;
   #endif

      VecH light;
   #if SET_LUM
      light=I.lum;
   #else
      light=1;
   #endif

      total_lum=light*shadow+ambient;
   }
   #else // per-pixel
   {
      total_lum=ambient;

      // directional light
      {
         // shadow
         Half shadow; if(SHADOW)shadow=Sat(ShadowDirValue(I.pos, 0, false, SHADOW_MAPS, false));

         // light
         VecH light_dir=LightDir.dir;
         LightParams lp; lp.set(nrm, light_dir);
         Half lum=lp.NdotL; if(SHADOW)lum*=shadow;
         if(translucent && -lum>EPS_LUM)
         {
            total_lum+=LightDir.color.rgb*(lum*-TRANSLUCENT_VAL);
         }
         BRANCH if(lum>EPS_LUM)
         {
            // light #1
            lp.set(nrm, light_dir, eye_dir);
            
            // specular
            Half specular=lp.specular(smooth, reflectivity, false)*lum;

            // diffuse !! after specular because it adjusts 'lum' !!
            lum*=lp.diffuse(smooth);

            total_lum     +=LightDir.color.rgb*lum     ;
            total_specular+=LightDir.color.rgb*specular;
         }
      }
   }
   #endif

   // reflection
   #if REFLECT
   {
   #if VTX_REFLECT
      Vec reflect_dir=I.reflect_dir;
   #else
      Vec reflect_dir=ReflectDir(eye_dir, nrm);
   #endif
      I.col.rgb=PBR1(I.col.rgb, I.col.rgb*total_lum, smooth, reflectivity, total_specular, -Dot(nrm, eye_dir), reflect_dir, false);
   }
   #else
      I.col.rgb=I.col.rgb*total_lum + total_specular*ReflectCol(I.col.rgb, reflectivity);
   #endif

#if SET_FOG
   I.col.rgb*=I.fog_rev;
#endif
   I.col.rgb+=I.col_add; // add after lighting and reflection because this could have fog

   outCol  =I.col;
   outAlpha=I.col.a;

#if USE_VEL
   outVel.xyz=GetVelocity_PS(I.vel, I.pos); outVel.w=I.col.a; // alpha needed because of blending
#endif
}
/******************************************************************************/
