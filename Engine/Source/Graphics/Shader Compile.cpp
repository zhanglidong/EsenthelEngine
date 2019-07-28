/******************************************************************************/
#include "stdafx.h"

#define NEW_COMPILER 1
#define SPIRV_CROSS  1
#define HLSL_CC      0

#define FORCE_LOG_SHADER_CODE (DEBUG && 0)

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
static const CChar8 *APIName[]=
{
   "DX"    , // 0
   "GL"    , // 1
   "VULKAN", // 2
   "METAL" , // 3
}; ASSERT(API_DX==0 && API_GL==1 && API_VULKAN==2 && API_METAL==3 && API_NUM==4);
static const CChar8 *ShaderTypeName[]=
{
   "VS", // 0
   "HS", // 1
   "DS", // 2
   "PS", // 3
}; ASSERT(ST_VS==0 && ST_HS==1 && ST_DS==2 && ST_PS==3 && ST_NUM==4);
/******************************************************************************/
#if   1 // with new shader compilers, the generated shaders are small, so disable compression to get best performance
   #define COMPRESS_GL       COMPRESS_NONE
   #define COMPRESS_GL_LEVEL 0
#elif 0
   #define COMPRESS_GL       COMPRESS_LZ4
   #define COMPRESS_GL_LEVEL 99
#elif 0
   #define COMPRESS_GL       COMPRESS_ZSTD
   #define COMPRESS_GL_LEVEL 99
#elif 1
   #define COMPRESS_GL       COMPRESS_LZMA
   #define COMPRESS_GL_LEVEL 9 // shader files are small, so we can use high compression level and still get small dictionary size / memory usage
#else // shader size was slightly bigger than LZMA, and loading all shaders was bit slower
   #define COMPRESS_GL       COMPRESS_LZHAM
   #define COMPRESS_GL_LEVEL 5
