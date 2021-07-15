/******************************************************************************/
#include "!Header.h"
/******************************************************************************

   Motion Blur is blurred in both ways:
      -from current position to previous position
      -from current position to new      position (new is estimated as opposite of previous)
   This mode is not correct, future is not yet known, however it helps prevent "leaking" when rotating camera around an object,
      where one side doesn't get blurred because it wants to get samples behind the object however they're not avaialble since the object covers them,
      so when blurring in both ways, we can just use samples from the other side

/******************************************************************************/
#ifndef VIEW_FULL
#define VIEW_FULL 1
#endif

#ifndef JITTER
#define JITTER 0
#endif

#ifndef RANGE
#define RANGE 0
#endif

#ifndef SAMPLES
#define SAMPLES 1
#endif

#ifndef GATHER
#define GATHER 0
#endif

#ifndef HAS_TAA
#define HAS_TAA 0
#endif

// disable PRECISE because for it to work best 'color_near' would have to be processed along 'base_uv_motion' line, however right now: PRECISE=1 improves moving background around moving object, however it has negative effect of unnatural blurring FPP weapons when rotating camera fast left/right constantly with a key, and with mouse up/down (while weapon rotates slightly based on up/down angle) in that case the weapons get blurred way too much
#define PRECISE 0 // if precisely (separately) calculate samples for far and near (base/center), this is to solve the problem of rotating camera in FPP view, with weapon attached to player/camera. in that case background is rotating, and on the background blur line it encounters an object (weapon) that is in focus. Blur algorithm counts the far samples that move over the base center, and then lerps to the near samples that move over the base center.

#define DEPTH_TOLERANCE 0.2 // 20 cm

#define SHOW_BLUR_PIXELS 0 // show what pixels actually get blurred (they will be set to GREEN for fast blur and RED for slow blur) use only for debugging

#if ALPHA
   #define MASK rgba
   #define COL  Vec4
   #define COLH VecH4
#else
   #define MASK rgb
   #define COL  Vec
   #define COLH VecH
#endif

#if 0 // use only for testing for fast compilation
   #define FAST_COMPILE 1
   #define UNROLL LOOP
