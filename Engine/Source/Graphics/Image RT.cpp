/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define KNOWN_IMAGE_TYPE_USAGE (DX11)
/******************************************************************************/
struct ImageRTType
{
   IMAGE_TYPE types[6]; // 6 is max IMAGE_TYPE elms per ImageRTType
};
static const ImageRTType ImageRTTypes[]=
{
#if LINEAR_GAMMA
   {IMAGE_R8G8B8A8_SRGB                                                    }, // 0 IMAGERT_SRGBA
   {IMAGE_R8G8B8A8_SRGB                                                    }, // 1 IMAGERT_SRGB   (can't use IMAGE_R10G10B10A2 because it's not sRGB and will have low quality in dark colors, can't use IMAGE_R11G11B10F because it's broken, ColLight looks like no light, can't use IMAGE_R9G9B9E5F because it's not supported for RT)
   {IMAGE_F16_3, IMAGE_F16_4, IMAGE_R8G8B8A8_SRGB                          }, // 2 IMAGERT_SRGB_P (can't use IMAGE_R10G10B10A2 because it's not sRGB and will have low quality in dark colors, can't use IMAGE_R11G11B10F because it's broken, ColLight looks like no light, can't use IMAGE_R9G9B9E5F because it's not supported for RT)
   {IMAGE_F16_4, IMAGE_R8G8B8A8_SRGB                                       }, // 3 IMAGERT_SRGBA_H
   {IMAGE_F16_3, IMAGE_F16_4, IMAGE_R8G8B8A8_SRGB                          }, // 4 IMAGERT_SRGB_H
   {IMAGE_F32_4, IMAGE_F16_4, IMAGE_R8G8B8A8_SRGB                          }, // 5 IMAGERT_SRGBA_F
   {IMAGE_F32_3, IMAGE_F32_4, IMAGE_F16_3, IMAGE_F16_4, IMAGE_R8G8B8A8_SRGB}, // 6 IMAGERT_SRGB_F
#else
   {IMAGE_R8G8B8A8                                                                             }, // 0 IMAGERT_SRGBA
   {IMAGE_R10G10B10A2, IMAGE_R8G8B8A8                                                          }, // 1 IMAGERT_SRGB
   {IMAGE_R10G10B10A2, IMAGE_F16_3, IMAGE_F16_4, IMAGE_R8G8B8A8                                }, // 2 IMAGERT_SRGB_P
   {IMAGE_F16_4      , IMAGE_R8G8B8A8                                                          }, // 3 IMAGERT_SRGBA_H
   {IMAGE_F16_3      , IMAGE_F16_4, IMAGE_R10G10B10A2, IMAGE_R8G8B8A8                          }, // 4 IMAGERT_SRGB_H
   {IMAGE_F32_4      , IMAGE_F16_4, IMAGE_R8G8B8A8                                             }, // 5 IMAGERT_SRGBA_F
   {IMAGE_F32_3      , IMAGE_F32_4, IMAGE_F16_3, IMAGE_F16_4, IMAGE_R10G10B10A2, IMAGE_R8G8B8A8}, // 6 IMAGERT_SRGB_F
#endif

   {IMAGE_R8G8B8A8                                                                             }, //  7 IMAGERT_RGBA
   {IMAGE_R10G10B10A2, IMAGE_R8G8B8A8                                                          }, //  8 IMAGERT_RGB
   {IMAGE_R10G10B10A2, IMAGE_F16_3, IMAGE_F16_4, IMAGE_R8G8B8A8                                }, //  9 IMAGERT_RGB_P
   {IMAGE_F16_4      , IMAGE_R8G8B8A8                                                          }, // 10 IMAGERT_RGBA_H
   {IMAGE_F16_3      , IMAGE_F16_4, IMAGE_R10G10B10A2, IMAGE_R8G8B8A8                          }, // 11 IMAGERT_RGB_H
   {IMAGE_F32_4      , IMAGE_F16_4, IMAGE_R8G8B8A8                                             }, // 12 IMAGERT_RGBA_F
   {IMAGE_F32_3      , IMAGE_F32_4, IMAGE_F16_3, IMAGE_F16_4, IMAGE_R10G10B10A2, IMAGE_R8G8B8A8}, // 13 IMAGERT_RGB_F

   {IMAGE_R8G8B8A8_SIGN, IMAGE_F16_4}, // 14 IMAGERT_RGBA_S

   {IMAGE_F32, IMAGE_F16}, // 15 IMAGERT_F32
   {IMAGE_F16, IMAGE_F32}, // 16 IMAGERT_F16

   {IMAGE_R8     , IMAGE_R8G8     ,            IMAGE_R8G8B8A8                  }, // 17 IMAGERT_ONE
   {IMAGE_R8_SIGN, IMAGE_R8G8_SIGN, IMAGE_F16, IMAGE_R8G8B8A8_SIGN, IMAGE_F32  }, // 18 IMAGERT_ONE_S
   {               IMAGE_R8G8     ,            IMAGE_R8G8B8A8                  }, // 19 IMAGERT_TWO
   {               IMAGE_R8G8_SIGN,            IMAGE_R8G8B8A8_SIGN, IMAGE_F16_2}, // 20 IMAGERT_TWO_S

   {IMAGE_D24S8, IMAGE_D24X8, IMAGE_D32, IMAGE_D16}, // 21 IMAGERT_DS
}; ASSERT(IMAGERT_SRGBA==0 && IMAGERT_SRGB==1 && IMAGERT_SRGB_P==2 && IMAGERT_SRGBA_H==3 && IMAGERT_SRGB_H==4 && IMAGERT_SRGBA_F==5 && IMAGERT_SRGB_F==6 && IMAGERT_RGBA==7 && IMAGERT_RGB==8 && IMAGERT_RGB_P==9 && IMAGERT_RGBA_H==10 && IMAGERT_RGB_H==11 && IMAGERT_RGBA_F==12 && IMAGERT_RGB_F==13 && IMAGERT_RGBA_S==14 && IMAGERT_F32==15 && IMAGERT_F16==16 && IMAGERT_ONE==17 && IMAGERT_ONE_S==18 && IMAGERT_TWO==19 && IMAGERT_TWO_S==20 && IMAGERT_DS==21 && IMAGERT_NUM==22 && Elms(ImageRTTypes)==IMAGERT_NUM);

