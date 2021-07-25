/******************************************************************************/
#include "!Header.h"
/******************************************************************************
VIEW_FULL, ALPHA, DUAL_HISTORY, GATHER, FILTER_MIN_MAX

Img=Cur (ImgSize), Img1=Old (RTSize), ImgXY=CurVel (ImgSize), Depth (ImgSize)

TAA_OLD_VEL=1
   ImgXY1=OldVel (ImgSize)

DUAL_HISTORY=1
   Img2=Old1 (RTSize)

ALPHA=0
   ImgXY2=Weight, Flicker
ALPHA=1
   ImgX=CurAlpha (ImgSize)
      MERGED_ALPHA=1
         Img3=Alpha, Weight, Flicker
      SEPARATE_ALPHA=1
         ImgXY2=Weight, Flicker
         ImgX1=Old Alpha


   Following code is used for flicker detection (assuming at least 2 big changes are needed in a cycle)
   Flt old=0, flicker=0;
   Int cycle=8;
   const Flt add_step=1.0f/(cycle*2-2);
   const Flt mul_step=1.0f/cycle;
   FREP(1000)
   {
      Int jitter=i%cycle;
      Flt cur=(jitter==0);
      //Flt cur=(jitter==1);
      //Flt cur=(jitter<cycle/2);
      //Flt cur=(i==0);
      Flt difference=(old!=cur);
      if(0) // this is correct however it produces sharp results which may produce artifacts on the screen (sharp transitions/artifacts)
      {
         if( difference)flicker+=1.0f/2;
         else           flicker-=add_step;
         Bool is_flicker=(flicker>0.5f+add_step/2);
         Flt  is_flicker_f=LerpR(0.5f, 0.5f+add_step, flicker);
      }else // instead use smooth version
      {
         flicker=Lerp(flicker, difference, mul_step);
         Flt is_flicker_f=LerpR(mul_step, Lerp(mul_step, 1, mul_step), flicker);
      }
      flicker=Sat(flicker);
      old=cur;
   }
/******************************************************************************/
#define SEPARATE_ALPHA ( TAA_SEPARATE_ALPHA && ALPHA)
#define   MERGED_ALPHA (!TAA_SEPARATE_ALPHA && ALPHA)

#define CYCLE 8
#define CUR_WEIGHT   (1.0/ CYCLE)
#define FLICKER_STEP (1.0/(CYCLE*2-2))

#define FLICKER_WEIGHT 0 // if flicker should be affected by weight (disable because if enabled then a lot of pixels at the screen edge get marked as flickering when rotating the camera)
#define FLICKER_EPS    0.2 // this is value for color distance ~0..1 in sRGB gamma (lower numbers increase detection of flicker, higher numbers decrease detection of flicker), 0.2 was the smallest value that didn't cause noticable artifacts/blurriness on a particle fire effect

#define YCOCG 0 // disable because didn't improve quality much and it also prevents FILTER_MIN_MAX
#if     YCOCG
   #define FILTER_MIN_MAX 0 // can't use FILTER_MIN_MAX with YCOCG
#endif

#define NEAREST_DEPTH_VEL 1

#define VEL_EPS 0.006

#define CUBIC 1
#if     CUBIC
   #include "Cubic.h"
#endif

#if ALPHA
   #undef DUAL_HISTORY // #TAADual
#endif

#define MERGE_CUBIC_MIN_MAX 0 // Actually disable since it causes ghosting (visible when rotating camera around a character in the dungeons, perhaps range for MIN MAX can't be big), enable since this version is slightly better because: uses 12 tex reads (4*4 -4 corners), uses 12 samples for MIN/MAX which reduces flickering a bit, however has a lot more arithmetic calculations because of min/max x12 and each sample color is multiplied by weight separately

#define MERGE_ADJUST_OLD 1 // 1=faster

