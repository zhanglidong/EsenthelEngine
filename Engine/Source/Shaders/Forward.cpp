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
#define SECONDARY_PASS (LIGHT_POINT || LIGHT_LINEAR || LIGHT_CONE) // local lights are enabled only for secondary shader passes
/******************************************************************************/
struct VS_PS
{
   Vec2     tex     :TEXCOORD0;
   Vec      pos     :TEXCOORD1;
   MatrixH3 mtrx    :TEXCOORD2; // !! may not be Normalized !!
   VecH     rfl     :TEXCOORD6;
   Half     fade_out:TEXCOORD5;
   VecH     col     :COLOR0   ;
   VecH4    material:COLOR1   ;
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

   if(TEXTURES || DETAIL)O.tex     =vtx.tex     (HEIGHTMAP);
   if(MATERIALS>1       )O.material=vtx.material();

   if(MATERIALS<=1)O.col.rgb=MaterialColor3();
   if(COLORS)
   {
      if(MATERIALS<=1)O.col.rgb*=vtx.colorFast3();
      else            O.col.rgb =vtx.colorFast3();
   }

   if(HEIGHTMAP && TEXTURES && MATERIALS==1)O.tex*=MaterialTexScale();

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

   if(!SKIN)
   {
      if(true) // instance
      {
         O.pos=TransformPos(pos, vtx.instance());

         if(BUMP_MODE>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm, vtx.instance());
         if(BUMP_MODE> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan, vtx.instance());
         if(FX==FX_GRASS)
         {
              BendGrass(pos, O.pos, vtx.instance());
            O.fade_out=GrassFadeOut(vtx.instance());
         }
      }else
      {
         O.pos=TransformPos(pos);

         if(BUMP_MODE>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm);
         if(BUMP_MODE> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan);
         if(FX==FX_GRASS)
         {
            BendGrass(pos, O.pos);
            O.fade_out=GrassFadeOut();
         }
      }
   }else
   {
      VecI bone=vtx.bone();
      O.pos=TransformPos(pos, bone, vtx.weight());

      if(BUMP_MODE>=SBUMP_FLAT)O.mtrx[2]=TransformDir(nrm, bone, vtx.weight());
      if(BUMP_MODE> SBUMP_FLAT)O.mtrx[0]=TransformDir(tan, bone, vtx.weight());
   }
   CLIP_PLANE(O.pos);

   // normalize (have to do all at the same time, so all have the same lengths)
   if(BUMP_MODE>SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || REFLECT && BUMP_MODE==SBUMP_FLAT // calculating per-vertex reflection
   || TESSELATE) // needed for tesselation
   {
                              O.mtrx[2]=Normalize(O.mtrx[2]);
      if(BUMP_MODE>SBUMP_FLAT)O.mtrx[0]=Normalize(O.mtrx[0]);
   }

   if(BUMP_MODE>SBUMP_FLAT)O.mtrx[1]=vtx.bin(O.mtrx[2], O.mtrx[0], HEIGHTMAP);

   if(REFLECT && BUMP_MODE==SBUMP_FLAT)O.rfl=Transform3(reflect((VecH)Normalize(O.pos), O.mtrx[2]), CamMatrix);

   O_vtx=Project(O.pos);
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
   VecH nrm;
   Half glow, specular;

