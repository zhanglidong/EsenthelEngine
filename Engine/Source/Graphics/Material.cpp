/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************

   If Material has only 1 texture (base_0), then it contains:
      Base0: RGBA

   If Material has 3 textures (base_0, base_1, base_2), then they contain:
      Base0: RGB, Glow
      Base1: NrmX, NrmY
      Base2: Smooth, Reflect, Bump, Alpha

   When changing the above to a different order, then look for "#MaterialTextureChannelOrder" text in Engine/Editor to update the codes.

/******************************************************************************/
#define CC4_MTRL CC4('M','T','R','L')

#define BUMP_DEFAULT_TEX 0 // 0..255, normally this should be 128, but 0 will allow to use BC5 (for Mtrl.base_2 tex if there's no Alpha) and always set Material.bump=0 when bump is not used #MaterialTextureChannelOrder
#define BUMP_DEFAULT_PAR 0.03f

#define BUMP_NORMAL_SCALE (1.0f/64) // 0.015625, this value should be close to average 'Material.bump' which are 0.015, 0.03, 0.05 (remember that in Editor that value may be scaled)

#define REFLECT_DEFAULT_PAR 0.04f

// #MaterialTextureChannelOrder
// Base 0
#define    GLOW_CHANNEL 3
// Base 1
#define    NRMX_CHANNEL 0
#define    NRMY_CHANNEL 1
// Base 2
#define  SMOOTH_CHANNEL 0
#define REFLECT_CHANNEL 1
#define    BUMP_CHANNEL 2
#define   ALPHA_CHANNEL 3
/******************************************************************************/
static Int Compare(C UniqueMultiMaterialKey &a, C UniqueMultiMaterialKey &b)
{
   if(a.m[0]<b.m[0])return -1; if(a.m[0]>b.m[0])return +1;
   if(a.m[1]<b.m[1])return -1; if(a.m[1]>b.m[1])return +1;
   if(a.m[2]<b.m[2])return -1; if(a.m[2]>b.m[2])return +1;
   if(a.m[3]<b.m[3])return -1; if(a.m[3]>b.m[3])return +1;
                                                return  0;
}
/******************************************************************************/
DEFINE_CACHE(Material, Materials, MaterialPtr, "Material");
Material               MaterialDefault,
                       MaterialDefaultNoCull;
const Material        *MaterialLast,
                      *MaterialLast4[4]; // can't merge with 'MaterialLast' because that sets 'Sh.Material' but this sets 'Sh.MultiMaterial'