#define DUAL_ADJUST_OLD 1
/******************************************************************************/
VecH RGBToYCoCg(VecH col)
{
   return VecH(Dot(col, VecH( 0.25, 0.50,  0.25)),
               Dot(col, VecH( 0.50, 0.00, -0.50)),
               Dot(col, VecH(-0.25, 0.50, -0.25)));
}
VecH4 RGBToYCoCg(VecH4 col)
{
   return VecH4(Dot(col.rgb, VecH( 0.25, 0.50,  0.25)),
                Dot(col.rgb, VecH( 0.50, 0.00, -0.50)),
                Dot(col.rgb, VecH(-0.25, 0.50, -0.25)),
                    col.a                            );
}
VecH RGBToYCoCg4(VecH col) // faster but scales RGB *4
{
#if 0
   return VecH(Dot(col, VecH( 1, 2,  1)),
               Dot(col, VecH( 2, 0, -2)),
               Dot(col, VecH(-1, 2, -1)));
#else
   return VecH(col.r   + col.g*2 + col.b  ,
               col.r*2           - col.b*2,
              -col.r   + col.g*2 - col.b );
#endif
}
VecH4 RGBToYCoCg4(VecH4 col) // faster but scales RGB *4
{
#if 0
   return VecH4(Dot(col.rgb, VecH( 1, 2,  1)),
                Dot(col.rgb, VecH( 2, 0, -2)),
                Dot(col.rgb, VecH(-1, 2, -1)),
                    col.a);
#else
   return VecH4(col.r   + col.g*2 + col.b  ,
                col.r*2           - col.b*2,
               -col.r   + col.g*2 - col.b  ,
                col.a                     );
#endif
}
/******************************************************************************/
VecH YCoCgToRGB(VecH YCoCg)
{
   Half Y =YCoCg.x,
        Co=YCoCg.y,
        Cg=YCoCg.z;

   return VecH(Y+Co-Cg ,
               Y+Cg    ,
               Y-Co-Cg);
}
VecH4 YCoCgToRGB(VecH4 YCoCg)
{
   Half Y =YCoCg.x,
        Co=YCoCg.y,
        Cg=YCoCg.z,
        A =YCoCg.a;

   return VecH4(Y+Co-Cg,
                Y+Cg   ,
                Y-Co-Cg,
                A     );
}
VecH YCoCg4ToRGB(VecH YCoCg4)
{
   Half Y =YCoCg4.x*0.25,
        Co=YCoCg4.y*0.25,
        Cg=YCoCg4.z*0.25;

   return VecH(Y+Co-Cg ,
               Y+Cg    ,
               Y-Co-Cg);
}
VecH4 YCoCg4ToRGB(VecH4 YCoCg4)
{
   Half Y =YCoCg4.x*0.25,
        Co=YCoCg4.y*0.25,
        Cg=YCoCg4.z*0.25,
        A =YCoCg4.a     ;

   return VecH4(Y+Co-Cg,
                Y+Cg   ,
                Y-Co-Cg,
                A     );
}
/******************************************************************************/
// these functions return how much we have to travel (0..1) from 'old' to 'cur' to be inside 'min', 'max'
Half GetBlend(VecH old, VecH cur, VecH min, VecH max) // 'cur' should be somewhere within 'min', 'max' (however due to cubic filtering it can be outside)
{
   VecH dir=cur-old,
    inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
   min_step=(min-old)*inv_dir, // how much travel needed to reach 'min' boundary
   max_step=(max-old)*inv_dir; // how much travel needed to reach 'max' boundary
   return Max(Min(min_step, max_step)); // don't 'Sat' to allow reporting high >1 differences (Warning: <0 can also be reported)
}
Half GetBlend(VecH4 old, VecH4 cur, VecH4 min, VecH4 max) // 'cur' should be somewhere within 'min', 'max' (however due to cubic filtering it can be outside)
{
   VecH4 dir=cur-old,
     inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
    min_step=(min-old)*inv_dir, // how much travel needed to reach 'min' boundary
    max_step=(max-old)*inv_dir; // how much travel needed to reach 'max' boundary
   return Max(Min(min_step, max_step)); // don't 'Sat' to allow reporting high >1 differences (Warning: <0 can also be reported)
}
/******************************************************************************/
void TestVel(VecH2 vel, VecH2 test_vel, inout Half max_delta_vel_len2)
{
   VecH2 delta_vel=vel-test_vel;
   delta_vel.x*=AspectRatio; // 'delta_vel' is in UV 0..1 for both XY so mul X by aspect ratio
   Half delta_vel_len2=Length2(delta_vel);
   if(  delta_vel_len2>max_delta_vel_len2)max_delta_vel_len2=delta_vel_len2;
}
/******************************************************************************/
void TestDepth(inout Flt depth, Flt d, inout VecI2 ofs, VecI2 o)
{
   if(DEPTH_SMALLER(d, depth)){depth=d; ofs=o;}
}
void NearestDepthRaw(out Flt depth, out VecI2 ofs, Vec2 uv, bool gather) // get raw depth nearest to camera around 'uv' !! TODO: Warning: this ignores VIEW_FULL, if this is fixed then 'UVClamp/UVInView' for uv+ofs can be removed !!
{
   if(gather)
   {
      ofs=VecI2(-1, 1); depth=TexDepthRawPointOfs(uv, ofs         );                     // -1,  1,  left-top
              TestDepth(depth,TexDepthRawPointOfs(uv, VecI2(1, -1)), ofs, VecI2(1, -1)); //  1, -1, right-bottom
      Vec2 tex=uv-RTSize.xy*0.5; // move to center between -1,-1 and 0,0 texels
      Vec4 d=TexDepthRawGather(tex); // get -1,-1 to 0,0 texels
      TestDepth(depth, d.x, ofs, VecI2(-1,  0));
      TestDepth(depth, d.y, ofs, VecI2( 0,  0));
      TestDepth(depth, d.z, ofs, VecI2( 0, -1));
      TestDepth(depth, d.w, ofs, VecI2(-1, -1));
      d=TexDepthRawGatherOfs(tex, VecI2(1, 1)); // get 0,0 to 1,1 texels
      TestDepth(depth, d.x, ofs, VecI2( 0,  1));
      TestDepth(depth, d.y, ofs, VecI2( 1,  1));
      TestDepth(depth, d.z, ofs, VecI2( 1,  0));
    //TestDepth(depth, d.w, ofs, VecI2( 0,  0)); already processed
   }else
   {
      ofs=0;
      depth=TexDepthRawPoint(uv);
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)if(x || y)TestDepth(depth, TexDepthRawPointOfs(uv, VecI2(x, y)), ofs, VecI2(x, y));
   }
}
void NearestDepth(out Flt depth, out VecI2 ofs, Vec2 uv, bool gather)
{
   NearestDepthRaw(depth, ofs, uv, gather);
   depth=LinearizeDepth(depth);
}
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
void TAA_PS(NOPERSP Vec2 uv   :UV,
          //NOPERSP Vec2 posXY:POS_XY,
          //NOPERSP PIXEL,
             #if MERGED_ALPHA
                out VecH  outData  :TARGET0,
             #else
                out VecH2 outData  :TARGET0,
             #endif
             #if SEPARATE_ALPHA
                out Half  outAlpha :TARGET1,
             #endif
                out VecH4 outCol   :TARGET2
             #if DUAL_HISTORY
              , out VecH4 outCol1  :TARGET3 // #TAADual
             #endif
            )
{
   // GET DEPTH
   Flt depth; VecI2 ofs;
   if(NEAREST_DEPTH_VEL)NearestDepthRaw(depth, ofs, uv, GATHER);
   else                      depth=TexDepthRawPoint(uv);

   // GET VEL
   VecH2 vel=TexPoint(ImgXY, NEAREST_DEPTH_VEL ? UVInView(uv+ofs*RTSize.xy, VIEW_FULL) : uv).xy;

   Vec2 cur_tex=uv+TAAOffset,
        old_tex=uv+vel;

   // OLD DATA (WEIGHT + FLICKER + ALPHA)
#if MERGED_ALPHA
   VecH  old_data=TexLod(Img3, old_tex).xyz;
   Half  old_alpha=old_data.x, old_weight=old_data.y, old_flicker=old_data.z;
#else
   VecH2 old_data=TexLod(ImgXY2, old_tex).xy;
   Half  old_weight=old_data.x, old_flicker=old_data.y;
#if ALPHA
   Half  old_alpha=TexLod(ImgX1, old_tex).x;
#endif
#endif

   if(UVOutsideView(old_tex)) // if 'old_tex' is outside viewport then ignore it
   {
      old_weight=0;
   #if !FLICKER_WEIGHT
      old_flicker=0;
   #endif
   }

   // OLD VEL TEST
#if TAA_OLD_VEL // if old velocity is different then ignore old, !! TODO: Warning: this ignores VIEW_FULL !!
   Vec2 old_tex_vel=old_tex+TAAOffsetCurToPrev;
   Half max_delta_vel_len2=0;
   #if GATHER
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(-1,  1)).xy, max_delta_vel_len2); // -1,  1,  left-top
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2( 1, -1)).xy, max_delta_vel_len2); //  1, -1, right-bottom
      old_tex_vel-=RTSize.xy*0.5; // move to center between -1,-1 and 0,0 texels
      VecH4 r=TexGatherR(ImgXY1, old_tex_vel); // get -1,-1 to 0,0 texels
      VecH4 g=TexGatherG(ImgXY1, old_tex_vel); // get -1,-1 to 0,0 texels
      TestVel(vel, VecH2(r.x, g.x), max_delta_vel_len2);
      TestVel(vel, VecH2(r.y, g.y), max_delta_vel_len2);
      TestVel(vel, VecH2(r.z, g.z), max_delta_vel_len2);
      TestVel(vel, VecH2(r.w, g.w), max_delta_vel_len2);
      r=TexGatherROfs(ImgXY1, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
      g=TexGatherGOfs(ImgXY1, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
      TestVel(vel, VecH2(r.x, g.x), max_delta_vel_len2);
      TestVel(vel, VecH2(r.y, g.y), max_delta_vel_len2);
      TestVel(vel, VecH2(r.z, g.z), max_delta_vel_len2);
    //TestVel(vel, VecH2(r.w, g.w), max_delta_vel_len2); already processed
   #else
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
         TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(x, y)).xy, max_delta_vel_len2);
   #endif
