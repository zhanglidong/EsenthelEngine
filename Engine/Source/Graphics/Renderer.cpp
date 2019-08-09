/******************************************************************************/
#include "stdafx.h"
#include "../Shaders/!Header CPU.h"
namespace EE{
#define SVEL_CLEAR Vec4Zero
#define  VEL_CLEAR Vec4(0.5f, 0.5f, 0.5f, 0)
#define SNRM_CLEAR Vec4(0   , 0   , -1, 0) // set Z to 0   to set VecZero normals, however Z set to -1 makes ambient occlusion more precise when it uses normals (because ambient occlusion will not work good on the VecZero normals)
#define  NRM_CLEAR Vec4(0.5f, 0.5f,  0, 0) // set Z to 0.5 to set VecZero normals, however Z set to  0 makes ambient occlusion more precise when it uses normals (because ambient occlusion will not work good on the VecZero normals)
#define  NRM_CLEAR_START 1 // 1 works faster on GeForce 650m GT, TODO: check on newer hardware
inline Bool ClearNrm() {return D.aoWant() && D.ambientNormal() || Renderer.stage==RS_NORMAL;}
/******************************************************************************

   Graphics API differences:
      Reading and Writing to the same Render Target - Yes: GL         No: DX10+
      Deferred Multi-Sampling                       - Yes: DX>=10.1   No: DX10.0, GL

/******************************************************************************/
RendererClass Renderer;
MeshRender    MshrBox ,
              MshrBoxR,
              MshrBall;
/******************************************************************************/
static void InitMshr()
{
   MeshBase mshb;

   mshb.createFast(Box(1));
                   MshrBox .create(mshb);
   mshb.reverse(); MshrBoxR.create(mshb);

   mshb.create(Ball(1), 0, 12);
   MshrBall.create(mshb);
}
static void ShutMshr()
{
   MshrBox .del();
   MshrBoxR.del();
   MshrBall.del();
}
/******************************************************************************/
RendererClass::RendererClass() : highlight(null), material_color_l(null)
{
#if 0 // there's only one 'RendererClass' global 'Renderer' and it doesn't need clearing members to zero
   stage=RS_DEFAULT;
   combine=false;
   wire=false;
   clear_color.zero();
   ms_samples_color.zero();
   target=null;

  _mode=RM_SOLID;
  _mesh_blend_alpha=ALPHA_NONE;
  _has_glow=_fur_is=_mirror=_mirror_want=_mirror_shadows=_palette_mode=_eye_adapt_scale_cur=_t_measure=_set_depth_needed=_get_target=_stereo=_mesh_early_z=_mesh_shader_vel=false;
  _outline=_clear=0;
  _mirror_priority=_mirror_resolution=0;
  _frst_light_offset=_blst_light_offset=0;
  _shd_range=0;
  _res.zero();
  _clip.zero();
  _mirror_plane.zero();
  _shader_early_z=_shader_shd_map=_shader_shd_map_skin=null;
  _render=null;
  _shader_param_changes=null;
   REPAO(_t_measures)=0;
  _t_last_measure=0;
   REPAO(_t_reflection)=0; REPAO(_t_prepare)=0; REPAO(_t_solid)=0; REPAO(_t_overlay)=0; REPAO(_t_water)=0; REPAO(_t_light)=0; REPAO(_t_sky)=0; REPAO(_t_edge_detect)=0; REPAO(_t_blend)=0; REPAO(_t_palette)=0; REPAO(_t_behind)=0; REPAO(_t_rays)=0; REPAO(_t_refract)=0; REPAO(_t_volumetric)=0; REPAO(_t_post_process)=0; REPAO(_t_gpu_wait)=0;
#endif

  _solid_mode_index=RM_SOLID;

   lowest_visible_point=-DBL_MAX;

  _first_pass=true;
  _eye=0; _eye_num=1;

  _type=_cur_type=(MOBILE ? RT_FORWARD : RT_DEFERRED);
  _forward_prec=!MOBILE;

  _mesh_blend_alpha  =ALPHA_BLEND_FACTOR;
  _mesh_stencil_value=STENCIL_REF_ZERO;
  _mesh_stencil_mode =STENCIL_NONE;
  _mesh_highlight    .zero();
  _mesh_draw_mask    =0xFFFFFFFF;
   SetVariation(0);

#if DX11
  _cull_mode[0]=0;
  _cull_mode[1]=1;
#endif

#if WEB // #WebSRGB
  _gui   =_cur_main   =&_main_temp;
  _gui_ds=_cur_main_ds=&_main_temp_ds;
#else
  _gui   =_cur_main   =&_main;
  _gui_ds=_cur_main_ds=&_main_ds;
#endif
}
void RendererClass::del()
{
   Sky   .del();
   Astros.del();
   Clouds.del();
   Water .del();

   ShutInstances();
   ShutMshr     ();
   ShutParticles();
   ShutMesh     ();
   ShutMaterial ();
   ShutLight    ();

   rtDel();
}
void RendererClass::create()
{
   if(LogInit)LogN("RendererClass.create");

#if 0 // convert DDS from SMAA source to Esenthel Image format
   Str in="D:/!/SMAA/", out="C:/Esenthel/Data/Img/";
   Image img;
   img.Import(in+"SearchTex.dds", -1, IMAGE_2D); img._type=IMAGE_R8;
   img.save(out+"SMAA Search.img");
   img.Import(in+"AreaTexDX10.dds", IMAGE_R8G8, IMAGE_2D);
   img.save(out+"SMAA Area.img");
#endif

#if SUPPORT_EARLY_Z
  _shader_early_z     =DefaultShaders(&MaterialDefault, VTX_POS         , 0, false).EarlyZ();
#endif
  _shader_shd_map     =DefaultShaders(&MaterialDefault, VTX_POS         , 0, false).Shadow();
  _shader_shd_map_skin=DefaultShaders(&MaterialDefault, VTX_POS|VTX_SKIN, 0, false).Shadow();

   InitCamera   ();
   InitLight    ();
   InitMaterial ();
   InitFur      ();
   InitMshr     ();
   InitInstances();

   Sky   .create();
   Clouds.create();
   Water .create();
}
RendererClass& RendererClass::type(RENDER_TYPE type)
{
	Clamp(type, RENDER_TYPE(0), RENDER_TYPE(RT_NUM-1));
   if(type==RT_DEFERRED && D.deferredUnavailable())return T;
   if(T._type!=type)
   {
      T._type=T._cur_type=type;
      if(type==RT_DEFERRED && D.deferredMSUnavailable())D.samples(1); // disable multi-sampling if we can't support it
      D.setShader(); // needed because shaders are set only for current renderer type
   }
   return T;
}
void RendererClass::mode(RENDER_MODE mode)
{
   T._mode            =mode;
   T._palette_mode    =(mode==RM_PALETTE || mode==RM_PALETTE1);
   T._mesh_shader_vel =(_vel && (mode==RM_SOLID || mode==RM_BLEND));
   T._solid_mode_index=(mirror() ? RM_SOLID_M : RM_SOLID);
#if DX11
   T._cull_mode[1]    =((mirror() && mode!=RM_SHADOW) ? 2 : 1);
#elif GL
   D.cullGL();
#endif
   Bool cull=D._cull; D.cull(false); D.cull(cull); // force reset
   MaterialClear(); // must be called when changing rendering modes, because when setting materials, we may set only some of their shader values depending on mode
}
RendererClass& RendererClass::forwardPrecision(Bool per_pixel)
{
   if(T._forward_prec!=per_pixel)
   {
      T._forward_prec=per_pixel;
      D.setShader();
   }
   return T;
}
/******************************************************************************/
void RendererClass::requestMirror(C PlaneM &plane, Int priority, Bool shadows, Int resolution)
{
   if(!_mirror_want || priority>T._mirror_priority)
   {
     _mirror_want      =true;
     _mirror_priority  =priority;
     _mirror_plane     =plane;
     _mirror_shadows   =shadows;
     _mirror_resolution=resolution;
   }
}
/******************************************************************************/
void RendererClass::linearizeDepth(ImageRT &dest, ImageRT &depth)
{
   D.alpha(ALPHA_NONE);
   set(&dest, null, true);
   if(!depth.multiSample() || depth.size()!=dest.size()){Sh.Depth  ->set(depth); Sh.LinearizeDepth[0][FovPerspective(D.viewFovMode())]->draw(); Sh.Depth  ->set(_ds_1s);}else // 1s->1s, set and restore depth, if we're resizing then we also need to use the simple version
   if(!dest .multiSample()                             ){Sh.DepthMS->set(depth); Sh.LinearizeDepth[1][FovPerspective(D.viewFovMode())]->draw(); Sh.DepthMS->set(_ds   );}else // ms->1s, set and restore depth
                                                        {Sh.DepthMS->set(depth); Sh.LinearizeDepth[2][FovPerspective(D.viewFovMode())]->draw(); Sh.DepthMS->set(_ds   );}     // ms->ms, set and restore depth
}
void RendererClass::setDepthForDebugDrawing()
{
   if(_set_depth_needed)
   {
     _set_depth_needed=false;
      if(_ds_1s)if(Shader *shader=Sh.SetDepth)
      {
         ImageRT *rt=_cur[0]; set(null, _cur_ds, true);
         ALPHA_MODE alpha=D.alpha(ALPHA_NONE); D.depthLock  (true); D.depthFunc(FUNC_ALWAYS); shader->draw();
                          D.alpha(alpha     ); D.depthUnlock(    ); D.depthFunc(FUNC_LESS  );
         set(rt, _cur_ds, true);
      }
   }
}
ImageRTPtr RendererClass::getBackBuffer() // this may get called during rendering and outside of it
{
   if(Image *src=_cur[0])
   {
      ImageRTPtr hlp(ImageRTDesc(src->w(), src->h(), IMAGERT_SRGBA)); // here Alpha is used for storing opacity
      src->copyHw(*hlp, true);
      return hlp;
   }
   return null;
}
void RendererClass::adaptEye(ImageRT &src, ImageRT &dest, Bool dither)
{
   Hdr.load();
   ImageRTPtr temp=&src;
   VecI2    size=RoundPos(fx()*(D.viewRect().size()/D.size2())); // calculate viewport size in pixels
   Int  max_size=size.min()/4;
   Int  s=1, num=1; for(;;){Int next_size=s*4; if(next_size>max_size)break; s=next_size; num++;} // go from 1 up to 'max_size', inrease *4 in each step
   FREP(num) // now go backwards, from up to 'max_size' to 1 inclusive
   {
      ImageRTPtr next=temp; next.get(ImageRTDesc(s, s, IMAGERT_F32)); s/=4; // we could use 16-bit as according to calculations, the max error for 1920x1080, starting with 256x256 as first step and going down to 1x1, with average luminance of 1.0 (255 byte) is 0.00244140625 at the final stage, which gives 410 possible colors, however we may use some special tricks in the shader that requires higher precision (for example BRIGHT with Sqr and Sqrt later, or use Linear/sRGB)
      set(next, null, false);
      Sh.imgSize(*temp);
      if(i){Sh.ImgXF[0]->set(temp); Hdr.HdrDS[1]->draw();}
      else {Sh.Img  [0]->set(temp); Hdr.HdrDS[0]->draw(null, D.screenToUV(D.viewRect()));}
      temp=next;
   }
   Sh.Step->set(Pow(Mid(1/D.eyeAdaptationSpeed(), EPS, 1.0f), Time.d())); // can use EPS and not EPS_GPU because we're using Pow here and not on GPU
   Sh.ImgXF[0]->set(temp); Sh.ImgXF[1]->set(_eye_adapt_scale[_eye_adapt_scale_cur]); _eye_adapt_scale_cur^=1; _eye_adapt_scale[_eye_adapt_scale_cur].discard(); set(&_eye_adapt_scale[_eye_adapt_scale_cur], null, false); Hdr.HdrUpdate                                                  ->draw();
                           Sh.ImgX [0]->set(_eye_adapt_scale[_eye_adapt_scale_cur]);                                                                            set(&dest                                  , null, true ); Hdr.Hdr[dither && src.highPrecision() && !dest.highPrecision()]->draw(src);
}
INLINE Shader* GetBloomDS(Bool glow, Bool uv_clamp, Bool half_res, Bool saturate, Bool gamma) {Shader* &s=Sh.BloomDS[glow][uv_clamp][half_res][saturate][gamma]; if(SLOW_SHADER_LOAD && !s)s=Sh.getBloomDS(glow, uv_clamp, half_res, saturate, gamma); return s;}
INLINE Shader* GetBloom  (Bool dither, Bool gamma                                           ) {Shader* &s=Sh.Bloom  [dither][gamma]                            ; if(SLOW_SHADER_LOAD && !s)s=Sh.getBloom  (dither, gamma                            ); return s;}
// !! Assumes that 'ImgClamp' was already set !!
void RendererClass::bloom(ImageRT &src, ImageRT &dest, Bool dither)
{
   // process bloom in sRGB gamma, because it will provide sharper results

   Bool gamma=LINEAR_GAMMA, swap=(gamma && src.canSwapSRV() && dest.canSwapRTV()); if(swap){gamma=false; src.swapSRV(); dest.swapRTV();}

   const Int     shift=(D.bloomHalf() ? 1 : 2);
   ImageRTDesc   rt_desc(fxW()>>shift, fxH()>>shift, IMAGERT_RGB); // using IMAGERT_RGB will clip to 0..1 range !! using high precision would require clamping in the shader to make sure values don't go below 0 !!
   ImageRTPtrRef rt0(D.bloomHalf() ? _h0 : _q0); rt0.get(rt_desc);
   ImageRTPtrRef rt1(D.bloomHalf() ? _h1 : _q1); rt1.get(rt_desc); Bool discard=false; // we've already discarded in 'get' so no need to do it again

   if(_has_glow || D.bloomScale()) // if we have something there
   {
      set(rt0, null, false);

      Rect ext_rect, *rect=null; // set rect, after setting render target
      if(!D._view_main.full){ext_rect=D.viewRect(); rect=&ext_rect.extend(Renderer.pixelToScreenSize((D.bloomMaximum()+D.bloomBlurs())*SHADER_BLUR_RANGE+1));} // when not rendering entire viewport, then extend the rectangle, add +1 because of texture filtering, have to use 'Renderer.pixelToScreenSize' and not 'D.pixelToScreenSize'

      Bool half_res=(Flt(src.h())/rt0->h() <= 2.5f); // half_res=scale 2, ..3.., quarter=scale 4, 2.5 was the biggest scale that didn't cause jittering when using half down-sampling
      const Int  res=(half_res ? 2 : 4);
      const Bool gamma_per_pixel=false; // !! must be the same as in shader !!

      Sh.BloomParams->setConditional(Vec(D.bloomOriginal(), _has_glow ? D.bloomScale()/((gamma && !gamma_per_pixel) ? res : Sqr(res)) // for "gamma && !gamma_per_pixel" use only res, because "LinearToSRGBFast(c.rgb/(res*res)) == LinearToSRGBFast(c.rgb)/Sqrt(res*res) == LinearToSRGBFast(c.rgb)/res"
                                                           : half_res ? D.bloomScale()
                                                                      : D.bloomScale()/(gamma ? 2 : 4),
                                                                       -D.bloomCut()*D.bloomScale()));
      Sh.imgSize( src); GetBloomDS(_has_glow, !D._view_main.full, half_res, D.bloomSaturate() || !D._bloom_cut, gamma)->draw(src, rect); // we can enable saturation (which is faster) if cut is zero, because zero cut won't change saturation
    //Sh.imgSize(*rt0); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
      if(D.bloomMaximum())
      { // 'discard' before 'set' because it already may have requested discard, and if we 'discard' manually after 'set' then we might discard 2 times
                         set(rt1, null, false); Sh.MaxX->draw(rt0, rect); discard=true; // discard next time
         rt0->discard(); set(rt0, null, false); Sh.MaxY->draw(rt1, rect);
      }
      REP(D.bloomBlurs())
      { // 'discard' before 'set' because it already may have requested discard, and if we 'discard' manually after 'set' then we might discard 2 times
         if(discard)rt1->discard(); set(rt1, null, false); Sh.BlurX[D.bloomSamples()]->draw(rt0, rect); discard=true; // discard next time
                    rt0->discard(); set(rt0, null, false); Sh.BlurY[D.bloomSamples()]->draw(rt1, rect);
      }
   }else
   {
      rt0->clearFull();
   }
   set(&dest, null, true);
   Sh.Img[1]->set(rt0);
   GetBloom(dither /*&& (src.highPrecision() || rt0->highPrecision())*/ && !dest.highPrecision(), gamma)->draw(src); // merging 2 RT's ('src' and 'rt0') with some scaling factors will give us high precision
   if(swap){src.swapSRV(); dest.swapRTV();} // restore
}
static Flt PixelsToScale(Flt pixels, Int res) {return pixels*2/res;} // 'pixels=max blur range in pixels in one direction, 'res'=total resolution
static Flt ScaleToPixels(Flt scale , Int res) {return scale*res/2 ;}
static void SetMotionBlurParams(Flt pixels) // !! this needs to be called when the RT is 'D.motionRes' sized because it needs that size and not the full size !!
{
   // see "C:\Users\Greg\SkyDrive\Code\Tests\Motion Blur.cpp"
   const Flt scale=PixelsToScale(MAX_MOTION_BLUR_PIXEL_RANGE, 1080); // scale should be small, because inside the shader we do "x/(1 +- blur.z)"
   const Flt limit=pixels/MAX_MOTION_BLUR_PIXEL_RANGE; // if we're using only 'pixels' then we have to limit from the full 0..1 range to the fraction

   Vec2 viewport_center=D._view_active.recti.centerF()/Renderer.res(), size2=D._unscaled_size*(2/scale)*limit;
   // pos=(inTex-viewport_center)*size2;
   // pos=inTex*size2 - viewport_center*size2;
   Mtn.MotionUVMulAdd     ->setConditional(Vec4(size2.x, size2.y, -viewport_center.x*size2.x, -viewport_center.y*size2.y));

   Mtn.MotionVelScaleLimit->setConditional(Vec4(D.scale()/D.viewFovTanFull().x*limit, -D.scale()/D.viewFovTanFull().y*limit, scale, limit));
   Mtn.MotionPixelSize    ->setConditional(Flt(MAX_MOTION_BLUR_PIXEL_RANGE)/Renderer.res()); // the same value is used for 'SetDirs' (D.motionRes) and 'Blur' (D.res)
}
// !! Assumes that 'ImgClamp' was already set !!
Bool RendererClass::motionBlur(ImageRT &src, ImageRT &dest, Bool dither)
{
   if(stage==RS_VEL && show(_vel, false, D.signedVelRT()))return true;

   Mtn.load();

   Bool camera_object=(_vel!=null); // remember blur mode because it depends on '_vel' which gets cleared

   VecI2 res;
   res.y=Min(ByteScaleRes(fxH(), D._mtn_res), 1080); // only up to 1080 is supported, because shaders support only up to MAX_MOTION_BLUR_PIXEL_RANGE pixels, but if we enable higher resolution then it would require more pixels
   res.x=Max(1, Round(res.y*D._unscaled_size.div())); // calculate proportionally to 'res.y' and current mode aspect (do not use 'D.aspectRatio' because that's the entire monitor screen aspect, and not application window), all of this is needed because we need to have square pixels for motion blur render targets, however the main application resolution may not have square pixels
   const Flt pixels=res.y*(Flt(MAX_MOTION_BLUR_PIXEL_RANGE)/1080);
   const Int dilate_round_range=1; // this value should be the same as "Int range" in "Dilate_PS" Motion Blur shader
         Int dilate_round_steps;
   switch(D.motionDilate()) // get round dilate steps 
   {
      default:
      case DILATE_ORTHO :
      case DILATE_ORTHO2: dilate_round_steps=                                    0; break; // zero round steps
      case DILATE_MIXED : dilate_round_steps=Round(pixels/dilate_round_range*0.3f); break; // 0.3f was chosen to achieve quality/performance that's between orthogonal and round mode
      case DILATE_ROUND : dilate_round_steps=Round(pixels/dilate_round_range     ); break; // 'Round' should be enough
   }
   Int  dilate_round_pixels=dilate_round_steps*dilate_round_range, dilate_ortho_pixels=Max(Round(pixels)-dilate_round_pixels, 0);
   Bool diagonal=(D.motionDilate()==DILATE_ORTHO2);
 C MotionBlur::Pixel *ortho=Mtn.pixel(dilate_ortho_pixels, diagonal); if(ortho)dilate_ortho_pixels=ortho->pixels; // reset 'dilate_ortho_pixels' because it can actually be bigger based on what is supported
   const Int total_pixels=dilate_round_pixels+dilate_ortho_pixels; // round+ortho
   DEBUG_ASSERT(D.motionDilate()==DILATE_ROUND ? dilate_ortho_pixels==0 : true, "Ortho should be zero in round mode");

   ImageRTDesc rt_desc(res.x, res.y, D.signedVelRT() ? IMAGERT_RGB_S : IMAGERT_RGB); // Alpha not used (XY=Dir, Z=Dir.length)
   ImageRTPtr  converted(rt_desc);
   Shader     *shader;
   if(camera_object)shader=Mtn.Convert[true ][!D._view_main.full];else
   {                shader=Mtn.Convert[false][!D._view_main.full];
                    SetFastVel();
   }
   set(converted, null, false);
   Rect ext_rect, *rect=null;
   if(D._view_main.full)REPS(_eye, _eye_num)
   {
      Rect *eye_rect=setEyeParams();
      SetMotionBlurParams(pixels); // call after 'setEyeParams' because we need to set 'D._view_active'
      shader->draw(_vel, eye_rect);
   }else
   {
      SetMotionBlurParams(pixels);
      ext_rect=D.viewRect(); rect=&ext_rect.extend(Renderer.pixelToScreenSize(total_pixels+1)); // when not rendering entire viewport, then extend the rectangle because of 'Dilate' and 'SetDirs' checking neighbors, add +1 because of texture filtering, we can ignore stereoscopic there because that's always disabled for not full viewports, have to use 'Renderer.pixelToScreenSize' and not 'D.pixelToScreenSize'
      shader->draw(_vel, rect);
   }
  _vel.clear();

   if(stage==RS_VEL_CONVERT && show(converted, false, D.signedVelRT()))return true;

   ImageRTPtr dilated=converted, helper;

   if(camera_object) // we apply Dilation only in MOTION_CAMERA_OBJECT mode, for MOTION_CAMERA it's not needed
   {
      rt_desc.rt_type=(D.signedVelRT() ? IMAGERT_RGB_S : IMAGERT_RGB); // Alpha not used (XY=Dir, Z=Max Dir length of all nearby pixels)
      // we need to apply Dilation, for example, if a ball object has movement, then it should be blurred, and also pixels around the ball should be blurred too
      // however velocity for pixels around the ball (the background) may be zero, so we need to apply dilation and apply the velocity onto neighboring pixels
      // remember that it doesn't make sense to perform depth based tests on not first steps "dilated!=converted",
      // because the depth tests compare only center pixels and the pixels around it, however if in step #0 we dilate velocity from pixel with X coordinate=2,
      // onto pixel X=1, then in step #1, we dilate pixel X=1 onto X=0 (the velocity is carried over from pixel X=2 into X=0, however we don't store information
      // about what was the depth of that velocity, so in step #1 we're comparing depth of pixel X=0 with X=1, however we're using velocity from X=2,
      // so we should have information about X=2 depth, however there's no easy way to do that
      // TODO: check if depth tests are useful for the first step ("dilated==converted")

    //Sh.imgSize(*dilated); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
      if(ortho) // do orthogonal first (this will result in slightly less artifacts when the camera is moving)
      {
         helper .get(rt_desc); set(helper , null, false); ortho->DilateX[diagonal]->draw(dilated, rect);
         dilated.get(rt_desc); set(dilated, null, false); ortho->DilateY[diagonal]->draw(helper , rect);
      }
      REP(dilate_round_steps)
      {
         if(!helper || helper==converted)helper.get(rt_desc);else helper->discard(); // don't write to original 'converted' in the next step, because we need it later
         set(helper, null, false); Mtn.Dilate->draw(dilated, rect); Swap(dilated, helper);
      }
   }
   if(stage==RS_VEL_DILATED && show(dilated, false, D.signedVelRT()))return true;

   // check how far can we go (remove leaks)
   Sh.ImgXY ->set(converted);
   Sh.Img[0]->set(dilated  );
   rt_desc.rt_type=(D.signedVelRT() ? IMAGERT_RGBA_S : IMAGERT_RGBA); // XY=Dir#0, ZW=Dir#1
   helper.get(rt_desc); // we always need to call this because 'helper' can be set to 'converted'
   set(helper, null, false); Mtn.SetDirs[!D._view_main.full]->draw(rect);
   if(stage==RS_VEL_LEAK && show(helper, false, D.signedVelRT()))return true;

   Sh.Img[1]->set(helper);
   set(&dest, null, true);
   Mtn.Blur[dither /*&& src.highPrecision()*/ && !dest.highPrecision()]->draw(src); // here blurring may generate high precision values

   return false;
}
INLINE Shader* GetDofDS(Bool clamp , Bool realistic, Bool half_res) {Shader* &s=Dof.DofDS[clamp ][realistic][half_res]; if(SLOW_SHADER_LOAD && !s)s=Dof.getDS(clamp , realistic, half_res); return s;}
INLINE Shader* GetDof  (Bool dither, Bool realistic               ) {Shader* &s=Dof.Dof  [dither][realistic]          ; if(SLOW_SHADER_LOAD && !s)s=Dof.get  (dither, realistic          ); return s;}
// !! Assumes that 'ImgClamp' was already set !!
void RendererClass::dof(ImageRT &src, ImageRT &dest, Bool dither)
{ // Depth of Field shader does not require stereoscopic processing because it just reads the depth buffer
   const Int   shift=1; // half_res
   ImageRTDesc rt_desc(fxW()>>shift, fxH()>>shift, src.highPrecision() ? IMAGERT_SRGBA_H : IMAGERT_SRGBA); // here Alpha is used to store amount of Blur, use high precision if source is to don't lose smooth gradients when having full blur (especially visible on sky), IMAGERT_SRGBA_H vs IMAGERT_SRGBA has no significant difference on GeForce 1050Ti
   ImageRTPtr  rt0(rt_desc),
               rt1(rt_desc);

   Bool half_res=(Flt(src.h())/rt0->h() <= 2.5f); // half_res=scale 2, ..3.., quarter=scale 4, 2.5 was the biggest scale that didn't cause jittering when using half down-sampling
   Dof.load();
 C DepthOfField::Pixel &pixel=Dof.pixel(Round(fxH()*(5.0f/1080))); // use 5 pixel range blur on a 1080 resolution

   Flt range_inv=1.0f/Max(D.dofRange(), EPS);
   Dof.DofParams->setConditional(Vec4(D.dofIntensity(), D.dofFocus(), range_inv, -D.dofFocus()*range_inv));

   set(rt0, null, false); Rect ext_rect, *rect=null; if(!D._view_main.full){ext_rect=D.viewRect(); rect=&ext_rect.extend(Renderer.pixelToScreenSize(pixel.pixels+1));} // when not rendering entire viewport, then extend the rectangle because of blurs checking neighbors, add +1 because of texture filtering, we can ignore stereoscopic there because that's always disabled for not full viewports, have to use 'Renderer.pixelToScreenSize' and not 'D.pixelToScreenSize' and call after setting RT
   Sh.imgSize( src); GetDofDS(!D._view_main.full, D.dofFocusMode(), half_res)->draw(src, rect);
 //Sh.imgSize(*rt0); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
   set(rt1, null, false);                 pixel.BlurX->draw(rt0, rect);
   set(rt0, null, false); rt0->discard(); pixel.BlurY->draw(rt1, rect);

   set(&dest, null, true);
   Sh.Img[1]->set(rt0);
   GetDof(dither && (src.highPrecision() || rt0->highPrecision()) && !dest.highPrecision(), D.dofFocusMode())->draw(src);
}
INLINE Shader* GetCombine       () {Shader* &s=Sh.Combine       ; if(SLOW_SHADER_LOAD && !s)s=Sh.get("Combine0"      ); return s;}
INLINE Shader* GetCombineSS     () {Shader* &s=Sh.CombineSS     ; if(SLOW_SHADER_LOAD && !s)s=Sh.get("Combine1"      ); return s;}
INLINE Shader* GetCombineMS     () {Shader* &s=Sh.CombineMS     ; if(SLOW_SHADER_LOAD && !s)s=Sh.get("Combine2"      ); return s;}
INLINE Shader* GetCombineSSAlpha() {Shader* &s=Sh.CombineSSAlpha; if(SLOW_SHADER_LOAD && !s)s=Sh.get("CombineSSAlpha"); return s;}
void RendererClass::Combine(IMAGE_PRECISION rt_prec)
{
   Bool alpha_premultiplied=false;
   if(_ds->multiSample() && Sh.CombineMS) // '_col' could have been resolved already, so check '_ds' instead
   {
      ImageRTPtr resolve=_final; if(resolve->compatible(*_ds_1s))D.alpha(ALPHA_BLEND);else
      {
         resolve.get(ImageRTDesc(_ds_1s->w(), _ds_1s->h(), GetImageRTType(true, rt_prec))); // resolve to a temp RT and apply that later, here Alpha is used for storing image opacity
         D.alpha(ALPHA_SETBLEND_SET); alpha_premultiplied=true;
      }
      set(resolve, _ds_1s, true, NEED_DEPTH_READ);
      Sh.imgSize(*_col);
      if(hasStencilAttached())
      {
         D.stencil(STENCIL_MSAA_TEST, STENCIL_REF_MSAA); GetCombineMS()->draw(_col);
                         D.stencilRef(0               ); GetCombine  ()->draw(_col);
         D.stencil(STENCIL_NONE     );
      }else
      {
         GetCombineMS()->draw(_col); // we have to run all at multi-sampled frequency
      }
     _col=resolve;
   }else
   if(_col->w()<_final->w()) // resolve first to small buffer
   {
      ImageRTPtr resolve=_col; resolve.get(ImageRTDesc(_col->w(), _col->h(), GetImageRTType(true, rt_prec))); // here Alpha is used for storing image opacity
      set(resolve, null, false); // request full viewport because we will need it below, when drawing black borders
      D.alpha(ALPHA_SETBLEND_SET); alpha_premultiplied=true;
      GetCombine()->draw(_col, &D.viewRect());
     _col=resolve;
   }else
   if(_ds->w()>_final->w()) // resolve Alpha first to full buffer, check '_ds' because '_col' could have been already downsampled
   {
      ImageRTPtr alpha; alpha.get(ImageRTDesc(_ds_1s->w(), _ds_1s->h(), IMAGERT_ONE));
      set(alpha, null, true);
      D.alpha(ALPHA_NONE);
      GetCombineSSAlpha()->draw();
      Sh.Img[1]->set(alpha);
      set(_final, null, true);
      D.alpha(ALPHA_BLEND);
      GetCombineSS()->draw(_col);
     _col=_final;
   }else
   {
      set(_final, null, true);
      D.alpha(ALPHA_BLEND);
      GetCombine()->draw(_col);
     _col=_final;
   }
   if(_col!=_final)
   {
      Shader *shader=null;
      Bool    upscale_none=false;
      if(_col->w()<_final->w()) // upscale
      {
         Bool dither=(D.dither() && !_final->highPrecision()); // disable dithering if destination has high precision
         Int  pixels=1+1; // 1 for filtering + 1 for borders (because source is smaller and may not cover the entire range for dest, for example in dest we want 100 pixels, but 1 source pixel covers 30 dest pixels, so we may get only 3 source pixels covering 90 dest pixels)
         switch(D.densityFilter()) // remember that cubic shaders are optional and can be null if failed to load
         {
            case FILTER_NONE:
            {
               upscale_none=true;
               pixels=1; // 1 for borders
            #if DX11
               SamplerPoint.setPS(SSI_DEFAULT);
            #elif GL // in GL 'ShaderImage.Sampler' does not affect filtering, so modify it manually
               D.texBind(GL_TEXTURE_2D, _col->_txtr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            #endif
            }break;

            case FILTER_CUBIC_FAST       :
            case FILTER_CUBIC_FAST_SMOOTH:
            case FILTER_CUBIC_FAST_SHARP :
               pixels=2+1; // 2 for filtering + 1 for borders
               Sh.imgSize(*_col); shader=Sh.DrawTexCubicFastF[dither]; // this doesn't need to check for "_col->highPrecision" because resizing and cubic filtering generates smooth values
            break;

            case FILTER_BEST       :
            case FILTER_CUBIC      :
            case FILTER_CUBIC_SHARP:
               pixels=3+1; // 3 for filtering + 1 for borders
               Sh.imgSize(*_col); Sh.loadCubicShaders(); shader=Sh.DrawTexCubicF[dither]; // this doesn't need to check for "_col->highPrecision" because resizing and cubic filtering generates smooth values
            break;
         }
         if(!D._view_main.full)
         {
            set(_col, null, false); // need full viewport
            D.viewRect().drawBorder(TRANSPARENT, Renderer.pixelToScreenSize(-pixels)); // draw black border around the viewport to clear and prevent from potential artifacts on viewport edges
         }
      }
      if(!shader)shader=Sh.Draw; // ignore dithering for simple filtering because we've resolved this to low precision RT

      set(_final, null, true);
      D.alpha(alpha_premultiplied ? ALPHA_MERGE : ALPHA_BLEND);
      shader->draw(_col);
      if(upscale_none)
      {
         #if DX11
            SamplerLinearClamp.setPS(SSI_DEFAULT);
         #elif GL
            if(_col->filterable()){D.texBind(GL_TEXTURE_2D, _col->_txtr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);}
         #endif
      }
     _col=_final;
   }
}
/******************************************************************************/
void RendererClass::cleanup()
{
  _ds          .clear();
//_ds_1s       .clear(); do not clear '_ds_1s' because 'setDepthForDebugDrawing' may be called after rendering finishes, also 'capture' makes use of it
  _nrm         .clear();
  _vel         .clear();
  _lum         .clear();
  _lum_1s      .clear();
  _shd_1s      .clear();
  _shd_ms      .clear();
  _water_col   .clear();
  _water_nrm   .clear();
  _water_ds    .clear();
  _water_lum   .clear();
  _vol         .clear();
  _ao          .clear();
  _mirror_rt   .clear();
  _outline_rt  .clear();
  _sky_coverage.clear();
   Lights.clear();
   ClearInstances();
}
RendererClass& RendererClass::operator()(void (&render)())
{
#if DEBUG
   if(Kb.b(KB_NP0))stage=RS_DEPTH;else
   if(Kb.b(KB_NP1))stage=RS_COLOR;else
   if(Kb.b(KB_NP2))stage=RS_NORMAL;else
   if(Kb.b(KB_NP3))stage=RS_VEL;else
   if(Kb.b(KB_NP4))stage=(Kb.b(KB_NP5) ? RS_LIGHT_AO : RS_LIGHT);else
   if(Kb.b(KB_NP5))stage=RS_AO;else
   if(Kb.b(KB_NP6))stage=RS_LIT_COLOR;else
   if(Kb.b(KB_NP7))stage=RS_WATER_COLOR;else
   if(Kb.b(KB_NP8))stage=RS_WATER_NORMAL;else
   if(Kb.b(KB_NP9))stage=RS_WATER_LIGHT;else
   if(Kb.b(KB_NPDIV))stage=RS_REFLECTION;else
   if(Kb.br(KB_NP0) || Kb.br(KB_NP1) || Kb.br(KB_NP2) || Kb.br(KB_NP3) || Kb.br(KB_NP4) || Kb.br(KB_NP5) || Kb.br(KB_NP6) || Kb.br(KB_NP7) || Kb.br(KB_NP8) || Kb.br(KB_NP9)
   || Kb.br(KB_NPDIV))stage=RS_DEFAULT;
#endif

   // Shadow Settings
   Sh.ShdRange->setConditional(T._shd_range=D._shd_frac*D.viewRange());
   {
      Flt from=T._shd_range*D._shd_fade,
          to  =T._shd_range;
      if(from>=D.viewRange()-EPSL) // disabled
      {
         Sh.ShdRangeMulAdd->setConditional(Vec2(0));
      }else
      {
         MAX(to, from+0.01f);
         from*=from; to*=to;
         Flt mul=1/(to-from), add=-from*mul;
         Sh.ShdRangeMulAdd->setConditional(Vec2(mul, add));
      }
   }

   // prepare
  _render     =render;
  _stereo     =(VR.active() && D._view_main.full && !combine && !target && !_get_target && D._allow_stereo); // use stereo only for full viewport, if we're not combining (games may use combining to display 3D items/characters in Gui)
  _eye_num    =_stereo+1; // _stereo ? 2 : 1
  _has_glow   =false;
  _fur_is     =false;
  _mirror_want=false;
  _outline    =0;
  _final      =(target ? target : _stereo ? VR.getNewRender() : _cur_main);

   if(VR.active())D.setViewFovTan(); // !! call after setting _stereo and _render !!

   // !! it is important to call this as the first thing during rendering, because 'Game.WorldManager.draw' assumes so, if this needs to be changed then rework 'Game.WorldManager.draw' !!
   // set water
   Water.prepare();

#define MEASURE(x) if(_t_measure){D.finish(); Dbl c=Time.curTime(); x+=c-t; t=c;}

   // render
   {
      Dbl t; Flt temp, water;
      if(_t_measure){D.finish(); t=Time.curTime(); temp=water=0; _t_measures[0]++;}

      if(reflection())goto finished; MEASURE(_t_reflection[1])

      prepare(); MEASURE(_t_prepare[1])
      solid  (); MEASURE(_t_solid  [1])
   #if TILE_BASED_GPU && !WEB // we need to make sure that depth RT is flushed to depth texture on tile-based deferred renderers, this is because on those devices the RT's (including depth buffer) are stored in fast on-chip memory and to be able to read from them, we need to flush them to the texture memory. This is done after reading solid's and before we may read from the depth buffer. No need to do on WEB because there we can never read from depth while writing to it.
      if(canReadDepth())
         if(D.edgeDetect() || D.particlesSoft() || Sky.wantDepth() || Clouds.wantDepth() || Fog.draw/* || Sun.wantDepth()*/) // here we need to check only effects that may read from depth without changing any RT's, because on any RT change the depth is flushed. Sun doesn't bind DS to FBO when drawing rays. TODO: we wouldn't need to do this if all shaders reading from the depth would use gl_LastFragDepth - https://www.khronos.org/registry/OpenGL/extensions/ARM/ARM_shader_framebuffer_fetch_depth_stencil.txt
      { // unbinding will force the flush (calling just 'glFlush' was not enough)
         glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT  , GL_RENDERBUFFER, 0);
         glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        _cur_ds=null; _cur_ds_id=0;
      }
   #endif
      overlay(); MEASURE(_t_overlay[1])

      // set background sky pixels not drawn by foreground object meshes (everything at the end of depth buffer), this needs to be done before 'blend' because fur may set depth buffer without setting velocities, and before water surface
      {
         Bool clear_nrm=(_nrm && !NRM_CLEAR_START && ClearNrm());
         if(  clear_nrm || _vel)
         {
            D.alpha    (ALPHA_NONE);
            D.depth2DOn(true);
            if(clear_nrm)
            {
               set(_nrm, _ds, true); Sh.clear(D.signedNrmRT() ? SNRM_CLEAR : NRM_CLEAR); // use DS because we use it for 'D.depth2D' optimization
            }
            if(_vel)
            {
               Mtn.load(); set(_vel, _ds, true); Mtn.ClearSkyVel->draw(); // use DS because we use it for 'D.depth2D' optimization
            }
            D.depth2DOff();
         }
      }

      if(stage)switch(stage)
      {
         case RS_COLOR : if(_cur_type==RT_DEFERRED && show(_col  , true                  ))goto finished; break; // only on deferred renderer the color is unlit
         case RS_NORMAL: if(                          show(_nrm  , false, D.signedNrmRT()))goto finished; break;
         case RS_DEPTH : if(                          show(_ds_1s, false                 ))goto finished; break; // this may be affected by test blend materials later
      }
      waterPreLight(); MEASURE(water)
      light        (); MEASURE(_t_light[1])
      if(stage)switch(stage)
      {
         case RS_LIT_COLOR: if(show(_col, true))goto finished; break;

         case RS_LIGHT: if(_lum_1s)
         {
            if(_ao && !D.aoAll()) // if there's AO available, then it means that ambient hasn't been applied yet to '_lum_1s', to keep consistency with '_lum_1s' when AO is disabled, apply flat ambient here
            {
               set(_lum_1s, null, true);
               D.alpha(ALPHA_ADD);
               Sh.clear(Vec4(D.ambientColorD(), 0));
            }
            if(show(_lum_1s, true))goto finished;
         }break;

         case RS_AO: if(_ao)
         {
            set(_final, null, true); D.alpha(ALPHA_NONE); Sh.ImgX[0]->set(_ao); (LINEAR_GAMMA ? Sh.DrawXG : Sh.DrawX)->draw();
            goto finished;
         }break;

         case RS_LIGHT_AO: if(_lum_1s)
         {
            if(_ao)
            {
               set(_lum_1s, null, true);
               Sh.ImgX[0]->set(_ao);
               if(D.aoAll())
               {
                  D.alpha(ALPHA_MUL);
                  Sh.DrawX->draw();
               }else
               {
                  D.alpha(ALPHA_ADD);
                  Sh.Color[0]->set(Vec4(D.ambientColorD(), 0));
                  Sh.Color[1]->set(Vec4Zero                  );
                  Sh.DrawXC[0][0]->draw();
               }
            }
            if(show(_lum_1s, true))goto finished;
         }break;
      }
      if(waterPostLight())goto finished; MEASURE(_t_water[1])if(_t_measure)_t_water[1]+=water;

      MEASURE(temp)

      sky       (); MEASURE(_t_sky[1])
      edgeDetect(); MEASURE(_t_edge_detect[1])
      blend     (); MEASURE(_t_blend[1])
    /*if(stage)switch(stage)
      {
         case RS_DEPTH: if(set(_ds_1s))goto finished; break;
      }*/

      palette(0);
      palette(1); MEASURE(_t_palette[1])
      behind ( ); MEASURE(_t_behind [1])
      outline( );

      // 2D
      finalizeGlow (); // !! assume that nothing below can trigger glow on the scene !!
      applyOutline ();
      edgeSoften   (); MEASURE(temp)
      // all following effects below that modify '_col' (and not create new '_col') should call 'downSample' first, otherwise they should call 'resolveMultiSample'
      if(AstroDrawRays())goto finished; MEASURE(_t_rays[1])
      volumetric   (); MEASURE(_t_volumetric[1])
      refract      (); MEASURE(_t_refract[1])
      postProcess  (); MEASURE(_t_post_process[1])
   finished:;
   }

   // cleanup
   {
     _render=null; // this specifies that we're outside of Rendering
     _final.clear();
      D.alpha(ALPHA_BLEND); mode(RM_SOLID);
      set(_cur_main, _cur_main_ds, true);

      Sky.setFracMulAdd(); // in case user draws billboards/particles outside of Renderer, call before 'cleanup' because this relies on depth buffer being available
      cleanup();

      if(VR.active())
      {
         D.setViewFovTan(); // !! call after clearing _render !!
         if(_stereo)
         {
            D.clearCol(); // normally after rendering has finished we expect to draw on top of the result, however for stereoscopic, we've rendered to a separate render target, that won't be used for 2D drawing now, instead we're now back to '_cur_main' which is not yet initialized, so we need to clear it here
            // restore settings to centered (not one of the eyes)
            D._view_main.setShader().setProjMatrix(); // viewport
            SetCam(ActiveCam.matrix); // camera, 'Frustum' remains the same
         }
      }
   }
   if(_shader_param_changes)Exit("'LinkShaderParamChanges' was called without 'UnlinkShaderParamChanges'.");
   return T;
}
ImageRTPtr RendererClass::get(void (&render)())
{
  _get_target=true ; T(render);
  _get_target=false; ImageRTPtr temp=_col; _col.clear(); return temp;
}
Bool RendererClass::reflection()
{
   // render reflection
   if(_mirror_want)
   {
      // remember current settings and disable fancy effects
      Camera           cam            =ActiveCam            ;
      Bool             combine        =T. combine           ;                     T. combine        =false             ;
      Bool             stereo         =T._stereo            ;                     T._stereo         =false             ; // don't reset FOV because we want to render the reflection with the same exact FOV settings as only one eye, because this reflection will be reused for both eyes
      Int              eye_num        =T._eye_num           ;                     T._eye_num        =1                 ;
      RENDER_TYPE      render_type    =T._cur_type          ;                     T._cur_type       =Water.reflectionRenderer();
      Bool             hp_col_rt      =D.highPrecColRT    ();                     D._hp_col_rt      =false             ;
      Bool             hp_nrm_rt      =D.highPrecNrmRT    ();                     D._hp_nrm_rt      =false             ;
      Bool             hp_lum_rt      =D.highPrecLumRT    ();                     D._hp_lum_rt      =false             ;
      IMAGE_PRECISION  lit_col_rt_prec=D.litColRTPrecision();                     D._lit_col_rt_prec=IMAGE_PRECISION_8 ;
      Bool             hp_nrm_calc    =D.highPrecNrmCalc  ();                     D.highPrecNrmCalc (false            );
      Bool             eye_adapt      =D.eyeAdaptation    ();                     D.eyeAdaptation   (false            );
      Bool             vol_light      =D.volLight         ();                     D.volLight        (false            ); // if it will be enabled, then calling 'volumetric' is required and clearing Renderer._vol_is
      AMBIENT_MODE     amb_mode       =D.ambientMode      ();                     D.ambientMode     (AMBIENT_FLAT     );
      MOTION_MODE      mtn_mode       =D.motionMode       ();                     D.motionMode      (MOTION_NONE      );
      SHADOW_MODE      shd_mode       =D.shadowMode       (); if(!_mirror_shadows)D.shadowMode      (SHADOW_NONE      );
      Byte             shd_soft       =D.shadowSoft       ();                     D.shadowSoft      (0                );
      Bool             shd_jitter     =D.shadowJitter     ();                     D.shadowJitter    (false            );
      EDGE_SOFTEN_MODE edge_soft      =D.edgeSoften       ();                     D._edge_soften    =EDGE_SOFTEN_NONE  ;
      Bool             tesselation    =D.tesselationAllow ();                     D.tesselationAllow(false            );
      Byte             density        =D.densityByte      ();                     D.densityFast     (Mid(((D.densityByte()+1)>>_mirror_resolution)-1, 0, 255));

      // set new settings
     _mirror=true;                                                           // set before viewport and camera
      // <- change viewport here if needed
      ConstCast(ActiveCam).matrix.mirror(_mirror_plane); ActiveCamChanged(); // set mirrored camera and frustum
      D.clipPlane(_mirror_plane);                                            // set clip plane after viewport and camera
   #if !GL // not needed for GL
      Sh.FrontFace->set(false);                                              // adjust back flipping for mirrored
   #endif
      D.lodSetCurrentFactor();

      // render !! adding new modes here will require setting there D.clipPlane !!
      prepare();
      solid  ();
      light  ();
      sky    ();
      blend  ();
      AstroDrawRays();

      // cleanup
      cleanup(); Swap(_mirror_rt, _col); // 'cleanup' clears '_mirror_rt' so we need to set it after calling the function

      // restore effects (before viewport and camera, because stereoscopic affects viewport fov)
      T. combine        =combine        ;
      T._stereo         =stereo         ;
      T._eye_num        =eye_num        ;
      T._cur_type       =render_type    ;
      D._hp_col_rt      =hp_col_rt      ;
      D._hp_nrm_rt      =hp_nrm_rt      ;
      D._hp_lum_rt      =hp_lum_rt      ;
      D._lit_col_rt_prec=lit_col_rt_prec;
      D.highPrecNrmCalc (hp_nrm_calc   );
      D.eyeAdaptation   (eye_adapt     );
      D.volLight        (vol_light     );
      D.ambientMode     (amb_mode      );
      D.motionMode      (mtn_mode      );
      D.shadowMode      (shd_mode      );
      D.shadowSoft      (shd_soft      );
      D.shadowJitter    (shd_jitter    );
      D._edge_soften    =edge_soft      ;
      D.tesselationAllow(tesselation   );
      D.densityFast     (density       );

      // restore previous settings (mirror, viewport and camera)
     _mirror=false;            // !! set before viewport and camera, because it affects the Frustum, and after 'cleanup' !!
      // <- reset viewport here if needed
      cam.set();               // camera, this will also reset 'Frustum'
   #if !GL // not needed for GL
      Sh.FrontFace->set(true); // restore back flipping
   #endif
      D.lodSetCurrentFactor();

      if(stage==RS_REFLECTION && show(_mirror_rt, true))return true;
   }
   return false;
}
/******************************************************************************/
Bool RendererClass:: hasEdgeSoften()C {return wantEdgeSoften() && !fastCombine();}
Bool RendererClass::wantEdgeSoften()C
{
   switch(D.edgeSoften())
   {
      case EDGE_SOFTEN_FXAA: return /*Sh.FXAA[0]!=null &&*/ Sh.FXAA[1]!=null; // check 'h_FXAA[1]' only, because it's set only if all loaded OK
   #if SUPPORT_MLAA
      case EDGE_SOFTEN_MLAA: return Sh.MLAAEdge && Sh.MLAABlend && Sh.MLAA && _mlaa_area;
   #endif
      case EDGE_SOFTEN_SMAA: return /*Sh.SMAAEdge[0] && Sh.SMAAEdge[1] && Sh.SMAABlend && Sh.SMAA && _smaa_area &&*/ _smaa_search!=null; // check '_smaa_search' only, because it's set only if all loaded OK
   }
   return false;
}
Bool RendererClass::wantDepth         ()C {return wantMotion() || wantDof() || D.aoWant() || D.edgeDetect() || D.particlesSoft() || D.volLight() || Sky.wantDepth() || Clouds.wantDepth() || Fog.draw || Sun.wantDepth() || !Water.max1Light();} // TODO: even though we check all known things here, there are still some things about we don't know up-front (like local fog, decals, Image.drawVolume, ..)
Bool RendererClass::canReadDepth      ()C {return        _ds->depthTexture();} // have to check '_ds' because this is the original source depth, it can be multi-sampled (in that case it's possible depth reading won't be available), but '_ds_1s' is 1-sampled (and could have depth-reads even if '_ds' doesn't)
Bool RendererClass::safeCanReadDepth  ()C {return _ds && _ds->depthTexture();}
Bool RendererClass::hasStencilAttached()C {return hasDepthAttached() && ImageTI[_cur_ds->hwType()].s;}
Bool RendererClass::hasDepthAttached  ()C
{
#if GL
   return _cur_ds && _cur_ds->_txtr==_cur_ds_id; // check both '_cur_ds' and '_cur_ds_id' because '_cur_ds_id' will be 0 when Image is a RenderBuffer or temporarily unbound Texture (only Textures can be temporarily unbound), this will work OK for RenderBuffers because both '_cur_ds_id' and 'cur_ds->_txtr' will be zero
#else
   return _cur_ds_id!=null; // we have to check '_cur_ds_id' because on DX10 it can be null if read-only is not supported
#endif
}
Bool RendererClass::canReadDepth1S()C
{
   return canReadDepth()
   #if DX11
      && _ds_1s->_dsv !=_cur_ds_id // we always read from '_ds_1s', we can do that only if it's not bound as Depth RT, on purpose we check '_dsv' and not '_rdsv' because '_rdsv' IS allowed
   #elif WEB
      && _ds_1s->_txtr!=_cur_ds_id // we always read from '_ds_1s', we can do that only if it's not bound as Depth RT
   #endif
   ;
}

Bool RendererClass::wantBloom   ()C {return D.bloomUsed();}
Bool RendererClass:: hasBloom   ()C {return wantBloom() && !fastCombine();}
Bool RendererClass::wantEyeAdapt()C {return D.eyeAdaptation() && _eye_adapt_scale[0].is();}
Bool RendererClass:: hasEyeAdapt()C {return wantEyeAdapt() && !fastCombine();}
Bool RendererClass::wantMotion  ()C {return D.motionMode() && FovPerspective(D.viewFovMode());}
Bool RendererClass:: hasMotion  ()C {return wantMotion() && canReadDepth() && !fastCombine();}
Bool RendererClass::wantDof     ()C {return D.dofWant();}
Bool RendererClass:: hasDof     ()C {return wantDof() && canReadDepth() && !fastCombine();}
Bool RendererClass:: hasAO      ()C {return D.aoWant() && canReadDepth() && !fastCombine();}

Bool RendererClass::fastCombine      ()C {return combine      && _col==_final;}
Bool RendererClass::slowCombine      ()C {return combine      && !fastCombine() && canReadDepth();}
Bool RendererClass::hasVolLight      ()C {return D.volLight() && canReadDepth();}
Bool RendererClass::anyDeferred      ()C {return type()==RT_DEFERRED || Water.reflectionRenderer()==RT_DEFERRED;}
Bool RendererClass::anyForward       ()C {return type()==RT_FORWARD  || Water.reflectionRenderer()==RT_FORWARD ;}
Bool RendererClass::lowDepthPrecision()C {return _main_ds.type()==IMAGE_D16;} // this can happen only on Android, and there we do have information about the depth buffer
/******************************************************************************/
Bool RendererClass::show(C ImageRTPtr &image, Bool srgb, Bool sign)
{
   if(image)
   {
      if(ImageTI[image->hwType()].d) // depth
      {
         if(!image->depthTexture())return false; // can't read
         set(_final, null, true); D.alpha(ALPHA_NONE); Sh.Depth->set(image); Sh.get("DrawDepth")->draw(); Sh.Depth->set(_ds_1s);
      }else
      if(sign)
      {
         // we need to draw image*0.5+0.5
         Sh.Color[0]->set(Vec4(0.5f, 0.5f, 0.5f, 0));
         Sh.Color[1]->set(Vec4(0.5f, 0.5f, 0.5f, 1));
         set(_final, null, true); D.alpha(ALPHA_NONE); ((LINEAR_GAMMA && !srgb) ? Sh.DrawCG : Sh.DrawC)->draw(image);
      }else
      if(LINEAR_GAMMA && !srgb)
      {
         set(_final, null, true); D.alpha(ALPHA_NONE); Sh.DrawG->draw(image);
      }else
         image->copyHw(*_final, false, D.viewRect());
      return true;
   }
   return false;
}
Bool RendererClass::swapDS1S(ImageRTPtr &ds_1s)
{
   if(!T._ds_1s->accessible())return false; // can't swap if current DS is not accessible
   if( T._ds==T._ds_1s)
   {
       if(T._ds==T._cur_main_ds)T._cur_main_ds=ds_1s;
          T._ds   = ds_1s ; Sh.DepthMS->set(T._ds   );
   } Swap(T._ds_1s, ds_1s); Sh.Depth  ->set(T._ds_1s);
   return true;
}
void RendererClass::setDS()
{
   if(_col==&_main    )_ds=&_main_ds    ;else // we should always pair '_main' with '_main_ds', even if it's not 'accessible'
   if(_col== _cur_main)_ds= _cur_main_ds;else // reuse '_cur_main_ds' if we're rendering to '_cur_main'
                       _ds.getDS(_col->w(), _col->h(), _col->samples()); // create a new one
}
void RendererClass::prepare()
{
   Byte  samples=(mirror() ? 1 : D.samples()); // disable multi-sampling for reflection
   VecI2 rt_size; if(VR.active() && D._allow_stereo) // following conditions affect this: _stereo, _allow_stereo, mirror()
   { /* We want this case when:
         -rendering to VR    'GuiTexture'
         -rendering to VR 'RenderTexture'
         -rendering to mirror reflection to be used for VR 'RenderTexture'
        Remember that '_stereo' gets temporarily disabled when rendering mirror reflection */
      rt_size=_final->size();
      if(D.densityUsed())
      {
         Int mul=D.densityByte()+1;
         rt_size.set(Mid((rt_size.x*mul+64)/128, 1, D.maxTexSize()),
                     Mid((rt_size.y*mul+64)/128, 1, D.maxTexSize()));
      }
   }else rt_size=D.render();

start:
   IMAGE_PRECISION prec=((_cur_type==RT_DEFERRED) ? D.highPrecColRT() ? IMAGE_PRECISION_10 : IMAGE_PRECISION_8 : D.litColRTPrecision()); // for deferred renderer we first render to col and only after that we mix it with light, other modes render color already mixed with light, for high precision we need only 10-bit, no need for 16-bit
  _col=_final;
   if(_cur_type==RT_DEFERRED || mirror() || _get_target // <- these always require
   || _col->size()!=rt_size || _col->samples()!=samples || _col->precision()<prec // if current RT does not match the requested rendering settings
   || wantBloom() || wantEdgeSoften() || wantMotion() || wantDof() || wantEyeAdapt() // if we want to perform post process effects then we will be rendering to a secondary RT anyway, so let's start with secondary with a chance that during the effect we can render directly to '_final'
   || (D.glowAllow() && ImageTI[_col->hwType()].a<8) // we need alpha for glow, this check is needed for example if we have IMAGE_R10G10B10A2
   || (_col==&_main && !_main_ds.depthTexture() && wantDepth()) // if we're setting '_main' which is always paired with '_main_ds', and that is not a depth texture but we need to access depth, then try getting custom RT with depth texture (don't check for '_cur_main' and '_cur_main_ds' because depth buffers other than '_main_ds' are always tried to be created as depth texture first, so if that failed, then there's no point in trying to create one again)
   )_col.get(ImageRTDesc(rt_size.x, rt_size.y, GetImageRTType(D.glowAllow(), prec), samples)); // here Alpha is used for glow

   // depth stencil buffer
   setDS();
   if(combine && !canReadDepth() && _col!=_final) // if we need to combine (treat combine with priority), but after getting the depth buffer it turns out we can't read it (can't do combine), then we must render to final render target directly
   {
     _col=_final; setDS();
   }

   if(!canReadDepth() && _cur_type==RT_DEFERRED) // if depth access is not available and we want deferred renderer then fall back to forward renderer
   {
      if(type                    ()==RT_DEFERRED)type                    (RT_FORWARD);
      if(Water.reflectionRenderer()==RT_DEFERRED)Water.reflectionRenderer(RT_FORWARD);
                                            _cur_type                    =RT_FORWARD ;
      goto start;
   }

   if(!_ds->multiSample ())_ds_1s=_ds;else // if the source is not multisampled then reuse it
   if( _ds->depthTexture())_ds_1s.getDS(_ds->w(), _ds->h());else // create new only if we can resolve multi-sample onto 1-sample
                           _ds_1s.clear(); // there's no point in creating a 1-sampled depth buffer if we can't resolve from the multi-sampled
   Sh.Depth  ->set(_ds_1s);
   Sh.DepthMS->set(_ds   );

  _set_depth_needed=(_ds!=_cur_main_ds && _ds_1s!=_cur_main_ds && canReadDepth()); // if we're not rendering to currently main depth buffer and we have depth access

   D.alpha(ALPHA_NONE);

   mode(RM_PREPARE); AstroPrepare(); // !! call after obtaining '_col', '_ds' and '_ds_1s' because we rely on having them, and after RM_PREPARE because we may add lights !!
  _eye=0; if(_stereo)SetCam(EyeMatrix[_eye]); // start with the first eye and set camera so we can calculate view_matrix for instances, this is important, because precalculated view_matrix is assumed to be for the first eye, so when rendering instances we need to adjust the projection matrix for next eye, this affects 'BeginPrecomputedViewMatrix', 'Frustum' remains the same
  _render(); // we can call '_render' only once for RM_PREPARE
   Bool clear_ds=true; // if need to clear depth

#if SUPPORT_EARLY_Z
   if(HasEarlyZInstances())
   {
      set(null, _ds, true);
      D.clearDS(); clear_ds=false; // already cleared so no need anymore
      D.set3D();

   early_z:
      setEyeViewport();
      DrawEarlyZInstances();
      if(++_eye<_eye_num)goto early_z;

      ClearEarlyZInstances();
      D.set2D();
   }
#endif

   const Bool clear_col=((!Sky.isActual() || stage==RS_COLOR || stage==RS_LIT_COLOR || _col->multiSample()) && !fastCombine()); // performance tests suggested it's better don't clear unless necessary, instead 'Image.discard' is used and improves performance (at least on Mobile), always have to clear for multi-sampled to allow for proper detection of MSAA pixels using 'Sh.DetectMSCol' (this is needed for all renderers, not only Deferred, without this edges of sky/meshes may not get multi-sampled, especially when there's small variation in material color texture or no texture at all having just a single color)
   switch(_cur_type)
   {
      case RT_DEFERRED:
      {
         const Bool merged_clear=D._view_main.full, // use when possible, should improve performance on tile-based renderers
                     clear_nrm  =(NRM_CLEAR_START && ClearNrm()),
                     clear_vel  =false; // this is not needed because "ClearSkyVel" is used later, performance tests suggested it's better don't clear unless necessary, instead 'Image.discard' is used and improves performance (at least on Mobile)

         if(D.motionMode()==MOTION_CAMERA_OBJECT && hasMotion() && D._max_rt>=3)
         {
           _vel.get(ImageRTDesc(_col->w(), _col->h(), D.signedVelRT() ? IMAGERT_RGB_S : IMAGERT_RGB, _col->samples())); // "_vel!=null" is treated as MOTION_CAMERA_OBJECT mode across the engine, doesn't use Alpha
            if(clear_vel && !merged_clear)_vel->clearViewport(D.signedVelRT() ? SVEL_CLEAR : VEL_CLEAR);
         }

        _nrm.get(ImageRTDesc(_col->w(), _col->h(), D.signedNrmRT() ? (D.highPrecNrmRT() ? IMAGERT_RGBA_SP : IMAGERT_RGBA_S) : (D.highPrecNrmRT() ? IMAGERT_RGBA_P : IMAGERT_RGBA), _col->samples())); // here Alpha is used for specular
         if(!merged_clear)
         {
            if(clear_nrm)_nrm->clearViewport(D.signedNrmRT() ? SNRM_CLEAR : NRM_CLEAR);
            if(clear_col)_col->clearViewport();
         }

         set(_col, _nrm, _vel, null, _ds, true);
         if(merged_clear)
         {
            if(clear_col)D.clearCol(0, Vec4Zero);
            if(clear_nrm)D.clearCol(1, D.signedNrmRT() ? SNRM_CLEAR : NRM_CLEAR);
            if(clear_vel)D.clearCol(2, D.signedVelRT() ? SVEL_CLEAR : VEL_CLEAR);
         }
         if(clear_ds)D.clearDS();
      }break;

      case RT_FORWARD:
      {
        _clear=((clear_col?1:0) | (clear_ds?2:0)); // set '_clear' to be checked later
         // don't set RT or clear here, because it's very likely we will draw shadows first, before drawing to '_col', this will avoid setting useless '_col' which is very important for Mobile platforms with tile-based deferred renderers, instead we will check this in 'setForwardCol'
      }break;
   }
}
void RendererClass::setForwardCol()
{
   set(_col, _ds, true);
   if(_clear) // need to clear something
   {
      if(_clear&1)D.clearCol(combine ? TRANSPARENT : Color(clear_color.r, clear_color.g, clear_color.b, 0));
      if(_clear&2)D.clearDS ();
     _clear=0; // cleared
   }
}
void RendererClass::aoApply()
{
   if(hasAO())
   {
      ao(); if(_ao)
      {
         set(_col, _ds, true); // restore rendering RT's after calculating AO
         D .alpha(ALPHA_MUL);
         D .depth2DOn();
         Sh.ImgX[0]->set(_ao);
         Sh.DrawX->draw();
         D .depth2DOff();
      }
   }
}
void RendererClass::solid()
{
   switch(_cur_type)
   {
      case RT_DEFERRED:
      {
         D.stencil(STENCIL_ALWAYS_SET, 0); D.set3D(); mode(RM_SOLID);
         REPS(_eye, _eye_num)
         {
            setEyeViewport();
            DrawSolidInstances(); _render();
         }
         ClearSolidInstances();
         D.stencil(STENCIL_NONE); D.set2D();

         resolveDepth();
      }break;

      case RT_FORWARD:
      {
         // Lights + Solid
         LimitLights();
          SortLights();

         // find initial light
         Int first_light=-1;
         if(Lights.elms() && (D.aoAll() || !hasAO())) // if we do "AO && !aoAll" then first we need to draw without lights (ambient only)
         {
            // lights are already sorted, and 0-th light is the most significant, its shadow map must be rendered last to be used by BLEND_LIGHT which happens at a later stage
            Int skip_light=-1;
            if(Lights[0].type==LIGHT_DIR && Lights[0].shadow) // 0-th is LightDir (which BlendLight only supports) and needs shadows
               for(Int i=Lights.elms(); --i>=1; )if(Lights[i].shadow) // if at least one different light needs shadows
                  {skip_light=0; break;} // it means we can't start with 0-th, because drawing another light will overwrite shadow map, so skip #0 when choosing the 'first_light'

            // find the best light
            Flt cost=FLT_MAX, view_rect_area=D.viewRect().area(); Dbl frustum_volume=Frustum.volume();
            REPA(Lights)if(i!=skip_light)
            {
               Flt c=Lights[i].firstLightCost(view_rect_area, frustum_volume); if(c<cost){cost=c; first_light=i;} // find light with smallest cost
            }
         }

         // draw main light
         if(first_light>=0)
         {
            Lights[first_light].drawForward(fastCombine() ? ALPHA_NONE_ADD : ALPHA_NONE);
         }else // no light
         {
            setForwardCol();
           _frst_light_offset=OFFSET(FRST, none);
            D.alpha(fastCombine() ? ALPHA_NONE_ADD : ALPHA_NONE);
            D.stencil(STENCIL_ALWAYS_SET, 0); D.set3D(); mode(RM_SOLID);
            REPS(_eye, _eye_num)
            {
               setEyeViewport();
               DrawSolidInstances(); _render();
            }
            ClearSolidInstances();
            D.stencil(STENCIL_NONE); D.set2D();

            resolveDepth();
         }

         // apply ambient occlusion
         if(!D.aoAll())aoApply();

         // draw rest of the lights
         if(Lights.elms()-(first_light>=0)>0)
         {
           _first_pass=false;
            Bool clip=D._clip, clip_allow=D._clip_allow; T._clip=(clip ? D._clip_rect : D.rect()); // remember clipping because 'drawForward' may change it
            Sh.AmbientColorNS_l->set(VecZero); Sh.AmbientMaterial->set(false); // disable ambient lighting
            D.depthFunc(FUNC_LESS_EQUAL); // need to make sure we can apply lights on existing depth
            REPA(Lights)if(i!=first_light)Lights[i].drawForward(ALPHA_ADD_KEEP); // draw 0-th at the end to setup shadow maps (needed for BLEND_LIGHT), keep alpha which is glow
            D.clip(clip ? &T._clip : null); D.clipAllow(clip_allow);
            D.depthFunc(FUNC_LESS);
           _first_pass=true;

            // restore settings
            D.ambientSet(); Sh.AmbientMaterial->set(true); // restore ambient lighting
         }

         // apply ambient occlusion
         if(D.aoAll())aoApply();

         Frustum=FrustumMain; // restore frustum after it being potentially changed when drawing shadow maps or setting frustum for visible objects for lights
       //resolveDepth(); was already called for the main light
      }break;
   }
}
void RendererClass::resolveDepth()
{
   // this resolves the entire '_ds' into '_ds_1s' (by choosing Min of depth samples), and sets 'STENCIL_REF_MSAA' if needed
   if(_ds->multiSample() && _ds->depthTexture())
   {
      Bool swap=(LINEAR_GAMMA && _col->canSwapSRV()); if(swap)_col->swapSRV(); // use sRGB if possible because we're interested in visual differences
      D.alpha(ALPHA_NONE);

      // set multi-sampled '_ds' MSAA
      if(_cur_type==RT_DEFERRED      // for     deferred set it always (needed for lighting)
      || Fog.draw || Sky.isActual()) // for non-deferred it will be used only for fog and sky
      {
         D.stencil(STENCIL_MSAA_SET, STENCIL_REF_MSAA);
         set(null, _ds, true);
       //if(_nrm){Sh.ImgMS[0]->set(_nrm); Sh.DetectMSNrm->draw();}else 'DetectMSNrm' generates too many MS pixels, making rendering slower, so don't use
                 {Sh.ImgMS[0]->set(_col); Sh.DetectMSCol->draw();}
      }

      // always resolve '_ds' into '_ds_1s'
      set(null, _ds_1s, true);
      D.stencil(STENCIL_ALWAYS_SET, 0); // use 'STENCIL_ALWAYS_SET' here so when down-sampling depth, we clear the entire stencil mask for '_ds_1s'
      D.depthFunc(FUNC_ALWAYS); D.depthLock  (true); Sh.ResolveDepth->draw();
      D.depthFunc(FUNC_LESS  ); D.depthUnlock(    );

      // set 1-sampled '_ds_1s' MSAA
      if(_cur_type==RT_DEFERRED // for     deferred set it always (needed for lighting)
      || slowCombine())         // for non-deferred it will be used only for slow combine
      {
         D.stencilRef(STENCIL_REF_MSAA);
       //if(_nrm){Sh.ImgMS[0]->set(_nrm); Sh.DetectMSNrm->draw();}else 'DetectMSNrm' generates too many MS pixels, making rendering slower, so don't use
                 {Sh.ImgMS[0]->set(_col); Sh.DetectMSCol->draw();}
      }

      D.stencil(STENCIL_NONE);
      if(swap)_col->swapSRV(); // restore
   }
}
void RendererClass::overlay()
{
   D.stencilRef(STENCIL_REF_TERRAIN); // set in case draw codes will use stencil

   if(_cur_type==RT_DEFERRED && D.bumpMode()!=BUMP_FLAT){set(_col, _nrm, null, null, _ds, true, WANT_DEPTH_READ); D.colWrite(COL_WRITE_RGB, 1);} // if we can blend normals
   else                                                  set(_col,                   _ds, true, WANT_DEPTH_READ);
   setDSLookup(); // 'setDSLookup' after 'set'
   D.alpha(ALPHA_BLEND_FACTOR);
   D.set3D(); D.depthWrite(false); D.bias(BIAS_OVERLAY); D.depthFunc(FUNC_LESS_EQUAL); D.depth(true); mode(RM_OVERLAY); // overlay requires BIAS because we may use 'MeshOverlay' which generates triangles by clipping existing ones
   REPS(_eye, _eye_num)
   {
      setEyeViewport();
      DrawOverlayObjects(); _render();
   }
   D.set2D(); D.depthWrite(true); D.bias(BIAS_ZERO); D.depthFunc(FUNC_LESS);
   D.colWrite(COL_WRITE_RGBA, 1);

   D.stencil(STENCIL_NONE); // disable any stencil that might have been enabled
   OverlayObjects.clear();
}
void RendererClass::waterPreLight()
{
   Water._use_secondary_rt=(!Water.max1Light()
                          && canReadDepth()
                          && D._max_rt>=2 // col+nrm
                          && _cur_type!=RT_FORWARD); // for forward for the moment we can't do it, because all lights have already been applied, but in current mode we expect solids to be drawn (so we have depth set because we copy it, and stencil set because we swap DS to preserve it and restore later)
   if(Water._use_secondary_rt)Water.drawSurfaces(); // if we use secondary RT's then we need to draw water surfaces before we calculate lights (otherwise setup lights first and then draw surfaces having shadow-maps known)
}
inline Shader* AmbientOcclusion::get(Int quality, Bool jitter, Bool normal)
{
   Shader* &s=AO[quality][jitter][normal]; if(!s)
   {
      if(!shader)shader=ShaderFiles("Ambient Occlusion");
      s=shader->get(S8+"AO"+quality+jitter+normal);
   }
   return s;
}
void RendererClass::ao()
{
   D.alpha(ALPHA_NONE);
   Shader *ao=AO.get(D.ambientMode()-1, D.ambientJitter(), D.ambientNormal() && _nrm);
   VecI2 res=ByteScaleRes(fx(), D._amb_res); _ao.get(ImageRTDesc(res.x, res.y, IMAGERT_ONE));

   // always downsample and linearize at the same time
   ImageRTPtr ao_depth;
   ao_depth.get(ImageRTDesc(_ao->w(), _ao->h(), IMAGERT_F32)); // don't try to reduce to IMAGERT_F16 because it can create artifacts on big view ranges under certain angles (especially when we don't use normal maps, like in forward renderer)
   linearizeDepth(*ao_depth, *_ds_1s);

 //Sh.imgSize(*_ao); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
   Sh.Img[0]->set(_nrm);
   Sh.Depth ->set(ao_depth);
   Bool foreground=_ao->compatible(*_ds_1s);
   if(_col->multiSample())foreground&=Sky.isActual(); // when having multi-sampling, then allow this optimization only if we're rendering Sky, this is related to smooth edges between solid and sky pixels
   if(stage)if(stage==RS_AO || stage==RS_LIGHT_AO)foreground=false; // if we will display AO then set fully

   if(foreground)D.depth2DOn();
   ImageRT *depth=(foreground ? _ds_1s() : null);
   set(_ao, depth, true, NEED_DEPTH_READ); // use DS for 'D.depth2D'
   REPS(_eye, _eye_num)ao->draw(setEyeParams()); // calculate occlusion
   ao_depth.clear(); // this one is no longer needed
   Sh.Depth->set(_ds_1s); // restore full resolution depth

   if(D.ambientSoft()) // this needs to be in sync with 'D.shadowSoft'
   {
   #if GL && !GL_ES // in GL 'ShaderImage.Sampler' does not affect filtering, so modify it manually, GLES3 doesn't support filtering F32/Depth textures - https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml   "depth textures are not filterable" - https://arm-software.github.io/opengl-es-sdk-for-android/occlusion_culling.html
      D.texBind(GL_TEXTURE_2D, _ds_1s->_txtr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   #endif
      ImageRTDesc rt_desc(_ao->w(), _ao->h(), IMAGERT_ONE);
    //Sh.imgSize(*_ao); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
      if(D.ambientSoft()>=5)
      {
         ImageRTPtr temp; temp.get(rt_desc);
         set(temp, depth, true, NEED_DEPTH_READ);                 Sh.ImgX[0]->set( _ao); Sh.ShdBlurX->draw(); // use DS for 'D.depth2D'
         set( _ao, depth, true, NEED_DEPTH_READ); _ao->discard(); Sh.ImgX[0]->set(temp); Sh.ShdBlurY->draw(); // use DS for 'D.depth2D'
      }else
      {
         ImageRTPtr src=_ao; _ao.get(rt_desc);
         set(_ao, depth, true, NEED_DEPTH_READ); Sh.ImgX[0]->set(src); Sh.ShdBlur[D.ambientSoft()-1]->draw(); // use DS for 'D.depth2D'
      }
   #if GL && !GL_ES
      D.texBind(GL_TEXTURE_2D, Renderer._ds_1s->_txtr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   #endif
   }
   if(foreground)D.depth2DOff();
}
INLINE Shader* GetApplyLight(Int multi_sample, Bool ao, Bool cel_shade, Bool night_shade, Bool glow) {Shader* &s=Sh.ApplyLight[multi_sample][ao][cel_shade][night_shade][glow]; if(SLOW_SHADER_LOAD && !s)s=Sh.getApplyLight(multi_sample, ao, cel_shade, night_shade, glow); return s;}
void RendererClass::light()
{
   if(_cur_type==RT_DEFERRED) // on other renderers light is applied when rendering solid
   {
      /*
      -set '_ao' as Ambient Occlusion (one channel, without D.ambientColor)
      -clear '_lum' and '_lum_1s'
      -add ambient light from meshes
      -calculate screen space light (on MSAA and non-MSAA)
      -final light = sum of all buffers together
     _ao    = AO;
     _lum   = 0 ; _lum+=mesh_ambient;     MSAA of _lum   +=light;
     _lum_1s= 0 ;                     non-MSAA of _lum_1s+=light;
      LIGHT =_lum + _lum_1s + _ao*D.ambientColor
   OR LIGHT =_lum + _lum_1s +     D.ambientColor (if "_ao==null")
      */

      // Ambient Occlusion
      if(hasAO())ao();

      // add dynamic lights
      LimitLights();
       SortLights();
       DrawLights();

           _nrm.clear();
   //_water_nrm.clear(); we may still need it for refraction

      getLumRT();

      // add ambient light from meshes
      set(_lum, _ds, true);
      D.alpha(ALPHA_ADD);
      D.set3D(); mode(RM_AMBIENT); D.depth(true);
      SortAmbientInstances();
      REPS(_eye, _eye_num)
      {
         setEyeViewport();
         DrawAmbientInstances(); _render();
      }
      ClearAmbientInstances();
      D.set2D();

      // light buffer is ready so we can combine it with color
      Bool ao=(_ao!=null), cel_shade=(cel_shade_palette!=null), night_shade=(D.nightShadeColorS().max()>EPS_COL), glow=(_has_glow && ImageTI[_col->hwType()].a); // process glow only if some object reported it and we actually have alpha channel in RT (otherwise glow could be always 1.0)
      Sh.Img [1]->set(_lum_1s             );
      Sh.Img [2]->set( cel_shade_palette());
      Sh.ImgX[0]->set(_ao                 );
      D .alpha(ALPHA_NONE);
      ImageRTPtr src=_col; // can't read and write to the same RT
      Bool has_last_frag_color=false, // TODO: there would be no need to write to a new RT if we would use gl_LastFragColor/gl_LastFragData[0] using extensions - https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_shader_framebuffer_fetch.txt and https://www.khronos.org/registry/OpenGL/extensions/ARM/ARM_shader_framebuffer_fetch.txt
           use_last_frag_color=(has_last_frag_color && (D.highPrecColRT() ? IMAGE_PRECISION_10 : IMAGE_PRECISION_8)==D.litColRTPrecision());
      if(!use_last_frag_color)_col.get(ImageRTDesc(_col->w(), _col->h(), GetImageRTType(D.glowAllow(), D.litColRTPrecision()), _col->samples())); // glow requires alpha

      set(_col, _ds, true, NEED_DEPTH_READ); // use DS because it may be used for 'D.depth2D' optimization and stencil tests
      if((_col==src || Sky.isActual()) && stage!=RS_LIT_COLOR)D.depth2DOn(); // we can skip background only if we're applying to the same RT or if the background will be later overwritten by Sky
      if(!_col->multiSample())GetApplyLight(0, ao, cel_shade, night_shade, glow)->draw(src);else
      {
         Sh.ImgMS[0]->set( src);
         Sh.ImgMS[1]->set(_lum);
         if(hasStencilAttached())
         {
            D.stencil   (STENCIL_MSAA_TEST, 0); GetApplyLight(1, ao, cel_shade, night_shade, glow)->draw(); // 1 sample
            if(Sky.isActual())D.depth2DOff();                                                               // multi-sampled always fill fully when sky will be rendered
            D.stencilRef(STENCIL_REF_MSAA    ); GetApplyLight(2, ao, cel_shade, night_shade, glow)->draw(); // n samples
            D.stencil   (STENCIL_NONE        );
         }else
         {
            if(Sky.isActual())D.depth2DOff();                           // multi-sampled always fill fully when sky will be rendered
            GetApplyLight(2, ao, cel_shade, night_shade, glow)->draw(); // n samples
         }
      }
      D.depth2DOff();
      src.clear();
      if(_lum!=_lum_1s && (_fur_is || stage==RS_LIGHT || stage==RS_LIGHT_AO)){set(_lum_1s, null, true); D.alpha(ALPHA_ADD); Sh.draw(*_lum);} // need to apply multi-sampled lum to 1-sample for fur and light stage
     _lum.clear(); // '_lum' will not be used after this point, however '_lum_1s' may be for rendering fur
   }
}
Bool RendererClass::waterPostLight()
{
   if(!Water._use_secondary_rt)Water.drawSurfaces();else // if we don't want to use secondary RT's
   if(_water_col) // only if we've got some water
   {
   /* -we can always do soft, because '_use_secondary_rt' is enabled only if we can read from depth buffer
      -we need to read from both depth buffers to perform softing, so in order to modify depth, we need to do this after in another operation
      -when doing refraction, we need to have a copy of '_col' and apply to '_col' (this is better for multi-sampling because copy can be 1-sampled)
          or alternatively apply to a new RT using existing '_col' (however the new RT would have to be multi-sampled for MS case)
      -when not doing refraction, we don't need any copy or separate RT, we can just apply to existing '_col' using alpha blending without reading from it, however doing this would prevent from applying glow, so don't do it
      -we can't use stencil optimizations, because:
         -when applying to MS RT the DS is MS and does not have any information about water
         -otherwise we apply to another RT and we have to write all pixels
   */
      getWaterLumRT(); // get in case we still haven't initialized it
      Bool refract=(Water.refract>EPS_MATERIAL_BUMP);

      ImageRTPtr src=_col;
      Bool depth_test;
      if(  depth_test=src->multiSample()) // multi-sampled textures can't be sampled smoothly as there will be artifacts, so resolve them, also in this case we render back to '_col' (which is not set to a new RT because we have a copy of it in 'src') but only to pixels with depth FUNC_LESS, this solves the problem of AA with water
      {  // convert to 1 sample
         ImageRTPtr temp(ImageRTDesc(src->w(), src->h(), GetImageRTType(src->type())));
         src->copyMs(*temp, false, false, D.viewRect()); Swap(src, temp);
         D.depthLock (true); // we need depth testing
         D.depthWrite(false); // disable depth writing because we can't read and write to same DS
         D.depthFunc (FUNC_LESS); // process only pixels that are closer (which means water on top of existing solid)
      }
      if(_col==src)_col.get(ImageRTDesc(_col->w(), _col->h(), GetImageRTType(_col->type()), _col->samples())); // can't read and write to same RT, in multi-sampling we're writing back to '_col' as it's not changed

      SetOneMatrix(); // needed for refraction
      set(_col, _ds, true, NEED_DEPTH_READ); // we need depth read because we always need to read depth buffer for softing, but still use '_ds' in case we apply to existing '_col'
      D.alpha(ALPHA_NONE);

      Water.set();
      Water.setImages(src, _water_ds);
      Sh.Img[0]->set(_water_nrm); // 'Img0' required by shader 'GetNormal', 'GetNormalMS' functions
      Sh.Img[3]->set(_water_col);
      Sh.Col[0]->set(_water_lum); MaterialClear(); // have to re-use Material texture shader image, because there are no other left, so have to call 'MaterialClear', no need for 'WaterMtrlLast' because we will don't draw any water after this, and later 'WaterMtrlLast' is automatically cleared at start of new water rendering
      REPS(_eye, _eye_num)
      {
         Water.setEyeViewport();
         WS.Apply[refract][depth_test]->draw(); // we need to output depth only if we need it for depth testing
      }
      if(depth_test)
      {
         D.depthUnlock();
         D.depthWrite(true);
      }
      Water.endImages();

      // now we have to modify the depth buffer
      if((!Water._swapped_ds || !swapDS1S(_water_ds)) && Sh.SetDepth) // if we haven't swapped before, or swap back failed, then we have to apply '_water_ds' on top of existing '_ds_1s', otherwise we just swap back '_water_ds' because it had the stencil values
      {
         set(null, _ds_1s, true);
         D.depthLock(true); Sh.Depth->set(_water_ds); Sh.SetDepth->draw(); Sh.Depth->set(_ds_1s); // keep FUNC_LESS to modify only those that are closer
         D.depthUnlock();
      }
      if(_ds!=_ds_1s && Sh.SetDepth) // multi-sample
      {
         set(null, _ds, true);
         D.depthLock(true); Sh.Depth->set(_water_ds); Sh.SetDepth->draw(); Sh.Depth->set(_ds_1s); // keep FUNC_LESS to modify only those that are closer
         D.depthUnlock();
      }

      if(stage)switch(stage)
      {
         case RS_WATER_COLOR : if(show(_water_col, true                  ))return true; break;
         case RS_WATER_NORMAL: if(show(_water_nrm, false, D.signedNrmRT()))return true; break;
         case RS_WATER_LIGHT : if(show(_water_lum, true                  ))return true; break;
      }
   }
  _water_col.clear();
  _water_nrm.clear();
  _water_ds .clear();
  _water_lum.clear();
  _mirror_rt.clear();
   return false;
}
void RendererClass::edgeDetect()
{
   if(D.edgeDetect() && !mirror() && canReadDepth())
   {
      Sky.setFracMulAdd();
      switch(D.edgeDetect()) // here can't use 'D.depth2DOn' because we want to affect sky
      {
         case EDGE_DETECT_THIN:
         {
            D.alpha(ALPHA_MUL); set(_col, _ds, true, NEED_DEPTH_READ); Sh.imgSize(*_ds_1s); Sh.EdgeDetect->draw();
         }break;

         case EDGE_DETECT_SOFT:
         {
            ImageRTPtr edge(ImageRTDesc(fxW(), fxH(), IMAGERT_ONE));
            D.alpha(ALPHA_NONE); set(edge, null, true, NEED_DEPTH_READ); Sh.imgSize(*_ds_1s); Sh.EdgeDetect->draw(); // we need to fill the entire buffer because below we're using blurring (which takes nearby texels)
            D.alpha(ALPHA_MUL ); set(_col, _ds , true,   NO_DEPTH_READ); Sh.imgSize(* edge ); Sh.ImgX[0]->set(edge); Sh.EdgeDetectApply->draw();
         }break;
      }
   }
}
void RendererClass::sky()
{
   Fog.Draw(false);
   Sky.draw();
   if(!mirror())AstroDraw();
   Clouds.drawAll();
      Fog.Draw(true);
}
void RendererClass::blend()
{
   Sky.setFracMulAdd();

   // set main light parameters for *BLEND_LIGHT* and 'Mesh.drawBlend'
   if(Lights.elms() && Lights[0].type==LIGHT_DIR) // use 0 index as it has already been set in 'SortLights'
   {
      Lights[0].dir.set();
     _blst_light_offset=OFFSET(BLST, dir[Lights[0].shadow ? D.shadowMapNumActual() : 0]);
   }else
   {
      LightDir(Vec(0, -1, 0), VecZero).set(); // set dummy light
     _blst_light_offset=OFFSET(BLST, dir[0]);
   }

   // apply light in case of drawing fur, which samples the light buffer
   if(_fur_is)
   {
      if(_ao)
      {
         set(_lum_1s, null, true);
         Sh.ImgX[0]->set(_ao);
         if(D.aoAll())
         {
            D.alpha(ALPHA_MUL);
            Sh.DrawX->draw();
         }else
         {
            D.alpha(ALPHA_ADD);
            Sh.Color[0]->set(Vec4(D.ambientColorD(), 0));
            Sh.Color[1]->set(Vec4Zero                  );
            Sh.DrawXC[0][0]->draw();
         }
      }
      PrepareFur();
   }
  _ao.clear(); // '_ao' will not be used after this point

   D.stencilRef(STENCIL_REF_TERRAIN); // set in case draw codes will use stencil

   const Bool blend_affect_vel=true; // #BlendRT
   set(_col, blend_affect_vel ? _vel() : null, null, null, _ds, true); setDSLookup(); // 'setDSLookup' after 'set'
   D.alpha(ALPHA_BLEND_FACTOR);
   D.set3D(); D.depthWrite(false); D.depthFunc(FUNC_LESS_EQUAL); D.depth(true); mode(RM_BLEND); // use less equal for blend because we may want to draw blend graphics on top of existing pixels (for example world editor terrain highlight)
   SortBlendInstances();
   REPS(_eye, _eye_num)
   {
      setEyeViewport();
   #if 1
     _render(); DrawBlendInstances(); // first call '_render' to for example get 'getBackBuffer' and then draw objects in 'DrawBlendInstances'
   #else
      DrawBlendInstances(); _render();
   #endif
   }
   ClearBlendInstances();
  _SetHighlight(TRANSPARENT);
   D.set2D(); D.depthWrite(true); D.depthFunc(FUNC_LESS);

   D.stencil(STENCIL_NONE); // disable any stencil that might have been enabled

  _lum_1s.clear(); // '_lum_1s' will not be used after this point
}
void RendererClass::palette(Int index)
{
   if(D.colorPaletteAllow())
      if(C ImagePtr &palette=D._color_palette[index])
   {
      ImageRT &ds=(_ds_1s ? *_ds_1s : *_ds); // Warning: this will disable applying palette only on terrain using STENCIL_REF_TERRAIN for multisampling
      Sky.setFracMulAdd();

      ImageRTPtr intensity(ImageRTDesc(_col->w(), _col->h(), IMAGERT_RGBA, ds.samples())); // we need to match depth multi-sampling, here Alpha is used for 4th palette channel
      D.stencilRef(STENCIL_REF_TERRAIN); // set in case draw codes will use stencil

      set(intensity, &ds, true, WANT_DEPTH_READ); setDSLookup(); // we need depth-testing, but want depth-read for particle softing, 'setDSLookup' after 'set'
      D.clearCol();
      D.alpha(ALPHA_ADD);
      D.set3D(); D.depthWrite(false); mode(index ? RM_PALETTE1 : RM_PALETTE);
      REPS(_eye, _eye_num)
      {
         setEyeViewport();
        _render(); if(index)DrawPalette1Objects();else DrawPaletteObjects();
      }
      D.set2D(); D.depthWrite(true);

      D.stencil(STENCIL_NONE); // disable any stencil that might have been enabled

      set(_col, null, true);
      D .alpha(ALPHA_BLEND_DEC);
      Sh.Img[1]->set(palette());
      Sh.PaletteDraw->draw(intensity);
   }
   if(index)
   {
      Palette1Objects.clear();
      Palette1Areas  .clear();
   }else
   {
      PaletteObjects.clear();
      PaletteAreas  .clear();
   }
}
void RendererClass::behind()
{
   if(canReadDepth())
   {
      Sky.setFracMulAdd();

      set(_col, _ds, true, NEED_DEPTH_READ); // we will read from the depth buffer
      D.alpha(ALPHA_BLEND_DEC);
      D.set3D(); D.depthWrite(false); D.depthFunc(FUNC_GREATER); D.depth(true); mode(RM_BEHIND);
      REPS(_eye, _eye_num)
      {
         setEyeViewport();
        _render(); DrawBehindObjects();
      }
      D.set2D(); D.depthWrite(true); D.depthFunc(FUNC_LESS);
   }
   BehindObjects.clear();
}
static const IMAGERT_TYPE IMAGERT_OUTLINE=IMAGERT_SRGBA; // here Alpha is used for outline opacity
void RendererClass::setOutline(C Color &color)
{
  _SetHighlight(color);
   if(!_outline) // not initialized at all
   {
     _outline_rt.get(ImageRTDesc(_col->w(), _col->h(), IMAGERT_OUTLINE, _col->samples())); // here Alpha is used for outline opacity
      set(_outline_rt, _ds, true);
      D.clearCol  ();
      D.alpha     (ALPHA_NONE);
      D.sampler3D ();
      D.depthFunc (FUNC_LESS_EQUAL);
      D.depthWrite(false);
      if(D.outlineMode()==EDGE_DETECT_THIN)D.stencil(STENCIL_OUTLINE_SET, STENCIL_REF_OUTLINE);
   }
   Int           outline_eye=(1<<_eye);
   if(!(_outline&outline_eye)) // not yet enabled for this eye
   {
     _outline|=outline_eye; // enable
      setEyeViewport(); // set viewport if needed
   }
}
void RendererClass::applyOutline()
{
   if(_outline_rt)
   {
     _SetHighlight(TRANSPARENT); // disable 'SetHighlight' which was called during mesh drawing
      D.sampler2D ();
      D.depthFunc (FUNC_LESS); // restore default
      D.depthWrite(true);

      resolveMultiSample(); // don't do 'downSample' here because 'edgeSoften' will be called later and it requires to operate on full-sampled data
      ImageRT *ds=_ds_1s; // we've resolved multi-sample so have to use 1-sample

      if(!Sh.Outline)
      {
         Sh.Outline     =Sh.get("Outline00");
         Sh.OutlineDS   =Sh.get("Outline10");
         Sh.OutlineClip =Sh.get("Outline01");
         Sh.OutlineApply=Sh.get("OutlineApply");
      }

      Sh.imgSize(*_outline_rt);
      switch(D.outlineMode())
      {
         case EDGE_DETECT_THIN: if(Sh.OutlineClip)
         {
            set(_col, (ds && ds->compatible(*_col)) ? ds : null, true);
            D.depth2DOn ();
            D.stencil   ((_cur_ds==_ds) ? STENCIL_OUTLINE_TEST : STENCIL_NONE); // we can use the stencil optimization only if we will use the DS to which we've written stencil to
            D.alpha     (ALPHA_BLEND_DEC);
            Sh.OutlineClip->draw(_outline_rt);
            D.stencil   (STENCIL_NONE);
            D.depth2DOff();
         }break;

         case EDGE_DETECT_SOFT: if(Sh.Outline && Sh.OutlineDS && Sh.OutlineApply)
         {
            ImageRTPtr temp(ImageRTDesc(fxW(), fxH(), IMAGERT_OUTLINE)); // here Alpha is used for outline opacity
            set(temp, null, true);
            D.alpha(ALPHA_NONE);
            ((temp->w()<_outline_rt->w()) ? Sh.OutlineDS : Sh.Outline)->draw(_outline_rt);
            set(_col, (ds && ds->compatible(*_col)) ? ds : null, true);
            D .alpha    (ALPHA_BLEND_DEC);
          //D.depth2DOn(); can't enable because we need to affect sky
            Sh.imgSize(*temp); Sh.OutlineApply->draw(temp);
          //D.depth2DOff();
         }break;
      }
     _outline_rt.clear();
   }
}
void RendererClass::outline()
{
   // start outline
   if(D.outlineMode())
   {
      mode(RM_OUTLINE); // 'sampler3D/2D' is called in 'setOutline' and 'applyOutline'
      REPS(_eye, _eye_num)
      {
       //setEyeViewport(); viewport is set in 'setOutline' method
         DrawOutlineObjects(); _render();
      }
   }
   OutlineObjects.clear();
}
void RendererClass::resolveMultiSample() // !! assumes that 'finalizeGlow' was called !! this should be called before 'downSample'
{
   if(_col->multiSample())
   {
      ImageRTPtr src=_col; _col.get(ImageRTDesc(_col->w(), _col->h(), GetImageRTType(_has_glow, D.litColRTPrecision())));
      src->copyMs(*_col, false, true, D.viewRect());
   }
}
void RendererClass::downSample() // !! assumes that 'finalizeGlow' was called !!
{
   resolveMultiSample();
   if(_col->w()>_final->w()) // if down-sample is needed
   {
      ImageRTPtr src=_col; _col.get(ImageRTDesc(_final->w(), _final->h(), GetImageRTType(_has_glow, D.litColRTPrecision())));
      src->copyHw(*_col, false, D.viewRect());
   }
}
void RendererClass::edgeSoften() // !! assumes that 'finalizeGlow' was called !!
{
   if(hasEdgeSoften())
   {
      resolveMultiSample();
      D.alpha(ALPHA_NONE);
      if(!D._view_main.full)
      {
         const Int pixel_range=6; // currently FXAA/SMAA shaders use this range
         set(_col, null, false); // need full viewport
         D.viewRect().drawBorder(TRANSPARENT, Renderer.pixelToScreenSize(-pixel_range)); // draw black border around the viewport to clear and prevent from potential artifacts on viewport edges
      }
      ImageRTPtr dest(ImageRTDesc(_col->w(), _col->h(), GetImageRTType(_has_glow, D.litColRTPrecision())));
      // D.depth2DOn/depth2DOff can't be applied here, this was tested and resulted in loss of softening at object/sky edges
    //Sh.imgSize(*_col); we can just use 'RTSize' instead of 'ImgSize' since there's no scale
      switch(D.edgeSoften())
      {
         case EDGE_SOFTEN_FXAA:
         {
            Bool gamma=LINEAR_GAMMA, swap=(gamma && _col->canSwapSRV() && dest->canSwapRTV()); if(swap){gamma=false; dest->swapRTV(); _col->swapSRV();} // if we have a non-sRGB access, then just use it instead of doing the more expensive shader, later we have to restore it
            set(dest, null, true); Sh.FXAA[gamma]->draw(_col);
            if(swap){dest->swapRTV(); _col->swapSRV();} // restore
         }break;

      #if SUPPORT_MLAA
         case EDGE_SOFTEN_MLAA:
         {
           _col->copyHw(*dest, false, D.viewRect());
            D.stencil(STENCIL_EDGE_SOFT_SET, STENCIL_REF_EDGE_SOFT); // have to use '_ds_1s' in write mode to be able to use stencil
            ImageRTPtr edge (ImageRTDesc(_col->w(), _col->h(), IMAGERT_TWO )); set(edge (), _ds_1s(), true); D.clearCol(); Sh.MLAAEdge ->draw(_col ()); Sh.Img[1]->set(_mlaa_area()); D.stencil(STENCIL_EDGE_SOFT_TEST);
            ImageRTPtr blend(ImageRTDesc(_col->w(), _col->h(), IMAGERT_RGBA)); set(blend(), _ds_1s(), true); D.clearCol(); Sh.MLAABlend->draw( edge()); Sh.Img[1]->set( blend    ()); edge.clear();
                                                                               set(dest (), _ds_1s(), true);               Sh.MLAA     ->draw(_col ());                               D.stencil(STENCIL_NONE          );
         }break;
      #endif

         case EDGE_SOFTEN_SMAA:
         {
         #if GL // in GL 'ShaderImage.Sampler' does not affect filtering, so modify it manually
            D.texBind(GL_TEXTURE_2D, _smaa_search->_txtr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         #endif

            Bool gamma=LINEAR_GAMMA, swap=(gamma && _col->canSwapSRV()); if(swap){gamma=false; _col->swapSRV();} // if we have a non-sRGB access, then just use it instead of doing the more expensive shader, later we have to restore it
            D.stencil(STENCIL_EDGE_SOFT_SET, STENCIL_REF_EDGE_SOFT); // have to use '_ds_1s' in write mode to be able to use stencil
            ImageRTPtr edge(ImageRTDesc(_col->w(), _col->h(), IMAGERT_TWO)); set(edge, _ds_1s, true); D.clearCol(); Sh.SMAAEdge[gamma]->draw(_col); Sh.Img[1]->set(_smaa_area()); Sh.Img[2]->set(_smaa_search()); Sh.Img[2]->_sampler=&SamplerPoint; D.stencil(STENCIL_EDGE_SOFT_TEST);
            if(swap)_col->swapSRV(); // restore

            ImageRTPtr blend(ImageRTDesc(_col->w(), _col->h(), IMAGERT_RGBA)); // this does not store color, but intensities how much to blend in each axis
            set(blend, _ds_1s, true); D.clearCol(); Sh.SMAABlend->draw(edge); Sh.Img[1]->set(blend); edge.clear(); Sh.Img[2]->_sampler=null; D.stencil(STENCIL_NONE);

            swap=(!LINEAR_GAMMA && dest->canSwapRTV() && _col->canSwapSRV()); if(swap){dest->swapRTV(); _col->swapSRV();} // this we have to perform if we're NOT using Linear Gamma, because if possible, we WANT to use it, as it will improve quality, making AA softer
            set(dest, null, true); Sh.SMAA->draw(_col);
            if(swap){dest->swapRTV(); _col->swapSRV();} // restore
         }break;
      }
      Swap(dest, _col);
   }
}
void RendererClass::volumetric()
{
   if(_vol)
   {
      downSample(); // we're modifying existing RT, so downSample if needed
      set(_col, null, true);
      SPSet("VolMax", Vec(D.volMax()));
      Sh.imgSize(*_vol);
      D.alpha(D.volAdd() ? ALPHA_ADD      : ALPHA_BLEND_DEC);
             (D.volAdd() ? VL.VolumetricA : VL.Volumetric)->draw(_vol);
     _vol.clear();
   }
}
void RendererClass::refract() // !! assumes that 'finalizeGlow' was called !!
{
   if(Water._under_mtrl && canReadDepth() && !fastCombine())
   {
      WS.load();
    C WaterMtrl &under=*Water._under_mtrl;

      Flt    under_step =Sat(Water._under_step),
             refract_val=under_step*under.refract_underwater;
      Bool   refract=(refract_val>EPS_MATERIAL_BUMP);
      if(   !refract)downSample        (); // we're modifying existing RT, so downSample if needed
      else           resolveMultiSample(); // we're writing to new RT so resolve the old first
      ImageRTPtr src=_col;
      if(    refract)_col.get(ImageRTDesc(Min(_col->w(), _final->w()), Min(_col->h(), _final->h()), GetImageRTType(_has_glow, D.litColRTPrecision())));

      set(_col, null, true);
      D .alpha(refract ? ALPHA_NONE : ALPHA_BLEND_DEC);
      Sh.Step->set(Time.time());
      SPSet("WaterPlnPos"   , Water._under_plane.pos   *CamMatrixInv      );
      SPSet("WaterPlnNrm"   , Water._under_plane.normal*CamMatrixInv.orn());
      SPSet("WaterUnder"    ,        under_step);
      SPSet("WaterUnderRfr" ,       refract_val);
      SPSet("WaterDns"      , Vec2(Mid(under.density_underwater , 0.0f, 1-EPS_GPU), under.density_underwater_add)); // avoid 1 in case "Pow(1-density, ..)" in shader would cause NaN or slow-downs
      SPSet("WaterUnderCol0", SRGBToDisplay(under.color_underwater0));
      SPSet("WaterUnderCol1", SRGBToDisplay(under.color_underwater1));
      REPS(_eye, _eye_num)WS.Under[refract]->draw(src, setEyeParams());
   }
}
void RendererClass::postProcess()
{
   Bool  eye_adapt= hasEyeAdapt(),
         bloom    =(hasBloom   () || _has_glow),
         motion   = hasMotion  (),
         dof      = hasDof     (),
         combine  = slowCombine(), // shader combine
         upscale  =(_final->w()>_col->w() || _final->h()>_col->h()), // we're going to upscale at the end
         fx_dither=(D.dither() && !upscale), // allow post process dither only if we're not going to upscale the image (because it would look bad)
         alpha_set=fastCombine(); // if alpha channel is set properly in the RT, skip this if we're doing 'fastCombine' because we're rendering to existing RT which has its Alpha already set
   VecI2 size     =_col->size(); MIN(size.x, _final->w()); MIN(size.y, _final->h()); // don't do post-process at higher res than needed

   D.alpha(ALPHA_NONE);

   ImageRTPtr dest;

   if(eye_adapt || bloom || motion || dof || combine || _get_target)resolveMultiSample(); // we need to resolve the MS Image so it's smooth for the effects

   Int fxs=((upscale || _get_target) ? -1 : eye_adapt+bloom+motion+dof+combine); // this counter specifies how many effects are still left in the queue, and if we can render directly to '_final', when up sampling then don't render to '_final'
   if( D._view_main.full && !_get_target && !combine && _col!=_final)_final->discard();
   if(!D._view_main.full)Sh.ImgClamp->setConditional(imgClamp(size)); // set 'ImgClamp' that may be needed for Bloom, DoF, MotionBlur, this is the viewport rect within texture, so reading will be clamped to what was rendered inside the viewport

   IMAGE_PRECISION rt_prec=D.litColRTPrecision();
   if(!_get_target) // if we're going to output to the monitor
   {
      IMAGE_PRECISION monitor_prec=D.monitorPrecision();
      if(fx_dither && monitor_prec==IMAGE_PRECISION_8)monitor_prec=IMAGE_PRECISION_10; // if we allow dither and it will be used (only for 8-bit) then operate on little better (10-bit) precision from which we can generate the dither
      MIN(rt_prec,    monitor_prec);
   }
   if(eye_adapt)
   {
      if(!--fxs)dest=_final;else dest.get(ImageRTDesc(size.x, size.y, GetImageRTType(_has_glow, rt_prec))); // can't read and write to the same RT, glow requires Alpha channel
      T.adaptEye(*_col, *dest, fx_dither); Swap(_col, dest); // Eye Adaptation keeps Alpha
   }
   IMAGERT_TYPE rt_type=GetImageRTType(false, rt_prec); // there's no need for alpha channel anymore after bloom
   if(bloom) // bloom needs to be done before motion/dof especially because of per-pixel glow
   {
      if(!--fxs)dest=_final;else dest.get(ImageRTDesc(size.x, size.y, rt_type)); // can't read and write to the same RT
      T.bloom(*_col, *dest, fx_dither); alpha_set=true; Swap(_col, dest); // Bloom sets Alpha
   }
   if(motion) // tests have shown that it's better to do Motion Blur before Depth of Field
   {
      if(!--fxs)dest=_final;else dest.get(ImageRTDesc(size.x, size.y, rt_type)); // can't read and write to the same RT
      if(T.motionBlur(*_col, *dest, fx_dither))return; alpha_set=true; Swap(_col, dest); // Motion Blur sets Alpha
   }
   if(dof) // after Motion Blur
   {
      if(!--fxs)dest=_final;else dest.get(ImageRTDesc(size.x, size.y, rt_type)); // can't read and write to the same RT
      T.dof(*_col, *dest, fx_dither); alpha_set=true; Swap(_col, dest); // DoF sets Alpha
   }
   if(ms_samples_color.a && D.multiSample())
   {
      D.alpha(ALPHA_BLEND);
      D.stencil(STENCIL_MSAA_TEST, STENCIL_REF_MSAA);
      set(_col, _ds_1s, true);
      D.viewRect().draw(ms_samples_color, true);
      D.stencil(STENCIL_NONE);
   }

   // 'upscale' will happen somewhere below

   if(combine)
   {
      T.Combine(rt_prec); alpha_set=true; // Combine sets Alpha
   }

   if(!_get_target) // for '_get_target' leave the '_col' result for further processing
   {
      if(_col!=_final)
      {
         if(_col->multiSample())
         {
            if(_col->size()==_final->size()){_col->copyMs(*_final, false, true, D.viewRect()); _col=_final;}else resolveMultiSample(); // if the size is the same then we can resolve directly into the '_final', otherwise resolve first to temp RT and copy will be done below
         }
         if(_col!=_final) // if after resolve this is still not equal, then
         {
            D.alpha(ALPHA_NONE);
            set(_final, null, true);
            Bool    dither=(D.dither() && !_final->highPrecision()); // disable dithering if destination has high precision
            Shader *shader=null;
            if(upscale)switch(D.densityFilter()) // remember that cubic shaders are optional and can be null if failed to load
            {
               case FILTER_NONE:
               {
               #if DX11
                  SamplerPoint.setPS(SSI_DEFAULT);
               #elif GL // in GL 'ShaderImage.Sampler' does not affect filtering, so modify it manually
                  D.texBind(GL_TEXTURE_2D, _col->_txtr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
               #endif
               }break;

               case FILTER_CUBIC_FAST       :
               case FILTER_CUBIC_FAST_SMOOTH:
               case FILTER_CUBIC_FAST_SHARP : Sh.imgSize(*_col); shader=Sh.DrawTexCubicFastFRGB[dither]; break; // this doesn't need to check for "_col->highPrecision" because resizing and cubic filtering generates smooth values

               case FILTER_BEST       :
               case FILTER_CUBIC      :
               case FILTER_CUBIC_SHARP: Sh.imgSize(*_col); Sh.loadCubicShaders(); shader=Sh.DrawTexCubicFRGB[dither]; break; // this doesn't need to check for "_col->highPrecision" because resizing and cubic filtering generates smooth values
            }
            if(!shader)
            {
               if(dither && (_col->highPrecision() || _col->size()!=_final->size()))shader=Sh.Dither;  // allow dithering only if the source has high precision, or if we're resizing (because that generates high precision too)
               else                                               {Sh.Step->set(1); shader=Sh.DrawA ;} // use 'DrawA' to set Alpha Channel
            }
            shader->draw(_col); alpha_set=true;
            if(upscale && D.densityFilter()==FILTER_NONE)
            {
            #if DX11
               SamplerLinearClamp.setPS(SSI_DEFAULT);
            #elif GL
               if(_col->filterable()){D.texBind(GL_TEXTURE_2D, _col->_txtr); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);}
            #endif
            }
         }
      }
     _col.clear(); // release as it's no longer needed
      if(!alpha_set && _back==_final) // if we need to have alpha channel set for back buffer effect
      {
         set(_final, null, true);
         D.alpha(ALPHA_ADD);
         Sh.clear(Vec4(0, 0, 0, 1)); // force full alpha so back buffer effects can work ok
      }
   }
}
/******************************************************************************/
}
/******************************************************************************/
