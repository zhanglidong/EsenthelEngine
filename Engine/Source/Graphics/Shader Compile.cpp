/******************************************************************************/
#include "stdafx.h"

#define NEW_COMPILER 0
#define SPIRV_CROSS 1
#define HLSL_CC 0

#if WINDOWS && NEW_COMPILER
const UINT32 CP_UTF16=1200;
#include "../../../ThirdPartyLibs/begin.h"
#include <vector>
#include <string>
#include "../../../ThirdPartyLibs/DirectXShaderCompiler/include/dxc/dxcapi.h"
#include "../../../ThirdPartyLibs/DirectXShaderCompiler/include/dxc/Support/microcom.h"
#include "../../../ThirdPartyLibs/DirectXShaderCompiler/include/dxc/DxilContainer/DxilContainer.h"
#include "../../../ThirdPartyLibs/end.h"
namespace llvm
{
   namespace sys
   {
      typedef UInt cas_flag;
   }
}
static HRESULT CreateLibrary            (IDxcLibrary             **pLibrary    ) {return DxcCreateInstance(CLSID_DxcLibrary            , __uuidof(IDxcLibrary            ), (void**)pLibrary    );}
static HRESULT CreateCompiler           (IDxcCompiler            **ppCompiler  ) {return DxcCreateInstance(CLSID_DxcCompiler           , __uuidof(IDxcCompiler           ), (void**)ppCompiler  );}
static HRESULT CreateContainerReflection(IDxcContainerReflection **ppReflection) {return DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)ppReflection);}
#endif

#include "../Shaders/!Header CPU.h"

#if SPIRV_CROSS
#include "../../../ThirdPartyLibs/begin.h"
#include "../../../ThirdPartyLibs/SPIRV-Cross/include/spirv_cross/spirv_cross_c.h"
#include "../../../ThirdPartyLibs/SPIRV-Cross/include/spirv_cross/spirv_glsl.hpp"
#include "../../../ThirdPartyLibs/end.h"
#endif

#if HLSL_CC
#pragma warning(push)
#pragma warning(disable:4267 4996)
#include "../../../ThirdPartyLibs/begin.h"
#include "../../../ThirdPartyLibs/HLSLcc/lib/include/hlslcc.h"
#include "../../../ThirdPartyLibs/end.h"
#pragma warning(pop)
#endif
/******************************************************************************/
namespace EE{
#include "Shader Compiler.h"
/******************************************************************************/
#define CC4_SHDR CC4('S','H','D','R')
/******************************************************************************/
static const CChar8* APIName[]=
{
   "DX"    , // 0
   "GL"    , // 1
   "VULKAN", // 2
   "METAL" , // 3
}; ASSERT(API_DX==0 && API_GL==1 && API_VULKAN==2 && API_METAL==3 && API_NUM==4);
/******************************************************************************/
#if   0 // Test #1: shader size 6.61 MB, engine load+render 1 frame = 0.40s on Windows, decompression 10x faster
   #define COMPRESS_GL       COMPRESS_LZ4
   #define COMPRESS_GL_LEVEL 11
#elif 0 // Test #2: Shaders compiled in: 50.527s, load+draw 1 frame: 0.86s, Main size 346 KB
   #define COMPRESS_GL       COMPRESS_ZSTD
   #define COMPRESS_GL_LEVEL 99
#elif 1 // Test #1: shader size 5.15 MB, engine load+render 1 frame = 0.45s on Windows, decompression 10x slower, Test #2: Shaders compiled in: 50.791s, load+draw 1 frame: 0.88s, Main size 337 KB
   #define COMPRESS_GL       COMPRESS_LZMA // FIXME still needed?
   #define COMPRESS_GL_LEVEL 9 // shader files are small, so we can use high compression level and still get small dictionary size / memory usage
#else // shader size was slightly bigger than LZMA, and loading all shader techs was bit slower
   #define COMPRESS_GL       COMPRESS_LZHAM
   #define COMPRESS_GL_LEVEL 5
#endif
   #define COMPRESS_GL_MT    true
/******************************************************************************/
static Bool HasData(CPtr data, Int size)
{
   if(C Byte *b=(Byte*)data)REP(size)if(*b++)return true;
   return false;
}
static void SaveTranslation(C Mems<ShaderParam::Translation> &translation, File &f, Int elms)
{
   if(elms<=1)translation.saveRaw(f);else
   {
      UShort single_translations=translation.elms()/elms,
         gpu_offset=translation[single_translations].gpu_offset-translation[0].gpu_offset,
         cpu_offset=translation[single_translations].cpu_offset-translation[0].cpu_offset;
      f.putMulti(gpu_offset, cpu_offset, single_translations);
      FREP(single_translations)f<<translation[i]; // save 1st element translation
   }
}
static void LoadTranslation(MemPtr<ShaderParam::Translation> translation, File &f, Int elms)
{
   if(elms<=1)translation.loadRaw(f);else
   {
      translation.clear();
      UShort single_translations, gpu_offset, cpu_offset; f.getMulti(gpu_offset, cpu_offset, single_translations);
      FREP(  single_translations)f>>translation.New(); // load 1st element translation
      for(Int e=1, co=0, go=0; e<elms; e++) // add rest of the elements
      {
         co+=cpu_offset; // element offset
         go+=gpu_offset; // element offset
         FREP(single_translations)
         {
            ShaderParam::Translation &t=translation.New(); // create and store reference !! memory address changes, do not perform adding new element and referencing previous element in one line of code !!
            t=translation[i];
            t.cpu_offset+=co;
            t.gpu_offset+=go;
         }
      }
   }
}
static void LimitTranslation(ShaderParam &sp)
{
   Memt<ShaderParam::Translation> translation;
   FREPA(sp._full_translation) // go from the start
   {
      ShaderParam::Translation &t=sp._full_translation[i];
      Int size=t.elm_size; // copy to temp var in case original is unsigned
      Int end=Min(t.cpu_offset+size, sp._cpu_data_size); MIN(size, end-t.cpu_offset);
          end=Min(t.gpu_offset+size, sp._gpu_data_size); MIN(size, end-t.gpu_offset);
      if(size>0){t.elm_size=size; translation.add(t);}
   }
   sp._full_translation=translation;
}
/******************************************************************************/
// INCLUDE
/******************************************************************************/
#if WINDOWS
/******************************************************************************/
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
         DYNAMIC_ASSERT(LoadEncoding(f)==ANSI, "File expected to be in ANSI encoding for performance reasons");
         Int   size=f.left();
         Byte *data=Alloc<Byte>(SIZEU(ShaderPath)+size);
         Set(((ShaderPath*)data)->path, path);
         data+=SIZE(ShaderPath);
         f.get(data, size);
        *ppData=data;
        *pBytes=size;
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
#if NEW_COMPILER
struct Include12 : IDxcIncludeHandler
{
   virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource)override
   {
      if(ppIncludeSource)
      {
         File f; if(f.readTry(pFilename))
         {
            UInt code_page;
            switch(LoadEncoding(f))
            {
               case ANSI  : code_page=CP_ACP  ; break;
               case UTF_8 : code_page=CP_UTF8 ; break;
               case UTF_16: code_page=CP_UTF16; break;
               default    : Exit("Invalid encoding"); break;
            }
            Mems<Byte> file_data(f.left()); if(f.getFast(file_data.data(), file_data.elms()))
            {
               IDxcLibrary *lib=null; CreateLibrary(&lib); if(lib)
               {
                  IDxcBlobEncoding *blob=null;
                  lib->CreateBlobWithEncodingOnHeapCopy(file_data.data(), file_data.elms(), code_page, &blob);
                  lib->Release();
                 *ppIncludeSource=blob;
                  return S_OK;
               }
            }
         }
        *ppIncludeSource=null;
      }
      return E_FAIL;
   }

   Include12(C Str &src) {m_dwRef++;}