#endif

 /* test current velocities, skip because didn't help
   UNROLL for(Int y=-1; y<=1; y++)
   UNROLL for(Int x=-1; x<=1; x++)if(x || y)
      TestVel(vel, TexPointOfs(ImgXY, uv, VecI2(x, y)).xy, max_delta_vel_len2);*/

   // OLD COLOR
      CubicFastSampler cs;
      VecH4 old, old1;
#if CUBIC
      cs.set(old_tex); if(!VIEW_FULL)cs.UVClamp(ImgClamp.xy, ImgClamp.zw); // here do clamping because for CUBIC we check many samples around texcoord
      old =Max(VecH4(0,0,0,0), cs.tex(Img1)); // use Max(0) because of cubic sharpening potentially giving negative values
   #if DUAL_HISTORY
      old1=Max(VecH4(0,0,0,0), cs.tex(Img2)); // use Max(0) because of cubic sharpening potentially giving negative values
   #endif
#else
   // clamping 'old_tex' shouldn't be done, because we already detect if 'old_tex' is outside viewport and zero 'old_weight'
      old =TexLod(Img1, old_tex);
   #if DUAL_HISTORY
      old1=TexLod(Img2, old_tex);
   #endif
#endif

#if YCOCG
   VecH4 ycocg_min, ycocg_max;
