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
   enum SHADER_TYPE : Byte
   {
      VS,
      HS,
      DS,
      PS,
      ST_NUM,
   };

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
   #endif
   };
   struct Buffer
   {
      Str8 name;
      Int  size, bind_slot;
      Bool bind_explicit;
    //Mems<Byte>  data;
      Mems<Param> params;

      Int  explicitBindSlot()C {return bind_explicit ? bind_slot : -1;}
      Bool operator==(C Buffer &b)C;
      Bool operator!=(C Buffer &b)C {return !(T==b);}
   };
   struct Image
   {
      Str8 name;
      Int  bind_slot;
   };

   struct IO
   {
      Str8 name;
      Int  index, reg;

   #if WINDOWS
      void operator=(C D3D11_SIGNATURE_PARAMETER_DESC &desc) {name=desc.SemanticName; index=desc.SemanticIndex; reg=desc.Register;}
   #endif
      Bool operator==(C IO &io)C {return name==io.name && index==io.index && reg==io.reg;}
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
      Int          shader_data_index;

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
      Memc<TextParam8> params;
      SubShader        sub[ST_NUM];
    C Source          *source;

      Shader& multiSample(Bool ms) {if(ms)MAX(model, SM_4_1); return T;} // SM_4_1 needed for 'SV_SampleIndex'

      Shader& operator()(C Str &n0, C Str &v0                                                                     ) {params.New().set(n0, v0);                                                                               return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1                                              ) {params.New().set(n0, v0); params.New().set(n1, v1);                                                     return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2                       ) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2);                           return T;}
      Shader& operator()(C Str &n0, C Str &v0,  C Str &n1, C Str &v1,  C Str &n2, C Str &v2,  C Str &n3, C Str &v3) {params.New().set(n0, v0); params.New().set(n1, v1); params.New().set(n2, v2); params.New().set(n3, v3); return T;}

      void finalizeName();
      Bool save(File &f, C Map<Str8, Buffer*> &buffers, C Memc<Str8> &images)C;
   };

   struct Source
   {
      Str             file_name;
      Mems<Byte>      file_data;
      Memc<Shader>    shaders;
      SHADER_MODEL    model;
      ShaderCompiler *compiler;

      Shader& New(C Str &name, C Str8 &vs, C Str8 &ps);
      Bool    load();
   };
   Str          dest, messages;
   SHADER_MODEL model;
   API          api;
   Memc<Source> sources;

   void message(C Str &t) {messages.line()+=t;}
   Bool error  (C Str &t) {message(t); return false;}

   ShaderCompiler& set(C Str &dest, SHADER_MODEL model, API api);
   Source& New(C Str &file_name);

   Bool compileTry(Threads &threads);
   void compile   (Threads &threads);
};
/******************************************************************************/
