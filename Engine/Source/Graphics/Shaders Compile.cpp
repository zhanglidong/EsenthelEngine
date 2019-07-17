/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
/******************************************************************************/
#define BUMP_MAPPING   1
#define MULTI_MATERIAL 1
#define FORCE_LOG      0

#if DX11   // DirectX 10+
   #define COMPILE_4 0
#endif
#if GL && !GL_ES // Desktop OpenGL
   #define COMPILE_GL 0
#endif

/**
#define MAIN

#define SIMPLE
#define DEFERRED
#define FORWARD // Forward Shaders in OpenGL compile almost an entire day and use ~5 GB memory during compilation
#define BLEND_LIGHT

#define AMBIENT
#define AMBIENT_OCCLUSION
#define AMBIENT_OCCLUSION_NEW
#define BEHIND
#define BLEND
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
// SHADER TECHNIQUE NAMES
/******************************************************************************/
Str8 TechNameSimple    (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int reflect, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel, Int tess) {return S8+"T"+skin+materials+textures+bump_mode+alpha_test+light_map+reflect+color+mtrl_blend+heightmap+fx+per_pixel+tess;}
Str8 TechNameDeferred  (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int detail, Int macro, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess) {return S8+"T"+skin+materials+textures+bump_mode+alpha_test+detail+macro+rflct+color+mtrl_blend+heightmap+fx+tess;}
Str8 TechNameForward   (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tess) {return S8+"T"+skin+materials+textures+bump_mode+alpha_test+light_map+detail+rflct+color+mtrl_blend+heightmap+fx+light_dir+light_dir_shd+light_dir_shd_num+light_point+light_point_shd+light_linear+light_linear_shd+light_cone+light_cone_shd+tess;}
Str8 TechNameBlendLight(Int skin, Int color    , Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps) {return S8+"T"+skin+color+textures+bump_mode+alpha_test+alpha+light_map+rflct+fx+per_pixel+shadow_maps;}
Str8 TechNamePosition  (Int skin, Int textures, Int test_blend, Int fx, Int tess) {return S8+"T"+skin+textures+test_blend+fx+tess;}
Str8 TechNameBlend     (Int skin, Int color, Int rflct, Int textures) {return S8+"T"+skin+color+rflct+textures;}
Str8 TechNameSetColor  (Int skin, Int textures, Int tess) {return S8+"T"+skin+textures+tess;}
Str8 TechNameBehind    (Int skin, Int textures) {return S8+"T"+skin+textures;}
Str8 TechNameEarlyZ    (Int skin) {return S8+"T"+skin;}
Str8 TechNameAmbient   (Int skin, Int alpha_test, Int light_map) {return S8+"T"+skin+alpha_test+light_map;}
Str8 TechNameOverlay   (Int skin, Int normal) {return S8+"T"+skin+normal;}
Str8 TechNameFurBase   (Int skin, Int size, Int diffuse) {return S8+"Base"+skin+size+diffuse;}
Str8 TechNameFurSoft   (Int skin, Int size, Int diffuse) {return S8+"Soft"+skin+size+diffuse;}
Str8 TechNameTattoo    (Int skin, Int tess             ) {return S8+"T"+skin+tess;}
/******************************************************************************/
#if COMPILE_4 || COMPILE_GL
/******************************************************************************/
// SHADER TECHNIQUE DECLARATIONS
/******************************************************************************/
static Str TechSimple(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int reflect, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel, Int tess)
{
   Str params=             S+skin+','+materials+','+textures+','+bump_mode+','+alpha_test+','+light_map+','+reflect+','+color+','+mtrl_blend+','+heightmap+','+fx+','+per_pixel+','+tess,
       name  =TechNameSimple(skin  ,  materials  ,  textures  ,  bump_mode  ,  alpha_test  ,  light_map  ,  reflect  ,  color  ,  mtrl_blend  ,  heightmap  ,  fx  ,  per_pixel  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}
static ShaderGLSL TechSimpleGlsl(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int reflect, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel, Int tess)
{
   return ShaderGLSL().set("Main", TechNameSimple(skin, materials, textures, bump_mode, alpha_test, light_map, reflect, color, mtrl_blend, heightmap, fx, per_pixel, tess))
      .par("skin"      , skin      )
      .par("materials" , materials )
      .par("textures"  , textures  )
      .par("bump_mode" , bump_mode )
      .par("alpha_test", alpha_test)
      .par("light_map" , light_map )
      .par("rflct"     , reflect   )
      .par("COLOR"     , color     )
      .par("mtrl_blend", mtrl_blend)
      .par("heightmap" , heightmap )
      .par("fx"        , fx        )
      .par("per_pixel" , per_pixel )
      .par("tesselate" , tess      );
}

static Str TechDeferred(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int detail, Int macro, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess)
{
   Str params=               S+skin+','+materials+','+textures+','+bump_mode+','+alpha_test+','+detail+','+macro+','+rflct+','+color+','+mtrl_blend+','+heightmap+','+fx+','+tess,
       name  =TechNameDeferred(skin  ,  materials  ,  textures  ,  bump_mode  ,  alpha_test  ,  detail  ,  macro  ,  rflct  ,  color  ,  mtrl_blend  ,  heightmap  ,  fx  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechForward(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tess)
{
   Str params=              S+skin+','+materials+','+textures+','+bump_mode+','+alpha_test+','+light_map+','+detail+','+rflct+','+color+','+mtrl_blend+','+heightmap+','+fx+','   +light_dir+','+light_dir_shd+','+light_dir_shd_num+   ','+light_point+','+light_point_shd+   ','+light_linear+','+light_linear_shd+   ','+light_cone+','+light_cone_shd+','+tess,
       name  =TechNameForward(skin  ,  materials  ,  textures  ,  bump_mode  ,  alpha_test  ,  light_map  ,  detail  ,  rflct  ,  color  ,  mtrl_blend  ,  heightmap  ,  fx  ,     light_dir  ,  light_dir_shd  ,  light_dir_shd_num     ,  light_point  ,  light_point_shd     ,  light_linear  ,  light_linear_shd     ,  light_cone  ,  light_cone_shd  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}
static Str TechForwardLight(Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess,   SHADER_MODEL model)
{
   Str names;
   REPD(shd, 2)
   {
      if(shd)for(Int maps=2; maps<=6; maps+=2) // 2, 4, 6 maps
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   true ,shd  ,maps,  false,false,  false,false,  false,false,  tess);else // light dir with shadow maps
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   true ,false,   0,  false,false,  false,false,  false,false,  tess);     // light dir no   shadow
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  true ,shd  ,  false,false,  false,false,  tess);     // light point
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  true ,shd  ,  false,false,  tess);     // light sqr
      names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  false,false,  true ,shd  ,  tess);     // light cone
   }  names+=TechForward(skin, materials, textures, bump_mode, alpha_test, light_map, detail, rflct, color, mtrl_blend, heightmap, fx,   false,false,   0,  false,false,  false,false,  false,false,  tess);     // no light
   return names;
}

