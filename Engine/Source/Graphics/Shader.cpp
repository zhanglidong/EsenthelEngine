/******************************************************************************

   Shader having 'SV_SampleIndex' will execute on a per-sample basis,
                           others will execute on per-pixel basis.

   Depth/Stencil tests however are always performed on a per-sample basis.

   TODO: !! All GLSL shaders need to be optimized either by hand or with a better converter, this could give performance boost even 2x !!

/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
#if DEBUG
   #define FORCE_TEX 0
   #define FORCE_BUF 0
#else
   #define FORCE_TEX 0
   #define FORCE_BUF 0
#endif

#define ALLOW_PARTIAL_BUFFERS 0 // using partial buffers (1) actually made things slower, 100fps(1) vs 102fps(0), so use default value (0), TODO: check on newer hardware
#define BUFFER_DYNAMIC        0 // for ALLOW_PARTIAL_BUFFERS=0, using 1 made no difference in performance, so use 0 to reduce API calls. But for ALLOW_PARTIAL_BUFFERS=1 using 1 was slower
/******************************************************************************/
#if DX11
static ID3D11ShaderResourceView *VSTex[MAX_SHADER_IMAGES], *HSTex[MAX_SHADER_IMAGES], *DSTex[MAX_SHADER_IMAGES], *PSTex[MAX_SHADER_IMAGES];
#elif GL
static UInt                      Tex[MAX_SHADER_IMAGES];
#endif
INLINE void DisplayState::texVS(Int index, GPU_API(ID3D11ShaderResourceView*, UInt) tex)
{
#if DX11
   if(VSTex[index]!=tex)D3DC->VSSetShaderResources(index, 1, &(VSTex[index]=tex));
#endif
}
INLINE void DisplayState::texHS(Int index, GPU_API(ID3D11ShaderResourceView*, UInt) tex)
{
#if DX11
   if(HSTex[index]!=tex)D3DC->HSSetShaderResources(index, 1, &(HSTex[index]=tex));
#endif
}
INLINE void DisplayState::texDS(Int index, GPU_API(ID3D11ShaderResourceView*, UInt) tex)
{
#if DX11
   if(DSTex[index]!=tex)D3DC->DSSetShaderResources(index, 1, &(DSTex[index]=tex));
#endif
}
INLINE void DisplayState::texPS(Int index, GPU_API(ID3D11ShaderResourceView*, UInt) tex)
{
#if DX11
   if(PSTex[index]!=tex || FORCE_TEX)D3DC->PSSetShaderResources(index, 1, &(PSTex[index]=tex));
#endif
}
void DisplayState::texClear(GPU_API(ID3D11ShaderResourceView*, UInt) tex)
{
#if DX11
   if(tex)REPA(PSTex)if(PSTex[i]==tex)PSTex[i]=null;
#elif GL
   if(tex)REPA(Tex)if(Tex[i]==tex)Tex[i]=~0;
#endif
}
#if GL
       static UInt ActiveTexture=0;
INLINE static void ActivateTexture(Int index)
{
   if(ActiveTexture!=index || FORCE_TEX)
   {
      ActiveTexture=index;
      glActiveTexture(GL_TEXTURE0+index);
   }
}
void DisplayState::texBind(UInt mode, UInt tex) // this should be called instead of 'glBindTexture'
{
   if(GetThreadId()==App.threadID()) // textures are bound per-context, so remember them only on the main thread
   {
      if(Tex[ActiveTexture]==tex)return;
         Tex[ActiveTexture]= tex;
   }
   glBindTexture(mode, tex);
}
INLINE static void TexBind(UInt mode, UInt tex)
{
    Tex[ActiveTexture]=tex;
   glBindTexture(mode, tex);
}
static void SetTexture(Int index, C Image *image, ShaderImage::Sampler *sampler) // this is called only on the Main thread
{
#if 0
   glBindMultiTextureEXT(GL_TEXTURE0+index, GL_TEXTURE_2D, txtr); // not supported on ATI (tested on Radeon 5850)
#else
   UInt txtr=(image ? image->_txtr : 0);
   if(Tex[index]!=txtr || FORCE_TEX)
   {
      ActivateTexture(index);
      if(!txtr) // clear all modes
      {
         Tex[index]=0;
         glBindTexture(GL_TEXTURE_2D      , 0);
         glBindTexture(GL_TEXTURE_3D      , 0);
         glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      }else
      switch(image->mode())
      {
         case IMAGE_2D:
         case IMAGE_RT:
         case IMAGE_DS:
         case IMAGE_SHADOW_MAP:
         {
            TexBind(GL_TEXTURE_2D, image->_txtr);
            UInt s, t;
            if(!sampler)s=t=D._sampler_address;else // use default
            {
               s=sampler->address[0];
               t=sampler->address[1];
            }
         #if X64
            if(image->_addr_u!=s)glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ConstCast(image->_addr_u)=s);
            if(image->_addr_v!=t)glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ConstCast(image->_addr_v)=t);
         #else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
         #endif
         }break;

         case IMAGE_3D:
         {
            TexBind(GL_TEXTURE_3D, image->_txtr);
            UInt s, t, r;
            if(!sampler)s=t=r=D._sampler_address;else
            {
               s=sampler->address[0];
               t=sampler->address[1];
               r=sampler->address[2];
            }
         #if X64
            if(image->_addr_u!=s)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, ConstCast(image->_addr_u)=s);
            if(image->_addr_v!=t)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, ConstCast(image->_addr_v)=t);
            if(image->_addr_w!=r)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, ConstCast(image->_addr_w)=r);
         #else
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, s);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, t);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, r);
         #endif
         }break;

         case IMAGE_CUBE:
         {
            TexBind(GL_TEXTURE_CUBE_MAP, image->_txtr);
         }break;
      }
   }else
   if(txtr)switch(image->mode()) // check if sampler states need to be adjusted
   {
      case IMAGE_2D:
      case IMAGE_RT:
      case IMAGE_DS:
      case IMAGE_SHADOW_MAP:
      {
         UInt s, t;
         if(!sampler)s=t=D._sampler_address;else
         {
            s=sampler->address[0];
            t=sampler->address[1];
         }
      #if X64
         if(image->_addr_u!=s || image->_addr_v!=t)
         {
            ActivateTexture(index);      TexBind(GL_TEXTURE_2D, image->_txtr);
            if(image->_addr_u!=s)glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ConstCast(image->_addr_u)=s);
            if(image->_addr_v!=t)glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ConstCast(image->_addr_v)=t);
         }
      #else
         ActivateTexture(index); TexBind(GL_TEXTURE_2D, image->_txtr);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
      #endif
      }break;

      case IMAGE_3D:
      {
         UInt s, t, r;
         if(!sampler)s=t=r=D._sampler_address;else
         {
            s=sampler->address[0];
            t=sampler->address[1];
            r=sampler->address[2];
         }
      #if X64
         if(image->_addr_u!=s || image->_addr_v!=t || image->_addr_w!=r)
         {
            ActivateTexture(index);      TexBind(GL_TEXTURE_3D, image->_txtr);
            if(image->_addr_u!=s)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, ConstCast(image->_addr_u)=s);
            if(image->_addr_v!=t)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, ConstCast(image->_addr_v)=t);
            if(image->_addr_w!=r)glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, ConstCast(image->_addr_w)=r);
         }
      #else
         ActivateTexture(index); TexBind(GL_TEXTURE_3D, image->_txtr);
         glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, s);
         glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, t);
         glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, r);
      #endif
      }break;
   }
#endif
}
#endif
/******************************************************************************/
#if DX11
static ID3D11Buffer *vs_buf[MAX_SHADER_BUFFERS], *hs_buf[MAX_SHADER_BUFFERS], *ds_buf[MAX_SHADER_BUFFERS], *ps_buf[MAX_SHADER_BUFFERS];