   DXC_MICROCOM_ADDREF_RELEASE_IMPL(m_dwRef);
private:
   DXC_MICROCOM_REF_FIELD(m_dwRef);
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) {return DoBasicQueryInterface<::IDxcIncludeHandler>(this, riid, ppvObject);}
};
#endif
#endif
/******************************************************************************/
// COMPILER
/******************************************************************************/
#if WINDOWS
// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
/* D3DCompile introduces some weird packing rules for min16float:
      min16float a; // offset=0
      min16float b; // offset=4

      min16float a; // offset=0
           float b; // offset=16

   Looks like 'min16float' and 'float' can't be together on the same Vec4, so when a change is detected (using 'was_min16'), switch to new Vec4

   Also:
      min16float a; // offset=0
      min16float b; // offset=4

      min16float2 a; // offset=0
      min16float  b; // offset=8

      min16float3 a; // offset=0
      min16float  b; // offset=12

      min16float4 a; // offset=0
      min16float  b; // offset=16
*/
void ShaderCompiler::Param::addTranslation(ID3D11ShaderReflectionType *type, C D3D11_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16) // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
{
   if(type_desc.Elements)offset=Ceil16(offset); // arrays are 16-byte aligned (even 1-element arrays "f[1]"), non-arrays have Elements=0, so check Elements!=0
   Int  elms=Max(type_desc.Elements, 1), last_index=elms-1; // 'Elements' is array size (it's 0 for non-arrays)
   FREP(elms)switch(type_desc.Class)
   {
      case D3D_SVC_SCALAR        : // for example: Flt f,f[];
      case D3D_SVC_VECTOR        : // for example: Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
      case D3D_SVC_MATRIX_COLUMNS: // for example: Matrix m,m[];
      {
      #define ALLOW_MIN16 0
         if(type_desc.Rows   <=0 || type_desc.Rows   >4)Exit("Invalid Shader Param Rows");
         if(type_desc.Columns<=0 || type_desc.Columns>4)Exit("Invalid Shader Param Cols");
         Bool half=false, min16=(type_desc.Type==D3D_SVT_MIN16FLOAT);
         if(type_desc.Type==D3D_SVT_FLOAT || (ALLOW_MIN16 && min16))
         {
            Int base_size=(half ? SIZE(Half) : SIZE(Flt)),
                 cpu_size=base_size*type_desc.Columns*type_desc.Rows,
                 gpu_size;
            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)gpu_size=cpu_size;
            else                                       gpu_size=base_size*4             *(type_desc.Columns-1) // for matrixes, the lines use  4 X's
                                                               +base_size*type_desc.Rows;                      //       except last line  uses what was specified
            if(offset/16 != (offset+gpu_size-1)/16 || (ALLOW_MIN16 && was_min16>=0 && was_min16!=(Byte)min16))offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."
            if(!half)cpu_data_size=Ceil4(cpu_data_size); // float's are 4-byte aligned on CPU, double too if using #pragma pack(4)

            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)translation.New().set(cpu_data_size, offset, cpu_size);else
            { // !! process y's first to add translation sorted by 'cpu_offset' to make later sorting faster !!
               FREPD(y, type_desc.Rows   )
               FREPD(x, type_desc.Columns)translation.New().set(cpu_data_size+base_size*(x+y*type_desc.Columns), offset+base_size*(y+x*4), base_size);
            }
                  
            cpu_data_size+=cpu_size;
                   offset+=((i==last_index) ? gpu_size : Ceil16(gpu_size)); // arrays are 16-byte aligned, and last element is 'gpu_size' only
            was_min16=min16;
         }else Exit(S+"Unhandled Shader Parameter Type for \""+name+'"');
      }break;

      case D3D_SVC_STRUCT:
      {
         offset=Ceil16(offset); // "Each structure forces the next variable to start on the next four-component vector."
         FREP(type_desc.Members) // iterate all struct members
         {
            ID3D11ShaderReflectionType *member=type->GetMemberTypeByIndex(i); if(!member)Exit("'GetMemberTypeByIndex' failed");
            D3D11_SHADER_TYPE_DESC member_desc; if(!OK(member->GetDesc(&member_desc)))Exit("'ID3D11ShaderReflectionType.GetDesc' failed");
            addTranslation(member, member_desc, type->GetMemberTypeName(i), offset, was_min16);
         }
       //offset=Ceil16(offset); "Each structure forces the next variable to start on the next four-component vector." even though documentation examples indicate this should align too, actual tests confirm that's not the case
      }break;
   }
}
void ShaderCompiler::Param::addTranslation(ID3D12ShaderReflectionType *type, C D3D12_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16) // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
{
   if(type_desc.Elements)offset=Ceil16(offset); // arrays are 16-byte aligned (even 1-element arrays "f[1]"), non-arrays have Elements=0, so check Elements!=0
   Int  elms=Max(type_desc.Elements, 1), last_index=elms-1; // 'Elements' is array size (it's 0 for non-arrays)
   FREP(elms)switch(type_desc.Class)
   {
      case D3D_SVC_SCALAR        : // for example: Flt f,f[];
      case D3D_SVC_VECTOR        : // for example: Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
      case D3D_SVC_MATRIX_COLUMNS: // for example: Matrix m,m[];
      {
      #define ALLOW_MIN16 0
         if(type_desc.Rows   <=0 || type_desc.Rows   >4)Exit("Invalid Shader Param Rows");
         if(type_desc.Columns<=0 || type_desc.Columns>4)Exit("Invalid Shader Param Cols");
         Bool half=false, min16=(type_desc.Type==D3D_SVT_MIN16FLOAT);
         if(type_desc.Type==D3D_SVT_FLOAT || (ALLOW_MIN16 && min16))
         {
            Int base_size=(half ? SIZE(Half) : SIZE(Flt)),
                 cpu_size=base_size*type_desc.Columns*type_desc.Rows,
                 gpu_size;
            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)gpu_size=cpu_size;
            else                                       gpu_size=base_size*4             *(type_desc.Columns-1) // for matrixes, the lines use  4 X's
                                                               +base_size*type_desc.Rows;                      //       except last line  uses what was specified
            if(offset/16 != (offset+gpu_size-1)/16 || (ALLOW_MIN16 && was_min16>=0 && was_min16!=(Byte)min16))offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."
            if(!half)cpu_data_size=Ceil4(cpu_data_size); // float's are 4-byte aligned on CPU, double too if using #pragma pack(4)

            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)translation.New().set(cpu_data_size, offset, cpu_size);else
            { // !! process y's first to add translation sorted by 'cpu_offset' to make later sorting faster !!
               FREPD(y, type_desc.Rows   )
               FREPD(x, type_desc.Columns)translation.New().set(cpu_data_size+base_size*(x+y*type_desc.Columns), offset+base_size*(y+x*4), base_size);
            }
                  
            cpu_data_size+=cpu_size;
                   offset+=((i==last_index) ? gpu_size : Ceil16(gpu_size)); // arrays are 16-byte aligned, and last element is 'gpu_size' only
            was_min16=min16;
         }else Exit(S+"Unhandled Shader Parameter Type for \""+name+'"');
      }break;

      case D3D_SVC_STRUCT:
      {
         offset=Ceil16(offset); // "Each structure forces the next variable to start on the next four-component vector."
         FREP(type_desc.Members) // iterate all struct members
         {
            ID3D12ShaderReflectionType *member=type->GetMemberTypeByIndex(i); if(!member)Exit("'GetMemberTypeByIndex' failed");
            D3D12_SHADER_TYPE_DESC member_desc; if(!OK(member->GetDesc(&member_desc)))Exit("'ID3D12ShaderReflectionType.GetDesc' failed");
            addTranslation(member, member_desc, type->GetMemberTypeName(i), offset, was_min16);
         }
       //offset=Ceil16(offset); "Each structure forces the next variable to start on the next four-component vector." even though documentation examples indicate this should align too, actual tests confirm that's not the case
      }break;
   }
}
#if SPIRV_CROSS
void ShaderCompiler::Param::addTranslation(spvc_compiler compiler, spvc_type var, Int offset)
{
   auto array_dimensions=spvc_type_get_num_array_dimensions(var); if(array_dimensions>1)Exit("Multi-dimensional arrays are not supported");
   auto array_elms      =(array_dimensions ? spvc_type_get_array_dimension(var, 0) : 0); // use 0 for non-arrays to match DX behavior
   Bool is_struct       =(spvc_type_get_basetype(var)==SPVC_BASETYPE_STRUCT);
   Int  elms=Max(array_elms, 1), last_index=elms-1; // 'array_elms' is 0 for non-arrays
   FREP(elms)
   {
      if(!is_struct)
      {
         //FIXME offset+=;
      }else
      {
         auto members=spvc_type_get_num_member_types(var);
         FREP(members)
         {
            auto member=spvc_type_get_member_type(var, i);
            auto member_handle=spvc_compiler_get_type_handle(compiler, member);
            unsigned member_offset=0; spvc_compiler_type_struct_member_offset(compiler, var, i, &member_offset);
            addTranslation(compiler, member_handle, offset+member_offset);
         }
         //FIXME offset+=;
      }
   }
}
#endif
#endif
static Int Compare(C ShaderParam::Translation &a, C ShaderParam::Translation &b) {return Compare(a.cpu_offset, b.cpu_offset);}
void ShaderCompiler::Param::sortTranslation() {translation.sort(Compare);} // this is useful for Translation optimization
Bool ShaderCompiler::Param::operator==(C Param &p)C
{
   if(translation.elms()!=p.translation.elms())return false; REPA(translation)if(translation[i]!=p.translation[i])return false;
   if(data.elms()!=p.data.elms() || !EqualMem(data.data(), p.data.data(), data.elms()))return false;
   return Equal(name, p.name, true) && array_elms==p.array_elms && cpu_data_size==p.cpu_data_size && gpu_data_size==p.gpu_data_size;
}
Bool ShaderCompiler::Buffer::operator==(C Buffer &b)C
{
   if(params.elms()!=b.params.elms())return false; REPA(params)if(params[i]!=b.params[i])return false;
   if(bind_explicit && bind_slot!=b.bind_slot)return false; // check 'bind_slot' only if they are explicit
   return Equal(name, b.name, true) && size==b.size && bind_explicit==b.bind_explicit;
}
static Int Compare(C ShaderCompiler::Bind &a, C ShaderCompiler::Bind &b)
{
   if(Int c=Compare(a.name     , b.name, true))return c;
   return   Compare(a.bind_slot, b.bind_slot );
}
static Int CompareBind(C ShaderCompiler::Buffer &a, C ShaderCompiler::Buffer &b) {return Compare(SCAST(C ShaderCompiler::Bind, a), SCAST(C ShaderCompiler::Bind, b));}
static Int CompareBind(C ShaderCompiler::Image  &a, C ShaderCompiler::Image  &b) {return Compare(SCAST(C ShaderCompiler::Bind, a), SCAST(C ShaderCompiler::Bind, b));}
/******************************************************************************/
void ShaderCompiler::Shader::finalizeName()
{
   FREPA(params)
   {
    C TextParam8 &p=params[i]; if(p.value.length()!=1 && i!=params.elms()-1)Exit("Shader Param Value Length != 1"); // allow last parameter to have length!=1 because it won't introduce conflicts
      name+=p.value;
   }
}
/******************************************************************************/
ShaderCompiler::Shader& ShaderCompiler::Source::New(C Str &name, C Str8 &vs_func_name, C Str8 &ps_func_name)
{
   Shader &shader=shaders.New();
   shader.model=model;
   shader.name =name;
   shader.sub[ST_VS].func_name=vs_func_name;
   shader.sub[ST_PS].func_name=ps_func_name;
   return shader;
}
ShaderCompiler::Source::~Source()
{
#if WINDOWS && NEW_COMPILER
   RELEASE(file_blob);
#endif
}
Bool ShaderCompiler::Source::load()
{
   File f; if(!f.readTry(file_name))return false;
   ENCODING encoding=LoadEncoding(f);
#if WINDOWS && NEW_COMPILER
   IDxcLibrary *lib=null; CreateLibrary(&lib); if(lib)
   {
      UInt code_page;
      switch(encoding)
      {
         case ANSI  : code_page=CP_ACP  ; break;
         case UTF_8 : code_page=CP_UTF8 ; break;
         case UTF_16: code_page=CP_UTF16; break;
         default    : Exit("Invalid encoding"); break;
      }
      file_data.setNum(f.left()); if(!f.getFast(file_data.data(), file_data.elms()))return false;
      lib->CreateBlobWithEncodingFromPinned(file_data.data(), file_data.elms(), code_page, &file_blob);
      lib->Release();
   }
   if(!file_blob)return false;
#else
   DYNAMIC_ASSERT(encoding==ANSI, "File expected to be in ANSI encoding for performance reasons");
   file_data.setNum(f.left()); if(!f.getFast(file_data.data(), file_data.elms()))return false;
#endif
   return true;
}
/******************************************************************************/
static Bool Match(C ShaderCompiler::SubShader &output, C ShaderCompiler::SubShader &input, Str &error)
{
   Bool ok=true;
   REPA(input.inputs) // have to check only inputs, we can ignore outputs not present in inputs
   {
    C ShaderCompiler::IO &in=input.inputs[i];
      if(!InRange(i, output.outputs) || in!=output.outputs[i])
         if(in.name!="SV_SampleIndex")
      {
         error.line()+=S+"Input "+in.name+in.index+" register:"+in.reg+" in \""+input.func_name+"\" doesn't match output in \""+output.func_name+'"';
         ok=false;
      }
   }
   return ok;
}
void ShaderCompiler::SubShader::compile()
{
#if WINDOWS
 C Source         *source  =shader->source;
 C ShaderCompiler *compiler=source->compiler;
   Char8 target[6+1];
   switch(type)
   {
      case ST_VS: target[0]='v'; break;
      case ST_HS: target[0]='h'; break;
      case ST_DS: target[0]='d'; break;
      case ST_PS: target[0]='p'; break;
   }
   target[1]='s';
   target[2]='_';
   target[4]='_';
   target[6]='\0';
   SHADER_MODEL model=shader->model; if(type==ST_HS || type==ST_DS)MAX(model, SM_5); // HS DS are supported only in SM5+
#if NEW_COMPILER
   MAX(model, SM_6);
#endif
   switch(model)
   {
      case SM_4  : target[3]='4'; target[5]='0'; break;
      case SM_4_1: target[3]='4'; target[5]='1'; break;
      case SM_5  : target[3]='5'; target[5]='0'; break;
      case SM_6  : target[3]='6'; target[5]='0'; break;
      default    : Exit("Invalid Shader Model"); break;
   }

#if NEW_COMPILER
   Int params=shader->params.elms();
   MemtN<DxcDefine, 64  > defines; defines.setNum(params  +API_NUM);
   MemtN<Str      , 64*2> temp   ; temp   .setNum(params*2+API_NUM); Int temps=0;
   FREP(params)
   {
      DxcDefine &define=defines[i]; C TextParam8 &param=shader->params[i];
      define.Name =(temp[temps++]=param.name );
      define.Value=(temp[temps++]=param.value);
   }
   FREP(API_NUM)
   {
      defines[params+i].Name =(temp[temps++]=APIName[i]);
      defines[params+i].Value=((compiler->api==i) ? L"1" : L"0");
   }

   MemtN<LPCWSTR, 16> arguments;
   arguments.add(L"-flegacy-macro-expansion"); // without this have to use "#define CONCAT(a,b) a##b"
   arguments.add(L"-flegacy-resource-reservation"); // will prevent other cbuffers from reusing indexes from explicit buffers
   arguments.add(L"/Zpc"); // D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR
   arguments.add(L"/O3");
   arguments.add(L"-HV");
   arguments.add(L"2016");
   if(compiler->api!=API_DX)arguments.add(L"-spirv");
   //if(Flags1 & D3DCOMPILE_IEEE_STRICTNESS    )arguments.add(L"/Gis");
   //if(Flags1 & D3DCOMPILE_RESOURCES_MAY_ALIAS)arguments.add(L"/res_may_alias");

   Bool ok=false;
   IDxcBlob *buffer=null; IDxcBlobEncoding *error_blob=null;
   IDxcCompiler *dxc_compiler=null; CreateCompiler(&dxc_compiler); if(dxc_compiler)
   {
      IDxcOperationResult *op_result=null;
      dxc_compiler->Compile(source->file_blob, source->file_name, (Str)func_name, (Str)target, arguments.data(), arguments.elms(), defines.data(), defines.elms(), &Include12(source->file_name), &op_result);
      if(op_result)
      {
         op_result->GetErrorBuffer(&error_blob);
         HRESULT hr; if(OK(op_result->GetStatus(&hr)))if(OK(hr))ok=OK(op_result->GetResult(&buffer));
         op_result->Release();
      }
      dxc_compiler->Release();
   }
#else
   Int params=shader->params.elms();
   MemtN<D3D_SHADER_MACRO, 64> macros; macros.setNum(params+API_NUM+1);
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
   D3DCompile(source->file_data.data(), source->file_data.elms(), (Str8)source->file_name, macros.data(), &Include11(source->file_name), func_name, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &buffer, &error_blob);
#endif
   if(error_blob)
   {
      CChar8 *e=(Char8*)error_blob->GetBufferPointer();
      Int     l=        error_blob->GetBufferSize   ();
      error.reserveAdd(l); REP(l)error+=*e++;
      error_blob->Release();
   }
   if(buffer)
   {
   #if NEW_COMPILER
      if(compiler->api!=API_DX)
      {
         shader_data.setNum(buffer->GetBufferSize()).copyFrom((Byte*)buffer->GetBufferPointer());
         result=GOOD;
      }else
      {
      IDxcContainerReflection *container_reflection=null; CreateContainerReflection(&container_reflection); if(container_reflection)
      {
         UINT32 part_index;
         if(OK(container_reflection->Load(buffer)))
         if(OK(container_reflection->FindFirstPartKind(hlsl::DFCC_DXIL, &part_index)))
         {
            ID3D12ShaderReflection *reflection=null; container_reflection->GetPartReflection(part_index, __uuidof(ID3D12ShaderReflection), (Ptr*)&reflection);
            if(reflection)
            {
               D3D12_SHADER_DESC desc; if(!OK(reflection->GetDesc(&desc))){error.line()+="'ID3D12ShaderReflection.GetDesc' failed.";}else
               {
                  Int images_elms=0, buffers_elms=0;
                  FREP(desc.BoundResources)
                  {
                     D3D12_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error;}
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
                     D3D12_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error;}
                     switch(desc.Type)
                     {
                        case D3D_SIT_TEXTURE:
                        {
                           if(!InRange(desc.BindPoint, MAX_SHADER_IMAGES)){error.line()+=S+"Image index: "+desc.BindPoint+", is too big"; goto error;}
                           Image &image=images[images_elms++]; image.name=desc.Name; image.bind_slot=desc.BindPoint;
                        }break;

                        case D3D_SIT_CBUFFER:
                        {
                           if(!InRange(desc.BindPoint, MAX_SHADER_BUFFERS)){error.line()+=S+"Constant Buffer index: "+desc.BindPoint+", is too big"; goto error;}
                           Buffer &buffer=buffers[buffers_elms++];
                           buffer.name=desc.Name;
                           buffer.bind_slot=desc.BindPoint;
                           buffer.bind_explicit=FlagTest(desc.uFlags, D3D_SIF_USERPACKED); // FIXME this is not set https://github.com/microsoft/DirectXShaderCompiler/issues/2356
                           ID3D12ShaderReflectionConstantBuffer *cb=reflection->GetConstantBufferByName(desc.Name); if(!cb){error.line()+="'GetConstantBufferByIndex' failed."; goto error;}
                           {
                              D3D12_SHADER_BUFFER_DESC desc; if(!OK(cb->GetDesc(&desc))){error.line()+="'ID3D12ShaderReflectionConstantBuffer.GetDesc' failed."; goto error;}
                              buffer.size=desc.Size;
                              buffer.params.setNum(desc.Variables);
                              FREP(desc.Variables)
                              {
                                 ID3D12ShaderReflectionVariable *var=cb->GetVariableByIndex(i); if(!var){error.line()+="'GetVariableByIndex' failed."; goto error;}
                                 ID3D12ShaderReflectionType *type=var->GetType(); if(!type){error.line()+="'GetType' failed."; goto error;}
                                 D3D12_SHADER_VARIABLE_DESC var_desc; if(!OK( var->GetDesc(& var_desc))){error.line()+="'ID3D12ShaderReflectionVariable.GetDesc' failed."; goto error;}
                                 D3D12_SHADER_TYPE_DESC    type_desc; if(!OK(type->GetDesc(&type_desc))){error.line()+="'ID3D12ShaderReflectionType.GetDesc' failed."; goto error;}

                                 Param &param=buffer.params[i];
                                 param.name=var_desc.Name;
                                 param.array_elms=type_desc.Elements;
                                 Int offset=var_desc.StartOffset; SByte was_min16=-1; param.addTranslation(type, type_desc, var_desc.Name, offset, was_min16); param.sortTranslation();
                                 param.gpu_data_size=offset-var_desc.StartOffset;
                                 if(!param.translation.elms()                             )Exit("Shader Param is empty.\nPlease contact Developer.");
                                 if( param.gpu_data_size!=var_desc.Size                   )Exit("Incorrect Shader Param size.\nPlease contact Developer.");
                                 if( param.translation[0].gpu_offset!=var_desc.StartOffset)Exit("Incorrect Shader Param Offset.\nPlease contact Developer.");
                                 if( param.gpu_data_size+var_desc.StartOffset>buffer.size )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
                               //if( SIZE(Vec4)         +var_desc.StartOffset>buffer.size )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4

                                 if(HasData(var_desc.DefaultValue, var_desc.Size)) // if parameter has any data
                                    param.data.setNum(param.gpu_data_size).copyFrom((Byte*)var_desc.DefaultValue);
                           
                               //type->Release(); this doesn't have 'Release'
                               //var ->Release(); this doesn't have 'Release'
                              }
                              DEBUG_ASSERT(buffer.bind_explicit==FlagTest(desc.uFlags, D3D_CBF_USERPACKED), "bind_explicit mismatch"); 
                           }
                        //cb->Release(); this doesn't have 'Release'
                        }break;
                     }
                  }
                  if(!shader->dummy)
                  {
                     // sort by bind members, to increase chance of generating the same bind map for multiple shaders
                     buffers.sort(CompareBind);
                      images.sort(CompareBind);

                      inputs.setNum(desc. InputParameters); FREPA( inputs){D3D12_SIGNATURE_PARAMETER_DESC desc; if(!OK(reflection->GetInputParameterDesc (i, &desc)))Exit("'GetInputParameterDesc' failed" );  inputs[i]=desc;}
                     outputs.setNum(desc.OutputParameters); FREPA(outputs){D3D12_SIGNATURE_PARAMETER_DESC desc; if(!OK(reflection->GetOutputParameterDesc(i, &desc)))Exit("'GetOutputParameterDesc' failed"); outputs[i]=desc;}
            
                  #if DEBUG && 0 // use this for generation of a basic Vertex Shader which can be used for Input Layout creation (see 'DX10_INPUT_LAYOUT' and 'VS_Code')
                     if(type==VS)
                     {
                        Byte *data=(Byte*)buffer->GetBufferPointer(); Int size=buffer->GetBufferSize();
                        Str t=S+"static Byte VS_Code["+size+"]={";
                        FREP(size){if(i)t+=','; t+=data[i];}
                        t+="};\n";
                        ClipSet(t);
                        Exit(t);
                     }
                  #endif

                     if(compiler->api==API_DX) // strip
                     {
                        // FIXME
                        //ID3DBlob *stripped=null; D3DStripShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), ~0, &stripped);
                        //if(stripped){buffer->Release(); buffer=stripped;}
                     }
                     shader_data.setNum(buffer->GetBufferSize()).copyFrom((Byte*)buffer->GetBufferPointer());
                  }

                  result=GOOD;
                  // !! do not make any changes here after setting 'result' because other threads may access this data !!
                  goto ok;
               }
            error:
               result=FAIL;
            ok:
               reflection->Release();
            }
         }
         container_reflection->Release();
      }
      }
   #else
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
                     if(!InRange(desc.BindPoint, MAX_SHADER_IMAGES)){error.line()+=S+"Image index: "+desc.BindPoint+", is too big"; goto error;}
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
                           param.array_elms=type_desc.Elements;
                           Int offset=var_desc.StartOffset; SByte was_min16=-1; param.addTranslation(type, type_desc, var_desc.Name, offset, was_min16); param.sortTranslation();
                           param.gpu_data_size=offset-var_desc.StartOffset;
                           if(!param.translation.elms()                             )Exit("Shader Param is empty.\nPlease contact Developer.");
                           if( param.gpu_data_size!=var_desc.Size                   )Exit("Incorrect Shader Param size.\nPlease contact Developer.");
                           if( param.translation[0].gpu_offset!=var_desc.StartOffset)Exit("Incorrect Shader Param Offset.\nPlease contact Developer.");
                           if( param.gpu_data_size+var_desc.StartOffset>buffer.size )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
                         //if( SIZE(Vec4)         +var_desc.StartOffset>buffer.size )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4

                           if(HasData(var_desc.DefaultValue, var_desc.Size)) // if parameter has any data
                              param.data.setNum(param.gpu_data_size).copyFrom((Byte*)var_desc.DefaultValue);
                           
                         //type->Release(); this doesn't have 'Release'
                         //var ->Release(); this doesn't have 'Release'
                        }
                      //DEBUG_ASSERT(buffer.bind_explicit==FlagTest(desc.uFlags, D3D_CBF_USERPACKED), "bind_explicit mismatch"); ignore because looks like 'desc.uFlags' is not set
                     }
                  //cb->Release(); this doesn't have 'Release'
                  }break;
               }
            }
            if(!shader->dummy)
            {
               // sort by bind members, to increase chance of generating the same bind map for multiple shaders
               buffers.sort(CompareBind);
                images.sort(CompareBind);

                inputs.setNum(desc. InputParameters); FREPA( inputs){D3D11_SIGNATURE_PARAMETER_DESC desc; if(!OK(reflection->GetInputParameterDesc (i, &desc)))Exit("'GetInputParameterDesc' failed" );  inputs[i]=desc;}
               outputs.setNum(desc.OutputParameters); FREPA(outputs){D3D11_SIGNATURE_PARAMETER_DESC desc; if(!OK(reflection->GetOutputParameterDesc(i, &desc)))Exit("'GetOutputParameterDesc' failed"); outputs[i]=desc;}
            
            #if DEBUG && 0 // use this for generation of a basic Vertex Shader which can be used for Input Layout creation (see 'DX10_INPUT_LAYOUT' and 'VS_Code')
               if(type==VS)
               {
                  Byte *data=(Byte*)buffer->GetBufferPointer(); Int size=buffer->GetBufferSize();
                  Str t=S+"static Byte VS_Code["+size+"]={";
                  FREP(size){if(i)t+=','; t+=data[i];}
                  t+="};\n";
                  ClipSet(t);
                  Exit(t);
               }
            #endif

               if(compiler->api==API_DX) // strip
               {
                  ID3DBlob *stripped=null; D3DStripShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), ~0, &stripped);
                  if(stripped){buffer->Release(); buffer=stripped;}
               }
               shader_data.setNum(buffer->GetBufferSize()).copyFrom((Byte*)buffer->GetBufferPointer());
            }

            result=GOOD;
            // !! do not make any changes here after setting 'result' because other threads may access this data !!
            goto ok;
         }
      error:
         result=FAIL;
      ok:
         reflection->Release();
      }
   #endif
      buffer->Release();
   }