static Str TechPosition(Int skin, Int textures, Int test_blend, Int fx, Int tess)
{
   Str params=               S+skin+','+textures+','+test_blend+','+fx+','+tess,
       name  =TechNamePosition(skin  ,  textures  ,  test_blend  ,  fx  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechBlend(Int skin, Int color, Int rflct, Int textures)
{
   Str params=            S+skin+','+color+','+rflct+','+textures,
       name  =TechNameBlend(skin  ,  color  ,  rflct  ,  textures);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}
static ShaderGLSL TechBlendGlsl(Int skin, Int color, Int rflct, Int textures)
{
   return ShaderGLSL().set("Main", TechNameBlend(skin, color, rflct, textures))
      .par("skin"    , skin    )
      .par("COLOR"   , color   )
      .par("rflct"   , rflct   )
      .par("textures", textures);
}

static Str TechBlendLight(Int skin, Int color, Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps)
{
   Str params=                 S+skin+','+color+','+textures+','+bump_mode+','+alpha_test+','+alpha+','+light_map+','+rflct+','+fx+','+per_pixel+','+shadow_maps,
       name  =TechNameBlendLight(skin  ,  color  ,  textures  ,  bump_mode  ,  alpha_test  ,  alpha  ,  light_map  ,  rflct  ,  fx  ,  per_pixel  ,  shadow_maps);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}
static ShaderGLSL TechBlendLightGlsl(Int skin, Int color, Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps)
{
   return ShaderGLSL().set("Main", TechNameBlendLight(skin, color, textures, bump_mode, alpha_test, alpha, light_map, rflct, fx, per_pixel, shadow_maps))
      .par("skin"       , skin       )
      .par("COLOR"      , color      )
      .par("textures"   , textures   )
      .par("bump_mode"  , bump_mode  )
      .par("alpha_test" , alpha_test )
      .par("ALPHA"      , alpha      )
      .par("light_map"  , light_map  )
      .par("rflct"      , rflct      )
      .par("fx"         , fx         )
      .par("per_pixel"  , per_pixel  )
      .par("shadow_maps", shadow_maps);
}

static Str TechSetColor(Int skin, Int textures, Int tess)
{
   Str params=               S+skin+','+textures+','+tess,
       name  =TechNameSetColor(skin  ,  textures  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechBehind(Int skin, Int textures)
{
   Str params=             S+skin+','+textures,
       name  =TechNameBehind(skin  ,  textures);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechEarlyZ(Int skin)
{
   Str params=             S+skin,
       name  =TechNameEarlyZ(skin);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechAmbient(Int skin, Int alpha_test, Int light_map)
{
   Str params=              S+skin+','+alpha_test+','+light_map,
       name  =TechNameAmbient(skin  ,  alpha_test  ,  light_map);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechOverlay(Int skin, Int normal)
{
   Str params=              S+skin+','+normal,
       name  =TechNameOverlay(skin  ,  normal);
   return S+"TECHNIQUE("+name+", VS("+params+"), PS("+params+"));";
}

static Str TechTattoo(Int skin, Int tess)
{
   Str params=             S+skin+','+tess,
       name  =TechNameTattoo(skin  ,  tess);
   return tess ? S+"TECHNIQUE_TESSELATION("+name+", VS("+params+"), PS("+params+"), HS("+params+"), DS("+params+"));"
               : S+"TECHNIQUE            ("+name+", VS("+params+"), PS("+params+")                                );";
}

static Str TechFurBase(Int skin, Int size, Int diffuse)
{
   Str params=              S+skin+','+size+','+diffuse,
       name  =TechNameFurBase(skin  ,  size  ,  diffuse);
   return S+"TECHNIQUE("+name+", Base_VS("+params+"), Base_PS("+params+"));";
}

static Str TechFurSoft(Int skin, Int size, Int diffuse)
{
   Str params=              S+skin+','+size+','+diffuse,
       name  =TechNameFurSoft(skin  ,  size  ,  diffuse);
   return S+"TECHNIQUE("+name+", Soft_VS("+params+"), Soft_PS("+params+"));";
}
/******************************************************************************/
// COMPILER
/******************************************************************************/
struct ShaderCompiler
{
   Str               src, dest;
   SHADER_MODEL      model;
   Memc<ShaderMacro> macros;
   Memc<ShaderGLSL > glsl;

   void compile()
   {
      Str messages; Bool ok=ShaderCompileTry(src, dest, model, macros, glsl, &messages);
      if(!ok || DEBUG || FORCE_LOG)if(messages.is())LogN(S+"Shader\n\""+src+"\"\nto file\n\""+dest+"\"\n"+messages);
      if(!ok)
      {
      #if !DX11
         if(model>=SM_4)Exit("Can't compile DX10+ Shaders when not using DX10+ engine version");
      #endif
      #if !GL
         if(model>=SM_GL_ES_3 && model<=SM_GL)Exit("Can't compile OpenGL Shaders when not using OpenGL engine version");
      #endif
         Exit(S+"Error compiling shader\n\""+src+"\"\nto file\n\""+dest+"\"."+(messages.is() ? S+"\n\nCompilation Messages:\n"+messages : S));
      }
   }
};
#if WINDOWS
// FIXME don't list constant buffers that have 'bind_explicit' in vs_buffers, etc, but LIST in buffers
struct ShaderCompiler1
{
   struct Source;
   struct Shader;
   struct TextParam8
   {
      Str8 name, value;
      void set(C Str8 &name, C Str8 &value) {T.name=name; T.value=value;}
   };
   struct Param
   {
      Str8 name;
      Int  elms, cpu_data_size=0;
      Mems<ShaderParam::Translation> translation;

      void addTranslation(ID3D11ShaderReflectionType *type, C D3D11_SHADER_TYPE_DESC &type_desc, Int &offset, CChar8 *name)
      {
         // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
         if(type_desc.Elements)offset=Ceil16(offset); // arrays are 16-byte aligned (even 1-element arrays "f[1]"), non-arrays have Elements=0, so check Elements!=0
         Int  elms=Max(type_desc.Elements, 1), last=elms-1; // 'Elements' is array size (it's 0 for non-arrays)
         FREP(elms)
         {
            if(type_desc.Class==D3D_SVC_SCALAR || type_desc.Class==D3D_SVC_VECTOR) // for example: Flt f,f[]; Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
            {
               if(type_desc.Rows!=1)Exit("Shader Param Rows!=1");
               if(type_desc.Type==D3D_SVT_FLOAT)
               {
                  Int size=SIZE(Flt)*type_desc.Columns;
                  if(offset/16 != (offset+size-1)/16)offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."
                  translation.New().set(cpu_data_size, offset, size);
                  cpu_data_size+=size;
                         offset+=(i==last) ? size : Ceil16(size); // arrays are 16-byte aligned, and last element is 'size' only
               }else Exit(S+"Unhandled Shader Parameter Type for \""+name+'"');
            }else
            if(type_desc.Class==D3D_SVC_MATRIX_COLUMNS)
            {
               if(type_desc.Rows   >4)Exit("Shader Param Matrix Rows>4");
               if(type_desc.Columns>4)Exit("Shader Param Matrix Cols>4");
               if(type_desc.Type!=D3D_SVT_FLOAT)Exit(S+"Unhandled Shader Parameter Type for \""+name+'"');

               Int size=SIZE(Flt)*type_desc.Rows*type_desc.Columns;
               if(offset/16 != (offset+size-1)/16)offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."
               FREPD(y, type_desc.Columns)
               FREPD(x, type_desc.Rows   )translation.New().set(cpu_data_size+SIZE(Flt)*(y+x*type_desc.Columns), offset+SIZE(Flt)*(x+y*4), SIZE(Flt));
               cpu_data_size+=size;
                      offset+=(i==last) ? size : Ceil16(size); // arrays are 16-byte aligned, and last element is 'size' only
            }else
            if(type_desc.Class==D3D_SVC_STRUCT)
            {
               offset=Ceil16(offset); // "Each structure forces the next variable to start on the next four-component vector."
               FREP(type_desc.Members) // iterate all struct members
               {
                  ID3D11ShaderReflectionType *member=type->GetMemberTypeByIndex(i); if(!member)Exit("'GetMemberTypeByIndex' failed");
                  D3D11_SHADER_TYPE_DESC member_desc; if(!OK(member->GetDesc(&member_desc)))Exit("'ID3D11ShaderReflectionType.GetDesc' failed");
                  addTranslation(member, member_desc, offset, type->GetMemberTypeName(i));
               }
             //offset=Ceil16(offset); // "Each structure forces the next variable to start on the next four-component vector." even though documentation examples indicate this should align too, actual tests confirm that's not the case
            }
         }
      }
   };
   struct Buffer
   {
      Str8 name;
      Int  size, bind_slot;
      Bool bind_explicit;
      Mems<Param> params;
   };
   struct Image
   {
      Str8 name;
      Int  bind_slot;
   };
   enum SHADER_TYPE : Byte
   {
      VS,
      HS,
      DS,
      PS,
      ST_NUM,
   };
   enum API : Byte
   {
      API_DX,
      API_GL,
      API_VULKAN,
      API_METAL,
      API_NUM,
   };
   static inline CChar8* APIName[]=
   {
      "DX",
      "GL",
      "VULKAN",
      "METAL",
   };
   struct SubShader
   {
    C Shader      *shader;
      SHADER_TYPE  type;
      Str8         func_name,
                   error;
      Mems<Buffer> buffers;
      Mems<Image > images;

      Bool is()C {return func_name.is();}
      void compile();
   };
   static void Compile(SubShader &shader, Ptr user, Int thread_index) {shader.compile();}
   struct Shader
   {
      Str              name;
      SHADER_MODEL     model;
      Memc<TextParam8> params;
      SubShader        sub[ST_NUM];
    C Source          *src;

      Shader& Model(SHADER_MODEL model) {T.model=model; return T;} // override model (needed for tesselation)

      Shader& operator()(C Str &n0, C Str &v0                                                                  ) {params.New().set(n0, v0);                                                                               return T;}
      Shader& operator()(C Str &n0, C Str &v0, C Str &n1, C Str &v1                                            ) {params.New().set(n0, v0); params.New().set(n1, v1);                                                     return T;}
      Shader& operator()(C Str &n0, C Str &v0, C Str &n1, C Str &v1, C Str &n2, C Str &v2                      ) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2);                           return T;}
      Shader& operator()(C Str &n0, C Str &v0, C Str &n1, C Str &v1, C Str &n2, C Str &v2, C Str &n3, C Str &v3) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2); params.New().set(n3, v3); return T;}

      void finalizeName()
      {
         FREPA(params)
         {
          C TextParam8 &p=params[i]; if(p.value.length()!=1)Exit("Shader Param Value Length != 1");
            name+=p.value;
         }
      }
   };
   struct Source
   {
      Str              file_name;
      Mems<Byte>       file_data;
      Memc<Shader>     shaders;
      SHADER_MODEL     model;
      ShaderCompiler1 *compiler;

      Shader& New(C Str &name, C Str8 &vs, C Str8 &ps)
      {
         Shader &shader=shaders.New();
         shader.model=model;
         shader.name =name;
         shader.sub[VS].func_name=vs;
         shader.sub[PS].func_name=ps;
         return shader;
      }
      Bool load()
      {
         File f; if(!f.readTry(file_name))return false;
         file_data.setNum(f.size()); if(!f.getFast(file_data.data(), file_data.elms()))return false;
         return true;
      }
   };
   Str          dest, messages;
   SHADER_MODEL model;
   API          api;
   Memc<Source> sources;

   void message(C Str &t) {messages.line()+=t;}
   Bool error(C Str &t) {message(t); return false;}

   ShaderCompiler1& set(C Str &dest, SHADER_MODEL model, API api=API_DX)
   {
      T.dest =dest ;
      T.model=model;
      T.api  =api  ;
      return T;
   }
   Source& New(C Str &src)
   {
      Source &source=sources.New();
      source.file_name=src;
      source.model    =model;
      source.compiler =this;
      return source;
   }
   Bool compile(Threads &threads)
   {
      FREPA(sources)
      {
         Source &source=sources[i];
         if(!source.load())return error(S+"Can't open file:"+source.file_name);
         FREPA(source.shaders)
         {
            Shader &shader=source.shaders[i];
            shader.src=&source; // link only during compilation because sources use Memc container which could change addresses while new sources were being added, however at this stage all have already been created
            FREPA(shader.sub)
            {
               SubShader &sub=shader.sub[i]; if(sub.is())
               {
                  sub.type  =(SHADER_TYPE)i;
                  sub.shader=&shader;
                  threads.queue(sub, Compile);
               }
            }
         }
         threads.wait();
      }
      return true;
   }
};
struct Include11 : ID3DInclude
{
   struct ShaderPath
   {
      Char path[MAX_LONG_PATH];
   };
   ShaderPath root;

   HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
   {
     *ppData=null;
     *pBytes=0;

      Str path=GetPath(pFileName);
      if(!FullPath(path))
         if(ShaderPath *parent=(pParentData ? (ShaderPath*)pParentData-1 : &root))
            path=NormalizePath(Str(parent->path).tailSlash(true)+path);

      File f; if(f.readStdTry(path.tailSlash(true)+GetBase(pFileName)))
      {
         Byte *data=Alloc<Byte>(SIZEU(ShaderPath)+f.size());
         Set(((ShaderPath*)data)->path, path);
         data+=SIZE(ShaderPath);
         f.get(data, f.size());
        *ppData=data;
        *pBytes=f.size();
         return S_OK;
      }
      return -1;
   }
   HRESULT __stdcall Close(LPCVOID pData)
   {
      if(pData)
      {
         Byte *data=((Byte*)pData)-SIZE(ShaderPath);
         Free( data);
      }
      return S_OK;
   }

   Include11(C Str &src)
   {
      Set(root.path, GetPath(src));
   }
};
void ShaderCompiler1::SubShader::compile()
{
 C Source          *src     =shader->src;
 C ShaderCompiler1 *compiler=src->compiler;
   Char8 target[6+1];
   switch(type)
   {
      case VS: target[0]='v'; break;
      case HS: target[0]='h'; break;
      case DS: target[0]='d'; break;
      case PS: target[0]='p'; break;
   }
   target[1]='s';
   target[2]='_';
   target[4]='_';
   target[6]='\0';
   SHADER_MODEL model=shader->model; if(type==HS || type==DS)MAX(model, SM_5); // HS DS are supported only in SM5+
   switch(model)
   {
      case SM_4  : target[3]='4'; target[5]='0'; break;
      case SM_4_1: target[3]='4'; target[5]='1'; break;
      case SM_5  : target[3]='5'; target[5]='0'; break;
      default    : Exit("Invalid Shader Model"); break;
   }

   MemtN<D3D_SHADER_MACRO, 64> macros;
   Int params=shader->params.elms();
   macros.setNum(params+API_NUM+1);
   FREP(params)
   {
      D3D_SHADER_MACRO &macro=macros[i]; C TextParam8 &param=shader->params[i];
      macro.Name      =param.name;
      macro.Definition=param.value;
   }
   FREP(API_NUM)
   {
      macros[params+i].Name      =APIName[i];
      macros[params+i].Definition=((compiler->api==i) ? "1" : "0");
   }
   Zero(macros.last()); // must be null-terminated

   ID3DBlob *buffer=null, *error_blob=null;
   D3DCompile(src->file_data.data(), src->file_data.elms(), (Str8)src->file_name, macros.data(), &Include11(src->file_name), func_name, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &buffer, &error_blob);
   if(error_blob){error=(Char8*)error_blob->GetBufferPointer(); error_blob->Release();}
   Bool ok=false;
   if(buffer)
   {
      ID3D11ShaderReflection *reflection=null; D3DReflect(buffer->GetBufferPointer(), buffer->GetBufferSize(), IID_ID3D11ShaderReflection, (Ptr*)&reflection); if(reflection)
      {
         D3D11_SHADER_DESC desc; if(!OK(reflection->GetDesc(&desc))){error.line()+="'ID3D11ShaderReflection.GetDesc' failed.";}else
         {
            Int images_elms=0, buffers_elms=0;
            FREP(desc.BoundResources)
            {
               D3D11_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error;}
               switch(desc.Type)
               {
                  case D3D_SIT_TEXTURE:  images_elms++; break;
                  case D3D_SIT_CBUFFER: buffers_elms++; break;
               }
            }
             images.setNum( images_elms);  images_elms=0;
            buffers.setNum(buffers_elms); buffers_elms=0;
            FREP(desc.BoundResources)
            {
               D3D11_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error;}
               switch(desc.Type)
               {
                  case D3D_SIT_TEXTURE:
                  {
                     if(!InRange(desc.BindPoint, MAX_TEXTURES)){error.line()+=S+"Texture index: "+desc.BindPoint+", is too big"; goto error;}
                     Image &image=images[images_elms++]; image.name=desc.Name; image.bind_slot=desc.BindPoint;
                  }break;

                  case D3D_SIT_CBUFFER:
                  {
                     if(!InRange(desc.BindPoint, MAX_SHADER_BUFFERS)){error.line()+=S+"Constant Buffer index: "+desc.BindPoint+", is too big"; goto error;}
                     Buffer &buffer=buffers[buffers_elms++];
                     buffer.name=desc.Name;
                     buffer.bind_slot=desc.BindPoint;
                     buffer.bind_explicit=FlagTest(desc.uFlags, D3D_SIF_USERPACKED);
                     ID3D11ShaderReflectionConstantBuffer *cb=reflection->GetConstantBufferByName(desc.Name); if(!cb){error.line()+="'GetConstantBufferByIndex' failed."; goto error;}
                     {
                        D3D11_SHADER_BUFFER_DESC desc; if(!OK(cb->GetDesc(&desc))){error.line()+="'ID3D11ShaderReflectionConstantBuffer.GetDesc' failed."; goto error;}
                        buffer.size=desc.Size;
                        buffer.params.setNum(desc.Variables);
                        FREP(desc.Variables)
                        {
                           ID3D11ShaderReflectionVariable *var=cb->GetVariableByIndex(i); if(!var){error.line()+="'GetVariableByIndex' failed."; goto error;}
                           ID3D11ShaderReflectionType *type=var->GetType(); if(!type){error.line()+="'GetType' failed."; goto error;}
                           D3D11_SHADER_VARIABLE_DESC var_desc; if(!OK( var->GetDesc(& var_desc))){error.line()+="'ID3D11ShaderReflectionVariable.GetDesc' failed."; goto error;}
                           D3D11_SHADER_TYPE_DESC    type_desc; if(!OK(type->GetDesc(&type_desc))){error.line()+="'ID3D11ShaderReflectionType.GetDesc' failed."; goto error;}

                           Param &param=buffer.params[i];
                           param.name=var_desc.Name;
                           param.elms=type_desc.Elements;
                           var_desc.DefaultValue; // FIXME
                           Int offset=var_desc.StartOffset; param.addTranslation(type, type_desc, offset, var_desc.Name);
               
               /*sp._gpu_data_size=type.UnpackedSize;
             //sp._constant_count= unused on DX10+

               if(sp._cpu_data_size!=type.  PackedSize
               || sp._gpu_data_size!=type.UnpackedSize)Exit("Incorrect Shader Param size.\nPlease contact Developer.");
               if(sp._gpu_data_size+sp._full_translation[0].gpu_offset>buf.size())Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
             //if(SIZE(Vec4)       +sp._full_translation[0].gpu_offset>buf.size())Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4*/

                         //type->Release(); this doesn't have 'Release'
                         //var ->Release(); this doesn't have 'Release'
                        }
                      //DEBUG_ASSERT(buffer.bind_explicit==FlagTest(desc.uFlags, D3D_CBF_USERPACKED), "bind_explicit mismatch"); ignore because looks like 'desc.uFlags' is not set
                     }
                  //cb->Release(); this doesn't have 'Release'
                  }break;
               }
            }
            ok=true;
      // FIXME: verify that stages have matching input->output
         }
      error:
         reflection->Release();
      }
      buffer->Release();
   }
   if(!ok)Exit(error);
}
static Memx<ShaderCompiler1> ShaderCompiler1s; // use Memx because we store pointers to 'ShaderCompiler1'
#endif
/******************************************************************************/
static Memc<ShaderCompiler> ShaderCompilers;
/******************************************************************************/
static void Add(C Str &src, C Str &dest, SHADER_MODEL model, C MemPtr<ShaderGLSL> &glsl=null)
{
   ShaderCompiler &sc=ShaderCompilers.New();
   sc.src  =src  ;
   sc.dest =dest ;
   sc.model=model;
   sc.glsl =glsl ;
}
static void Add(C Str &src, C Str &dest, SHADER_MODEL model, C Str &names, C MemPtr<ShaderGLSL> &glsl=null)
{
   ShaderCompiler &sc=ShaderCompilers.New();
   sc.src  =src  ;
   sc.dest =dest ;
   sc.model=model;
   sc.glsl =glsl ;
   sc.macros.New().set("CUSTOM_TECHNIQUE", names);
}
/******************************************************************************/
// LISTING ALL SHADER TECHNIQUES
/******************************************************************************/
static void Compile(SHADER_MODEL model)
{
   if(!DataPath().is())Exit("Can't compile default shaders - 'DataPath' not specified");

   Str src,
       src_path=GetPath(GetPath(__FILE__))+"\\Shaders\\", // for example "C:/Esenthel/Engine/Src/Shaders/"
      dest_path=DataPath();                               // for example "C:/Esenthel/Data/Shader/4/"
   switch(model)
   {
      case SM_UNKNOWN: return;

      case SM_GL_ES_3:
      case SM_GL     : dest_path+="Shader\\GL\\"; break;

      default        : dest_path+="Shader\\4\\" ; break;
   }
   FCreateDirs(dest_path);

   // list first those that take the most time to compile

#ifdef FORWARD
{
#if GL && !X64 && COMPILE_GL
   #error "Can't compile GL forward shaders on 32-bit because you will run out of memory"
#endif
   Str names;

   // zero
   REPD(skin , 2)
   REPD(color, 2)names+=TechForward(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE,   false,false,0,   false,false,   false,false,   false,false,   false);

   REPD(tess     , (model>=SM_4) ? 2 : 1)
   REPD(heightmap, 2)
   {
      // 1 material, 0-2 textures, flat
      REPD(skin  , heightmap ? 1 : 2)
      REPD(detail, 2)
      REPD(rflct , 2)
      REPD(color , 2)
      {
         names+=TechForwardLight(skin, 1, 0, SBUMP_FLAT, false, false, detail, rflct, color, false, heightmap, FX_NONE, tess, model); // 1 material, 0 tex
         REPD(alpha_test, heightmap ? 1 : 2)
         REPD(light_map , 2)
         {
            names+=TechForwardLight(skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess, model); // 1 material, 1 tex
            names+=TechForwardLight(skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess, model); // 1 material, 2 tex
         }
      }

   #if BUMP_MAPPING
      // 1 material, 2 tex, normal
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(light_map , 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)names+=TechForwardLight(skin, 1, 2, SBUMP_NORMAL, alpha_test, light_map, detail, rflct, color, false, heightmap, FX_NONE, tess, model);
   #endif

   #if MULTI_MATERIAL
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      REPD(color     , 2)
      REPD(mtrl_blend, 2)
      REPD(rflct     , 2)
      {
         // 2-4 materials, 1-2 textures, flat
         for(Int textures=1; textures<=2; textures ++)names+=TechForwardLight(false, materials, textures, SBUMP_FLAT, false, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess, model);

      #if BUMP_MAPPING
         // 2-4 materials, 2 textures, normal
         names+=TechForwardLight(false, materials, 2, SBUMP_NORMAL, false, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess, model);
      #endif
      }
   #endif
   }

   // grass + leaf
   for(Int textures=1; textures<=2; textures++)
   REPD(color, 2)
   {
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_GRASS, false, model); // 1 material, 1-2 tex, grass, flat
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAF , false, model); // 1 material, 1-2 tex, leaf , flat
      names+=TechForwardLight(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAFS, false, model); // 1 material, 1-2 tex, leafs, flat
      if(textures==2)
      {
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_GRASS, false, model); // 1 material, 1-2 tex, grass, normal
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAF , false, model); // 1 material, 1-2 tex, leaf , normal
         names+=TechForwardLight(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAFS, false, model); // 1 material, 1-2 tex, leafs, normal
      }
   }

   Add(src_path+"Forward.cpp", dest_path+"Forward", model, names);
}
#endif