#else
   VecH4 col_min, col_max;
#endif
   VecH4 cur;

   // CUR COLOR + CUR ALPHA
#if CUBIC
   cs.set(cur_tex); if(!VIEW_FULL)cs.UVClamp(ImgClamp.xy, ImgClamp.zw);
   #if ALPHA
      Half cur_alpha=Sat(cs.texX(ImgX)); // use Sat because of cubic sharpening potentially giving negative values
   #endif
#else
   if(!VIEW_FULL)cur_tex=UVClamp(cur_tex);
   #if ALPHA
      Half cur_alpha=TexLod(ImgX, cur_tex);
   #endif
#endif

#if CUBIC && MERGE_CUBIC_MIN_MAX // merged CUBIC with MIN MAX
   UNROLL for(Int y=0; y<4; y++)
   UNROLL for(Int x=0; x<4; x++)
      if((x!=0 && x!=3) || (y!=0 && y!=3)) // skip corners
   {
   #if VIEW_FULL
      VecH4 col=TexPointOfs(Img, cs.tc[0], VecI2(x, y));
   #else
      VecH4 col=TexPoint(Img, cs.uv(x, y));
   #endif
   #if YCOCG
      VecH4 ycocg=RGBToYCoCg4(col);
   #endif
      Half weight=cs.weight(x, y);
      if(x==1 && y==0) // first is (1,0) because corners are skipped
      {
         cur=col*weight;
      #if YCOCG
         ycocg_min=ycocg_max=ycocg;
      #else
         col_min=col_max=col;
      #endif
      }else
      {
         cur+=col*weight;
      #if YCOCG
         ycocg_min=Min(ycocg_min, ycocg);
         ycocg_max=Max(ycocg_max, ycocg);
      #else
         col_min=Min(col_min, col);
         col_max=Max(col_max, col);
      #endif
      }
   }
   cur/=1-cs.weight(0,0)-cs.weight(3,0)-cs.weight(0,3)-cs.weight(3,3); // we've skipped corners, so adjust by their weight (TotalWeight-CornerWeight)
   /* Adjust because based on following code, max weight for corners can be "corners=0.015625"
   Flt corners=0; Vec4 wy, wx;
   for(Flt y=0; y<=1; y+=0.01f)
   {
      Lerp4Weights(wy, y);
      for(Flt x=0; x<=1; x+=0.01f)
      {
         Lerp4Weights(wx, x);
         Flt weight=0;
         for(Int y=0; y<4; y++)
         for(Int x=0; x<4; x++)weight+=wy.c[y]*wx.c[x];
         Flt c=wy.c[0]*wx.c[0] + wy.c[3]*wx.c[0] + wy.c[0]*wx.c[3] + wy.c[3]*wx.c[3]; MAX(corners, c); if(!Equal(weight, 1))Exit("weight should be 1");
      }
   }*/
   cur=Max(VecH4(0,0,0,0), cur); // use Max(0) because of cubic sharpening potentially giving negative values