#else
   result=FAIL;
#endif
   if(result==GOOD)
   {
      // verify that stages have matching output->input
      for(Int i=type;         --i>=0      ; ){C SubShader &prev=shader->sub[i]; if(prev.is()){if(prev.result==GOOD && !Match(prev, T, error))result=FAIL; break;}} // can check only if other shader also completed successfully, stop on first valid sub shader
      for(Int i=type; InRange(++i, ST_NUM); ){C SubShader &next=shader->sub[i]; if(next.is()){if(next.result==GOOD && !Match(T, next, error))result=FAIL; break;}} // can check only if other shader also completed successfully, stop on first valid sub shader
   }
   if(result!=GOOD)Exit(S+"Compiling \""+shader->name+"\" in \""+source->file_name+"\" failed:\n"+error);
}
/******************************************************************************/
ShaderCompiler& ShaderCompiler::set(C Str &dest, SHADER_MODEL model, API api)
{
   T.dest =dest ;
   T.model=model;
   T.api  =api  ;
   return T;
}
ShaderCompiler::Source& ShaderCompiler::New(C Str &file_name)
{
   Source &source=sources.New();
   source.file_name=file_name;
   source.model    =model;
   source.compiler =this;
   return source;
}
/******************************************************************************/
#pragma pack(push, 1)
struct ConstantIndex
{
   Byte  bind_index;
   UShort src_index;

        void set(Int bind_index, Int src_index) {_Unaligned(T.bind_index, bind_index); _Unaligned(T.src_index, src_index); DYNAMIC_ASSERT(T.bind_index==bind_index && T.src_index==src_index, "Constant index out of range");}
   ConstantIndex(Int bind_index, Int src_index) {set(bind_index, src_index);}
   ConstantIndex() {}
};
#pragma pack(pop)

static UShort AsUShort(Int i) {DYNAMIC_ASSERT(InRange(i, USHORT_MAX+1), "Value too big to be represented as UShort"); return i;}

Bool ShaderCompiler::Param::save(File &f)C
{
   f.putStr(name).putMulti(cpu_data_size, gpu_data_size, array_elms); // name+info
   SaveTranslation(translation, f, array_elms);                       // translation
   if(data.elms())                                                    // data
   {
      f.putBool(true);
      data.saveRawData(f);
   }else
   {
      f.putBool(false);
   }
   return f.ok();
}
#pragma pack(push, 1)
struct Indexes
{
   Int    shader_data_index[ST_NUM];
   UShort buffer_bind_index[ST_NUM], image_bind_index[ST_NUM];
};
#pragma pack(pop)
Bool ShaderCompiler::Shader::save(File &f, C ShaderCompiler &compiler)C
{
   // name
   f.putStr(name);

   if(compiler.api!=API_GL)
   {
      // indexes
      Indexes indexes;
      FREPA(sub)
      {
       C SubShader &sub=T.sub[i];
         indexes.shader_data_index[i]=         sub.shader_data_index ;
         indexes.buffer_bind_index[i]=AsUShort(sub.buffer_bind_index);
         indexes. image_bind_index[i]=AsUShort(sub. image_bind_index);
      }
      f<<indexes;

      // all buffers
      MemtN<UShort, 256> all_buffers;
      FREPA(sub)
      {
       C SubShader &sub=T.sub[i]; FREPA(sub.buffers)
         {
          C Buffer &buffer=sub.buffers[i];
            Int src_index=compiler.buffers.findValidIndex(buffer.name); if(src_index<0)Exit("Buffer not found in Shader");
            all_buffers.binaryInclude(AsUShort(src_index));
         }
      }
      all_buffers.saveRaw(f);
   }else
   {
      // indexes
      f.putMulti(sub[ST_VS].shader_data_index, sub[ST_PS].shader_data_index);
   }

   return f.ok();
}
/******************************************************************************/
static void Compile(ShaderCompiler::SubShader &shader, Ptr user, Int thread_index) {shader.compile();}
static Bool Create (ShaderCompiler::Buffer* &buffer, C Str8 &name, Ptr user) {buffer=null; return true;}
static Int  Compare(ShaderCompiler::Shader*C &a, ShaderCompiler::Shader*C &b) {return CompareCS(a->name, b->name);}
   
ShaderCompiler::ShaderCompiler() : buffers(CompareCS, Create) {}

/*struct Refl : HLSLccReflection
{
    // Called on errors or diagnostic messages
    virtual void OnDiagnostics(const std::string &error, int line, bool isError)override {}

    virtual void OnInputBinding(const std::string &name, int bindIndex)override {}

    // Returns false if this constant buffer is not needed for this shader. This info can be used for pruning unused
    // constant buffers and vars from compute shaders where we need broader context than a single kernel to know
    // if something can be dropped, as the constant buffers are shared between all kernels in a .compute file.
    virtual bool OnConstantBuffer(const std::string &name, size_t bufferSize, size_t memberCount)override { return true; }

    // Returns false if this constant var is not needed for this shader. See above.
    virtual bool OnConstant(const std::string &name, int bindIndex, SHADER_VARIABLE_TYPE cType, int rows, int cols, bool isMatrix, int arraySize, bool isUsed)override { return true; }

    virtual void OnConstantBufferBinding(const std::string &name, int bindIndex)override {}
    virtual void OnTextureBinding(const std::string &name, int bindIndex, int samplerIndex, bool multisampled, HLSLCC_TEX_DIMENSION dim, bool isUAV)override {}
    virtual void OnBufferBinding(const std::string &name, int bindIndex, bool isUAV)override {}
    virtual void OnThreadGroupSize(unsigned int xSize, unsigned int ySize, unsigned int zSize)override {}
    virtual void OnTessellationInfo(uint32_t tessPartitionMode, uint32_t tessOutputWindingOrder, uint32_t tessMaxFactor, uint32_t tessNumPatchesInThreadGroup)override {}
    virtual void OnTessellationKernelInfo(uint32_t patchKernelBufferCount)override {}
};*/
struct ConvertContext
{
   ShaderCompiler &compiler;
   SyncLock        lock;
#if HLSL_CC
   HLSLccSamplerPrecisionInfo sampler_precision;
   GlExtensions ext;
#endif
#if DEBUG
   Memc<                ShaderData> (&shader_datas)[ST_NUM];
   Mems<ShaderCompiler::Shader*   >  &shaders;

   ShaderCompiler::Shader& shader(C ShaderData &shader_data)C // get first 'Shader' using 'shader_data'
   {
      FREPAD(type, shader_datas)
      {
         Int shader_data_index=shader_datas[type].index(&shader_data); if(shader_data_index>=0)
         {
            FREPAD(si, shaders)
            {
               ShaderCompiler::Shader &shader=*shaders[si]; if(shader.sub[type].shader_data_index==shader_data_index)return shader;
            }
            break;
         }
      }
      Exit("Can't find shader");
      return *(ShaderCompiler::Shader*)null;
   }
#endif

   ConvertContext(ShaderCompiler &compiler
   #if DEBUG
    , Memc<                ShaderData> (&shader_datas)[ST_NUM]
    , Mems<ShaderCompiler::Shader*   >  &shaders
   #endif
   ) : compiler(compiler)
   #if DEBUG
     , shader_datas(shader_datas), shaders(shaders)
   #endif
   {
   #if HLSL_CC
      sampler_precision.insert(std::pair<std::string, REFLECT_RESOURCE_PRECISION>("Depth" , REFLECT_RESOURCE_PRECISION_HIGHP));
      sampler_precision.insert(std::pair<std::string, REFLECT_RESOURCE_PRECISION>("ImgXF" , REFLECT_RESOURCE_PRECISION_HIGHP));
      sampler_precision.insert(std::pair<std::string, REFLECT_RESOURCE_PRECISION>("ImgXF1", REFLECT_RESOURCE_PRECISION_HIGHP));

      Zero(ext);
      ext.ARB_explicit_uniform_location=true;
      ext.ARB_explicit_attrib_location=true;
      ext.ARB_shading_language_420pack=true;
   #endif
   }
};
static void ErrorCallback(void *userdata, const char *error)
{
   Exit(error);
}
static void Convert(ShaderData &shader_data, ConvertContext &cc, Int thread_index)
{
   Str8 code;
   ShaderCompiler &compiler=cc.compiler;

#if SPIRV_CROSS
   const SpvId *spirv=(SpvId*)shader_data.data();
   size_t word_count=shader_data.elms()/SIZE(SpvId);
   if(word_count*SIZE(SpvId)!=shader_data.elms())Exit("Incorrect Spir-V size");

   spvc_context context=null; spvc_context_create(&context); if(!context)Exit("Can't create context");
   spvc_context_set_error_callback(context, ErrorCallback, null);
   spvc_parsed_ir ir=null; spvc_context_parse_spirv(context, spirv, word_count, &ir); if(!ir)Exit("'spvc_context_parse_spirv' failed");
   spvc_compiler spirv_compiler=null; spvc_context_create_compiler(context, (compiler.api==API_METAL) ? SPVC_BACKEND_MSL : SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &spirv_compiler);
   if(!spirv_compiler)Exit("'spvc_context_create_compiler' failed");

   spvc_resources resources=null; spvc_compiler_create_shader_resources(spirv_compiler, &resources);
   const spvc_reflected_resource *list=null; size_t count=0; spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &list, &count);
   Memc<ShaderCompiler::Buffer> buffers; buffers.setNum((Int)count); FREPA(buffers)
   {
   // FIXME
   /*ShaderResources res = compiler->get_shader_resources();
    for (const Resource& ub_res : res.uniform_buffers) {
        const SPIRType& ub_type = compiler->get_type(ub_res.base_type_id);
        for (int m_index = 0; m_index < int(ub_type.member_types.size()); m_index++) {
            const SPIRType& m_type = compiler->get_type(ub_type.member_types[m_index]);
            if ((m_type.basetype == SPIRType::Float) && (m_type.vecsize > 1) && (m_type.columns > 1)) {
                compiler->set_member_decoration(ub_res.base_type_id, m_index, DecorationColMajor);
            }
        }
    }
    or alternative replace "row_major" with "column/col_major" ?*/

      ShaderCompiler::Buffer  &buffer=buffers[i];
    C spvc_reflected_resource &res   =list[i]; auto buffer_handle=spvc_compiler_get_type_handle(spirv_compiler, res.type_id);
    //CChar8 *base_type_name=spvc_compiler_get_name(spirv_compiler, res.base_type_id);
    //CChar8 *     type_name=spvc_compiler_get_name(spirv_compiler, res.type_id);

      buffer.name=spvc_compiler_get_name(spirv_compiler, res.id);
      buffer.bind_slot=0; // FIXME
      buffer.bind_explicit=false; // FIXME
      size_t size=0; spvc_compiler_get_declared_struct_size(spirv_compiler, buffer_handle, &size); buffer.size=(Int)size;

      //auto a=spvc_type_get_bit_width(spvc_compiler_get_type_handle(spirv_compiler, res.base_type_id));
      //auto b=spvc_type_get_bit_width(spvc_compiler_get_type_handle(spirv_compiler, res.     type_id));
      //size_t c=0; spvc_compiler_get_declared_struct_size(spirv_compiler, buffer_handle, &c);
      //size_t d=0; spvc_compiler_get_declared_struct_size(spirv_compiler, spvc_compiler_get_type_handle(spirv_compiler, res.base_type_id), &d);
      
      buffer.params.setNum(spvc_type_get_num_member_types(buffer_handle));
      FREPA(buffer.params)
      {
         ShaderCompiler::Param &param=buffer.params[i];
         param.name=spvc_compiler_get_member_name(spirv_compiler, res.base_type_id, i);
         auto member=spvc_type_get_member_type(buffer_handle, i);
         auto member_handle=spvc_compiler_get_type_handle(spirv_compiler, member);
         spvc_basetype member_type=spvc_type_get_basetype(member_handle);
         auto array_dimensions=spvc_type_get_num_array_dimensions(member_handle); if(array_dimensions>1)Exit("Multi-dimensional arrays are not supported");
         param.array_elms=(array_dimensions ? spvc_type_get_array_dimension(member_handle, 0) : 0); // use 0 for non-arrays to match DX behavior
         unsigned offset=0; spvc_compiler_type_struct_member_offset(spirv_compiler, buffer_handle, i, &offset); unsigned start=offset;
         param.addTranslation(spirv_compiler, member_handle, offset); param.sortTranslation();

         size_t member_size=0;
         if(member_type==SPVC_BASETYPE_STRUCT)
         {
            if(param.array_elms)spvc_compiler_get_declared_struct_size_runtime_array(spirv_compiler, member_handle, param.array_elms, &member_size);
            else                spvc_compiler_get_declared_struct_size              (spirv_compiler, member_handle,                   &member_size);
         }else
         {
            auto vec_size=spvc_type_get_vector_size(member_handle);
            auto cols=spvc_type_get_columns(member_handle);
            member_size=vec_size*cols*SIZE(Flt);
         }

         if(!param.translation.elms()                   )Exit("Shader Param is empty.\nPlease contact Developer.");
         ShaderParam::Translation &last_translation=param.translation.last();
         param.gpu_data_size=last_translation.gpu_offset+last_translation.elm_size;
         if( param.gpu_data_size!=member_size           )Exit("Incorrect Shader Param size.\nPlease contact Developer.");
         if( param.translation[0].gpu_offset!=start     )Exit("Incorrect Shader Param Offset.\nPlease contact Developer.");
         if( param.gpu_data_size+start>buffer.size      )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
       //if( SIZE(Vec4)+var_desc.StartOffset>buffer.size)Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4

         //FIXME if(HasData(var_desc.DefaultValue, var_desc.Size)) // if parameter has any data
         //   param.data.setNum(param.gpu_data_size).copyFrom((Byte*)var_desc.DefaultValue);
/*
unsigned spvc_compiler_get_member_decoration(spvc_compiler compiler, spvc_type_id id, unsigned member_index, SpvDecoration decoration);
const char *spvc_compiler_get_member_name(spvc_compiler compiler, spvc_type_id id, unsigned member_index);

spvc_basetype spvc_type_get_basetype(spvc_type type);
unsigned spvc_type_get_bit_width(spvc_type type);
unsigned spvc_type_get_vector_size(spvc_type type);
unsigned spvc_type_get_columns(spvc_type type);
unsigned spvc_type_get_num_array_dimensions(spvc_type type);
spvc_bool spvc_type_array_dimension_is_literal(spvc_type type, unsigned dimension);
SpvId spvc_type_get_array_dimension(spvc_type type, unsigned dimension);
unsigned spvc_type_get_num_member_types(spvc_type type);
spvc_type_id spvc_type_get_member_type(spvc_type type, unsigned index);
SpvStorageClass spvc_type_get_storage_class(spvc_type type);

spvc_result spvc_compiler_get_declared_struct_size              (spvc_compiler compiler, spvc_type struct_type, size_t *size);
spvc_result spvc_compiler_get_declared_struct_size_runtime_array(spvc_compiler compiler, spvc_type struct_type, size_t array_size, size_t *size);

spvc_result spvc_compiler_type_struct_member_offset(spvc_compiler compiler, spvc_type type, unsigned index, unsigned *offset);
spvc_result spvc_compiler_type_struct_member_array_stride(spvc_compiler compiler, spvc_type type, unsigned index, unsigned *stride);
spvc_result spvc_compiler_type_struct_member_matrix_stride(spvc_compiler compiler, spvc_type type, unsigned index, unsigned *stride);
      {
         Int loc  =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationLocation);
         Int comp =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationComponent);
         Int index=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationIndex);
         Int bind =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationBinding);
         Int offs =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationOffset);
         Int arr_s=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationArrayStride);
         Int mt_s =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationMatrixStride);
         Int uni  =spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationUniform);
      }
      {
         Int loc  =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationLocation);
         Int comp =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationComponent);
         Int index=spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationIndex);
         Int bind =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationBinding);
         Int offs =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationOffset);
         Int arr_s=spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationArrayStride);
         Int mt_s =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationMatrixStride);
         Int uni  =spvc_compiler_get_decoration(spirv_compiler, res.type_id, SpvDecorationUniform);*/
      }
   }

   spvc_compiler_options options=null;
   spvc_compiler_create_compiler_options(spirv_compiler, &options); if(!options)Exit("'spvc_compiler_create_compiler_options' failed");
   switch(compiler.api)
   {
      case API_GL:
      {
         // FIXME
         //spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 330);
         //spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
         spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 300);
         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_SUPPORT_NONZERO_BASE_INSTANCE, SPVC_FALSE);
         //spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, SPVC_FALSE);
         //SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_FLOAT_PRECISION_HIGHP
         //SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_INT_PRECISION_HIGHP
         
      }break;

      case API_METAL:
      {
      }break;
   }
   spvc_compiler_install_compiler_options(spirv_compiler, options);

   spvc_compiler_build_combined_image_samplers(spirv_compiler);
   const spvc_combined_image_sampler *samplers=null; size_t num_samplers=0;
   spvc_compiler_get_combined_image_samplers(spirv_compiler, &samplers, &num_samplers);
   FREP(num_samplers)
   {
    C spvc_combined_image_sampler &cis=samplers[i];
      CChar8 *  image_name=spvc_compiler_get_name(spirv_compiler, cis.  image_id);
    //CChar8 *sampler_name=spvc_compiler_get_name(spirv_compiler, cis.sampler_id);
      spvc_compiler_set_name(spirv_compiler, cis.combined_id, image_name);
      {SyncLocker lock(cc.lock); compiler.images.binaryInclude(Str8(image_name), CompareCS);}
   }

   const char *glsl=null; spvc_compiler_compile(spirv_compiler, &glsl);
   code=glsl;
   spvc_context_destroy(context); // Frees all memory we allocated so far

   code=Replace(code, "in_var_ATTR", "ATTR", true);

   //FIXME
   //ClipSet(code);
   //LogN("/******************************************************************************/");
   //LogN(S+"Shader:"+cc.shader(shader_data).name);
   //LogN(code);