#endif
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
      // save translations only for the first element, remaining elements will be reconstructed based on the first element translation and offets between elements
      if(translation.elms()%elms)Exit("ShaderParam.Translation mod");
      UShort single_translations=translation.elms()/elms,
         gpu_offset=translation[single_translations].gpu_offset-translation[0].gpu_offset, // gpu offset between the next array element and previous
         cpu_offset=translation[single_translations].cpu_offset-translation[0].cpu_offset; // cpu offset between the next array element and previous
      f.putMulti(gpu_offset, cpu_offset, single_translations);
      FREP(single_translations)f<<translation[i]; // save 1st element translation

   #if DEBUG // verify that all elements have same translation
      for(Int e=1, co=0, go=0, t=single_translations; e<elms; e++) // add rest of the elements
      {
         co+=cpu_offset; // offset between elements
         go+=gpu_offset; // offset between elements
         FREP(single_translations)
         {
          C ShaderParam::Translation &first=translation[i], &trans=translation[t++];
            DYNAMIC_ASSERT(first.cpu_offset+co==trans.cpu_offset
                        && first.gpu_offset+go==trans.gpu_offset
                        && first.elm_size     ==trans.elm_size  , "ShaderParam has irregular translation");
         }
      }
   #endif
   }
}
static void LoadTranslation(MemPtr<ShaderParam::Translation> translation, File &f, Int elms)
{
   if(elms<=1)translation.loadRaw(f);else
   {
      UShort single_translations, gpu_offset, cpu_offset; f.getMulti(gpu_offset, cpu_offset, single_translations);
      translation.setNum(single_translations*elms);
      Int t=0; FREPS(t, single_translations)f>>translation[t]; // load 1st element translation
      for(Int e=1, co=0, go=0; e<elms; e++) // add rest of the elements
      {
         co+=cpu_offset; // offset between elements
         go+=gpu_offset; // offset between elements
         FREP(single_translations)
         {
            ShaderParam::Translation &trans=translation[t++];
            trans=translation[i];
            trans.cpu_offset+=co;
            trans.gpu_offset+=go;
         }
      }
   }
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
void ShaderCompiler::Param::addTranslation(ID3D11ShaderReflectionType *type, C D3D11_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16) // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
{
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
   if(type_desc.Elements)offset=Ceil16(offset); // arrays are 16-byte aligned (even 1-element arrays "f[1]"), non-arrays have Elements=0, so check Elements!=0
   Int  elms=Max(type_desc.Elements, 1), last_index=elms-1; // 'Elements' is array size (it's 0 for non-arrays)
   FREP(elms)switch(type_desc.Class)
   {
      case D3D_SVC_SCALAR        : // for example: Flt f,f[];
      case D3D_SVC_VECTOR        : // for example: Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
      case D3D_SVC_MATRIX_COLUMNS: // for example: Matrix m,m[];
      {
         if(type_desc.Rows   <=0 || type_desc.Rows   >4)Exit("Invalid Shader Param Rows");
         if(type_desc.Columns<=0 || type_desc.Columns>4)Exit("Invalid Shader Param Cols");
         Bool min16=(type_desc.Type==D3D_SVT_MIN16FLOAT);
         if(type_desc.Type==D3D_SVT_FLOAT || min16)
         {
            Int base_size=SIZE(Flt),
                 cpu_size=base_size*type_desc.Columns*type_desc.Rows,
                 gpu_size;
            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)gpu_size=cpu_size;
            else                                       gpu_size=base_size*4             *(type_desc.Columns-1) // for matrixes, the lines use  4 X's
                                                               +base_size*type_desc.Rows;                      //       except last line  uses what was specified
            if(offset/16 != (offset+gpu_size-1)/16 || (was_min16>=0 && was_min16!=(Byte)min16))offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."

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
void ShaderCompiler::Param::addTranslation(ID3D12ShaderReflectionType *type, C D3D12_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, Bool true_half)
{
   if(type_desc.Elements)offset=Ceil16(offset); // arrays are 16-byte aligned (even 1-element arrays "f[1]"), non-arrays have Elements=0, so check Elements!=0
   Int  elms=Max(type_desc.Elements, 1), last_index=elms-1; // 'Elements' is array size (it's 0 for non-arrays)
   FREP(elms)switch(type_desc.Class)
   {
      case D3D_SVC_SCALAR        : // for example: Flt f,f[];
      case D3D_SVC_VECTOR        : // for example: Vec2 v,v[]; Vec v,v[]; Vec4 v,v[];
      case D3D_SVC_MATRIX_COLUMNS: // for example: Matrix m,m[];
      {
         if(type_desc.Rows   <=0 || type_desc.Rows   >4)Exit("Invalid Shader Param Rows");
         if(type_desc.Columns<=0 || type_desc.Columns>4)Exit("Invalid Shader Param Cols");
         if(type_desc.Type==D3D_SVT_FLOAT || type_desc.Type==D3D_SVT_MIN16FLOAT)
         {
            Bool half=(type_desc.Type==D3D_SVT_MIN16FLOAT && true_half);
            Int base_size=((type_desc.Type==D3D_SVT_DOUBLE) ? SIZE(Dbl) : half ? SIZE(Half) : SIZE(Flt)),
                  up_size=Max(base_size, SIZE(Flt)), // some half-sizes are upped to flt
                 cpu_size=base_size*type_desc.Columns*type_desc.Rows,
                 gpu_size;
            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)gpu_size=cpu_size;
            else                                       gpu_size= up_size*          4   *(type_desc.Columns-1) // for matrixes, the lines use  4 X's
                                                               + up_size*type_desc.Rows                     ; //       except last line  uses what was specified
            if(!half)
            {
                                                 cpu_data_size=Ceil4(cpu_data_size); // float's are 4-byte aligned on CPU, double too if using #pragma pack(4)
               if(type_desc.Type==D3D_SVT_DOUBLE)offset       =Ceil8(offset       ); // on GPU Dbl is 8-byte aligned
               else                              offset       =Ceil4(offset       ); // on GPU Flt is 4-byte aligned
            }
            if(offset/16 != (offset+gpu_size-1)/16)offset=Ceil16(offset); // "Additionally, HLSL packs data so that it does not cross a 16-byte boundary."

            if(type_desc.Class!=D3D_SVC_MATRIX_COLUMNS)translation.New().set(cpu_data_size, offset, cpu_size);else
            { // !! process y's first to add translation sorted by 'cpu_offset' to make later sorting faster !!
               FREPD(y, type_desc.Rows   )
               FREPD(x, type_desc.Columns)translation.New().set(cpu_data_size+base_size*(x+y*type_desc.Columns), offset+base_size*y+x*up_size*4, base_size);
            }
                  
            cpu_data_size+=cpu_size;
                   offset+=((i==last_index) ? gpu_size : Ceil16(gpu_size)); // arrays are 16-byte aligned, and last element is 'gpu_size' only
         }else Exit(S+"Unhandled Shader Parameter Type for \""+name+'"');
      }break;

      case D3D_SVC_STRUCT:
      {
         offset=Ceil16(offset); // "Each structure forces the next variable to start on the next four-component vector."
         FREP(type_desc.Members) // iterate all struct members
         {
            ID3D12ShaderReflectionType *member=type->GetMemberTypeByIndex(i); if(!member)Exit("'GetMemberTypeByIndex' failed");
            D3D12_SHADER_TYPE_DESC member_desc; if(!OK(member->GetDesc(&member_desc)))Exit("'ID3D12ShaderReflectionType.GetDesc' failed");
            addTranslation(member, member_desc, type->GetMemberTypeName(i), offset, true_half);
         }
       //offset=Ceil16(offset); "Each structure forces the next variable to start on the next four-component vector." even though documentation examples indicate this should align too, actual tests confirm that's not the case
      }break;
   }
}
#if SPIRV_CROSS
void ShaderCompiler::Param::addTranslation(spvc_compiler compiler, spvc_type_id parent_id, spvc_type parent, spvc_type_id var_id, spvc_type var, Int var_i, Int offset, C Str8 &names)
{
   auto          name            =spvc_compiler_get_member_name(compiler, parent_id, var_i);
   auto          array_dimensions=spvc_type_get_num_array_dimensions(var); if(array_dimensions>1)Exit("Multi-dimensional arrays are not supported");
   auto          array_elms      =(array_dimensions ? spvc_type_get_array_dimension(var, 0) : 0); // use 0 for non-arrays to match DX behavior
   unsigned      array_stride    =0; if(array_elms>1)spvc_compiler_type_struct_member_array_stride(compiler, parent, var_i, &array_stride);
   spvc_basetype type            =spvc_type_get_basetype(var);
   Bool          is_struct       =(type==SPVC_BASETYPE_STRUCT); Str8 names_name; if(is_struct){names_name.reserve(names.length()+1+Length(name)); names_name=names; if(names_name.is())names_name+='.'; names_name+=name;}
   Int  elms=Max(array_elms, 1), last_index=elms-1; // 'array_elms' is 0 for non-arrays
   FREP(elms)
   {
      if(!is_struct)
      {
         if(type!=SPVC_BASETYPE_FP32)Exit(S+"Unhandled Shader Parameter Type for \""+names+'.'+name+'"');
         auto vec_size=spvc_type_get_vector_size(var),
              cols    =spvc_type_get_columns    (var);
         if(vec_size<=0 || vec_size>4)Exit("Invalid Shader Param Vector Size");
         if(cols    <=0 || cols    >4)Exit("Invalid Shader Param Columns");
         Int base_size=SIZE(Flt),
              cpu_size=base_size*cols*vec_size;
         if(cols>1) // matrix
         {
            unsigned matrix_stride=0; spvc_compiler_type_struct_member_matrix_stride(compiler, parent, var_i, &matrix_stride);
            FREPD(y, cols    )
            FREPD(x, vec_size)translation.New().set(cpu_data_size + base_size*(x+y*vec_size), offset + base_size*y + x*matrix_stride, base_size);
         }else // scalar, vector
         {
            translation.New().set(cpu_data_size, offset, cpu_size);
         }
         cpu_data_size+=cpu_size;
      }else
      {
         auto members=spvc_type_get_num_member_types(var);
         FREP(members)
         {
            auto member=spvc_type_get_member_type(var, i);
            auto member_handle=spvc_compiler_get_type_handle(compiler, member);
            unsigned member_offset=0; spvc_compiler_type_struct_member_offset(compiler, var, i, &member_offset);
            addTranslation(compiler, var_id, var, member, member_handle, i, offset+member_offset, names_name);
         }
      }
      offset+=array_stride;
   }
}
#endif
#endif
static Int Compare(C ShaderParam::Translation &a, C ShaderParam::Translation &b) {return Compare(a.cpu_offset, b.cpu_offset);}
void ShaderCompiler::Param::sortTranslation() {translation.sort(Compare);} // this is useful for Translation optimization
static void AssertRange(Int offset, Int size, Int total_size)
{
   DYNAMIC_ASSERT(InRange(offset, total_size) && offset+size<=total_size, "Element out of range");
}
void ShaderCompiler::Param::setDataFrom(C Param &src)
{
   DYNAMIC_ASSERT(cpu_data_size==src.cpu_data_size, "Parameters have different size");
   if(src.data.elms())
   {
      DYNAMIC_ASSERT(src.translation.elms() && translation.elms(), "Parameters have no translation");
      Memt<Byte> cpu_data; cpu_data.setNumZero(src.cpu_data_size);
      Int start=src.translation[0].gpu_offset;
      FREPA(src.translation)
      {
       C ShaderParam::Translation &trans=src.translation[i]; Int gpu_offset=trans.gpu_offset-start;
         AssertRange(trans.cpu_offset, trans.elm_size, cpu_data.elms());
         AssertRange(      gpu_offset, trans.elm_size, src.data.elms());
         CopyFast(cpu_data.data()+trans.cpu_offset, src.data.data()+gpu_offset, trans.elm_size);
      }
      data.setNumZero(gpu_data_size);
      start=translation[0].gpu_offset;
      FREPA(translation)
      {
       C ShaderParam::Translation &trans=translation[i]; Int gpu_offset=trans.gpu_offset-start;
         AssertRange(trans.cpu_offset, trans.elm_size, cpu_data.elms());
         AssertRange(      gpu_offset, trans.elm_size,     data.elms());
         CopyFast(data.data()+gpu_offset, cpu_data.data()+trans.cpu_offset, trans.elm_size);
      }
   }
}
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
ShaderCompiler::Param* ShaderCompiler::Buffer::findParam(C Str8 &name)
{
   REPA(params)if(Equal(params[i].name, name, true))return &params[i];
   return null;
}
ShaderCompiler::Param& ShaderCompiler::Buffer::getParam(C Str8 &name)
{
   Param  *param=findParam(name); if(!param)Exit(S+"Param \""+name+"\" not found");
   return *param;
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
   if(newCompiler())
   {
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
      Exit("New Compiler unavailable");
      return false;
   #endif
   }else
   {
      DYNAMIC_ASSERT(encoding==ANSI, "File expected to be in ANSI encoding for performance reasons");
      file_data.setNum(f.left()); if(!f.getFast(file_data.data(), file_data.elms()))return false;
   }
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
         if(in.name!="SV_SampleIndex" && in.name!="SV_IsFrontFace")
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
REPD(get_default_val, (compiler->api!=API_DX) ? 2 : 1) // non-DX shaders have to process 2 times, first to extract default values, then to get actual results
{
   SHADER_MODEL model=shader->model; if(type==ST_HS || type==ST_DS)MAX(model, SM_5); // HS DS are supported only in SM5+
   if(!get_default_val && compiler->api!=API_DX)MAX(model, SM_6); // need to use new compiler for DX-> conversions
   switch(model)
   {
      case SM_4  : target[3]='4'; target[5]='0'; break;
      case SM_4_1: target[3]='4'; target[5]='1'; break;
      case SM_5  : target[3]='5'; target[5]='0'; break;
      case SM_6  : target[3]='6'; target[5]='0'; break;
      case SM_6_2: target[3]='6'; target[5]='2'; break; // required for Half
      default    : Exit("Invalid Shader Model"); break;
   }

   if(model>=SM_6)
   {
   #if NEW_COMPILER
      Int params=shader->params.elms()+shader->extra_params.elms();
      MemtN<DxcDefine, 64  > defines; defines.setNum(params  +API_NUM); Int defs =0;
      MemtN<Str      , 64*2> temp   ; temp   .setNum(params*2+API_NUM); Int temps=0;
      FREPA(shader->params)
      {
         DxcDefine &define=defines[defs++]; C TextParam8 &param=shader->params[i];
         define.Name =(temp[temps++]=param.name );
         define.Value=(temp[temps++]=param.value);
      }
      FREPA(shader->extra_params)
      {
         DxcDefine &define=defines[defs++]; C TextParam8 &param=shader->extra_params[i];
         define.Name =(temp[temps++]=param.name );
         define.Value=(temp[temps++]=param.value);
      }
      FREP(API_NUM)
      {
         DxcDefine &define=defines[defs++];
         define.Name =(temp[temps++]=APIName[i]);
         define.Value=((compiler->api==i) ? L"1" : L"0");
      }
      if(get_default_val)
      {
         DxcDefine &define=defines.New();
         define.Name =L"GET_DEFAULT_VALUE";
         define.Value=L"1";
      }

      MemtN<LPCWSTR, 16> arguments;
      arguments.add(L"-flegacy-macro-expansion"); // without this have to use "#define CONCAT(a,b) a##b"
      arguments.add(L"-flegacy-resource-reservation"); // will prevent other cbuffers from reusing indexes from explicit buffers
      arguments.add(L"/Zpc"); // D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR
      if(model>=SM_6_2)arguments.add(L"-enable-16bit-types");
      //D3DCOMPILE_IEEE_STRICTNESS     arguments.add(L"/Gis");
      //D3DCOMPILE_RESOURCES_MAY_ALIAS arguments.add(L"/res_may_alias");
      if(get_default_val)
      {
         arguments.add(L"/VD"); // skip validation
         arguments.add(L"/Od"); // skip optimizations
      }else
      {
         arguments.add(L"/O3");
         if(compiler->api!=API_DX)arguments.add(L"-spirv");
      }

      IDxcBlob *buffer=null; IDxcBlobEncoding *error_blob=null;
      IDxcCompiler *dxc_compiler=null; CreateCompiler(&dxc_compiler); if(dxc_compiler)
      {
         IDxcOperationResult *op_result=null;
         dxc_compiler->Compile(source->file_blob, source->file_name, (Str)func_name, (Str)target, arguments.data(), arguments.elms(), defines.data(), defines.elms(), &Include12(source->file_name), &op_result);
         if(op_result)
         {
            op_result->GetErrorBuffer(&error_blob);
            HRESULT hr; if(OK(op_result->GetStatus(&hr)))if(OK(hr))op_result->GetResult(&buffer);
            op_result->Release();
         }
         dxc_compiler->Release();
      }
      if(error_blob)
      {
         BOOL known=false; UINT32 code_page=0; error_blob->GetEncoding(&known, &code_page);
         CPtr data=error_blob->GetBufferPointer();
         Int  size=error_blob->GetBufferSize   ();
         switch(code_page)
         {
            default      : {Memt<wchar_t> w  ; w  .setNum(size+1); Int out=MultiByteToWideChar(code_page, 0, (LPCCH)data, size, w.data(), size); w   [out]='\0'; error+=           w.data() ;} break;
            case CP_UTF8 : {Memt<Char8  > utf; utf.setNum(size+1); CopyFast(utf.data(), data, size);                                             utf[size]='\0'; error+=FromUTF8(utf.data());} break;
          //case CP_ACP  : {CChar8 *e=(Char8*)data;                   error.reserveAdd(size); REP(size)error+=*e++;} break;
            case CP_UTF16: {CChar  *e=(Char* )data; size/=SIZE(Char); error.reserveAdd(size); REP(size)error+=*e++;} break;
         }
         error_blob->Release();
      }
      if(buffer)
      {
         if(!get_default_val && compiler->api!=API_DX)
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
                           D3D12_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error_new;}
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
                           D3D12_SHADER_INPUT_BIND_DESC desc; if(!OK(reflection->GetResourceBindingDesc(i, &desc))){error.line()+="'GetResourceBindingDesc' failed."; goto error_new;}
                           switch(desc.Type)
                           {
                              case D3D_SIT_TEXTURE:
                              {
                                 if(!InRange(desc.BindPoint, MAX_SHADER_IMAGES)){error.line()+=S+"Image index: "+desc.BindPoint+", is too big"; goto error_new;}
                                 Image &image=images[images_elms++]; image.name=desc.Name; image.bind_slot=desc.BindPoint;
                              }break;

                              case D3D_SIT_CBUFFER:
                              {
                                 if(!InRange(desc.BindPoint, MAX_SHADER_BUFFERS)){error.line()+=S+"Constant Buffer index: "+desc.BindPoint+", is too big"; goto error_new;}
                                 Buffer &buffer=buffers[buffers_elms++];
                                 buffer.name=desc.Name;
                                 buffer.bind_slot=desc.BindPoint;
                                 buffer.bind_explicit=FlagTest(desc.uFlags, D3D_SIF_USERPACKED); // FIXME this is not set https://github.com/microsoft/DirectXShaderCompiler/issues/2356
                                 ID3D12ShaderReflectionConstantBuffer *cb=reflection->GetConstantBufferByName(desc.Name); if(!cb){error.line()+="'GetConstantBufferByIndex' failed."; goto error_new;}
                                 {
                                    D3D12_SHADER_BUFFER_DESC desc; if(!OK(cb->GetDesc(&desc))){error.line()+="'ID3D12ShaderReflectionConstantBuffer.GetDesc' failed."; goto error_new;}
                                    buffer.size=desc.Size;
                                    buffer.params.setNum(desc.Variables);
                                    FREP(desc.Variables)
                                    {
                                       ID3D12ShaderReflectionVariable *var=cb->GetVariableByIndex(i); if(!var){error.line()+="'GetVariableByIndex' failed."; goto error_new;}
                                       ID3D12ShaderReflectionType *type=var->GetType(); if(!type){error.line()+="'GetType' failed."; goto error_new;}
                                       D3D12_SHADER_VARIABLE_DESC var_desc; if(!OK( var->GetDesc(& var_desc))){error.line()+="'ID3D12ShaderReflectionVariable.GetDesc' failed."; goto error_new;}
                                       D3D12_SHADER_TYPE_DESC    type_desc; if(!OK(type->GetDesc(&type_desc))){error.line()+="'ID3D12ShaderReflectionType.GetDesc' failed."; goto error_new;}

                                       Param &param=buffer.params[i];
                                       param.name=var_desc.Name;
                                       param.array_elms=type_desc.Elements;
                                       Int offset=var_desc.StartOffset; param.addTranslation(type, type_desc, var_desc.Name, offset, model>=SM_6_2); param.sortTranslation();
                                       param.gpu_data_size=offset-var_desc.StartOffset;
                                       if(!param.translation.elms()                             )Exit("Shader Param is empty.\nPlease contact Developer.");
                                       if(        param.gpu_data_size >        var_desc.Size
                                       ||  Ceil16(param.gpu_data_size)!=Ceil16(var_desc.Size)   )Exit("Incorrect Shader Param size.\nPlease contact Developer."); // FIXME DX Compiler returns padded struct sizes, so it can be a little bigger than actually used 'gpu_data_size' - https://github.com/microsoft/DirectXShaderCompiler/issues/2376
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
                        if(get_default_val)images.del(); // we don't need images for default value, only buffers
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
                              // FIXME - https://github.com/microsoft/DirectXShaderCompiler/issues/2374
                              //ID3DBlob *stripped=null; D3DStripShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), ~0, &stripped);
                              //if(stripped){buffer->Release(); buffer=stripped;}
                           }
                           if(!get_default_val)shader_data.setNum(buffer->GetBufferSize()).copyFrom((Byte*)buffer->GetBufferPointer());
                        }

                        result=GOOD;
                        // !! do not make any changes here after setting 'result' because other threads may access this data !!
                     }
                  error_new:
                     reflection->Release();
                  }
               }
               container_reflection->Release();
            }
         }
         buffer->Release();
      }
   #else
      error+="New Compiler not available";
   #endif
   }else
   {
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
      D3DCompile(source->file_data.data(), source->file_data.elms(), (Str8)source->file_name, macros.data(), &Include11(source->file_name), func_name, target, get_default_val ? D3DCOMPILE_SKIP_VALIDATION|D3DCOMPILE_SKIP_OPTIMIZATION : D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &buffer, &error_blob);
      if(error_blob)
      {
         CChar8 *e=(Char8*)error_blob->GetBufferPointer();
         Int     l=        error_blob->GetBufferSize   ();
         error.reserveAdd(l); REP(l)error+=*e++;
         error_blob->Release();
      }
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
               if(get_default_val)images.del(); // we don't need images for default value, only buffers
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
                  if(!get_default_val)shader_data.setNum(buffer->GetBufferSize()).copyFrom((Byte*)buffer->GetBufferPointer());
               }

               result=GOOD;
               // !! do not make any changes here after setting 'result' because other threads may access this data !!
            }
         error:
            reflection->Release();
         }
         buffer->Release();
      }
   }
   if(get_default_val){if(result!=GOOD)break; error.clear(); result=NONE;}
}
#else
   error+="Compiler not available";