static CChar8 *ImageRTName[]=
{
   "SRGBA"  , // 0
   "SRGB"   , // 1
   "SRGB_P" , // 2
   "SRGBA_H", // 3
   "SRGB_H" , // 4
   "SRGBA_F", // 5
   "SRGB_F" , // 6

   "RGBA"  , // 7
   "RGB"   , // 8
   "RGB_P" , // 9
   "RGBA_H", // 10
   "RGB_H" , // 11
   "RGBA_F", // 12
   "RGB_F" , // 13

   "RGBA_S", // 14
   "F32"   , // 15
   "F16"   , // 16
   "ONE"   , // 17
   "ONE_S" , // 18
   "TWO"   , // 19
   "TWO_S" , // 20
   "DS"    , // 21
}; ASSERT(IMAGERT_SRGBA==0 && IMAGERT_SRGB==1 && IMAGERT_SRGB_P==2 && IMAGERT_SRGBA_H==3 && IMAGERT_SRGB_H==4 && IMAGERT_SRGBA_F==5 && IMAGERT_SRGB_F==6 && IMAGERT_RGBA==7 && IMAGERT_RGB==8 && IMAGERT_RGB_P==9 && IMAGERT_RGBA_H==10 && IMAGERT_RGB_H==11 && IMAGERT_RGBA_F==12 && IMAGERT_RGB_F==13 && IMAGERT_RGBA_S==14 && IMAGERT_F32==15 && IMAGERT_F16==16 && IMAGERT_ONE==17 && IMAGERT_ONE_S==18 && IMAGERT_TWO==19 && IMAGERT_TWO_S==20 && IMAGERT_DS==21 && IMAGERT_NUM==22 && Elms(ImageRTName)==IMAGERT_NUM);