   if(BUMP_MODE==SBUMP_ZERO)
   {
      nrm     =0;
      glow    =MaterialGlow();
      specular=0;
   }else
   if(MATERIALS==1)
   {
      VecH4 tex_nrm; // #MaterialTextureChannelOrder
      if(TEXTURES==0)
      {
         Half tex_col=1; if(DETAIL)tex_col+=GetDetail(I.tex).z; I.col.rgb*=tex_col;
         nrm     =Normalize(I.mtrx[2]);
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
         nrm     =Normalize(I.mtrx[2]);
         glow    =MaterialGlow    ();
         specular=MaterialSpecular();

         if(DETAIL)tex_col.rgb+=GetDetail(I.tex).z; I.col.rgb*=tex_col.rgb;
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
         if(BUMP_MODE==SBUMP_FLAT)
         {
            nrm=Normalize(I.mtrx[2]);
            VecH tex_col=Tex(Col, I.tex).rgb; if(DETAIL)tex_col+=GetDetail(I.tex).z; I.col.rgb*=tex_col;
         }else // normal mapping
         {
            VecH      det;
            if(DETAIL)det=GetDetail(I.tex);

                      nrm.xy =(tex_nrm.xy*2-1)*MaterialRough();
            if(DETAIL)nrm.xy+=det.xy;
                      nrm.z  =CalcZ(nrm.xy);
                      nrm    =Normalize(Transform(nrm, I.mtrx));

            VecH tex_col=Tex(Col, I.tex).rgb; if(DETAIL)tex_col+=det.z; I.col.rgb*=tex_col;
         }
      }

      // reflection
      if(REFLECT)
      {
         if(BUMP_MODE>SBUMP_FLAT)I.rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
         I.col.rgb+=TexCube(Rfl, I.rfl).rgb*((TEXTURES==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
      }
   }else // MATERIALS>1
   {
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
      if(MATERIALS<=1 || COLORS)I.col.rgb*=tex;
      else                      I.col.rgb =tex;

      // normal, specular, glow !! do this second after modifying 'I.material' !!
      if(TEXTURES<=1)
      {
         nrm=Normalize(I.mtrx[2]);

                   VecH2 tex =I.material.x*MultiMaterial0NormalAdd().zw // #MaterialTextureChannelOrder
                             +I.material.y*MultiMaterial1NormalAdd().zw;
         if(MATERIALS>=3)tex+=I.material.z*MultiMaterial2NormalAdd().zw;
         if(MATERIALS>=4)tex+=I.material.w*MultiMaterial3NormalAdd().zw;

         specular=tex.x;
         glow    =tex.y;

         // reflection
         if(REFLECT)
         {
            if(BUMP_MODE>SBUMP_FLAT)I.rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
                            I.col.rgb+=TexCube(Rfl , I.rfl).rgb*(MultiMaterial0Reflect()*I.material.x);
                            I.col.rgb+=TexCube(Rfl1, I.rfl).rgb*(MultiMaterial1Reflect()*I.material.y);
            if(MATERIALS>=3)I.col.rgb+=TexCube(Rfl2, I.rfl).rgb*(MultiMaterial2Reflect()*I.material.z);
            if(MATERIALS>=4)I.col.rgb+=TexCube(Rfl3, I.rfl).rgb*(MultiMaterial3Reflect()*I.material.w);
         }
      }else
      {
         Half tex_spec[4];

         if(BUMP_MODE==SBUMP_FLAT)
         {
            nrm=Normalize(I.mtrx[2]);

                             VecH2 tex; // #MaterialTextureChannelOrder
                            {VecH2 nrm0; nrm0=Tex(Nrm , tex0).zw; if(REFLECT)tex_spec[0]=nrm0.x; nrm0=nrm0*MultiMaterial0NormalMul().zw+MultiMaterial0NormalAdd().zw; tex =I.material.x*nrm0;}
                            {VecH2 nrm1; nrm1=Tex(Nrm1, tex1).zw; if(REFLECT)tex_spec[1]=nrm1.x; nrm1=nrm1*MultiMaterial1NormalMul().zw+MultiMaterial1NormalAdd().zw; tex+=I.material.y*nrm1;}
            if(MATERIALS>=3){VecH2 nrm2; nrm2=Tex(Nrm2, tex2).zw; if(REFLECT)tex_spec[2]=nrm2.x; nrm2=nrm2*MultiMaterial2NormalMul().zw+MultiMaterial2NormalAdd().zw; tex+=I.material.z*nrm2;}
            if(MATERIALS>=4){VecH2 nrm3; nrm3=Tex(Nrm3, tex3).zw; if(REFLECT)tex_spec[3]=nrm3.x; nrm3=nrm3*MultiMaterial3NormalMul().zw+MultiMaterial3NormalAdd().zw; tex+=I.material.w*nrm3;}

            specular=tex.x;
            glow    =tex.y;
         }else // normal mapping
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

         // reflection
         if(REFLECT)
         {
            if(BUMP_MODE>SBUMP_FLAT)I.rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
                            I.col.rgb+=TexCube(Rfl , I.rfl).rgb*(MultiMaterial0Reflect()*I.material.x*tex_spec[0]);
                            I.col.rgb+=TexCube(Rfl1, I.rfl).rgb*(MultiMaterial1Reflect()*I.material.y*tex_spec[1]);
            if(MATERIALS>=3)I.col.rgb+=TexCube(Rfl2, I.rfl).rgb*(MultiMaterial2Reflect()*I.material.z*tex_spec[2]);
            if(MATERIALS>=4)I.col.rgb+=TexCube(Rfl3, I.rfl).rgb*(MultiMaterial3Reflect()*I.material.w*tex_spec[3]);
         }
      }
   }

   if(FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS)BackFlip(nrm, front);

   // lighting
   VecH total_lum,
        total_specular=0;

   if(BUMP_MODE==SBUMP_ZERO     )total_lum=1;
   else                          total_lum=AmbNSColor;
   if(MATERIALS<=1 && !SECONDARY_PASS) // ambient values are always disabled for secondary passes (so don't bother adding them)
   {
      if(LIGHT_MAP)total_lum+=AmbMaterial*MaterialAmbient()*Tex(Lum, I.tex).rgb;
      else         total_lum+=AmbMaterial*MaterialAmbient();
   }

   Vec2 jitter_value;
   if(LIGHT_DIR_SHD || LIGHT_POINT_SHD || LIGHT_LINEAR_SHD || LIGHT_CONE_SHD)jitter_value=ShadowJitter(pixel.xy);

   if(LIGHT_DIR)
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

   if(LIGHT_POINT)
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

   if(LIGHT_LINEAR)
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

   if(LIGHT_CONE)
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

   I.col.rgb=(I.col.rgb+Highlight.rgb)*total_lum + total_specular;

   return VecH4(I.col.rgb, glow);
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
                                       O.pos     =I[cp_id].pos     ;
   if(MATERIALS<=1 || COLORS          )O.col     =I[cp_id].col     ;
   if(TEXTURES || DETAIL              )O.tex     =I[cp_id].tex     ;
   if(REFLECT && BUMP_MODE==SBUMP_FLAT)O.rfl     =I[cp_id].rfl     ;
   if(MATERIALS>1                     )O.material=I[cp_id].material;
   if(FX==FX_GRASS                    )O.fade_out=I[cp_id].fade_out;
   if(BUMP_MODE==SBUMP_FLAT           )O.mtrx[2] =I[cp_id].mtrx[2] ;
   if(BUMP_MODE> SBUMP_FLAT           )O.mtrx    =I[cp_id].mtrx    ;
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
   if(MATERIALS<=1 || COLORS          )O.col     =I[0].col     *B.z + I[1].col     *B.x + I[2].col     *B.y;
   if(TEXTURES || DETAIL              )O.tex     =I[0].tex     *B.z + I[1].tex     *B.x + I[2].tex     *B.y;
   if(REFLECT && BUMP_MODE==SBUMP_FLAT)O.rfl     =I[0].rfl     *B.z + I[1].rfl     *B.x + I[2].rfl     *B.y;
   if(MATERIALS>1                     )O.material=I[0].material*B.z + I[1].material*B.x + I[2].material*B.y;
   if(FX==FX_GRASS                    )O.fade_out=I[0].fade_out*B.z + I[1].fade_out*B.x + I[2].fade_out*B.y;

   if(BUMP_MODE>SBUMP_FLAT)
   {
      O.mtrx[0]=I[0].mtrx[0]*B.z + I[1].mtrx[0]*B.x + I[2].mtrx[0]*B.y;
      O.mtrx[1]=I[0].mtrx[1]*B.z + I[1].mtrx[1]*B.x + I[2].mtrx[1]*B.y;
      //mtrx[2] is handled below
   }

   SetDSPosNrm(O.pos, O.mtrx[2], I[0].pos, I[1].pos, I[2].pos, I[0].mtrx[2], I[1].mtrx[2], I[2].mtrx[2], B, hs_data, false, 0);
   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
