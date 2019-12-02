/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
/******************************************************************************/
#if WINDOWS
   #define COMPILE_DX_AMD 0 // #ShaderAMD
   #define COMPILE_DX 0
   #define COMPILE_GL 0
#endif

/**
#define MAIN

#define DEFERRED
#define BLEND_LIGHT
#define FORWARD

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
#define DX10_INPUT_LAYOUT // #VTX_INPUT_LAYOUT
/******************************************************************************/
// SHADER NAMES
/******************************************************************************/
Str8 ShaderDeferred  (Int skin, Int materials, Int layout, Int bump_mode, Int alpha_test, Int detail, Int macro, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tesselate) {return S8+skin+materials+layout+bump_mode+alpha_test+detail+macro+color+mtrl_blend+heightmap+fx+tesselate;}
Str8 ShaderBlendLight(Int skin, Int color    , Int layout, Int bump_mode, Int alpha_test, Int alpha, Int reflect, Int light_map, Int fx, Int per_pixel, Int shadow_maps, Int tesselate) {return S8+skin+color+layout+bump_mode+alpha_test+alpha+reflect+light_map+fx+per_pixel+shadow_maps+tesselate;}
Str8 ShaderForward   (Int skin, Int materials, Int layout, Int bump_mode, Int alpha_test, Int reflect, Int light_map, Int detail, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tesselate) {return S8+skin+materials+layout+bump_mode+alpha_test+reflect+light_map+detail+color+mtrl_blend+heightmap+fx+per_pixel+light_dir+light_dir_shd+light_dir_shd_num+light_point+light_point_shd+light_linear+light_linear_shd+light_cone+light_cone_shd+tesselate;}