#if KNOWN_IMAGE_TYPE_USAGE
static IMAGE_TYPE  ImageRTTypesOK[2][IMAGERT_NUM]; // [MultiSample][IMAGERT_NUM], this keeps info about result of creating different IMAGE_TYPE for 1-sample and multi-sample, this is because some formats may fail to create multi-sampled but succeed with 1-sample
#else
static ImageRTType ImageRTTypesOK[2][IMAGERT_NUM];
#endif
/******************************************************************************/
// !! WARNING: The functions below are for SRGB ONLY !!
static const IMAGERT_TYPE GetImageRTTypeLookup[IMAGE_PRECISION_NUM][2]= // [precision][alpha]
{
   // no alpha   ,     alpha
   {IMAGERT_SRGB  , IMAGERT_SRGBA  }, // 0 IMAGE_PRECISION_8
   {IMAGERT_SRGB_P, IMAGERT_SRGBA_P}, // 1 IMAGE_PRECISION_10
   {IMAGERT_SRGB_H, IMAGERT_SRGBA_H}, // 2 IMAGE_PRECISION_16
   {IMAGERT_SRGB_F, IMAGERT_SRGBA_F}, // 3 IMAGE_PRECISION_24
   {IMAGERT_SRGB_F, IMAGERT_SRGBA_F}, // 4 IMAGE_PRECISION_32
   {IMAGERT_SRGB_F, IMAGERT_SRGBA_F}, // 5 IMAGE_PRECISION_64
}; ASSERT(IMAGE_PRECISION_8==0 && IMAGE_PRECISION_10==1 && IMAGE_PRECISION_16==2 && IMAGE_PRECISION_24==3 && IMAGE_PRECISION_32==4 && IMAGE_PRECISION_64==5 && IMAGE_PRECISION_NUM==6);
IMAGERT_TYPE GetImageRTType(                 Bool       alpha, IMAGE_PRECISION     precision) {return GetImageRTTypeLookup[precision][alpha];}
IMAGERT_TYPE GetImageRTType(IMAGE_TYPE type, Bool allow_alpha, IMAGE_PRECISION max_precision)
{
 C ImageTypeInfo &ti=ImageTI[type]; return GetImageRTType(ti.a>=8 && allow_alpha, Min(ti.precision, max_precision)); // compare alpha as >=8 instead of >0 to treat types such as IMAGE_R10G10B10A2 as without alpha (this type is chosen for 10-bit color and ignoring alpha), because we could actually increase precision when operating on 'IMAGE_R10G10B10A2' with 'allow_alpha'=true
}
IMAGERT_TYPE GetImageRTType(IMAGE_TYPE type, Bool allow_alpha)
{
 C ImageTypeInfo &ti=ImageTI[type]; return GetImageRTType(ti.a>=8 && allow_alpha, ti.precision); // compare alpha as >=8 instead of >0 to treat types such as IMAGE_R10G10B10A2 as without alpha (this type is chosen for 10-bit color and ignoring alpha), because we could actually increase precision when operating on 'IMAGE_R10G10B10A2' with 'allow_alpha'=true
}
IMAGERT_TYPE GetImageRTType(IMAGE_TYPE type)
{
 C ImageTypeInfo &ti=ImageTI[type]; return GetImageRTType(ti.a>=8, ti.precision); // compare alpha as >=8 instead of >0 to treat types such as IMAGE_R10G10B10A2 as without alpha (this type is chosen for 10-bit color and ignoring alpha), because we could actually increase precision when operating on 'IMAGE_R10G10B10A2' with 'allow_alpha'=true
}
/******************************************************************************/
void ResetImageTypeCreateResult()
{
#if KNOWN_IMAGE_TYPE_USAGE
   REPD(rt_type, IMAGERT_NUM) // process all IMAGERT's
   {
    C ImageRTType &src=ImageRTTypes[rt_type]; REPD(ms, 2) // have to separately for multi-sampled
      {
         IMAGE_TYPE &dest=ImageRTTypesOK[ms][rt_type];
         UInt flag=(ms ? ImageTypeInfo::USAGE_IMAGE_MS : (ImageTypeInfo::USAGE_IMAGE_RT|ImageTypeInfo::USAGE_IMAGE_DS)); // for multi-sampling 'ms' we need USAGE_IMAGE_MS (this implies multisampled RT or DS), otherwise RT or DS is enough
         FREPA(src.types){IMAGE_TYPE type=src.types[i]; if(ImageTI[type].usage()&flag){dest=type; goto set;}} // find first matching
         dest=IMAGE_NONE;
      set:;
      }
   }
#else
   CopyFast(ImageRTTypesOK[0], ImageRTTypes);
   CopyFast(ImageRTTypesOK[1], ImageRTTypes);
#endif
}
void ImageRC:: zero   () {_srv_srgb=null; _rtv_srgb=null;} // don't zero '_ptr_num' here, because this is called in 'delThis', however ref count should be kept
     ImageRC:: ImageRC() {_ptr_num=0; zero();}
     ImageRC::~ImageRC() {delThis();}