#endif
   if(result==GOOD)
   {
      // verify that stages have matching output->input
      for(Int i=type;         --i>=0      ; ){C SubShader &prev=shader->sub[i]; if(prev.is()){if(prev.result==GOOD && !Match(prev, T, error))result=FAIL; break;}} // can check only if other shader also completed successfully, stop on first valid sub shader
      for(Int i=type; InRange(++i, ST_NUM); ){C SubShader &next=shader->sub[i]; if(next.is()){if(next.result==GOOD && !Match(T, next, error))result=FAIL; break;}} // can check only if other shader also completed successfully, stop on first valid sub shader
   }
   if(result!=GOOD)
   {
      result=FAIL;
      Exit(S+"Compiling \""+shader->name+"\" in \""+source->file_name+"\" failed:\n"+error);
   }
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

static UShort AsUShort(Int i) {RANGE_ASSERT_ERROR(i, USHORT_MAX+1, "Value too big to be represented as UShort"); return i;}

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
//static Bool Create (ShaderCompiler::Buffer* &buffer, C Str8 &name, Ptr user) {buffer=null; return true;}
static Int  Compare(ShaderCompiler::Shader*C &a, ShaderCompiler::Shader*C &b) {return CompareCS(a->name, b->name);}
   
static Int ExpectedBufferSlot(C Str8 &name)
{
   if(name=="Global"   )return SBI_GLOBAL;
   if(name=="ObjMatrix")return SBI_OBJ_MATRIX;
   if(name=="ObjVel"   )return SBI_OBJ_VEL;
   if(name=="Mesh"     )return SBI_MESH;
   if(name=="Material" )return SBI_MATERIAL;
   if(name=="Viewport" )return SBI_VIEWPORT;
   if(name=="Color"    )return SBI_COLOR;
                        ASSERT(SBI_NUM==7);
                        return -1;
}
static void TestBuffer(C Str8 &name, Int bind_slot)
{
   Int expected=ExpectedBufferSlot(name);
   if( expected==bind_slot
   ||  expected<0 && (bind_slot<0 || bind_slot>=SBI_NUM))return;
   Exit(S+"Shader Buffer \""+name+"\" was expected to be at slot: "+expected+", but got: "+bind_slot);
}
static void TestBuffer(C ShaderBuffer *buffer, Int bind_slot)
{
 C Str8 *name=ShaderBuffers.dataToKey(buffer); if(!name)Exit("Can't find ShaderBuffer name");
   return TestBuffer(*name, bind_slot);
}