MaterialPtr            MaterialNull;
ThreadSafeMap<UniqueMultiMaterialKey, UniqueMultiMaterialData> UniqueMultiMaterialMap(Compare);
/******************************************************************************/
Vec4 MaterialParams::colorS(               )C {return LinearToSRGB(color_l);}
void MaterialParams::colorS(C Vec4 &color_s)  {return colorL(SRGBToLinear(color_s));}
/******************************************************************************/
Material::Material()
{
   color_l.set(1, 1, 1, 1);
   ambient.set(0, 0, 0);
   smooth   =0;
   reflect  =REFLECT_DEFAULT_PAR;
   glow     =0;
   normal   =1;
   bump     =BUMP_DEFAULT_PAR;
   det_power=0.3f;
   det_scale=4;
   tex_scale=1.0f;

   cull     =true;
   technique=MTECH_DEFAULT;

   clear();
   validate();
}
Material::~Material()
{
#if !SYNC_LOCK_SAFE // if 'SyncLock' is not safe then crash may occur when trying to lock, to prevent that, check if we have any elements (this means cache was already initialized)
   if(UniqueMultiMaterialMap.elms())
#endif
   {
      UniqueMultiMaterialMap.lock  (); REPA(UniqueMultiMaterialMap){C UniqueMultiMaterialKey &key=UniqueMultiMaterialMap.lockedKey(i); if(key.m[0]==this || key.m[1]==this || key.m[2]==this || key.m[3]==this)UniqueMultiMaterialMap.remove(i);}
      UniqueMultiMaterialMap.unlock();
   }
}
/******************************************************************************/
static Bool HasAlphaTest(MATERIAL_TECHNIQUE technique)
{
   switch(technique)
   {
      case MTECH_ALPHA_TEST            :
      case MTECH_GRASS                 :
      case MTECH_LEAF                  :
      case MTECH_TEST_BLEND_LIGHT      :
      case MTECH_TEST_BLEND_LIGHT_GRASS:
      case MTECH_TEST_BLEND_LIGHT_LEAF : return true;

      default: return false;
   }
}
Bool Material::hasAlpha          ()C {return technique==MTECH_ALPHA_TEST || technique==MTECH_GRASS || technique==MTECH_LEAF || technique==MTECH_BLEND || technique==MTECH_BLEND_LIGHT || technique==MTECH_BLEND_LIGHT_GRASS || technique==MTECH_BLEND_LIGHT_LEAF || technique==MTECH_TEST_BLEND_LIGHT || technique==MTECH_TEST_BLEND_LIGHT_GRASS || technique==MTECH_TEST_BLEND_LIGHT_LEAF;}
Bool Material::hasAlphaBlend     ()C {return                                                                                   technique==MTECH_BLEND || technique==MTECH_BLEND_LIGHT || technique==MTECH_BLEND_LIGHT_GRASS || technique==MTECH_BLEND_LIGHT_LEAF || technique==MTECH_TEST_BLEND_LIGHT || technique==MTECH_TEST_BLEND_LIGHT_GRASS || technique==MTECH_TEST_BLEND_LIGHT_LEAF;}
Bool Material::hasAlphaBlendLight()C {return                                                                                                             technique==MTECH_BLEND_LIGHT || technique==MTECH_BLEND_LIGHT_GRASS || technique==MTECH_BLEND_LIGHT_LEAF || technique==MTECH_TEST_BLEND_LIGHT || technique==MTECH_TEST_BLEND_LIGHT_GRASS || technique==MTECH_TEST_BLEND_LIGHT_LEAF;}
Bool Material::hasGrass          ()C {return                                technique==MTECH_GRASS ||                                                                                    technique==MTECH_BLEND_LIGHT_GRASS ||                                                                           technique==MTECH_TEST_BLEND_LIGHT_GRASS                                          ;}
Bool Material::hasLeaf           ()C {return                                                          technique==MTECH_LEAF ||                                                                                                 technique==MTECH_BLEND_LIGHT_LEAF ||                                                                                 technique==MTECH_TEST_BLEND_LIGHT_LEAF;}
/******************************************************************************/
Bool Material::wantTanBin()C
{
   // #MaterialTextureChannelOrder
   return (base_1     && normal   >EPS_COL          )  // normal        is in Base1
       || (base_2     && bump     >EPS_MATERIAL_BUMP)  // bump          is in Base2
       || (detail_map && det_power>EPS_COL          ); // normal detail is in DetailMap
}
/******************************************************************************/
Material& Material::reset   () {T=MaterialDefault; return validate();}
Material& Material::validate() // #MaterialTextureChannelOrder
{
                      if(this==MaterialLast    )MaterialLast    =null;
   REPA(MaterialLast4)if(this==MaterialLast4[i])MaterialLast4[i]=null;

  _has_alpha_test=HasAlphaTest(technique); // !! set this first, because codes below rely on this !!
  _depth_write   =!(hasAlphaBlend() && !hasAlphaTest ());
//_coverage      = (hasAlphaTest () && !hasAlphaBlend());
  _alpha_factor.set(0, 0, 0, FltToByte(T.glow));

   // set multi
   {
     _multi.color    =(LINEAR_GAMMA ? colorL() : colorS());
     _multi.glow     =glow; // 'glow_mul', 'glow_add' not needed because Glow is stored in Base0 (which can be either Alpha or Glow, however Alpha is never used for multi-materials, so it's always 1.0)
     _multi.tex_scale=tex_scale;
     _multi.det_scale=det_scale;

      // normal map
      if(base_1)
      {
        _multi.normal=normal;
      }else
      {
        _multi.normal=0;
      }

      // base2
      if(base_2)
      {
        _multi.base2_mul.c[SMOOTH_CHANNEL]=smooth;
        _multi.base2_add.c[SMOOTH_CHANNEL]=0;

        _multi.base2_mul.c[REFLECT_CHANNEL]=reflect;
        _multi.base2_add.c[REFLECT_CHANNEL]=0;

        _multi.bump=bump;
      }else
      {
        _multi.base2_mul=0;
        _multi.base2_add.c[ SMOOTH_CHANNEL]=smooth;
        _multi.base2_add.c[REFLECT_CHANNEL]=reflect;

        _multi.bump=0;
      }

      if(detail_map)
      {
         // (tex-0.5)*det_power == tex*det_power - det_power*0.5
        _multi.det_mul=det_power;
        _multi.det_add=det_power*-0.5f;
      }else
      {
        _multi.det_mul=0;
        _multi.det_add=0;
      }
     _multi.macro=(macro_map!=null);
   }
   return T;
}
/******************************************************************************
Bool Material::convertAlphaTest(Bool blend)
{
   if(blend)
   {
      if(technique==MTECH_ALPHA_TEST){technique=MTECH_TEST_BLEND_LIGHT      ; color.w=1; validate(); return true;}
      if(technique==MTECH_GRASS     ){technique=MTECH_TEST_BLEND_LIGHT_GRASS; color.w=1; validate(); return true;}
      if(technique==MTECH_LEAF      ){technique=MTECH_TEST_BLEND_LIGHT_LEAF ; color.w=1; validate(); return true;}
   }else
   {
      if(technique==MTECH_TEST_BLEND_LIGHT      ){technique=MTECH_ALPHA_TEST; color.w=0.5f; validate(); return true;}
      if(technique==MTECH_TEST_BLEND_LIGHT_GRASS){technique=MTECH_GRASS     ; color.w=0.5f; validate(); return true;}
      if(technique==MTECH_TEST_BLEND_LIGHT_LEAF ){technique=MTECH_LEAF      ; color.w=0.5f; validate(); return true;}
   }
   return false;
}
/******************************************************************************/
void Material::setSolid()C
{
   if(MaterialLast!=this)
   {
      MaterialLast    =this;
      MaterialLast4[0]=null; // because they use the same shader images

      if(_alpha_factor.a)Renderer._has_glow=true;
      Sh.Col[0]  ->set(    base_0());
      Sh.Nrm[0]  ->set(    base_1());
      Sh.Ext[0]  ->set(    base_2());
      Sh.Det[0]  ->set(detail_map());
      Sh.Mac[0]  ->set( macro_map());
      Sh.Lum     ->set( light_map());
      Sh.Material->set<MaterialParams>(T);
   #if !LINEAR_GAMMA
      Renderer.material_color_l->set(colorS());
   #endif
   }
}
void Material::setAmbient()C
{
   if(MaterialLast!=this)
   {
      MaterialLast=this;
    //MaterialLast4[0]=null; not needed since multi materials not rendered in ambient mode

      // textures needed for alpha-test
      Sh.Col[0]  ->set(base_0   ());
      Sh.Ext[0]  ->set(base_2   ());
      Sh.Lum     ->set(light_map());
      Sh.Material->set<MaterialParams>(T); // params needed for alpha-test and ambient
   #if !LINEAR_GAMMA
      Renderer.material_color_l->set(colorS());
   #endif
   }
}
void Material::setBlend()C
{
   if(MaterialLast!=this)
   {
      MaterialLast=this;
    //MaterialLast4[0]=null; not needed since multi materials not rendered in blend mode

      D.alphaFactor(_alpha_factor); if(_alpha_factor.a)Renderer._has_glow=true;

      Sh.Col[0]  ->set(    base_0());
      Sh.Nrm[0]  ->set(    base_1());
      Sh.Ext[0]  ->set(    base_2());
      Sh.Det[0]  ->set(detail_map());
      Sh.Mac[0]  ->set( macro_map());
      Sh.Lum     ->set( light_map());
      Sh.Material->set<MaterialParams>(T);
   #if !LINEAR_GAMMA
      Renderer.material_color_l->set(colorS());
   #endif
   }
}
void Material::setBlendForce()C
{
   if(_alpha_factor.a && !hasAlpha()) // if has glow in material settings and on texture channel, then it means we need to disable it for forced blend, which operates on alpha instead of glow
   {
      if(MaterialLast==this)MaterialLast=null;
      D.alphaFactor(TRANSPARENT);
   }else
   {
      if(MaterialLast==this)return;
         MaterialLast= this;

      D.alphaFactor(_alpha_factor); if(_alpha_factor.a)Renderer._has_glow=true;
   }

   Sh.Col[0]  ->set(    base_0());
   Sh.Nrm[0]  ->set(    base_1());
   Sh.Ext[0]  ->set(    base_2());
   Sh.Det[0]  ->set(detail_map());
   Sh.Mac[0]  ->set( macro_map());
   Sh.Lum     ->set( light_map());
   Sh.Material->set<MaterialParams>(T);
#if !LINEAR_GAMMA
   Renderer.material_color_l->set(colorS());
#endif
}
void Material::setOutline()C
{
   if(MaterialLast!=this)
   {
      MaterialLast=this;
    //MaterialLast4[0]=null; not needed since multi materials not rendered in outline mode
      Sh.Col[0]->set(base_0());
      Sh.Ext[0]->set(base_2());
      Renderer.material_color_l->set(LINEAR_GAMMA ? colorL() : colorS()); // only Material Color is used for potential alpha-testing
   }
}
void Material::setBehind()C
{
   if(MaterialLast!=this)
   {
      MaterialLast=this;
    //MaterialLast4[0]=null; not needed since multi materials not rendered in behind mode
      Sh.Col[0]->set(base_0());
      Sh.Ext[0]->set(base_2());
      Renderer.material_color_l->set(LINEAR_GAMMA ? colorL() : colorS()); // only Material Color is used
   }
}
void Material::setShadow()C
{
   if(hasAlphaTest() && MaterialLast!=this) // this shader needs params/textures only for alpha test (if used)
   {
      MaterialLast=this;
    //MaterialLast4[0]=null; not needed since multi materials don't have alpha test and don't need to set values in shadow mode
      Sh.Col[0]->set(base_0());
      Sh.Ext[0]->set(base_2());
      Renderer.material_color_l->set(LINEAR_GAMMA ? colorL() : colorS()); // only Material Color is used
   }
}
void Material::setMulti(Int i)C
{
   DEBUG_RANGE_ASSERT(i, Sh.MultiMaterial);
   if(MaterialLast4[i]!=this)
   {
            MaterialLast4[i]=this;
      if(!i)MaterialLast    =null; // because they use the same shader images

      if(_alpha_factor.a)Renderer._has_glow=true;

      Sh.Col          [i]->set(  base_0  ());
      Sh.Nrm          [i]->set(  base_1  ());
      Sh.Ext          [i]->set(  base_2  ());
      Sh.Det          [i]->set(detail_map());
      Sh.Mac          [i]->set( macro_map());
      Sh.MultiMaterial[i]->set(_multi      );
   }
}
void Material::setAuto()C
{
   switch(Renderer())
   {
      case RM_PREPARE :
      case RM_SOLID   :
      case RM_SOLID_M : setSolid(); break;

      case RM_AMBIENT : setAmbient(); break;

      case RM_FUR     :
      case RM_CLOUD   :
      case RM_BLEND   :
      case RM_PALETTE :
      case RM_PALETTE1:
      case RM_OVERLAY : setBlend(); break;

      case RM_BEHIND  : setBehind(); break;

      case RM_OUTLINE : setOutline(); break;

      case RM_EARLY_Z :
      case RM_SHADOW  : setShadow(); break;
   }
}
/******************************************************************************/
void Material::_adjustParams(UInt old_base_tex, UInt new_base_tex)
{
   UInt changed=(old_base_tex^new_base_tex);
   if(changed&BT_BUMP)
   {
      if(!(new_base_tex&BT_BUMP))bump=0;else
      if(bump<=EPS_MATERIAL_BUMP)bump=BUMP_DEFAULT_PAR;
   }
   if(changed&(BT_BUMP|BT_NORMAL))
   {
      if(!(new_base_tex&BT_BUMP) && !(new_base_tex&BT_NORMAL))normal=0;else
      if(                                     normal<=EPS_COL)normal=1;
   }

   if(changed&BT_SMOOTH)
      if((new_base_tex&BT_SMOOTH) && smooth<=EPS_COL)smooth=1;

   if(changed&BT_REFLECT)
      if((new_base_tex&BT_REFLECT) && reflect<=REFLECT_DEFAULT_PAR+EPS_COL)reflect=1;

   if(changed&BT_GLOW)
      if((new_base_tex&BT_GLOW) && glow<=EPS_COL)glow=1;

   if(changed&BT_ALPHA)
   {
      if(new_base_tex&BT_ALPHA)
      {
         if(!hasAlphaBlend() && color_l.w>=1-EPS_COL)color_l.w=0.5f;
         if(!hasAlpha     ()                        )technique=MTECH_ALPHA_TEST;
      }else
      {
         if(hasAlpha())technique=MTECH_DEFAULT; // disable alpha technique if alpha map is not available
      }
   }

   validate();
}
/******************************************************************************/
Bool Material::saveData(File &f, CChar *path)C
{
   f.putMulti(Byte(10), cull, technique)<<SCAST(C MaterialParams, T); // version

   // textures
   f.putStr(    base_0.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr(    base_1.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr(    base_2.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr(detail_map.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr( macro_map.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr( light_map.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!

   return f.ok();
}
Bool Material::loadData(File &f, CChar *path)
{
   MaterialParams &mp=T; Char temp[MAX_LONG_PATH]; Flt sss, old_reflect;
   switch(f.decUIntV())
   {
      case 10:
      {
         f.getMulti(cull, technique)>>mp;
         f.getStr(temp);     base_0.require(temp, path);
         f.getStr(temp);     base_1.require(temp, path);
         f.getStr(temp);     base_2.require(temp, path);
         f.getStr(temp); detail_map.require(temp, path);
         f.getStr(temp);  macro_map.require(temp, path);
         f.getStr(temp);  light_map.require(temp, path);
      }break;

      case 9:
      {
         f.getMulti(cull, technique)>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f.getStr(temp);     base_0.require(temp, path);
         f.getStr(temp);     base_1.require(temp, path);
                             base_2=null;
         f.getStr(temp); detail_map.require(temp, path);
         f.getStr(temp);  macro_map.require(temp, path);
         f.getStr(temp);
         f.getStr(temp);  light_map.require(temp, path);
         f.getStr(temp);
         f.getStr(temp);
      }break;

      case 8:
      {
         f.getMulti(cull, technique)>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f._getStr1(temp);     base_0.require(temp, path);
         f._getStr1(temp);     base_1.require(temp, path);
                               base_2=null;
         f._getStr1(temp); detail_map.require(temp, path);
         f._getStr1(temp);  macro_map.require(temp, path);
         f._getStr1(temp);
         f._getStr1(temp);  light_map.require(temp, path);
         f._getStr1(temp);
         f._getStr1(temp);
      }break;

      case 7:
      {
         f>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect>>cull>>technique; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f._getStr(temp);     base_0.require(temp, path);
         f._getStr(temp);     base_1.require(temp, path);
                              base_2=null;
         f._getStr(temp); detail_map.require(temp, path);
         f._getStr(temp);  macro_map.require(temp, path);
         f._getStr(temp);
         f._getStr(temp);  light_map.require(temp, path);
         f._getStr(temp);
         f._getStr(temp);
      }break;

      case 6:
      {
         f>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect>>cull>>technique; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f._getStr(temp);     base_0.require(temp, path);
         f._getStr(temp);     base_1.require(temp, path);
                              base_2=null;
         f._getStr(temp); detail_map.require(temp, path);
         f._getStr(temp);  macro_map.require(temp, path);
         f._getStr(temp);
         f._getStr(temp);  light_map.require(temp, path);
         f._getStr8();
      }break;

      case 5:
      {
         f>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect>>cull>>technique; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f._getStr(temp);     base_0.require(temp, path);
         f._getStr(temp);     base_1.require(temp, path);
                              base_2=null;
         f._getStr(temp); detail_map.require(temp, path);
         f._getStr(temp);
         f._getStr(temp);  light_map.require(temp, path);
                           macro_map=null;
         f._getStr8();
      }break;

      case 4:
      {
         f>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>tex_scale>>det_scale>>det_power>>old_reflect>>cull>>technique; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         f._getStr(temp);     base_0.require(temp, path);
         f._getStr(temp);     base_1.require(temp, path);
                              base_2=null;
         f._getStr(temp); detail_map.require(temp, path);
         f._getStr(temp);
         f._getStr(temp);  light_map.require(temp, path);
                           macro_map=null;
      }break;

      case 3:
      {
         f>>color_l>>ambient>>smooth>>sss>>glow>>normal>>bump>>det_scale>>det_power>>old_reflect>>cull>>technique; tex_scale=1; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
             base_0.require(f._getStr8(), path);
             base_1.require(f._getStr8(), path);
             base_2=null;
         detail_map.require(f._getStr8(), path);
                            f._getStr8();
          light_map.require(f._getStr8(), path);
          macro_map=null;
      }break;

      case 2:
      {
         f.skip(1);
         f>>color_l>>smooth>>sss>>glow>>normal>>bump>>det_scale>>det_power>>old_reflect>>cull>>technique; ambient=0; tex_scale=1; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         if(technique==MTECH_FUR){det_power=color_l.w; color_l.w=1;}
             base_0.require(f._getStr8(), path);
             base_1.require(f._getStr8(), path);
             base_2=null;
         detail_map.require(f._getStr8(), path);
                            f._getStr8();
          light_map=null;
          macro_map=null;
      }break;

      case 1:
      {
         f.skip(1);
         f>>color_l>>smooth>>glow>>normal>>bump>>det_scale>>det_power>>old_reflect>>cull>>technique; sss=0; ambient=0; tex_scale=1; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         if(technique==MTECH_FUR){det_power=color_l.w; color_l.w=1;}
             base_0.require(f._getStr8(), path);
             base_1.require(f._getStr8(), path);
             base_2=null;
         detail_map.require(f._getStr8(), path);
                            f._getStr8();
          light_map=null;
          macro_map=null;
      }break;

      case 0:
      {
         f.skip(1);
         f>>color_l>>smooth>>glow>>normal>>bump>>det_scale>>det_power>>old_reflect>>cull; sss=0; ambient=0; tex_scale=1; colorS(color_l); reflect=REFLECT_DEFAULT_PAR;
         switch(f.getByte())
         {
            default: technique=MTECH_DEFAULT   ; break;
            case 1 : technique=MTECH_ALPHA_TEST; break;
            case 4 : technique=MTECH_FUR       ; break;
            case 5 : technique=MTECH_GRASS     ; break;
         }
         if(technique==MTECH_FUR){det_power=color_l.w; color_l.w=1;}
         Char8 tex[80];
         f>>tex;     base_0.require(tex, path);
         f>>tex;     base_1.require(tex, path);
                     base_2=null;
         f>>tex; detail_map.require(tex, path);
         f>>tex;
                  light_map=null;
                  macro_map=null;
      }break;

      default: goto error;
   }
   if(f.ok()){validate(); return true;}
error:
   reset(); return false;
}
/******************************************************************************/
Bool Material::save(File &f, CChar *path)C
{
   f.putUInt(CC4_MTRL);
   return saveData(f, path);
}
Bool Material::load(File &f, CChar *path)
{
   if(f.getUInt()==CC4_MTRL)return loadData(f, path);
   reset(); return false;
}

Bool Material::save(C Str &name)C
{
   File f; if(f.writeTry(name)){if(save(f, _GetPath(name)) && f.flush())return true; f.del(); FDelFile(name);}
   return false;
}
Bool Material::load(C Str &name)
{
   File f; if(f.readTry(name))return load(f, _GetPath(name));
   reset(); return false;
}
/******************************************************************************/
void MaterialClear() // must be called: after changing 'Renderer.mode', after changing 'D.alphaFactor', or material parameters/textures
{
         MaterialLast  =null;
   REPAO(MaterialLast4)=null;
}
/******************************************************************************/
UInt CreateBaseTextures(Image &base_0, Image &base_1, Image &base_2, C Image &col, C Image &alpha, C Image &bump, C Image &normal, C Image &smooth, C Image &reflect, C Image &glow, Bool resize_to_pow2, Bool flip_normal_y, FILTER_TYPE filter)
{
   // FIXME
   UInt  ret=0;
   Image dest_0, dest_1;
   {
      Image     col_temp; C Image *    col_src=&    col; if(    col_src->compressed())if(    col_src->copyTry(    col_temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))    col_src=&    col_temp;else goto error;
      Image   alpha_temp; C Image *  alpha_src=&  alpha; if(  alpha_src->compressed())if(  alpha_src->copyTry(  alpha_temp, -1, -1, -1, IMAGE_L8A8         , IMAGE_SOFT, 1))  alpha_src=&  alpha_temp;else goto error;
      Image    bump_temp; C Image *   bump_src=&   bump; if(   bump_src->compressed())if(   bump_src->copyTry(   bump_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1))   bump_src=&   bump_temp;else goto error;
      Image  normal_temp; C Image * normal_src=& normal; if( normal_src->compressed())if( normal_src->copyTry( normal_temp, -1, -1, -1, IMAGE_R8G8         , IMAGE_SOFT, 1)) normal_src=& normal_temp;else goto error;
      Image  smooth_temp; C Image * smooth_src=& smooth; if( smooth_src->compressed())if( smooth_src->copyTry( smooth_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1)) smooth_src=& smooth_temp;else goto error;
      Image reflect_temp; C Image *reflect_src=&reflect; if(reflect_src->compressed())if(reflect_src->copyTry(reflect_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1))reflect_src=&reflect_temp;else goto error;
      Image    glow_temp; C Image *   glow_src=&   glow; if(   glow_src->compressed())if(   glow_src->copyTry(   glow_temp, -1, -1, -1, IMAGE_L8A8         , IMAGE_SOFT, 1))   glow_src=&   glow_temp;else goto error;

      // set alpha
      if(!alpha_src->is() && ImageTI[col_src->type()].a) // if there's no alpha map but there is alpha in color map
      {
         Byte min_alpha=255;
         alpha_src=&alpha_temp.create(col_src->w(), col_src->h(), 1, IMAGE_A8, IMAGE_SOFT, 1);
         if(col_src->lockRead())
         {
            REPD(y, col_src->h())
            REPD(x, col_src->w())
            {
               Byte a=col_src->color(x, y).a;
                    alpha_temp.pixel(x, y, a);
               MIN(min_alpha, a);
            }
            col_src->unlock();
         }
         if(min_alpha>=253)alpha_temp.del(); // alpha channel in color map is almost fully white
      }else
      if(alpha_src->is() && ImageTI[alpha_src->type()].channels>1 && ImageTI[alpha_src->type()].a) // if alpha has both RGB and Alpha channels, then check which one to use
         if(alpha_src->lockRead())
      {
         Byte min_alpha=255, min_lum=255;
         REPD(y, alpha_src->h())
         REPD(x, alpha_src->w())
         {
            Color c=alpha_src->color(x, y);
            MIN(min_alpha, c.a    );
            MIN(min_lum  , c.lum());
         }
         alpha_src->unlock();
         if(min_alpha>=253 && min_lum<253)if(alpha_src->copyTry(alpha_temp, -1, -1, -1, IMAGE_L8, IMAGE_SOFT, 1))alpha_src=&alpha_temp;else goto error; // alpha channel is almost fully white -> use luminance as alpha
      }

      // if we're using two textures
      Bool tex2=(bump_src->is() || normal_src->is() || smooth_src->is() || glow_src->is());

      // set what textures do we have (set this before 'normal' is generated from 'bump')
      if(    col_src->is())ret|=BT_COLOR  ;
      if(  alpha_src->is())ret|=BT_ALPHA  ;
      if(   bump_src->is())ret|=BT_BUMP   ;
      if( normal_src->is())ret|=BT_NORMAL ;
      if( smooth_src->is())ret|=BT_SMOOTH ;
      if(reflect_src->is())ret|=BT_REFLECT;
      if(   glow_src->is())ret|=BT_GLOW   ;

      // generate textures, below operate on separate set of temporary images, in case one source image is used for both texture, but they will be used at different sizes (to avoid double stretching and loss of quality)

      // 1st texture
      if(tex2) // put bump in W channel
      {
         Int w=Max(col_src->w(), bump_src->w()),
             h=Max(col_src->h(), bump_src->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
         Image  col_temp; C Image *cs= col_src; if(cs->is() && (cs->w()!=w || cs->h()!=h))if(cs->copyTry( col_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))cs=& col_temp;else goto error;
         Image bump_temp; C Image *bs=bump_src; if(bs->is() && (bs->w()!=w || bs->h()!=h))if(bs->copyTry(bump_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))bs=&bump_temp;else goto error;
         dest_0.createSoftTry(w, h, 1, IMAGE_R8G8B8A8_SRGB);
         if(!cs->is() || cs->lockRead())
         {
            if(!bs->is() || bs->lockRead())
            {
               REPD(y, dest_0.h())
               REPD(x, dest_0.w()){Color c=(cs->is() ? cs->color(x, y) : WHITE); c.a=(bs->is() ? bs->color(x, y).lum() : BUMP_DEFAULT_TEX); dest_0.color(x, y, c);}
               bs->unlock();
            }
            cs->unlock();
         }
      }else // put alpha in W channel
      {
         Int w=Max(col_src->w(), alpha_src->w()),
             h=Max(col_src->h(), alpha_src->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
         Image   col_temp; C Image *cs=  col_src; if(cs->is() && (cs->w()!=w || cs->h()!=h))if(cs->copyTry(  col_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))cs=&  col_temp;else goto error;
         Image alpha_temp; C Image *as=alpha_src; if(as->is() && (as->w()!=w || as->h()!=h))if(as->copyTry(alpha_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))as=&alpha_temp;else goto error;
         dest_0.createSoftTry(w, h, 1, IMAGE_R8G8B8A8_SRGB);
         if(!cs->is() || cs->lockRead())
         {
            if(!as->is() || as->lockRead())
            {
               Int alpha_component=(ImageTI[as->type()].a ? 3 : 0); // use Alpha or Red in case src is R8
               REPD(y, dest_0.h())
               REPD(x, dest_0.w()){Color c=(cs->is() ? cs->color(x, y) : WHITE); c.a=(as->is() ? as->color(x, y).c[alpha_component] : 255); dest_0.color(x, y, c);} // full alpha
               as->unlock();
            }
            cs->unlock();
         }
      }

      // 2nd texture
      if(tex2)
      {
         Int w=Max(normal_src->w(), smooth_src->w(), alpha_src->w(), glow_src->w()),
             h=Max(normal_src->h(), smooth_src->h(), alpha_src->h(), glow_src->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}

       C Image *bump=null;
         if(  bump_src->is() && !normal_src->is()           )bump=  bump_src;else // if bump available and normal not, then create normal from bump
         if(normal_src->is() &&  normal_src->monochromatic())bump=normal_src;     // if normal is provided as monochromatic, then treat it as bump and convert to normal
         if(bump) // create normal from bump
         {
            MAX(w, bump->w());
            MAX(h, bump->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
            if(bump->w()!=w || bump->h()!=h)if(bump->copyTry(normal_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))bump=&normal_temp;else goto error;
            bump->bumpToNormal(normal_temp, AvgF(w, h)*BUMP_NORMAL_SCALE); normal_src=&normal_temp;
            flip_normal_y=false; // no need to flip since normal map generated from bump is always correct
         }

         dest_1.createSoftTry(w, h, 1, IMAGE_R8G8B8A8);

         Image normal_temp; C Image *ns=normal_src; if(ns->is() && (ns->w()!=w || ns->h()!=h))if(ns->copyTry(normal_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))ns=&normal_temp;else goto error;
         Image smooth_temp; C Image *ss=smooth_src; if(ss->is() && (ss->w()!=w || ss->h()!=h))if(ss->copyTry(smooth_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))ss=&smooth_temp;else goto error;
         Image  alpha_temp; C Image *as= alpha_src; if(as->is() && (as->w()!=w || as->h()!=h))if(as->copyTry( alpha_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))as=& alpha_temp;else goto error;
         Image   glow_temp; C Image *gs=  glow_src; if(gs->is() && (gs->w()!=w || gs->h()!=h))if(gs->copyTry(  glow_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))gs=&  glow_temp;else goto error;

         if(!ns->is() || ns->lockRead())
         {
            if(!ss->is() || ss->lockRead())
            {
               if(!as->is() || as->lockRead())
               {
                  if(!gs->is() || gs->lockRead())
                  {
                     Int alpha_component=(ImageTI[as->type()].a ? 3 : 0); // use Alpha or Red in case src is R8
                     REPD(y, dest_1.h())
                     REPD(x, dest_1.w())
                     {
                        Color nrm =(ns->is() ? ns->color(x, y)       : Color(128, 128, 255, 0)); if(flip_normal_y)nrm.g=255-nrm.g;
                        Byte  spec=(ss->is() ? ss->color(x, y).lum() : 255);
                        Byte  alpha_glow;
                        if(as->is())                          alpha_glow=as->color(x, y).c[alpha_component];else
                        if(gs->is()){Color c=gs->color(x, y); alpha_glow=DivRound(c.lum()*c.a, 255);}else
                                                              alpha_glow=255;
                        Color c;
                        c.c[  NRMX_CHANNEL]=nrm.r;
                        c.c[  NRMY_CHANNEL]=nrm.g;
                        c.c[SMOOTH_CHANNEL]=spec;
                        c.c[  GLOW_CHANNEL]=alpha_glow;
                        dest_1.color(x, y, c);
                     }
                     gs->unlock();
                  }
                  as->unlock();
               }
               ss->unlock();
            }
            ns->unlock();
         }
      }
   }

error:
   Swap(dest_0, base_0);
   Swap(dest_1, base_1);
   return ret;
}
void CreateDetailTexture(Image &detail, C Image &col, C Image &bump, C Image &normal, Bool resize_to_pow2, Bool flip_normal_y, FILTER_TYPE filter)
{
   Image dest;
   {
      Image    col_temp; C Image *   col_src=&   col; if(   col_src->compressed())if(   col_src->copyTry(   col_temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))   col_src=&   col_temp;else goto error;
      Image   bump_temp; C Image *  bump_src=&  bump; if(  bump_src->compressed())if(  bump_src->copyTry(  bump_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1))  bump_src=&  bump_temp;else goto error;
      Image normal_temp; C Image *normal_src=&normal; if(normal_src->compressed())if(normal_src->copyTry(normal_temp, -1, -1, -1, IMAGE_R8G8         , IMAGE_SOFT, 1))normal_src=&normal_temp;else goto error;

      Int w=Max(col_src->w(), bump_src->w(), normal_src->w()),
          h=Max(col_src->h(), bump_src->h(), normal_src->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}

    C Image *cs=   col_src; if(cs->is() && (cs->w()!=w || cs->h()!=h))if(cs->copyTry(   col_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))cs=&   col_temp;else goto error;
    C Image *bs=  bump_src; if(bs->is() && (bs->w()!=w || bs->h()!=h))if(bs->copyTry(  bump_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))bs=&  bump_temp;else goto error;
    C Image *ns=normal_src; if(ns->is() && (ns->w()!=w || ns->h()!=h))if(ns->copyTry(normal_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))ns=&normal_temp;else goto error;

      if(bs->is() && !ns->is()){bs->bumpToNormal(normal_temp, AvgF(w, h)*BUMP_NORMAL_SCALE); ns=&normal_temp; flip_normal_y=false;} // create normal from bump map, no need to flip since normal map generated from bump is always correct

      dest.createSoftTry(w, h, 1, IMAGE_R8G8B8A8);

      REPD(y, dest.h())
      REPD(x, dest.w())
      {
         Color nrm =(ns->is() ? ns->color(x, y)       : Color(128, 128, 255, 0)); if(flip_normal_y)nrm.g=255-nrm.g;
         Byte  col =(cs->is() ? cs->color(x, y).lum() : 128);
         Byte  bump=(bs->is() ? bs->color(x, y).lum() : BUMP_DEFAULT_TEX);
         dest.color(x, y, Color(nrm.r, nrm.g, col, bump)); // #MaterialTextureChannelOrder
      }
   }

error:
   Swap(dest, detail);
}
Bool CreateBumpFromColor(Image &bump, C Image &col, Flt min_blur_range, Flt max_blur_range, Threads *threads)
{
   Image col_temp; C Image *col_src=&col; if(col_src->compressed())if(col_src->copyTry(col_temp, -1, -1, -1, ImageTypeUncompressed(col_src->type()), IMAGE_SOFT, 1))col_src=&col_temp;else goto error;
   {
      Image bump_temp; if(bump_temp.createSoftTry(col.w(), col.h(), 1, IMAGE_F32)) // operate on temporary in case "&bump==&col", create as high precision to get good quality for blur/normalize
      {
         if(col_src->lockRead())
         {
            REPD(y, bump_temp.h())
            REPD(x, bump_temp.w())bump_temp.pixF(x, y)=col_src->colorF(x, y).xyz.max();
            col_src->unlock();

            if(min_blur_range<0)min_blur_range=0; // auto
            if(max_blur_range<0)max_blur_range=3; // auto
            Bool  first=true;
            Flt   power=1;
            Image bump_step;
            for(Flt blur=max_blur_range; ; ) // start with max range, because it's the most important, so we want it to be precise, and then we will go with half steps down
            {
               bump_temp.blur(first ? bump : bump_step, blur, false, threads); // always set the first blur into 'bump' to set it as base, or in case we finish after one step
               if(!first)
               {
                  REPD(y, bump.h())
                  REPD(x, bump.w())bump.pixF(x, y)+=bump_step.pixF(x, y)*power;
               }
               if(blur<=min_blur_range)break;
               first =false;
               blur *=0.5f; if(blur<1)blur=0; // if we reach below 1 blur, then go straight to 0 to avoid doing 0.5, 0.25, 0.125, ..
               power*=0.5f;
            }
            bump.normalize();
            return true;
         }
      }
   }
error:
   bump.del(); return false;
}
/******************************************************************************/
static inline Flt LightSpecular(C Vec &nrm, C Vec &light_dir, C Vec &eye_dir, Flt power=64)
{
#if 1 // blinn
   return Pow(Sat(Dot(nrm, !(light_dir+eye_dir))), power);
#else // phong
   Vec reflection=!(nrm*(2*Dot(nrm, light_dir)) - light_dir);
   return Pow(Sat(Dot(reflection, eye_dir)), power);
#endif
}
Bool MergeBaseTextures(Image &base_0, C Material &material, Int image_type, Int max_image_size, C Vec *light_dir, Flt light_power, Flt spec_mul, FILTER_TYPE filter)
{//FIXME
   if(material.base_0 && material.base_0->is()
   && material.base_1 && material.base_1->is()) // if have both textures
   {
      // dimensions
      VecI2 size=Max(material.base_0->size(), material.base_1->size());
      if(max_image_size>0)
      {
         MIN(size.x, max_image_size);
         MIN(size.y, max_image_size);
      }

      Image color; // operate on temp variable in case 'base_0' argument is set to other images used in this func
      if(material.base_0->copyTry(color, size.x, size.y, 1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1, filter, IC_WRAP)) // create new color map, use IMAGE_R8G8B8A8_SRGB to always include Alpha
      {
         Image b1; // 'base_1' resized to 'color' resolution
         MAX(light_power, 0);
              spec_mul*=material.smooth*light_power/255.0f;
         Flt   nrm_mul =material.normal            /127.0f,
              glow_mul =material.glow*(2*1.75f/255.0f), // *2 because shaders use this multiplier, *1.75 because shaders iterate over few pixels and take the max out of them (this is just approximation)
              glow_blur=0.07f;
         Bool has_alpha=material.hasAlpha(),
              has_nrm  =( light_dir && material.normal*light_power>0.01f),
              has_spec =( light_dir && material.smooth*light_power>0.01f),
              has_glow =(!has_alpha && material.glow              >0.01f);
         if( (has_alpha || has_nrm || has_spec || has_glow) && material.base_1->copyTry(b1, color.w(), color.h(), 1, ImageTypeUncompressed(material.base_1->type()), IMAGE_SOFT, 1, filter, IC_WRAP))
         {
            // setup alpha
            if(has_alpha)
               REPD(y, color.h())
               REPD(x, color.w()){Color c=color.color(x, y); c.a=b1.color(x, y).c[GLOW_CHANNEL]; color.color(x, y, c);}

            // setup glow (before baking normals)
            Image glow; if(has_glow && glow.createSoftTry(color.w(), color.h(), 1, IMAGE_F32_3)) // use Vec because we're storing glow with multiplier
            {
               REPD(y, color.h())
               REPD(x, color.w())
               {
                  Vec4 c=color.colorF(x, y)                ; // RGB
                  Byte g=b1   .color (x, y).c[GLOW_CHANNEL]; // glow
                  c.xyz*=g*glow_mul;
                  glow.colorF(x, y, c);
               }
               glow.blur(RoundPos(glow.size().avgF()*glow_blur), false);
            }

            // bake normal map
            if(has_nrm || has_spec)
            {
               Flt light=Sat(light_dir->z)*light_power, // light intensity at flat normal without ambient
                 ambient=1-light; // setup ambient so light intensity at flat normal is 1
               REPD(y, color.h())
               REPD(x, color.w())
               {
                  // I'm assuming that the texture plane is XY plane with Z=0, and facing us (towards -Z) just like browsing image in some viewer
                  Color col=color.color(x, y),
                        nrm=b1   .color(x, y);
                  Vec n;
                  n.x=(nrm.c[NRMX_CHANNEL]-128)*nrm_mul; // NrmX increases towards right
                  n.y=(128-nrm.c[NRMY_CHANNEL])*nrm_mul; // NrmY increases towards down
                  n.z=-CalcZ(n.xy);
                  n.normalize();

                  if(has_nrm)
                  {
                     Flt d=Sat(-Dot(n, *light_dir)), l=ambient + light_power*d;
                     col=ColorBrightness(col, l);
                  }
                  if(has_spec)if(Byte s=nrm.c[SMOOTH_CHANNEL])
                  {
                     Flt spec=LightSpecular(-n, *light_dir, Vec(0, 0, 1))*spec_mul;
                     Color cs=ColorBrightness(s*spec); cs.a=0;
                     col=ColorAdd(col, cs);
                  }
                  color.color(x, y, col);
               }
            }

            // apply glow map (after baking normal)
            if(glow.is())
               REPD(y, color.h())
               REPD(x, color.w())
            {
               Color c=color.color(x, y),
                     g=glow .color(x, y); g.a=0;
               color.color(x, y, ColorAdd(c, g));
            }
         }else has_alpha=false; // disable alpha if failed to copy image

         if(!has_alpha) // set full alpha channel
            REPD(y, color.h())
            REPD(x, color.w()){Color c=color.color(x, y); c.a=255; color.color(x, y, c);}

         // image type
         if(image_type<=0)
         {
            image_type=material.base_0->type();
            if(has_alpha)image_type=ImageTypeIncludeAlpha(IMAGE_TYPE(image_type)); // convert image type to one with    alpha channel
            else         image_type=ImageTypeExcludeAlpha(IMAGE_TYPE(image_type)); // convert image type to one without alpha channel
         }
         if(image_type==IMAGE_PVRTC1_2 || image_type==IMAGE_PVRTC1_4 || image_type==IMAGE_PVRTC1_2_SRGB || image_type==IMAGE_PVRTC1_4_SRGB)size=NearestPow2(size.avgI()); // PVRTC1 must be square and pow2

         // final copy
         if(color.copyTry(color, size.x, size.y, 1, image_type, material.base_0->mode(), (material.base_0->mipMaps()>1) ? 0 : 1, filter, IC_WRAP))
         {
            Swap(base_0, color);
            return true;
         }
      }
   }
   return false;
}
/******************************************************************************/
static Bool CanBeRemoved(C Material &material) {return material.canBeRemoved();} // Renderer Instancing doesn't use incRef/decRef for more performance, so we need to do additional checking for materials if they can be removed from cache, by checking if they're not assigned to any instance

void ShutMaterial() {Materials.del();}
void InitMaterial()
{
   MaterialDefault.cull=true;
   MaterialDefault.validate();

   MaterialDefaultNoCull=MaterialDefault;
   MaterialDefaultNoCull.cull=false;

   Materials.canBeRemoved(CanBeRemoved);
}
/******************************************************************************/
}
/******************************************************************************/
