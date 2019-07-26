/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
#include "Shader Compiler.h"
/******************************************************************************/
#define MULTI_MATERIAL 1

#if WINDOWS
   #define COMPILE_DX 0
   #define COMPILE_GL 0
#endif

/**/
#define MAIN

#define DEFERRED
#define BLEND_LIGHT
//#define FORWARD

#define AMBIENT
#define AMBIENT_OCCLUSION
#define BEHIND
#define BLEND
#define BONE_HIGHLIGHT
#define DEPTH_OF_FIELD
#define EARLY_Z
#define EFFECTS_2D
#define EFFECTS_3D
#define FOG_LOCAL
#define FUR
#define FXAA
#define HDR
#define LAYERED_CLOUDS
#define MOTION_BLUR
#define OVERLAY
#define POSITION
#define SET_COLOR
#define VOLUMETRIC_CLOUDS
#define VOLUMETRIC_LIGHTS
#define WATER
#define WORLD_EDITOR
/******************************************************************************
#define DX10_INPUT_LAYOUT
/******************************************************************************/
// SHADER NAMES
/******************************************************************************/
Str8 ShaderDeferred  (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int detail, Int macro, Int reflect, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tesselate) {return S8+skin+materials+textures+bump_mode+alpha_test+detail+macro+reflect+color+mtrl_blend+heightmap+fx+tesselate;}
Str8 ShaderBlendLight(Int skin, Int color    , Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int reflect, Int fx, Int per_pixel, Int shadow_maps, Int tesselate) {return S8+skin+color+textures+bump_mode+alpha_test+alpha+light_map+reflect+fx+per_pixel+shadow_maps+tesselate;}
Str8 ShaderForward   (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int reflect, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tesselate) {return S8+skin+materials+textures+bump_mode+alpha_test+light_map+detail+reflect+color+mtrl_blend+heightmap+fx+per_pixel+light_dir+light_dir_shd+light_dir_shd_num+light_point+light_point_shd+light_linear+light_linear_shd+light_cone+light_cone_shd+tesselate;}