#ifdef BLEND_LIGHT
{
#if GL && !X64 && COMPILE_GL
   #error "Can't compile GL Blend Light shaders on 32-bit because you will run out of memory"
#endif
   Str names; Memc<ShaderGLSL> glsl;

   REPD(per_pixel  , 2)
   REPD(shadow_maps, per_pixel ? 7 : 1) // 7=(6+off), 1=off
   {
      // base
      REPD(skin      , 2)
      REPD(color     , 2)
      REPD(textures  , 3)
      REPD(bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test,               textures     ? 2 : 1)
      REPD(alpha     ,               textures     ? 2 : 1)
      REPD(light_map ,               textures     ? 2 : 1)
      REPD(rflct     , 2)
      {
                                            names+=(TechBlendLight    (skin, color, textures, bump_mode ? SBUMP_NORMAL: SBUMP_FLAT, alpha_test, alpha, light_map, rflct, FX_NONE, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(skin, color, textures, bump_mode ? SBUMP_NORMAL: SBUMP_FLAT, alpha_test, alpha, light_map, rflct, FX_NONE, per_pixel, shadow_maps));
      }

      // grass+leaf
      REPD(color     , 2)
      REPD(textures  , 3)
      REPD(bump_mode , (per_pixel && textures==2) ? 2 : 1)
      REPD(alpha_test,               textures     ? 2 : 1)
      {
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_GRASS, per_pixel, shadow_maps));
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAF , per_pixel, shadow_maps));
                                            names+=(TechBlendLight    (false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAFS, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_GRASS, per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAF , per_pixel, shadow_maps));
         if(shadow_maps==0 && bump_mode==0)glsl.add(TechBlendLightGlsl(false, color, textures, bump_mode ? SBUMP_NORMAL : SBUMP_FLAT, alpha_test, true, false, false, FX_LEAFS, per_pixel, shadow_maps));
      }
   }

   Add(src_path+"Blend Light.cpp", dest_path+"Blend Light", model, names, glsl);
}
#endif