static INLINE void BufVS(Int index, ID3D11Buffer *buf) {if(vs_buf[index]!=buf || FORCE_BUF)D3DC->VSSetConstantBuffers(index, 1, &(vs_buf[index]=buf));}
static INLINE void BufHS(Int index, ID3D11Buffer *buf) {if(hs_buf[index]!=buf || FORCE_BUF)D3DC->HSSetConstantBuffers(index, 1, &(hs_buf[index]=buf));}
static INLINE void BufDS(Int index, ID3D11Buffer *buf) {if(ds_buf[index]!=buf || FORCE_BUF)D3DC->DSSetConstantBuffers(index, 1, &(ds_buf[index]=buf));}
static INLINE void BufPS(Int index, ID3D11Buffer *buf) {if(ps_buf[index]!=buf || FORCE_BUF)D3DC->PSSetConstantBuffers(index, 1, &(ps_buf[index]=buf));}
#endif
/******************************************************************************/
Cache<ShaderFile> ShaderFiles("Shader");
/******************************************************************************/
INLINE static void COPY(Ptr dest, CPtr src, UInt size)
{
   U32 *d=(U32*)dest,
       *s=(U32*)src ;
   REP(DivCeil4(size))*d++=*s++;
}
/******************************************************************************/
// SHADER IMAGE
/******************************************************************************/
ThreadSafeMap<Str8, ShaderImage> ShaderImages(CompareCS);
/******************************************************************************/
#if DX11
void ShaderImage::Sampler::del()
{
   if(state)
   {
    //SyncLocker locker(D._lock); if(state) lock not needed for DX11 'Release'
         {if(D.created())state->Release(); state=null;} // clear while in lock
   }
}
Bool ShaderImage::Sampler::createTry(D3D11_SAMPLER_DESC &desc)
{
 //SyncLocker locker(D._lock); lock not needed for DX11 'D3D'
   del();
   if(D3D)D3D->CreateSamplerState(&desc, &state);
   return state!=null;
}
void ShaderImage::Sampler::create(D3D11_SAMPLER_DESC &desc)
{
   if(!createTry(desc))Exit(S+"Can't create Sampler State\n"
                              "Filter: "+desc.Filter+"\n"
                              "Address: "+desc.AddressU+','+desc.AddressV+','+desc.AddressW+"\n"
                              "MipLODBias: "+desc.MipLODBias+"\n"
                              "Anisotropy: "+desc.MaxAnisotropy+"\n"
                              "ComparisonFunc: "+desc.ComparisonFunc+"\n"
                              "MinMaxLOD: "+desc.MinLOD+','+desc.MaxLOD);
}
void ShaderImage::Sampler::setVS(Int index) {D3DC->VSSetSamplers(index, 1, &state);}
void ShaderImage::Sampler::setHS(Int index) {D3DC->HSSetSamplers(index, 1, &state);}
void ShaderImage::Sampler::setDS(Int index) {D3DC->DSSetSamplers(index, 1, &state);}
void ShaderImage::Sampler::setPS(Int index) {D3DC->PSSetSamplers(index, 1, &state);}
void ShaderImage::Sampler::set  (Int index) {setVS(index); setHS(index); setDS(index); setPS(index);}
#endif
/******************************************************************************/
// SHADER BUFFER
/******************************************************************************/
ThreadSafeMap<Str8, ShaderBuffer> ShaderBuffers(CompareCS);
/******************************************************************************/
void ShaderBuffer::Buffer::del()
{
   if(buffer)
   {
   #if GL_LOCK // lock not needed for DX11 'Release'
      SafeSyncLocker lock(D._lock);
   #endif

      if(D.created())
      {
      #if DX11
         buffer->Release();
      #elif GL
         glDeleteBuffers(1, &buffer);
      #endif
      }
      
      buffer=GPU_API(null, 0);
   }
   size=0;
}
void ShaderBuffer::Buffer::create(Int size)
{
 //if(T.size!=size) can't check for this, because buffers can be dynamically resized
   {
   #if GL_LOCK // lock not needed for DX11 'D3D'
      SyncLocker locker(D._lock);
   #endif

      del();
      if(D.created())
      {
         T.size=size;
      #if DX11
         D3D11_BUFFER_DESC desc;
         desc.ByteWidth          =size;
         desc.Usage              =(BUFFER_DYNAMIC ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
         desc.CPUAccessFlags     =(BUFFER_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0);
         desc.BindFlags          =D3D11_BIND_CONSTANT_BUFFER;
         desc.MiscFlags          =0;
         desc.StructureByteStride=0;

         D3D->CreateBuffer(&desc, null, &buffer);
      #elif GL
         glGenBuffers(1, &buffer);
         glBindBuffer(GL_UNIFORM_BUFFER, buffer);
         glBufferData(GL_UNIFORM_BUFFER, size, null, GL_STREAM_DRAW);
      #endif
      }
   }
   if(!buffer)Exit("Can't create Constant Buffer");
}
/******************************************************************************/
// !! Warning: if we have any 'parts', then 'buffer' does not own the resources, but is just a raw copy !!
/******************************************************************************/
ShaderBuffer::~ShaderBuffer()
{
   if(parts.elms())buffer.zero(); // if we have any 'parts', then 'buffer' does not own the resources, so just zero it, and they will be released in the 'parts' container
   Free(data);
}
ShaderBuffer::ShaderBuffer()
{
   changed=false;
   data   =null;
}
void ShaderBuffer::create(Int size) // no locks needed because this is called only in shader loading, and there 'ShaderBuffers.lock' is called
{
   buffer.create(size);
   AllocZero(data, Ceil4(size+SIZEI(Vec4))); // add extra "Vec4 padd" at the end, because all 'ShaderParam.set' for performance reasons assume that there is at least SIZE(Vec4) size, use "+" instead of "Max" in case we have "Flt p[2]" and we call 'ShaderParam.set(Vec4)' for ShaderParam created from "p[1]" which would overwrite "p[1..4]", and do 'Ceil4' because 'COPY' is used which copies 'Ceil4'
   changed=true;
#if GL
   if(buffer.is())glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint(), buffer.buffer); //glBindBufferRange(GL_UNIFORM_BUFFER, bindPoint(), buffer.buffer, 0, buffer.size);
#endif
}
void ShaderBuffer::update()
{
#if DX11
   if(BUFFER_DYNAMIC)
   {
      D3D11_MAPPED_SUBRESOURCE map;
      if(OK(D3DC->Map(buffer.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
      {
         COPY(map.pData, data, buffer.size);
         D3DC->Unmap(buffer.buffer, 0);
      }
   }else
#if ALLOW_PARTIAL_BUFFERS // check for partial updates only if we may operate on partial buffers, because otherwise we always set entire buffers (which are smaller and separated into parts) and we can avoid the overhead of setting up 'D3D11_BOX'
   if(D3DC1) // use partial updates where available to reduce amount of memory
   {
      D3D11_BOX box;
      box.front=box.top=box.left=0;
      box.right=Ceil16(buffer.size); box.back=box.bottom=1; // must be 16-byte aligned or DX will fail
         D3DC1->UpdateSubresource1(buffer.buffer, 0, &box, data, 0, 0, D3D11_COPY_DISCARD);
   }else
#endif
         D3DC ->UpdateSubresource (buffer.buffer, 0, null, data, 0, 0);
#elif GL
   glBindBuffer   (GL_UNIFORM_BUFFER, buffer.buffer);
   glBufferSubData(GL_UNIFORM_BUFFER, 0, buffer.size, data);
#endif
   changed=false;
}
void ShaderBuffer::bind(Int index)
{
#if DX11
   BufVS(index, buffer.buffer);
   BufHS(index, buffer.buffer);
   BufDS(index, buffer.buffer);
   BufPS(index, buffer.buffer);
#endif
}
void ShaderBuffer::bindCheck(Int index)
{
#if DX11
   if(index>=0)
   {
      RANGE_ASSERT_ERROR(index, MAX_SHADER_BUFFERS, "Invalid ShaderBuffer bind index");
      ID3D11Buffer *buf=vs_buf[index];
                 if(buffer  .buffer==buf)return;
      REPA(parts)if(parts[i].buffer==buf)return;
      Exit(S+"ShaderBuffer was expected to be bound at slot "+index);
   }
#endif
}
void ShaderBuffer::setPart(Int part)
{
   buffer =parts[part]; // perform a raw copy
   changed=true;
}
void ShaderBuffer::createParts(C Int *elms, Int elms_num)
{
   Int elm_size=buffer.size/elms[0];
   parts.setNum(elms_num);          parts[0]=buffer; // store a raw copy of the buffer that was already created in the first slot, so we can keep it as backup and use later
   for(Int i=1; i<parts.elms(); i++)parts[i].create(elm_size*elms[i]);
}
#if GL
Int ShaderBuffer::bindPoint()C
{
   DEBUG_ASSERT(ShaderBuffers.containsData(this), "'ShaderBuffer' not inside 'ShaderBuffers'"); // needed to get correct abs index
   return ShaderBuffers.dataInMapToAbsIndex(this)+1; // skip zero, because it's used for 'glBindBuffer'
}
#endif
/******************************************************************************/
// SHADER PARAM
/******************************************************************************/
ThreadSafeMap<Str8, ShaderParam> ShaderParams(CompareCS);
/******************************************************************************/
void ShaderParam::OptimizeTranslation(C MemPtr<Translation> &src, Mems<Translation> &dest)
{
   dest=src;
   REPA(dest)if(i)
   {
      Translation &prev=dest[i-1],
                  &next=dest[i  ];
      if(prev.cpu_offset+prev.elm_size==next.cpu_offset
      && prev.gpu_offset+prev.elm_size==next.gpu_offset)
      {
         prev.elm_size+=next.elm_size;
         dest.remove(i, true);
      }
   }
}
/******************************************************************************/
ShaderParam::~ShaderParam()
{
   if(_owns_data)
   {
      Free(_data   );
      Free(_changed);
   }
  _data   =null;
  _changed=null;
  _cpu_data_size=_gpu_data_size=_elements=0;
}
ShaderParam::ShaderParam()
{
  _data   =null;
  _changed=null;
  _cpu_data_size=_gpu_data_size=_elements=0;
  _owns_data=false;
}
/******************************************************************************/
void ShaderParam::initAsElement(ShaderParam &parent, Int index)
{
   DEBUG_ASSERT(this!=&parent, "Can't init from self");
   RANGE_ASSERT(index, parent._elements);

  _owns_data    =false;
  _cpu_data_size=parent._cpu_data_size/parent._elements; // set size of a single element
  _data         =parent._data;
  _changed      =parent._changed;

 /*if(parent._full_translation.elms()==1)
   {
     _full_translation=parent._full_translation;
      Translation &t=_full_translation[0];
      if(t.elm_size% parent._elements || parent._gpu_data_size%parent._elements)Exit("ShaderParam Mod");
         t.elm_size/=parent._elements;
      DEBUG_ASSERT(t.cpu_offset==0 && t.gpu_offset==0, "Invalid translation offsets");
     _data+=t.elm_size*index;
     _optimized_translation=_full_translation;
     _gpu_data_size        =parent._gpu_data_size/parent._elements;
   }else*/
   {
      if(                  parent._full_translation.elms()%parent._elements)Exit("ShaderParam Mod");
      Int elm_translations=parent._full_translation.elms()/parent._elements; // calculate number of translations for a single element
     _full_translation.clear(); FREP(elm_translations)_full_translation.add(parent._full_translation[index*elm_translations+i]); // add translations for 'index-th' single element
      Int offset=_full_translation[0].gpu_offset; _data+=offset; REPAO(_full_translation).gpu_offset-=offset; // apply offset
          offset=_full_translation[0].cpu_offset;                REPAO(_full_translation).cpu_offset-=offset; // apply offset
      optimize();
     _gpu_data_size=0; REPA(_optimized_translation)MAX(_gpu_data_size, _optimized_translation[i].gpu_offset+_optimized_translation[i].elm_size);
   }
}
/******************************************************************************/
void ShaderParam::set(  Bool   b          ) {setChanged(); *(Flt *)_data=b;}
void ShaderParam::set(  Int    i          ) {setChanged(); *(Flt *)_data=i;}
void ShaderParam::set(  Flt    f          ) {setChanged(); *(Flt *)_data=f;}
void ShaderParam::set(  Dbl    d          ) {setChanged(); *(Flt *)_data=d;}
void ShaderParam::set(C Vec2  &v          ) {setChanged(); *(Vec2*)_data=v;}
void ShaderParam::set(C VecD2 &v          ) {setChanged(); *(Vec2*)_data=v;}
void ShaderParam::set(C VecI2 &v          ) {setChanged(); *(Vec2*)_data=v;}
void ShaderParam::set(C Vec   &v          ) {setChanged(); *(Vec *)_data=v;}
void ShaderParam::set(C VecD  &v          ) {setChanged(); *(Vec *)_data=v;}
void ShaderParam::set(C VecI  &v          ) {setChanged(); *(Vec *)_data=v;}
void ShaderParam::set(C Vec4  &v          ) {setChanged(); *(Vec4*)_data=v;}
void ShaderParam::set(C VecD4 &v          ) {setChanged(); *(Vec4*)_data=v;}
void ShaderParam::set(C VecI4 &v          ) {setChanged(); *(Vec4*)_data=v;}
void ShaderParam::set(C Rect  &rect       ) {setChanged(); *(Rect*)_data=rect;}
void ShaderParam::set(C Color &color      ) {setChanged(); *(Vec4*)_data=SRGBToDisplay(color);}
void ShaderParam::set(C Vec   *v, Int elms)
{
   setChanged();
#if DX11
   Vec4 *gpu=(Vec4*)_data;
   REP(Min(elms, (_gpu_data_size+SIZEU(Flt))/SIZEU(Vec4)))gpu[i].xyz=v[i]; // add SIZE(Flt) because '_gpu_data_size' may be SIZE(Vec) and div by SIZE(Vec4) would return 0 even though one Vec would fit (elements are aligned by 'Vec4' but we're writing only 'Vec')
#elif GL
   // FIXME
   COPY(_data, v, Min(_gpu_data_size, SIZEU(*v)*elms));
#endif
}
void ShaderParam::set(C Vec4 *v, Int elms) {setChanged(); COPY(_data, v, Min(_gpu_data_size, SIZEU(*v)*elms));}

void ShaderParam::set(C Matrix3 &matrix)
{
#if DX11
   if(_gpu_data_size>=SIZE(Vec4)+SIZE(Vec4)+SIZE(Vec)) // do not test for 'SIZE(Matrix)' !! because '_gpu_data_size' may be SIZE(Matrix) minus last Flt, because it's not really used (this happens on DX10+)
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[0].xyz.set(matrix.x.x, matrix.y.x, matrix.z.x); // SIZE(Vec4)
      gpu[1].xyz.set(matrix.x.y, matrix.y.y, matrix.z.y); // SIZE(Vec4)
      gpu[2].xyz.set(matrix.x.z, matrix.y.z, matrix.z.z); // SIZE(Vec )
   }
#elif GL
   // FIXME
   if(_gpu_data_size>=SIZE(matrix))
   {
      setChanged();
      Vec *gpu=(Vec*)_data;
      gpu[0].set(matrix.x.x, matrix.y.x, matrix.z.x);
      gpu[1].set(matrix.x.y, matrix.y.y, matrix.z.y);
      gpu[2].set(matrix.x.z, matrix.y.z, matrix.z.z);
   }
#endif
}
void ShaderParam::set(C Matrix &matrix)
{
   if(_gpu_data_size>=SIZE(matrix))
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[0].set(matrix.x.x, matrix.y.x, matrix.z.x, matrix.pos.x);
      gpu[1].set(matrix.x.y, matrix.y.y, matrix.z.y, matrix.pos.y);
      gpu[2].set(matrix.x.z, matrix.y.z, matrix.z.z, matrix.pos.z);
   }
}
void ShaderParam::set(C MatrixM &matrix)
{
   if(_gpu_data_size>=SIZE(Matrix)) // we're setting as 'Matrix' and not 'MatrixM'
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[0].set(matrix.x.x, matrix.y.x, matrix.z.x, matrix.pos.x);
      gpu[1].set(matrix.x.y, matrix.y.y, matrix.z.y, matrix.pos.y);
      gpu[2].set(matrix.x.z, matrix.y.z, matrix.z.z, matrix.pos.z);
   }
}
void ShaderParam::set(C Matrix4 &matrix)
{
   if(_gpu_data_size>=SIZE(matrix))
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[0].set(matrix.x.x, matrix.y.x, matrix.z.x, matrix.pos.x);
      gpu[1].set(matrix.x.y, matrix.y.y, matrix.z.y, matrix.pos.y);
      gpu[2].set(matrix.x.z, matrix.y.z, matrix.z.z, matrix.pos.z);
      gpu[3].set(matrix.x.w, matrix.y.w, matrix.z.w, matrix.pos.w);
   }
}
void ShaderParam::set(C Matrix *matrix, Int elms)
{
   setChanged();
   Vec4 *gpu=(Vec4*)_data;
   REP(Min(elms, UInt(_gpu_data_size)/SIZEU(*matrix)))
   {
      gpu[0].set(matrix->x.x, matrix->y.x, matrix->z.x, matrix->pos.x);
      gpu[1].set(matrix->x.y, matrix->y.y, matrix->z.y, matrix->pos.y);
      gpu[2].set(matrix->x.z, matrix->y.z, matrix->z.z, matrix->pos.z);
      gpu+=3;
      matrix++;
   }
}
void ShaderParam::set(CPtr data, Int size) // !! Warning: 'size' is ignored here for performance reasons !!
{
   setChanged();
   REPA(_optimized_translation)
   {
    C ShaderParam::Translation &trans=_optimized_translation[i];
      COPY(T._data+trans.gpu_offset, (Byte*)data+trans.cpu_offset, trans.elm_size);
   }
}

