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
#ifndef CLAMP
#define CLAMP 0
#endif

#ifndef RANGE
#define RANGE 0
#endif

#ifndef SAMPLES
#define SAMPLES 1
#endif

#define DEPTH_TOLERANCE 0.2 // 20 cm

#define SHOW_BLUR_PIXELS 0 // show what pixels actually get blurred (they will be set to GREEN for fast blur and RED for slow blur) use only for debugging

#if ALPHA
   #define MASK rgba
#else
   #define MASK rgb
#endif
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
    NOPERSP out Vec2 outTex  :TEXCOORD0,
    NOPERSP out Vec2 outPosXY:TEXCOORD1,
    NOPERSP out Vec4 outVtx  :POSITION )
{
   outTex=vtx.tex();
   outPosXY=UVToPosXY(outTex);
   outVtx=vtx.pos4();
}
VecH2 SetVel_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET
{
   Vec pos=GetPosPoint(inTex, inPosXY);
   return GetMotionCameraOnly(pos, inTex);
}
/******************************************************************************/
void Process(inout VecH4 motion, inout VecH2 length2, VecH2 sample_motion) // XY=biggest, ZW=smallest
{
   Half sample_len2=ScreenLength2(sample_motion);
   if(  sample_len2>length2.x){length2.x=sample_len2; motion.xy=sample_motion;} // biggest
   if(  sample_len2<length2.y){length2.y=sample_len2; motion.zw=sample_motion;} // smallest
}
void Process(inout VecH4 max_min_motion, inout VecH2 length2, VecH4 sample_motion, VecH2 pixel_delta, VecH2 uv_to_pixel) // XY=biggest, ZW=smallest
{
 //if(all(Abs(sample_motion.xy)>=Abs(uv_delta))) this check slows down so don't use, also it would need some EPS +eps (ImgSize/RTSize *1 *0.99 *0.5) etc.
   {
      VecH2 pixel_motion=sample_motion.xy*uv_to_pixel; // convert UV motion to pixel motion
    //Half  pixel_motion_len2=Length2(pixel_motion);
      Half  pixel_motion_len=Length(pixel_motion);
    //if(  !pixel_motion_len)return; slows down
      if(   pixel_motion_len)pixel_motion/=pixel_motion_len; // normalize
      VecH2 pixel_motion_p=Perp(pixel_motion); // perpendicular to movement
      Half  pixel_ext=(Abs(pixel_motion.x)+Abs(pixel_motion.y))*1.1; // make smallest movements dilate neighbors, increase extent because later we use linear filtering, so because of blending movement might get minimized, especially visible in cases such as moving object to the right (motion x=1, y=0) row up and down could have velocity at 0 and linear filtering could reduce motion at the object border (1.1 value was the smallest that fixed most problems)
      VecH2 pos_motion=VecH2(Dot(pixel_delta, pixel_motion  ),  // position along            motion vector
                             Dot(pixel_delta, pixel_motion_p)); // position perpendicular to motion vector
    /*if(Abs(pos_motion.x)<pixel_ext+pixel_motion_len
      && Abs(pos_motion.y)<pixel_ext)*/
      if(all(Abs(pos_motion)<VecH2(pixel_ext+pixel_motion_len, pixel_ext)))
      {
         Half sample_len2=ScreenLength2(sample_motion.xy); if(sample_len2>length2.x){length2.x=sample_len2; max_min_motion.xy=sample_motion.xy;} // biggest
              sample_len2=ScreenLength2(sample_motion.zw); if(sample_len2<length2.y){length2.y=sample_len2; max_min_motion.zw=sample_motion.zw;} // smallest
      }
   }
}
/******************************************************************************

   Convert finds the Max and Min Motions (determined by their screen length, screen = UV corrected by aspect ratio)
       Input =         UV Motion
      Output = Max Min UV Motion * MotionScale

/******************************************************************************/
VecH4 Convert_PS(NOPERSP Vec2 inTex:TEXCOORD0):TARGET
{
   // WARNING: code below might still set ZW (smallest) to some very small values, only XY gets forced to 0
   VecH2 length2=VecH2(ScreenLength2(ImgSize.xy)*Sqr(0.5), 2); // x=biggest, y=smallest, initially set biggest to 0 so it always gets updated (actually set to half of pixel to make sure we will ignore small motions and keep 0), initially set smallest to 2 so it always gets updated
   VecH4 motion =0; // XY=biggest, ZW=smallest
#if 0 // process samples individually
   // for RANGE=1 (no scale  ) inTex should remain unmodified              , because it's already at the center of 1x1 texel
   // for RANGE>1 (downsample) inTex should be moved to the center of texel, because it's         at the center of 2x2 texels
   if(RANGE>1)inTex+=ImgSize.xy*0.5;
   const Int ofs=RANGE/2, min=0-ofs, max=RANGE-ofs;
   UNROLL for(Int y=min; y<max; y++)
   UNROLL for(Int x=min; x<max; x++)
      Process(motion, length2, TexPoint(ImgXY, UVClamp(inTex+Vec2(x, y)*ImgSize.xy, CLAMP)).xy);
#else // process samples in 2x2 blocks using linear filtering
   // for RANGE=1 (no scale          ) offset should be 0, because inTex is already at the center of 1x1 texel
   // for RANGE=2 (2x downsample, 2x2) offset should be 0, because inTex is already at the center of 2x2 texels (linear filtering is used)
   // for RANGE=4 (4x downsample, 4x4) offset should be 1, because inTex is already at the center of 4x4 texels, however we want to process (2x2 2x2) so have to position at the center of top left 2x2
   // for RANGE=8 (8x downsample, 8x8) offset should be 3                                                                                   (2x2 2x2)
   const Int ofs=(RANGE-1)/2, min=0-ofs, max=RANGE-ofs; // correctness can be verified with this code: "Int RANGE=1,2,4,8; const Int ofs=(RANGE-1)/2, min=0-ofs, max=RANGE-ofs; Str s; for(Int x=min; x<max; x+=2)s.space()+=x; Exit(s);"
   #if RANGE<=(GL ? 16 : 256) // for GL limit to 16 because compilation is very slow
      UNROLL for(Int y=min; y<max; y+=2)
      UNROLL for(Int x=min; x<max; x+=2)
   #else
      LOOP for(Int y=min; y<max; y+=2)
      LOOP for(Int x=min; x<max; x+=2)
   #endif
         Process(motion, length2, TexLod(ImgXY, UVClamp(inTex+Vec2(x, y)*ImgSize.xy, CLAMP)).xy);
#endif
   motion*=MotionScale_2; // for best precision this should be done for every sample, however doing it here just once, increases performance
   { // limit max length - this prevents stretching objects to distances the blur can't handle anyway, making only certain samples of it visible but not all
      length2*=Sqr(MotionScale_2); // since we've scaled 'motion' above after setting 'length2', then we have to scale 'length2' too
      Half max_length=0.25/2; // #MaxMotionBlurLength
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
VecH4 Dilate_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 motion =TexPoint(Img, inTex);
   VecH2 length2=VecH2(ScreenLength2(motion.xy), ScreenLength2(motion.zw));
#if RANGE<=(GL ? 5 : 7) // only up to 7 is supported here because 'TexPointOfs' accepts offsets in -8..7 range, for GL limit to 5 because compilation is very slow
   UNROLL for(Int y=-RANGE; y<=RANGE; y++)
   UNROLL for(Int x=-RANGE; x<=RANGE; x++)
      if(x || y)Process(motion, length2, TexPointOfs(Img, inTex, VecI2(x, y)), VecH2(x, y), RTSize.zw);
#else
   LOOP for(Int y=-RANGE; y<=RANGE; y++)
   LOOP for(Int x=-RANGE; x<=RANGE; x++)
      if(x || y)Process(motion, length2, TexPoint(Img, inTex+Vec2(x, y)*RTSize.xy), VecH2(x, y), RTSize.zw);
#endif
   return motion;
}
/******************************************************************************/
// BLUR
/******************************************************************************/
Half SampleWeight(Flt base_depth, Flt sample_depth, Half base_uv_motion, Half sample_uv_motion, Half uv_motion_to_step, Half step)
{
   Half   depth_delta =sample_depth-base_depth;
   VecH2  depth_weight=Sat(depth_delta*VecH2(1.0/DEPTH_TOLERANCE, -1.0/DEPTH_TOLERANCE)+0.5); // X=how much base is in front of sample, Y=how much sample is in front of base. 0.5 (middle) is returned if both have the same depth. Here it's always X=1-Y and Y=1-X
   VecH2 motion_weight=Sat(VecH2(base_uv_motion, sample_uv_motion)*uv_motion_to_step-step); // have to convert motion into step (instead of step into motion) to make sure that we reach 1.0 weights, this weight is about checking if one position motion is covering another position (smoothly). X=if base motion is covering sample, Y=if sample motion is covering base
 //depth_weight.x*motion_weight.x = this is needed for cases where base is the moving object     and sample is a static background (base  =object is in front, so depth_weight.x=1, base  =object is moving so motion_weight.x=1), we're returning weight=1 so background sample will be used on the object     position, which will make it appear semi-transparent
 //depth_weight.y*motion_weight.y = this is needed for cases where base is the static background and sample is a moving object     (sample=object is in front, so depth_weight.y=1, sample=object is moving so motion_weight.y=1), we're returning weight=1 so object     sample will be used on the background position, which will draw object on top of background
   return Dot(depth_weight, motion_weight); // return sum of both cases, this will always be 0..1, because even if both base and sample have motion_weight, then depth weight is always X=1-Y
}
Half UVLength(VecH2 motion)
{
 //return Abs(Dot(motion, Normalize(dir.xy)))*MotionScale_2; don't use because it might lower blurring (if object is moving, but camera is rotating faster and in perpendicular way to the object movement, camera blur direction would take priority, then object motion would become 0, and it would become focused and not blurry, so better to keep the wrong blur object direction as long as it's blurry)
   return Length(motion)*MotionScale_2; // here do not use AspectRatio/UVToScreen because we need UV's
}
/******************************************************************************

   Img1  = dilated = downsampled Max Min UV Motion * MotionScale
   ImgXY =              full-res         UV Motion      (unscaled)

/******************************************************************************/
VecH4 Blur_PS(NOPERSP Vec2 uv0:TEXCOORD,
              NOPERSP PIXEL            ):TARGET
{
   VecH4 dilated=TexLod(Img1, uv0); // dilated motion (XY=biggest, ZW=smallest), use linear filtering because 'Img1' may be smaller
   VecH4 color; color.MASK=TexLod(Img, uv0).MASK; // can't use 'TexPoint' because 'Img' can be supersampled
#if !ALPHA
   color.a=1; // force full alpha so back buffer effects can work ok
#endif

   BRANCH if(any(dilated.xy)) // XY=biggest, can use 'any' because small motions were already forced to 0 in 'Convert'
   {
      Vec4 dir=Vec4(dilated.xy, -dilated.xy);
      if(CLAMP)
      {
         dir.xy=UVClamp(uv0+dir.xy)-uv0; // first calculate target and clamp it
         dir.zw=UVClamp(uv0+dir.zw)-uv0; // first calculate target and clamp it
      }

      Int steps=SAMPLES;
      dir/=steps;
      Vec2 uv1=uv0;

      BRANCH if(Length2(dilated.zw)>Length2(dilated.xy)*Sqr(0.64)) // if smallest motion is close to biggest motion then just do a fast and simple blur, ignoring depths and precise motions
      {
      #if ALPHA
         Vec4 color_hp=color.MASK; // use HP because we operate on many samples
      #else
         Vec  color_hp=color.MASK; // use HP because we operate on many samples
      #endif

         UNROLL for(Int i=1; i<=steps; i++) // start from 1 because we've already got #0 before
         {
            color_hp.MASK+=TexLod(Img, uv0+=dir.xy).MASK  // use linear filtering
                          +TexLod(Img, uv1+=dir.zw).MASK; // use linear filtering
         }
         color.MASK=color_hp.MASK/(steps*2+1); // already have 1 sample

      #if SHOW_BLUR_PIXELS
         color.g=1;
      #endif
      }else
      {
      #if ALPHA
         Vec4  color_hp=0; // use HP because we operate on many samples
      #else
         Vec   color_hp=0; // use HP because we operate on many samples
      #endif
         Flt   weight  =0; // use HP because we operate on many samples
         VecH2 base_motion=TexPoint(ImgXY, uv0).xy; Half base_uv_motion=UVLength(base_motion);
         Flt   base_depth =TexDepthPoint(uv0);
         Half  uv_motion_to_step0=1/Length(dir.xy); // allows to convert travelled UV distance into how many steps (travelled_uv*uv_motion_to_step0=step)
         Half  uv_motion_to_step1=1/Length(dir.zw);
       //return SRGBToLinear(VecH4(motion.xy*0.5+0.5,0.5,1));
       //return SRGBToLinear(VecH4((motion.xy-0.2)*20*0.5+0.5,0.5,1));
       //return SRGBToLinear(VecH4((dilated.xy-0.2)*20*0.5+0.5,0.5,1));
       //return SRGBToLinear(VecH4((Length(dilated.xy).xx-0.2)*20*0.5+0.5,0.5,1));
       //return SRGBToLinear(VecH4((Length(dilated.xy)-0.2)*20*0.5+0.5,(Length(motion.xy)-0.2)*20*0.5+0.5,0.5,1));
       //return SRGBToLinear(VecH4((dilated.x-0.2)*20*0.5+0.5, (motion.x-0.2)*20*0.5+0.5, motion.x>dilated.x, 1));
       //return SRGBToLinear(VecH4(base_depth.xxx,1));
       //return SRGBToLinear(VecH4((dir.xy*20)*0.5+0.5,0,1));
       //return SRGBToLinear(VecH4(motion.xxx>dilated.xxx,1));
       //return SRGBToLinear(VecH4(((motion.xy-dilated.xy)*200),0,1));
         UNROLL for(Int i=1; i<=steps; i++) // start from 1 because #0 is processed at the end
         {
            uv0+=dir.xy;
            uv1+=dir.zw;
            VecH2 sample0_motion=TexLod(ImgXY, uv0).xy; Flt sample0_depth=TexDepthLinear(uv0); // TODO: DepthPoint?
            VecH2 sample1_motion=TexLod(ImgXY, uv1).xy; Flt sample1_depth=TexDepthLinear(uv1);

            Half sample0_uv_motion=UVLength(sample0_motion);
            Half sample1_uv_motion=UVLength(sample1_motion);

            Half step=Max(0, i-1.5); // use -1.5 instead of -1 because on a 3D ball moving right, pixels in the center have higher movement due to perspective correction (pixels closer to camera move faster than those far), so when calculating biggest movement from neighbors, then the pixels at the border will get biggest/dilated movement (coming from ball center) that's slightly bigger than border movement. So the search vector that's set from biggest/dilated motion will be bigger than the sample movement, and for example its motion might cover only 9/10 steps instead of 10/10. To workaround this, make step offset slightly smaller.
            
            Half w0=SampleWeight(base_depth, sample0_depth, base_uv_motion, sample0_uv_motion, uv_motion_to_step0, step);
            Half w1=SampleWeight(base_depth, sample1_depth, base_uv_motion, sample1_uv_motion, uv_motion_to_step1, step);

            // this blurs background to make it look more like simple/fast blur
            if(0) // don't use
            {
               bool2 state=bool2(sample1_depth<sample0_depth, sample1_uv_motion>sample0_uv_motion); // X=if sample1 is in front of sample0, Y=if sample1 has bigger motion (moving faster) than sample0
			      if( all(state))w0=w1; // S1 in front of S0 and moving faster
			      if(!any(state))w1=w0; // S0 in front of S1 and moving faster
            }

            color_hp.MASK+=w0*TexLod(Img, uv0).MASK  // use linear filtering
                          +w1*TexLod(Img, uv1).MASK; // use linear filtering
            weight+=w0+w1;
         }
         color_hp.MASK*=1.0/(steps*2+1); // in every step we have 2 samples + 1 to make room for base sample
         weight       *=1.0/(steps*2+1); // in every step we have 2 samples + 1 to make room for base sample
         color.MASK=color_hp.MASK + (1-weight)*color.MASK;

      #if SHOW_BLUR_PIXELS
         color.r=1;
      #endif
      }

   #if 0 // test how many samples were used for blurring
      color.rgb=steps/Half(SAMPLES);
   #endif

   #if DITHER
      ApplyDither(color.rgb, pixel.xy);
   #endif
   }
   return color;
}
/******************************************************************************
void Explosion_VS(VtxInput vtx,
              out Vec  outPos:TEXCOORD0,
              out Vec  outVel:TEXCOORD1,
              out Vec4 outVtx:POSITION )
{
   outVel=TransformTP (Normalize(vtx.pos())*Step, (Matrix3)CamMatrix);
   outPos=TransformPos(vtx.pos());
   outVtx=Project     ( outPos  );
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
