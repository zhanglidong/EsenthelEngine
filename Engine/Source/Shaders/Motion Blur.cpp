/******************************************************************************/
#include "!Header.h"
#ifndef CLAMP
#define CLAMP 0
#endif
#ifndef DIAGONAL
#define DIAGONAL 0
#endif
#ifndef RANGE
#define RANGE 0
#endif
/******************************************************************************

   TexCoord.x=0 -> Left, TexCoord.x=1 -> Right
   TexCoord.y=0 -> Up  , TexCoord.y=1 -> Down

   Warning: because max_dir_length is encoded in Signed RT Z channel without MAD range adjustment, it loses 1 bit precision

/******************************************************************************/
#define      MAX_BLUR_SAMPLES 7
#define VARIABLE_BLUR_SAMPLES 0 // 0=is much faster for all 3 cases (no/small/full blur), default=0
#define     TEST_BLUR_PIXELS  0 // test what pixels actually get blurred (they will be set to RED color) use only for debugging
#define ALWAYS_DILATE_LENGTH  0 // 0=gave better results (when camera was rotated slightly), default=0
#define                ROUND  1 // if apply rounding (makes smaller res buffers look more similar to full res), default=1
#define TEST_DEPTH            0
/******************************************************************************/
BUFFER(MotionBlur)
   Vec4 MotionUVMulAdd;
   Vec4 MotionVelScaleLimit;
   Vec2 MotionPixelSize;
