/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, MATERIALS, LAYOUT, BUMP_MODE, ALPHA_TEST, REFLECT, LIGHT_MAP, DETAIL, COLORS, MTRL_BLEND, HEIGHTMAP, FX, PER_PIXEL
LIGHT_DIR, LIGHT_DIR_SHD, LIGHT_DIR_SHD_NUM
LIGHT_POINT, LIGHT_POINT_SHD
LIGHT_LINEAR, LIGHT_LINEAR_SHD
LIGHT_CONE, LIGHT_CONE_SHD
TESSELATE

FinalLight = Ambient + Light*Shadow
Final = (TexCol*MtrlCol*VtxCol+Detail)*FinalLight
/******************************************************************************/
#define NO_AMBIENT  0 // this could be set to 1 for Secondary Passes, if we would use this then we could remove 'FirstPass'
#define HAS_AMBIENT (!NO_AMBIENT)

#define LIGHT          (LIGHT_DIR || LIGHT_POINT || LIGHT_LINEAR || LIGHT_CONE)
#define SHADOW         (LIGHT_DIR_SHD || LIGHT_POINT_SHD || LIGHT_LINEAR_SHD || LIGHT_CONE_SHD)
#define VTX_LIGHT      (LIGHT && !PER_PIXEL)
#define AMBIENT_IN_VTX (VTX_LIGHT && !SHADOW && !LIGHT_MAP) // if stored in 'vtx.col' or 'vtx.lum'
#define LIGHT_IN_COL   (VTX_LIGHT && !DETAIL && (NO_AMBIENT || !SHADOW) && !REFLECT) // can't mix light with vtx.col when REFLECT because for reflections we need unlit color
#define SET_POS        ((LIGHT && PER_PIXEL) || SHADOW || REFLECT || TESSELATE)
#define SET_TEX        (LAYOUT || DETAIL || LIGHT_MAP || BUMP_MODE>SBUMP_FLAT)
#define SET_COL        (COLORS || LIGHT_IN_COL)
#define SET_LUM        (VTX_LIGHT && !LIGHT_IN_COL)
#define VTX_REFLECT    (REFLECT && BUMP_MODE<=SBUMP_FLAT)
#define PIXEL_NORMAL   ((PER_PIXEL && LIGHT) || REFLECT) // if calculate normal in the pixel shader
#define GRASS_FADE     (FX==FX_GRASS)
/******************************************************************************/
struct VS_PS
{
#if SET_POS
   Vec pos:POS;
#endif

#if SET_TEX
   Vec2 tex:TEXCOORD;
#endif

#if   BUMP_MODE> SBUMP_FLAT && PIXEL_NORMAL
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE==SBUMP_FLAT && (PIXEL_NORMAL || TESSELATE)
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif

#if MATERIALS>1
   VecH4 material:MATERIAL;
#endif

#if SET_COL
   VecH col:COLOR;
#endif

#if VTX_REFLECT
   VecH rfl:REFLECTION;
#endif

#if GRASS_FADE
   Half fade_out:FADE_OUT;
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
   out Vec4  O_vtx:POSITION,