void ShaderParam::set(C Vec &v, Int elm)
{
#if DX11
   if(_gpu_data_size>=SIZE(Vec4)*elm+SIZE(Vec)) // elements are aligned by 'Vec4' but we're writing only 'Vec'
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[elm].xyz=v;
   }
#elif GL
   // FIXME
   if(_gpu_data_size>=SIZE(v)*(elm+1))
   {
      setChanged();
      Vec *gpu=(Vec*)_data;
      gpu[elm]=v;
   }
#endif
}
void ShaderParam::set(C Vec4 &v, Int elm)
{
   if(_gpu_data_size>=SIZE(v)*(elm+1))
   {
      setChanged();
      Vec4 *gpu=(Vec4*)_data;
      gpu[elm]=v;
   }
}
void ShaderParam::set(C Matrix &matrix, Int elm)
{
   if(_gpu_data_size>=SIZE(matrix)*(elm+1))
   {
      setChanged();
      Vec4 *gpu=(Vec4*)&(((GpuMatrix*)_data)[elm]);
      gpu[0].set(matrix.x.x, matrix.y.x, matrix.z.x, matrix.pos.x);
      gpu[1].set(matrix.x.y, matrix.y.y, matrix.z.y, matrix.pos.y);
      gpu[2].set(matrix.x.z, matrix.y.z, matrix.z.z, matrix.pos.z);
   }
}