#else // this version uses 5 tex reads for CUBIC and 8 (or 9 if FILTER_MIN_MAX unavailable) tex reads for MIN MAX (13 total)
   #if CUBIC
      cur=Max(VecH4(0,0,0,0), cs.tex(Img)); // use Max(0) because of cubic sharpening potentially giving negative values
   #else
      cur=TexLod(Img, cur_tex);
   #endif

   // MIN MAX
   #if FILTER_MIN_MAX // check 3x3 samples by using 2x2 tex reads using Min/Max filtering (where each tex read gives minimum/maximum out of 2x2 samples)
   {
      Vec2 uv_clamp[2];
   #if VIEW_FULL
      uv_clamp[0]=uv-RTSize.xy/2;
      uv_clamp[1]=uv+RTSize.xy/2;
   #else
      uv_clamp[0]=Vec2(Max(uv.x-RTSize.x/2, ImgClamp.x), Max(uv.y-RTSize.y/2, ImgClamp.y));
      uv_clamp[1]=Vec2(Min(uv.x+RTSize.x/2, ImgClamp.z), Min(uv.y+RTSize.y/2, ImgClamp.w));
   #endif
      UNROLL for(Int y=0; y<=1; y++)
      UNROLL for(Int x=0; x<=1; x++)
      {
         VecH4 min=TexMin(Img, Vec2(uv_clamp[x].x, uv_clamp[y].y));
         VecH4 max=TexMax(Img, Vec2(uv_clamp[x].x, uv_clamp[y].y));
      #if YCOCG
         VecH4 ycocg_lo=RGBToYCoCg4(min), ycocg_hi=RGBToYCoCg4(max);
      #endif
         if(y==0 && x==0) // first
         {
         #if YCOCG
            ycocg_min=ycocg_lo; ycocg_max=ycocg_hi;
         #else
            col_min=min; col_max=max;
         #endif
         }else
         {
         #if YCOCG
            ycocg_min=Min(ycocg_min, ycocg_lo);
            ycocg_max=Max(ycocg_max, ycocg_hi);
         #else
            col_min=Min(col_min, min);
            col_max=Max(col_max, max);
         #endif
         }
      }
   }
   #else // check all 3x3 samples individually
   {
   #if !VIEW_FULL
      Vec2 uv_clamp[3];
           uv_clamp[0]=Vec2(Max(uv.x-RTSize.x, ImgClamp.x), Max(uv.y-RTSize.y, ImgClamp.y)); uv_clamp[1]=uv;
           uv_clamp[2]=Vec2(Min(uv.x+RTSize.x, ImgClamp.z), Min(uv.y+RTSize.y, ImgClamp.w));
   #endif
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
      {
         VecH4 col;
      #if VIEW_FULL
         col=TexPointOfs(Img, uv, VecI2(x, y));
      #else
         col=TexPoint(Img, Vec2(uv_clamp[x+1].x, uv_clamp[y+1].y));
      #endif
      #if YCOCG
         VecH4 ycocg=RGBToYCoCg4(col);
      #endif
         if(y==-1 && x==-1) // first
         {
         #if YCOCG
            ycocg_min=ycocg_max=ycocg;
         #else
            col_min=col_max=col;
         #endif
         }else
         {
         #if YCOCG
            ycocg_min=Min(ycocg_min, ycocg);
            ycocg_max=Max(ycocg_max, ycocg);
         #else
            col_min=Min(col_min, col);
            col_max=Max(col_max, col);
         #endif
         }
      }
   }
   #endif