   CLIP_DIST
)
{
   Vec  pos=vtx.pos();
   VecH nrm, tan; if(BUMP_MODE>=SBUMP_FLAT)nrm=vtx.nrm(); if(BUMP_MODE>SBUMP_FLAT)tan=vtx.tan(nrm, HEIGHTMAP);

#if SET_TEX
   O.tex=vtx.tex(HEIGHTMAP);
   if(HEIGHTMAP && MATERIALS==1)O.tex*=Material.tex_scale;
#endif

#if MATERIALS>1
   O.material=vtx.material();
#endif

#if COLORS
   if(MATERIALS<=1)O.col=vtx.colorFast3()*Material.color.rgb;
   else            O.col=vtx.colorFast3();
#endif

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

   Vec local_pos; if(FX==FX_GRASS)local_pos=pos;
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

         if(FX==FX_GRASS)BendGrass(local_pos, pos, vtx.instance());
      #if GRASS_FADE
         O.fade_out=GrassFadeOut(vtx.instance());
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

         if(FX==FX_GRASS)BendGrass(local_pos, pos);
      #if GRASS_FADE
         O.fade_out=GrassFadeOut();
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
#elif BUMP_MODE==SBUMP_FLAT && (PIXEL_NORMAL || TESSELATE)
   O.nrm=nrm;
#endif

#if VTX_REFLECT
   O.rfl=ReflectDir(Normalize(pos), nrm);
#endif

   //  per-vertex light
   #if VTX_LIGHT
   {
      VecH total_lum;

      // AMBIENT
      if(HAS_AMBIENT && AMBIENT_IN_VTX)
      {
         total_lum=AmbientNSColor;
         if(MATERIALS<=1 && FirstPass)total_lum+=Material.ambient;
      }else total_lum=0;

      // LIGHTS
      #if LIGHT_DIR
      {
         // diffuse
         VecH light_dir=LightDir.dir;
         Half lum      =LightDiffuse(nrm, light_dir);

         total_lum+=LightDir.color.rgb*lum;
      }
      #endif

      #if LIGHT_POINT
      {
         // distance
         Vec  delta=LightPoint.pos-pos; Flt inv_dist2=1/Length2(delta);
         Half power=LightPointDist(inv_dist2);

         // diffuse
         VecH light_dir=delta*Sqrt(inv_dist2); // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         total_lum+=LightPoint.color.rgb*(lum*power);
      }
      #endif

      #if LIGHT_LINEAR
      {
         // distance
         Vec  delta=LightLinear.pos-pos; Flt dist=Length(delta);
         Half power=LightLinearDist(dist);

         // diffuse
         VecH light_dir=delta/dist; // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         total_lum+=LightLinear.color.rgb*(lum*power);
      }
      #endif

      #if LIGHT_CONE
      {
         // distance & angle
         Vec  delta=LightCone.pos-pos; Flt dist=Length(delta);
         Vec  dir  =TransformTP(delta, LightCone.mtrx); dir.xy/=dir.z;
         Half power=LightConeAngle(dir.xy)*LightConeDist(dist); power*=(dir.z>0);

         // diffuse
         VecH light_dir=delta/dist; // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         total_lum+=LightCone.color.rgb*(lum*power);
      }
      #endif

      // STORE
      #if LIGHT_IN_COL
         if(COLORS      )O.col*=total_lum                   ;else
         if(MATERIALS<=1)O.col =total_lum*Material.color.rgb;else
                         O.col =total_lum                   ;
      #else
         O.lum=total_lum;
      #endif
   }
   #endif

   O_vtx=Project(pos); CLIP_PLANE(pos);
#if SET_POS
   O.pos=pos;
#endif
}
/******************************************************************************/
// PS
/******************************************************************************/
VecH4 PS
(
   VS_PS I,
   PIXEL

#if PIXEL_NORMAL && FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
 , IS_FRONT
#endif

):TARGET
{
   VecH col;
#if PIXEL_NORMAL
   VecH nrm;
#endif
   Half glow, smooth, reflectivity;

#if SET_COL
   col=I.col;
#else
   if(MATERIALS<=1)col=Material.color.rgb;
#endif

#if MATERIALS==1
   VecH det;
#if DETAIL
   det=GetDetail(I.tex);
#endif

   // #MaterialTextureLayout
   #if LAYOUT==0
   {
      if(DETAIL)col+=det.z;
      glow        =Material.glow;
      smooth      =Material.smooth;
      reflectivity=Material.reflect;
   }
   #elif LAYOUT==1
   {
      VecH4 tex_col=Tex(Col, I.tex);
      if(ALPHA_TEST)
      {
      #if GRASS_FADE
         tex_col.a-=I.fade_out;
      #endif
         AlphaTest(tex_col.a);
      }
      col        *=tex_col.rgb; if(DETAIL)col+=det.z;
      glow        =Material.glow;
      smooth      =Material.smooth;
      reflectivity=Material.reflect;
   }
   #elif LAYOUT==2
   {
      VecH4 tex_ext=Tex(Ext, I.tex);
      if(ALPHA_TEST)
      {
      #if GRASS_FADE
         tex_ext.w-=I.fade_out;
      #endif
         AlphaTest(tex_ext.w);
      }
      VecH4 tex_col=Tex(Col, I.tex);
      col        *=tex_col.rgb; if(DETAIL)col+=det.z;
      glow        =Material.glow   *tex_col.w;
      smooth      =Material.smooth *tex_ext.x;
      reflectivity=Material.reflect*tex_ext.y;
   }
   #endif

   // normal
   #if PIXEL_NORMAL
      #if   BUMP_MODE==SBUMP_ZERO
         nrm=0;
      #elif BUMP_MODE==SBUMP_FLAT
                    nrm    =Normalize(I.Nrm());
         if(DETAIL){nrm.xy+=det.xy; nrm=Normalize(nrm);}
      #else
                   nrm.xy =Tex(Nrm, I.tex).xy*Material.normal;
         if(DETAIL)nrm.xy+=det.xy;
                   nrm.z  =CalcZ(nrm.xy);
                   nrm    =Normalize(Transform(nrm, I.mtrx));
      #endif
   #endif

#else // MATERIALS>1
   // assuming that in multi materials LAYOUT!=0
   Vec2 tex0, tex1, tex2, tex3;
                   tex0=I.tex*MultiMaterial0.tex_scale;
                   tex1=I.tex*MultiMaterial1.tex_scale;
   if(MATERIALS>=3)tex2=I.tex*MultiMaterial2.tex_scale;
   if(MATERIALS>=4)tex3=I.tex*MultiMaterial3.tex_scale;

   // #MaterialTextureLayout

   // detail texture
   VecH det0, det1, det2, det3;
   if(DETAIL)
   {
                      det0=Tex(Det , tex0*MultiMaterial0.det_scale).xyz*MultiMaterial0.det_mul+MultiMaterial0.det_add;
                      det1=Tex(Det1, tex1*MultiMaterial1.det_scale).xyz*MultiMaterial1.det_mul+MultiMaterial1.det_add;
      if(MATERIALS>=3)det2=Tex(Det2, tex2*MultiMaterial2.det_scale).xyz*MultiMaterial2.det_mul+MultiMaterial2.det_add;
      if(MATERIALS>=4)det3=Tex(Det3, tex3*MultiMaterial3.det_scale).xyz*MultiMaterial3.det_mul+MultiMaterial3.det_add;
   }

   // macro texture
   //Half mac_blend; if(MACRO)mac_blend=LerpRS(MacroFrom, MacroTo, Length(I.pos))*MacroMax;

   // Smooth, Reflect, Bump, Alpha !! DO THIS FIRST because it may modify 'I.material' which affects everything !!
   VecH2 sr; // smooth_reflect
   if(LAYOUT==2)
   {
      VecH ext0, ext1, ext2, ext3;
                      ext0=Tex(Ext , tex0).xyz; // we need only smooth,reflect,bump
                      ext1=Tex(Ext1, tex1).xyz;
      if(MATERIALS>=3)ext2=Tex(Ext2, tex2).xyz;
      if(MATERIALS>=4)ext3=Tex(Ext3, tex3).xyz;
      if(MTRL_BLEND)
      {
                          I.material.x=MultiMaterialWeight(I.material.x, ext0.BUMP_CHANNEL);
                          I.material.y=MultiMaterialWeight(I.material.y, ext1.BUMP_CHANNEL); if(MATERIALS==2)I.material.xy  /=I.material.x+I.material.y;
         if(MATERIALS>=3){I.material.z=MultiMaterialWeight(I.material.z, ext2.BUMP_CHANNEL); if(MATERIALS==3)I.material.xyz /=I.material.x+I.material.y+I.material.z;}
         if(MATERIALS>=4){I.material.w=MultiMaterialWeight(I.material.w, ext3.BUMP_CHANNEL); if(MATERIALS==4)I.material.xyzw/=I.material.x+I.material.y+I.material.z+I.material.w;}
      }
                      sr =(ext0.xy*MultiMaterial0.base2_mul+MultiMaterial0.base2_add)*I.material.x
                        + (ext1.xy*MultiMaterial1.base2_mul+MultiMaterial1.base2_add)*I.material.y;
      if(MATERIALS>=3)sr+=(ext2.xy*MultiMaterial2.base2_mul+MultiMaterial2.base2_add)*I.material.z;
      if(MATERIALS>=4)sr+=(ext3.xy*MultiMaterial3.base2_mul+MultiMaterial3.base2_add)*I.material.w;
   }else
   {
                      sr =MultiMaterial0.base2_add*I.material.x
                        + MultiMaterial1.base2_add*I.material.y;
      if(MATERIALS>=3)sr+=MultiMaterial2.base2_add*I.material.z;
      if(MATERIALS>=4)sr+=MultiMaterial3.base2_add*I.material.w;
   }
   smooth      =sr.x;
   reflectivity=sr.y;

   // Color + Detail + Macro + Glow !! do this second after modifying 'I.material' !!
   VecH4 rgb_glow;
                   {VecH4 col0=Tex(Col , tex0); col0.rgb*=MultiMaterial0.color.rgb; if(LAYOUT==2)col0.a*=MultiMaterial0.glow;else col0.a=MultiMaterial0.glow; if(DETAIL)col0.rgb+=det0.z; /*if(MACRO)col0.rgb=Lerp(col0.rgb, Tex(Mac , tex0*MacroScale).rgb, MultiMaterial0.macro*mac_blend);*/ rgb_glow =I.material.x*col0;}
                   {VecH4 col1=Tex(Col1, tex1); col1.rgb*=MultiMaterial1.color.rgb; if(LAYOUT==2)col1.a*=MultiMaterial1.glow;else col1.a=MultiMaterial1.glow; if(DETAIL)col1.rgb+=det1.z; /*if(MACRO)col1.rgb=Lerp(col1.rgb, Tex(Mac1, tex1*MacroScale).rgb, MultiMaterial1.macro*mac_blend);*/ rgb_glow+=I.material.y*col1;}
   if(MATERIALS>=3){VecH4 col2=Tex(Col2, tex2); col2.rgb*=MultiMaterial2.color.rgb; if(LAYOUT==2)col2.a*=MultiMaterial2.glow;else col2.a=MultiMaterial2.glow; if(DETAIL)col2.rgb+=det2.z; /*if(MACRO)col2.rgb=Lerp(col2.rgb, Tex(Mac2, tex2*MacroScale).rgb, MultiMaterial2.macro*mac_blend);*/ rgb_glow+=I.material.z*col2;}
   if(MATERIALS>=4){VecH4 col3=Tex(Col3, tex3); col3.rgb*=MultiMaterial3.color.rgb; if(LAYOUT==2)col3.a*=MultiMaterial3.glow;else col3.a=MultiMaterial3.glow; if(DETAIL)col3.rgb+=det3.z; /*if(MACRO)col3.rgb=Lerp(col3.rgb, Tex(Mac3, tex3*MacroScale).rgb, MultiMaterial3.macro*mac_blend);*/ rgb_glow+=I.material.w*col3;}
#if SET_COL
   col*=rgb_glow.rgb;
#else
   col =rgb_glow.rgb;
#endif
   glow=rgb_glow.w;

   // normal
   #if PIXEL_NORMAL
      #if   BUMP_MODE==SBUMP_ZERO
         nrm=0;
      #elif BUMP_MODE==SBUMP_FLAT
         nrm=Normalize(I.Nrm());
         if(DETAIL)
         {
                            nrm.xy+=det0.xy*I.material.x;
                            nrm.xy+=det1.xy*I.material.y;
            if(MATERIALS>=3)nrm.xy+=det2.xy*I.material.z;
            if(MATERIALS>=4)nrm.xy+=det3.xy*I.material.w;
            nrm=Normalize(nrm);
         }
      #else
         if(DETAIL)
         {
                            nrm.xy =(Tex(Nrm , tex0).xy*MultiMaterial0.normal + det0.xy)*I.material.x;
                                  + (Tex(Nrm1, tex1).xy*MultiMaterial1.normal + det1.xy)*I.material.y;
            if(MATERIALS>=3)nrm.xy+=(Tex(Nrm2, tex2).xy*MultiMaterial2.normal + det2.xy)*I.material.z;
            if(MATERIALS>=4)nrm.xy+=(Tex(Nrm3, tex3).xy*MultiMaterial3.normal + det3.xy)*I.material.w;
         }else
         {
                            nrm.xy =Tex(Nrm , tex0).xy*(MultiMaterial0.normal*I.material.x);
                                  + Tex(Nrm1, tex1).xy*(MultiMaterial1.normal*I.material.y);
            if(MATERIALS>=3)nrm.xy+=Tex(Nrm2, tex2).xy*(MultiMaterial2.normal*I.material.z);
            if(MATERIALS>=4)nrm.xy+=Tex(Nrm3, tex3).xy*(MultiMaterial3.normal*I.material.w);
         }
         nrm.z=CalcZ(nrm.xy);
         nrm  =Normalize(Transform(nrm, I.mtrx));
      #endif
   #endif

#endif // MATERIALS

   col+=Highlight.rgb;

#if PIXEL_NORMAL && FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   BackFlip(nrm, front);
#endif

   Vec2 jitter_value; if(SHADOW)jitter_value=ShadowJitter(pixel.xy);

   VecH ambient;
   if(HAS_AMBIENT && !AMBIENT_IN_VTX)
   {
      ambient=AmbientNSColor;
      if(MATERIALS<=1 && FirstPass)
      {
      #if LIGHT_MAP
         ambient+=Material.ambient*Tex(Lum, I.tex).rgb;
      #else
         ambient+=Material.ambient;
      #endif
      }
   }else ambient=0;

   VecH total_specular;

   // lighting
   #if VTX_LIGHT // per-vertex
   {
      Half shadow;
   #if SHADOW
      if(LIGHT_DIR_SHD   )shadow=Sat        (ShadowDirValue  (I.pos, jitter_value, true, LIGHT_DIR_SHD_NUM, false));
      if(LIGHT_POINT_SHD )shadow=ShadowFinal(ShadowPointValue(I.pos, jitter_value, true));
      if(LIGHT_LINEAR_SHD)shadow=ShadowFinal(ShadowPointValue(I.pos, jitter_value, true));
      if(LIGHT_CONE_SHD  )shadow=ShadowFinal(ShadowConeValue (I.pos, jitter_value, true));
   #else
      shadow=1;
   #endif

      VecH light;
   #if SET_LUM
      light=I.lum;
   #else
      light=1;
   #endif

      col*=light*shadow+ambient;
   }
   #else // per-pixel
   {
      VecH total_lum=ambient;
           total_specular=0;

      #if LIGHT_DIR
      {
         // shadow
         Half shadow; if(LIGHT_DIR_SHD)shadow=Sat(ShadowDirValue(I.pos, jitter_value, true, LIGHT_DIR_SHD_NUM, false));

         // diffuse
         VecH light_dir=LightDir.dir;
         Half lum      =LightDiffuse(nrm, light_dir); if(LIGHT_DIR_SHD)lum*=shadow;

         // specular
         BRANCH if(lum*smooth>EPS_LUM)
         {
            VecH eye_dir=Normalize    (I.pos);
            Half spec   =LightSpecular(nrm, smooth, light_dir, eye_dir); if(LIGHT_DIR_SHD)spec*=shadow;
            total_specular+=LightDir.color.rgb*spec;
         }  total_lum     +=LightDir.color.rgb*lum ;
      }
      #endif

      #if LIGHT_POINT
      {
         // shadow
         Half shadow; if(LIGHT_POINT_SHD)shadow=ShadowFinal(ShadowPointValue(I.pos, jitter_value, true));

         // distance
         Vec  delta=LightPoint.pos-I.pos; Flt inv_dist2=1/Length2(delta);
         Half power=LightPointDist(inv_dist2); if(LIGHT_POINT_SHD)power*=shadow;

         // diffuse
         VecH light_dir=delta*Sqrt(inv_dist2); // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         // specular
         BRANCH if(lum*smooth>EPS_LUM)
         {
            VecH eye_dir=Normalize    (I.pos);
            Half spec   =LightSpecular(nrm, smooth, light_dir, eye_dir);
            total_specular+=LightPoint.color.rgb*(spec*power);
         }  total_lum     +=LightPoint.color.rgb*(lum *power);
      }
      #endif

      #if LIGHT_LINEAR
      {
         // shadow
         Half shadow; if(LIGHT_LINEAR_SHD)shadow=ShadowFinal(ShadowPointValue(I.pos, jitter_value, true));

         // distance
         Vec  delta=LightLinear.pos-I.pos; Flt dist=Length(delta);
         Half power=LightLinearDist(dist); if(LIGHT_LINEAR_SHD)power*=shadow;

         // diffuse
         VecH light_dir=delta/dist; // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         // specular
         BRANCH if(lum*smooth>EPS_LUM)
         {
            VecH eye_dir=Normalize    (I.pos);
            Half spec   =LightSpecular(nrm, smooth, light_dir, eye_dir);
            total_specular+=LightLinear.color.rgb*(spec*power);
         }  total_lum     +=LightLinear.color.rgb*(lum *power);
      }
      #endif

      #if LIGHT_CONE
      {
         // shadow
         Half shadow; if(LIGHT_CONE_SHD)shadow=ShadowFinal(ShadowConeValue(I.pos, jitter_value, true));

         // distance & angle
         Vec  delta=LightCone.pos-I.pos; Flt dist=Length(delta);
         Vec  dir  =TransformTP(delta, LightCone.mtrx); dir.xy/=dir.z; // clip(Vec(1-Abs(dir.xy), dir.z));
         Half power=LightConeAngle(dir.xy)*LightConeDist(dist); if(LIGHT_CONE_SHD)power*=shadow; power*=(dir.z>0);

         // diffuse
         VecH light_dir=delta/dist; // Normalize(delta);
         Half lum      =LightDiffuse(nrm, light_dir);

         // specular
         BRANCH if(lum*smooth>EPS_LUM)
         {
            VecH eye_dir=Normalize    (I.pos);
            Half spec   =LightSpecular(nrm, smooth, light_dir, eye_dir);
            total_specular+=LightCone.color.rgb*(spec*power);
         }  total_lum     +=LightCone.color.rgb*(lum *power);
      }
      #endif

      col*=total_lum;
   }
   #endif

   // reflection
   #if REFLECT
   {
      if(FirstPass)
      {
      #if VTX_REFLECT
         Vec rfl=I.rfl;
      #else
         Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
      #endif
         col=Lerp(col, TexCube(Env, rfl).rgb*EnvColor, reflectivity);
      }else col*=1-reflectivity;
   }
   #endif

   #if !VTX_LIGHT
      col+=total_specular;
   #endif

   return VecH4(col, glow);
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if TESSELATE
HSData HSConstant(InputPatch<VS_PS,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm());}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
VS_PS HS
(
   InputPatch<VS_PS,3> I, UInt cp_id:SV_OutputControlPointID
)
{
   VS_PS O;
   O.pos=I[cp_id].pos;

#if   BUMP_MODE> SBUMP_FLAT
   O.mtrx=I[cp_id].mtrx;
#elif BUMP_MODE==SBUMP_FLAT
   O.nrm =I[cp_id].nrm;
#endif


#if SET_TEX
   O.tex=I[cp_id].tex;
#endif

#if MATERIALS>1
   O.material=I[cp_id].material;
#endif

#if SET_COL
   O.col=I[cp_id].col;
#endif

#if GRASS_FADE
   O.fade_out=I[cp_id].fade_out;
#endif

#if VTX_REFLECT
   O.rfl=I[cp_id].rfl;
#endif

#if SET_LUM
   O.lum=I[cp_id].lum;
#endif

   return O;
}
/******************************************************************************/
[domain("tri")]
void DS
(
   HSData hs_data, const OutputPatch<VS_PS,3> I, Vec B:SV_DomainLocation,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
#if   BUMP_MODE> SBUMP_FLAT && PIXEL_NORMAL
   O.mtrx[0]=I[0].mtrx[0]*B.z + I[1].mtrx[0]*B.x + I[2].mtrx[0]*B.y;
   O.mtrx[1]=I[0].mtrx[1]*B.z + I[1].mtrx[1]*B.x + I[2].mtrx[1]*B.y;
   SetDSPosNrm(O.pos, O.mtrx[2], I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#elif BUMP_MODE==SBUMP_FLAT
   SetDSPosNrm(O.pos, O.nrm    , I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#endif

#if SET_TEX
   O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;
#endif

#if MATERIALS>1
   O.material=I[0].material*B.z + I[1].material*B.x + I[2].material*B.y;
#endif

#if SET_COL
   O.col=I[0].col*B.z + I[1].col*B.x + I[2].col*B.y;
#endif

#if GRASS_FADE
   O.fade_out=I[0].fade_out*B.z + I[1].fade_out*B.x + I[2].fade_out*B.y;
#endif

#if VTX_REFLECT
   O.rfl=I[0].rfl*B.z + I[1].rfl*B.x + I[2].rfl*B.y;
#endif

#if SET_LUM
   O.lum=I[0].lum*B.z + I[1].lum*B.x + I[2].lum*B.y;
#endif

   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