void ShaderParam::fromMul(C Matrix &a, C Matrix &b)
{
   if(_gpu_data_size>=SIZE(GpuMatrix))
   {
      setChanged();
      ((GpuMatrix*)_data)->fromMul(a, b);
   }
}
void ShaderParam::fromMul(C Matrix &a, C MatrixM &b)
{
   if(_gpu_data_size>=SIZE(GpuMatrix))
   {
      setChanged();
      ((GpuMatrix*)_data)->fromMul(a, b);
   }
}
void ShaderParam::fromMul(C MatrixM &a, C MatrixM &b)
{
   if(_gpu_data_size>=SIZE(GpuMatrix))
   {
      setChanged();
      ((GpuMatrix*)_data)->fromMul(a, b);
   }
}
void ShaderParam::fromMul(C Matrix &a, C Matrix &b, Int elm)
{
   if(_gpu_data_size>=SIZE(GpuMatrix)*(elm+1))
   {
      setChanged();
      GpuMatrix *gpu=(GpuMatrix*)_data;
      gpu[elm].fromMul(a, b);
   }
}
void ShaderParam::fromMul(C Matrix &a, C MatrixM &b, Int elm)
{
   if(_gpu_data_size>=SIZE(GpuMatrix)*(elm+1))
   {
      setChanged();
      GpuMatrix *gpu=(GpuMatrix*)_data;
      gpu[elm].fromMul(a, b);
   }
}
void ShaderParam::fromMul(C MatrixM &a, C MatrixM &b, Int elm)
{
   if(_gpu_data_size>=SIZE(GpuMatrix)*(elm+1))
   {
      setChanged();
      GpuMatrix *gpu=(GpuMatrix*)_data;
      gpu[elm].fromMul(a, b);
   }
}

void ShaderParam::set(C GpuMatrix &matrix)
{
   if(_gpu_data_size>=SIZE(matrix))
   {
      setChanged();
      GpuMatrix &gpu=*(GpuMatrix*)_data;
      gpu=matrix;
   }
}
void ShaderParam::set(C GpuMatrix &matrix, Int elm)
{
   if(_gpu_data_size>=SIZE(matrix)*(elm+1))
   {
      setChanged();
      GpuMatrix *gpu=(GpuMatrix*)_data;
      gpu[elm]=matrix;
   }
}
void ShaderParam::set(C GpuMatrix *matrix, Int elms)
{
   setChanged();
   COPY(_data, matrix, Min(_gpu_data_size, SIZEU(*matrix)*elms));
}

void ShaderParam::setConditional(C Flt &f)
{
   U32 &dest =*(U32*)_data,
       &src  =*(U32*)&f   ;
   if(  dest!=src){setChanged(); dest=src;}
}
void ShaderParam::setConditional(C Vec2 &v)
{
   Vec2 &dest =*(Vec2*)_data;
   if(   dest!=v){setChanged(); dest=v;}
}
void ShaderParam::setConditional(C Vec &v)
{
   Vec &dest =*(Vec*)_data;
   if(  dest!=v){setChanged(); dest=v;}
}
void ShaderParam::setConditional(C Vec4 &v)
{
   Vec4 &dest =*(Vec4*)_data;
   if(   dest!=v){setChanged(); dest=v;}
}
void ShaderParam::setConditional(C Rect &r)
{
   Rect &dest =*(Rect*)_data;
   if(   dest!=r){setChanged(); dest=r;}
}

void ShaderParam::setConditional(C Vec &v, Int elm)
{
#if DX11
   if(_gpu_data_size>=SIZE(Vec4)*elm+SIZE(Vec)) // elements are aligned by 'Vec4' but we're writing only 'Vec'
   {
      Vec &dest=((Vec4*)_data)[elm].xyz;
      if(  dest!=v){setChanged(); dest=v;}
   }
#elif GL
   // FIXME
   if(_gpu_data_size>=SIZE(v)*(elm+1))
   {
      Vec &dest=((Vec*)_data)[elm];
      if(  dest!=v){setChanged(); dest=v;}
   }
#endif
}

void ShaderParam::setSafe(C Vec4 &v) {setChanged(); COPY(_data, &v, Min(_gpu_data_size, SIZEU(v)));}
/******************************************************************************/
// SHADERS
/******************************************************************************/
#if DX11
// lock not needed for DX11 'Release'
ShaderVS11::~ShaderVS11() {if(vs){/*SyncLocker locker(D._lock); if(vs)*/{if(D.created())vs->Release(); vs=null;}}} // clear while in lock
ShaderHS11::~ShaderHS11() {if(hs){/*SyncLocker locker(D._lock); if(hs)*/{if(D.created())hs->Release(); hs=null;}}} // clear while in lock
ShaderDS11::~ShaderDS11() {if(ds){/*SyncLocker locker(D._lock); if(ds)*/{if(D.created())ds->Release(); ds=null;}}} // clear while in lock
ShaderPS11::~ShaderPS11() {if(ps){/*SyncLocker locker(D._lock); if(ps)*/{if(D.created())ps->Release(); ps=null;}}} // clear while in lock
#endif

#if GL_LOCK
ShaderVSGL::~ShaderVSGL() {if(vs){SyncLocker locker(D._lock); if(D.created())glDeleteShader(vs); vs=0;}} // clear while in lock
ShaderPSGL::~ShaderPSGL() {if(ps){SyncLocker locker(D._lock); if(D.created())glDeleteShader(ps); ps=0;}} // clear while in lock
#elif GL
ShaderVSGL::~ShaderVSGL() {if(vs){if(D.created())glDeleteShader(vs); vs=0;}} // clear while in lock
ShaderPSGL::~ShaderPSGL() {if(ps){if(D.created())glDeleteShader(ps); ps=0;}} // clear while in lock
#endif

#if DX11
// lock not needed for DX11 'D3D', however we need a lock because this may get called from multiple threads at the same time, but we can use another lock to allow processing during rendering (when D._lock is locked)
static SyncLock ShaderLock; // use custom lock instead of 'D._lock' to allow shader creation while rendering
ID3D11VertexShader* ShaderVS11::create() {if(!vs && elms()){SyncLocker locker(ShaderLock); if(!vs && elms() && D3D){D3D->CreateVertexShader(data(), elms(), null, &vs); clean();}} return vs;}
ID3D11HullShader  * ShaderHS11::create() {if(!hs && elms()){SyncLocker locker(ShaderLock); if(!hs && elms() && D3D){D3D->CreateHullShader  (data(), elms(), null, &hs); clean();}} return hs;}
ID3D11DomainShader* ShaderDS11::create() {if(!ds && elms()){SyncLocker locker(ShaderLock); if(!ds && elms() && D3D){D3D->CreateDomainShader(data(), elms(), null, &ds); clean();}} return ds;}
ID3D11PixelShader * ShaderPS11::create() {if(!ps && elms()){SyncLocker locker(ShaderLock); if(!ps && elms() && D3D){D3D->CreatePixelShader (data(), elms(), null, &ps); clean();}} return ps;}
#elif GL
CChar8* GLSLVersion()
{
   switch(D.shaderModel())
   {
      default        : return ""; // avoid null in case some drivers will crash
      case SM_GL     : return "#version 330\n"; // needed for Mac and Win when using GL3
      case SM_GL_ES_3: return "#version 300 es\n";
   }
}
static SyncLock ShaderLock; // use custom lock instead of 'D._lock' to allow shader creation while rendering
UInt ShaderVSGL::create(Bool clean, Str *messages)
{
   if(!vs && elms())
   {
      SyncLocker locker(GL_LOCK ? D._lock : ShaderLock);
      if(!vs && elms())
      {
         File src, temp; src.readMem(data(), elms()); if(!Decompress(src, temp, true))return 0; temp.pos(0); // decompress shader
         UInt vs=glCreateShader(GL_VERTEX_SHADER); if(!vs)Exit("Can't create GL_VERTEX_SHADER"); // create into temp var first and set to this only after fully initialized
         CChar8 *code=(CChar8*)temp.mem(); // read code
      #if GL_ES
         for(; CChar8 *gl=TextPos(code, "gl_ClipDistance"); ){Char8 *t=(Char8*)gl; t[0]=t[1]='/';} // VS plane clipping not available on GLES 3
      #endif
         CChar8 *srcs[]={GLSLVersion(), code}; // version must be first
         glShaderSource(vs, Elms(srcs), srcs, null); glCompileShader(vs); // compile

         int ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
         if( ok)T.vs=vs;else // set to this only after all finished, so if another thread runs this method, it will detect 'vs' presence only after it was fully initialized
         {
            if(messages)
            {
               Char8 error[64*1024]; error[0]=0; glGetShaderInfoLog(vs, Elms(error), null, error);
               messages->line()+=(S+"Vertex Shader compilation failed:\n"+error).line()+"Vertex Shader code:\n";
               FREPA(srcs)*messages+=srcs[i];
               messages->line();
            }
            glDeleteShader(vs); //vs=0; not needed since it's a temporary
         }

         if(clean)T.clean();
      }
   }
   return vs;
}
UInt ShaderPSGL::create(Bool clean, Str *messages)
{
   if(!ps && elms())
   {
      SyncLocker locker(GL_LOCK ? D._lock : ShaderLock);
      if(!ps && elms())
      {
         File src, temp; src.readMem(data(), elms()); if(!Decompress(src, temp, true))return 0; temp.pos(0); // decompress shader
         UInt ps=glCreateShader(GL_FRAGMENT_SHADER); if(!ps)Exit("Can't create GL_FRAGMENT_SHADER"); // create into temp var first and set to this only after fully initialized
         CChar8 *code=(CChar8*)temp.mem(); // read code
         CChar8 *srcs[]={GLSLVersion(), code}; // version must be first
         glShaderSource(ps, Elms(srcs), srcs, null); glCompileShader(ps); // compile

         int ok; glGetShaderiv(ps, GL_COMPILE_STATUS, &ok);
         if( ok)T.ps=ps;else // set to this only after all finished, so if another thread runs this method, it will detect 'ps' presence only after it was fully initialized
         {
            if(messages)
            {
               Char8 error[64*1024]; error[0]=0; glGetShaderInfoLog(ps, Elms(error), null, error);
               messages->line()+=(S+"Pixel Shader compilation failed:\n"+error).line()+"Pixel Shader code:\n";
               FREPA(srcs)*messages+=srcs[i];
               messages->line();
            }
            glDeleteShader(ps); //ps=0; not needed since it's a temporary
         }

         if(clean)T.clean();
      }
   }
   return ps;
}
static Str ShaderSource(UInt shader)
{
   Char8  source[64*1024]; if(shader)glGetShaderSource(shader, SIZE(source), null, source);else source[0]=0;
   return source;
}
Str ShaderVSGL::source()
{
   return ShaderSource(vs);
}
Str ShaderPSGL::source()
{
   return ShaderSource(ps);
}
#endif
/******************************************************************************/
// SHADER TECHNIQUE
/******************************************************************************/
#if DX11
// these members must have native alignment because we use them in atomic operations for set on multiple threads
ALIGN_ASSERT(Shader11, vs);
ALIGN_ASSERT(Shader11, hs);
ALIGN_ASSERT(Shader11, ds);
ALIGN_ASSERT(Shader11, ps);
/*Shader11::~Shader11()
{
   /* no need to release 'vs,hs,ds,ps', shaders since they're just copies from 'Shader*11'
   if(D.created())
   {
    //SyncLocker locker(D._lock); lock not needed for DX11 'Release'
      if(vs)vs->Release();
      if(hs)hs->Release();
      if(ds)ds->Release();
      if(ps)ps->Release();
   }*
}*/
Bool Shader11::validate(ShaderFile &shader, Str *messages) // this function should be multi-threaded safe
{
   if(!vs && InRange(data_index[ST_VS], shader._vs))AtomicSet(vs, shader._vs[data_index[ST_VS]].create());
   if(!hs && InRange(data_index[ST_HS], shader._hs))AtomicSet(hs, shader._hs[data_index[ST_HS]].create());
   if(!ds && InRange(data_index[ST_DS], shader._ds))AtomicSet(ds, shader._ds[data_index[ST_DS]].create());
   if(!ps && InRange(data_index[ST_PS], shader._ps))AtomicSet(ps, shader._ps[data_index[ST_PS]].create());
   return vs && ps;
}
#if 0 // did not make any performance difference (set together with 'SetPrimitiveTopology' from "Vertex Index Buffer.cpp")
static ID3D11VertexShader *VS;   static INLINE void SetVS(ID3D11VertexShader *shader) {if(VS!=shader || Kb.shift())D3DC->VSSetShader(VS=shader, null, 0);}
static ID3D11HullShader   *HS;   static INLINE void SetHS(ID3D11HullShader   *shader) {if(HS!=shader || Kb.shift())D3DC->HSSetShader(HS=shader, null, 0);}
static ID3D11DomainShader *DS;   static INLINE void SetDS(ID3D11DomainShader *shader) {if(DS!=shader || Kb.shift())D3DC->DSSetShader(DS=shader, null, 0);}
static ID3D11PixelShader  *PS;   static INLINE void SetPS(ID3D11PixelShader  *shader) {if(PS!=shader || Kb.shift())D3DC->PSSetShader(PS=shader, null, 0);}