#endif

   // when there are pixels moving in different directions fast, then restart TAA
   Half max_delta_vel_len=Sqrt(max_delta_vel_len2),
        blend_move=Sat(1-max_delta_vel_len/VEL_EPS);
#if DUAL_HISTORY
   if(blend_move<=0)old_weight=0; // for DUAL_HISTORY 'old_weight' affects 'old' and 'old1' in a special way, so can't just modify it easily
#else
   old_weight*=blend_move;
#endif
#if !FLICKER_WEIGHT
   old_flicker*=blend_move;
#endif

   // NEIGHBOR CLAMP
#if YCOCG
      VecH ycocg_old=RGBToYCoCg4(old.rgb);
      VecH ycocg_cur=RGBToYCoCg4(cur.rgb);
      Half     blend=GetBlend(ycocg_old, ycocg_cur, ycocg_min.rgb, ycocg_max.rgb);
   #if !MERGE_ADJUST_OLD
      Half glow_clamp=Mid(old.a, ycocg_min.a, ycocg_max.a); // glow in Alpha Channel #RTOutput
   #endif
#else
      Half blend=GetBlend(old.rgb, cur.rgb, col_min.rgb, col_max.rgb);
   #if !MERGE_ADJUST_OLD
      Half glow_clamp=Mid(old.a, col_min.a, col_max.a); // glow in Alpha Channel #RTOutput
   #endif