#endif

#if HLSL_CC
   HLSLccReflection reflection;
   GLSLShader       converted;
   TranslateHLSLFromMem((char*)shader_data.data(), HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT, LANG_330, &cc.ext, null, cc.sampler_precision, reflection, &converted); // LANG_330, LANG_ES_300
   code=converted.sourceCode.c_str();
   code=Replace(code, "#version 330\n", S);
   code=Replace(code, "#version 300 es\n", S);
   code=Replace(code, "in_ATTR", "ATTR", true);
#endif

   if(!code.length()) // this is also needed for null char below
   {
   #if DEBUG
      ShaderCompiler::Shader &shader=cc.shader(shader_data);
   #endif
      Exit("Can't convert HLSL to GLSL");
   }
#if 0 // uncompressed
   shader_data.setNum(code.length()+1).copyFrom((Byte*)code()); // include null char
#else // compressed
   File f; f.readMem(code(), code.length()+1); // include null char
   File cmp; if(!Compress(f, cmp.writeMem(), COMPRESS_GL, COMPRESS_GL_LEVEL, false))Exit("Can't compress shader data");
   f.del(); cmp.pos(0); shader_data.setNum(cmp.size()).loadRawData(cmp);
#endif
}
/******************************************************************************/
static ShaderImage * Get(Int i, C MemtN<ShaderImage *, 256> &images ) {if(!InRange(i, images ))Exit("Invalid ShaderImage index" ); return  images[i];}
static ShaderBuffer* Get(Int i, C MemtN<ShaderBuffer*, 256> &buffers) {if(!InRange(i, buffers))Exit("Invalid ShaderBuffer index"); return buffers[i];}

#if DEBUG
static C Str8& Name(ShaderImage  &image ) {C Str8 *name=ShaderImages .dataToKey(&image ); if(!name)Exit("Can't find ShaderImage name" ); return *name;}
static C Str8& Name(ShaderBuffer &buffer) {C Str8 *name=ShaderBuffers.dataToKey(&buffer); if(!name)Exit("Can't find ShaderBuffer name"); return *name;}
#else
static C Str8& Name(ShaderImage  &image ) {return ShaderImages .dataInMapToKey(image );}
static C Str8& Name(ShaderBuffer &buffer) {return ShaderBuffers.dataInMapToKey(buffer);}
#endif

#if DEBUG && !GL
static void Test(BufferLink &b)
{
   switch(b.index)
   {
      case SBI_GLOBAL    : if(Name(*b.buffer)!="Global"   )error: Exit(S+"Invalid Shader Constant Index "+b.index+' '+Name(*b.buffer)); break;
      case SBI_OBJ_MATRIX: if(Name(*b.buffer)!="ObjMatrix")goto error; break;
      case SBI_OBJ_VEL   : if(Name(*b.buffer)!="ObjVel"   )goto error; break;
      case SBI_MESH      : if(Name(*b.buffer)!="Mesh"     )goto error; break;
      case SBI_MATERIAL  : if(Name(*b.buffer)!="Material" )goto error; break;
      case SBI_VIEWPORT  : if(Name(*b.buffer)!="Viewport" )goto error; break;
      case SBI_COLOR     : if(Name(*b.buffer)!="Color"    )goto error; break;
   }ASSERT(SBI_NUM==7);
}
#endif
/******************************************************************************/
// FIXME store buffers+images in one map?
struct BindMap : Mems<ShaderCompiler::Bind>
{
   void operator=(C Mems<ShaderCompiler::Image > & images) {setNum( images.elms()); FREPAO(T)= images[i];}
 //void operator=(C Mems<ShaderCompiler::Buffer> &buffers) {setNum(buffers.elms()); FREPAO(T)=buffers[i];}
   void operator=(C Mems<ShaderCompiler::Buffer> &buffers) // !! we shouldn't store buffers with explicit bind slots, because they're always bound at their creation, this will avoid overhead when drawing shaders !!
   {
                Int elms=0; FREPA(buffers)elms+=!buffers[i].bind_explicit; // skip explicit
      setNum(elms); elms=0; FREPA(buffers)   if(!buffers[i].bind_explicit)T[elms++]=buffers[i];
   }

   Bool operator==(C Mems<ShaderCompiler::Image > & images)C {if(elms()!= images.elms())return false; REPA(T)if(T[i]!= images[i])return false; return true;}
 //Bool operator==(C Mems<ShaderCompiler::Buffer> &buffers)C {if(elms()!=buffers.elms())return false; REPA(T)if(T[i]!=buffers[i])return false; return true;}
   Bool operator==(C Mems<ShaderCompiler::Buffer> &buffers)C // !! we shouldn't store buffers with explicit bind slots, because they're always bound at their creation, this will avoid overhead when drawing shaders !!
   {
      Int elms=0;
      FREPA(buffers)
      {
       C ShaderCompiler::Buffer &buffer=buffers[i]; if(!buffer.bind_explicit) // skip explicit
         {
            if(!InRange(elms, T) || T[elms]!=buffer)return false; elms++;
         }
      }
      return elms==T.elms();
   }

   // BUFFERS
   Bool save       (File &f, C ShaderCompiler &compiler)C {return saveBuffers(f, compiler);}
   Bool saveBuffers(File &f, C ShaderCompiler &compiler)C
   {
      MemtN<ConstantIndex, 256> save;
      FREPA(T)
      {
       C ShaderCompiler::Bind &bind=T[i];
         Int src_index=compiler.buffers.findValidIndex(bind.name); if(src_index<0)Exit("Buffer not found in Shader");
         save.New().set(bind.bind_slot, src_index); // save to which index this buffer should be bound for this shader, and index of buffer in 'file_buffers' array
      }
      return save.saveRaw(f);
   }