BUFFER_END
/******************************************************************************/
#define DEPTH_TOLERANCE 0.1 // 10 cm
inline Half DepthBlend(Flt z_test, Flt z_base)
{
   return Sat((z_base-z_test)/DEPTH_TOLERANCE+1); // we can apply overlap only if tested depth is smaller
}
inline Bool InFront(Flt z_test, Flt z_base)
{
   return z_test<=z_base+DEPTH_TOLERANCE; // if 'z_test' is in front of 'z_base' with DEPTH_TOLERANCE
}
/******************************************************************************/
void Explosion_VS(VtxInput vtx,
              out Vec  outPos:TEXCOORD0,
              out Vec  outVel:TEXCOORD1,
              out Vec4 outVtx:POSITION )
{
   outVel=TransformTP(Normalize(vtx.pos())*Step, (Matrix3)CamMatrix);
   outPos=TransformPos(vtx.pos());
   outVtx=Project     ( outPos  );
}
void Explosion_PS(Vec    inPos:TEXCOORD0,
                  Vec    inVel:TEXCOORD1,
              out VecH4 outVel:TARGET1  ) // #BlendRT
{
   // there's no NaN because inPos.z is always >0 in PixelShader
   inVel/=inPos.z;
#if !SIGNED_VEL_RT
   inVel=inVel*0.5+0.5;
#endif
   outVel.xyz=inVel.xyz; outVel.w=0;
}
/******************************************************************************/
void ClearSkyVel_VS(VtxInput vtx,
        NOPERSP out VecH4 outVel:TEXCOORD,
        NOPERSP out Vec4  outVtx:POSITION)
{
   Vec pos=Vec(ScreenToPosXY(vtx.tex()), 1); // we shouldn't normalize this vector, instead, we should keep it at Z=1 so we don't have to divide by Z later

   outVel.xyz=Cross(pos, CamAngVel);
#if !SIGNED_VEL_RT
   outVel.xyz=outVel.xyz*0.5+0.5;
#endif

   outVel.w=0;
   outVtx  =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
VecH4 ClearSkyVel_PS(NOPERSP VecH4 inVel:TEXCOORD):TARGET {return inVel;} // yes, per-vertex precision is enough, as it generates the same results as if drawing a half sky ball mesh (results with the half ball mesh were the same as the one from this pixel shader)
/******************************************************************************/
void Convert_VS(VtxInput vtx,
    NOPERSP out Vec2 outTex  :TEXCOORD0,
    NOPERSP out Vec2 outPos  :TEXCOORD1, // position relative to viewport center scaled from UV to ScreenPos
         #if MODE==0
    NOPERSP out Vec2 outPosXY:TEXCOORD2,
         #endif
    NOPERSP out Vec4 outVtx  :POSITION )
{
   outTex=vtx.tex();
   outPos=outTex*MotionUVMulAdd.xy+MotionUVMulAdd.zw;
#if MODE==0
   outPosXY=ScreenToPosXY(outTex);
#endif
   outVtx=vtx.pos4();
}
VecH4 Convert_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPos  :TEXCOORD1 // position relative to viewport center scaled from UV to ScreenPos
              #if MODE==0
               , NOPERSP Vec2 inPosXY:TEXCOORD2
              #endif
                ):TARGET
{
   Vec blur;
#if MODE==0
   Vec pos=(CLAMP ? GetPosLinear(UVClamp(inTex, CLAMP)) : GetPosLinear(inTex, inPosXY));
   blur=GetVelocitiesCameraOnly(pos);
#else
      blur=TexLod(Img, UVClamp(inTex, CLAMP)).xyz; // have to use linear filtering because we may draw to smaller RT
   #if !SIGNED_VEL_RT // convert 0..1 -> -1..1 (*2-1) and fix zero, unsigned texture formats don't have a precise zero when converted to signed, because both 127/255*2-1 and 128/255*2-1 aren't zeros, 127: (127/255-0.5)==-0.5/255, 128: (128/255-0.5)==0.5/255, 129: (129/255-0.5)==1.5/255, so let's compare <=1.0/255
      blur=((Abs(blur-0.5)<=1.0/255) ? Vec(0, 0, 0) : blur*2-1); // this performs comparisons for all channels separately, force 0 when source value is close to 0.5, otherwise scale to -1..1
   #endif
#endif

   // see "C:\Users\Greg\SkyDrive\Code\Tests\Motion Blur.cpp"
   // the following generates 2 delta vectors in screen space, they have different lengths (when blur.z!=0) due to perspective correction, one points towards center, and one away from center
   blur*=MotionVelScaleLimit.xyz;
   Vec4 delta=Vec4((inPos+blur.xy)/(1+blur.z)-inPos,  // #1 is along   blur vector (delta.xy)
                   (inPos-blur.xy)/(1-blur.z)-inPos); // #2 is against blur vector (delta.zw)

   // store only one direction and its length
   // delta.xy.setLength(Avg(delta.xy.length(), delta.zw.length()))
   Flt len0=Length(delta.xy), len1=Length(delta.zw), len=Min(Avg(len0, len1), MotionVelScaleLimit.w); // alternatively we could use "Max" instead of "Avg"
   if( len0)delta.xy*=(len + ROUND*0.5/MAX_MOTION_BLUR_PIXEL_RANGE)/len0; // NaN, add half pixel length which will trigger rounding effect
            delta.z  = len; // here don't include rounding

#if !SIGNED_VEL_RT
   delta.xy=delta.xy*0.5+0.5; // scale XY -1..1 -> 0..1, but leave Z in 0..1
#endif
   return delta;
}
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
VecH4 Dilate_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   const Int range=1;
   VecH4 blur;
   blur.xyz=TexPoint(Img, inTex).xyz;
   blur.w=0;
#if !SIGNED_VEL_RT
   blur.xy=blur.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
