/******************************************************************************/
#include "!Header.h"
/******************************************************************************
SKIN, MATERIALS, TEXTURES, BUMP_MODE, ALPHA_TEST, LIGHT_MAP, DETAIL, REFLECT, COLORS, MTRL_BLEND, HEIGHTMAP, FX, PER_PIXEL
LIGHT_DIR, LIGHT_DIR_SHD, LIGHT_DIR_SHD_NUM
LIGHT_POINT, LIGHT_POINT_SHD
LIGHT_LINEAR, LIGHT_LINEAR_SHD
LIGHT_CONE, LIGHT_CONE_SHD
TESSELATE
/******************************************************************************/
#define LIGHT          (LIGHT_DIR || LIGHT_POINT || LIGHT_LINEAR || LIGHT_CONE)
#define SHADOW         (LIGHT_DIR_SHD || LIGHT_POINT_SHD || LIGHT_LINEAR_SHD || LIGHT_CONE_SHD)
#define SECONDARY_PASS (LIGHT_POINT || LIGHT_LINEAR || LIGHT_CONE) // local lights are enabled only for secondary shader passes
#define SET_POS        ((LIGHT && PER_PIXEL) || SHADOW || (REFLECT && PER_PIXEL && BUMP_MODE>SBUMP_FLAT) || TESSELATE)
/******************************************************************************/
struct VS_PS
{
#if SET_POS
   Vec pos:POS;
#endif

   Vec2 tex:TEXCOORD;

#if   BUMP_MODE> SBUMP_FLAT && PER_PIXEL
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE==SBUMP_FLAT && (PER_PIXEL || TESSELATE)
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif

#if MATERIALS>1
   VecH4 material:MATERIAL;
#endif

#if COLORS
   VecH col:COLOR;
#endif

#if REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT)
   VecH rfl:REFLECTION;
#endif

#if FX==FX_GRASS
   Half fade_out:FADE_OUT;
#endif

#if !PER_PIXEL && LIGHT && SHADOW
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

#if TEXTURES || DETAIL || LIGHT_MAP
   O.tex=vtx.tex(HEIGHTMAP);
   if(HEIGHTMAP && MATERIALS==1)O.tex*=MaterialTexScale();
#endif

#if MATERIALS>1
   O.material=vtx.material();
#endif

#if COLORS
   if(MATERIALS<=1)O.col=vtx.colorFast3()*MaterialColor3();
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

      #if FX==FX_GRASS
         BendGrass(local_pos, pos, vtx.instance());
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

      #if FX==FX_GRASS
         BendGrass(local_pos, pos);
         O.fade_out=GrassFadeOut();
      #endif
      }
   }else
   {
      VecI bone=vtx.bone();
      pos=TransformPos(pos, bone, vtx.weight());

   #if   BUMP_MODE> SBUMP_FLAT
      nrm=TransformDir(nrm, bone, vtx.weight());
      tan=TransformDir(tan, bone, vtx.weight());
   #elif BUMP_MODE==SBUMP_FLAT
      nrm=TransformDir(nrm, bone, vtx.weight());
   #endif
   }

   // normalize (have to do all at the same time, so all have the same lengths)
   if(BUMP_MODE>SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT) // per-vertex reflection
   || !PER_PIXEL && LIGHT // per-vertex lighting
   || TESSELATE) // needed for tesselation
   {
                              nrm=Normalize(nrm);
      if(BUMP_MODE>SBUMP_FLAT)tan=Normalize(tan);
   }

#if REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT)
   O.rfl=Transform3(reflect((VecH)Normalize(pos), nrm), CamMatrix);
#endif

#if   BUMP_MODE> SBUMP_FLAT && PER_PIXEL
   O.mtrx[0]=tan;
   O.mtrx[2]=nrm;
   O.mtrx[1]=vtx.bin(nrm, tan, HEIGHTMAP);
#elif BUMP_MODE==SBUMP_FLAT && PER_PIXEL
   O.nrm=nrm;