ShaderCompiler::ShaderCompiler() : buffers(CompareCS) {}

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
   GlExtensions               ext;
#endif
#if DEBUG
   Memc<                ShaderData> (&shader_datas)[ST_NUM];
   Mems<ShaderCompiler::Shader*   >  &shaders;
#endif
   ShaderCompiler::Shader* findShader(C ShaderData &shader_data)C // find first 'Shader' using 'shader_data'
   {
   #if DEBUG
      FREPAD(type, shader_datas)
      {
         Int shader_data_index=shader_datas[type].index(&shader_data); if(shader_data_index>=0)
         {
            FREPAD(si, shaders)
            {
               ShaderCompiler::Shader &shader=*shaders[si]; if(shader.sub[type].shader_data_index==shader_data_index)return &shader;
            }
            break;
         }
      }
   #endif
      return null;
   }
   ShaderCompiler::Shader& shader(C ShaderData &shader_data)C // get first 'Shader' using 'shader_data'
   {
      ShaderCompiler::Shader *shader=findShader(shader_data); if(!shader)Exit("Can't find shader using 'shader_data'");
      return *shader;
   }
   Str shaderName(C ShaderData &shader_data)C
   {
      if(ShaderCompiler::Shader *shader=findShader(shader_data))return shader->name;
      return S;
   }

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
static Str8 RemoveSpaces(C Str8 &str)
{
   Str8 s; if(str.is())
   {
      s.reserve(str.length());
      Bool possible_preproc=true, preproc=false;
      CChar8 *text=str; for(Char8 last=0;;)
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
   }
   return s;
}
static Str8 RemoveEmptyLines(C Str8 &str)
{
   Str8 s; if(str.is())
   {
      s.reserve(str.length());
      CChar8 *text=str;
      for(Char8 last='\n'; ; )
      {
         Char8 c=*text++; if(!c)break;
         if(c=='\n' && (last=='\n' || !*text))continue;
         s+=c; last=c;
      }
      if(s.last()=='\n')s.removeLast();
   }
   return s;
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

   SHADER_TYPE type;
   switch(spvc_compiler_get_execution_model(spirv_compiler))
   {
      case SpvExecutionModelVertex                : type=ST_VS; break;
      case SpvExecutionModelTessellationControl   : type=ST_HS; break;
      case SpvExecutionModelTessellationEvaluation: type=ST_DS; break;
    //case SpvExecutionModelGeometry              : type=ST_GS; break;
      case SpvExecutionModelFragment              : type=ST_PS; break;
      default: Exit("Invalid Execution model"); break;
   }

   spvc_resources resources=null; spvc_compiler_create_shader_resources(spirv_compiler, &resources);
   const spvc_reflected_resource *list=null; size_t count=0; spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &list, &count);
   Memc<Str8> buffer_instances;
   Memc<ShaderCompiler::Buffer> buffers; buffers.setNum((Int)count); FREPA(buffers)
   {
      ShaderCompiler::Buffer  &buffer=buffers[i];
    C spvc_reflected_resource &res   =list[i]; auto buffer_handle=spvc_compiler_get_type_handle(spirv_compiler, res.type_id);
    //CChar8 *base_type_name=spvc_compiler_get_name(spirv_compiler, res.base_type_id); // "type_##NAME" name prefixed with "type_", "layout(std140) uniform type_Viewport {"
    //CChar8 *     type_name=spvc_compiler_get_name(spirv_compiler, res.type_id);
                 buffer.name=spvc_compiler_get_name(spirv_compiler, res.id); // actual buffer name as defined in HLSL, "layout(std140) uniform .. {} NAME"

      Str8 &instance_name=buffer_instances.New(); instance_name=S+"_BufferInstance_"+i;
      spvc_compiler_set_name(spirv_compiler, res.id, instance_name);
      spvc_compiler_set_name(spirv_compiler, res.base_type_id, (compiler.api==API_GL) ? S8+'_'+buffer.name : buffer.name); // prefix all cbuffers on GL with '_' to avoid buffer/param name conflicts

      //spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationBinding) not needed for GL, because we obtain it in 'ShaderGL.validate'
      buffer.bind_slot    =ExpectedBufferSlot(buffer.name);
      buffer.bind_explicit=(buffer.bind_slot>=0);
      size_t size=0; spvc_compiler_get_declared_struct_size(spirv_compiler, buffer_handle, &size); buffer.size=(Int)size;
      
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
         unsigned offset=0; spvc_compiler_type_struct_member_offset(spirv_compiler, buffer_handle, i, &offset);

         param.addTranslation(spirv_compiler, res.base_type_id, buffer_handle, member, member_handle, i, offset); param.sortTranslation();

         Int member_size=0;
         if(param.array_elms)
         {
            unsigned    stride=0; spvc_compiler_type_struct_member_array_stride(spirv_compiler, buffer_handle, i, &stride);
            member_size=stride*(param.array_elms-1); // all elements except last
         }
         // add last element
         if(member_type==SPVC_BASETYPE_STRUCT)
         {
            size_t struct_size=0; spvc_compiler_get_declared_struct_size(spirv_compiler, member_handle, &struct_size); member_size+=(Int)struct_size;
         }else
         {
            auto vec_size=spvc_type_get_vector_size(member_handle),
                 cols    =spvc_type_get_columns    (member_handle);
            if(cols>1) // matrix
            {
               unsigned stride=0; spvc_compiler_type_struct_member_matrix_stride(spirv_compiler, buffer_handle, i, &stride);
               member_size+=(vec_size-1)*stride     //  all vectors except last
                           +cols        *SIZE(Flt); // last vector
            }else // scalar, vector
            {
               member_size+=vec_size*SIZE(Flt);
            }
         }

         Int min=INT_MAX, max=0; REPA(param.translation){ShaderParam::Translation &translation=param.translation[i]; MIN(min, translation.gpu_offset); MAX(max, translation.gpu_offset+translation.elm_size);} // iterate all translations just for safety (normally checking just the first and last one should be enough)
         param.gpu_data_size=max-min;

         if(!param.translation.elms()                              )Exit("Shader Param is empty.\nPlease contact Developer.");
         if(        param.gpu_data_size >        member_size
         ||  Ceil16(param.gpu_data_size)!=Ceil16(member_size)      )Exit("Incorrect Shader Param size.\nPlease contact Developer."); // SPIR-V returns padded struct sizes, so it can be a little bigger than actually used 'gpu_data_size'
         if( param.translation[0].gpu_offset!=offset || offset!=min)Exit("Incorrect Shader Param Offset.\nPlease contact Developer.");
         if( param.gpu_data_size+offset>buffer.size                )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer.");
       //if( SIZE(Vec4)+var_desc.StartOffset>buffer.size           )Exit("Shader Param does not fit in Constant Buffer.\nPlease contact Developer."); some functions assume that '_gpu_data_size' is at least as big as 'Vec4' to set values without checking for size, !! this is not needed and shouldn't be called because in DX10+ Shader Params are stored in Shader Buffers, and 'ShaderBuffer' already allocates padding for Vec4

         //if(HasData(var_desc.DefaultValue, var_desc.Size)) // if parameter has any data
         //   param.data.setNum(param.gpu_data_size).copyFrom((Byte*)var_desc.DefaultValue); FIXME - https://github.com/KhronosGroup/SPIRV-Cross/issues/1094   https://github.com/KhronosGroup/SPIRV-Headers/issues/125
      }
   }

   if(buffers.elms())
   {
      SyncLocker lock(cc.lock); FREPA(buffers)
      {
       C ShaderCompiler::Buffer &src = buffers[i];
         ShaderCompiler::Buffer &dest=*compiler.buffers(src.name);
         if(!dest.name.is())dest=src;else if(dest!=src)Exit(S+"Buffer \""+dest.name+"\" is not always the same in all shaders");
      }
   }

   Char8 start[256], temp[256];
   Memt<Int> locations;
   list=null; count=0; spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_OUTPUT, &list, &count); FREP(count)
   {
    C spvc_reflected_resource &res=list[i];
    //CChar8 *name=spvc_compiler_get_name(spirv_compiler, res.id); name=_SkipStart(name, "out_var_");
      Int loc=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationLocation); //DYNAMIC_ASSERT(loc==i, S+"location!=i "+cc.shaderName(shader_data));
    //Bool no_persp=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationNoPerspective);
      DYNAMIC_ASSERT(locations.binaryInclude(loc), "location included multiple times");
      Set(start, (type==ST_PS) ? "RT" : "IO"); Append(start, TextInt(loc, temp)); // OUTPUT name must match INPUT name, this solves problem when using "TEXCOORD" and "TEXCOORD0"
      spvc_compiler_set_name(spirv_compiler, res.id, start);
   }
   locations.clear();
   list=null; count=0; spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &list, &count); FREP(count)
   {
    C spvc_reflected_resource &res=list[i];
      CChar8 *name=spvc_compiler_get_name(spirv_compiler, res.id); name=_SkipStart(name, "in_var_");
      Int loc=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationLocation); //DYNAMIC_ASSERT(loc==i, S+"location!=i "+cc.shaderName(shader_data));
    //Bool no_persp=spvc_compiler_get_decoration(spirv_compiler, res.id, SpvDecorationNoPerspective);
      DYNAMIC_ASSERT(locations.binaryInclude(loc), "location included multiple times");
      if(Starts(name, "ATTR"))DYNAMIC_ASSERT(TextInt(name+4)==loc, "ATTR index!=loc"); // verify vtx input ATTR index
      Set(start, (type==ST_VS) ? "ATTR" : "IO"); Append(start, TextInt(loc, temp)); // OUTPUT name must match INPUT name, this solves problem when using "TEXCOORD" and "TEXCOORD0"
      spvc_compiler_set_name(spirv_compiler, res.id, start);
   }

   spvc_compiler_options options=null;
   spvc_compiler_create_compiler_options(spirv_compiler, &options); if(!options)Exit("'spvc_compiler_create_compiler_options' failed");
   switch(compiler.api)
   {
      case API_GL:
      {
      #if 1 // use GL_ES to output precisions
         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
         spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 300);
      #else
         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_FALSE);
         spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION, 330);
      #endif

         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_FLOAT_PRECISION_HIGHP, type!=ST_PS);
         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_INT_PRECISION_HIGHP  , SPVC_TRUE  );

         spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_SUPPORT_NONZERO_BASE_INSTANCE, SPVC_FALSE);

       //spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, SPVC_FALSE);
      }break;

      case API_METAL:
      {
      }break;
   }
   spvc_compiler_install_compiler_options(spirv_compiler, options);

 //spvc_variable_id dummy=0; spvc_compiler_build_dummy_sampler_for_combined_images(spirv_compiler, &dummy);
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

   code=Replace(code, "#version 330\n", S);
   code=Replace(code, "#version 300 es\n", S);
   code=RemoveEmptyLines(RemoveSpaces(code));

   FREPA(buffer_instances)
   {
      Str8 &inst=buffer_instances[i];
      code=Replace(code, inst+'.', S, true, true);
      code=Replace(code, inst    , S, true, true);
   }