#endif
   Half len=Length(blur.xy); // can't use 'blur.z' as length here, because it's max length of all nearby pixels and not just this pixel
   Flt  z_base; if(TEST_DEPTH)z_base=TexDepthLinear(inTex); // use linear filtering because RT can be smaller
   // this function iterates nearby pixels and calculates how much the should blur over this one (including angle between pixels, distances, and their blur velocities)
   UNROLL for(Int y=-range; y<=range; y++)
   UNROLL for(Int x=-range; x<=range; x++)if(x || y)
   {
      Vec2 t=inTex+Vec2(x, y)*RTSize.xy;
      VecH b=TexPoint(Img, t).xyz;
   #if !SIGNED_VEL_RT
      b.xy=b.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
   #endif

   #if 1 // version that multiplies length by cos between pixels (faster and more accurate for diagonal blurs, however bloats slightly)
      Half l=Abs(Dot(b.xy, Normalize(VecH2(x, y)))); // we want this to be proportional to 'b.xy' length
      if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
      l-=DistH(x, y)/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
      if(l>len){blur.xy=b.xy*(l/Length(b.xy)); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;}
   #else // version that does line intersection tests (slower)
      Half b_len=Length(b.xy), // can't use 'b.z' as length here, because it's max length of all nearby pixels and not just this pixel
           l=b_len; if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
      l-=DistH(x, y)/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
      if(l>len)
      {
         const Half eps=0.7; // 1.0 would include neighbors too, and with each call to this function we would bloat by 1 pixel, this needs to be slightly below SQRT2_2 0.7071067811865475 to avoid diagonal bloating too
         Half line_dist=Dot(b.xy, VecH2(y, -x)); if(Abs(line_dist)<=b_len*eps) // VecH2 perp_n=VecH2(b.y, -b.x)/b_len; Flt line_dist=Dot(perp_n, VecH2(x, y)); if(Abs(line_dist)<=eps)..   (don't try to do "VecH2(y, -x)/eps" instead of "b_len*eps", because compiler can optimize the Dot better without it)
         {blur.xy=b.xy*(l/b_len); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;}
      }
   #endif

      if(ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z);
   }

#if !SIGNED_VEL_RT
   blur.xy=blur.xy*0.5+0.5; // scale XY -1..1 -> 0..1, but leave Z in 0..1
#endif
   return blur;
}
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
VecH4 DilateX_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 blur=TexPoint(Img, inTex); // XY=Dir, Z=Max Dir length of all nearby pixels
#if !SIGNED_VEL_RT
   blur.xy=blur.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
#endif
   Half len=Length(blur.xy); // can't use 'blur.z' as length here, because it's max length of all nearby pixels and not just this pixel
   Flt  z_base; if(TEST_DEPTH)z_base=TexDepthLinear(inTex); // use linear filtering because RT can be smaller
   Vec2 t; t.y=inTex.y;

   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      t.x=inTex.x+RTSize.x*i;
      VecH b=TexPoint(Img, t).xyz;
   #if !SIGNED_VEL_RT
      b.xy=b.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
   #endif

      Half ll=Abs(b.x), l=ll;
      if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
      l-=Abs(Half(i))/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
      if(l>len){blur.xy=b.xy*(l/(DIAGONAL ? Length(b.xy) : ll)); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;} // when not doing diagonal then use 'll' to artificially boost intensity, because it helps in making it look more like "Dilate"

      if(ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z);
   }

   if(DIAGONAL)
   {
      Int range=Round(RANGE*SQRT2_2);
      UNROLL for(Int i=-range; i<=range; i++)if(i)
      {
         t=inTex+RTSize.xy*i;
         VecH b=TexPoint(Img, t).xyz;
      #if !SIGNED_VEL_RT
         b.xy=b.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
      #endif

         Half l=Abs(Dot(b.xy, VecH2(SQRT2_2, SQRT2_2)));
         if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
         l-=Abs(Half(i*SQRT2))/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
         if(l>len){blur.xy=b.xy*(l/Length(b.xy)); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;}

         if(ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z);
      }
   }

#if !SIGNED_VEL_RT
   blur.xy=blur.xy*0.5+0.5; // scale XY -1..1 -> 0..1, but leave Z in 0..1
#endif
   return blur;
}
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
VecH4 DilateY_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4 blur=TexPoint(Img, inTex); // XY=Dir, Z=Max Dir length of all nearby pixels
#if !SIGNED_VEL_RT
   blur.xy=blur.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