#ifdef DEFERRED
{
   Str names;

   // zero
   REPD(skin , 2)
   REPD(color, 2)names+=TechDeferred(skin, 1, 0, SBUMP_ZERO, false, false, false, false, color, false, false, FX_NONE, false);

   REPD(tess     , (model>=SM_4) ? 2 : 1)
   REPD(heightmap, 2)
   {
      // 1 material, 0-2 tex, flat
      REPD(skin  , heightmap ? 1 : 2)
      REPD(detail, 2)
      REPD(rflct , 2)
      REPD(color , 2)
      {
         names+=TechDeferred(skin, 1, 0, SBUMP_FLAT, false, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 0 tex
         REPD(alpha_test, heightmap ? 1 : 2)
         {
            names+=TechDeferred(skin, 1, 1, SBUMP_FLAT, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 1 tex
            names+=TechDeferred(skin, 1, 2, SBUMP_FLAT, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess); // 1 material, 2 tex
         }
      }

      // 1 material, 1-2 tex, flat, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, 1, textures, SBUMP_FLAT, false, false, true, false, color, false, heightmap, FX_NONE, tess);

   #if BUMP_MAPPING
      // 1 material, 2 tex, normal + parallax
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)
      for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
         names+=TechDeferred(skin, 1, 2, bump_mode, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess);

      // 1 material, 1-2 tex, normal, macro
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, false, false, true, false, color, false, heightmap, FX_NONE, tess);

      // 1 material, 2 tex, relief
      REPD(skin      , heightmap ? 1 : 2)
      REPD(alpha_test, heightmap ? 1 : 2)
      REPD(detail    , 2)
      REPD(rflct     , 2)
      REPD(color     , 2)names+=TechDeferred(skin, 1, 2, SBUMP_RELIEF, alpha_test, detail, false, rflct, color, false, heightmap, FX_NONE, tess);
   #endif

   #if MULTI_MATERIAL
      for(Int materials=2; materials<=MAX_MTRLS; materials++)
      REPD(color     , 2)
      REPD(mtrl_blend, 2)
      REPD(rflct     , 2)
      {
         // 2-4 materials, 1-2 tex, flat
         REPD(detail, 2)
         for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, materials, textures, SBUMP_FLAT, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 1-2 tex, flat, macro
         for(Int textures=1; textures<=2; textures++)names+=TechDeferred(false, materials, textures, SBUMP_FLAT, false, false, true, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

      #if BUMP_MAPPING
         // 2-4 materials, 2 textures, normal + parallax
         REPD(detail, 2)
         for(Int bump_mode=SBUMP_NORMAL; bump_mode<=SBUMP_PARALLAX_MAX_MULTI; bump_mode++)if(bump_mode==SBUMP_NORMAL || bump_mode>=SBUMP_PARALLAX_MIN)
            names+=TechDeferred(false, materials, 2, bump_mode, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 2 textures, normal, macro
         names+=TechDeferred(false, materials, 2, SBUMP_NORMAL, false, false, true, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);

         // 2-4 materials, 2 textures, relief
         REPD(detail, 2)
            names+=TechDeferred(false, materials, 2, SBUMP_RELIEF, false, detail, false, rflct, color, mtrl_blend, heightmap, FX_NONE, tess);
      #endif
      }
   #endif
   }

   // grass + leaf
   for(Int textures=1; textures<=2; textures++)
   REPD(color, 2)
   {
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, flat
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , flat
      names+=TechDeferred(false, 1, textures, SBUMP_FLAT, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, flat
      if(textures==2)
      {
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_GRASS, false); // 1 material, 1-2 tex, grass, normal
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAF , false); // 1 material, 1-2 tex, leaf , normal
         names+=TechDeferred(false, 1, textures, SBUMP_NORMAL, true, false, false, false, color, false, false, FX_LEAFS, false); // 1 material, 1-2 tex, leafs, normal
      }
   }

   Add(src_path+"Deferred.cpp", dest_path+"Deferred", model, names);
}
#endif

#ifdef SIMPLE
{
   Str names; Memc<ShaderGLSL> glsl;

   REPD(per_pixel, 2)
   {
      // zero
      REPD(skin , 2)
      REPD(color, 2)
      {
          names+=(TechSimple    (skin, 1, 0, SBUMP_ZERO, false, false, false, color, false, false, FX_NONE, per_pixel, false));
         glsl.add(TechSimpleGlsl(skin, 1, 0, SBUMP_ZERO, false, false, false, color, false, false, FX_NONE, per_pixel, false));
      }

      REPD(tess     , (model>=SM_4) ? 2 : 1)
      REPD(heightmap, 2)
      {
         // 1 material, 0-2 textures
         REPD(skin    , heightmap ? 1 : 2)
         REPD(rflct   , 2)
         REPD(color   , 2)
       //REPD(instance, (model>=SM_4 && !skin) ? 2 : 1)
         {
             names+=(TechSimple    (skin, 1, 0, SBUMP_FLAT, false, false, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 0 tex
            glsl.add(TechSimpleGlsl(skin, 1, 0, SBUMP_FLAT, false, false, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
            REPD(alpha_test, heightmap ? 1 : 2)
            REPD(light_map , 2)
            {
                names+=(TechSimple    (skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 1 tex
                names+=(TechSimple    (skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess)); // 1 material, 2 tex
               glsl.add(TechSimpleGlsl(skin, 1, 1, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
               glsl.add(TechSimpleGlsl(skin, 1, 2, SBUMP_FLAT, alpha_test, light_map, rflct, color, false, heightmap, FX_NONE, per_pixel, tess));
            }
         }

      #if MULTI_MATERIAL
         // 2-4 materials, 1 textures
         for(Int materials=2; materials<=MAX_MTRLS; materials++)
         REPD(color     , 2)
         REPD(mtrl_blend, 2)
         REPD(rflct     , 2)
         {
                       names+=(TechSimple    (false, materials, 1, SBUMP_FLAT, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tess));
            if(!rflct)glsl.add(TechSimpleGlsl(false, materials, 1, SBUMP_FLAT, false, false, rflct, color, mtrl_blend, heightmap, FX_NONE, per_pixel, tess));
         }
      #endif
      }

      // grass + leaf
    //REPD(instance, (model>=SM_4) ? 2 : 1)
      REPD(color, 2)
      for(Int textures=1; textures<=2; textures++)
      {
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_GRASS, per_pixel, false));
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAF , per_pixel, false));
          names+=(TechSimple    (false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAFS, per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_GRASS, per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAF , per_pixel, false));
         glsl.add(TechSimpleGlsl(false, 1, textures, SBUMP_FLAT, true, false, false, color, false, false, FX_LEAFS, per_pixel, false));
      }
   }

   // bone highlight
   REPD(skin, 2)names+=TechSimple(skin, 1, 0, SBUMP_FLAT, false, false, false, false, false, false, FX_BONE, true, false); // !! this name must be in sync with other calls in the engine that use FX_BONE !!

   Add(src_path+"Simple.cpp", dest_path+"Simple", model, names, glsl);
}
#endif

#ifdef MAIN
{
   Memc<ShaderGLSL> glsl;

   // Draw2DTex
   REPD(c, 2) // color
      glsl.New().set("Draw2DTex", S+"Draw2DTex"+(c?'C':'\0')).par("COLOR", c);

   // Draw3DTex
   REPD(c , 2) // color
   REPD(at, 2) // alpha_test
      glsl.New().set("Draw3DTex", S+"Draw3DTex"+(c?"Col":"")+(at?"AT":"")).par("COLOR", c).par("alpha_test", at);

   // font
   glsl.New().set("Font", "Font");

   // blur
   REPD(x, 2) // axis
   REPD(h, 2) // high
      glsl.New().set("Blur", S+"Blur"+(x?'X':'Y')+(h?'H':'\0')).par("axis", x ? '0' : '1').par("high", h);

   // bloom downsample
   REPD(g, 2) // glow
   REPD(c, 2) // clamp
   REPD(h, 2) // half
   REPD(s, 2) // saturate
      glsl.New().set("BloomDS", S+"Bloom"+(g?'G':'\0')+"DS"+(c?'C':'\0')+(h?'H':'\0')+(s?'S':'\0')).par("DoGlow", g).par("DoClamp", c).par("half", h).par("saturate", s);

   // bloom
   glsl.New().set("Bloom", "Bloom");

   // shd blur
   glsl.New().set("ShdBlurX", "ShdBlurX2").par("range", 2);
   glsl.New().set("ShdBlurY", "ShdBlurY2").par("range", 2);

   // particles
   REPD(p, 2) // palette
   REPD(a, 3) // anim
   REPD(m, 2) // motion
      glsl.New().set("Particle", S+"ParticleTex"+(p?'P':'\0')+((a==2)?"AA":(a==1)?"A":"")+(m?'M':'\0')).par("palette",  p ).par("anim",  a ).par("motion_stretch", "1").par("stretch_alpha",  m );
      glsl.New().set("Particle", S+"Bilb"                                                             ).par("palette", "0").par("anim", "0").par("motion_stretch", "0").par("stretch_alpha", "0");

   // sky
      // Textures Flat
      REPD(t, 2) // textures
      REPD(c, 2) // clouds
         glsl.New().set("Sky", S+"SkyTF"+(t+1)+(c?'C':'\0')).par("per_vertex", "0").par("DENSITY", "0").par("textures", t+1).par("stars", "0").par("clouds", c);
      // Atmospheric Flat
      REPD(v, 2) // per-vertex
      REPD(s, 2) // stars
      REPD(c, 2) // clouds
         glsl.New().set("Sky", S+"SkyAF"+(v?'V':'\0')+(s?'S':'\0')+(c?'C':'\0')).par("per_vertex", v).par("DENSITY", "0").par("textures", "0").par("stars", s).par("clouds", c);

   // AA
#if 0 // disable GLSL versions because neither Mac/Linux succeed in compiling them
   REPD(g, 2) // gamma
      glsl.New().set("SMAAEdge" , S+"SMAAEdgeColor"+(g?'G':'\0')).par("GAMMA", TextBool(g));
      glsl.New().set("SMAABlend", "SMAABlend");
      glsl.New().set("SMAA"     , "SMAA");
#endif

   Add(src_path+"Main.cpp", dest_path+"Main", model, glsl);
}
#endif

#ifdef BLEND
{
   Str names; Memc<ShaderGLSL> glsl;

   REPD(skin    , 2)
   REPD(color   , 2)
   REPD(rflct   , 2)
   REPD(textures, 3)
   {
       names+=(TechBlend    (skin, color, rflct, textures));
      glsl.add(TechBlendGlsl(skin, color, rflct, textures));
   }

   Add(src_path+"Blend.cpp", dest_path+"Blend", model, names, glsl);
}
#endif

#ifdef AMBIENT_OCCLUSION
   Add(src_path+"Ambient Occlusion.cpp", dest_path+"Ambient Occlusion", model);
#endif

#ifdef AMBIENT_OCCLUSION_NEW
   {
      ShaderCompiler1::Source &src=ShaderCompiler1s.New().set(dest_path+"Ambient Occlusion", model).New(src_path+"Ambient Occlusion.cpp");
      REPD(mode  , 4)
      REPD(jitter, 2)
      REPD(normal, 2)
         src.New("AO", "AO_VS", "AO_PS")("MODE", mode, "JITTER", jitter, "NORMALS", normal);
   }
#endif

#ifdef DEPTH_OF_FIELD
   Add(src_path+"Depth of Field.cpp", dest_path+"Depth of Field", model);
#endif

#ifdef LAYERED_CLOUDS
   Add(src_path+"Layered Clouds.cpp", dest_path+"Layered Clouds", model);
#endif

#ifdef EFFECTS_2D
   Add(src_path+"Effects 2D.cpp", dest_path+"Effects 2D", model);
#endif

#ifdef EFFECTS_3D
   Add(src_path+"Effects 3D.cpp", dest_path+"Effects 3D", model);
#endif

#ifdef FOG_LOCAL
   Add(src_path+"Fog Local.cpp", dest_path+"Fog Local", model);
#endif

#ifdef FXAA
{
   Memc<ShaderGLSL> glsl;
#if 0 // disable GLSL versions because neither Mac/Linux succeed in compiling them
   REPD(g, 2) // gamma
      glsl.New().set("FXAA", S+"FXAA"+(g?'G':'\0')).par("GAMMA", TextBool(g));
#endif
   Add(src_path+"FXAA.cpp", dest_path+"FXAA", model, glsl);
}
#endif

#ifdef HDR
   Add(src_path+"Hdr.cpp", dest_path+"Hdr", model);
#endif

#ifdef MOTION_BLUR
   Add(src_path+"Motion Blur.cpp", dest_path+"Motion Blur", model);
#endif

#ifdef VOLUMETRIC_CLOUDS
   Add(src_path+"Volumetric Clouds.cpp", dest_path+"Volumetric Clouds", model);
#endif

#ifdef VOLUMETRIC_LIGHTS
   Add(src_path+"Volumetric Lights.cpp", dest_path+"Volumetric Lights", model);
#endif

#ifdef WATER
   Add(src_path+"Water.cpp", dest_path+"Water", model);
#endif

#ifdef POSITION
{
   Str names;

   REPD(tess, (model>=SM_4) ? 2 : 1)
   {
      // base
      REPD(skin      , 2               )
      REPD(textures  , 3               )
      REPD(test_blend, textures ? 2 : 1)names+=TechPosition(skin, textures, test_blend, FX_NONE, tess);
   }

   // grass + leafs
   for(Int textures=1; textures<=2; textures++)
   REPD(test_blend, 2)
   {
      names+=TechPosition(0, textures, test_blend, FX_GRASS, false);
      names+=TechPosition(0, textures, test_blend, FX_LEAF , false);
      names+=TechPosition(0, textures, test_blend, FX_LEAFS, false);
   }

   Add(src_path+"Position.cpp", dest_path+"Position", model, names);
}
#endif

#ifdef SET_COLOR
{
   Str names;

   // base
   REPD(tess    , (model>=SM_4) ? 2 : 1)
   REPD(skin    , 2)
   REPD(textures, 3)names+=TechSetColor(skin, textures, tess);

   Add(src_path+"Set Color.cpp", dest_path+"Set Color", model, names);
}
#endif

#ifdef BEHIND
{
   Str names;

   // base
   REPD(skin    , 2)
   REPD(textures, 3)names+=TechBehind(skin, textures);

   Add(src_path+"Behind.cpp", dest_path+"Behind", model, names);
}
#endif

#ifdef AMBIENT
{
   Str names;

   // base
   REPD(skin      , 2)
   REPD(alpha_test, 3)
   REPD(light_map , 2)
      names+=TechAmbient(skin, alpha_test, light_map);

   Add(src_path+"Ambient.cpp", dest_path+"Ambient", model, names);
}
#endif

#ifdef OVERLAY
{
   Str names;

   // base
   REPD(skin  , 2)
   REPD(normal, 2)names+=TechOverlay(skin, normal);

   Add(src_path+"Overlay.cpp", dest_path+"Overlay", model, names);
}
{
   Str names;

   // base
   REPD(tess, (model>=SM_4) ? 2 : 1)
   REPD(skin, 2)names+=TechTattoo(skin, tess);

   Add(src_path+"Tattoo.cpp", dest_path+"Tattoo", model, names);
}
#endif

#ifdef EARLY_Z
{
   Str names;

   // base
   REPD(skin, 2)names+=TechEarlyZ(skin);

   Add(src_path+"Early Z.cpp", dest_path+"Early Z", model, names);
}
#endif

#ifdef FUR
{
   Str names;

   // base
   REPD(skin   , 2)
   REPD(size   , 2)
   REPD(diffuse, 2)names+=TechFurBase(skin, size, diffuse);

   // soft
   REPD(skin   , 2)
   REPD(size   , 2)
   REPD(diffuse, 2)names+=TechFurSoft(skin, size, diffuse);

   Add(src_path+"Fur.cpp", dest_path+"Fur", model, names);
}
#endif

#ifdef WORLD_EDITOR
   Add(src_path+"World Editor.cpp", dest_path+"World Editor", model);
#endif

#ifdef DX10_INPUT_LAYOUT
   Add(src_path+"DX10+ Input Layout.cpp", S, model);
#endif
}
/******************************************************************************/
static void ThreadCompile(ShaderCompiler &shader_compiler, Ptr user, Int thread_index)
{
   ThreadMayUseGPUData();
   shader_compiler.compile();
}
/******************************************************************************/
#endif // COMPILE
/******************************************************************************/
void MainShaderClass::compile()
{
#if COMPILE_4 || COMPILE_GL
   App.stayAwake(AWAKE_SYSTEM);

#if COMPILE_GL
   Compile(SM_GL);
#endif
#if COMPILE_4
   Compile(SM_4);
#endif

   ProcPriority(-1); // compiling shaders may slow down entire CPU, so make this process have smaller priority
   Dbl t=Time.curTime();
   MultiThreadedCall(ShaderCompilers, ThreadCompile);
   Threads threads; threads.create(false, Cpu.threads()-1);
   FREPAO(ShaderCompiler1s).compile(threads);
   LogN(S+"Shaders compiled in: "+Flt(Time.curTime()-t)+'s');

   App.stayAwake(AWAKE_OFF);
#endif
}
/******************************************************************************/
}
/******************************************************************************/