Str8 ShaderAmbient   (Int skin, Int alpha_test, Int light_map) {return S8+skin+alpha_test+light_map;}
Str8 ShaderBehind    (Int skin, Int alpha_test) {return S8+skin+alpha_test;}
Str8 ShaderBlend     (Int skin, Int color, Int layout, Int bump_mode, Int reflect) {return S8+skin+color+layout+bump_mode+reflect;}
Str8 ShaderEarlyZ    (Int skin) {return S8+skin;}
Str8 ShaderFurBase   (Int skin, Int size, Int diffuse) {return S8+"Base"+skin+size+diffuse;}
Str8 ShaderFurSoft   (Int skin, Int size, Int diffuse) {return S8+"Soft"+skin+size+diffuse;}
Str8 ShaderOverlay   (Int skin, Int normal, Int layout) {return S8+skin+normal+layout;}
Str8 ShaderPosition  (Int skin, Int alpha_test, Int test_blend, Int fx, Int tesselate) {return S8+skin+alpha_test+test_blend+fx+tesselate;}
Str8 ShaderSetColor  (Int skin, Int alpha_test, Int tesselate) {return S8+skin+alpha_test+tesselate;}
Str8 ShaderTattoo    (Int skin, Int tesselate) {return S8+skin+tesselate;}
/******************************************************************************/
#if COMPILE_DX_AMD || COMPILE_DX || COMPILE_GL
/******************************************************************************/
static Memx<ShaderCompiler> ShaderCompilers; // use 'Memx' because we store pointers to 'ShaderCompiler'
/******************************************************************************/
// LISTING ALL SHADER TECHNIQUES
/******************************************************************************/
static void Compile(API api, Bool amd=false) // #ShaderAMD
{
   if(!DataPath().is())Exit("Can't compile default shaders - 'DataPath' not specified");

   Str src_path=GetPath(GetPath(__FILE__))+"\\Shaders\\", // for example "C:/Esenthel/Engine/Src/Shaders/"
      dest_path=DataPath();                               // for example "C:/Esenthel/Data/Shader/4/"
   switch(api)
   {
      default: return;

      case API_GL: dest_path+="Shader\\GL\\"; break;
      case API_DX: dest_path+=(amd ? "Shader\\4 AMD\\" : "Shader\\4\\" ); break;
   }
   FCreateDirs(dest_path);
   SHADER_MODEL model=SM_4;

   Bool ms  =(api==API_DX), // if support multi-sampling in shaders
        tess=(api==API_DX); // if support tesselation    in shaders
   Int fxs[]={FX_GRASS_2D, FX_GRASS_3D, FX_LEAF, FX_LEAFS};

#ifdef MAIN
{
   ShaderCompiler &compiler=ShaderCompilers.New().set(dest_path+"Main", model, api);
   {
      ShaderCompiler::Source &src=compiler.New(src_path+"Main.cpp");
   #if DEBUG && 0
      #pragma message("!! Warning: Use this only for debugging !!")
      src.New("Test", "Draw_VS", "Test_PS")("MODE", 0);
      src.New("Test", "Draw_VS", "Test_PS")("MODE", 1);
   #endif
      src.New("Draw2DFlat", "Draw2DFlat_VS", "DrawFlat_PS");
      src.New("Draw3DFlat", "Draw3DFlat_VS", "DrawFlat_PS");
      src.New("SetCol"    , "Draw_VS"      , "DrawFlat_PS");

      src.New("ClearDeferred", "ClearDeferred_VS", "ClearDeferred_PS");
      src.New("ClearLight"   , "Draw_VS"         , "ClearLight_PS");

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

      src.New("DrawTexXSG" , "Draw2DTex_VS", "DrawTexXSG_PS");
      src.New("DrawTexXYSG", "Draw2DTex_VS", "DrawTexXYSG_PS");
      src.New("DrawTexSG"  , "Draw2DTex_VS", "DrawTexSG_PS");

      src.New("DrawTexNrm"   , "Draw2DTex_VS", "DrawTexNrm_PS");
      src.New("DrawTexDetNrm", "Draw2DTex_VS", "DrawTexDetNrm_PS");
      src.New("Draw"         ,      "Draw_VS",  "Draw2DTex_PS");
      src.New("DrawC"        ,      "Draw_VS", "Draw2DTexC_PS");
      src.New("DrawCG"       ,      "Draw_VS", "DrawTexCG_PS");
      src.New("DrawG"        ,      "Draw_VS", "DrawTexG_PS");
      src.New("DrawA"        ,      "Draw_VS", "Draw2DTexA_PS");

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

            src.New("SetAlphaFromDepth"        , "Draw_VS", "SetAlphaFromDepth_PS");
      if(ms)src.New("SetAlphaFromDepthMS"      , "Draw_VS", "SetAlphaFromDepthMS_PS").multiSample();
            src.New("SetAlphaFromDepthAndCol"  , "Draw_VS", "SetAlphaFromDepthAndCol_PS");
      if(ms)src.New("SetAlphaFromDepthAndColMS", "Draw_VS", "SetAlphaFromDepthAndColMS_PS").multiSample();
            src.New("CombineAlpha"             , "Draw_VS", "CombineAlpha_PS");
            src.New("ReplaceAlpha"             , "Draw_VS", "ReplaceAlpha_PS");

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
      src.New("Params2", S, "Params2_PS").dummy=true;
   }
   { // BLOOM
      ShaderCompiler::Source &src=compiler.New(src_path+"Bloom.cpp");
      REPD(glow    , 2)
      REPD(clamp   , 2)
      REPD(half_res, 2)
      REPD(saturate, 2)
      REPD(gamma   , 2)
         src.New("BloomDS", "BloomDS_VS", "BloomDS_PS")("GLOW", glow, "CLAMP", clamp, "HALF_RES", half_res, "SATURATE", saturate)("GAMMA", gamma);

      REPD(dither, 2)
      REPD(gamma , 2)
      REPD(alpha , 2)
         src.New("Bloom", "Draw_VS", "Bloom_PS")("DITHER", dither, "GAMMA", gamma, "ALPHA", alpha);
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
      REPD(diffuse     , DIFFUSE_NUM)
      REPD(shadow      , 2)
      REPD(multi_sample, ms ? 2 : 1)
      REPD(water       , 2)
      {
                           src.New("DrawLightDir"     , "DrawPosXY_VS", "LightDir_PS"   ).multiSample(multi_sample)("DIFFUSE_MODE", diffuse, "SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "WATER", water); // Directional light is always fullscreen, so can use 2D shader
         REPD(gl_es, (api==API_GL) ? 2 : 1) // GL ES doesn't support NOPERSP and 'D.depthClip'
         {
                           src.New("DrawLightPoint"   , "Geom_VS"     , "LightPoint_PS" ).multiSample(multi_sample)("DIFFUSE_MODE", diffuse, "SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "WATER", water)(                "GL_ES", gl_es).extra("CLAMP_DEPTH", gl_es);  // 3D Geom Mesh (only ball based are depth-clamped, because Dir is fullscreen and Cone has too many artifacts when depth clamping)
                           src.New("DrawLightLinear"  , "Geom_VS"     , "LightLinear_PS").multiSample(multi_sample)("DIFFUSE_MODE", diffuse, "SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "WATER", water)(                "GL_ES", gl_es).extra("CLAMP_DEPTH", gl_es);  // 3D Geom Mesh (only ball based are depth-clamped, because Dir is fullscreen and Cone has too many artifacts when depth clamping)
            REPD(image, 2){src.New("DrawLightCone"    , "Geom_VS"     , "LightCone_PS"  ).multiSample(multi_sample)("DIFFUSE_MODE", diffuse, "SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "WATER", water)("IMAGE", image, "GL_ES", gl_es)                            ;  // 3D Geom Mesh
                  if(gl_es)src.New("DrawLightConeFlat", "DrawPosXY_VS", "LightCone_PS"  ).multiSample(multi_sample)("DIFFUSE_MODE", diffuse, "SHADOW", shadow, "MULTI_SAMPLE", multi_sample, "WATER", water)("IMAGE", image                )                            ;} // 2D Flat
         }
      }
   }
   { // LIGHT APPLY
      ShaderCompiler::Source &src=compiler.New(src_path+"Light Apply.cpp");
      REPD(multi_sample, ms ? 3 : 1)
      REPD(ao          , 2)
      REPD(  cel_shade , 2)
      REPD(night_shade , 2)
      REPD(glow        , 2)
      REPD(reflect     , 2)
         src.New("ApplyLight", "DrawPosXY_VS", "ApplyLight_PS")("MULTI_SAMPLE", multi_sample, "AO", ao, "CEL_SHADE", cel_shade, "NIGHT_SHADE", night_shade)("GLOW", glow, "REFLECT", reflect);
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
      REPD(gl_es, (api==API_GL) ? 3 : 1) // GL ES doesn't support NOPERSP, 'D.depthClip' and TexDepthLinear (0=no GL ES, 1=GL ES, 2=GL ES with GATHER support)
      REPD(geom , 2)
      {
         CChar8 *vs=(geom ? "Geom_VS" : "Draw_VS");
         Bool    gather=(gl_es==2);
         src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES",  4).gather(gather);
       //src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES",  5).gather(gather);
         src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES",  6).gather(gather);
         src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES",  8).gather(gather);
       //src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES",  9).gather(gather);
         src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES", 12).gather(gather);
       //src.New("ShdBlur" , vs, "ShdBlur_PS" )("GEOM", geom)("GL_ES", gl_es)("SAMPLES", 13).gather(gather);
       //src.New("ShdBlurX", vs, "ShdBlurX_PS")("GEOM", geom)("GL_ES", gl_es)("RANGE"  ,  1).gather(gather);
       //src.New("ShdBlurY", vs, "ShdBlurY_PS")("GEOM", geom)("GL_ES", gl_es)("RANGE"  ,  1).gather(gather);
         src.New("ShdBlurX", vs, "ShdBlurX_PS")("GEOM", geom)("GL_ES", gl_es)("RANGE"  ,  2).gather(gather);
         src.New("ShdBlurY", vs, "ShdBlurY_PS")("GEOM", geom)("GL_ES", gl_es)("RANGE"  ,  2).gather(gather);
      }
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

   REPD(skin      , 2)
   REPD(alpha_test, 3)
      src.New(S, "VS", "PS")("SKIN", skin, "ALPHA_TEST", alpha_test);
}
#endif

#ifdef BLEND
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Blend", model, api).New(src_path+"Blend.cpp");

   REPD(skin   , 2)
   REPD(color  , 2)
   REPD(layout , 3)
   for(Int bump_mode=SBUMP_ZERO; bump_mode<=SBUMP_NORMAL; bump_mode++)
   REPD(reflect, (bump_mode==SBUMP_ZERO) ? 1 : 2)
      src.New(S, "VS", "PS")("SKIN", skin, "COLORS", color, "LAYOUT", layout, "BUMP_MODE", bump_mode)("REFLECT", reflect);
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
   REPD(alpha    , 2)
   REPD(half_res , 2)
   REPD(gather   , 2)
      src.New("DofDS", "Draw_VS", "DofDS_PS")("CLAMP", clamp, "REALISTIC", realistic, "ALPHA", alpha, "HALF_RES", half_res)("GATHER", gather).gather(gather);

   REPD(alpha, 2)
   for(Int range=2; range<=12; range++)
   {
      src.New("DofBlurX", "Draw_VS", "DofBlurX_PS")("ALPHA", alpha, "RANGE", range);
      src.New("DofBlurY", "Draw_VS", "DofBlurY_PS")("ALPHA", alpha, "RANGE", range);
   }

   REPD(dither   , 2)
   REPD(realistic, 2)
   REPD(alpha    , 2)
      src.New("Dof", "Draw_VS", "Dof_PS")("DITHER", dither, "REALISTIC", realistic, "ALPHA", alpha);
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
   for(Int layout=1; layout<=2; layout++)
   REPD(mode      , 3) // 0-default, 1-normals, 2-palette
      src.New("Decal", "Decal_VS", "Decal_PS")("FULLSCREEN", fullscreen, "LAYOUT", layout, "MODE", mode);
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

   REPD(mode , 2)
   REPD(clamp, 2)src.New("Convert", "Convert_VS", "Convert_PS")("MODE", mode, "CLAMP", clamp);

   src.New("Dilate", "Draw_VS", "Dilate_PS");

   REPD(clamp, 2)src.New("SetDirs", "Draw_VS", "SetDirs_PS")("CLAMP", clamp);

   REPD(dither, 2)
   REPD(alpha , 2)
      src.New("Blur", "Draw_VS", "Blur_PS")("DITHER", dither, "ALPHA", alpha);

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
   REPD(normal, 2)
   for(Int layout=1; layout<=2; layout++)
      src.New(S, "VS", "PS")("SKIN", skin, "NORMALS", normal, "LAYOUT", layout);
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
   REPD(alpha_test, 3)
   REPD(test_blend, alpha_test ? 2 : 1)
      src.New().position(skin, alpha_test, test_blend, FX_NONE, tesselate);

   // grass + leafs
   for(Int alpha_test=1; alpha_test<=2; alpha_test++)
   REPD (test_blend, 2)
   REPAD(fx        , fxs)
      src.New().position(0, alpha_test, test_blend, fxs[fx], 0);
}
#endif

#ifdef SET_COLOR
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Set Color", model, api).New(src_path+"Set Color.cpp");
   REPD(tesselate , tess ? 2 : 1)
   REPD(skin      , 2)
   REPD(alpha_test, 3)
      src.New(S, "VS", "PS")("SKIN", skin, "ALPHA_TEST", alpha_test).tesselate(tesselate);
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
   REPD(num  , 6)
   REPD(cloud, 2)
      src.New("VolDir", "DrawPosXY_VS", "VolDir_PS")("NUM", num+1, "CLOUD", cloud);

   src.New("VolPoint" , "DrawPosXY_VS", "VolPoint_PS" );
   src.New("VolLinear", "DrawPosXY_VS", "VolLinear_PS");
   src.New("VolCone"  , "DrawPosXY_VS", "VolCone_PS"  );

   REPD(add, 2)src.New("Volumetric", "Draw_VS", "Volumetric_PS")("ADD", add);
}
#endif

#ifdef WATER
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Water", model, api).New(src_path+"Water.cpp");
   src.New("Lake" , "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0)("REFLECT_ENV", 0, "REFLECT_MIRROR", 0, "REFRACT", 0).extra("WAVES", 0, "RIVER", 0);
   src.New("River", "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0)("REFLECT_ENV", 0, "REFLECT_MIRROR", 0, "REFRACT", 0).extra("WAVES", 0, "RIVER", 1);
   src.New("Ocean", "Surface_VS", "Surface_PS")("LIGHT", 0, "SHADOW", 0, "SOFT", 0)("REFLECT_ENV", 0, "REFLECT_MIRROR", 0, "REFRACT", 0).extra("WAVES", 1, "RIVER", 0);
   REPD(refract, 2)
   {
      REPD(reflect_env   , 2)
      REPD(reflect_mirror, 2)
      REPD(gather        , 2)
      {
         REPD(shadow, 7)
         REPD(soft  , 2)
         {
            src.New("Lake" , "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft)("REFLECT_ENV", reflect_env, "REFLECT_MIRROR", reflect_mirror, "REFRACT", refract, "GATHER", gather).extra("WAVES", 0, "RIVER", 0).gather(gather);
            src.New("River", "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft)("REFLECT_ENV", reflect_env, "REFLECT_MIRROR", reflect_mirror, "REFRACT", refract, "GATHER", gather).extra("WAVES", 0, "RIVER", 1).gather(gather);
            src.New("Ocean", "Surface_VS", "Surface_PS")("LIGHT", 1, "SHADOW", shadow, "SOFT", soft)("REFLECT_ENV", reflect_env, "REFLECT_MIRROR", reflect_mirror, "REFRACT", refract, "GATHER", gather).extra("WAVES", 1, "RIVER", 0).gather(gather);
         }
         REPD(set_depth, 2)
            src.New("Apply", "DrawPosXY_VS", "Apply_PS")("SET_DEPTH", set_depth)("REFLECT_ENV", reflect_env, "REFLECT_MIRROR", reflect_mirror, "REFRACT", refract, "GATHER", gather).gather(gather);
      }
      src.New("Under", "DrawPosXY_VS", "Under_PS")("REFRACT", refract);
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
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path, model, api).New(src_path+"DX10+ Input Layout.cpp"); // #ShaderAMD replace 'dest_path' with 'S'
   src.New("Shader", "VS", "PS");
}
#endif

#ifdef DEFERRED
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Deferred", model, api).New(src_path+"Deferred.cpp");
   REPD(color, 2)
   {
      // zero (no vtx normals)
      REPD(skin, 2)
         src.New().deferred(skin, 1, 0, SBUMP_ZERO, false, false, false, color, false, false, FX_NONE, false);

      // default
      REPD(skin, 2)
      for(Int materials=1, materials_max=(skin ? 1 : MAX_MTRLS); materials<=materials_max; materials++)
      for(Int layout=((materials==1) ? 0 : 1); layout<=2; layout++) // multi-materials don't support 0 textures
      for(Int bump_mode=SBUMP_FLAT, bump_max=((layout>=2) ? SBUMP_RELIEF : SBUMP_NORMAL); bump_mode<=bump_max; bump_mode++)if(bump_mode<=SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
      REPD(mtrl_blend, (materials> 1 && layout>=2           ) ? 2 : 1) // can do per-pixel mtrl-blend only if we have bump map
      REPD(heightmap , (!skin                               ) ? 2 : 1)
      REPD(alpha_test, (materials==1 && layout && !heightmap) ? 2 : 1)
      REPD(macro     , (!skin  &&                  heightmap) ? 2 : 1)
      REPD(tesselate , tess ? 2 : 1)
      REPD(detail    , 2)
         src.New().deferred(skin, materials, layout, bump_mode, alpha_test, detail, macro, color, mtrl_blend, heightmap, FX_NONE, tesselate);

      // grass + leaf, 1 material, 1-2 tex
      for(Int layout=1; layout<=2; layout++)
      REPD (bump_mode, 2)
      REPAD(fx       , fxs)
         src.New().deferred(false, 1, layout, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, true, false, false, color, false, false, fxs[fx], false);
   }
}
#endif

#ifdef BLEND_LIGHT
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Blend Light", model, api).New(src_path+"Blend Light.cpp");
   REPD(per_pixel  , 2)
   REPD(shadow_maps, 7) // 7=(6+off), 0=off
   REPD(color      , 2)
   REPD(reflect    , 2)
   {
      // base
      REPD(skin      , 2)
      REPD(layout    , 3)
      REPD(bump_mode , per_pixel ? 2 : 1)
      REPD(alpha_test, layout    ? 2 : 1)
      REPD(alpha     , layout    ? 2 : 1)
      REPD(light_map , 2)
         src.New().blendLight(skin, color, layout, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, alpha, reflect, light_map, FX_NONE, per_pixel, shadow_maps);

      // grass+leaf, 1 material, 1-2 tex
      for(Int layout=1; layout<=2; layout++)
      REPD (bump_mode , per_pixel ? 2 : 1)
      REPD (alpha_test, layout    ? 2 : 1)
      REPAD(fx        , fxs)
         src.New().blendLight(false, color, layout, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, reflect, false, fxs[fx], per_pixel, shadow_maps);
   }
}
#endif

#ifdef FORWARD
{
   ShaderCompiler::Source &src=ShaderCompilers.New().set(dest_path+"Forward", model, api).New(src_path+"Forward.cpp");
   REPD(color, 2)
   {
      // zero (no vtx normals)
      REPD(skin, 2)
         src.forward(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE, false,   false,false,0,   false,false,   false,false,   false,false,   false);

      // default
      REPD(skin      , 2)
      REPD(per_pixel , 2)
      for(Int materials=1, materials_max=(skin ? 1 : MAX_MTRLS); materials<=materials_max; materials++)
      for(Int layout=((materials==1) ? 0 : 1); layout<=2; layout++) // multi-materials don't support 0 textures
      REPD(bump_mode , per_pixel                              ? 2 : 1)
      REPD(mtrl_blend, (materials> 1 && layout>=2           ) ? 2 : 1) // can do per-pixel mtrl-blend only if we have bump map
      REPD(heightmap , (!skin                               ) ? 2 : 1)
      REPD(alpha_test, (materials==1 && layout && !heightmap) ? 2 : 1)
      REPD(light_map , (materials==1 &&           !heightmap) ? 2 : 1)
      REPD(tesselate , (tess && SUPPORT_FORWARD_TESSELATE   ) ? 2 : 1)
      REPD(detail    , (        SUPPORT_FORWARD_DETAIL      ) ? 2 : 1)
      REPD(reflect   , 2)
         src.forwardLight(skin, materials, layout, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, reflect, light_map, detail, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tesselate);

      // grass + leaf, 1 material, 1-2 tex
      for(Int layout=1; layout<=2; layout++)
      REPD (per_pixel, 2)
      REPD (bump_mode, per_pixel ? 2 : 1)
      REPD (reflect  , 2)
      REPAD(fx       , fxs)
         src.forwardLight(false, 1, layout, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, true, reflect, false, false, color, false, false, fxs[fx], per_pixel, false);
   }
}
#endif
}
/******************************************************************************/
#endif // COMPILE
/******************************************************************************/
void MainShaderClass::compile()
{
#if COMPILE_DX_AMD || COMPILE_DX || COMPILE_GL
   App.stayAwake(AWAKE_SYSTEM);

#if COMPILE_DX
   Compile(API_DX);
#endif
#if COMPILE_DX_AMD
   Compile(API_DX, true);
#endif
#if COMPILE_GL
   Compile(API_GL);
#endif

   ProcPriority(-1); // compiling shaders may slow down entire CPU, so make this process have smaller priority
   if(ShaderCompilers.elms())
   {
      Threads threads; threads.create(false, Cpu.threads()-1, 0, "EE.ShaderCompiler #");
      FREPAO(ShaderCompilers).compile(threads);
   }
   ProcPriority(0);

   App.stayAwake(AWAKE_OFF);
#endif
}
/******************************************************************************/
}
/******************************************************************************/