Str8 ShaderAmbient   (Int skin, Int alpha_test, Int light_map) {return S8+skin+alpha_test+light_map;}
Str8 ShaderBehind    (Int skin, Int textures) {return S8+skin+textures;}
Str8 ShaderBlend     (Int skin, Int color, Int reflect, Int textures) {return S8+skin+color+reflect+textures;}
Str8 ShaderEarlyZ    (Int skin) {return S8+skin;}
Str8 ShaderFurBase   (Int skin, Int size, Int diffuse) {return S8+"Base"+skin+size+diffuse;}
Str8 ShaderFurSoft   (Int skin, Int size, Int diffuse) {return S8+"Soft"+skin+size+diffuse;}
Str8 ShaderOverlay   (Int skin, Int normal) {return S8+skin+normal;}
Str8 ShaderPosition  (Int skin, Int textures, Int test_blend, Int fx, Int tesselate) {return S8+skin+textures+test_blend+fx+tesselate;}
Str8 ShaderSetColor  (Int skin, Int textures, Int tesselate) {return S8+skin+textures+tesselate;}
Str8 ShaderTattoo    (Int skin, Int tesselate) {return S8+skin+tesselate;}
/******************************************************************************/
#if COMPILE_DX || COMPILE_GL
/******************************************************************************/
static Memx<ShaderCompiler> ShaderCompilers; // use 'Memx' because we store pointers to 'ShaderCompiler'
/******************************************************************************/
// LISTING ALL SHADER TECHNIQUES
/******************************************************************************/
static void Compile(API api)
{
   if(!DataPath().is())Exit("Can't compile default shaders - 'DataPath' not specified");

   Str src_path=GetPath(GetPath(__FILE__))+"\\Shaders\\", // for example "C:/Esenthel/Engine/Src/Shaders/"
      dest_path=DataPath();                               // for example "C:/Esenthel/Data/Shader/4/"
   switch(api)
   {
      default: return;

      case API_GL: dest_path+="Shader\\GL\\"; break;
      case API_DX: dest_path+="Shader\\4\\" ; break;
   }
   FCreateDirs(dest_path);
   SHADER_MODEL model=SM_4;

   Bool ms  =(api==API_DX), // if support multi-sampling in shaders
        tess=(api==API_DX); // if support tesselation    in shaders
   Int fxs[]={FX_GRASS, FX_LEAF, FX_LEAFS};

#ifdef MAIN
{
   ShaderCompiler &compiler=ShaderCompilers.New().set(dest_path+"Main", model, api);
   {
      ShaderCompiler::Source &src=compiler.New(src_path+"Main.cpp");
                     src.New("Draw2DFlat", "Draw2DFlat_VS", "DrawFlat_PS");
                     src.New("Draw3DFlat", "Draw3DFlat_VS", "DrawFlat_PS");
      if(api!=API_DX)src.New("SetCol"    , "Draw_VS"      , "DrawFlat_PS"); // this version fails on DX
      else           src.New("SetCol"    , "SetCol_VS"    , "SetCol_PS"  ); // THERE IS A BUG ON NVIDIA GEFORCE DX10+ when trying to clear normal render target using SetCol "Bool clear_nrm=(_nrm && !NRM_CLEAR_START && ClearNrm());", with D.depth2DOn(true) entire RT is cleared instead of background pixels only, this was verified on Windows 10 GeForce 650m, drivers 381, this version works OK on DX, TODO: check again in the future and remove SetCol_VS SetCol_PS

      src.New("Draw2DCol", "Draw2DCol_VS", "Draw2DCol_PS");
      src.New("Draw3DCol", "Draw3DCol_VS", "Draw3DCol_PS");

      src.New("Draw2DTex" , "Draw2DTex_VS",  "Draw2DTex_PS");
      src.New("Draw2DTexC", "Draw2DTex_VS", "Draw2DTexC_PS");

      src.New("DrawTexX", "Draw2DTex_VS", "DrawTexX_PS");
      src.New("DrawTexY", "Draw2DTex_VS", "DrawTexY_PS");
      src.New("DrawTexZ", "Draw2DTex_VS", "DrawTexZ_PS");
      src.New("DrawTexW", "Draw2DTex_VS", "DrawTexW_PS");

      src.New("DrawTexXG", "Draw2DTex_VS", "DrawTexXG_PS");
      src.New("DrawTexYG", "Draw2DTex_VS", "DrawTexYG_PS");
      src.New("DrawTexZG", "Draw2DTex_VS", "DrawTexZG_PS");
      src.New("DrawTexWG", "Draw2DTex_VS", "DrawTexWG_PS");

      src.New("DrawTexNrm", "Draw2DTex_VS", "DrawTexNrm_PS");
      src.New("Draw"      ,      "Draw_VS",  "Draw2DTex_PS");
      src.New("DrawC"     ,      "Draw_VS", "Draw2DTexC_PS");
      src.New("DrawCG"    ,      "Draw_VS", "DrawTexCG_PS");
      src.New("DrawG"     ,      "Draw_VS", "DrawTexG_PS");
      src.New("DrawA"     ,      "Draw_VS", "Draw2DTexA_PS");

      src.New("DrawX" , "Draw_VS", "DrawX_PS" );
      src.New("DrawXG", "Draw_VS", "DrawXG_PS");
      REPD(dither, 2)
      REPD(gamma , 2)src.New("DrawXC", "Draw_VS", "DrawXC_PS")("DITHER", dither, "GAMMA", gamma);

      src.New("DrawTexPoint" , "Draw2DTex_VS", "DrawTexPoint_PS");
      src.New("DrawTexPointC", "Draw2DTex_VS", "DrawTexPointC_PS");

      src.New("Draw2DTexCol", "Draw2DTexCol_VS", "Draw2DTexCol_PS");

      REPD(alpha_test, 2)
      REPD(color     , 2)
      {
                     src.New("Draw2DDepthTex", "Draw2DDepthTex_VS", "Draw2DDepthTex_PS")("ALPHA_TEST", alpha_test, "COLORS", color);
         REPD(fog, 2)src.New("Draw3DTex"     , "Draw3DTex_VS"     , "Draw3DTex_PS"     )("ALPHA_TEST", alpha_test, "COLORS", color, "FOG", fog);
      }

      if(ms) // Multi-Sampling
      {
         src.New("DrawMs1", "DrawPixel_VS", "DrawMs1_PS");
         src.New("DrawMsN", "DrawPixel_VS", "DrawMsN_PS");
         src.New("DrawMsM", "DrawPixel_VS", "DrawMsM_PS").multiSample();

         src.New("DetectMSCol", "DrawPixel_VS", "DetectMSCol_PS");
       //src.New("DetectMSNrm", "DrawPixel_VS", "DetectMSNrm_PS");
      }

      src.New("DrawMask", "DrawMask_VS", "DrawMask_PS");
      src.New("DrawCubeFace", "DrawCubeFace_VS", "DrawCubeFace_PS");
      src.New("Simple", "Simple_VS", "Simple_PS");

      src.New("Dither", "Draw_VS", "Dither_PS");

                              src.New("CombineSSAlpha", "Draw_VS", "CombineSSAlpha_PS");
      REPD(sample, ms ? 3 : 2)src.New("Combine"       , "Draw_VS", "Combine_PS")("SAMPLE", sample);

      if(ms)src.New("ResolveDepth", "DrawPixel_VS", "ResolveDepth_PS");
      src.New("SetDepth", "Draw_VS", "SetDepth_PS");
      src.New("DrawDepth", "Draw_VS", "DrawDepth_PS");

      REPD(perspective, 2)
      {
                src.New("LinearizeDepth0", "Draw_VS"     , "LinearizeDepth0_PS")("PERSPECTIVE", perspective);
         if(ms){src.New("LinearizeDepth1", "DrawPixel_VS", "LinearizeDepth1_PS")("PERSPECTIVE", perspective);
                src.New("LinearizeDepth2", "DrawPixel_VS", "LinearizeDepth2_PS")("PERSPECTIVE", perspective).multiSample();
         }
      }

      src.New("EdgeDetect"     , "DrawPosXY_VS",      "EdgeDetect_PS");
      src.New("EdgeDetectApply", "Draw_VS"     , "EdgeDetectApply_PS");

      src.New("PaletteDraw", "Draw_VS", "PaletteDraw_PS");

      if(api==API_GL)src.New("WebLToS", "Draw_VS", "WebLToS_PS"); // #WebSRGB

      src.New("Params0", S, "Params0_PS").dummy=true;
      src.New("Params1", S, "Params1_PS").dummy=true;
   }
   { // BLOOM
      ShaderCompiler::Source &src=compiler.New(src_path+"Bloom.cpp");
      REPD(glow    , 2)
      REPD(clamp   , 2)
      REPD(half_res, 2)
      REPD(saturate, 2)
      REPD(gamma   , 2)src.New("BloomDS", "BloomDS_VS", "BloomDS_PS")("GLOW", glow, "CLAMP", clamp, "HALF_RES", half_res, "SATURATE", saturate)("GAMMA", gamma);

      REPD(dither, 2)
      REPD(gamma , 2)src.New("Bloom", "Draw_VS", "Bloom_PS")("DITHER", dither, "GAMMA", gamma);
   }
   { // BLUR
      ShaderCompiler::Source &src=compiler.New(src_path+"Blur.cpp");
      src.New("BlurX", "Draw_VS", "BlurX_PS")("SAMPLES", 4);
      src.New("BlurX", "Draw_VS", "BlurX_PS")("SAMPLES", 6);
      src.New("BlurY", "Draw_VS", "BlurY_PS")("SAMPLES", 4);
      src.New("BlurY", "Draw_VS", "BlurY_PS")("SAMPLES", 6);

      src.New("BlurX_X", "Draw_VS", "BlurX_X_PS");
      src.New("BlurY_X", "Draw_VS", "BlurY_X_PS");

      src.New("MaxX", "Draw_VS", "MaxX_PS");
      src.New("MaxY", "Draw_VS", "MaxY_PS");
   }
   { // CUBIC
      ShaderCompiler::Source &src=compiler.New(src_path+"Cubic.cpp");
      REPD(color, 2)
      {
         src.New("DrawTexCubicFast", "Draw2DTex_VS", "DrawTexCubicFast_PS")("COLORS", color);
         src.New("DrawTexCubic"    , "Draw2DTex_VS", "DrawTexCubic_PS"    )("COLORS", color);
      }
      REPD(dither, 2)
      {
         src.New("DrawTexCubicFastF"   , "Draw_VS", "DrawTexCubicFast_PS"   )("DITHER", dither);
         src.New("DrawTexCubicFastFRGB", "Draw_VS", "DrawTexCubicFastRGB_PS")("DITHER", dither);
         src.New("DrawTexCubicF"       , "Draw_VS", "DrawTexCubic_PS"       )("DITHER", dither);
         src.New("DrawTexCubicFRGB"    , "Draw_VS", "DrawTexCubicRGB_PS"    )("DITHER", dither);
      }
   }
   { // FOG
      ShaderCompiler::Source &src=compiler.New(src_path+"Fog.cpp");
      REPD(multi_sample, ms ? 3 : 1)src.New("Fog", "DrawPosXY_VS", "Fog_PS")("MULTI_SAMPLE", multi_sample).multiSample(multi_sample>=2);
   }
   { // FONT
      ShaderCompiler::Source &src=compiler.New(src_path+"Font.cpp");
      REPD(depth, 2)
      REPD(gamma, 2)
      {
         src.New("Font"  , "Font_VS"  , "Font_PS"  )("SET_DEPTH", depth, "GAMMA", gamma);
         src.New("FontSP", "FontSP_VS", "FontSP_PS")("SET_DEPTH", depth, "GAMMA", gamma);
      }
   }
   { // LIGHT
      ShaderCompiler::Source &src=compiler.New(src_path+"Light.cpp");
      REPD(shadow      , 2)
      REPD(multi_sample, ms ? 2 : 1)
      REPD(quality     , multi_sample ? 1 : 2) // no Quality version for MSAA
      {
                       src.New("DrawLightDir"   , "DrawPosXY_VS", "LightDir_PS"   ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
                       src.New("DrawLightPoint" , "DrawPosXY_VS", "LightPoint_PS" ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
                       src.New("DrawLightLinear", "DrawPosXY_VS", "LightLinear_PS").multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality);
         REPD(image, 2)src.New("DrawLightCone"  , "DrawPosXY_VS", "LightCone_PS"  ).multiSample(multi_sample)("SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "QUALITY", quality, "IMAGE", image);
      }
   }
   { // LIGHT APPLY
      ShaderCompiler::Source &src=compiler.New(src_path+"Light Apply.cpp");
      REPD(multi_sample, ms ? 3 : 1)
      REPD(ao          , 2)
      REPD(  cel_shade , 2)
      REPD(night_shade , 2)
         src.New("ApplyLight", "Draw_VS", "ApplyLight_PS")("MULTI_SAMPLE", multi_sample, "AO", ao, "CEL_SHADE", cel_shade, "NIGHT_SHADE", night_shade);
   }
   { // SHADOW
      ShaderCompiler::Source &src=compiler.New(src_path+"Shadow.cpp");
      REPD(multi_sample, ms ? 2 : 1)
      {
         REPD(map_num, 6)
         REPD(cloud  , 2)src.New("ShdDir"  , "DrawPosXY_VS", "ShdDir_PS"  ).multiSample(multi_sample)("MULTI_SAMPLE", multi_sample, "MAP_NUM", map_num+1, "CLOUD", cloud);
                         src.New("ShdPoint", "DrawPosXY_VS", "ShdPoint_PS").multiSample(multi_sample)("MULTI_SAMPLE", multi_sample);
                         src.New("ShdCone" , "DrawPosXY_VS", "ShdCone_PS" ).multiSample(multi_sample)("MULTI_SAMPLE", multi_sample);
      }
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 4);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 5);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 6);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 8);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 9);
      src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 12);
    //src.New("ShdBlur" , "Draw_VS", "ShdBlur_PS" )("SAMPLES", 13);
    //src.New("ShdBlurX", "Draw_VS", "ShdBlurX_PS")("RANGE", 1);
    //src.New("ShdBlurY", "Draw_VS", "ShdBlurY_PS")("RANGE", 1);
      src.New("ShdBlurX", "Draw_VS", "ShdBlurX_PS")("RANGE", 2);
      src.New("ShdBlurY", "Draw_VS", "ShdBlurY_PS")("RANGE", 2);
   }
   { // OUTLINE
      ShaderCompiler::Source &src=compiler.New(src_path+"Outline.cpp");
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 0, "CLIP", 0);
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 1, "CLIP", 0);
      src.New("Outline", "Draw_VS", "Outline_PS")("DOWN_SAMPLE", 0, "CLIP", 1);
      src.New("OutlineApply", "Draw_VS", "OutlineApply_PS");
   }
   { // PARTICLES
      ShaderCompiler::Source &src=compiler.New(src_path+"Particles.cpp");
      REPD(palette             , 2)
      REPD(soft                , 2)
      REPD(anim                , 3)
      REPD(motion_affects_alpha, 2)
         src.New("Particle", "Particle_VS", "Particle_PS")("PALETTE", palette, "SOFT", soft, "ANIM", anim)("MOTION_STRETCH", 1, "MOTION_AFFECTS_ALPHA", motion_affects_alpha);
         src.New("Particle", "Particle_VS", "Particle_PS")("PALETTE",       0, "SOFT",    0, "ANIM",    0)("MOTION_STRETCH", 0, "MOTION_AFFECTS_ALPHA",                    0);
   }
   { // SKY
      ShaderCompiler::Source &src=compiler.New(src_path+"Sky.cpp");
      REPD(dither, 2)
      REPD(cloud , 2)
      {
         // Textures Flat
         REPD(textures, 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", 0, "FLAT", 1, "DENSITY", 0, "TEXTURES", textures+1)("STARS", 0, "DITHER", dither, "PER_VERTEX", 0, "CLOUD", cloud);

         // Atmospheric Flat
         REPD(per_vertex, 2)
         REPD(stars     , 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", 0, "FLAT", 1, "DENSITY", 0, "TEXTURES", 0)("STARS", stars, "DITHER", dither, "PER_VERTEX", per_vertex, "CLOUD", cloud);

         REPD(multi_sample, ms ? 3 : 1)
         {
            // Textures
            REPD(textures, 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", multi_sample, "FLAT", 0, "DENSITY", 0, "TEXTURES", textures+1)("STARS", 0, "DITHER", dither, "PER_VERTEX", 0, "CLOUD", cloud).multiSample(multi_sample>=2);

            // Atmospheric
            REPD(per_vertex, 2)
            REPD(density   , 2)
            REPD(stars     , 2)src.New("Sky", "Sky_VS", "Sky_PS")("MULTI_SAMPLE", multi_sample, "FLAT", 0, "DENSITY", density, "TEXTURES", 0)("STARS", stars, "DITHER", dither, "PER_VERTEX", per_vertex, "CLOUD", cloud).multiSample(multi_sample>=2);
         }
      }
   }
   { // SMAA
      ShaderCompiler::Source &src=compiler.New(src_path+"SMAA.cpp");
      REPD(gamma, 2)src.New("SMAAEdge" , "SMAAEdge_VS" , "SMAAEdge_PS" )("GAMMA", gamma);
                    src.New("SMAABlend", "SMAABlend_VS", "SMAABlend_PS");
                    src.New("SMAA"     , "SMAA_VS"     , "SMAA_PS"     );
                 #if SUPPORT_MLAA
                    src.New("MLAAEdge" , "MLAA_VS", "MLAAEdge_PS" );
                    src.New("MLAABlend", "MLAA_VS", "MLAABlend_PS");
                    src.New("MLAA"     , "MLAA_VS", "MLAA_PS"     );
                 #endif
   }
   { // SUN
      ShaderCompiler::Source &src=compiler.New(src_path+"Sun.cpp");
      REPD(mask, 2)src.New("SunRaysMask", "DrawPosXY_VS", "SunRaysMask_PS")("MASK", mask);

      REPD(mask  , 2)
      REPD(dither, 2)
      REPD(jitter, 2)
      REPD(gamma , 2)
         src.New("SunRays", "DrawPosXY_VS", "SunRays_PS")("MASK", mask, "DITHER", dither, "JITTER", jitter, "GAMMA", gamma);
   }
   { // VIDEO
      ShaderCompiler::Source &src=compiler.New(src_path+"Video.cpp");
      REPD(gamma, 2)
      REPD(alpha, 2)
         src.New("YUV", "Draw2DTex_VS", "YUV_PS")("GAMMA", gamma, "ALPHA", alpha);
   }
}
#endif

#ifdef AMBIENT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Ambient", model, api).New(src_path+"Ambient.cpp");

   REPD(skin      , 2)
   REPD(alpha_test, 3)
   REPD(light_map , 2)
      src.New(S, "VS", "PS")("SKIN", skin, "ALPHA_TEST", alpha_test, "LIGHT_MAP", light_map);
}
#endif

#ifdef AMBIENT_OCCLUSION
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Ambient Occlusion", model, api).New(src_path+"Ambient Occlusion.cpp");
   REPD(mode  , 4)
   REPD(jitter, 2)
   REPD(normal, 2)
      src.New("AO", "AO_VS", "AO_PS")("MODE", mode, "JITTER", jitter, "NORMALS", normal);
}
#endif