#endif

   // update flicker
   Half new_flicker;
   if(DEPTH_FOREGROUND(depth))
   {
      // calculate difference between 'old' and 'cur'
      Half difference=Dist(LinearToSRGBFast(old.rgb), LinearToSRGBFast(cur.rgb)); // it's better to use 'Dist' rather than 'Dist2', because it will prevent smooth changes from particles (like fire effect) being reported as flickering (if fire is reported as flickering then it will look very blurry)
           difference=Sat(difference/FLICKER_EPS);

    /*{ no because results are too sharp
         new_flicker=old_flicker + Lerp(-CUR_WEIGHT, CUR_WEIGHT, difference);
         Half flicker=                                    new_flicker  ; // too sharp
         Half flicker=Sat(LerpR(CUR_WEIGHT,            1, new_flicker)); // too sharp!
         Half flicker=Sat(LerpR(CUR_WEIGHT, CUR_WEIGHT*2, new_flicker)); // too sharp!!
      }
      {
         new_flicker=old_flicker + Lerp(-FLICKER_STEP, 0.5, difference);
         Half flicker=                                 new_flicker  ; // too sharp
         Half flicker=Sat(LerpR(0.5, 1               , new_flicker)); // too sharp!
         Half flicker=Sat(LerpR(0.5, 0.5+FLICKER_STEP, new_flicker)); // too sharp!!
      }*/
   #if FLICKER_WEIGHT
      new_flicker=old_flicker*old_weight_1 + difference*cur_weight_1; // always 0..1
   #else
      new_flicker=Lerp(old_flicker, difference, CUR_WEIGHT); // always 0..1
   #endif

    //Half     flicker=Sat(LerpR(CUR_WEIGHT, 1, new_flicker)); // cut-off small differences
      Half     flicker=new_flicker; // faster
      Half not_flicker=1-Sqr(flicker); // use Sqr to consider only big flickering

   #if !MERGE_ADJUST_OLD
      old.a=Lerp(old.a, glow_clamp, not_flicker); // glow in Alpha Channel #RTOutput
   #endif
      blend*=not_flicker;
   }else
   {
      new_flicker=0; // always disable flickering for sky because on cam zoom in/out the sky motion vectors don't change, and it could retain some flicker from another object that wouldn't get cleared
   #if !MERGE_ADJUST_OLD
      old.a=glow_clamp; // glow in Alpha Channel #RTOutput
   #endif
   }

   blend=Sat(blend);

#if !DUAL_HISTORY
   Half cur_weight=CUR_WEIGHT, new_weight=old_weight+cur_weight,
        old_weight_1=old_weight/new_weight, // old_weight_1+cur_weight_1=1
        cur_weight_1;

   // adjust 'old' towards 'cur'
   #if MERGE_ADJUST_OLD // instead of adjusting 'old' we can just tweak its weight because it gets combined later with 'cur' anyway
      // this needs to modify 'old_weight_1' and not ('old_weight' and 'new_weight') because that would increase flickering and in some tests it caused jittered ghosting (some pixels will look brighter and some look darker, depending on the color difference between old and new)
      old_weight_1*=1-blend;
      cur_weight_1 =1-old_weight_1;
   #else
      cur_weight_1=cur_weight/new_weight; // could be "1-old_weight_1" but then it won't be calculated in parallel
      #if YCOCG
         old.rgb=YCoCg4ToRGB(Lerp(ycocg_old, ycocg_cur, blend));
      #else
         old.rgb=Lerp(old.rgb, cur.rgb, blend); // old=Max(Min(old, col_max), col_min) is not enough because it can distort colors, what needs to be done is interpolation from 'old' to 'cur'
      #endif
      #if ALPHA
         old_alpha=Lerp(old_alpha, cur_alpha, blend);
      #endif
   #endif

              outCol=old      *old_weight_1 + cur      *cur_weight_1;
   #if ALPHA
      Half new_alpha=old_alpha*old_weight_1 + cur_alpha*cur_weight_1;
   #endif
   #if MERGED_ALPHA
      outData.x=new_alpha; // !! STORE ALPHA IN X CHANNEL SO IT CAN BE USED FOR '_alpha' RT !!
      outData.y=new_weight;
      outData.z=new_flicker;
   #else
      outData.x=new_weight;
      outData.y=new_flicker;
   #if ALPHA
      outAlpha=new_alpha;
   #endif
   #endif
   //if(T)outScreen=VecH4(SRGBToLinearFast(new_flicker).xxx, 0); // visualize flicker
