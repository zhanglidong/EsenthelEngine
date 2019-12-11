/******************************************************************************

   Have to keep this as a separate file, so it won't be linked if unused.
   Because it's linked separately, its name can't include spaces (due to Android building toolchain).

/******************************************************************************/
#include "stdafx.h"

#include "C:/Esenthel/ThirdPartyLibs/begin.h"
#include "C:/Esenthel/ThirdPartyLibs/Waifu2x/lib/src/w2xconv.h"
#include "C:/Esenthel/ThirdPartyLibs/Waifu2x/lib/src/modelHandler.hpp"
#include "C:/Esenthel/ThirdPartyLibs/end.h"

namespace EE{
/******************************************************************************/
static struct WaifuClass
{
   w2xc::W2XConv *waifu=null;
   Bool           tried=false;
   Mems<Byte>     data; // this keeps the data needed for waifu to work
   SyncLock       lock;

  ~WaifuClass()
   {
      if(waifu)
      {
         w2xc::w2xconv_fini(waifu);
         waifu=null;
      }
   }
   static void ConvertModel(C Str &json, C Str &bin)
   {
      TextData td;
      if(!td.loadJSON(json))Exit("Can't open model JSON");
      if( td.nodes.elms()!=1)Exit("invalid nodes");
      TextNode &models=td.nodes.first();
      File f; f.writeMem();
      f.putUInt(models.nodes.elms());
      Int last_output;
      FREPA(models.nodes)
      {
         TextNode &model=models.nodes[i];
         TextNode * nInputPlane=model.findNode( "nInputPlane"); if(! nInputPlane)Exit("fail"); Int  inputPlanes= nInputPlane->asInt(); f.putUInt( inputPlanes); if(i && inputPlanes!=last_output)Exit("input!=last_output");
         TextNode *nOutputPlane=model.findNode("nOutputPlane"); if(!nOutputPlane)Exit("fail"); Int outputPlanes=nOutputPlane->asInt(); f.putUInt(outputPlanes);
         TextNode *kW          =model.findNode("kW"); if(!kW)Exit("kW");
         TextNode *kH          =model.findNode("kH"); if(!kH)Exit("kH");
         int kernelSize=kW->asInt(); if(kernelSize!=kH->asInt())Exit("kernel not square"); if(kernelSize!=3)Exit("kernelSize not 3"); f.putUInt(kernelSize);

         TextNode *weight=model.findNode("weight"); if(!weight)Exit("weight");
         if(weight->nodes.elms()!=outputPlanes)Exit("outputPlanes");
         FREPA(weight->nodes)
         {
            TextNode &outputPlane=weight->nodes[i];
            if(outputPlane.nodes.elms()!=inputPlanes)Exit("inputPlanes");
            FREPA(outputPlane.nodes)
            {
               TextNode &weight=outputPlane.nodes[i];
               if(weight.nodes.elms()!=kernelSize)Exit("weight kernelSize");
               FREPA(weight.nodes)
               {
                  TextNode &weightRow=weight.nodes[i];
                  if(weightRow.nodes.elms()!=kernelSize)Exit("weight kernelSize");
                  FREPA(weightRow.nodes)
                  {
                     TextNode &w=weightRow.nodes[i];
                     f<<w.asFlt();
                  }
               }
            }
         }

         TextNode *bias=model.findNode("bias"); if(!bias || bias->nodes.elms()!=outputPlanes)Exit("bias");
         FREPA(bias->nodes)
         {
            TextNode &b=bias->nodes[i];
            f<<b.asFlt();
         }

         last_output=outputPlanes;
      }
      f.pos(0);
      Exit(SafeOverwrite(f, bin) ? "OK" : "save failed");
   }
   static void convert()
   {
      ConvertModel("C:/Esenthel/ThirdPartyLibs/Waifu2x/lib/models_rgb/scale2.0x_model.json", "C:/Esenthel/Data/Waifu/scale2.0x_model.dat");
   }
   Bool init()
   {
   #if 0
      #pragma message("!! Warning: Use this only one time !!")
      convert();
   #endif
      if(!tried)
      {
         SyncLocker locker(lock); if(!tried)
         {
            File f; if(f.readTry("Waifu/scale2.0x_model.dat"))
            {
               data.setNum(f.size());
               if(f.getFast(data.data(), data.elms()))
               {
                  auto temp=w2xc::w2xconv_init_with_tta(w2xc::W2XCONV_GPU_DISABLE, Cpu.threads(), 0, false);
                  f.readMem(data.data(), data.elms());
                  temp->scale2_models.resize(f.getUInt());
                  for(Int i=0; i<temp->scale2_models.size(); i++)
                  {
                     auto &model=temp->scale2_models[i];
                     model. nInputPlanes=f.getUInt();
                     model.nOutputPlanes=f.getUInt();
                     model.kernelSize   =f.getUInt(); DYNAMIC_ASSERT(model.kernelSize==3, "kernel size!=3");
                     // just store pointers to 'f' data, which actually points to 'data'
                     model.weights=(w2xc::Model::Weight*)f.memFast(); f.skip(SIZE(w2xc::Model::Weight)*model.getNInputPlanes()*model.getNOutputPlanes());
                     model. biases=(float              *)f.memFast(); f.skip(SIZE(Flt                )*model.biases_size());
                  }
                  DYNAMIC_ASSERT(f.end() && f.ok(), "invalid waifu data");
                  waifu=temp; // set once fully initialized
               }
            }
            tried=true; // set at the end
         }
      }
      return waifu!=null;
   }
   Bool process(Image &image, Bool clamp)
   {
      SyncLocker locker(lock); // waifu is not thread-safe (however it operates on a thread pool)
      if(waifu)
      {
         return w2xc::w2xconv_2x_rgb_f32_esenthel(waifu, image.data(), image.pitch(), image.w(), image.h(), clamp);
      }
      return false;
   }
}Waifu;
/******************************************************************************/
Bool _ResizeWaifu(C Image &src, Image &dest, Bool clamp) // assumes that images are not compressed, processes locked mip-map only
{
   if(Waifu.init())
   {
      Image temp[2];
    C Image *s=&src;
      Int i=0;
      do{
         Image &dest=temp[i]; i^=1;
         if(!dest.createSoftTry(s->lw()*2, s->lh()*2, 1, IMAGE_F32_3))return false;
         REPD(y, dest.lh())
         REPD(x, dest.lw())dest.colorF(x, y, s->colorF(x/2, y/2));
         if(!Waifu.process(dest, clamp))return false;
         s=&dest;
      }while(s->lw()<dest.lw() || s->lh()<dest.lh());
      return dest.injectMipMap(*s, dest.lMipMap(), dest.lCubeFace(), FILTER_BEST, (clamp?IC_CLAMP:IC_WRAP)|IC_IGNORE_GAMMA);
      // FIXME process alpha channel
   }
   return false;
}
/******************************************************************************/
}
/******************************************************************************/