#ifdef BEHIND
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Behind", model, api).New(src_path+"Behind.cpp");

   REPD(skin    , 2)
   REPD(textures, 3)
      src.New(S, "VS", "PS")("SKIN", skin, "TEXTURES", textures);
}
#endif

#ifdef BLEND
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Blend", model, api).New(src_path+"Blend.cpp");

   REPD(skin    , 2)
   REPD(color   , 2)
   REPD(reflect , 2)
   REPD(textures, 3)
      src.New(S, "VS", "PS")("SKIN", skin, "COLORS", color, "REFLECT", reflect, "TEXTURES", textures);
}
#endif

#ifdef BONE_HIGHLIGHT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Bone Highlight", model, api).New(src_path+"Bone Highlight.cpp");
   REPD(skin     , 2)
   REPD(bump_mode, 2)src.New(S, "VS", "PS")("SKIN", skin, "BUMP_MODE", bump_mode ? SBUMP_FLAT : SBUMP_ZERO);
}
#endif

#ifdef DEPTH_OF_FIELD
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Depth of Field", model, api).New(src_path+"Depth of Field.cpp");
   REPD(clamp    , 2)
   REPD(realistic, 2)
   REPD(half_res , 2)
   REPD(gather   , 2)src.New("DofDS", "Draw_VS", "DofDS_PS")("CLAMP", clamp, "REALISTIC", realistic, "HALF_RES", half_res, "GATHER", gather).gather(gather);

   for(Int range=2; range<=12; range++)
   {
      src.New("DofBlurX", "Draw_VS", "DofBlurX_PS")("RANGE", range);
      src.New("DofBlurY", "Draw_VS", "DofBlurY_PS")("RANGE", range);
   }

   REPD(dither   , 2)
   REPD(realistic, 2)src.New("Dof", "Draw_VS", "Dof_PS")("DITHER", dither, "REALISTIC", realistic);
}
#endif