#else
   #if YCOCG
      old.rgb=YCoCg4ToRGB(Lerp(ycocg_old, ycocg_cur, blend));
   #else
      old.rgb=Lerp(old.rgb, cur.rgb, blend); // old=Max(Min(old, col_max), col_min) is not enough because it can distort colors, what needs to be done is interpolation from 'old' to 'cur'
   #endif
   #if ALPHA
      old_alpha=Lerp(old_alpha, cur_alpha, blend);
   #endif

   old1=Lerp(old1, cur, blend); // TODO: for better precision this could use a separately calculated/processed 'blend1', and 'old1.a' glow could be processed separately too

   // #TAADualAlpha
   // old_weight<0.5 means 'old' is being filled from 0 (empty) .. 0.5 (full) and 'old1' is empty, old_weight>0.5 means 'old' is full and 'old1' is being filled from 0.5 (empty) .. 1 (full)
   Half cur_weight=CUR_WEIGHT/2, // since we operate on 0 (empty) .. 0.5 (full) we need to make cur weight 2x smaller
        new_weight=old_weight+cur_weight;
#if MERGED_ALPHA
   outData.y=new_weight;
   outData.z=new_flicker;
#else
   outData.x=new_weight;
   outData.y=new_flicker;
#endif
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT (since 'old_weight' is stored in 8-bit RT, then it won't be exactly 0.5, so we must use some epsilon, the best choice is half of 'cur_weight' step which is "cur_weight/2")
   {
      outCol1=outCol=outScreen=old*(old_weight/new_weight) + cur*(cur_weight/new_weight);
   }else // old_weight>0.5 = 1st history RT is full, fill 2nd history RT
   {
      if(DUAL_ADJUST_OLD)
      {
            Half ow=1, cw=CUR_WEIGHT, tw=ow+cw; // here we know 1st history RT is full, so we can use constants to make calculations faster
            outCol=old*(ow/tw) + cur*(cw/tw); // apply new color onto 'old'
      }else outCol=old;

      Half old1_weight=old_weight-0.5, // weight of the 2nd history RT calculated from 'old_weight', gives range 0 .. 0.5
           new1_weight=old1_weight+cur_weight; // 'new1_weight' means 'old1_weight' after applying new data, in range 0 .. 0.5
      outCol1=old1*(old1_weight/new1_weight) + cur*(cur_weight/new1_weight); // apply new color onto 'old1'

      outScreen=Lerp(outCol, outCol1, DUAL_ADJUST_OLD ? Sqr(new1_weight*2) : new1_weight*2); // 'old1' is more recent, but may not be fully set yet, so use it based on its weight and remaining values take from 'old'. If we're adjusting DUAL_ADJUST_OLD then 'old' gets updated with latest color, so we can make 'old1' less significant. *2 because here range is 0 .. 0.5

      if(new1_weight>=0.5 - cur_weight/2) // filled all history RT's (1st history RT is full, 2nd history RT is full)
      {
         outCol=outCol1; // move Old1 to Old
      #if MERGED_ALPHA
         outData.y=0.5; // mark Old1 as empty
      #else
         outData.x=0.5; // mark Old1 as empty
      #endif
      }
   }
#endif
}
/******************************************************************************/