   // IMAGES
   Bool save      (File &f, C ShaderCompiler &compiler, C ShaderCompiler &)C {return saveImages(f, compiler);}
   Bool saveImages(File &f, C ShaderCompiler &compiler)C
   {
      f.cmpUIntV(elms());
      FREPA(T)
      {
       C ShaderCompiler::Bind &bind=T[i];
         Int src_index; if(!compiler.images.binarySearch(bind.name, src_index, CompareCS))Exit("Image not found in Shader");
         f<<ConstantIndex(bind.bind_slot, src_index);
      }
      return f.ok();
   }
};
#if !GL
Bool BufferLink::load(File &f, C MemtN<ShaderBuffer*, 256> &buffers)
{
   ConstantIndex ci; f>>ci; index=ci.bind_index; if(!InRange(index, MAX_SHADER_BUFFERS))Exit(S+"Buffer index: "+index+", is too big"); buffer=Get(ci.src_index, buffers); if(DEBUG)Test(T);
   return f.ok();
}
Bool ImageLink::load(File &f, C MemtN<ShaderImage*, 256> &images)
{
   ConstantIndex ci; f>>ci; index=ci.bind_index; if(!InRange(index, MAX_SHADER_IMAGES))Exit(S+"Image index: "+index+", is too big"); image=Get(ci.src_index, images);
   return f.ok();
}
#endif
/******************************************************************************/
Bool ShaderCompiler::compileTry(Threads &threads)
{
   Int shaders_num=0;
   FREPA(sources)
   {
      Source &source=sources[i];
      if(!source.load())return error(S+"Can't open file:"+source.file_name);
    //shaders_num+=source.shaders.elms(); need to check if dummy
      FREPA(source.shaders)
      {
         Shader &shader=source.shaders[i];
         shader.source=&source; // link only during compilation because sources use 'Memc' container which could change addresses while new sources were being added, however at this stage all have already been created
         shader.finalizeName();
         shaders_num+=!shader.dummy;
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
   }
   threads.wait1();
   Memc<BindMap   > buffer_maps, image_maps;
   Memc<ShaderData> shader_datas[ST_NUM];
   Mems<Shader*   > shaders(shaders_num); shaders_num=0;
   FREPA(sources)
   {
      Source &source=sources[i]; FREPA(source.shaders)
      {
         Shader &shader=source.shaders[i]; FREPA(shader.sub)
         {
            SubShader &sub=shader.sub[i]; if(sub.result==FAIL)return error(S+"Compiling \""+shader.name+"\" in \""+source.file_name+"\" failed:\n"+sub.error);
            FREPA(sub.buffers)
            {
               Buffer  &sub_buffer=sub.buffers[i];
               Buffer* &    buffer=*buffers(sub_buffer.name);
               if(!buffer)buffer=&sub_buffer;else if(*buffer!=sub_buffer)return error(S+"Buffer \""+buffer->name+"\" is not always the same in all shaders");
            }
            FREPA(sub.images)images.binaryInclude(sub.images[i].name, CompareCS);

            if(!shader.dummy)
            {
               // buffer map
               FREPA(buffer_maps)if(buffer_maps[i]==sub.buffers){sub.buffer_bind_index=i; goto got_buffer;} // find same
               sub.buffer_bind_index=buffer_maps.elms(); buffer_maps.add(sub.buffers);
            got_buffer:
            
               // image map
               FREPA(image_maps)if(image_maps[i]==sub.images){sub.image_bind_index=i; goto got_image;} // find same
               sub.image_bind_index=image_maps.elms(); image_maps.add(sub.images);
            got_image:

               // shader data
               if(sub.shader_data.elms())
               {
                  Memc<ShaderData> &sds=shader_datas[i];
                  FREPA(sds)if(sds[i]==sub.shader_data){sub.shader_data_index=i; goto got_shader_data;} // find same
                  sub.shader_data_index=sds.elms(); Swap(sds.New(), sub.shader_data); // add new, just swap
               got_shader_data:
                  sub.shader_data.del(); // no longer needed
               }
            }
         }
         if(!shader.dummy)shaders[shaders_num++]=&shader;
      }
   }
   shaders.sort(); // sort by name so we can do binary search when looking for shaders

   if(api!=API_DX)
   {
      ConvertContext cc(T
      #if DEBUG
       , shader_datas, shaders
      #endif
      );
      FREPA(shader_datas)
      {
         Memc<ShaderData> &sds=shader_datas[i];
         FREPA(sds)threads.queue(sds[i], Convert, cc);
      }
      threads.wait1();
   }

   File f; if(f.writeTry(dest))
   {
      f.putUInt (CC4_SHDR); // CC4
      f.putByte (api     ); // API
      f.cmpUIntV(0       ); // version

      // constants
      f.cmpUIntV(buffers.elms()); FREPA(buffers)
      {
       C Buffer &buf=*buffers[i];

         // constant buffer
         f.putStr(buf.name).cmpUIntV(buf.size).putSByte(buf.explicitBindSlot());

         // params
         if(!buf.params.save(f))goto error;
      }

      // images
      f.cmpUIntV(images.elms()); FREPA(images)f.putStr(images[i]);

      // buffer+image map
      if(api!=API_GL)
      {
         if(!buffer_maps.save(f, T   ))goto error;
         if(! image_maps.save(f, T, T))goto error; // use T,T to call secondary 'save' method for images
      }

      // shader data
      FREPA(shader_datas)
      {
         Memc<ShaderData> &shader_data=shader_datas[i]; if(!shader_data.save(f))goto error;
      }

      // shaders
      f.cmpUIntV(shaders.elms()); FREPA(shaders)
      {
       C Shader &shader=*shaders[i];
         if(!shader.save(f, T))goto error;
      }

      if(f.flushOK())return true;

   error:
      f.del(); FDelFile(dest);
   }
   return false;
}
void ShaderCompiler::compile(Threads &threads)
{
   if(!compileTry(threads))Exit(S+"Failed to compile:"+messages);
}
/******************************************************************************/
// IO
/******************************************************************************/
Bool Shader11::load(File &f, C ShaderFile &shader_file, C MemtN<ShaderBuffer*, 256> &file_buffers)
{
   Indexes indexes; f.getStr(name)>>indexes;
   FREPA(data_index)
   {
      data_index[i]=indexes.shader_data_index[i];
      DYNAMIC_ASSERT(InRange(indexes.buffer_bind_index[i], shader_file._buffer_links), "Buffer Bind Index out of range"); buffers[i]=shader_file._buffer_links[indexes.buffer_bind_index[i]];
      DYNAMIC_ASSERT(InRange(indexes. image_bind_index[i], shader_file. _image_links),  "Image Bind Index out of range");  images[i]=shader_file. _image_links[indexes. image_bind_index[i]];
   }
   all_buffers.setNum(f.decUIntV()); FREPAO(all_buffers)=Get(f.getUShort(), file_buffers);
   if(f.ok())return true;
  /*del();*/ return false;
}
/******************************************************************************/
#if GL
Bool ShaderGL::load(File &f, C ShaderFile &shader_file, C MemtN<ShaderBuffer*, 256> &buffers)
{
   // name + indexes
   f.getStr(name).getMulti(vs_index, ps_index);
   if(f.ok())return true;
  /*del();*/ return false;
}
#endif
/******************************************************************************/
static void ExitParam(C Str &param_name, C Str &shader_name)
{
   Exit(S+"Shader Param \""+param_name+"\"\nfrom Shader File \""+shader_name+"\"\nAlready exists in Shader Constants Map but with different parameters.\nThis means that some of your shaders were compiled with different headers.\nPlease recompile your shaders.");
}
Bool ShaderFile::load(C Str &name)
{
   del();

   Str8 temp_str;
   File f; if(f.readTry(Sh.path+name) && f.getUInt()==CC4_SHDR && f.getByte()==GPU_API(API_DX, API_GL))switch(f.decUIntV()) // version
   {
      case 0:
      {
         // buffers
         MemtN<ShaderBuffer*, 256> buffers; buffers.setNum(f.decUIntV());
         ShaderBuffers.lock();
         ShaderParams .lock();
         FREPA(buffers)
         {
            // buffer
            f.getStr(temp_str); ShaderBuffer &sb=*ShaderBuffers(temp_str); buffers[i]=&sb;
            if(!sb.is()) // wasn't yet created
            {
               sb.create(f.decUIntV());
               Int index=f.getSByte(); if(index>=0){SyncLocker lock(D._lock); sb.bind(index);}
            }else // verify if it's identical to previously created
            {
               if(sb.size()!=f.decUIntV())ExitParam(temp_str, name);
               sb.bindCheck(f.getSByte());
            }

            // params
            REP(f.decUIntV())
            {
               f.getStr(temp_str); ShaderParam &sp=*ShaderParams(temp_str);
               if(!sp.is()) // wasn't yet created
               {
                  sp._owns_data= false;
                  sp._data     = sb.data;
                  sp._changed  =&sb.changed;
                  f.getMulti(sp._cpu_data_size, sp._gpu_data_size, sp._elements); // info
                  LoadTranslation(sp._full_translation, f, sp._elements);         // translation
                  Int offset=sp._full_translation[0].gpu_offset; sp._data+=offset; REPAO(sp._full_translation).gpu_offset-=offset; // apply offset
                  if(f.getBool())f.get(sp._data, sp._gpu_data_size);              // load default value, no need to zero in other case, because data is stored in ShaderBuffer's, and they're always zeroed at start
                  sp.optimize(); // optimize
               }else // verify if it's identical to previously created
               {
                  Int cpu_data_size, gpu_data_size, elements; f.getMulti(cpu_data_size, gpu_data_size, elements);
                  Memt<ShaderParam::Translation> translation;
                  if(sp._changed      !=&sb.changed                               // check matching Constant Buffer
                  || sp._cpu_data_size!= cpu_data_size                            // check cpu size
                  || sp._gpu_data_size!= gpu_data_size                            // check gpu size
                  || sp._elements     != elements     )ExitParam(temp_str, name); // check number of elements
                  LoadTranslation(translation, f, sp._elements);                  // translation
                  Int offset=translation[0].gpu_offset; REPAO(translation).gpu_offset-=offset; // apply offset
                  if(f.getBool())f.skip(gpu_data_size);                           // ignore default value

                  // check translation
                  if(                  translation.elms()!=sp._full_translation.elms())ExitParam(temp_str, name);
                  FREPA(translation)if(translation[i]    !=sp._full_translation[i]    )ExitParam(temp_str, name);
               }
            }
         }
         ShaderParams .unlock();
         ShaderBuffers.unlock();

         // images
         MemtN<ShaderImage*, 256> images; images.setNum(f.decUIntV());
         FREPA(images){f.getStr(temp_str); images[i]=ShaderImages(temp_str);}

         // shaders
      #if !GL
         if(_buffer_links.load(f, buffers)) // buffer link map
         if( _image_links.load(f,  images)) //  image link map
      #endif
         if(_vs     .load(f))
         if(_hs     .load(f))
         if(_ds     .load(f))
         if(_ps     .load(f))
         if(_shaders.load(f, T, buffers))
            if(f.ok())return true;
      }break;

      /*CG
      FIXME
      case 0:
      {
         // params
         MemtN<ShaderParam*, 256> params; params.setNum(f.decUIntV());
         ShaderParams.lock();
         FREPA(params)
         {
            f.getStr(temp_str); ShaderParam &sp=*ShaderParams(temp_str); params[i]=&sp;
            if(!sp.is()) // wasn't yet created
            {
               sp._owns_data=true;
               f.getMulti(sp._cpu_data_size, sp._gpu_data_size, sp._elements); // info
               LoadTranslation(sp._full_translation, f, sp._elements);         // translation
               Alloc(sp._data, sp._gpu_data_size);                             // data
               Alloc(sp._changed                );
               if(f.getBool())f.get   (sp._data, sp._gpu_data_size);           // load default value
               else           ZeroFast(sp._data, sp._gpu_data_size);           // zero default value
               sp.optimize();
            }else // verify if it's identical to previously created
            {
               Int cpu_data_size, gpu_data_size, elements; f.getMulti(cpu_data_size, gpu_data_size, elements);
               Memt<ShaderParam::Translation> translation;
               if(sp._cpu_data_size!=cpu_data_size                            // check cpu size
               || sp._gpu_data_size!=gpu_data_size                            // check gpu size
               || sp._elements     !=elements     )ExitParam(temp_str, name); // check number of elements
               LoadTranslation(translation, f, elements);                     // translation
               if(f.getBool())f.skip(gpu_data_size);                          // ignore default value

               // check translation
               if(                  translation.elms()!=sp._full_translation.elms()) ExitParam(temp_str, name);else
               FREPA(translation)if(translation[i]    !=sp._full_translation[i]    ){ExitParam(temp_str, name); break;}
            }
         }
         ShaderParams.unlock();

         // images
         MemtN<ShaderImage*, 256> images; images.setNum(f.decUIntV());
         FREPA(images){f.getStr(temp_str); images[i]=ShaderImages(temp_str);}

         // shaders
         if(_vs     .load(f))
         if(_ps     .load(f))
         if(_shaders.load(f, params, images))
            if(f.ok())return true;
      }break;*/
   }
//error:
   del(); return false;
}
/******************************************************************************/
// FIXME remove: ThirdPartyLibs\D3DX11
// FIXME remove codes below
/******************************************************************************/
#if 0
#if WINDOWS
}
#define THIS void
#include "../../../ThirdPartyLibs/begin.h"
#include "../../../ThirdPartyLibs/D3DX11/inc/d3dx11effect.h"
#include "../../../ThirdPartyLibs/end.h"
namespace EE{
static void Error(ID3DBlob* &error, Str *messages)
{
   if(error)
   {
      if(messages)messages->line()+=(Char8*)error->GetBufferPointer();
      RELEASE(error);
   }
}
#endif
static ShaderParam * Get(Int i, C MemtN<ShaderParam *, 256> &params ) {if(!InRange(i, params ))Exit("Invalid ShaderParam index" ); return  params[i];}
static C Str8& Name(ShaderParam  &param, C Map<Str8, ShaderParam> &params) {C Str8 *name=      params .dataToKey(&param ); if(!name)Exit("Can't find ShaderParam name" ); return *name;}
static C Str8& Name(ShaderParam  &param, C Map<Str8, ShaderParam> &params) {return       params .dataInMapToKey(param );}
/******************************************************************************/
struct ShaderParamEx : ShaderParam 
{
   Bool save(File &f, C Str &name)C
   {
    C Byte *param_data=_data+_full_translation[0].gpu_offset; // in DX10+, '_data' when saving, points to Shader Constant Buffer data, that's why we need to use offset

      f.putStr(name).putMulti(_cpu_data_size, _gpu_data_size, _elements); // name+info
      SaveTranslation(_full_translation, f, _elements);                   // translation
      if(HasData(param_data, _gpu_data_size))                             // data
      {
         f.putBool(true);
         f.put(param_data, _gpu_data_size);
      }else
      {
         f.putBool(false);
      }
      return f.ok();
   }
};
struct ShaderParamName : ShaderParamEx
{
   Str8 name;
};
struct ShaderBufferParams
{
   ShaderBuffer         *buffer;
   Int                   index;
   Mems<ShaderParamName> params;
};

static Int Compare(C ShaderBufferParams &a, C ShaderBufferParams &b) {return ComparePtr(a.buffer, b.buffer);} // sort by buffer pointer, because that's the only thing we can access from 'Shader.Constant'
static Int Compare(C ShaderBufferParams &a,   ShaderBuffer*C     &b) {return ComparePtr(a.buffer, b       );} // sort by buffer pointer, because that's the only thing we can access from 'Shader.Constant'
static Int Compare(  ShaderImage*C      &a,   ShaderImage*C      &b) {return ComparePtr(a       , b       );} // sort by image  pointer, because that's the only thing we can access from 'Shader.Texture'

static Int GetIndex(C Memc<ShaderImage*       > & images, ShaderImage  *image ) {Int index; if(! images.binarySearch(image , index, Compare))Exit("Image not found in Shader" ); return index;}
static Int GetIndex(C Map <Str8, ShaderParamEx> & params, ShaderParam  *param ) {Int index  =    params. dataToIndex((ShaderParamEx*)param); if(index<0)Exit("Param not found in Shader" ); return index;}
static Int GetIndex(C Memc<ShaderBufferParams > &buffers, ShaderBuffer *buffer) {Int index; if(!buffers.binarySearch(buffer, index, Compare))Exit("Buffer not found in Shader"); return index;}
/******************************************************************************/
// TRANSLATION
/******************************************************************************/
#if DX11
static void AddTranslation11(ShaderParam &sp, ID3DX11EffectVariable *par, C D3DX11_EFFECT_VARIABLE_DESC &var_desc, C D3DX11_EFFECT_TYPE_DESC &par_desc)
{
   if(par_desc.Elements<=1) // array size
   {
      if(par_desc.Class==D3D_SVC_SCALAR || par_desc.Class==D3D_SVC_VECTOR) // for example: Flt f,f[]; Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
      {
         if(par_desc.Rows!=1)Exit("Shader Param Rows!=1");
         if(par_desc.Type==D3D_SVT_FLOAT)
         {
            sp._full_translation.New().set(sp._cpu_data_size, var_desc.BufferOffset, SIZE(Flt)*par_desc.Columns);
            sp._cpu_data_size+=SIZE(Flt)*par_desc.Columns;
         }else Exit(S+"Unhandled Shader Parameter Type for \""+var_desc.Name+'"');
      }else
      if(par_desc.Class==D3D_SVC_MATRIX_COLUMNS)
      {
         if(par_desc.Rows   >4)Exit("Shader Param Matrix Rows>4");
         if(par_desc.Columns>4)Exit("Shader Param Matrix Cols>4");
         if(par_desc.Type!=D3D_SVT_FLOAT)Exit(S+"Unhandled Shader Parameter Type for \""+var_desc.Name+'"');

         FREPD(y, par_desc.Columns)
         FREPD(x, par_desc.Rows   )sp._full_translation.New().set(sp._cpu_data_size+SIZE(Flt)*(y+x*par_desc.Columns), var_desc.BufferOffset+SIZE(Flt)*(x+y*4), SIZE(Flt));
         sp._cpu_data_size+=SIZE(Flt)*par_desc.Rows*par_desc.Columns;
      }else
      if(par_desc.Class==D3D_SVC_STRUCT)
      {
         FREP(par_desc.Members) // number of members
         {
            ID3DX11EffectVariable *member=par->GetMemberByIndex(i);
            D3DX11_EFFECT_VARIABLE_DESC var_desc; member->GetDesc(&var_desc);
            D3DX11_EFFECT_TYPE_DESC     par_desc; member->GetType()->GetDesc(&par_desc);
            AddTranslation11(sp, member, var_desc, par_desc);
            RELEASE(member);
         }
      }
   }else
   {
      FREP(par_desc.Elements)
      {
         ID3DX11EffectVariable *elm=par->GetElement(i);
         D3DX11_EFFECT_VARIABLE_DESC var_desc; elm->GetDesc(&var_desc);
         D3DX11_EFFECT_TYPE_DESC     par_desc; elm->GetType()->GetDesc(&par_desc);
         AddTranslation11(sp, elm, var_desc, par_desc);
         RELEASE(elm);
      }
   }
}
#endif
/******************************************************************************/
// SAVE
/******************************************************************************/
#if DX11
static void SaveBuffers(File &f, C Mems<Shader11::Buffer> &constants, C Memc<ShaderBufferParams> &file_buffers, MemtN<UShort, 256> &all)
{
   MemtN<ConstantIndex, 256> save;
   FREPA(constants)
   {
      Int buffer_index=GetIndex(file_buffers, constants[i].buffer); // index of buffer in 'file_buffers' array
    C ShaderBufferParams &buffer=file_buffers[buffer_index];
      if(buffer.index<0) // here we have to save only buffers that don't have a constant bind point index
         save.New().set(constants[i].index, buffer_index); // save to which index this buffer should be bound for this shader, and index of buffer in 'file_buffers' array
      all.binaryInclude(AsUShort(buffer_index));
   }
   save.saveRaw(f);
}
struct Shader11Ex : Shader11
{
   Bool save(File &f, C Memc<ShaderBufferParams> &buffers, C Memc<ShaderImage*> &images)C
   {
      // name
      f.putStr(name).putMulti(vs_index, hs_index, ds_index, ps_index);

      // images
      f.cmpUIntV(vs_textures.elms()); FREPA(vs_textures)f<<ConstantIndex(vs_textures[i].index, GetIndex(images, vs_textures[i].image));
      f.cmpUIntV(hs_textures.elms()); FREPA(hs_textures)f<<ConstantIndex(hs_textures[i].index, GetIndex(images, hs_textures[i].image));
      f.cmpUIntV(ds_textures.elms()); FREPA(ds_textures)f<<ConstantIndex(ds_textures[i].index, GetIndex(images, ds_textures[i].image));
      f.cmpUIntV(ps_textures.elms()); FREPA(ps_textures)f<<ConstantIndex(ps_textures[i].index, GetIndex(images, ps_textures[i].image));

      // buffers
      MemtN<UShort, 256> all;
      SaveBuffers(f, vs_buffers, buffers, all);
      SaveBuffers(f, hs_buffers, buffers, all);
      SaveBuffers(f, ds_buffers, buffers, all);
      SaveBuffers(f, ps_buffers, buffers, all);
      all.saveRaw(f);

      return f.ok();
   }
};
static Bool ShaderSave(C Str &name, C Memc<ShaderBufferParams> &buffers, C Memc<ShaderImage*> &images, C Memc<ShaderVS11> &vs, C Memc<ShaderHS11> &hs, C Memc<ShaderDS11> &ds, C Memc<ShaderPS11> &ps, C Memc<Shader11Ex> &techs)
{
   File f; if(f.writeTry(name))
   {
      f.putUInt (CC4_SHDR); // cc4
      f.putByte (API_DX  ); // API
      f.cmpUIntV(0       ); // version

      // constants
      f.cmpUIntV(buffers.elms()); FREPA(buffers)
      {
       C ShaderBufferParams &buf=buffers[i];

         // constant buffer
         f.putStr(Name(*buf.buffer)).cmpUIntV(buf.buffer->size()).putSByte(buf.index); DYNAMIC_ASSERT(buf.index>=-1 && buf.index<=127, "buf.index out of range");

         // params
         f.cmpUIntV(buf.params.elms());
         FREPAO(    buf.params).save(f, buf.params[i].name);
      }

      // images
      f.cmpUIntV(images.elms());
      FREPA(images)f.putStr(Name(*images[i]));

      if(vs   .save(f)) // shaders
      if(hs   .save(f))
      if(ds   .save(f))
      if(ps   .save(f))
      if(techs.save(f, buffers, images)) // techniques
         if(f.flushOK())return true;

      f.del(); FDelFile(name);
   }
   return false;
}
#endif
struct ShaderGLEx : ShaderGL
{
   Bool save(File &f, C Map<Str8, ShaderParamEx> &params, C Memc<ShaderImage*> &images)C
   {
      f.putStr(name);
      f.putMulti(vs_index, ps_index);
      f.cmpUIntV(glsl_params.elms()); FREPA(glsl_params)f<<glsl_params[i].gpu_offset<<AsUShort(GetIndex(params, glsl_params[i].param))<<glsl_params[i].glsl_name;
    //f.cmpUIntV(glsl_images.elms()); FREPA(glsl_images)f<<                           AsUShort(GetIndex(images, glsl_images[i]      ));
      return f.ok();
   }
};
static Bool ShaderSave(C Str &name, C Map<Str8, ShaderParamEx> &params, C Memc<ShaderImage*> &images, C Memc<ShaderVSGL> &vs, C Memc<ShaderPSGL> &ps, C Memc<ShaderGLEx> &techs)
{
   File f; if(f.writeTry(name))
   {
      f.putUInt (CC4_SHDR); // cc4
      f.putByte (API_GL  ); // API
      f.cmpUIntV(0       ); // version

      // params
      f.cmpUIntV(params.elms());
      FREPAO(    params).save(f, params.key(i));

      // images
      f.cmpUIntV(images.elms());
      FREPA(images)f.putStr(Name(*images[i]));

      if(vs   .save(f)) // shaders
      if(ps   .save(f))
      if(techs.save(f, params, images)) // techniques
         if(f.flushOK())return true;

      f.del(); FDelFile(name);
   }
   return false;
}
/******************************************************************************/
// COMPILE
/******************************************************************************/
#if DX11
static Int Compare(C Shader11Ex &a, C Shader11Ex &b) {return CompareCS(a.name, b.name);}
#endif
static Int Compare(C ShaderGLEx &a, C ShaderGLEx &b) {return CompareCS(a.name, b.name);}
static Int Compare(C ShaderGLEx &a, C Str8       &b) {return CompareCS(a.name, b     );}
/******************************************************************************/
static Bool ShaderCompile11(C Str &src, C Str &dest, C MemPtr<ShaderMacro> &macros, Str *messages)
{
#if DX11
   /*{
      if(messages)messages->clear();

      FileText f; if(!f.read(src)){if(messages)messages->line()+="Failed to open file."; return false;}
      Str data; if(!f.getAll(data).ok()){if(messages)messages->line()+="Failed to read from file."; return false;} f.del(); // release the file handle after reading

      IDxcBlobEncoding *input=null;
      ID3DBlob *buffer=null, *error=null;
      IDxcLibrary *lib=null; CreateLibrary(&lib); if(lib)
      {
         lib->CreateBlobWithEncodingFromPinned(data(), data.length()*SIZE(Char), CP_UTF16, &input);
         lib->Release();
      }
      Mems<D3D_SHADER_MACRO> d3d_macros; d3d_macros.setNum(macros.elms()+1); FREPA(macros){D3D_SHADER_MACRO &m=d3d_macros[i]; m.Name=macros[i].name; m.Definition=macros[i].definition;} Zero(d3d_macros.last());
      int r=CompileFromBlob(input, src, d3d_macros.data(), &Include12(src), "Test_PS", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &buffer, &error); Error(error, messages);
      if(buffer)
      {
       //ID3DX11Effect *effect=null; D3DX11CreateEffectFromMemory(buffer->GetBufferPointer(), buffer->GetBufferSize(), 0, D3D, &effect);
         ID3D11PixelShader *ps=null; D3D->CreatePixelShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), null, &ps); // this is ok?
         int z=0;
      }
      int z=0;
      ClipSet(*messages);
      Exit();
   }*/
   if(messages)messages->clear();

   File f; if(!f.readTry(src)){if(messages)messages->line()+="Failed to open file."; return false;}
   Mems<Byte> data; data.setNum(f.size()); if(!f.get(data.data(), f.size())){if(messages)messages->line()+="Failed to read from file."; return false;} f.del(); // release the file handle after reading

   ID3DBlob *buffer=null, *error=null;
   Mems<D3D_SHADER_MACRO> d3d_macros; d3d_macros.setNum(macros.elms()+1); FREPA(macros){D3D_SHADER_MACRO &m=d3d_macros[i]; m.Name=macros[i].name; m.Definition=macros[i].definition;} Zero(d3d_macros.last());
   D3DCompile(data.data(), data.elms(), (Str8)src, d3d_macros.data(), &Include11(src), null, "fx_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_NO_PRESHADER, 0, &buffer, &error); Error(error, messages);

   ID3DX11Effect *effect=null;
   if(buffer)
   {
    //SyncLocker lock(D._lock); lock not needed for DX11 'D3D'
      D3DX11CreateEffectFromMemory(buffer->GetBufferPointer(), buffer->GetBufferSize(), 0, D3D, &effect);
      buffer->Release();
   }

   if(effect)
   {
      ShaderFile               shader;
      Memc<ShaderBufferParams> buffers;
      Memc<ShaderImage*>       images;
      Memc<ShaderVS11>         vs;
      Memc<ShaderHS11>         hs;
      Memc<ShaderDS11>         ds;
      Memc<ShaderPS11>         ps;
      Memc<Shader11Ex>         techs;

      D3DX11_EFFECT_DESC desc; effect->GetDesc(&desc);

      // build list of parameters
      FREP(desc.ConstantBuffers)if(ID3DX11EffectConstantBuffer *cb=effect->GetConstantBufferByIndex(i))
      {
         ID3D11Buffer               *buffer=null; cb    ->GetConstantBuffer(&buffer);
         D3D11_BUFFER_DESC           bd         ; buffer->GetDesc(&bd);
         D3DX11_EFFECT_VARIABLE_DESC desc       ; cb    ->GetDesc(&desc);
         RELEASE(buffer);

         // create Constant Buffer
         ShaderBuffers.lock();
         ShaderBuffer       &buf       =*ShaderBuffers(Str8Temp(desc.Name));
         ShaderBufferParams &buf_params=buffers.New(); buf_params.buffer=&buf; buf_params.index=((desc.Flags&D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT) ? desc.ExplicitBindPoint : -1);
         if(!buf.is()) // not yet initialized
         {
            buf.create(bd.ByteWidth);
            cb->GetRawValue(buf.data, 0, buf.size()); buf.changed=true; // set default value
            if(buf_params.index>=0){SyncLocker lock(D._lock); buf.bind(buf_params.index);}
         }else
         {
            if(buf.size()!=bd.ByteWidth)Exit(S+"Constant Buffer \""+desc.Name+"\" already exists in constant buffer Map however with different size");
            buf.bindCheck(buf_params.index);
         }
         ShaderBuffers.unlock();

         // set all members
         D3DX11_EFFECT_TYPE_DESC type; cb->GetType()->GetDesc(&type);
         FREP(type.Members)
         {
            ID3DX11EffectVariable      *member=cb->GetMemberByIndex(i);
            D3DX11_EFFECT_VARIABLE_DESC desc; member->GetDesc(&desc);
            D3DX11_EFFECT_TYPE_DESC     type; member->GetType()->GetDesc(&type);
            ShaderParamName            &sp=buf_params.params.New();
            if(sp.is())Exit(S+"Shader parameter \""+desc.Name+"\" listed more than once");else // if wasn't yet created
            {
               sp. name     = desc.Name;
               sp._owns_data= false;
               sp._changed  =&buf .changed;
               sp._data     = buf .data; // not yet offsetted
               sp._elements = type.Elements;

               AddTranslation11(sp, member, desc, type);
               sp._gpu_data_size=type.UnpackedSize;

               if(sp._cpu_data_size!=type.  PackedSize
               || sp._gpu_data_size!=type.UnpackedSize)Exit("Incorrect Shader Param size.\nPlease contact Developer.");
               if(sp._gpu_data_size+sp._full_translation[0].gpu_offset>buf.size())Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
             //if(SIZE(Vec4)       +sp._full_translation[0].gpu_offset>buf.size())Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4
            }
         }
         RELEASE(cb);
      }
      buffers.sort(Compare); // once we have all buffers for this file, sort them, so we can use binary search later while saving techniques when looking for buffer indexes

      // build list of textures/samplers
      FREP(desc.GlobalVariables)if(ID3DX11EffectVariable *var=effect->GetVariableByIndex(i))
      {
         D3DX11_EFFECT_VARIABLE_DESC desc; var->GetDesc(&desc);
         D3DX11_EFFECT_TYPE_DESC     type; var->GetType()->GetDesc(&type);

         if(type.Type==D3D_SVT_TEXTURE2D || type.Type==D3D_SVT_TEXTURE3D || type.Type==D3D_SVT_TEXTURECUBE || type.Type==D3D_SVT_TEXTURE2DMS)
            images.add(ShaderImages(Str8Temp(desc.Name)));
         RELEASE(var);
      }
      images.sort(Compare); // once we have all images for this file, sort them, so we can use binary search later while saving techniques when looking for image indexes

      // build list of techniques
      FREP(desc.Techniques)if(ID3DX11EffectTechnique *tech_handle=effect->GetTechniqueByIndex(i))
      {
         D3DX11_TECHNIQUE_DESC tech_desc; tech_handle->GetDesc            (&tech_desc); if(tech_desc.Passes!=1)Exit("Technique pass count!=1"); ID3DX11EffectPass *pass=tech_handle->GetPassByIndex(0);
         D3DX11_PASS_DESC      pass_desc; pass       ->GetDesc            (&pass_desc);
         D3DX11_PASS_SHADER_DESC vs_desc; pass       ->GetVertexShaderDesc(&  vs_desc); D3DX11_EFFECT_SHADER_DESC vsd; Zero(vsd); if(vs_desc.pShaderVariable)vs_desc.pShaderVariable->GetShaderDesc(0, &vsd);
         D3DX11_PASS_SHADER_DESC hs_desc; pass       ->GetHullShaderDesc  (&  hs_desc); D3DX11_EFFECT_SHADER_DESC hsd; Zero(hsd); if(hs_desc.pShaderVariable)hs_desc.pShaderVariable->GetShaderDesc(0, &hsd);
         D3DX11_PASS_SHADER_DESC ds_desc; pass       ->GetDomainShaderDesc(&  ds_desc); D3DX11_EFFECT_SHADER_DESC dsd; Zero(dsd); if(ds_desc.pShaderVariable)ds_desc.pShaderVariable->GetShaderDesc(0, &dsd);
         D3DX11_PASS_SHADER_DESC ps_desc; pass       ->GetPixelShaderDesc (&  ps_desc); D3DX11_EFFECT_SHADER_DESC psd; Zero(psd); if(ps_desc.pShaderVariable)ps_desc.pShaderVariable->GetShaderDesc(0, &psd);

         Shader11 &tech=techs.New(); tech.name=tech_desc.Name;

         // get shader data
         ShaderData vs_data; vs_data.setNum(vsd.BytecodeLength).copyFrom(vsd.pBytecode);
         ShaderData hs_data; hs_data.setNum(hsd.BytecodeLength).copyFrom(hsd.pBytecode);
         ShaderData ds_data; ds_data.setNum(dsd.BytecodeLength).copyFrom(dsd.pBytecode);
         ShaderData ps_data; ps_data.setNum(psd.BytecodeLength).copyFrom(psd.pBytecode);

#if DEBUG && 0 // use this for generation of a basic Vertex Shader which can be used for Input Layout creation (see 'DX10_INPUT_LAYOUT' and 'VS_Code')
   Str t=S+"static Byte VS_Code["+vs_data.elms()+"]={";
   FREPA(vs_data){if(i)t+=','; t+=vs_data[i];}
   t+="};\n";
   ClipSet(t);
   Exit(t);
#endif

         // store shaders
         if(vs_data.elms()){FREPA(vs)if(vs[i].elms()==vs_data.elms() && EqualMem(vs[i].data(), vs_data.data(), vs_data.elms())){tech.vs_index=i; break;} if(tech.vs_index<0){tech.vs_index=vs.elms(); Swap(SCAST(ShaderData, vs.New()), vs_data);}}
         if(hs_data.elms()){FREPA(hs)if(hs[i].elms()==hs_data.elms() && EqualMem(hs[i].data(), hs_data.data(), hs_data.elms())){tech.hs_index=i; break;} if(tech.hs_index<0){tech.hs_index=hs.elms(); Swap(SCAST(ShaderData, hs.New()), hs_data);}}
         if(ds_data.elms()){FREPA(ds)if(ds[i].elms()==ds_data.elms() && EqualMem(ds[i].data(), ds_data.data(), ds_data.elms())){tech.ds_index=i; break;} if(tech.ds_index<0){tech.ds_index=ds.elms(); Swap(SCAST(ShaderData, ds.New()), ds_data);}}
         if(ps_data.elms()){FREPA(ps)if(ps[i].elms()==ps_data.elms() && EqualMem(ps[i].data(), ps_data.data(), ps_data.elms())){tech.ps_index=i; break;} if(tech.ps_index<0){tech.ps_index=ps.elms(); Swap(SCAST(ShaderData, ps.New()), ps_data);}}

         FREPD(shader, 4) // vs, hs, ds, ps
         {
            Mems<Shader11::Buffer >   & buffers=((shader==0) ? tech.vs_buffers  : (shader==1) ? tech.hs_buffers  : (shader==2) ? tech.ds_buffers  : tech.ps_buffers );
            Mems<Shader11::Texture>   &textures=((shader==0) ? tech.vs_textures : (shader==1) ? tech.hs_textures : (shader==2) ? tech.ds_textures : tech.ps_textures);
            D3DX11_EFFECT_SHADER_DESC &sd      =((shader==0) ?      vsd         : (shader==1) ?      hsd         : (shader==2) ?      dsd         :      psd        );
            if(sd.pBytecode)
            {
               ID3D11ShaderReflection *reflection=null; D3DReflect(sd.pBytecode, sd.BytecodeLength, IID_ID3D11ShaderReflection, (Ptr*)&reflection); if(!reflection)Exit("Failed to get reflection");
               D3D11_SHADER_DESC desc; reflection->GetDesc(&desc);

               FREP(desc.BoundResources)
               {
                  D3D11_SHADER_INPUT_BIND_DESC desc; reflection->GetResourceBindingDesc(i, &desc);
                  switch(desc.Type)
                  {
                     case D3D_SIT_CBUFFER: if(!InRange(desc.BindPoint, MAX_SHADER_BUFFERS))Exit(S+"Constant Buffer index: "+desc.BindPoint+", is too big");  buffers.New().set(desc.BindPoint, *ShaderBuffers(Str8Temp(desc.Name))); break;
                     case D3D_SIT_TEXTURE: if(!InRange(desc.BindPoint, MAX_SHADER_IMAGES ))Exit(S+"Image index: "          +desc.BindPoint+", is too big"); textures.New().set(desc.BindPoint, *ShaderImages (Str8Temp(desc.Name))); break;
                  }
               }

               RELEASE(reflection);
            }
         }
         RELEASE(tech_handle);
      }

      {SyncLocker lock(D._lock); RELEASE(effect);}

      techs.sort(Compare);

      return ShaderSave(dest, buffers, images, vs, hs, ds, ps, techs);
   }
#endif
   return false;
}
/******************************************************************************/
#if GL && !GL_ES && WINDOWS
static Str8 RemoveR       (CChar8 *text) {return Replace(text, '\r', '\0');}
static Str8 RemoveComments(CChar8 *text)
{
   Str8 s; s.reserve(Length(text));
   if(text)for(Int in_comment=0; ; ) // 0=no, 1=in line "//", 2=in big "/* */"
   {
      Char8 c=*text++; if(!c)break;
      switch(in_comment)
      {
         default:
         {
            if(c=='/')
            {
               if(text[0]=='/'){in_comment=1; text++; break;} // don't process '/' again
               if(text[0]=='*'){in_comment=2; text++; break;} // don't process '*' again, important to treat /*/ not as closure
            }
            s+=c;
         }break;
         case 1: if(c=='\n'               ){in_comment=0; s.line();} break;
         case 2: if(c=='*' && text[0]=='/'){in_comment=0;   text++;} break; // don't process '*' again, important to treat /*..*/* not as new comment
      }
   }
   return s;
}
static Str8 GetToken(CChar8* &text)
{
   Str8 s;
   if(text)for(;;)
   {
      Char8 c=*text;
      if(CharType(c)==CHART_CHAR){s+=c; text++;}else // append name
      if(!s.is() && c>32){text++; return c;}else     // return operator
      if( s.is() || c>32 || c==0)break;else text++;  // continue
   }
   return s;
}
static Str8 RemoveUnusedStructs(CChar8 *text)
{
   Str8 s; s.reserve(Length(text));
   if(text)for(;;)
   {
      if(Starts(text, "struct", true, true))
      {
         CChar8 *token=text+6; // Length("struct") -> 6
         Str8    name =GetToken(token);
         if(name.is() && !Contains(token, name, true, true))
         {
            for(Int level=0; ; )
            {
               Char8 c=*token++; if(!c)break;
               if(c=='{')   ++level;
               if(c=='}')if(--level<=0)
               {
                  if(GetToken(token)==';'){text=token; goto next;}
                  break;
               }
            }
         }
      }
      {
         Char8 c=*text++; if(!c)break;
         s+=c;
      }
   next:;
   }
   return s;
}
static Str8 RemoveSpaces(CChar8 *text)
{
   Str8 s; s.reserve(Length(text));
   Bool possible_preproc=true, preproc=false;
   if(text && text[0])for(Char8 last=0;;)
   {
      if(text[0]==' '
      && (CharType(last)!=CHART_CHAR || CharType(text[1])!=CHART_CHAR)
      && (preproc ? last!=')' && text[1]!='(' : true) // don't remove spaces around brackets when in preprocessor mode "#define X(a) a" would turn into "#define X(a)a", same thing for "#define X (X+1)" would turn into "#define X(X+1)"
      && (last!='/' || text[1]!='*')  // don't remove spaces around / * because this could trigger comment mode
      && (last!='*' || text[1]!='/')  // don't remove spaces around * / because this could trigger comment mode
      )
      {
         text+=1;
         last =s.last();
      }else
      {
         last=*text++; if(!last)break;
         s+=last;
         if(last=='\n'){possible_preproc=true; preproc=false;}else
         if(last!=' ' && last!='\t' && last!='#')possible_preproc=false;else
         if(last=='#' && possible_preproc)preproc=true;
      }
   }
   return s;
}
static Str8 RemoveEmptyLines(CChar8 *text)
{
   Str8 s; s.reserve(Length(text));
   if(text)for(Char8 last='\n'; ; )
   {
      Char8 c=*text++; if(!c)break;
      if(c=='\n' && (last=='\n' || !*text))continue;
      s+=c; last=c;
   }
   return s;
}
static Str8 Clean(C Str8 &text)
{
   return RemoveEmptyLines(RemoveSpaces(RemoveUnusedStructs(RemoveComments(RemoveR(text)))));
}
/******************************************************************************/
static struct Varyings
{
   CChar8 *from, *to;
}varyings[]=
{
   // vertex shader
   {"gl_FrontColor"         , "GL_Col" },
   {"gl_FrontSecondaryColor", "GL_Col1"},

   // pixel shader
   {"gl_Color"         , "GL_Col" },
   {"gl_SecondaryColor", "GL_Col1"},

   // both
   {"gl_TexCoord[0]", "GL_Tex0"},
   {"gl_TexCoord[1]", "GL_Tex1"},
   {"gl_TexCoord[2]", "GL_Tex2"},
   {"gl_TexCoord[3]", "GL_Tex3"},
   {"gl_TexCoord[4]", "GL_Tex4"},
   {"gl_TexCoord[5]", "GL_Tex5"},
   {"gl_TexCoord[6]", "GL_Tex6"},
   {"gl_TexCoord[7]", "GL_Tex7"},
   {"gl_TexCoord[8]", "GL_Tex8"},
};
static Str8 CleanCGShader(CChar8 *shader)
{
   Int      length=Length(shader);
   Str8     cleaned; cleaned.reserve(length);
   FileText f; f.readMem(shader, length);
   for(Str8 line; !f.end(); )
   {
      f.getLine(line);
      if(line.is() && !Starts(line, "//") && !Starts(line, "#extension GL_NV_fragdepth") && line!="attribute ivec4 ATTR15;")
      {
         cleaned+=line;
         cleaned+='\n';
      }
   }

   Char8 temp[256], dest[256];

   // rename CG style samplers, example "//var sampler2DSHADOW ShdMap :  : _TMP214 : -1 : 1"
   for(CChar8 *t=shader; t=TextPos(t, "//var sampler2DSHADOW ", true, true); )
   {
      t+=22;
      if(CChar8 *cg=TextPos(t, " :  : "))
      {
         Int name_length=cg-t+1; if(name_length>=Elms(dest))Exit("name too long");
         Set(dest, t, name_length);
         if(t=_SkipWhiteChars(cg+6))
         {
            Int i=0; for(;;)
            {
               Char8 c=*t++;
               if(i>=Elms(temp)-1)Exit(S+"Uniform name too long"); // leave room for '\0'
               if(!c || c=='[' || c==',' || c==':' || c==';' || c=='=' || WhiteChar(c))break;
               temp[i++]=c;
            }
            if(i)
            {
               temp[i]='\0';
               cleaned=Replace(cleaned, temp, dest, true, true);
            }
         }
      }
   }
   // rename CG style uniforms "_name" to "name"
   for(CChar8 *t=shader; t=TextPos(t, "uniform ", true, true); )
      if(t=_SkipWhiteChars(TextPos(_SkipWhiteChars(t+8), ' ')))
   {
      Int i=0; for(;;)
      {
         Char8 c=*t++;
         if(i>=Elms(temp)-1)Exit(S+"Uniform name too long"); // leave room for '\0'
         if(!c || c=='[' || c==',' || c==':' || c==';' || c=='=' || WhiteChar(c))break;
         temp[i++]=c;
      }
      temp[i]='\0';
      if(temp[0]!='_')Exit(S+"Uniform \""+temp+"\" doesn't start with '_'");
      cleaned=Replace(cleaned, temp, temp+1, true, true);
   }

   // user clip plane, CG doesn't support 'gl_ClipDistance', so instead 'gl_BackSecondaryColor' is used, after CG compiles successfully, 'gl_BackSecondaryColor' is replaced with 'gl_ClipDistance'
   cleaned=Replace(cleaned, "gl_BackSecondaryColor.x", "gl_ClipDistance[0]", true, true); // replace dummy "gl_BackSecondaryColor" with "gl_ClipDistance[0]"

   // vtx.instance() is hardcoded as "uint _instance:ATTR15", because CG doesn't support "gl_InstanceID" or "SV_InstanceID" semantics
   cleaned=Replace(cleaned, "int(ATTR15.x)", "gl_InstanceID", true);

   // replace built-in varyings, and keep only those that are used
   REPA(varyings)if(Contains(cleaned, varyings[i].from))cleaned=S+"varying vec4 "+varyings[i].to+";\n"+Replace(cleaned, varyings[i].from, varyings[i].to); // "varying vec4 GL_Col;"

   return Clean(cleaned);
}
static Str StrInclude(FileText &f, C Str &parent)
{
   Str out;
   for(; !f.end(); )
   {
      C Str &line=f.fullLine();
      if(Starts(line, "#include"))
      {
         Str file=StrInside(line, '"', '"', true, true);
         if(!FullPath(file))file=GetPath(parent).tailSlash(true)+file; file=NormalizePath(file);
         FileText f; if(!f.read(file))Exit(S+"Error opening file: \""+file+"\"");
         out+=StrInclude(f, file);
      }else
      {
         out+=line;
      }
      out+='\n';
   }
   return out;
}
static Str StrInclude(C Str &str, C Str &parent)
{
   return StrInclude(FileText().writeMem().putText(str).rewind(), parent);
}
static struct FromTo
{
   CChar8 *from, *to;
}ee_glsl[]=
{
   {"inline ", ""     },
   {"in out" , "inout"},

   {"Bool" , "bool" },
   {"Int"  , "int"  },
   {"UInt" , "uint" },
   {"Flt"  , "float"},
   {"VecI2", "ivec2"},
   {"VecI" , "ivec3"},
   {"VecI4", "ivec4"},
   {"Vec2" , "vec2" },
   {"Vec"  , "vec3" },
   {"Vec4" , "vec4" },

   {"Dot"      , "dot"},
   {"Cross"    , "cross"},
   {"Sign"     , "sign"},
   {"Abs"      , "abs"},
   {"Min"      , "min"},
   {"Max"      , "max"},
   {"Mid"      , "clamp"},
   {"Frac"     , "fract"},
   {"Round"    , "round"},
   {"Trunc"    , "trunc"},
   {"Floor"    , "floor"},
   {"Ceil"     , "ceil"},
   {"Sqrt"     , "sqrt"},
   {"Normalize", "normalize"},
   {"Pow"      , "pow"},
   {"Sin"      , "sin"},
   {"Cos"      , "cos"},
   {"Lerp"     , "mix"},
   {"Length"   , "length"},

   {"Image"    , "sampler2D"},
   {"Image3D"  , "sampler3D"},
   {"ImageCube", "samplerCube"},

   {"Tex"    , "texture2D"},
   {"Tex3D"  , "texture3D"},
   {"TexCube", "textureCube"},

   {"O_vtx", "gl_Position"},
};

/*
 "= MP Vec(..)" code is not supported on Mali (Samsung Galaxy S2)

 MP, HP always point to mediump, highp (LP lowp is no longer used)

https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions

GL  GLSL
2.0 110
2.1 120
3.0 130
3.1 140
3.2 150
3.3 330
4.0 400

2.0ES 100 es
3.0ES 300 es

*/

static Str8 GLSLVSShader(Str8 code)
{
   Bool texture2DLod=Contains(code, "texture2DLod", true, true),
        texture3DLod=Contains(code, "texture3DLod", true, true);
   return S
      +"#ifdef GL_ES\n" // GLSL may not support "#if GL_ES" if GL_ES is not defined
      +   "#define MP mediump\n"
      +   "#define HP highp\n"
      +   "precision HP float;\n"
      +   "precision HP int;\n"
      +"#else\n"
      +   "#define MP\n"
      +   "#define HP\n"
      +"#endif\n"
      +(texture2DLod ? "#define texture2DLod textureLod\n" : "") // 'texture2DLod' is actually 'textureLod' on GL3
      +(texture3DLod ? "#define texture3DLod textureLod\n" : "") // 'texture3DLod' is actually 'textureLod' on GL3
      +"#define attribute in\n"
      +"#define varying out\n"
      +code;
}
static Str8 GLSLPSShader(Str8 code, Bool force_hp)
{
   // replace "shadow2DProj(..).x" with "shadow2DProj(..)"
   for(Int offset=0; ; )
   {
   again:
      Int i=TextPosI(code()+offset, "shadow2DProj(", true, true); if(i<0)break; offset+=i+12; // 12=Length("shadow2DProj")
      for(Int level=0; offset<code.length(); offset++)switch(code[offset])
      {
         case '(':     ++level; break;
         case ')': if(!--level)
         {
            offset++;
            if(code[offset]=='.')code.remove(offset, 2); // remove ".x"
            goto again;
         }break;
      }
   }

   Bool dd=(Contains(code, "dFdx", true, true) // detect if uses 'ddx/ddy' functions
         || Contains(code, "dFdy", true, true)),
       mrt= Contains(code, "#extension GL_ARB_draw_buffers:require", true, true), // detect if uses MRT
       rt0=(Contains(code, "gl_FragData[0]", true, true) || Contains(code, "gl_FragColor", true, true)),
       rt1= Contains(code, "gl_FragData[1]", true, true),
       rt2= Contains(code, "gl_FragData[2]", true, true),
       rt3= Contains(code, "gl_FragData[3]", true, true),
       sampler2DShadow=Contains(code, "sampler2DShadow", true, true),
       sampler3D      =Contains(code, "sampler3D"      , true, true),       texture2D      =Contains(code, "texture2D"      , true, true),
       texture2DLod   =Contains(code, "texture2DLod"   , true, true),
       texture3D      =Contains(code, "texture3D"      , true, true),
       texture3DLod   =Contains(code, "texture3DLod"   , true, true),
       textureCube    =Contains(code, "textureCube"    , true, true),
       shadow2D       =Contains(code, "shadow2D"       , true, true),
       shadow2DProj   =Contains(code, "shadow2DProj"   , true, true);

   if(mrt)code=Replace(code, "#extension GL_ARB_draw_buffers:require\n", S, true, true); // CG may generate this after some other commands, however compiling on Radeon will fail, because it needs to be at start "syntax error: #extension must always be before any non-preprocessor tokens", so remove it and place at the start manually

   if(rt0)code=Replace(Replace(code, "gl_FragData[0]", "RT0", true, true), "gl_FragColor", "RT0", true, true);
   if(rt1)code=        Replace(code, "gl_FragData[1]", "RT1", true, true);
   if(rt2)code=        Replace(code, "gl_FragData[2]", "RT2", true, true);
   if(rt3)code=        Replace(code, "gl_FragData[3]", "RT3", true, true);

   return S // extensions must be listed before any non-preprocessor codes or compilation will fail on Samsung Galaxy S3 with error "Extension directive must occur before any non-preprocessor tokens"
      +      "#extension GL_EXT_shader_texture_lod:enable\n"         // without this, pixel/fragment shaders using TexLod  will not work on Mobile GLES2
      +      "#extension GL_EXT_shadow_samplers:enable\n"            // without this, pixel/fragment shaders using shadows will not work on Mobile GLES2
      +(dd ? "#extension GL_OES_standard_derivatives:enable\n" : "") // without this, pixel/fragment shaders using ddx/ddy will not work on Mobile GLES2
      // set things after extensions
      +"#ifdef GL_ES\n" // GLSL may not support "#if GL_ES" if GL_ES is not defined
      +   "#define MP mediump\n"
      +   "#define HP highp\n"
      +   (force_hp        ? "precision HP float;\n"           : "precision MP float;\n")
      +   (force_hp        ? "precision HP int;\n"             : "precision MP int;\n")
      +   (force_hp        ? "precision HP sampler2D;\n"       : "") // may be needed for depth textures
      +   (sampler2DShadow ? "precision MP sampler2DShadow;\n" : "")
      +   (sampler3D       ? "precision MP sampler3D;\n"       : "")
      +"#else\n"
      +   "#define MP\n"
      +   "#define HP\n"
      +"#endif\n"
      +(texture2D    ? "#define texture2D texture\n"        : "") // 'texture2D'    is actually 'texture'     on GL3
      +(texture2DLod ? "#define texture2DLod textureLod\n"  : "") // 'texture2DLod' is actually 'textureLod'  on GL3
      +(texture3D    ? "#define texture3D texture\n"        : "") // 'texture3D'    is actually 'texture'     on GL3
      +(texture3DLod ? "#define texture3DLod textureLod\n"  : "") // 'texture3DLod' is actually 'textureLod'  on GL3
      +(textureCube  ? "#define textureCube texture\n"      : "") // 'textureCube'  is actually 'texture'     on GL3
      +(shadow2D     ? "#define shadow2D texture\n"         : "") // 'shadow2D'     is actually 'texture'     on GL3
      +(shadow2DProj ? "#define shadow2DProj textureProj\n" : "") // 'shadow2DProj' is actually 'textureProj' on GL3
      +(rt0 ? "layout(location=0) out HP vec4 RT0;\n" : "")
      +(rt1 ? "layout(location=1) out HP vec4 RT1;\n" : "")
      +(rt2 ? "layout(location=2) out HP vec4 RT2;\n" : "")
      +(rt3 ? "layout(location=3) out HP vec4 RT3;\n" : "")
      +"#define varying in\n"
      +code;
}
static Str8 GetVarName(C Str8 &text, Int offset)
{
   Str8 name;
   for(Int i=offset; i<text.length(); i++)
   {
      Char8 c=text[i]; if(!c || c==',' || c==';' || c=='[' || c==' ' || c=='}')break; // "variable;", "var1, var2;", "var[100];", ..
      name+=c;
   }
   return name;
}
static void SpecifyCGPrecisionModifiers(Str8 &vs_code, Str8 &ps_code, Bool vs_hp, Bool ps_hp) // OpenGL ES will fail if vertex and pixel shaders both use the same uniforms but with different precision modifiers, that's why we need to make sure that if same uniform or class is used between shaders then they have the same precision specified
{
   Bool max_hp=(vs_hp || ps_hp); // maximum precision of shaders
   if(vs_hp!=ps_hp) // shaders operate on different precisions
      FREPD(s, 2) // shader (0=vertex, 1=pixel)
   {
      Str8 &code =(s ? ps_code : vs_code), // current code
           &code2=(s ? vs_code : ps_code); // other   code
      Bool  hp   =(s ? ps_hp   : vs_hp  ), // current precision
            hp2  =(s ? vs_hp   : ps_hp  ); // other   precision
      for(Int uniform_index=0; ; uniform_index++)
      {
         Int uniform_pos =TextPosIN(code, "uniform", uniform_index, true, true); if(uniform_pos<0)break; // find all "uniform" occurences (like "uniform float Var", "uniform MaterialClass Material")
         Int uniform_type=uniform_pos+8; // Length("uniform ")->8
         if(!Starts(code()+uniform_type, "HP"       , true, true)
         && !Starts(code()+uniform_type, "MP"       , true, true)  // if precision was not yet specified
         && !Starts(code()+uniform_type, "sampler1D", true, true)
         && !Starts(code()+uniform_type, "sampler2D", true, true)
         && !Starts(code()+uniform_type, "sampler3D", true, true)) // not a sampler
         {
            Int  uniform_name=TextPosI  (code()+uniform_type, ' '); if(uniform_name<0)continue; uniform_name+=1+uniform_type; // space + type offset
            Str8     var_name=GetVarName(code, uniform_name);
            if(Starts(code()+uniform_type, "float", true, true)
            || Starts(code()+uniform_type, "vec2" , true, true)
            || Starts(code()+uniform_type, "vec3" , true, true)
            || Starts(code()+uniform_type, "vec4" , true, true)) // basic type
            {
               Str8 var_def; for(Int i=uniform_pos; i<uniform_name+var_name.length(); i++)var_def+=code[i];
               Int  code2_var_def_pos=TextPosI(code2, var_def, true, true); // find "uniform type name" string in other shader
               if(  code2_var_def_pos>=0) // if other shader also uses this variable
               {
                  code .insert(uniform_type                                , max_hp ? "HP " : "MP "); // add precision modifier "uniform precision type name"
                  code2.insert(code2_var_def_pos+(uniform_type-uniform_pos), max_hp ? "HP " : "MP "); // add precision modifier "uniform precision type name"
               }
            }else
            {
               Str8 type_name=GetVarName(code, uniform_type), // struct, for example "MaterialClass"
                    type_def =S8+"struct "+type_name;
               Int  code2_struct_pos=TextPosI(code2, type_def, true, true); // find "struct MaterialClass" in other shader
               if(  code2_struct_pos>=0) // if other shader also uses this class
               {
                  Int  code2_struct_end=TextPosI(code2()+code2_struct_pos, '}'); if(code2_struct_end<0)continue; code2_struct_end+=code2_struct_pos;
                  Str8 struct_def; for(Int i=code2_struct_pos; i<=code2_struct_end; i++)struct_def+=code2[i]; // 'struct_def' now contains "struct MaterialClass{..}"
                  Int  code_struct_pos=TextPosI(code, struct_def, true, true); if(code_struct_pos<0)continue; // proceed only if first code has exact same struct definition
                  
                  Bool changed=false;
                  for(Int member_start=0; member_start<struct_def.length(); member_start++)
                     if(Starts(struct_def()+member_start, "\nfloat", true, true)
                     || Starts(struct_def()+member_start, "\nvec2" , true, true)
                     || Starts(struct_def()+member_start, "\nvec3" , true, true)
                     || Starts(struct_def()+member_start, "\nvec4" , true, true)) // basic type (include '\n' character to skip those with specified precision)
                  {
                     Int  member_name=       TextPosI  (struct_def()+member_start, ' '); if(member_name<0)continue; member_name+=1+member_start; // space + start offset
                     Str8    var_name=S8+'.'+GetVarName(struct_def, member_name); // ".member"
                     Bool used =Contains(code , var_name, true, true), // if used in       code
                          used2=Contains(code2, var_name, true, true); // if used in other code
                     Bool var_hp=((used && used2) ? (hp || hp2) // used by both shaders
                                 : used           ?  hp         // used by first
                                 :         used2  ?        hp2  // used by second
                                 : true);                       // used by none
                     struct_def.insert(member_start+1, var_hp ? "HP " : "MP "); // add precision modifier "\nprecision type name"
                     changed=true;
                  }
                  if(changed)
                  {
                     code .remove(code_struct_pos , code2_struct_end-code2_struct_pos+1).insert(code_struct_pos , struct_def);
                     code2.remove(code2_struct_pos, code2_struct_end-code2_struct_pos+1).insert(code2_struct_pos, struct_def);
                  }
               }
            }
         }
      }
   }
}
T1(TYPE) static Int StoreShader(C Str8 &code, Memc<TYPE> &shaders, File &src, File &temp)
{
   if(!code.is())Exit("Shader is empty"); // needed for null char
   src.reset().put(code(), code.length()+1); src.pos(0); // include null char
   if(!Compress(src, temp.reset(), COMPRESS_GL, COMPRESS_GL_LEVEL, COMPRESS_GL_MT))Exit("Can't compress shader");
   REPA(shaders)
   {
      ShaderData &shader_data=shaders[i];
      if(shader_data.elms()==temp.size())
      {
         File data; data.readMem(shader_data.data(), shader_data.elms());
         temp.pos(0); if(temp.equal(data))return i;
      }
   }
   ShaderData &data=shaders.New();
   temp.pos(0); temp.get(data.setNum(temp.size()).data(), temp.size());
   return shaders.elms()-1;
}
struct ParamMember
{
   Int          gpu_offset;
   Str8         name;
   ShaderParam *sp;

   void set(C Str8 &name, ShaderParam &sp, Int gpu_offset) {T.name=name; T.sp=&sp; T.gpu_offset=gpu_offset;}

   ParamMember() {sp=null; gpu_offset=0;}
};
static C ParamMember* Find(C Memc<ParamMember> &pm, CChar8 *name)
{
   REPA(pm)if(Equal(pm[i].name, name, true))return &pm[i];
   return null;
}
static Bool FindNameReplacement(Char8 (&temp)[1024], CChar8 *glsl_name, CChar8 *code)
{
   Set(temp, " :  : _"); Append(temp, glsl_name);
   if(CChar8 *t=TextPos(code, temp, true, true))for(CChar8 *name=t-1; name>code; name--)if(*name==' ')
   {
      Set(temp, name+1, t-name);
      return true;
   }
   return false;
}

#include <Cg/cg.h>
#include <Cg/cgGL.h>

static void AddTranslationGL(ShaderParam &sp, CGparameter par, Memc<ParamMember> &pm)
{
   CChar8 *pname=cgGetParameterName(par);

   if(cgGetArraySize(par, 0)<=1) // array size
   {
      CGparameterclass pclass=cgGetParameterClass        (par);
      CGtype           type  =cgGetParameterType         (par);
   #if 0
      Int              rsize =cgGetParameterResourceSize (par);
      CChar8          *sem   =cgGetParameterSemantic     (par);
      CGresource       bres  =cgGetParameterBaseResource (par);
      CGresource       res   =cgGetParameterResource     (par);
      CChar8          *rname =cgGetParameterResourceName (par);
      Int              resi  =cgGetParameterResourceIndex(par);
      CGtype           rtype =cgGetParameterResourceType (par);
      CGenum           var   =cgGetParameterVariability  (par);
      Int              bi    =cgGetParameterBufferIndex  (par);
      Int              bo    =cgGetParameterBufferOffset (par);
      Int              pi    =cgGetParameterIndex        (par);
   #endif

      if(pclass==CG_PARAMETERCLASS_SCALAR || pclass==CG_PARAMETERCLASS_VECTOR)
      {
         pm.New().set(pname, sp, sp._gpu_data_size);

         if(type==CG_FLOAT  || type==CG_HALF  || type==CG_FIXED ){sp._full_translation.New().set(sp._cpu_data_size, sp._gpu_data_size, SIZE(Flt )); sp._cpu_data_size+=SIZE(Flt ); sp._gpu_data_size+=SIZE(Flt );}else
         if(type==CG_FLOAT2 || type==CG_HALF2 || type==CG_FIXED2){sp._full_translation.New().set(sp._cpu_data_size, sp._gpu_data_size, SIZE(Vec2)); sp._cpu_data_size+=SIZE(Vec2); sp._gpu_data_size+=SIZE(Vec2);}else
         if(type==CG_FLOAT3 || type==CG_HALF3 || type==CG_FIXED3){sp._full_translation.New().set(sp._cpu_data_size, sp._gpu_data_size, SIZE(Vec )); sp._cpu_data_size+=SIZE(Vec ); sp._gpu_data_size+=SIZE(Vec );}else
         if(type==CG_FLOAT4 || type==CG_HALF4 || type==CG_FIXED4){sp._full_translation.New().set(sp._cpu_data_size, sp._gpu_data_size, SIZE(Vec4)); sp._cpu_data_size+=SIZE(Vec4); sp._gpu_data_size+=SIZE(Vec4);}else
            Exit(S+"Unhandled Shader Parameter Type for \""+pname+'"');
      }else
      if(pclass==CG_PARAMETERCLASS_MATRIX)
      {
         pm.New().set(pname, sp, sp._gpu_data_size);

         Int rows   =cgGetParameterRows   (par),
             columns=cgGetParameterColumns(par);

         FREPD(y, columns)
         FREPD(x, rows   )sp._full_translation.New().set(sp._cpu_data_size+SIZE(Flt)*(y+x*columns), sp._gpu_data_size+SIZE(Flt)*(x+y*rows), SIZE(Flt));
         sp._cpu_data_size+=SIZE(Flt)*rows*columns;
         sp._gpu_data_size+=SIZE(Flt)*rows*columns;
      }else
      if(pclass==CG_PARAMETERCLASS_STRUCT)
      {
         for(CGparameter member=cgGetFirstStructParameter(par); member; member=cgGetNextParameter(member))AddTranslationGL(sp, member, pm);
      }
   }else
   {
      FREP(cgGetArraySize(par, 0))
      {
         CGparameter elm=cgGetArrayParameter(par, i);
         AddTranslationGL(sp, elm, pm);
      }
   }
}
struct CGCONTEXT
{
   CGcontext _;

   operator CGcontext() {return _;}

  ~CGCONTEXT() {cgDestroyContext(_);}
   CGCONTEXT()
   {
      if(_=cgCreateContext())
      {
      #ifdef CG_BEHAVIOR_CURRENT
         cgSetContextBehavior(_, CG_BEHAVIOR_CURRENT);
      #endif
       //cgSetAutoCompile(_, CG_COMPILE_IMMEDIATE);
         cgGLRegisterStates(_);
      }
   }
   Str error()
   {
      CGerror error;
      CChar8 *string=cgGetLastErrorString(&error);
      switch(error)
      {
         case CG_NO_ERROR: break;
         //case CG_INVALID_PARAMETER_ERROR:
         //case CG_NON_NUMERIC_PARAMETER_ERROR: break;
      
         //case CG_COMPILER_ERROR: return S+string+'\n'+cgGetLastListing(T); break;

         default:
         {
            Str s=string; s.line()+=cgGetLastListing(T);
            if(string=(CChar8*)glGetString(GL_PROGRAM_ERROR_STRING_ARB))
            {
               int loc; glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &loc);
               s.line()+=string;
            }
            return s;
         }break;
      }
      return S;
   }
};
struct CGEFFECT
{
   CGeffect _;