#ifdef EARLY_Z
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Early Z", model, api).New(src_path+"Early Z.cpp");
   REPD(skin, 2)src.New(S, "VS", "PS")("SKIN", skin);
}
#endif

#ifdef EFFECTS_2D
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Effects 2D", model, api).New(src_path+"Effects 2D.cpp");
   src.New("ColTrans"   , "Draw_VS", "ColTrans_PS");
   src.New("ColTransHB" , "Draw_VS", "ColTransHB_PS");
   src.New("ColTransHSB", "Draw_VS", "ColTransHSB_PS");

   src.New("Fade", "Draw_VS", "Fade_PS");
   src.New("RadialBlur", "Draw_VS", "RadialBlur_PS");
   src.New("Ripple", "Draw2DTex_VS", "Ripple_PS");
   src.New("Wave", "Wave_VS", "Wave_PS");
}
#endif

#ifdef EFFECTS_3D
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Effects 3D", model, api).New(src_path+"Effects 3D.cpp");
   REPD(inside, 3)
   REPD(la    , 2)src.New("Volume", "Volume_VS", "Volume_PS")("INSIDE", inside, "LA", la);

   REPD(normals, 2)src.New("Laser", "Laser_VS", "Laser_PS")("NORMALS", normals);

   REPD(fullscreen, 2)
   REPD(mode      , 3) // 0-default, 1-normals, 2-palette
      src.New("Decal", "Decal_VS", "Decal_PS")("FULLSCREEN", fullscreen, "MODE", mode);
}
#endif