#if FORCE_LOG_SHADER_CODE
   #pragma message("!! Warning: Use this only for debugging !!")
   LogN(S+"/******************************************************************************/\nShader:"+cc.shaderName(shader_data)+' '+ShaderTypeName[type]+'\n'+code);
#endif

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
   if(COMPRESS_GL) // compressed
   {
      File f; f.readMem(code(), code.length()+1); // include null char
      File cmp; if(!Compress(f, cmp.writeMem(), COMPRESS_GL, COMPRESS_GL_LEVEL, false))Exit("Can't compress shader data");
      f.del(); cmp.pos(0); shader_data.setNum(cmp.size()).loadRawData(cmp);
   }else // uncompressed
   {
      shader_data.setNum(code.length()+1).copyFrom((Byte*)code()); // include null char
   }
}
/******************************************************************************/
static ShaderImage * Get(Int i, C MemtN<ShaderImage *, 256> &images ) {RANGE_ASSERT_ERROR(i, images , "Invalid ShaderImage index" ); return  images[i];}
static ShaderBuffer* Get(Int i, C MemtN<ShaderBuffer*, 256> &buffers) {RANGE_ASSERT_ERROR(i, buffers, "Invalid ShaderBuffer index"); return buffers[i];}
/******************************************************************************/
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
   ConstantIndex ci; f>>ci; index=ci.bind_index; RANGE_ASSERT_ERROR(index, MAX_SHADER_BUFFERS, S+"Buffer index: "+index+", is too big"); buffer=Get(ci.src_index, buffers); if(DEBUG)TestBuffer(buffer, index);
   return f.ok();
}
Bool ImageLink::load(File &f, C MemtN<ShaderImage*, 256> &images)
{
   ConstantIndex ci; f>>ci; index=ci.bind_index; RANGE_ASSERT_ERROR(index, MAX_SHADER_IMAGES, S+"Image index: "+index+", is too big"); image=Get(ci.src_index, images);
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
               Buffer &sub_buffer=sub.buffers[i];
               Buffer &    buffer=*buffers(sub_buffer.name);
               if(!buffer.name.is())buffer=sub_buffer;else if(buffer!=sub_buffer)return error(S+"Buffer \""+buffer.name+"\" is not always the same in all shaders");
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
      Map<Str8, Buffer> def_val_buffers(CompareCS); Swap(buffers, def_val_buffers);

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
      // process dummies to get buffers and images
      FREPA(sources)
      {
         Source &source=sources[i]; FREPA(source.shaders)
         {
            Shader &shader=source.shaders[i]; if(shader.dummy)FREPA(shader.sub)
            {
               ShaderData &sd=shader.sub[i].shader_data; if(sd.elms())threads.queue(sd, Convert, cc);
            }
         }
      }
      threads.wait1();

      REPA(buffers)
      {
         Buffer &dest=buffers[i], *src=def_val_buffers.find(dest.name);
         if(!src)Exit(S+"def_val_buffer \""+dest.name+"\" not found");
         REPA(dest.params)
         {
            Param &param=dest.params[i], &src_p=src->getParam(param.name);
            param.setDataFrom(src_p);
         }
      }
   }

   File f; if(f.writeTry(dest))
   {
      f.putUInt (CC4_SHDR); // CC4
      f.putByte (api     ); // API
      f.cmpUIntV(0       ); // version

      // constants
      f.cmpUIntV(buffers.elms()); FREPA(buffers)
      {
       C Buffer &buf=buffers[i];

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
      RANGE_ASSERT_ERROR(indexes.buffer_bind_index[i], shader_file._buffer_links, "Buffer Bind Index out of range"); buffers[i]=shader_file._buffer_links[indexes.buffer_bind_index[i]];
      RANGE_ASSERT_ERROR(indexes. image_bind_index[i], shader_file. _image_links,  "Image Bind Index out of range");  images[i]=shader_file. _image_links[indexes. image_bind_index[i]];
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
               f>>sb.explicit_bind_slot; if(sb.explicit_bind_slot>=0){SyncLocker lock(D._lock); sb.bind(sb.explicit_bind_slot);}
               if(DEBUG)TestBuffer(temp_str, sb.explicit_bind_slot);
            }else // verify if it's identical to previously created
            {
               if(sb.full_size!=f.decUIntV())ExitParam(temp_str, name);
               sb.bindCheck(f.getSByte());
            }

            // params
            REP(f.decUIntV())
            {
               f.getStr(temp_str); ShaderParam &sp=*ShaderParams(temp_str);
               if(!sp.is()) // wasn't yet created
               {
                  sp._data   = sb.data;
                  sp._changed=&sb.changed;
                  f.getMulti(sp._cpu_data_size, sp._gpu_data_size, sp._elements); // info
                  LoadTranslation(sp._full_translation, f, sp._elements);         // translation
                  Int offset=sp._full_translation[0].gpu_offset; sp._data+=offset; REPAO(sp._full_translation).gpu_offset-=offset; // apply offset. 'gpu_offset' is stored relative to start of cbuffer, however when loading we want to adjust 'sp.data' to point to the start of the param, so since we're adjusting it we have to adjust 'gpu_offset' too.
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
   }
//error:
   del(); return false;
}
/******************************************************************************/
}
/******************************************************************************/