void ImageRC:: delThis() // delete only this class members without super
{
#if DX11
   if(_srv_srgb || _rtv_srgb)
   {
      D.texClear(_srv_srgb);
    //SyncLocker locker(D._lock); lock not needed for DX11 'Release'
      if(D.created())
      {
         RELEASE(_srv_srgb);
         RELEASE(_rtv_srgb);
      }
   }
#endif
   zero();
}
Bool ImageRC::create(C ImageRTDesc &desc)
{
   Bool ok;
   delThis(); // delete only this without super 'del', because it's possible this image already has what 'desc' is requesting, so 'createTryEx' could do nothing as long as we're not deleting it here first
   if(ImageTI[desc._type].d) // if this is a depth buffer
   {
             ok=createTryEx(desc.size.x, desc.size.y, 1, desc._type, IMAGE_DS_RT, 1, desc.samples); // try first as a render target
   #if GL // for DX10+ IMAGE_DS_RT is the same as IMAGE_DS so don't bother checking it again
      if(!ok)ok=createTryEx(desc.size.x, desc.size.y, 1, desc._type, IMAGE_DS   , 1, desc.samples);
   #endif
   }else
   {
      ok=createTryEx(desc.size.x, desc.size.y, 1, desc._type, IMAGE_RT, 1, desc.samples);
   #if DX11
      if(ok)if(IMAGE_TYPE type_srgb=ImageTypeToggleSRGB(type()))if(type_srgb!=type()) // try creating toggled sRGB Resource Views
      {
         D3D11_SHADER_RESOURCE_VIEW_DESC srvd; Zero(srvd);
         D3D11_RENDER_TARGET_VIEW_DESC   rtvd; Zero(rtvd);
         if(multiSample()){srvd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DMS; rtvd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DMS;}
         else             {srvd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D  ; rtvd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D  ; srvd.Texture2D.MipLevels=mipMaps();}
         srvd.Format=rtvd.Format=ImageTI[type_srgb].format;
         // lock not needed for DX11 'D3D'
         D3D->CreateShaderResourceView(_txtr, &srvd, &_srv_srgb);
         D3D->CreateRenderTargetView  (_txtr, &rtvd, &_rtv_srgb);
      }
   #endif
   }
   if(ok)Time.skipUpdate();
   return ok;
}
/******************************************************************************/
void ImageRC::swapSRV()
{
#if DX11
   Swap(_srv, _srv_srgb); D.texClear(_srv_srgb); // we have to remove from tex cache, because if we're going to try to bind this as Render Target later, then DX automatically unbinds its SRV's, engine already clears cache in that case, however only for current '_srv' and not the secondary '_srv_srgb'
#endif
}
void ImageRC::swapRTV()
{
#if DX11
   Swap(_rtv, _rtv_srgb);
#endif
}
static Int CompareDesc(C ImageRC &image, C ImageRTDesc &desc)
{
   if(Int c=Compare(image.w      (), desc.size.x ))return c;
   if(Int c=Compare(image.h      (), desc.size.y ))return c;
   if(Int c=Compare(image.hwType (), desc._type  ))return c; // !! have to compare 'hwType' instead of 'type', because 'hwType' is always non-sRGB (same as 'desc._type') while 'type' can be sRGB-toggled in 'swapSRGB' !!
   if(Int c=Compare(image.samples(), desc.samples))return c;
   return 0;
}
void ImageRC::swapSRGB()
{
#if DX11
   swapSRV(); swapRTV(); _type=ImageTypeToggleSRGB(type()); // !! have to toggle 'type' and not 'hwType' because 'CompareDesc' and 'Set' expect that !!
#endif
}
static void Set(ImageRTPtr &p, ImageRC &rt, Bool want_srgb) // this is called only when "_ptr_num==0"
{
#if CAN_SWAP_SRGB
   if(want_srgb!=IsSRGB(rt.type()))rt.swapSRGB(); // !! explicitly check 'type' and not 'hwType' and not 'rt.sRGB', because 'hwType' is always non-sRGB (same as 'desc._type') while 'type' can be sRGB toggled in 'swapSRGB' !!
#endif
   rt._ptr_num++; p._data=&rt; rt.discard();
}
/******************************************************************************/
ImageRTPtr& ImageRTPtr::clear()
{
   if(_data)
   {
      DEBUG_ASSERT(_data->_ptr_num, "ImageRC._ptr_num should be >0");
          _data->_ptr_num--;
      if(!_data->_ptr_num)_data->discard();
          _data=null;
   }
   return T;
}
ImageRTPtr& ImageRTPtr::operator=(C ImageRTPtr &p)
{
   if(T!=p)
   {
      clear();
      if(T._data=p._data){T._data->_ptr_num++; T._last_index=p._last_index;} // assign to new
   }
   return T;
}
ImageRTPtr& ImageRTPtr::operator=(ImageRC *p)
{
   if(T!=p)
   {
      clear();
      if(T._data=p)T._data->_ptr_num++; // assign to new
   }
   return T;
}
ImageRTPtr::ImageRTPtr(C ImageRTPtr &p)
{
   if(T._data=p._data){T._data->_ptr_num++; T._last_index=p._last_index;}else T._last_index=-1;
}
ImageRTPtr::ImageRTPtr(ImageRC *p)
{
   if(T._data=p)T._data->_ptr_num++;else T._last_index=-1;
}
/******************************************************************************/
Bool ImageRTPtr::find(C ImageRTDesc &desc)
{
   clear(); // clear first so we can find the same Image if possible

   Bool multi_sample=(desc.samples>1);
#if KNOWN_IMAGE_TYPE_USAGE
   ConstCast(desc._type)=ImageRTTypesOK[multi_sample][desc.rt_type];
#else
   ImageRTType    &types=ImageRTTypesOK[multi_sample][desc.rt_type];
again:
   ConstCast(desc._type)=types.types[0];
#endif

   Bool want_srgb=false;
#if CAN_SWAP_SRGB // try to create non-sRGB, and later swap sRGB views, this allows to potentially reduce amount of needed Render Targets, since they can reuse each other memory (instead of using 1 RGB and 1 sRGB, we can just allocate 1 RGB, and swap its views if needed)
   IMAGE_TYPE non_srgb=ImageTypeRemoveSRGB(desc._type); if(desc._type!=non_srgb) // if we're requesting sRGB
   {
      ConstCast(desc._type)=non_srgb; // request non-sRGB
      want_srgb=true; // and remember that we want sRGB
   }
#endif

   Bool found; if(InRange(_last_index, Renderer._rts) && !CompareDesc(Renderer._rts[_last_index], desc))found=true; // in range and matches ("!CompareDesc" -> match)
   else found=Renderer._rts.binarySearch(desc, _last_index, CompareDesc);

   if(found)
   {
      // check '_last_index' first
      ImageRC &rt=Renderer._rts[_last_index];
      if(rt.available()){Set(T, rt, want_srgb); return true;}

      // check all neighbors with the same desc
      for(Int i=_last_index-1;         i>=0             ; i--) {ImageRC &rt=Renderer._rts[i]; if(CompareDesc(rt, desc))break; if(rt.available()){T._last_index=i; Set(T, rt, want_srgb); return true;}}
      for(Int i=_last_index+1; InRange(i, Renderer._rts); i++) {ImageRC &rt=Renderer._rts[i]; if(CompareDesc(rt, desc))break; if(rt.available()){T._last_index=i; Set(T, rt, want_srgb); return true;}}
   }
#if KNOWN_IMAGE_TYPE_USAGE
   if(desc._type) // check this after 'found' because in most cases we will already return from codes above
   { // since we have KNOWN_IMAGE_TYPE_USAGE, and a valid type, then we assume that this should always succeed
      ImageRC &rt=Renderer._rts.NewAt(_last_index); if(rt.create(desc)){Set(T, rt, want_srgb); return true;}
      Exit(S+"Can't create Render Target "+desc.size.x+'x'+desc.size.y+' '+ImageRTName[desc.rt_type]+", samples:"+desc.samples);
   }
#else
   ImageRC temp; // try to create first as a standalone variable (not in 'Renderer._rts') in case it fails so we don't have to remove it
   if(temp.create(desc)){ImageRC &rt=Renderer._rts.NewAt(_last_index); Swap(rt, temp); Set(T, rt, want_srgb); return true;}
   // fail
   if(desc._type!=IMAGE_NONE) // try another type, and don't try this again
   {
      MoveFastN(&types.types[0], &types.types[1], ELMS(types.types)-1); // move all elements from index 1 and right, to the left by 1, to index 0..
      types.types[ELMS(types.types)-1]=IMAGE_NONE; // set last type as none
      if(types.types[0]!=IMAGE_NONE)goto again; // try the new type
   }
#endif
   return false;
}
/******************************************************************************/
ImageRTPtr& ImageRTPtr::get(C ImageRTDesc &desc)
{
   if(!find(desc))Exit(S+"Can't create Render Target "+desc.size.x+'x'+desc.size.y+' '+ImageRTName[desc.rt_type]+", samples:"+desc.samples);
   return T;
}
ImageRTPtr& ImageRTPtr::getDS(Int w, Int h, Byte samples, Bool reuse_main)
{
   clear(); // clear first so we can find the same Image if possible
   if(reuse_main)
   {
      ImageRC &ds=Renderer._main_ds, *cur_ds=Renderer._cur_main_ds;
   #if GL
      // on OpenGL we can't reuse '_main_ds' because it has different flipping orientation as it is always paired with '_main' in the main FBO
      // <- here don't check for 'ds'
      if(cur_ds==&ds)cur_ds=null; // if 'cur_ds' is 'ds' then don't check it either
   #else
      if(              /*ds .available() && */    ds .accessible() &&     ds .w()==w &&     ds .h()==h &&     ds .samples()==samples){T=   &ds; return T;} // if ds is not used (actually don't check this because '_gui_ds' can be set to it at the start of each frame), accessible and compatible then we can use it
   #endif
      if(cur_ds && /*cur_ds->available() && */cur_ds->accessible() && cur_ds->w()==w && cur_ds->h()==h && cur_ds->samples()==samples){T=cur_ds; return T;} // if ds is not used (actually don't check this because '_gui_ds' can be set to it at the start of each frame), accessible and compatible then we can use it
   }
   get(ImageRTDesc(w, h, IMAGERT_DS, samples)); return T;
}
/******************************************************************************/
Bool        ImageRTPtr::find(Int w, Int h, IMAGERT_TYPE rt_type, Byte samples) {return find(ImageRTDesc(w, h, rt_type, samples));}
ImageRTPtr& ImageRTPtr:: get(Int w, Int h, IMAGERT_TYPE rt_type, Byte samples) {return  get(ImageRTDesc(w, h, rt_type, samples));}
/******************************************************************************/
}
/******************************************************************************/