#endif
#define FAST_UNROLL [unroll] // always unroll because the operation is fast and won't slow down compilation
/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(MotionBlur)
   Half MotionScale_2; // MotionScale/2 is used because we blur in both ways (read above why), so we have to make scale 2x smaller
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
VecH2 UVToScreen   (VecH2 uv) {return VecH2(uv.x*AspectRatio, uv.y);} // this is only to maintain XY proportions (it does not convert to screen coordinates)
Half  ScreenLength2(VecH2 uv) {return Length2(UVToScreen(uv));}
/******************************************************************************/
VecH2 GetMotionCameraOnly(Vec view_pos, Vec2 uv)
{
   Vec view_pos_prev=Transform(view_pos, ViewToViewPrev); // view_pos/ViewMatrix*ViewMatrixPrev
   return PosToUV(view_pos_prev) - uv;
}
void SetVel_VS(VtxInput vtx,
    NOPERSP out Vec2 uv   :UV,
    NOPERSP out Vec2 posXY:POS_XY,
    NOPERSP out Vec4 pixel:POSITION)
{
   uv   =vtx.uv();
   posXY=UVToPosXY(uv);
   pixel=vtx.pos4();
}
VecH2 SetVel_PS(NOPERSP Vec2 uv   :UV,
                NOPERSP Vec2 posXY:POS_XY):TARGET
{
   Vec pos=GetPosPoint(uv, posXY);
   return GetMotionCameraOnly(pos, uv);
}
/******************************************************************************/
void GetPixelMotion(VecH2 uv_motion, VecH2 uv_to_pixel, out VecH2 pixel_motion, out VecH2 pixel_motion_perp, out VecH2 pixel_motion_size) // 'pixel_motion'=direction, 'pixel_motion_perp'=direction perpendicular, 'pixel_motion_size'=size to check along directions
{
         pixel_motion=uv_motion*uv_to_pixel; // convert UV motion to pixel motion
 //Half  pixel_motion_len2=Length2(pixel_motion);
   Half  pixel_motion_len=Length(pixel_motion);
 //if(  !pixel_motion_len)return; slows down
   if(   pixel_motion_len)pixel_motion/=pixel_motion_len; // normalize
         pixel_motion_perp=Perp(pixel_motion); // perpendicular to movement
   Half  pixel_ext=(Abs(pixel_motion.x)+Abs(pixel_motion.y))*2; // make smallest movements dilate neighbors, increase extent because later we use linear filtering, so because of blending movement might get minimized, especially visible in cases such as moving object to the right (motion x=1, y=0) row up and down could have velocity at 0 and linear filtering could reduce motion at the object border (2 value was the smallest that fixed most problems)
         pixel_motion_size=VecH2(pixel_ext+pixel_motion_len, pixel_ext);
}
/******************************************************************************/
void Process(inout VecH4 motion, inout VecH2 length2, VecH2 sample_motion) // XY=biggest, ZW=smallest
{
   Half sample_len2=ScreenLength2(sample_motion);
   if(  sample_len2>length2.x){length2.x=sample_len2; motion.xy=sample_motion;} // biggest
   if(  sample_len2<length2.y){length2.y=sample_len2; motion.zw=sample_motion;} // smallest
}
void Process(inout VecH4 max_min_motion, inout VecH2 length2, VecH4 sample_motion, VecH2 pixel_delta, VecH2 uv_to_pixel, VecH2 base_pixel_motion, VecH2 base_pixel_motion_perp, VecH2 base_pixel_motion_size) // XY=biggest, ZW=smallest
{
 //if(all(Abs(sample_motion.xy)>=Abs(uv_delta))) this check slows down so don't use, also it would need some EPS +eps (ImgSize/RTSize *1 *0.99 *0.5) etc.
   {
      VecH2                                         sample_pixel_motion, sample_pixel_motion_perp, sample_pixel_motion_size ;
      GetPixelMotion(sample_motion.xy, uv_to_pixel, sample_pixel_motion, sample_pixel_motion_perp, sample_pixel_motion_size);
      VecH2   base_pos_motion=VecH2(Dot(pixel_delta, sample_pixel_motion     ),  // base   position along            sample motion vector
                                    Dot(pixel_delta, sample_pixel_motion_perp)); // base   position perpendicular to sample motion vector
      VecH2 sample_pos_motion=VecH2(Dot(pixel_delta,   base_pixel_motion     ),  // sample position along            base   motion vector
                                    Dot(pixel_delta,   base_pixel_motion_perp)); // sample position perpendicular to base   motion vector
      Bool sample_reaches_base  =all(Abs(  base_pos_motion)<sample_pixel_motion_size);
      Bool   base_reaches_sample=all(Abs(sample_pos_motion)<  base_pixel_motion_size);
      if(sample_reaches_base) // if sample reaches base
      {
         Half sample_len2=ScreenLength2(sample_motion.xy); if(sample_len2>length2.x){length2.x=sample_len2; max_min_motion.xy=sample_motion.xy;} // biggest
      }
      if(sample_reaches_base || base_reaches_sample) // if either sample reaches base or base reaches sample, adjust smallest
      {
         Half sample_len2=ScreenLength2(sample_motion.zw); if(sample_len2<length2.y){length2.y=sample_len2; max_min_motion.zw=sample_motion.zw;} // smallest
      }
   }
}
/******************************************************************************

   Convert finds the Max and Min Motions (determined by their screen length, screen = UV corrected by aspect ratio)
       Input =         UV Motion
      Output = Max Min UV Motion * MotionScale

/******************************************************************************/
VecH4 Convert_PS(NOPERSP Vec2 uv:UV):TARGET
{
   // WARNING: code below might still set ZW (smallest) to some very small values, only XY gets forced to 0
   VecH2 length2=VecH2(ScreenLength2(ImgSize.xy)*Sqr(0.5*2), 2); // x=biggest, y=smallest, initially set biggest to 0 so it always gets updated (actually set to half of pixel (*2 because later we mul by 'MotionScale_2' instead of 'MotionScale') to make sure we will ignore small motions and keep 0), initially set smallest to 2 so it always gets updated
   VecH4 motion =0; // XY=biggest, ZW=smallest
#if 0 // process samples individually
   // for RANGE=1 (no scale  ) uv should remain unmodified              , because it's already at the center of 1x1 texel
   // for RANGE>1 (downsample) uv should be moved to the center of texel, because it's         at the center of 2x2 texels
   if(RANGE>1)uv+=ImgSize.xy*0.5;
   const Int ofs=RANGE/2, min=0-ofs, max=RANGE-ofs;
   UNROLL for(Int y=min; y<max; y++)
   UNROLL for(Int x=min; x<max; x++)
      Process(motion, length2, TexPoint(ImgXY, UVInView(uv+Vec2(x, y)*ImgSize.xy, VIEW_FULL)).xy);
#else // process samples in 2x2 blocks using linear filtering
   // for RANGE=1 (no scale          ) offset should be 0, because uv is already at the center of 1x1 texel
   // for RANGE=2 (2x downsample, 2x2) offset should be 0, because uv is already at the center of 2x2 texels (linear filtering is used)
   // for RANGE=4 (4x downsample, 4x4) offset should be 1, because uv is already at the center of 4x4 texels, however we want to process (2x2 2x2) so have to position at the center of top left 2x2
   // for RANGE=8 (8x downsample, 8x8) offset should be 3                                                                                   (2x2 2x2)
   const Int ofs=(RANGE-1)/2, min=0-ofs, max=RANGE-ofs; // correctness can be verified with this code: "Int RANGE=1,2,4,8; const Int ofs=(RANGE-1)/2, min=0-ofs, max=RANGE-ofs; Str s; for(Int x=min; x<max; x+=2)s.space()+=x; Exit(s);"
   #if RANGE<=(GL ? 16 : 256) // for GL limit to 16 because compilation is very slow
      UNROLL for(Int y=min; y<max; y+=2)
      UNROLL for(Int x=min; x<max; x+=2)
   #else
      LOOP for(Int y=min; y<max; y+=2)
      LOOP for(Int x=min; x<max; x+=2)
   #endif
         Process(motion, length2, TexLod(ImgXY, UVInView(uv+Vec2(x, y)*ImgSize.xy, VIEW_FULL)).xy);
#endif
   motion*=MotionScale_2; // for best precision this should be done for every sample, however doing it here just once, increases performance
   { // limit max length - this prevents stretching objects to distances the blur can't handle anyway, making only certain samples of it visible but not all
      length2*=Sqr(MotionScale_2); // since we've scaled 'motion' above after setting 'length2', then we have to scale 'length2' too
      Half max_length=0.2/2; // #MaxMotionBlurLength
      if(length2.x>Sqr(max_length))motion.xy*=max_length/Sqrt(length2.x);
    //if(length2.y>Sqr(max_length))motion.zw*=max_length/Sqrt(length2.y); don't have to scale Min motion, because it's only used to detect fast/simple blur
   }
 //if(length2.x<ScreenLength2(ImgSize.xy)*Sqr(0.5))motion=0; // motions less than 0.5 pixel size force to 0 (ignore this code because this is done faster by just setting initial value of 'length2.x')
   return motion;
}
/******************************************************************************

    Input =         Max Min UV Motion * MotionScale
   Output = Dilated Max Min UV Motion * MotionScale

/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
// Dilate doesn't use UV clamping, instead border around viewport is cleared
VecH4 Dilate_PS(NOPERSP Vec2 uv:UV):TARGET
{
   VecH4 motion =TexPoint(Img, uv);
   VecH2 length2=VecH2(ScreenLength2(motion.xy), ScreenLength2(motion.zw));
   VecH2 uv_to_pixel=RTSize.zw, pixel_motion, pixel_motion_perp, pixel_motion_size;
   GetPixelMotion(motion.xy, uv_to_pixel, pixel_motion, pixel_motion_perp, pixel_motion_size);

#if !FAST_COMPILE && RANGE<=(GL ? 5 : 7) // only up to 7 is supported here because 'TexPointOfs' accepts offsets in -8..7 range, for GL limit to 5 because compilation is very slow
   UNROLL for(Int y=-RANGE; y<=RANGE; y++)
   UNROLL for(Int x=-RANGE; x<=RANGE; x++)
      if(x || y)Process(motion, length2, TexPointOfs(Img, uv, VecI2(x, y)), VecH2(x, y), uv_to_pixel, pixel_motion, pixel_motion_perp, pixel_motion_size);
#else
   LOOP for(Int y=-RANGE; y<=RANGE; y++)
   LOOP for(Int x=-RANGE; x<=RANGE; x++)
      if(x || y)Process(motion, length2, TexPoint(Img, uv+Vec2(x, y)*RTSize.xy), VecH2(x, y), uv_to_pixel, pixel_motion, pixel_motion_perp, pixel_motion_size);
#endif
   return motion;
}
/******************************************************************************/
// BLUR
/******************************************************************************/
VecH2 SampleWeight(Flt base_depth, Flt sample_depth, Half base_uv_motion_len, Half sample_uv_motion_len, Half uv_motion_len_to_step, Half step) // X=weight to be used for the specific sample, Y=if this sample is covered by base movement
{
   Half   depth_delta =sample_depth-base_depth;
   VecH2  depth_weight=Sat(depth_delta*VecH2(1.0/DEPTH_TOLERANCE, -1.0/DEPTH_TOLERANCE)+0.5); // X=how much base is in front of sample, Y=how much sample is in front of base. 0.5 (middle) is returned if both have the same depth. Here it's always X=1-Y and Y=1-X
   VecH2 motion_weight=Sat(VecH2(base_uv_motion_len, sample_uv_motion_len)*uv_motion_len_to_step-step); // have to convert motion into step (instead of step into motion) to make sure that we reach 1.0 weights, this weight is about checking if one position motion is covering another position (smoothly). X=if base motion is covering sample, Y=if sample motion is covering base
 //depth_weight.x*motion_weight.x = this is needed for cases where base is the moving object     and sample is a static background (base  =object is in front, so depth_weight.x=1, base  =object is moving so motion_weight.x=1), we're returning weight=1 so background sample will be used on the object     position, which will make it appear semi-transparent
 //depth_weight.y*motion_weight.y = this is needed for cases where base is the static background and sample is a moving object     (sample=object is in front, so depth_weight.y=1, sample=object is moving so motion_weight.y=1), we're returning weight=1 so object     sample will be used on the background position, which will draw object on top of background
   Half weight=Dot(depth_weight, motion_weight); // return sum of both cases, this will always be 0..1, because even if both base and sample have motion_weight, then depth weight is always X=1-Y
   return VecH2(weight, PRECISE ? motion_weight.x*depth_weight.x : motion_weight.x); // for PRECISE also mul by base depth_weight, because 'color_near' would have to be processed along 'base_uv_motion' line to don't do extra mul, but now we always do along biggest motion line, so treat as far only if depth is good, because without it, it just increases weight of fast moving objects when rotating camera around them
}
Half UVMotionLength(VecH2 uv_motion)
{
 //return Abs(Dot(uv_motion, Normalize(dir.xy)))*MotionScale_2; don't use because it might lower blurring (if object is moving, but camera is rotating faster and in perpendicular way to the object movement, camera blur direction would take priority, then object motion would become 0, and it would become focused and not blurry, so better to keep the wrong blur object direction as long as it's blurry)
   return Length(uv_motion)*MotionScale_2; // here do not use AspectRatio/UVToScreen because we need UV's
}
/******************************************************************************

   Img1  = dilated = downsampled Max Min UV Motion * MotionScale
   ImgXY =              full-res         UV Motion      (unscaled)

/******************************************************************************/
VecH4 Blur_PS(NOPERSP Vec2 uv0:UV,
              NOPERSP PIXEL      ):TARGET
{
   VecH4 dilated=TexLod(Img1, uv0); // dilated motion (XY=biggest, ZW=smallest), use linear filtering because 'Img1' may be smaller
   VecH4 base_color; base_color.MASK=TexLod(Img, uv0).MASK; // can't use 'TexPoint' because 'Img' can be supersampled
#if !ALPHA
   base_color.a=1; // force full alpha so back buffer effects can work ok
#endif
   //return VecH4(SRGBToLinear(Vec(Length(dilated.xy)*10,Length(dilated.zw)*10,0)),1);

   BRANCH if(any(dilated.xy)) // XY=biggest, can use 'any' because small motions were already forced to 0 in 'Convert'
   {
      Vec4 dir=Vec4(dilated.xy, -dilated.xy);
      Int  steps=SAMPLES;
      dir/=steps;
      Half jitter; if(JITTER)jitter=Noise1D_4(pixel.xy); // use only 4 step dither because others might be too distracting (individual pixels visible)
      Vec2 uv1=uv0;

      BRANCH if(Length2(dilated.zw)>Length2(dilated.xy)*Sqr(0.5)) // if smallest motion is close to biggest motion then just do a fast and simple blur, ignoring depths and precise motions
      {
         COL color_blur=JITTER ? 0 : base_color.MASK; // use HP because we operate on many samples (when using JITTER the base sample has to be jittered too so we can't use 'base_color')
         Flt weight    =                           0; // use HP because we operate on many samples
         if(JITTER) // for JITTER we have to process steps starting from 0.5 because we're not leaving extra weight for the base sample (since it has to be jittered too), so move the starting UV's by 0.5 back and apply jitter offset
         {
            Half step0=(/*i=0*/-0.5)+jitter; uv0+=step0*dir.xy;
            Half step1=(/*i=0*/-0.5)-jitter; uv1+=step1*dir.zw;
         }
         UNROLL for(Int i=1; i<=steps; i++) // start from 1 because we've already got #0 before
         {
            Half w0=UVInsideView(uv0+=dir.xy);
            Half w1=UVInsideView(uv1+=dir.zw);
            color_blur+=w0*TexLod(Img, uv0).MASK  // use linear filtering
                       +w1*TexLod(Img, uv1).MASK; // use linear filtering
            weight+=w0+w1;
         }
         if(!JITTER || weight>0.5) // update only if have any samples
            base_color.MASK=color_blur/(weight+(JITTER ? 0 : 1)); // already have 1 sample (only used without JITTER)

         if(SHOW_BLUR_PIXELS)base_color.g+=0.1;
      }else
      {
         COL  color_near=0; // use HP because we operate on many samples
         COL  color_far =0; // use HP because we operate on many samples
         Vec2 weight    =0; // use HP because we operate on many samples (X=near, Y=far)

         // GET DEPTH + MOTION
         Flt   base_depth;
         VecH2 base_uv_motion;
         Half  base_uv_motion_len;
         if(HAS_TAA) // in TAA the color RT is already adjusted by UV (to be always the same each frame), but depth and motion RT's are jittered every frame, which gives inconsistency between color and DepthMotion.
          {// to workaround this problem, DepthMotion are taken from the pixel that's closest to camera and has highest motion
            // TODO: Warning: these ignore UVClamp/UVInView
            if(GATHER)
            { 
               Vec2  taa_uv=uv0+TAAOffset;
               Vec4  d=TexDepthRawGather(taa_uv);
               VecH4 r=TexGatherR(ImgXY, taa_uv); // motion.x
               VecH4 g=TexGatherG(ImgXY, taa_uv); // motion.y

               VecH2 test_uv_motion; Half test_len;
               if(1) // slower, higher quality. This improves blur on pixels around object (border). To verify improvement, set very low 'D.density', and rotate player+camera in TPP very fast left or right, object has to be fixed to the camera, and background rotating/blurry, you will see that this mode improves smoothness of object border pixels.
               {
                 // set initial values with 'TAAOffsetGatherIndex' as if we were using 'uv0' without 'TAAOffset'
                  base_depth        =      d[TAAOffsetGatherIndex];
                  base_uv_motion    =VecH2(r[TAAOffsetGatherIndex], g[TAAOffsetGatherIndex]);
                  base_uv_motion_len=Length2(base_uv_motion);

                  test_uv_motion=VecH2(r.x, g.x); test_len=Length2(test_uv_motion); if(DEPTH_SMALLER(d.x, base_depth) && test_len>base_uv_motion_len){base_depth=d.x; base_uv_motion=test_uv_motion ; base_uv_motion_len=test_len;}
               }else{                                                                                                                                 base_depth=d.x; base_uv_motion=VecH2(r.x, g.x); base_uv_motion_len=Length2(base_uv_motion);} // faster, lower quality, just set from first one
                  test_uv_motion=VecH2(r.y, g.y); test_len=Length2(test_uv_motion); if(DEPTH_SMALLER(d.y, base_depth) && test_len>base_uv_motion_len){base_depth=d.y; base_uv_motion=test_uv_motion ; base_uv_motion_len=test_len;}
                  test_uv_motion=VecH2(r.z, g.z); test_len=Length2(test_uv_motion); if(DEPTH_SMALLER(d.z, base_depth) && test_len>base_uv_motion_len){base_depth=d.z; base_uv_motion=test_uv_motion ; base_uv_motion_len=test_len;}
                  test_uv_motion=VecH2(r.w, g.w); test_len=Length2(test_uv_motion); if(DEPTH_SMALLER(d.w, base_depth) && test_len>base_uv_motion_len){base_depth=d.w; base_uv_motion=test_uv_motion ; base_uv_motion_len=test_len;}
            }else
            { // Warning: this ignores 'TAAOffsetGatherIndex'
               Vec2 test_uv=uv0-(TAAOffset<0)*RTSize.xy; // if TAAOffset is negative, then move starting UV to negative too
               FAST_UNROLL for(Int y=0; y<=1; y++)
               FAST_UNROLL for(Int x=0; x<=1; x++)
               {
                  Flt   test_depth    =TexDepthRawPointOfs(test_uv, VecI2(x, y));
                  VecH2 test_uv_motion=TexPointOfs (ImgXY, test_uv, VecI2(x, y));
                  Half  test_len      =Length2     (test_uv_motion);
                  if((x==0 && y==0) // first sample
                  || (DEPTH_SMALLER(test_depth, base_depth)
                   &&               test_len  > base_uv_motion_len))
                  {
                     base_depth        =test_depth;
                     base_uv_motion    =test_uv_motion;
                     base_uv_motion_len=test_len;
                  }
               }
            }
            base_uv_motion_len=Sqrt(base_uv_motion_len)*MotionScale_2;
         }else
         {
            base_depth        =TexDepthRawPoint(uv0);
            base_uv_motion    =TexPoint(ImgXY,  uv0);
            base_uv_motion_len=UVMotionLength(base_uv_motion);
         }
         base_depth=LinearizeDepth(base_depth);

         Half uv_motion_len_to_step0=1/Length(dir.xy); // allows to convert travelled UV distance into how many steps (travelled_uv*uv_motion_len_to_step=step)
         Half uv_motion_len_to_step1=1/Length(dir.zw);
         Half step_add=-1.5; // this value allows the last step to still has some weight, use -1.5 instead of -1 because on a 3D ball moving right, pixels in the center have higher movement due to perspective correction (pixels closer to camera move faster than those far), so when calculating biggest movement from neighbors, then the pixels at the border will get biggest/dilated movement (coming from ball center) that's slightly bigger than border movement. So the search vector that's set from biggest/dilated motion will be bigger than the sample movement, and for example its motion might cover only 9/10 steps instead of 10/10. To workaround this, make step offset slightly smaller.
         if(JITTER) // for JITTER we have to process steps starting from 0.5 because we're not leaving extra weight for the base sample (since it has to be jittered too), so move the starting UV's by 0.5 back and apply jitter offset
         {
            Half step0=(/*i=0*/-0.5)+jitter; uv0+=step0*dir.xy;
            Half step1=(/*i=0*/-0.5)-jitter; uv1+=step1*dir.zw;
         }
         UNROLL for(Int i=1; i<=steps; i++) // start from 1 because #0 is processed at the end
         {
            Half step0, step1;
            if(JITTER)
            {
             /*step0=(i-0.5)+jitter;
               step1=(i-0.5)-jitter;
               uv0  =start+step0*dir.xy;
               uv1  =start+step1*dir.zw;
               step0=Max(0, step0+step_add);
               step1=Max(0, step1+step_add);*/
               step0=Max(0, (i-0.5+step_add)+jitter);
               step1=Max(0, (i-0.5+step_add)-jitter);
            }else
            {
               step0=step1=Max(0, i+step_add);
            }
            uv0+=dir.xy;
            uv1+=dir.zw;

            // need to disable filtering to avoid ghosting on borders
            VecH2 sample0_uv_motion=TexPoint(ImgXY, uv0).xy; Flt sample0_depth=TexDepthPoint(uv0);
            VecH2 sample1_uv_motion=TexPoint(ImgXY, uv1).xy; Flt sample1_depth=TexDepthPoint(uv1);

            Half sample0_uv_motion_len=UVMotionLength(sample0_uv_motion);
            Half sample1_uv_motion_len=UVMotionLength(sample1_uv_motion);

            VecH2 w0=SampleWeight(base_depth, sample0_depth, base_uv_motion_len, sample0_uv_motion_len, uv_motion_len_to_step0, step0); w0.x*=UVInsideView(uv0);
            VecH2 w1=SampleWeight(base_depth, sample1_depth, base_uv_motion_len, sample1_uv_motion_len, uv_motion_len_to_step1, step1); w1.x*=UVInsideView(uv1);

            COLH col0=TexPoint(Img, uv0).MASK,
                 col1=TexPoint(Img, uv1).MASK;

            if(PRECISE)
            {
               w0=w0.x*VecH2(w0.y, 1-w0.y); // adjust X weight to be sample weight * near weight, Y weight to be sample weight * far weight (1-near)
               w1=w1.x*VecH2(w1.y, 1-w1.y); // adjust X weight to be sample weight * near weight, Y weight to be sample weight * far weight (1-near)
               color_near+=col0*w0.x + col1*w1.x;
               color_far +=col0*w0.y + col1*w1.y;
            }else
            {
               color_near+=col0*w0.x + col1*w1.x; // in !PRECISE we just need one color (doesn't matter if we operate on 'color_far' or 'color_near', so just choose any)
               w0.y=Max(w0.y, w0.x); // adjust Y weight to be maximum of (near weight (Y) and sample weight (X)), it works as if sample weight but if it's near then it's always 1 (to ignore depth: anything covering the base sample), this weight will be used to blend with the final color and the base color
               w1.y=Max(w1.y, w1.x);
            }
            weight+=w0+w1;
         }
         if(PRECISE)
         {
            // FIXME optimize
            // Warning: these codes don't adjust 'weight.x'
            if(JITTER)
            {
               if(weight.x<1)color_near.MASK+=(1-weight.x)*base_color.MASK; // if gathered near colors are less than 1 full sample, then increase to 'base_color' (use it only if necessary because we want 'base_color' to be jittered)
               else          color_near/=weight.x; // otherwise normalize
            }else
            {
               color_near.MASK+=base_color.MASK; // here we can always add base sample because we don't need it to be jittered
               color_near/=(weight.x+1); // normalize including base sample
            }
         #if 0 // original
            if(weight.y)color_far/=weight.y; // normalize
            weight.y*=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            base_color.MASK=Lerp(color_near, color_far, weight.y);
         #else // optimized
            color_far*=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            weight.y *=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            base_color.MASK=color_near*(1-weight.y) + color_far;
         #endif
         }else
         if(1) // process using near/far weights
         {
            if(weight.x) // process if we've got any samples (if not then just keep 'base_color' as is)
            {
               weight.y*=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            #if 0 // original
               color_near/=weight.x; // normalize
               base_color.MASK=Lerp(base_color.MASK, color_near.MASK, weight.y);
            #else // optimized
               base_color.MASK=base_color.MASK*(1-weight.y) + color_near.MASK*(weight.y/weight.x);
            #endif
            }
         }else // simple mode ignoring near/far
         {
            color_near*=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            weight.x  *=1.0/(steps*2+(JITTER ? 0 : 1)); // in every step we have 2 samples + 1 to make room for base sample (can't do that for JITTER because when base is moving then it has to be jittered too, so it has to be processed by codes in the loop)
            base_color.MASK=base_color.MASK*(1-weight.x) + color_near.MASK;
         }

         if(SHOW_BLUR_PIXELS)base_color.r+=0.1;
      }

   #if 0 // test how many samples were used for blurring
      base_color.rgb=steps/Half(SAMPLES);
   #endif

   #if DITHER
      ApplyDither(base_color.rgb, pixel.xy);
   #endif
   }
   return base_color;
}
/******************************************************************************
void Explosion_VS(VtxInput vtx,
              out Vec  outPos:TEXCOORD0,
              out Vec  outVel:TEXCOORD1,
              out Vec4 pixel :POSITION )
{
   outVel=TransformTP (Normalize(vtx.pos())*Step, (Matrix3)CamMatrix);
   outPos=TransformPos(vtx.pos());
   pixel =Project     ( outPos  );
}
void Explosion_PS(Vec   inPos:TEXCOORD0,
                  Vec   inVel:TEXCOORD1,
              out VecH outVel:TARGET1  ) // #RTOutput
{
   need to update
   inVel/=inPos.z;
   outVel=inVel;
}
/******************************************************************************/