#endif
   Half len=Length(blur.xy); // can't use 'blur.z' as length here, because it's max length of all nearby pixels and not just this pixel
   Flt  z_base; if(TEST_DEPTH)z_base=TexDepthLinear(inTex); // use linear filtering because RT can be smaller
   Vec2 t; t.x=inTex.x;

   UNROLL for(Int i=-RANGE; i<=RANGE; i++)if(i)
   {
      t.y=inTex.y+RTSize.y*i;
      VecH b=TexPoint(Img, t).xyz;
   #if !SIGNED_VEL_RT
      b.xy=b.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
   #endif

      Half ll=Abs(b.y), l=ll;
      if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
      l-=Abs(Half(i))/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
      if(l>len){blur.xy=b.xy*(l/(DIAGONAL ? Length(b.xy) : ll)); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;} // when not doing diagonal then use 'll' to artificially boost intensity, because it helps in making it look more like "Dilate"

      if(ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z);
   }

   if(DIAGONAL)
   {
      Int range=Round(RANGE*SQRT2_2);
      UNROLL for(Int i=-range; i<=range; i++)if(i)
      {
         t=inTex+RTSize.xy*Vec2(i, -i);
         VecH b=TexPoint(Img, t).xyz;
      #if !SIGNED_VEL_RT
         b.xy=b.xy*2-1; // scale XY 0..1 -> -1..1, but leave Z in 0..1
      #endif

         Half l=Abs(Dot(b.xy, VecH2(SQRT2_2, -SQRT2_2)));
         if(TEST_DEPTH)l*=DepthBlend(TexDepthLinear(t), z_base); // use linear filtering because RT can be smaller
         l-=Abs(Half(i*SQRT2))/MAX_MOTION_BLUR_PIXEL_RANGE; // when loop is unrolled and MAX_MOTION_BLUR_PIXEL_RANGE is uniform then it can be evaluated to a constant expression
         if(l>len){blur.xy=b.xy*(l/Length(b.xy)); if(!ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z); len=l;}

         if(ALWAYS_DILATE_LENGTH)blur.z=Max(blur.z, b.z);
      }
   }

#if !SIGNED_VEL_RT
   blur.xy=blur.xy*0.5+0.5; // scale XY -1..1 -> 0..1, but leave Z in 0..1
