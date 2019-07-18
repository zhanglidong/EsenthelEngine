/******************************************************************************/
#include "!Header.h"
#include "Simple.h"
/******************************************************************************/
#define PARAMS             \
   uniform Bool skin      ,\
   uniform Int  materials ,\
   uniform Int  textures  ,\
   uniform Int  bump_mode ,\
   uniform Bool alpha_test,\
   uniform Bool light_map ,\
   uniform Bool rflct     ,\
   uniform Bool color     ,\
   uniform Bool mtrl_blend,\
   uniform Bool heightmap ,\
   uniform Int  fx        ,\
   uniform Bool per_pixel ,\
   uniform Bool tesselate
/******************************************************************************/
struct VS_PS
{
   VecH  nrm     :TEXCOORD0; // !! may not be Normalized !!
   Vec2  tex     :TEXCOORD1;
   Vec   pos     :TEXCOORD2;
   Half  fade_out:TEXCOORD3;
   VecH  rfl     :TEXCOORD4;
   VecH  col     :COLOR0   ;
   VecH4 material:COLOR1   ;
};
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out VS_PS O,
   out Vec4  O_vtx:POSITION,

   IF_IS_CLIP
   PARAMS
)
{
   Vec  pos=vtx.pos();
   VecH nrm; if(bump_mode>=SBUMP_FLAT)nrm=vtx.nrm();

   if(textures   )O.tex     =vtx.tex     (heightmap);
   if(materials>1)O.material=vtx.material();

   if(materials<=1)O.col.rgb=MaterialColor3();/*else
   if(!mtrl_blend )
   {
                      O.col.rgb =O.material.x*MultiMaterial0Color3()
                                +O.material.y*MultiMaterial1Color3();
      if(materials>=3)O.col.rgb+=O.material.z*MultiMaterial2Color3();
      if(materials>=4)O.col.rgb+=O.material.w*MultiMaterial3Color3();
   }*/
   if(color)
   {
      if(materials<=1/* || !mtrl_blend*/)O.col.rgb*=vtx.colorFast3();
      else                               O.col.rgb =vtx.colorFast3();
   }

   if(heightmap && textures && materials==1)O.tex*=MaterialTexScale();

   if(fx==FX_LEAF)
   {
      if(bump_mode>=SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm);
      else                     BendLeaf(vtx.hlp(), pos);
   }
   if(fx==FX_LEAFS)
   {
      if(bump_mode>=SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm);
      else                     BendLeafs(vtx.hlp(), vtx.size(), pos);
   }

   if(!skin)
   {
      if(true) // instance
      {
                                  O.pos=TransformPos(pos, vtx.instance());
         if(bump_mode>=SBUMP_FLAT)O.nrm=TransformDir(nrm, vtx.instance());
         if(fx==FX_GRASS)
         {
              BendGrass(pos, O.pos, vtx.instance());
            O.fade_out=GrassFadeOut(vtx.instance());
         }
      }else
      {
                                  O.pos=TransformPos(pos);
         if(bump_mode>=SBUMP_FLAT)O.nrm=TransformDir(nrm);
         if(fx==FX_GRASS)
         {
            BendGrass(pos, O.pos);
            O.fade_out=GrassFadeOut();
         }
      }
   }else
   {
      VecI bone=vtx.bone();
                               O.pos=TransformPos(pos, bone, vtx.weight());
      if(bump_mode>=SBUMP_FLAT)O.nrm=TransformDir(nrm, bone, vtx.weight());
   }

   // normalize
   if(bump_mode>=SBUMP_FLAT && (tesselate || rflct || !per_pixel))O.nrm=Normalize(O.nrm);

   if(fx==FX_BONE)
   {
      Flt b; if(!skin)b=0;else
      {
         VecI bone=vtx._bone; // use real bone regardless if skinning is enabled
         b=vtx.weight().x*Sat(1-Abs(bone.x-BoneHighlight))
          +vtx.weight().y*Sat(1-Abs(bone.y-BoneHighlight))
          +vtx.weight().z*Sat(1-Abs(bone.z-BoneHighlight));
      }
      O.col=((b>EPS) ? Vec(b, 0, 1-b) : Vec(1, 1, 1));
   }

   if(rflct)O.rfl=Transform3(reflect((VecH)Normalize(O.pos), O.nrm), CamMatrix);

   if(!per_pixel)
   {
      if(!(materials<=1 /*|| !mtrl_blend*/ || color || fx==FX_BONE))O.col=1;

      // lighting
      if(bump_mode>=SBUMP_FLAT)
      {
         Half d  =Sat(Dot(O.nrm, Light_dir.dir));
         VecH lum=Light_dir.color.rgb*d + AmbNSColor;
         if(materials<=1 && fx!=FX_BONE)lum+=MaterialAmbient();
         O.col.rgb*=lum;
      }
   }

   O_vtx=Project(O.pos); CLIP(O.pos);
}
/******************************************************************************/
VecH4 PS
(
   VS_PS I,

   IF_IS_FRONT

   PARAMS
):TARGET
{
   Half glow;

   if(fx==FX_BONE)
   {
      glow=0;
   }else
   {
      if(materials<=1)
      {
         if(textures==0)
         {
            glow=MaterialGlow();
         }else
         {
            VecH4 tex_nrm; // #MaterialTextureChannelOrder
            if(textures==1)
            {
               VecH4 tex_col=Tex(Col, I.tex); if(alpha_test)AlphaTest(tex_col.w, I.fade_out, fx);
               I.col.rgb*=tex_col.rgb;
               glow=MaterialGlow();
            }else
            if(textures==2)
            {
               /*if(per_pixel || alpha_test || rflct || is_glow)*/
               {
                  tex_nrm=Tex(Nrm, I.tex); if( alpha_test)AlphaTest(tex_nrm.a, I.fade_out, fx);
                  glow   =MaterialGlow() ; if(!alpha_test)glow*=tex_nrm.a;
               }//else glow=MaterialGlow(); always read 2nd texture to set glow, but in the future add a separate shader path that doesn't do this if material has no glow
               I.col.rgb*=Tex(Col, I.tex).rgb;
            }

            // reflection
            if(rflct)I.col.rgb+=TexCube(Rfl, I.rfl).rgb*((textures==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
         }
      }else // materials>1
      {
         glow=0;
         VecH tex;
         if(mtrl_blend)
         {
            VecH4 col0, col1, col2, col3;
                             col0=Tex(Col , I.tex*MultiMaterial0TexScale()); col0.rgb*=MultiMaterial0Color3(); I.material.x=MultiMaterialWeight(I.material.x, col0.a);
                             col1=Tex(Col1, I.tex*MultiMaterial1TexScale()); col1.rgb*=MultiMaterial1Color3(); I.material.y=MultiMaterialWeight(I.material.y, col1.a); if(materials==2)I.material.xy  /=I.material.x+I.material.y;
            if(materials>=3){col2=Tex(Col2, I.tex*MultiMaterial2TexScale()); col2.rgb*=MultiMaterial2Color3(); I.material.z=MultiMaterialWeight(I.material.z, col2.a); if(materials==3)I.material.xyz /=I.material.x+I.material.y+I.material.z;}
            if(materials>=4){col3=Tex(Col3, I.tex*MultiMaterial3TexScale()); col3.rgb*=MultiMaterial3Color3(); I.material.w=MultiMaterialWeight(I.material.w, col3.a); if(materials==4)I.material.xyzw/=I.material.x+I.material.y+I.material.z+I.material.w;}
                            tex =I.material.x*col0.rgb
                                +I.material.y*col1.rgb;
            if(materials>=3)tex+=I.material.z*col2.rgb;
            if(materials>=4)tex+=I.material.w*col3.rgb;
         }else
         {
                            tex =I.material.x*Tex(Col , I.tex*MultiMaterial0TexScale()).rgb*MultiMaterial0Color3()
                                +I.material.y*Tex(Col1, I.tex*MultiMaterial1TexScale()).rgb*MultiMaterial1Color3();
            if(materials>=3)tex+=I.material.z*Tex(Col2, I.tex*MultiMaterial2TexScale()).rgb*MultiMaterial2Color3();
            if(materials>=4)tex+=I.material.w*Tex(Col3, I.tex*MultiMaterial3TexScale()).rgb*MultiMaterial3Color3();
         }
         if(materials<=1 /*|| !mtrl_blend*/ || color || fx==FX_BONE || !per_pixel)I.col.rgb*=tex;
         else                                                                     I.col.rgb =tex;

         // reflection
         if(rflct)
         {
                            I.col.rgb+=TexCube(Rfl , I.rfl).rgb*(MultiMaterial0Reflect()*I.material.x*((textures==2) ? Tex(Nrm , I.tex*MultiMaterial0TexScale()).z : 1));
                            I.col.rgb+=TexCube(Rfl1, I.rfl).rgb*(MultiMaterial1Reflect()*I.material.y*((textures==2) ? Tex(Nrm1, I.tex*MultiMaterial1TexScale()).z : 1));
            if(materials>=3)I.col.rgb+=TexCube(Rfl2, I.rfl).rgb*(MultiMaterial2Reflect()*I.material.z*((textures==2) ? Tex(Nrm2, I.tex*MultiMaterial2TexScale()).z : 1));
            if(materials>=4)I.col.rgb+=TexCube(Rfl3, I.rfl).rgb*(MultiMaterial3Reflect()*I.material.w*((textures==2) ? Tex(Nrm3, I.tex*MultiMaterial3TexScale()).z : 1));
         }
      }
   }

   I.col.rgb+=Highlight.rgb;

   // perform lighting
   if(per_pixel && bump_mode>=SBUMP_FLAT)
   {
      VecH nrm=Normalize(I.nrm); if(fx!=FX_GRASS && fx!=FX_LEAF && fx!=FX_LEAFS)BackFlip(nrm, front);
      Half d  =Sat(Dot(nrm, Light_dir.dir));
      VecH lum=Light_dir.color.rgb*d + AmbNSColor;
      if(materials<=1 && fx!=FX_BONE)
      {
         if(light_map)lum+=MaterialAmbient()*Tex(Lum, I.tex).rgb;
         else         lum+=MaterialAmbient();
      }
      I.col.rgb*=lum;
   }

   return VecH4(I.col.rgb, glow);
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if !CG
HSData HSConstant(InputPatch<VS_PS,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm);}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
VS_PS HS
(
   InputPatch<VS_PS,3> I, UInt cp_id:SV_OutputControlPointID,
   PARAMS
)
{
   VS_PS O;
                                                                            O.pos     =I[cp_id].pos;
                                                                            O.nrm     =I[cp_id].nrm;
   if(materials<=1 /*|| !mtrl_blend*/ || color || fx==FX_BONE || !per_pixel)O.col     =I[cp_id].col;
   if(textures                                                             )O.tex     =I[cp_id].tex;
   if(rflct                                                                )O.rfl     =I[cp_id].rfl;
   if(materials>1                                                          )O.material=I[cp_id].material;
   if(fx==FX_GRASS                                                         )O.fade_out=I[cp_id].fade_out;
   return O;
}
/******************************************************************************/
[domain("tri")]
void DS
(
   HSData hs_data, const OutputPatch<VS_PS,3> I, Vec B:SV_DomainLocation,

   out VS_PS O,
   out Vec4  O_vtx:POSITION,

   PARAMS
)
{
   if(materials<=1 /*|| !mtrl_blend*/ || color || fx==FX_BONE || !per_pixel)O.col     =I[0].col     *B.z + I[1].col     *B.x + I[2].col     *B.y;
   if(textures                                                             )O.tex     =I[0].tex     *B.z + I[1].tex     *B.x + I[2].tex     *B.y;
   if(rflct                                                                )O.rfl     =I[0].rfl     *B.z + I[1].rfl     *B.x + I[2].rfl     *B.y;
   if(materials>1                                                          )O.material=I[0].material*B.z + I[1].material*B.x + I[2].material*B.y;
   if(fx==FX_GRASS                                                         )O.fade_out=I[0].fade_out*B.z + I[1].fade_out*B.x + I[2].fade_out*B.y;

   SetDSPosNrm(O.pos, O.nrm, I[0].pos, I[1].pos, I[2].pos, I[0].nrm, I[1].nrm, I[2].nrm, B, hs_data, false, 0);
   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
// TECHNIQUES
/******************************************************************************/
CUSTOM_TECHNIQUE
/******************************************************************************/