   operator CGeffect() {return _;}

           ~CGEFFECT() {cgDestroyEffect(_);}
   explicit CGEFFECT(CGeffect fx) {_=fx;}
};
#endif
#define SHOW_GLSL_SRC 0
static Bool ShaderCompileGL(Str name, C Str &dest, C MemPtr<ShaderMacro> &macros, Str *messages, C MemPtr<ShaderGLSL> &stg)
{
#if GL && !GL_ES && WINDOWS
   Memc<Str8   > args_str; FREPA(  macros)args_str.add(S+"-D"+macros[i].name+'='+macros[i].definition);
   Memc<CChar8*> args    ; FREPA(args_str)args    .add(args_str[i]); args.add("-O3"); args.add((CChar8*)null);

   name=NormalizePath(name); if(!FExistSystem(name))name=DataPath()+name;
   Bool                      ok=false;
   Map <Str8, ShaderParamEx> params(CompareCS);
   Memc<ShaderImage*>        images;
   Memc<ShaderVSGL>          vs;
   Memc<ShaderPSGL>          ps;
   Memc<ShaderGLEx>          techs;
   CGCONTEXT context; if(context)
   {
      CGEFFECT effect(cgCreateEffectFromFile(context, UnixPathUTF8(name), args.data())); if(effect)
      {
         Memc<ParamMember> pm;

         // add parameters
         for(CGparameter par=cgGetFirstEffectParameter(effect); par; par=cgGetNextParameter(par))
         {
            CChar8          *pname =cgGetParameterName        (par);
            CGparameterclass pclass=cgGetParameterClass       (par);
          //Int              rsize =cgGetParameterResourceSize(par);
            if(pclass==CG_PARAMETERCLASS_SAMPLER)images.add(ShaderImages(Str8Temp(pname)));else
            if(pclass!=CG_PARAMETERCLASS_OBJECT )
            {
               ShaderParam &sp=*params(Str8Temp(pname));
               if(sp.is())Exit(S+"Shader parameter \""+pname+"\" listed more than once");else // if wasn't yet created
               {
                  sp._owns_data=true;
                  sp._elements =cgGetArraySize(par, 0);

                  AddTranslationGL(sp, par, pm);
                  sp.optimize(); // required for setting default value

                  MAX(sp._gpu_data_size, SIZEI(Vec4)); // in OpenGL, parameters must be stored without padding
                //if (sp._gpu_data_size!=rsize    )Exit("Incorrect Shader Param Size.\nPlease contact Developer.");
                  if (sp._gpu_data_size<SIZE(Vec4))Exit("Shader Param Size < SIZE(Vec4)"); // some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size

                  // alloc data
                  AllocZero(sp._data, sp._gpu_data_size);
                 *Alloc    (sp._changed)=true;

                  // set default value
                  Flt temp[4*1024]; if(Int elms=cgGetParameterDefaultValuefc(par, Elms(temp), temp))sp.set(Ptr(temp), elms*SIZE(Flt));
               }
            }
         }
         images.sort(Compare); // once we have all images for this file, sort them, so we can use binary search later while saving techniques when looking for image indexes

         Str8 vs_code, ps_code, glsl_params;
         File src, temp; src.writeMem(); temp.writeMem();

         // techniques
         Int tech_num=0; for(CGtechnique technique=cgGetFirstTechnique(effect); technique; technique=cgGetNextTechnique(technique), tech_num++); // get number of all techniques
         Int t       =1; for(CGtechnique technique=cgGetFirstTechnique(effect); technique; technique=cgGetNextTechnique(technique), t++       )
         {
            CChar8   *tech_name=cgGetTechniqueName(technique);
            ShaderGL &tech=techs.New(); tech.name=tech_name;
         #if DEBUG
            LogN(S+"Processing tech \""+tech_name+"\" "+t+'/'+tech_num);
         #endif

            // try to find GLSL replacement
            Bool cg=true;
            REPA(stg)if(Equal(tech.name, stg[i].tech_name, true))
            {
             C ShaderGLSL &st=stg[i];
               Str file; FileText f; if(f.read(name))for(; !f.end(); )file+=f.getLine()+'\n';
                   file  =StrInside(file, S+"@GROUP \""+st.group_name+'"', "@GROUP_END" , true, true); if(!file  .is())Exit(S+"@GROUP \""+st.group_name+"\" @GROUP_END not defined");
               Str shared=StrInside(file,   "@SHARED"                    , "@SHARED_END", true, true); if(!shared.is())Exit(  "@SHARED @SHARED_END not defined"                    ); shared=       StrInclude(shared, name);
               Str vs    =StrInside(file,   "@VS"                        , "@VS_END"    , true, true); if(!vs    .is())Exit(  "@VS @VS_END not defined"                            ); vs    =shared+StrInclude(vs    , name);
               Str ps    =StrInside(file,   "@PS"                        , "@PS_END"    , true, true); if(!ps    .is())Exit(  "@PS @PS_END not defined"                            ); ps    =shared+StrInclude(ps    , name);
               vs=Clean(vs);
               ps=Clean(ps);
               REPA(ee_glsl)
               {
                  vs=Replace(vs, ee_glsl[i].from, ee_glsl[i].to, true, true);
                  ps=Replace(ps, ee_glsl[i].from, ee_glsl[i].to, true, true);
               }
               glsl_params.clear(); FREPA(st.params)glsl_params+=S+"#define "+st.params[i].name+' '+st.params[i].definition+'\n';
               cg=false;
               vs_code=GLSLVSShader(glsl_params+vs);
               ps_code=GLSLPSShader(glsl_params+ps, false); // default hand written GLSL shaders to low precision
               break;
            }

            CChar8 *vs_cg_code=null, *ps_cg_code=null;
            if(cg) // if haven't found GLSL replacement, then use CG version
            {
               CGpass    pass =cgGetFirstPass  (technique);
               CGprogram cg_vs=cgGetPassProgram(pass, CG_VERTEX_DOMAIN  );
               CGprogram cg_ps=cgGetPassProgram(pass, CG_FRAGMENT_DOMAIN);
               vs_cg_code=cgGetProgramString(cg_vs, CG_COMPILED_PROGRAM); if(!Is(vs_cg_code)){if(messages)messages->line()+=S+"Empty Vertex Shader Code in Technique \""+tech_name+"\""+context.error(); return false;}
               ps_cg_code=cgGetProgramString(cg_ps, CG_COMPILED_PROGRAM); if(!Is(ps_cg_code)){if(messages)messages->line()+=S+"Empty Pixel Shader Code in Technique \"" +tech_name+"\""+context.error(); return false;}

               Bool ps_hp=true;

               vs_code=GLSLVSShader(CleanCGShader(vs_cg_code));
               ps_code=GLSLPSShader(CleanCGShader(ps_cg_code), ps_hp);

               // specify precision modifiers
               SpecifyCGPrecisionModifiers(vs_code, ps_code, true, ps_hp);
            }

         #if DEBUG
            if(SHOW_GLSL_SRC)LogN(S+"// VERTEX SHADER\n\n"+vs_code+"\n\n// PIXEL SHADER\n\n"+ps_code);
         #endif

            // get shader data
            tech.vs_index=StoreShader(vs_code, vs, src, temp);
            tech.ps_index=StoreShader(ps_code, ps, src, temp);

            // get GLSL->ShaderParam names
            Str glsl_messages;
            tech.compile(vs, ps, &glsl_messages); // compilation is required in order to get list of used parameters and their corresponding names and register address, don't clean the shader codes because they'll be needed for saving
            if(!tech.prog)Exit(S+"Error compiling GLSL program:\n"+glsl_messages);
            Int  program_params=0; glGetProgramiv(tech.prog, GL_ACTIVE_UNIFORMS, &program_params);
            FREP(program_params)
            {
               Char8  glsl_name[1024], temp_name[1024]; glsl_name[0]=0;
               Int    size=0;
               GLenum type;
               glGetActiveUniform(tech.prog, i, Elms(glsl_name), null, &size, &type, glsl_name);

               Bool ok=false;
               if(type==GL_SAMPLER_2D || type==GL_SAMPLER_3D || type==GL_SAMPLER_CUBE || type==GL_SAMPLER_2D_SHADOW) // Image
               {
                  REPA(images)if(Equal(ShaderImages.dataInMapToKey(*images[i]), glsl_name, true))
                  {
                   //tech.glsl_images.add(images[i]);
                     ok=true;
                     break;
                  }
               }else // ShaderParam
               {
                C ParamMember *p=Find(pm, glsl_name);
                  if(!p)
                  {
                     if(cg)
                     {
                        if(FindNameReplacement(temp_name, glsl_name, vs_cg_code)
                        || FindNameReplacement(temp_name, glsl_name, ps_cg_code))p=Find(pm, temp_name);
                     }else
                     if(Ends(glsl_name, "[0]"))
                     {
                        Set(temp_name, glsl_name, Length(glsl_name)-2);
                        p=Find(pm, temp_name);
                     }
                  }
                  if(p)
                  {
                     tech.glsl_params.New().set(p->gpu_offset, *p->sp, Equal(glsl_name, ShaderParams.dataInMapToKey(*p->sp), true) ? null : glsl_name); // if 'glsl_name' is exactly the same as ShaderParam name, then skip it so we don't have to save it. in other cases we need to store the 'glsl_name' because it can contain "obj.member" things, which we can't detect
                     ok=true;
                  }
               }
               if(!ok)
               {
               #if DEBUG
                  Log(vs_code);
                  Log(ps_code);
               #endif
                  Exit(S+"GLSL Parameter \""+glsl_name+"\" not found");
               }
            }

            // free
            if(tech.prog){SyncLocker locker(D._lock); glDeleteProgram(tech.prog); tech.prog=0;} // clear while in lock
         }
         ok=true;
      }else if(messages)messages->line()+=S+"Can't create CG effect: "+context.error();
   }else if(messages)messages->line()+="Can't create CG context";
   if(ok)
   {
      // sort any verify
      techs.sort(Compare);
      REPA(stg)if(!techs.binaryHas(stg[i].tech_name, Compare))Exit(S+"Shader doesn't have "+stg[i].tech_name);

      // save
      return ShaderSave(dest, params, images, vs, ps, techs);
   }
#endif
   return false;
}
/******************************************************************************/
Bool ShaderCompileTry(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros, C MemPtr<ShaderGLSL> &stg, Str *messages)
{
   Memc<ShaderMacro> temp; temp=macros;
   switch(api)
   {
      default: return false;
      case API_DX: temp.New().set("DX", "1"); temp.New().set("GL", "0");                            return ShaderCompile11(src, dest, temp, messages);
      case API_GL: temp.New().set("DX", "0"); temp.New().set("GL", "1"); temp.New().set("CG", "1"); return ShaderCompileGL(src, dest, temp, messages, stg);
   }
}
/******************************************************************************/
void ShaderCompile(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros, C MemPtr<ShaderGLSL> &stg)
{
   Str messages;
   if(!ShaderCompileTry(src, dest, api, model, macros, stg, &messages))
   {
   #if !DX11
      if(api==API_DX)Exit("Can't compile DX10+ Shaders when not using DX10+ engine version");
   #endif
   #if !GL
      if(api==API_GL)Exit("Can't compile OpenGL Shaders when not using OpenGL engine version");
   #endif
      Exit(S+"Error compiling shader\n\""+src+"\"\nto file\n\""+dest+"\"."+(messages.is() ? S+"\n\nCompilation Messages:\n"+messages : S));
   }
}
Bool ShaderCompileTry(C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros, Str *messages) {return ShaderCompileTry(src, dest, api, model, macros, null, messages);}
void ShaderCompile   (C Str &src, C Str &dest, API api, SHADER_MODEL model, C MemPtr<ShaderMacro> &macros               ) {       ShaderCompile   (src, dest, api, model, macros, null          );}
/******************************************************************************/
#endif
}
/******************************************************************************/