#ifdef FOG_LOCAL
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Fog Local", model, api).New(src_path+"Fog.cpp");
                  src.New("FogBox" , "FogBox_VS" , "FogBox_PS" )                  .extra("HEIGHT", 0);
   REPD(inside, 2)src.New("FogBoxI", "FogBoxI_VS", "FogBoxI_PS")("INSIDE", inside).extra("HEIGHT", 0);

                  src.New("FogHeight" , "FogBox_VS" , "FogBox_PS" )                  .extra("HEIGHT", 1);
   REPD(inside, 2)src.New("FogHeightI", "FogBoxI_VS", "FogBoxI_PS")("INSIDE", inside).extra("HEIGHT", 1);

                  src.New("FogBall" , "FogBall_VS" , "FogBall_PS" );
   REPD(inside, 2)src.New("FogBallI", "FogBallI_VS", "FogBallI_PS")("INSIDE", inside);
}
#endif

#ifdef FUR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Fur", model, api).New(src_path+"Fur.cpp");
   REPD(skin   , 2)
   REPD(size   , 2)
   REPD(diffuse, 2)
   {
      src.New("Base", "Base_VS", "Base_PS")("SKIN", skin, "SIZE", size, "DIFFUSE", diffuse); // base
      src.New("Soft", "Soft_VS", "Soft_PS")("SKIN", skin, "SIZE", size, "DIFFUSE", diffuse); // soft
   }
}
#endif