#endif
   return blur;
}
/******************************************************************************/
VecH4 SetDirs_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET // goes simultaneously in both ways from starting point and notices how far it can go, travelled distance is put into texture
{
   // Input: ImgXY - pixel   velocity
   //        Img   - dilated velocity (main blur direction)

   VecH blur_dir=TexPoint(Img, inTex).xyz; // XY=Dir, Z=Max Dir length of all nearby pixels
#if !SIGNED_VEL_RT
   blur_dir.xy=((Abs(blur_dir.xy-0.5)<=1.0/255) ? VecH2(0, 0) : blur_dir.xy*2-1); // this performs comparisons for all channels separately, force 0 when source value is close to 0.5, otherwise scale to -1..1
#endif

   VecH4 dirs=0;

#if 0 && !ALWAYS_DILATE_LENGTH // when we don't always dilate the length, then we could check it here instead of 'blur_dir.xy' length, performance results were similar, at the moment it's not sure which version is better
   BRANCH if(blur_dir.z>0)
   {
        Half blur_dir_length2=Length2(blur_dir.xy);
#else
        Half blur_dir_length2=Length2(blur_dir.xy);
   BRANCH if(blur_dir_length2>Sqr(Half(0.5/MAX_MOTION_BLUR_PIXEL_RANGE))) // Length(blur_dir.xy)*MAX_MOTION_BLUR_PIXEL_RANGE>0.5. Check 'blur_dir.xy' instead of 'blur_dir.z' because can be big (because of nearby pixels moving) but 'blur_dir.xy' can be zero. Always check for 0.5 regardless of ROUND (this was tested and it looks much better)
   {
#endif
      /*
         This algorithm works by going in 2 directions, starting from the center, directions are opposite:
            left <--- center ---> right
         Left and Right are just examples, because directions can go in any way (up/down, diagonal, etc)
         This function needs to calculate the length of those directions - how much we want to travel in each direction.
         Those vectors will be used in the final blur, inside it, all pixels along the way will be included.
         So we need to choose the lengths carefully, to avoid blurring with non-moving objects that are in front of us.
         At the start we take the movement velocity of the starting center pixel, to see how much it should blur.
         Along the way we test if there are other moving pixels, and if they overlap with the starting center position,
         if so, we extend blur length up to that pixel, but not further. We also check depth values of encountered pixels,
         and calculate the minimum of them in each direction, to know if they will form an obstacle, that will block next pixels.
         We also allow a special case when all pixels move in the same direction (this happens when rotating the camera).
         In this loop we can't break, because even if we encounter some obstacles blocking pixels, we have to keep going,
         because we may find a pixel later, that is not blocked by any obstacles and overlaps the center.
      */

      // calculate both directions
      Vec4 tex_step;
      tex_step.xy= blur_dir.xy*MotionPixelSize;
      tex_step.zw=-blur_dir.xy*MotionPixelSize;
   #if 0 // no need to do clamping here because we do it anyway below for the final dirs
      if(CLAMP)
      {
         tex_step.xy=UVClamp(inTex+tex_step.xy, CLAMP)-inTex; // first calculate target and clamp it
         tex_step.zw=UVClamp(inTex+tex_step.zw, CLAMP)-inTex; // first calculate target and clamp it
      }
   #endif
      Half  blur_dir_length=Sqrt(blur_dir_length2);
      VecH2 blur_dir_normalized=blur_dir.xy/blur_dir_length;
      Flt   pixel_range=blur_dir_length*MAX_MOTION_BLUR_PIXEL_RANGE; tex_step/=pixel_range; // 'tex_step' is now 1 pixel length (we need this length, because we compare blur lengths/distances to "Int i" step)
      Int   range0=0, range1=0;

      VecH2 pixel_vel=TexPoint(ImgXY, inTex).xy;
   #if !SIGNED_VEL_RT
      pixel_vel=pixel_vel*2-1;
   #endif
      Half length0=Abs(Dot(pixel_vel, blur_dir_normalized))*MAX_MOTION_BLUR_PIXEL_RANGE, length1=length0; // how many pixels in left and right this pixel wants to move starting from the center. We want this to be proportional to 'pixel_vel' length, so don't normalize. This formula gives us how much this pixel wants to travel along the main blur direction
      Flt  depth0=TexDepthLinear(inTex), depth1=depth0, // depth values of left and right pixels with most movement, use linear filtering because RT can be smaller
           depth_min0=depth0, depth_min1=depth1; // minimum of encountered left and right depth values
      Int  same_vel0=true, same_vel1=true; // if all encountered so far in this direction have the same velocity (this is to support cases where pixels move in the same direction but have diffent positions, for example when rotating the camera)
      Int  allowed0=0, allowed1=0;

      Vec2 t0=inTex, t1=inTex;
   #if 0 // slower
      Int samples=MAX_MOTION_BLUR_PIXEL_RANGE; UNROLL for(Int i=1; i<=samples; i++)
   #else
      Int samples=Round(blur_dir.z*MAX_MOTION_BLUR_PIXEL_RANGE); LOOP for(Int i=1; i<=samples; i++)
   #endif
      {
         t0+=tex_step.xy;
         t1+=tex_step.zw;
         VecH2 v0=TexLod(ImgXY, t0).xy, v1=TexLod(ImgXY, t1).xy; // use linear filtering because texcoords are not rounded
      #if !SIGNED_VEL_RT
         v0=v0*2-1;
         v1=v1*2-1;
      #endif
         Flt z0=TexDepthLinear(t0), z1=TexDepthLinear(t1); // use linear filtering because RT can be smaller and because texcoords are not rounded

      #if 0
         Half l0=Length(v0)*MAX_MOTION_BLUR_PIXEL_RANGE, l1=Length(v1)*MAX_MOTION_BLUR_PIXEL_RANGE;
      #else
         Half l0=Abs(Dot(v0, blur_dir_normalized))*MAX_MOTION_BLUR_PIXEL_RANGE, l1=Abs(Dot(v1, blur_dir_normalized))*MAX_MOTION_BLUR_PIXEL_RANGE;
      #endif

       //if(InFront(z0, depth_min0)) // if this sample is in front of all encountered so far in this direction, this check isn't needed because we do depth tests either way below
            if(l0>=i // this sample movement reaches the center
            && i>length0) // and it extends blurring to the left, compared to what we already have
         {
            length0= i; // extend possible blurring only up to this sample, but not any further
             depth0=z0;
         }

       //if(InFront(z1, depth_min1)) // if this sample is in front of all encountered so far in this direction, this check isn't needed because we do depth tests either way below
            if(l1>=i // this sample movement reaches the center
            && i>length1) // and it extends blurring to the right, compared to what we already have
         {
            length1= i; // extend possible blurring only up to this sample, but not any further
             depth1=z1;
         }

         depth_min0=Min(depth_min0, z0);
         depth_min1=Min(depth_min1, z1);

         // TODO: can this be improved?
         same_vel0*=(Dist2(pixel_vel, v0)<=Sqr(Half(1.5/MAX_MOTION_BLUR_PIXEL_RANGE)));
         same_vel1*=(Dist2(pixel_vel, v1)<=Sqr(Half(1.5/MAX_MOTION_BLUR_PIXEL_RANGE)));

         Bool allow0=(InFront(depth0, depth_min0) || same_vel0),
              allow1=(InFront(depth1, depth_min1) || same_vel1);

         allowed0+=allow0;
         allowed1+=allow1;

         if(length0>=i && allow0)range0=i;
         if(length1>=i && allow1)range1=i;
      }

      Half dir_length0=range0/Half(MAX_MOTION_BLUR_PIXEL_RANGE),
           dir_length1=range1/Half(MAX_MOTION_BLUR_PIXEL_RANGE);

   #if 1
      // normally with the formula above, we can get only integer precision, 'range0' and 'range1' can only be set to integer steps, based on which we set vectors
      // we get multiples of pixel ranges, without fraction, this gets much worse with smaller resolutions, in which steps are bigger
      // to solve this problem, if there were no obstacles found, then maximize directions by original smooth value
      Half actual_length=blur_dir_length - Half(ROUND*0.5/MAX_MOTION_BLUR_PIXEL_RANGE), // we need to subtract what we've added before, no need to do Sat, because we're inside BRANCH if that tests for length
             test_length=blur_dir_length +      1.0/128                               ; // we have to use a bigger value than 'actual_length' and 'blur_dir_length' to improve chances of this going through (the epsilon was tested carefully it supports both small and big velocities)
      if(allowed0==samples && (range1>0 || samples<=1) && test_length>=dir_length0)dir_length0=actual_length; // if there were no obstacles in this direction (also check opposite direction because of linear filtering using neighbor sample which causes visible leaking, we check that we've blurred at least one pixel "range>0", however if the velocity is <1 less than 1 pixel then range will not get >0, to support very small smooth velocities, we have to do another check for "samples <= 1"), then try using original
      if(allowed1==samples && (range0>0 || samples<=1) && test_length>=dir_length1)dir_length1=actual_length; // if there were no obstacles in this direction (also check opposite direction because of linear filtering using neighbor sample which causes visible leaking, we check that we've blurred at least one pixel "range>0", however if the velocity is <1 less than 1 pixel then range will not get >0, to support very small smooth velocities, we have to do another check for "samples <= 1"), then try using original
   #endif

      dirs.xy= blur_dir_normalized.xy*dir_length0;
      dirs.zw=-blur_dir_normalized.xy*dir_length1;
      if(CLAMP) // clamp final dirs
      {
         dirs.xy=(UVClamp(inTex+dirs.xy*MotionPixelSize, CLAMP)-inTex)/MotionPixelSize; // first calculate target and clamp it
         dirs.zw=(UVClamp(inTex+dirs.zw*MotionPixelSize, CLAMP)-inTex)/MotionPixelSize; // first calculate target and clamp it
      }
   }

#if !SIGNED_VEL_RT
   dirs=dirs*0.5+0.5; // scale -1..1 -> 0..1
#endif
   return dirs;
}
/******************************************************************************/
VecH4 Blur_PS(NOPERSP Vec2 inTex:TEXCOORD,
              NOPERSP PIXEL              ):TARGET // no need to do clamp because we've already done that in 'SetDirs'
{
   // Input: Img  - color
   //        Img1 - 2 blur ranges (XY, ZW)

   VecH4 blur=TexLod(Img1, inTex); // use linear filtering because 'Img1' may be smaller
#if !SIGNED_VEL_RT
   blur=((Abs(blur-0.5)<=1.0/255) ? VecH4(0, 0, 0, 0) : blur*2-1); // this performs comparisons for all channels separately, force 0 when source value is close to 0.5, otherwise scale to -1..1
#endif

   VecH4 color=VecH4(TexLod(Img, inTex).rgb, 1); // force full alpha so back buffer effects can work ok, can't use 'TexPoint' because 'Img' can be supersampled

   BRANCH if(any(blur)) // we can use 'any' here because small values got clipped out already in 'SetDirs'
   {
      Vec2 dir0=blur.xy*MotionPixelSize,
           dir1=blur.zw*MotionPixelSize;
      if(CLAMP)
      {
         dir0=UVClamp(inTex+dir0, CLAMP)-inTex; // first calculate target and clamp it
         dir1=UVClamp(inTex+dir1, CLAMP)-inTex; // first calculate target and clamp it
      }
      Int samples=MAX_BLUR_SAMPLES;
   #if VARIABLE_BLUR_SAMPLES
      Flt eps=0.15;
      #if 1
         samples=Ceil(Sat(Max(Abs(blur))/MotionVelScaleLimit.w+eps)*samples);
      #else
         samples=Ceil(Sat(Sqrt(Max(Length2(blur.xy), Length2(blur.zw)))/MotionVelScaleLimit.w+eps)*samples); // have to calculate max of 2 lengths, because one can be clipped due to an obstacle
      #endif
   #endif

      Vec2 t1=inTex;
      dir0/=samples;
      dir1/=samples;

      Vec color_hp=color.rgb; // use HP because we operate on many samples
   #if VARIABLE_BLUR_SAMPLES
      LOOP
   #else
      UNROLL
   #endif
         for(Int i=1; i<=samples; i++) // start from 1 because we've already got #0 before
      {
         // TODO: implement new high quality mode that doesn't use 'SetDirs' but calculates per-sample weights based on velocity and depth (for this have to use 'CLAMP')
         color_hp.rgb+=TexLod(Img, inTex+=dir0).rgb; // use linear filtering
         color_hp.rgb+=TexLod(Img,   t1 +=dir1).rgb; // use linear filtering
      }
      color.rgb=color_hp.rgb/(samples*2+1);

   #if TEST_BLUR_PIXELS
      color.r=1;
   #endif
   #if 0 // test how many samples were used for blurring
      color.rgb=samples/16.0;
   #endif
   }

#if DITHER
   ApplyDither(color.rgb, pixel.xy);
#endif
   return color;
}
/******************************************************************************/