#endif

   //  per-vertex light
   #if !PER_PIXEL && LIGHT
   {
      FIXME
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
   PIXEL,
   IS_FRONT
):TARGET
{
   VecH col, nrm;
   Half glow, specular;

#if COLORS
   col=I.col;
#else
   if(MATERIALS<=1)col=MaterialColor3();
#endif

#if BUMP_MODE==SBUMP_ZERO
   nrm     =0;
   glow    =MaterialGlow();
   specular=0;
#elif MATERIALS==1
   VecH4 tex_nrm; // #MaterialTextureChannelOrder
   if(TEXTURES==0)
   {
      if(DETAIL)col+=GetDetail(I.tex).z;
      nrm     =Normalize(I.Nrm());
      glow    =MaterialGlow    ();
      specular=MaterialSpecular();
   }else
   if(TEXTURES==1)
   {
      VecH4 tex_col=Tex(Col, I.tex);
      if(ALPHA_TEST)
      {
      #if FX==FX_GRASS
         tex_col.a-=I.fade_out;
      #endif
         AlphaTest(tex_col.a);
      }
      col    *=tex_col.rgb; if(DETAIL)col+=GetDetail(I.tex).z;
      nrm     =Normalize(I.Nrm());
      glow    =MaterialGlow    ();
      specular=MaterialSpecular();
   }else
   if(TEXTURES==2)
   {
      tex_nrm=Tex(Nrm, I.tex); // #MaterialTextureChannelOrder
      if(ALPHA_TEST)
      {
      #if FX==FX_GRASS
         tex_nrm.w-=I.fade_out;
      #endif
         AlphaTest(tex_nrm.w);
      }
      glow    =MaterialGlow    (); if(!ALPHA_TEST)glow*=tex_nrm.a;
      specular=MaterialSpecular()*tex_nrm.z;

      #if BUMP_MODE==SBUMP_FLAT
      {
         nrm =Normalize(I.Nrm());
         col*=Tex(Col, I.tex).rgb; if(DETAIL)col+=GetDetail(I.tex).z;
      }
      #elif BUMP_MODE>SBUMP_FLAT // normal mapping
      {
         VecH      det;
         if(DETAIL)det=GetDetail(I.tex);

                   nrm.xy =(tex_nrm.xy*2-1)*MaterialRough();
         if(DETAIL)nrm.xy+=det.xy;
                   nrm.z  =CalcZ(nrm.xy);
                   nrm    =Normalize(Transform(nrm, I.mtrx));

                   col*=Tex(Col, I.tex).rgb;
         if(DETAIL)col+=det.z;
      }
      #endif
   }

   // reflection
   #if REFLECT
   {
   #if PER_PIXEL && BUMP_MODE>SBUMP_FLAT
      Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
   #else
      Vec rfl=I.rfl;
   #endif
      col+=TexCube(Rfl, rfl).rgb * ((TEXTURES==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
   }
   #endif
#else // MATERIALS>1
   // assuming that in multi materials TEXTURES!=0
   Vec2 tex0, tex1, tex2, tex3;
                   tex0=I.tex*MultiMaterial0TexScale();
                   tex1=I.tex*MultiMaterial1TexScale();
   if(MATERIALS>=3)tex2=I.tex*MultiMaterial2TexScale();
   if(MATERIALS>=4)tex3=I.tex*MultiMaterial3TexScale();

   // color !! do this first because it may modify 'I.material' which affects secondary texture !!
   VecH tex;
   if(MTRL_BLEND)
   {
      VecH4 col0, col1, col2, col3;
                       col0=Tex(Col , tex0); col0.rgb*=MultiMaterial0Color3(); I.material.x=MultiMaterialWeight(I.material.x, col0.a);
                       col1=Tex(Col1, tex1); col1.rgb*=MultiMaterial1Color3(); I.material.y=MultiMaterialWeight(I.material.y, col1.a); if(MATERIALS==2)I.material.xy  /=I.material.x+I.material.y;
      if(MATERIALS>=3){col2=Tex(Col2, tex2); col2.rgb*=MultiMaterial2Color3(); I.material.z=MultiMaterialWeight(I.material.z, col2.a); if(MATERIALS==3)I.material.xyz /=I.material.x+I.material.y+I.material.z;}
      if(MATERIALS>=4){col3=Tex(Col3, tex3); col3.rgb*=MultiMaterial3Color3(); I.material.w=MultiMaterialWeight(I.material.w, col3.a); if(MATERIALS==4)I.material.xyzw/=I.material.x+I.material.y+I.material.z+I.material.w;}

                      tex =I.material.x*col0.rgb
                          +I.material.y*col1.rgb;
      if(MATERIALS>=3)tex+=I.material.z*col2.rgb;
      if(MATERIALS>=4)tex+=I.material.w*col3.rgb;
   }else
   {
                      tex =I.material.x*Tex(Col , tex0).rgb*MultiMaterial0Color3()
                          +I.material.y*Tex(Col1, tex1).rgb*MultiMaterial1Color3();
      if(MATERIALS>=3)tex+=I.material.z*Tex(Col2, tex2).rgb*MultiMaterial2Color3();
      if(MATERIALS>=4)tex+=I.material.w*Tex(Col3, tex3).rgb*MultiMaterial3Color3();
   }
#if COLORS
   col*=tex;
#else
   col =tex;
#endif

   // normal, specular, glow !! do this second after modifying 'I.material' !!
   if(TEXTURES<=1)
   {
      nrm=Normalize(I.Nrm());

                VecH2 tex =I.material.x*MultiMaterial0NormalAdd().zw // #MaterialTextureChannelOrder
                          +I.material.y*MultiMaterial1NormalAdd().zw;
      if(MATERIALS>=3)tex+=I.material.z*MultiMaterial2NormalAdd().zw;
      if(MATERIALS>=4)tex+=I.material.w*MultiMaterial3NormalAdd().zw;

      specular=tex.x;
      glow    =tex.y;

      // reflection
      #if REFLECT
      {
      #if PER_PIXEL && BUMP_MODE>SBUMP_FLAT
         Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
      #else
         Vec rfl=I.rfl;
      #endif
                         col+=TexCube(Rfl , rfl).rgb*(MultiMaterial0Reflect()*I.material.x);
                         col+=TexCube(Rfl1, rfl).rgb*(MultiMaterial1Reflect()*I.material.y);
         if(MATERIALS>=3)col+=TexCube(Rfl2, rfl).rgb*(MultiMaterial2Reflect()*I.material.z);
         if(MATERIALS>=4)col+=TexCube(Rfl3, rfl).rgb*(MultiMaterial3Reflect()*I.material.w);
      }
      #endif
   }else
   {
      Half tex_spec[4];

      #if BUMP_MODE==SBUMP_FLAT
      {
         nrm=Normalize(I.Nrm());

                          VecH2 tex; // #MaterialTextureChannelOrder
                         {VecH2 nrm0; nrm0=Tex(Nrm , tex0).zw; if(REFLECT)tex_spec[0]=nrm0.x; nrm0=nrm0*MultiMaterial0NormalMul().zw+MultiMaterial0NormalAdd().zw; tex =I.material.x*nrm0;}
                         {VecH2 nrm1; nrm1=Tex(Nrm1, tex1).zw; if(REFLECT)tex_spec[1]=nrm1.x; nrm1=nrm1*MultiMaterial1NormalMul().zw+MultiMaterial1NormalAdd().zw; tex+=I.material.y*nrm1;}
         if(MATERIALS>=3){VecH2 nrm2; nrm2=Tex(Nrm2, tex2).zw; if(REFLECT)tex_spec[2]=nrm2.x; nrm2=nrm2*MultiMaterial2NormalMul().zw+MultiMaterial2NormalAdd().zw; tex+=I.material.z*nrm2;}
         if(MATERIALS>=4){VecH2 nrm3; nrm3=Tex(Nrm3, tex3).zw; if(REFLECT)tex_spec[3]=nrm3.x; nrm3=nrm3*MultiMaterial3NormalMul().zw+MultiMaterial3NormalAdd().zw; tex+=I.material.w*nrm3;}

         specular=tex.x;
         glow    =tex.y;
      }
      #elif BUMP_MODE>SBUMP_FLAT // normal mapping
      {
                          VecH4 tex; // #MaterialTextureChannelOrder
                         {VecH4 nrm0=Tex(Nrm , tex0); if(REFLECT)tex_spec[0]=nrm0.z; nrm0=nrm0*MultiMaterial0NormalMul()+MultiMaterial0NormalAdd(); tex =I.material.x*nrm0;}
                         {VecH4 nrm1=Tex(Nrm1, tex1); if(REFLECT)tex_spec[1]=nrm1.z; nrm1=nrm1*MultiMaterial1NormalMul()+MultiMaterial1NormalAdd(); tex+=I.material.y*nrm1;}
         if(MATERIALS>=3){VecH4 nrm2=Tex(Nrm2, tex2); if(REFLECT)tex_spec[2]=nrm2.z; nrm2=nrm2*MultiMaterial2NormalMul()+MultiMaterial2NormalAdd(); tex+=I.material.z*nrm2;}
         if(MATERIALS>=4){VecH4 nrm3=Tex(Nrm3, tex3); if(REFLECT)tex_spec[3]=nrm3.z; nrm3=nrm3*MultiMaterial3NormalMul()+MultiMaterial3NormalAdd(); tex+=I.material.w*nrm3;}

         nrm=VecH(tex.xy, CalcZ(tex.xy));
         nrm=Normalize(Transform(nrm, I.mtrx));
            
         specular=tex.z;
         glow    =tex.w;
      }
      #endif

      // reflection
      #if REFLECT
      {
      #if PER_PIXEL && BUMP_MODE>SBUMP_FLAT
         Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
      #else
         Vec rfl=I.rfl;
      #endif
                         col+=TexCube(Rfl , rfl).rgb*(MultiMaterial0Reflect()*I.material.x*tex_spec[0]);
                         col+=TexCube(Rfl1, rfl).rgb*(MultiMaterial1Reflect()*I.material.y*tex_spec[1]);
         if(MATERIALS>=3)col+=TexCube(Rfl2, rfl).rgb*(MultiMaterial2Reflect()*I.material.z*tex_spec[2]);
         if(MATERIALS>=4)col+=TexCube(Rfl3, rfl).rgb*(MultiMaterial3Reflect()*I.material.w*tex_spec[3]);
      }
      #endif
   }
#endif

   if(FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS)BackFlip(nrm, front);

   // lighting
   VecH total_lum,
        total_specular=0;

   FIXME
   if(BUMP_MODE==SBUMP_ZERO     )total_lum=1;
   else                          total_lum=AmbNSColor;
   if(MATERIALS<=1 && !SECONDARY_PASS) // ambient values are always disabled for secondary passes (so don't bother adding them)
   {
      if(LIGHT_MAP)total_lum+=AmbMaterial*MaterialAmbient()*Tex(Lum, I.tex).rgb;
      else         total_lum+=AmbMaterial*MaterialAmbient();
   }

   Vec2 jitter_value; if(SHADOW)jitter_value=ShadowJitter(pixel.xy);

   FIXME
   #if LIGHT_DIR
   {
      // shadow
      Half shadow; if(LIGHT_DIR_SHD)shadow=Sat(ShadowDirValue(I.pos, jitter_value, true, LIGHT_DIR_SHD_NUM, false));

      // diffuse
      VecH light_dir=LightDir.dir;
      Half lum      =LightDiffuse(nrm, light_dir); if(LIGHT_DIR_SHD)lum*=shadow;

      // specular
      BRANCH if(lum*specular>EPS_LUM)
      {
         VecH eye_dir=Normalize    (-I.pos);
         Half spec   =LightSpecular(   nrm, specular, light_dir, eye_dir); if(LIGHT_DIR_SHD)spec*=shadow;
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
      BRANCH if(lum*specular>EPS_LUM)
      {
         VecH eye_dir=Normalize    (-I.pos);
         Half spec   =LightSpecular(   nrm, specular, light_dir, eye_dir);
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
      BRANCH if(lum*specular>EPS_LUM)
      {
         VecH eye_dir=Normalize    (-I.pos);
         Half spec   =LightSpecular(   nrm, specular, light_dir, eye_dir);
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
      BRANCH if(lum*specular>EPS_LUM)
      {
         VecH eye_dir=Normalize    (-I.pos);
         Half spec   =LightSpecular(   nrm, specular, light_dir, eye_dir);
         total_specular+=LightCone.color.rgb*(spec*power);
      }  total_lum     +=LightCone.color.rgb*(lum *power);
   }
   #endif

   col=(col+Highlight.rgb)*total_lum + total_specular;

   return VecH4(col, glow);
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if TESSELATE
HSData HSConstant(InputPatch<VS_PS,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].mtrx[2], I[1].mtrx[2], I[2].mtrx[2]);}
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
   O.nrm =I[cp_id].nrm ;
#endif


#if TEXTURES || DETAIL || LIGHT_MAP
   O.tex=I[cp_id].tex;
#endif

#if MATERIALS>1
   O.material=I[cp_id].material;
#endif

#if COLORS
   O.col=I[cp_id].col;
#endif

#if FX==FX_GRASS
   O.fade_out=I[cp_id].fade_out;
#endif

#if REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT)
   O.rfl=I[cp_id].rfl;
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
#if   BUMP_MODE> SBUMP_FLAT
   O.mtrx[0]=I[0].mtrx[0]*B.z + I[1].mtrx[0]*B.x + I[2].mtrx[0]*B.y;
   O.mtrx[1]=I[0].mtrx[1]*B.z + I[1].mtrx[1]*B.x + I[2].mtrx[1]*B.y;
   SetDSPosNrm(O.pos, O.mtrx[2], I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#elif BUMP_MODE==SBUMP_FLAT
   SetDSPosNrm(O.pos, O.nrm    , I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#endif

#if TEXTURES || DETAIL || LIGHT_MAP
   O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;
#endif

#if MATERIALS>1
   O.material=I[0].material*B.z + I[1].material*B.x + I[2].material*B.y;
#endif

#if COLORS
   O.col=I[0].col*B.z + I[1].col*B.x + I[2].col*B.y;
#endif

#if FX==FX_GRASS
   O.fade_out=I[0].fade_out*B.z + I[1].fade_out*B.x + I[2].fade_out*B.y;
#endif

#if REFLECT && !(PER_PIXEL && BUMP_MODE>SBUMP_FLAT)
   O.rfl=I[0].rfl*B.z + I[1].rfl*B.x + I[2].rfl*B.y;
#endif

   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