#ifdef FXAA // FXAA unlike SMAA is kept outside of Main shader, because it's rarely used.
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"FXAA", model, api).New(src_path+"FXAA.cpp");
   REPD(gamma, 2)src.New("FXAA", "Draw_VS", "FXAA_PS")("GAMMA", gamma);
}
#endif

#ifdef HDR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Hdr", model, api).New(src_path+"Hdr.cpp");
   REPD(step, 2)src.New("HdrDS", "Draw_VS", "HdrDS_PS")("STEP", step);

   src.New("HdrUpdate", "Draw_VS", "HdrUpdate_PS");

   REPD(dither, 2)src.New("Hdr", "Draw_VS", "Hdr_PS")("DITHER", dither);
}
#endif

#ifdef LAYERED_CLOUDS
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Layered Clouds", model, api).New(src_path+"Layered Clouds.cpp");
   REPD(num  , 4)
   REPD(blend, 2)src.New("Clouds", "LayeredClouds_VS", "LayeredClouds_PS")("NUM", num+1, "BLEND", blend);
#endif

#ifdef MOTION_BLUR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Motion Blur", model, api).New(src_path+"Motion Blur.cpp");
   src.New("Explosion", "Explosion_VS", "Explosion_PS");

   src.New("ClearSkyVel", "ClearSkyVel_VS", "ClearSkyVel_PS");

   REPD(mode , 2)
   REPD(clamp, 2)src.New("Convert", "Convert_VS", "Convert_PS")("MODE", mode, "CLAMP", clamp);

   src.New("Dilate", "Draw_VS", "Dilate_PS");

   REPD(clamp, 2)src.New("SetDirs", "Draw_VS", "SetDirs_PS")("CLAMP", clamp);

   REPD(dither, 2)src.New("Blur", "Draw_VS", "Blur_PS")("DITHER", dither);

   Int ranges[]={1, 2, 4, 6, 8, 12, 16, 24, 32};
   REPD (diagonal, 2)
   REPAD(range   , ranges)
   {
      src.New("DilateX", "Draw_VS", "DilateX_PS")("DIAGONAL", diagonal, "RANGE", ranges[range]);
      src.New("DilateY", "Draw_VS", "DilateY_PS")("DIAGONAL", diagonal, "RANGE", ranges[range]);
   }
}
#endif

#ifdef OVERLAY
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Overlay", model, api).New(src_path+"Overlay.cpp");
   REPD(skin  , 2)
   REPD(normal, 2)src.New(S, "VS", "PS")("SKIN", skin, "NORMALS", normal);
}
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Tattoo", model, api).New(src_path+"Tattoo.cpp");
   REPD(tesselate, tess ? 2 : 1)
   REPD(skin     , 2           )src.New(S, "VS", "PS")("SKIN", skin).tesselate(tesselate);
}
#endif

#ifdef POSITION
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Position", model, api).New(src_path+"Position.cpp");
   REPD(tesselate , tess ? 2 : 1)
   REPD(skin      , 2)
   REPD(textures  , 3)
   REPD(test_blend, textures ? 2 : 1)src.New().position(skin, textures, test_blend, FX_NONE, tesselate);

   // grass + leafs
   for(Int textures=1; textures<=2; textures++)
   REPD (test_blend, 2)
   REPAD(fx        , fxs)
      src.New().position(0, textures, test_blend, fxs[fx], 0);
}
#endif

#ifdef SET_COLOR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Set Color", model, api).New(src_path+"Set Color.cpp");
   REPD(tesselate, tess ? 2 : 1)
   REPD(skin     , 2)
   REPD(textures , 3)
      src.New(S, "VS", "PS")("SKIN", skin, "TEXTURES", textures).tesselate(tesselate);
}
#endif

