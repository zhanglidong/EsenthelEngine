/******************************************************************************/
enum API : Byte // !! These enums are saved !!
{
   API_DX,
   API_GL,
   API_VULKAN,
   API_METAL,
   API_NUM,
};
struct ShaderCompiler
{
   struct Param
   {
      Str8 name;
      Int  array_elms, cpu_data_size=0, gpu_data_size;
      Mems<ShaderParam::Translation> translation;
      Mems<Byte> data;

      Bool operator==(C Param &p)C;
      Bool operator!=(C Param &p)C {return !(T==p);}

      Bool save(File &f)C;
   #if WINDOWS
      void addTranslation(ID3D11ShaderReflectionType *type, C D3D11_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16); // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
      void addTranslation(ID3D12ShaderReflectionType *type, C D3D12_SHADER_TYPE_DESC &type_desc, CChar8 *name, Int &offset, SByte &was_min16); // 'was_min16'=if last inserted parameter was of min16 type (-1=no last parameter)
   #if SPIRV_CROSS
      void addTranslation(spvc_compiler compiler, spvc_type parent, spvc_type var, Int var_i, Int offset);
   #endif
      void sortTranslation();
   #endif
   };
   struct Bind
   {
      Str8 name;
      Int  bind_slot;

      Bool operator==(C Bind &b)C {return bind_slot==b.bind_slot && Equal(name, b.name, true);}
      Bool operator!=(C Bind &b)C {return !(T==b);}
   };
   struct Buffer : Bind
   {
      Int         size;
      Bool        bind_explicit;
      Mems<Param> params;

      Int  explicitBindSlot()C {return bind_explicit ? bind_slot : -1;}
      Bool operator==(C Buffer &b)C; // this checks 'bind_slot' only for 'bind_explicit'
      Bool operator!=(C Buffer &b)C {return !(T==b);}
   };
   struct Image : Bind
   {
   };
   struct IO
   {
      Str8 name;
      Int  index, reg;

   #if WINDOWS
      void operator=(C D3D11_SIGNATURE_PARAMETER_DESC &desc) {name=desc.SemanticName; index=desc.SemanticIndex; reg=desc.Register;}
      void operator=(C D3D12_SIGNATURE_PARAMETER_DESC &desc) {name=desc.SemanticName; index=desc.SemanticIndex; reg=desc.Register;}
   #endif
      Bool operator==(C IO &io)C {return index==io.index && reg==io.reg && Equal(name, io.name, true);}
      Bool operator!=(C IO &io)C {return !(T==io);}
   };

   enum RESULT : Byte
   {
      NONE,
      FAIL,
      GOOD,
   };

   struct Shader;
   struct SubShader
   {
    C Shader      *shader;
      SHADER_TYPE  type;
      RESULT       result=NONE;
      Str8         func_name;
      Str          error;
      Mems<Buffer> buffers;
      Mems<Image > images;
      Mems<IO    > inputs, outputs;
      ShaderData   shader_data;
      Int          shader_data_index=-1, // index of 'ShaderData' in 'ShaderFile'
                   buffer_bind_index=-1, // index of buffer binds in 'ShaderFile'
                    image_bind_index=-1; // index of image  binds in 'ShaderFile'

      Bool is()C {return func_name.is();}
      void compile();
   };

   struct TextParam8
   {
      Str8 name, value;
      void set(C Str8 &name, C Str8 &value) {T.name=name; T.value=value;}
   };

   struct Source;
   struct Shader
   {
      Str              name;
      SHADER_MODEL     model;
      Bool             dummy=false; // if skip saving this shader, this is used only to access info about ConstantBuffers/ShaderParams
      Memc<TextParam8> params,
                       extra_params; // params not affecting shader name
      SubShader        sub[ST_NUM];
    C Source          *source;

      Shader& multiSample(       ) {MAX(model, SM_4_1);  return T;} // SM_4_1 needed for 'SV_SampleIndex'
      Shader& multiSample(Bool ms) {if(ms)multiSample(); return T;} // SM_4_1 needed for 'SV_SampleIndex'

      Shader& operator()(C Str &n0, C Str &v0                                                                     ) {params.New().set(n0, v0);                                                                               return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1                                              ) {params.New().set(n0, v0); params.New().set(n1, v1);                                                     return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2                       ) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2);                           return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2,  C Str &n3, C Str &v3) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2); params.New().set(n3, v3); return T;}

      Shader& extra(C Str &n0, C Str &v0                                                                     ) {extra_params.New().set(n0, v0);                                                                                                 return T;}
      Shader& extra(C Str &n0, C Str &v0,  C Str &n1, C Str &v1                                              ) {extra_params.New().set(n0, v0); extra_params.New().set(n1, v1);                                                                 return T;}
      Shader& extra(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2                       ) {extra_params.New().set(n0, v0); extra_params.New().set(n1, v1); extra_params.New().set(n2, v2);                                 return T;}
      Shader& extra(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2,  C Str &n3, C Str &v3) {extra_params.New().set(n0, v0); extra_params.New().set(n1, v1); extra_params.New().set(n2, v2); extra_params.New().set(n3, v3); return T;}

      Shader& tesselate(Bool tesselate)
      {
         if(tesselate)
         {
            sub[ST_HS].func_name="HS";
            sub[ST_DS].func_name="DS";
         }
         return T("TESSELATE", tesselate);
      }
      Shader& position(Int skin, Int textures, Int test_blend, Int fx, Int tesselate) {return T("SKIN", skin, "TEXTURES", textures, "TEST_BLEND", test_blend, "FX", fx).tesselate(tesselate);}

      void finalizeName();
      Bool save(File &f, C ShaderCompiler &compiler)C;
   };

   struct Source
   {
      Str               file_name;
      Mems<Byte>        file_data;
   #if WINDOWS && NEW_COMPILER
      IDxcBlobEncoding *file_blob=null;
   #else
      Ptr               file_blob=null;
   #endif
      Memc<Shader>      shaders;
      SHADER_MODEL      model;
      ShaderCompiler   *compiler;

      Shader& New(C Str &name=S, C Str8 &vs_func_name="VS", C Str8 &ps_func_name="PS");
      Bool    load();
     ~Source();
   };
   Str                dest, messages;
   SHADER_MODEL       model;
   API                api;
   Memc<Source>       sources;
   Map <Str8, Buffer> buffers;
   Memc<Str8        > images;

   void message(C Str &t) {messages.line()+=t;}
   Bool error  (C Str &t) {message(t); return false;}

   ShaderCompiler& set(C Str &dest, SHADER_MODEL model, API api);
   Source& New(C Str &file_name);

   Bool compileTry(Threads &threads);
   void compile   (Threads &threads);

   ShaderCompiler();
};
/******************************************************************************/