static D3D11_PRIMITIVE_TOPOLOGY PT;   INLINE void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY pt) {if(PT!=pt || Kb.shift())D3DC->IASetPrimitiveTopology(PT=pt);}
#else
static INLINE void SetVS(ID3D11VertexShader *shader) {D3DC->VSSetShader(shader, null, 0);}
static INLINE void SetHS(ID3D11HullShader   *shader) {D3DC->HSSetShader(shader, null, 0);}
static INLINE void SetDS(ID3D11DomainShader *shader) {D3DC->DSSetShader(shader, null, 0);}
static INLINE void SetPS(ID3D11PixelShader  *shader) {D3DC->PSSetShader(shader, null, 0);}

static INLINE void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY pt) {D3DC->IASetPrimitiveTopology(pt);}
#endif

void Shader11::commit()
{
   REPA(all_buffers){ShaderBuffer &b=*all_buffers[i]; if(b.changed)b.update();}
}
void Shader11::commitTex()
{
   if(hs)
   {
      REPA(images[ST_HS]){C ImageLink &t=images[ST_HS][i]; D.texHS(t.index, t.image->getSRV());}
      REPA(images[ST_DS]){C ImageLink &t=images[ST_DS][i]; D.texDS(t.index, t.image->getSRV());}
   }
   REPA(images[ST_VS]){C ImageLink &t=images[ST_VS][i]; D.texVS(t.index, t.image->getSRV());}
   REPA(images[ST_PS]){C ImageLink &t=images[ST_PS][i]; D.texPS(t.index, t.image->getSRV());}
}
void Shader11::start() // same as 'begin' but without committing buffers and textures
{
   SetVS(vs);
   SetPS(ps);
   if(hs/* && D.tesselationAllow()*/) // currently disabled to avoid extra overhead as tesselation isn't generally used, TODO:
   {
      SetHS(hs);
      SetDS(ds);
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
      REPA(buffers[ST_HS]){C BufferLink &b=buffers[ST_HS][i]; BufHS(b.index, b.buffer->buffer.buffer);}
      REPA(buffers[ST_DS]){C BufferLink &b=buffers[ST_DS][i]; BufDS(b.index, b.buffer->buffer.buffer);}
   }else
   {
      SetHS(null);
      SetDS(null);
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   }
   REPA(buffers[ST_VS]){C BufferLink &b=buffers[ST_VS][i]; BufVS(b.index, b.buffer->buffer.buffer);}
   REPA(buffers[ST_PS]){C BufferLink &b=buffers[ST_PS][i]; BufPS(b.index, b.buffer->buffer.buffer);}
}
void Shader11::begin()
{
   SetVS(vs);
   SetPS(ps);
   if(hs/* && D.tesselationAllow()*/) // currently disabled to avoid extra overhead as tesselation isn't generally used, TODO:
   {
      SetHS(hs);
      SetDS(ds);
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
      REPA( images[ST_HS]){C  ImageLink &t= images[ST_HS][i]; D.texHS(t.index, t.image->getSRV());}
      REPA( images[ST_DS]){C  ImageLink &t= images[ST_DS][i]; D.texDS(t.index, t.image->getSRV());}
      REPA(buffers[ST_HS]){C BufferLink &b=buffers[ST_HS][i];   BufHS(b.index, b.buffer->buffer.buffer);}
      REPA(buffers[ST_DS]){C BufferLink &b=buffers[ST_DS][i];   BufDS(b.index, b.buffer->buffer.buffer);}
   }else
   {
      SetHS(null);
      SetDS(null);
      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   }
   REPA(     images[ST_VS]){C      ImageLink &t=      images[ST_VS][i]; D.texVS(t.index, t.image->getSRV());}
   REPA(     images[ST_PS]){C      ImageLink &t=      images[ST_PS][i]; D.texPS(t.index, t.image->getSRV());}
   REPA(    buffers[ST_VS]){C     BufferLink &b=     buffers[ST_VS][i];   BufVS(b.index, b.buffer->buffer.buffer);}
   REPA(    buffers[ST_PS]){C     BufferLink &b=     buffers[ST_PS][i];   BufPS(b.index, b.buffer->buffer.buffer);}
   REPA(all_buffers       ){ShaderBuffer     &b=*all_buffers       [i]; if(b.changed)b.update();}
}
#elif GL
ShaderGL::~ShaderGL()
{
   if(prog)
   {
   #if GL_LOCK
      SyncLocker locker(D._lock);
   #endif
      if(D.created())glDeleteProgram(prog); prog=0; // clear while in lock
   }
   // no need to release 'vs,ps' shaders since they're just copies from 'Shader*GL'
}
Str ShaderGL::source()
{
   return S+"Vertex Shader:\n"+ShaderSource(vs)
          +"\nPixel Shader:\n"+ShaderSource(ps);
}
UInt ShaderGL::compileEx(MemPtr<ShaderVSGL> vs_array, MemPtr<ShaderPSGL> ps_array, Bool clean, ShaderFile *shader, Str *messages) // this function doesn't need to be multi-threaded safe, it's called by 'validate' where it's already surrounded by a lock, and by 'compile' during shader pre-processing (where it's called for the same object only from the same thread), GL thread-safety should be handled outside of this function
{
   // prepare shaders
   if(messages)messages->clear();
   if(!vs && InRange(vs_index, vs_array)){if(LogInit)LogN(S+"Compiling vertex shader in technique \""+name+"\" of shader \""+ShaderFiles.name(shader)+"\""); vs=vs_array[vs_index].create(clean, messages);} // no need for 'AtomicSet' because we don't need to be multi-thread safe here
   if(!ps && InRange(ps_index, ps_array)){if(LogInit)LogN(S+ "Compiling pixel shader in technique \""+name+"\" of shader \""+ShaderFiles.name(shader)+"\""); ps=ps_array[ps_index].create(clean, messages);} // no need for 'AtomicSet' because we don't need to be multi-thread safe here

   // prepare program
   UInt prog=0; // have to operate on temp variable, so we can return it to 'validate' which still has to do some things before setting it into 'this'
   if(vs && ps)
   {
      if(LogInit)Log(S+"Linking vertex+pixel shader in technique \""+name+"\" of shader \""+ShaderFiles.name(shader)+"\": ");
      prog=glCreateProgram(); if(!prog)Exit("Can't create GL Shader Program");
      FREP(16)
      {
         Char8 name[16], temp[256]; Set(name, "ATTR"); Append(name, TextInt(i, temp));
         glBindAttribLocation(prog, VtxSemanticToIndex(i), name);
      }
      glAttachShader(prog, vs);
      glAttachShader(prog, ps);
      glLinkProgram (prog);
      int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
      if(!ok)
      {
         int max_length; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &max_length);
         Mems<char> error; error.setNumZero(max_length+1); glGetProgramInfoLog(prog, max_length, null, error.data());
         if(messages)messages->line()+=(S+"Error linking vertex+pixel shader in technique \""+name+"\" of shader \""+ShaderFiles.name(shader)+"\"\n"+error.data()).line()+source().line();
         glDeleteProgram(prog); prog=0;
      }
      if(LogInit)LogN("Success");
   }
   return prog;
}
void ShaderGL::compile(MemPtr<ShaderVSGL> vs_array, MemPtr<ShaderPSGL> ps_array, Str *messages) // this function doesn't need to be multi-threaded safe, it's called only during shader pre-processing
{
#if GL_LOCK
   SyncLocker locker(D._lock);
#endif
   if(!prog)prog=compileEx(vs_array, ps_array, false, null, messages);
}
Bool ShaderGL::validate(ShaderFile &shader, Str *messages) // this function should be multi-threaded safe
{
   if(prog || !D.canDraw())return true; // skip shader compilation if we don't need it (this is because compiling shaders on Linux with no GPU can exit the app with a message like "Xlib:  extension "XFree86-VidModeExtension" missing on display ":99".")
   SyncLocker locker(GL_LOCK ? D._lock : ShaderLock);
   if(!prog)
      if(UInt prog=compileEx(shader._vs, shader._ps, true, &shader, messages)) // create into temp var first and set to this only after fully initialized
   {
      MemtN<ImageLink, 256> images;
      Int  params=0; glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &params);
      FREP(params)
      {
         // GLSL name
         Char8  glsl_name[1024]; glsl_name[0]=0;
         Int    size=0;
         GLenum type;
         glGetActiveUniform(prog, i, Elms(glsl_name), null, &size, &type, glsl_name);
         Bool   found=false;
         switch(type)
         {
            case GL_SAMPLER_2D:
            case GL_SAMPLER_CUBE:
         #ifdef GL_SAMPLER_3D
            case GL_SAMPLER_3D:
         #endif
         #ifdef GL_SAMPLER_2D_SHADOW
            case GL_SAMPLER_2D_SHADOW:
         #endif
         #if defined GL_SAMPLER_2D_SHADOW_EXT && GL_SAMPLER_2D_SHADOW_EXT!=GL_SAMPLER_2D_SHADOW
            case GL_SAMPLER_2D_SHADOW_EXT:
         #endif
            {
               Int tex_unit=images.elms(); if(!InRange(tex_unit, Tex))Exit(S+"Texture index: "+tex_unit+", is too big");
               Int location=glGetUniformLocation(prog, glsl_name); if(location<0)
               {
               #if WEB // this can happen on MS Edge for images that aren't actually used
                  LogN
               #else
                  Exit
               #endif
                     (S+"Invalid Uniform Location ("+location+") of GLSL Parameter \""+glsl_name+"\"");
                  continue;
               }
               images.New().set(tex_unit, *GetShaderImage(glsl_name));

               glUseProgram(prog);
               glUniform1i (location, tex_unit); // set 'location' sampler to use 'tex_unit' texture unit

               found=true;
            }break;

      // FIXME
            /*default:
            {
               REPA(glsl_params)
               {
                    GLSLParam &gp=glsl_params[i];
                  ShaderParam &sp=*gp.param;
                C Str8        &gp_name=ShaderParams.dataInMapToKey(sp);
                  if(Equal(gp_name     , glsl_name, true)
                  || Equal(gp.glsl_name, glsl_name, true))
                  {
                     if(gp.gpu_offset+SIZE(Flt)>sp._gpu_data_size)Exit(S+"Shader \""+name+"\" refers to Shader Param \""+gp_name+"\" with invalid offset");
                     Int       l=glGetUniformLocation(prog, glsl_name); if(l<0)Exit(S+"Invalid Uniform Location ("+l+") of GLSL Parameter \""+glsl_name+"\"");
                     Constant &c=constants.New();
                     c.set(l, size, sp._data+gp.gpu_offset, sp);
                     switch(type)
                     {
                        case GL_FLOAT     : c.uniform=glUniform1fv; break;
                        case GL_FLOAT_VEC2: c.uniform=glUniform2fv; break;
                        case GL_FLOAT_VEC3: c.uniform=glUniform3fv; break;
                        case GL_FLOAT_VEC4: c.uniform=glUniform4fv; break;
                        default           : Exit("Unrecognized Shader Parameter OpenGL Uniform Type"); break;
                     }

                     found=true; break;
                  }
               }
            }break;*/
         }
         /*if(!found)
         {
            // Some OpenGL drivers (ATI or Apple) aren't that good in optimizing shaders, so they can sometimes return variables
            // which normally because of optimizations should be eliminated, in this case we'll just ignore them.
         #if DEBUG && !GL_ES
            Str s=S+"Unrecognized GLSL Parameter \""+glsl_name+"\"";
            LogN(s); // Exit(s);
         #endif
         }*/
      }
      T.images=images;

      // FIXME
      params=0; glGetProgramiv(prog, GL_ACTIVE_UNIFORM_BLOCKS, &params); all_buffers.setNum(params);
      FREP(params)
      {
         Char8 name[256]; name[0]='\0'; Int length=0;
         glGetActiveUniformBlockName(prog, i, Elms(name), &length, name);
         if(!name[0])Exit("Can't access UBO name");
         all_buffers[i]=ShaderBuffers(Str8Temp(name));
         glUniformBlockBinding(prog, i, all_buffers[i]->bindPoint());
      }

      // !! at the end !!
      T.prog=prog; // set to this only after all finished, so if another thread runs this method, it will detect 'prog' presence only after it was fully initialized
   }
   return prog!=0;
}
void ShaderGL::commit()
{
   REPA(all_buffers){ShaderBuffer &b=*all_buffers[i]; if(b.changed)b.update();}
}
void ShaderGL::commitTex()
{
   REPA(images){C ImageLink &t=images[i]; SetTexture(t.index, t.image->get(), t.image->_sampler);}
}
void ShaderGL::start() // same as 'begin' but without committing buffers and textures
{
   glUseProgram(prog);
   REPA(buffers){C BufferLink &b=buffers[i]; glBindBufferBase(GL_UNIFORM_BUFFER, b.index, b.buffer);} // bind buffer
}
void ShaderGL::begin()
{
   glUseProgram(prog);
   REPA(all_buffers){ ShaderBuffer &b=*all_buffers[i]; if(b.changed)b.update();} // 'commit'
   REPA(     images){ C  ImageLink &t=      images[i]; SetTexture(t.index, t.image->get(), t.image->_sampler);} // 'commitTex'
   REPA(    buffers){ C BufferLink &b=     buffers[i]; glBindBufferBase(GL_UNIFORM_BUFFER, b.index, b.buffer);} // bind buffer
}
#endif
/******************************************************************************/
// MANAGE
/******************************************************************************/
ShaderFile::ShaderFile()
{
   // !! keep constructor here to properly initialize containers, because type sizes and constructors are hidden !!
}
void ShaderFile::del()
{
   // !! keep this to properly delete '_shaders', because type sizes and constructors are hidden !!
  _shaders.del(); // first delete this, then individual shaders

  _vs.del();
  _hs.del();
  _ds.del();
  _ps.del();

  _buffer_links.del();
   _image_links.del();
}
/******************************************************************************/
// GET / SET
/******************************************************************************/
Shader* ShaderFile::first()
{
   if(_shaders.elms())
   {
      Shader &shader=_shaders.first(); if(shader.validate(T))return &shader;
   }
   return null;
}
Shader* ShaderFile::find(C Str8 &name, Str *messages)
{
   if(name.is())for(Int l=0, r=_shaders.elms(); l<r; )
   {
      Int mid=UInt(l+r)/2,
          compare=Compare(name, _shaders[mid].name, true);
      if(!compare  ){Shader &shader=_shaders[mid]; return shader.validate(T, messages) ? &shader : null;}
      if( compare<0)r=mid;
      else          l=mid+1;
   }
   if(messages)*messages="Technique not found in shader.";
   return null;
}
Shader* ShaderFile::find(C Str8 &name)
{
   return find(name, null);
}
Shader* ShaderFile::get(C Str8 &name)
{
   if(name.is())
   {
      Str messages;
      if(Shader *shader=find(name, &messages))return shader;
      Exit(S+"Error accessing Shader \""+name+"\" in ShaderFile \""+ShaderFiles.name(this)+"\"."+(messages.is() ? S+"\n"+messages : S));
   }
   return null;
}
/******************************************************************************/
// DRAW
/******************************************************************************/
void Shader::draw(C Image *image, C Rect *rect) {Sh.Img[0]->set(image); draw(rect);}
void Shader::draw(                C Rect *rect)
{
   VI.shader (this);
   VI.setType(VI_2D_TEX, VI_STRIP);
   if(Vtx2DTex *v=(Vtx2DTex*)VI.addVtx(4))
   {
      if(!D._view_active.full || rect)
      {
       C RectI &viewport=D._view_active.recti; RectI recti;

         if(!rect)
         {
            recti=viewport;
            v[0].pos.set(-1,  1);
            v[1].pos.set( 1,  1);
            v[2].pos.set(-1, -1);
            v[3].pos.set( 1, -1);
         }else
         {
            recti=Renderer.screenToPixelI(*rect);
            Bool flip_x=(recti.max.x<recti.min.x),
                 flip_y=(recti.max.y<recti.min.y);
            if(  flip_x)Swap(recti.min.x, recti.max.x);
            if(  flip_y)Swap(recti.min.y, recti.max.y);
            if(!Cuts(recti, viewport)){VI.clear(); return;}
            Flt  xm=2.0f/viewport.w(),
                 ym=2.0f/viewport.h();
            Rect frac((recti.min.x-viewport.min.x)*xm-1, (viewport.max.y-recti.max.y)*ym-1,
                      (recti.max.x-viewport.min.x)*xm-1, (viewport.max.y-recti.min.y)*ym-1);
            if(flip_x)Swap(frac.min.x, frac.max.x);
            if(flip_y)Swap(frac.min.y, frac.max.y);
            v[0].pos.set(frac.min.x, frac.max.y);
            v[1].pos.set(frac.max.x, frac.max.y);
            v[2].pos.set(frac.min.x, frac.min.y);
            v[3].pos.set(frac.max.x, frac.min.y);
         }

         Rect tex(Flt(recti.min.x)/Renderer.resW(), Flt(recti.min.y)/Renderer.resH(),
                  Flt(recti.max.x)/Renderer.resW(), Flt(recti.max.y)/Renderer.resH());
         v[0].tex.set(tex.min.x, tex.min.y);
         v[1].tex.set(tex.max.x, tex.min.y);
         v[2].tex.set(tex.min.x, tex.max.y);
         v[3].tex.set(tex.max.x, tex.max.y);
      }else
      {
         v[0].pos.set(-1,  1);
         v[1].pos.set( 1,  1);
         v[2].pos.set(-1, -1);
         v[3].pos.set( 1, -1);
         v[0].tex.set(0, 0);
         v[1].tex.set(1, 0);
         v[2].tex.set(0, 1);
         v[3].tex.set(1, 1);
      }
   #if GL
      if(!D.mainFBO()) // in OpenGL when drawing to RenderTarget the 'dest.pos.y' must be flipped
      {
         CHS(v[0].pos.y);
         CHS(v[1].pos.y);
         CHS(v[2].pos.y);
         CHS(v[3].pos.y);
      }
   #endif
   }
   VI.end();
}
void Shader::draw(C Image *image, C Rect *rect, C Rect &tex) {Sh.Img[0]->set(image); draw(rect, tex);}
void Shader::draw(                C Rect *rect, C Rect &tex)
{
   VI.shader (this);
   VI.setType(VI_2D_TEX, VI_STRIP);
   if(Vtx2DTex *v=(Vtx2DTex*)VI.addVtx(4))
   {
      if(!D._view_active.full || rect)
      {
       C RectI &viewport=D._view_active.recti; RectI recti;

         if(!rect)
         {
            recti=viewport;
            v[0].pos.set(-1,  1);
            v[1].pos.set( 1,  1);
            v[2].pos.set(-1, -1);
            v[3].pos.set( 1, -1);
         }else
         {
            recti=Renderer.screenToPixelI(*rect);
            Bool flip_x=(recti.max.x<recti.min.x),
                 flip_y=(recti.max.y<recti.min.y);
            if(  flip_x)Swap(recti.min.x, recti.max.x);
            if(  flip_y)Swap(recti.min.y, recti.max.y);
            if(!Cuts(recti, viewport)){VI.clear(); return;}
            Flt  xm=2.0f/viewport.w(),
                 ym=2.0f/viewport.h();
            Rect frac((recti.min.x-viewport.min.x)*xm-1, (viewport.max.y-recti.max.y)*ym-1,
                      (recti.max.x-viewport.min.x)*xm-1, (viewport.max.y-recti.min.y)*ym-1);
            if(flip_x)Swap(frac.min.x, frac.max.x);
            if(flip_y)Swap(frac.min.y, frac.max.y);
            v[0].pos.set(frac.min.x, frac.max.y);
            v[1].pos.set(frac.max.x, frac.max.y);
            v[2].pos.set(frac.min.x, frac.min.y);
            v[3].pos.set(frac.max.x, frac.min.y);
         }
      }else
      {
         v[0].pos.set(-1,  1);
         v[1].pos.set( 1,  1);
         v[2].pos.set(-1, -1);
         v[3].pos.set( 1, -1);
      }
      v[0].tex.set(tex.min.x, tex.min.y);
      v[1].tex.set(tex.max.x, tex.min.y);
      v[2].tex.set(tex.min.x, tex.max.y);
      v[3].tex.set(tex.max.x, tex.max.y);
   #if GL
      if(!D.mainFBO()) // in OpenGL when drawing to RenderTarget the 'dest.pos.y' must be flipped
      {
         CHS(v[0].pos.y);
         CHS(v[1].pos.y);
         CHS(v[2].pos.y);
         CHS(v[3].pos.y);
      }
   #endif
   }
   VI.end();
}
/******************************************************************************/
void DisplayState::clearShader()
{
   // set ~0 for pointers because that's the most unlikely value that they would have
#if DX11
   SetMem(VSTex, ~0);
   SetMem(HSTex, ~0);
   SetMem(DSTex, ~0);
   SetMem(PSTex, ~0);
   SetMem(vs_buf, ~0);
   SetMem(hs_buf, ~0);
   SetMem(ds_buf, ~0);
   SetMem(ps_buf, ~0);
#elif GL
   SetMem(Tex, ~0);
#endif
}
/******************************************************************************/
// FORWARD RENDERER SHADER TECHNIQUE
/******************************************************************************/
static Int Compare(C FRSTKey &a, C FRSTKey &b)
{
   if(Int c=Compare(a.skin      , b.skin      ))return c;
   if(Int c=Compare(a.materials , b.materials ))return c;
   if(Int c=Compare(a.textures  , b.textures  ))return c;
   if(Int c=Compare(a.bump_mode , b.bump_mode ))return c;
   if(Int c=Compare(a.alpha_test, b.alpha_test))return c;
   if(Int c=Compare(a.light_map , b.light_map ))return c;
   if(Int c=Compare(a.detail    , b.detail    ))return c;
   if(Int c=Compare(a.rflct     , b.rflct     ))return c;
   if(Int c=Compare(a.color     , b.color     ))return c;
   if(Int c=Compare(a.mtrl_blend, b.mtrl_blend))return c;
   if(Int c=Compare(a.heightmap , b.heightmap ))return c;
   if(Int c=Compare(a.fx        , b.fx        ))return c;
   if(Int c=Compare(a.tess      , b.tess      ))return c;
   return 0;
}
static Bool Create(FRST &frst, C FRSTKey &key, Ptr)
{
   ShaderFile *shader_file=ShaderFiles("Forward");
   if(key.bump_mode==SBUMP_ZERO)
   {
      Shader *shader=shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, 0, 0,   false, 0,   false, 0,   false, 0,   false));
      frst.all_passes=false;
      frst.none  =shader;
      frst.dir   =shader;
      frst.point =shader;
      frst.linear=shader;
      frst.cone  =shader;
      REPAO(frst.   dir_shd)=shader;
            frst. point_shd =shader;
            frst.linear_shd =shader;
            frst.  cone_shd =shader;
   }else
   {
      frst.all_passes=true;
      frst.none  =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false, 0,   false, false,   false, false,   false, false,   key.tess));
      frst.dir   =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   true , false, 0,   false, false,   false, false,   false, false,   key.tess));
      frst.point =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false, 0,   true , false,   false, false,   false, false,   key.tess));
      frst.linear=shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false, 0,   false, false,   true , false,   false, false,   key.tess));
      frst.cone  =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false, 0,   false, false,   false, false,   true , false,   key.tess));

      if(D.shadowSupported())
      {
         REPAO(frst.   dir_shd)=shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   true , true , Ceil2(i+1),   false, false,   false, false,   false, false,  key.tess));
               frst. point_shd =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false,         0 ,   true , true ,   false, false,   false, false,  key.tess));
               frst.linear_shd =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false,         0 ,   false, false,   true , true ,   false, false,  key.tess));
               frst.  cone_shd =shader_file->get(TechNameForward(key.skin, key.materials, key.textures, key.bump_mode, key.alpha_test, key.light_map, key.detail, key.rflct, key.color, key.mtrl_blend, key.heightmap, key.fx,   false, false,         0 ,   false, false,   false, false,   true , true ,  key.tess));
      }else
      {
         REPAO(frst.   dir_shd)=null;
               frst. point_shd =null;
               frst.linear_shd =null;
               frst.  cone_shd =null;
      }
   }
   return true;
}
ThreadSafeMap<FRSTKey, FRST> Frsts(Compare, Create);
/******************************************************************************/
// BLEND LIGHT SHADER TECHNIQUE
/******************************************************************************/
static Int Compare(C BLSTKey &a, C BLSTKey &b)
{
   if(Int c=Compare(a.skin      , b.skin      ))return c;
   if(Int c=Compare(a.color     , b.color     ))return c;
   if(Int c=Compare(a.textures  , b.textures  ))return c;
   if(Int c=Compare(a.bump_mode , b.bump_mode ))return c;
   if(Int c=Compare(a.alpha_test, b.alpha_test))return c;
   if(Int c=Compare(a.alpha     , b.alpha     ))return c;
   if(Int c=Compare(a.light_map , b.light_map ))return c;
   if(Int c=Compare(a.rflct     , b.rflct     ))return c;
   if(Int c=Compare(a.fx        , b.fx        ))return c;
   if(Int c=Compare(a.per_pixel , b.per_pixel ))return c;
   return 0;
}
static Bool Create(BLST &blst, C BLSTKey &key, Ptr)
{
   ShaderFile *shader=ShaderFiles("Blend Light");
            blst.dir[0  ]=shader->get(TechNameBlendLight(key.skin, key.color, key.textures, key.bump_mode, key.alpha_test, key.alpha, key.light_map, key.rflct, key.fx, key.per_pixel, 0));
   if(D.shadowSupported() && key.per_pixel)
   {
      REP(6)blst.dir[i+1]=shader->get(TechNameBlendLight(key.skin, key.color, key.textures, key.bump_mode, key.alpha_test, key.alpha, key.light_map, key.rflct, key.fx, key.per_pixel, i+1));
   }else
   {
      REP(6)blst.dir[i+1]=blst.dir[0];
   }
   return true;
}
ThreadSafeMap<BLSTKey, BLST> Blsts(Compare, Create);
/******************************************************************************
can't be used because in RM_PREPARE we add models to the list and lights simultaneously
Shader* FRST::getShader()
{
   return *(Shader**)(((Byte*)this)+Renderer._frst_light_offset);
}
/******************************************************************************/
       Int  Matrixes, FurVels;