#ifdef VOLUMETRIC_CLOUDS
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Volumetric Clouds", model, api).New(src_path+"Volumetric Clouds.cpp");
                 src.New("Clouds"    , "Clouds_VS"    , "Clouds_PS"    );
                 src.New("CloudsMap" , "CloudsMap_VS" , "CloudsMap_PS" );
   REPD(gamma, 2)src.New("CloudsDraw", "CloudsDraw_VS", "CloudsDraw_PS")("GAMMA", gamma);
}
#endif

#ifdef VOLUMETRIC_LIGHTS
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Volumetric Lights", model, api).New(src_path+"Volumetric Lights.cpp");
   REPD(num  , 2)
   REPD(cloud, 2)
      src.New("VolDir", "DrawPosXY_VS", "VolDir_PS")("NUM", num, "CLOUD", cloud);

   src.New("VolPoint" , "DrawPosXY_VS", "VolPoint_PS" );
   src.New("VolLinear", "DrawPosXY_VS", "VolLinear_PS");
   src.New("VolCone"  , "DrawPosXY_VS", "VolCone_PS"  );

   REPD(add, 2)src.New("Volumetric", "Draw_VS", "Volumetric_PS")("ADD", add);
}
#endif

#ifdef WATER
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Water", model, api).New(src_path+"Water.cpp");
   REPD(refract  , 2)
   REPD(set_depth, 2)
      src.New("Apply", "DrawPosXY_VS", "Apply_PS")("REFRACT", refract, "SET_DEPTH", set_depth);

   REPD(refract, 2)
      src.New("Under", "DrawPosXY_VS", "Under_PS")("REFRACT", refract);

   REPD(fake_reflection, 2)
   {
      src.New("Lake" , "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 0, "RIVER", 0);
      src.New("River", "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 0, "RIVER", 1);
      src.New("Ocean", "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 1, "RIVER", 0);

      REPD(shadow, 7)
      REPD(soft  , 2)
      {
         src.New("Lake" , "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 0, "RIVER", 0);
         src.New("River", "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 0, "RIVER", 1);
         src.New("Ocean", "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft, "FAKE_REFLECTION", fake_reflection).extra("WAVES", 1, "RIVER", 0);
      }
   }
}
#endif

#ifdef WORLD_EDITOR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"World Editor", model, api).New(src_path+"World Editor.cpp");
   src.New("WhiteVtx", "Color_VS", "Color_PS").extra("COL_VALUE", "1, 1, 1", "VTX_COL", 1);
   src.New("White"   , "Color_VS", "Color_PS").extra("COL_VALUE", "1, 1, 1");
   src.New("Green"   , "Color_VS", "Color_PS").extra("COL_VALUE", "0, 1, 0");
   src.New("Yellow"  , "Color_VS", "Color_PS").extra("COL_VALUE", "1, 1, 0");
   src.New("Red"     , "Color_VS", "Color_PS").extra("COL_VALUE", "1, 0, 0");

   src.New("Circle" , "FX_VS", "Circle_PS");
   src.New("Square" , "FX_VS", "Square_PS");
   src.New("Grid"   , "FX_VS",   "Grid_PS");
}
#endif

#ifdef DX10_INPUT_LAYOUT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(S, model, api).New(src_path+"DX10+ Input Layout.cpp");
   src.New("Shader", "VS", "PS");
}
#endif

#ifdef DEFERRED
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Deferred", model, api).New(src_path+"Deferred.cpp");

   // zero
   REPD(skin , 2)
   REPD(color, 2)src.New().deferred(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE, false);

   REPD(tesselate, tess ? 2 : 1)
   REPD(heightmap, 2)
   {
      // 1 material, 0-2 tex, flat
      REPD(skin   , heightmap ? 1 : 2)
      REPD(detail , 2)
      REPD(reflect, 2)
      REPD(color  , 2)
      for(Int textures=0; textures<=2; textures++)
      REPD(alpha_test, (textures && !heightmap) ? 2 : 1)
         src.New().deferred(skin, 1, textures, SBUMP_FLAT, alpha_test, detail, false, reflect, color, false, heightmap, FX_NONE, tesselate); // 1 material, 0 tex

      // 1 material, 1-2 tex, flat, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)
         src.New().deferred(false, 1, textures, SBUMP_FLAT, false, false, true, false, color, false, heightmap, FX_NONE, tesselate);

      // 1 material, 2 tex, normal + parallax
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(reflect   , 2)
      REPD(color     , 2)
      for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
         src.New().deferred(skin, 1, 2, bump_mode, alpha_test, detail, false, reflect, color, false, heightmap, FX_NONE, tesselate);

      // 1 material, 1-2 tex, normal, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)
         src.New().deferred(false, 1, textures, SBUMP_NORMAL, false, false, true, false, color, false, heightmap, FX_NONE, tesselate);

      // 1 material, 2 tex, relief
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(reflect   , 2)
      REPD(color     , 2)
         src.New().deferred(skin, 1, 2, SBUMP_RELIEF, alpha_test, detail, false, reflect, color, false, heightmap, FX_NONE, tesselate);

   #if MULTI_MATERIAL
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      REPD(color     , 2)
      REPD(mtrl_blend, 2)
      REPD(reflect   , 2)
      {
         // 2-4 materials, 1-2 tex, flat
         REPD(detail, 2)
         for(Int textures=1; textures<=2; textures++)src.New().deferred(false, materials, textures, SBUMP_FLAT, false, detail, false, reflect, color, mtrl_blend, heightmap, FX_NONE, tesselate);

         // 2-4 materials, 1-2 tex, flat, macro
         for(Int textures=1; textures<=2; textures++)src.New().deferred(false, materials, textures, SBUMP_FLAT, false, false, true, reflect, color, mtrl_blend, heightmap, FX_NONE, tesselate);

         // 2-4 materials, 2 textures, normal + parallax
         REPD(detail, 2)
         for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX_MULTI; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
            src.New().deferred(false, materials, 2, bump_mode, false, detail, false, reflect, color, mtrl_blend, heightmap, FX_NONE, tesselate);

         // 2-4 materials, 2 textures, normal, macro
         src.New().deferred(false, materials, 2, SBUMP_NORMAL, false, false, true, reflect, color, mtrl_blend, heightmap, FX_NONE, tesselate);

         // 2-4 materials, 2 textures, relief
         REPD(detail, 2)
            src.New().deferred(false, materials, 2, SBUMP_RELIEF, false, detail, false, reflect, color, mtrl_blend, heightmap, FX_NONE, tesselate);
      }
   #endif
   }

   // grass + leaf, 1 material, 1-2 tex
   for(Int textures=1; textures<=2; textures++)
   REPD (bump_mode, (textures==2) ? 2 : 1)
   REPD (color    , 2)
   REPAD(fx       , fxs)
      src.New().deferred(false, 1, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, true, false, false, false, color, false, false, fxs[fx], false);
}
#endif

#ifdef BLEND_LIGHT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Blend Light", model, api).New(src_path+"Blend Light.cpp");

   REPD(per_pixel  , 2)
   REPD(shadow_maps, 7) // 7=(6+off), 1=off
   REPD(color      , 2)
   {
      // base
      REPD(skin      , 2)
      REPD(textures  , 3)
      REPD(bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test,               textures     ? 2 : 1)
      REPD(alpha     ,               textures     ? 2 : 1)
      REPD(light_map ,               textures     ? 2 : 1)
      REPD(reflect   , 2)
         src.New().blendLight(skin, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, alpha, light_map, reflect, FX_NONE, per_pixel, shadow_maps);

      // grass+leaf, 1 material, 1-2 tex
      for(Int textures=1; textures<=2; textures++)
      REPD (bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD (alpha_test,               textures     ? 2 : 1)
      REPAD(fx        , fxs)
         src.New().blendLight(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, fxs[fx], per_pixel, shadow_maps);
   }
}
#endif

#ifdef FORWARD
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Forward", model, api).New(src_path+"Forward.cpp");

   // zero
   REPD(skin , 2)
   REPD(color, 2)
      src.forward(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE, false,   false,false,0,   false,false,   false,false,   false,false,   false);

   REPD(tesselate, tess ? 2 : 1)
   REPD(heightmap, 2)
   REPD(per_pixel, 2)
   REPD(color    , 2)
   REPD(reflect  , 2)
   {
      // 1 material, 0-2 textures
      REPD(skin      , heightmap ? 1 : 2)
      REPD(textures  , 3)
      REPD(bump_mode , ( per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test, (!heightmap && textures   ) ? 2 : 1)
      REPD(light_map , (!heightmap && textures   ) ? 2 : 1)
      REPD(detail    , 2)
         src.forwardLight(skin, 1, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, light_map, detail, reflect, color, false, heightmap, FX_NONE, per_pixel, tesselate);

      // 2-4 materials, 1-2 textures
   #if MULTI_MATERIAL
      REPD(mtrl_blend, 2)
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      for(Int textures =1; textures <=2        ; textures ++)
      REPD(bump_mode, (per_pixel && textures==2) ? 2 : 1)
         src.forwardLight(false, materials, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, false, false, false, reflect, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tesselate);
   #endif
   }

   // grass + leaf, 1 material, 1-2 tex
   for(Int textures=1; textures<=2; textures++)
   REPD (per_pixel, 2)
   REPD (bump_mode, (per_pixel && textures==2) ? 2 : 1)
   REPD (color    , 2)
   REPAD(fx       , fxs)
      src.forwardLight(false, 1, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, true, false, false, false, color, false, false, fxs[fx], per_pixel, false);
}
#endif
}
/******************************************************************************/
#endif // COMPILE
/******************************************************************************/
void MainShaderClass::compile()
{
#if COMPILE_DX || COMPILE_GL
   App.stayAwake(AWAKE_SYSTEM);

#if COMPILE_GL
   Compile(API_GL);
#endif
#if COMPILE_DX
   Compile(API_DX);
#endif

   ProcPriority(-1); // compiling shaders may slow down entire CPU, so make this process have smaller priority
   if(ShaderCompilers.elms())
   {
      Threads threads; threads.create(false, Cpu.threads()-1);
      FREPAO(ShaderCompilers).compile(threads);
   }
   ProcPriority(0);

   App.stayAwake(AWAKE_OFF);
#endif
}
/******************************************************************************/
}
/******************************************************************************/