#if DX11
static Int  MatrixesPart, FurVelPart;
static Byte BoneNumToPart[256+1];
#endif
static ShaderBuffer *SBObjMatrix, *SBObjVel, *SBFurVel;
void SetMatrixCount(Int num)
{
   if(Matrixes!=num)
   {
      Matrixes=num;
      // !! Warning: for performance reasons this doesn't adjust 'ShaderParam.translation', so using 'ShaderParam.set*' based on translation will use full size, so make sure that method isn't called for 'ObjMatrix' and 'ObjVel' !!
   #if DX11
   #if ALLOW_PARTIAL_BUFFERS
      if(D3DC1)
      {
         SBObjMatrix->buffer.size=SIZE(GpuMatrix)*Matrixes;
         SBObjVel   ->buffer.size=SIZE(Vec4     )*Matrixes; // #VelAngVel
         Int m16=Ceil16(Matrixes*3);
      #if DEBUG
         static Int old_vel_count; Int vel_count=Matrixes*1; if(MatrixesPart!=m16)old_vel_count=Ceil16(vel_count);else if(vel_count>old_vel_count)Exit("Need to test vel count separately"); // check if when not making a change below, we need more constants for vel buffer than what was set last time, currently keep *1 but replace with *2 when merging with angular velocities #VelAngVel
      #endif
         if(MatrixesPart!=m16)
         {
            MatrixesPart=m16;
            // Warning: code below does not set the cached buffers as 'bind' does, as it's not needed, because those buffers have constant bind index
            ASSERT(SBI_OBJ_VEL==SBI_OBJ_MATRIX+1); // can do this only if they're next to each other
            UInt        first[]={0, 0}, // must be provided or DX will fail
                          num[]={Ceil16(Matrixes*3), Ceil16(Matrixes*1)}; // #VelAngVel
            ID3D11Buffer *buf[]={SBObjMatrix->buffer.buffer, SBObjVel->buffer.buffer};
            D3DC1->VSSetConstantBuffers1(SBI_OBJ_MATRIX, 2, buf, first, num);
            D3DC1->HSSetConstantBuffers1(SBI_OBJ_MATRIX, 2, buf, first, num);
            D3DC1->DSSetConstantBuffers1(SBI_OBJ_MATRIX, 2, buf, first, num);
            D3DC1->PSSetConstantBuffers1(SBI_OBJ_MATRIX, 2, buf, first, num);
         }
      }else
   #endif
      {
         Int part=BoneNumToPart[num]; if(MatrixesPart!=part)
         {
            MatrixesPart=part;
            SBObjMatrix->setPart(part);
            SBObjVel   ->setPart(part);
         #if 0
            SBObjMatrix->bind(SBI_OBJ_MATRIX);
            SBObjVel   ->bind(SBI_OBJ_VEL   );
         #else // bind 2 at the same time
            ASSERT(SBI_OBJ_VEL==SBI_OBJ_MATRIX+1); // can do this only if they're next to each other
            // Warning: code below does not set the cached buffers as 'bind' does, as it's not needed, because those buffers have constant bind index
            ID3D11Buffer *buf[]={SBObjMatrix->buffer.buffer, SBObjVel->buffer.buffer};
            D3DC->VSSetConstantBuffers(SBI_OBJ_MATRIX, 2, buf);
            D3DC->HSSetConstantBuffers(SBI_OBJ_MATRIX, 2, buf);
            D3DC->DSSetConstantBuffers(SBI_OBJ_MATRIX, 2, buf);
            D3DC->PSSetConstantBuffers(SBI_OBJ_MATRIX, 2, buf);
         #endif
         }
      }
   #elif GL
      // will affect 'ShaderBuffer::update()'
      SBObjMatrix->buffer.size=SIZE(GpuMatrix)*Matrixes;
      SBObjVel   ->buffer.size=SIZE(Vec4     )*Matrixes; // #VelAngVel
   #endif
   }
}
void SetFurVelCount(Int num) // !! unlike 'SetMatrixCount' this needs to be called before Shader start/begin, because it doesn't bind the new buffer !!
{
   if(FurVels!=num)
   {
      FurVels=num;

   #if DX11
      Int part=BoneNumToPart[num]; if(FurVelPart!=part)SBFurVel->setPart(FurVelPart=part);
   #elif GL
      SBFurVel->buffer.size=SIZE(Vec4)*num;
   #endif
   }
}
/******************************************************************************/
void InitMatrix()
{
   ViewMatrix=Sh.ViewMatrix->asGpuMatrix();

   DYNAMIC_ASSERT(Sh.ViewMatrix->_cpu_data_size==SIZE(GpuMatrix)*MAX_MATRIX, "Unexpected size of ViewMatrix");
   DYNAMIC_ASSERT(Sh.ObjVel    ->_cpu_data_size==SIZE(Vec      )*MAX_MATRIX, "Unexpected size of ObjVel"); // #VelAngVel
   DYNAMIC_ASSERT(Sh.FurVel    ->_cpu_data_size==SIZE(Vec      )*MAX_MATRIX, "Unexpected size of FurVel");

   SBObjMatrix=ShaderBuffers(Str8Temp("ObjMatrix")); DYNAMIC_ASSERT(SBObjMatrix->size()==SIZE(GpuMatrix)*MAX_MATRIX, "Unexpected size of ObjMatrix");
   SBObjVel   =ShaderBuffers(Str8Temp("ObjVel"   )); DYNAMIC_ASSERT(SBObjVel   ->size()==SIZE(Vec4     )*MAX_MATRIX, "Unexpected size of ObjVel"   ); // #VelAngVel
   SBFurVel   =ShaderBuffers(Str8Temp("FurVel"   )); DYNAMIC_ASSERT(SBFurVel   ->size()==SIZE(Vec4     )*MAX_MATRIX, "Unexpected size of FurVel"   );

#if DX11
   const Int parts[]={MAX_MATRIX, 192, 160, 128, 96, 80, 64, 56, 48, 32, 16, 8, 1}; // start from the biggest, because 'ShaderBuffer.size' uses it as the total size
   if(!ALLOW_PARTIAL_BUFFERS || !D3DC1) // have to create parts only if we won't use partial buffers
   {
      SBObjMatrix->createParts(parts, Elms(parts));
      SBObjVel   ->createParts(parts, Elms(parts));
   }else
   { // when we use ALLOW_PARTIAL_BUFFERS then for now we still have to create at least 1 part, because ShaderBuffer.size needs it to know the full size, which we dynamically resize (needed when loading other shaders and comparing that buffer total size matches)
      SBObjMatrix->createParts(parts, 1);
      SBObjVel   ->createParts(parts, 1);
   }
      SBFurVel   ->createParts(parts, Elms(parts));
   Int end=Elms(BoneNumToPart); for(Int i=0; i<Elms(parts)-1; i++){Int start=parts[i+1]+1; SetMem(&BoneNumToPart[start], i, end-start); end=start;} REP(end)BoneNumToPart[i]=Elms(parts)-1;
#endif
}
/******************************************************************************/
}
/******************************************************************************/
