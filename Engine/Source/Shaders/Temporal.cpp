/******************************************************************************/
#include "!Header.h"
#include "Temporal.h"
#include "Motion Blur.h"
/******************************************************************************
MODE, VIEW_FULL, ALPHA, DUAL_HISTORY, GATHER, FILTER_MIN_MAX

ImgSize=src
 RTSize=dest (is 2x bigger for SUPER)

Img=CurCol (ImgSize), Img1=OldCol (RTSize), ImgXY=CurVel (ImgSize), Img2=CurDilatedMotion (small res), Depth (ImgSize)

ALPHA=0
   ImgX1=Flicker
ALPHA=1
   ImgX=CurAlpha (ImgSize)
      MERGED_ALPHA=1
         ImgXY1=Alpha, Flicker
      SEPARATE_ALPHA=1
         ImgX1=Flicker
         ImgX2=Old Alpha


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
#define ANTI_ALIAS (((MODE+1)&1)!=0)
#define SUPER      (((MODE+1)&2)!=0)

#define SEPARATE_ALPHA ( TEMPORAL_SEPARATE_ALPHA && ALPHA)
#define   MERGED_ALPHA (!TEMPORAL_SEPARATE_ALPHA && ALPHA)

#define OLD_WEIGHT         (1-1.0/8)
#define OLD_FLICKER_WEIGHT (1-1.0/4) // can update flicker at a faster rate

#define FLICKER_EPS 0.2 // this is value for color distance ~0..1 in sRGB gamma (lower numbers increase detection of flicker, higher numbers decrease detection of flicker), 0.2 was the smallest value that didn't cause noticable artifacts/blurriness on a particle fire effect

#define YCOCG 0 // this reduces ghosting, however increases flickering in certain cases (high contrast black and white, seen on "UID(2793579270, 1288897959, 2826231732, 169976521)" model)
#if     YCOCG
   #define FILTER_MIN_MAX 0 // can't use FILTER_MIN_MAX with YCOCG
#endif

#define NEAREST_DEPTH_VEL 1

#define SHOW_FLICKER T

#define CUBIC 1
#if     CUBIC
   #include "Cubic.h"
#endif

#define MERGE_CUBIC_MIN_MAX 1 // enable since this version is slightly better because: uses 12 tex reads (4*4 -4 corners), uses 12 samples for MIN/MAX which reduces flickering a bit, however has a lot more arithmetic calculations because of min/max x12 and each sample color is multiplied by weight separately

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
Half GetBlend(VecH2 old, VecH2 cur, VecH2 min, VecH2 max) // 'cur' should be somewhere within 'min', 'max' (however due to cubic filtering it can be outside)
{
   VecH2 dir=cur-old,
     inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
    min_step=(min-old)*inv_dir, // how much travel needed to reach 'min' boundary
    max_step=(max-old)*inv_dir; // how much travel needed to reach 'max' boundary
   return Max(Min(min_step, max_step));
}
Half GetBlend(VecH old, VecH cur, VecH min, VecH max) // 'cur' should be somewhere within 'min', 'max' (however due to cubic filtering it can be outside)
{
   VecH dir=cur-old,
    inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
   min_step=(min-old)*inv_dir, // how much travel needed to reach 'min' boundary
   max_step=(max-old)*inv_dir; // how much travel needed to reach 'max' boundary
   return Max(Min(min_step, max_step));
}
Half GetBlend(VecH4 old, VecH4 cur, VecH4 min, VecH4 max) // 'cur' should be somewhere within 'min', 'max' (however due to cubic filtering it can be outside)
{
   VecH4 dir=cur-old,
     inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
    min_step=(min-old)*inv_dir, // how much travel needed to reach 'min' boundary
    max_step=(max-old)*inv_dir; // how much travel needed to reach 'max' boundary
   return Max(Min(min_step, max_step));
}
/******************************************************************************/
Half GetBlend1(VecH2 old, VecH2 cur, VecH2 min, VecH2 max) // returns "1-GetBlend"
{
   VecH2 dir=old-cur,
     inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
    min_step=(min-cur)*inv_dir, // how much travel needed to reach 'min' boundary
    max_step=(max-cur)*inv_dir; // how much travel needed to reach 'max' boundary
   return Min(Max(min_step, max_step));
}
Half GetBlend1(VecH old, VecH cur, VecH min, VecH max) // returns "1-GetBlend"
{
   VecH dir=old-cur,
    inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
   min_step=(min-cur)*inv_dir, // how much travel needed to reach 'min' boundary
   max_step=(max-cur)*inv_dir; // how much travel needed to reach 'max' boundary
   return Min(Max(min_step, max_step));
}
Half GetBlend1(VecH4 old, VecH4 cur, VecH4 min, VecH4 max) // returns "1-GetBlend"
{
   VecH4 dir=old-cur,
     inv_dir=((dir!=0) ? rcp(dir) : HALF_MAX), // NaN, this is a per-component operation
    min_step=(min-cur)*inv_dir, // how much travel needed to reach 'min' boundary
    max_step=(max-cur)*inv_dir; // how much travel needed to reach 'max' boundary
   return Min(Max(min_step, max_step));
}
/******************************************************************************/
VecH2 UVToScreen   (VecH2 uv) {return VecH2(uv.x*AspectRatio, uv.y);} // this is only to maintain XY proportions (it does not convert to screen coordinates)
Half  ScreenLength2(VecH2 uv) {return Length2(UVToScreen(uv));}
/******************************************************************************/
void TestMotion(VecH2 uv_motion, VecH2 test_uv_motion, inout Half max_screen_delta_len2)
{
   Half screen_delta_len2=ScreenLength2(uv_motion-test_uv_motion);
   if(  screen_delta_len2>max_screen_delta_len2)max_screen_delta_len2=screen_delta_len2;
}
/******************************************************************************/
void NearestDepthRaw3x3(out Flt depth_center, out VecI2 ofs, Vec2 uv, bool gather) // get raw depth nearest to camera around 'uv' !! TODO: Warning: this ignores VIEW_FULL, if this is fixed then 'UVClamp/UVInView' for uv+ofs can be removed !!
{
   Flt depth;
   if(gather)
   {
      ofs=VecI2(-1, 1); depth=TexDepthRawPointOfs(uv, ofs         );                     // -1,  1,  left-top
              TestDepth(depth,TexDepthRawPointOfs(uv, VecI2(1, -1)), ofs, VecI2(1, -1)); //  1, -1, right-bottom
      uv-=(SUPER ? RTSize.xy : ImgSize.xy*0.5); // move to center between -1,-1 and 0,0 texels
      Vec4 d=TexDepthRawGather(uv); // get -1,-1 to 0,0 texels
      TestDepth(depth, d.x, ofs, VecI2(-1,  0));
      TestDepth(depth, d.y, ofs, VecI2( 0,  0)); depth_center=d.y;
      TestDepth(depth, d.z, ofs, VecI2( 0, -1));
      TestDepth(depth, d.w, ofs, VecI2(-1, -1));
      d=TexDepthRawGatherOfs(uv, VecI2(1, 1)); // get 0,0 to 1,1 texels
      TestDepth(depth, d.x, ofs, VecI2( 0,  1));
      TestDepth(depth, d.y, ofs, VecI2( 1,  1));
      TestDepth(depth, d.z, ofs, VecI2( 1,  0));
    //TestDepth(depth, d.w, ofs, VecI2( 0,  0)); already processed
   }else
   {
      ofs=0;
      depth_center=depth=TexDepthRawPoint(uv);
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)if(x || y)TestDepth(depth, TexDepthRawPointOfs(uv, VecI2(x, y)), ofs, VecI2(x, y));
   }
}
void NearestDepthRaw4x4(out Flt depth_center, out VecI2 ofs, Vec2 uv, bool gather) // get raw depth nearest to camera around 'uv' !! TODO: Warning: this ignores VIEW_FULL, if this is fixed then 'UVClamp/UVInView' for uv+ofs can be removed !!
{ /* Unoptimized:
   depth_center=depth=TexDepthRawPoint(uv); ofs=0;
   Int min_y=-1, max_y=1, min_x=-1, max_x=1;
   if(TemporalOffset.x<0)min_x--;else max_x++;
   if(TemporalOffset.y<0)min_y--;else max_y++;
   for(Int y=min_y; y<=max_y; y++)
   for(Int x=min_x; x<=max_x; x++)TestDepth(depth, TexDepthRawPoint(uv+VecI2(x, y)*ImgSize), ofs, VecI2(x, y)); */
   Flt   depth;
   VecI2 sub_offset=(TemporalOffsetStart<0); // if negative then we will start from -1, so have to subtract 1. 'sub_offset' is also coordinate of the center (0,0) pixel when using -1..2 range
   if(gather)
   {
      uv+=TemporalOffsetStart-(SUPER ? RTSize.xy : ImgSize.xy*0.5); // move to center between -1,-1 and 0,0 texels
   #if 0 // original
      Vec4 ld=TexDepthRawGather(uv); // get -1,-1 to 0,0 texels
                depth= ld.x; ofs= VecI2(-1,  0) ; // first
    //TestDepth(depth, ld.x, ofs, VecI2(-1,  0));
      TestDepth(depth, ld.y, ofs, VecI2( 0,  0)); /*if(all(VecI2(0, 0)==sub_offset))*/depth_center=ld.y;
      TestDepth(depth, ld.z, ofs, VecI2( 0, -1));
      TestDepth(depth, ld.w, ofs, VecI2(-1, -1));

      Vec4 rd=TexDepthRawGatherOfs(uv, VecI2(2, 0)); // get 1,-1 to 2,0 texels
      TestDepth(depth, rd.x, ofs, VecI2( 1,  0)); if(all(VecI2(1, 0)==sub_offset))depth_center=rd.x;
      TestDepth(depth, rd.y, ofs, VecI2( 2,  0));
      TestDepth(depth, rd.z, ofs, VecI2( 2, -1));
      TestDepth(depth, rd.w, ofs, VecI2( 1, -1));

      Vec4 lu=TexDepthRawGatherOfs(uv, VecI2(0, 2)); // get -1,1 to 0,2 texels
      TestDepth(depth, lu.x, ofs, VecI2(-1,  2));
      TestDepth(depth, lu.y, ofs, VecI2( 0,  2));
      TestDepth(depth, lu.z, ofs, VecI2( 0,  1)); if(all(VecI2(0, 1)==sub_offset))depth_center=lu.z;
      TestDepth(depth, lu.w, ofs, VecI2(-1,  1));

      Vec4 ru=TexDepthRawGatherOfs(uv, VecI2(2, 2)); // get 1,1 to 2,2 texels
      TestDepth(depth, ru.x, ofs, VecI2( 1,  2));
      TestDepth(depth, ru.y, ofs, VecI2( 2,  2));
      TestDepth(depth, ru.z, ofs, VecI2( 2,  1));
      TestDepth(depth, ru.w, ofs, VecI2( 1,  1)); if(all(VecI2(1, 1)==sub_offset))depth_center=ru.w;
   #else // test center pixels first, this is for cases where multiple pixels have the same depth and we want to prioritize selecting offset closer to the center
      Vec4 ld=TexDepthRawGather   (uv             ); // get -1,-1 to 0,0 texels
      Vec4 rd=TexDepthRawGatherOfs(uv, VecI2(2, 0)); // get  1,-1 to 2,0 texels
      Vec4 lu=TexDepthRawGatherOfs(uv, VecI2(0, 2)); // get -1, 1 to 0,2 texels
      Vec4 ru=TexDepthRawGatherOfs(uv, VecI2(2, 2)); // get  1, 1 to 2,2 texels

                depth= ld.y; ofs= VecI2( 0,  0) ; /*if(all(VecI2(0, 0)==sub_offset))*/depth_center=ld.y; // first, skip 'if' to avoid overhead
    //TestDepth(depth, ld.y, ofs, VecI2( 0,  0)); /*if(all(VecI2(0, 0)==sub_offset))*/depth_center=ld.y;
      TestDepth(depth, rd.x, ofs, VecI2( 1,  0));   if(all(VecI2(1, 0)==sub_offset))  depth_center=rd.x;
      TestDepth(depth, lu.z, ofs, VecI2( 0,  1));   if(all(VecI2(0, 1)==sub_offset))  depth_center=lu.z;
      TestDepth(depth, ru.w, ofs, VecI2( 1,  1));   if(all(VecI2(1, 1)==sub_offset))  depth_center=ru.w;

      TestDepth(depth, ld.x, ofs, VecI2(-1,  0));
    //TestDepth(depth, ld.y, ofs, VecI2( 0,  0)); if(all(VecI2(0, 0)==sub_offset))depth_center=ld.y;
      TestDepth(depth, ld.z, ofs, VecI2( 0, -1));
      TestDepth(depth, ld.w, ofs, VecI2(-1, -1));

    //TestDepth(depth, rd.x, ofs, VecI2( 1,  0)); if(all(VecI2(1, 0)==sub_offset))depth_center=rd.x;
      TestDepth(depth, rd.y, ofs, VecI2( 2,  0));
      TestDepth(depth, rd.z, ofs, VecI2( 2, -1));
      TestDepth(depth, rd.w, ofs, VecI2( 1, -1));

      TestDepth(depth, lu.x, ofs, VecI2(-1,  2));
      TestDepth(depth, lu.y, ofs, VecI2( 0,  2));
    //TestDepth(depth, lu.z, ofs, VecI2( 0,  1)); if(all(VecI2(0, 1)==sub_offset))depth_center=lu.z;
      TestDepth(depth, lu.w, ofs, VecI2(-1,  1));

      TestDepth(depth, ru.x, ofs, VecI2( 1,  2));
      TestDepth(depth, ru.y, ofs, VecI2( 2,  2));
      TestDepth(depth, ru.z, ofs, VecI2( 2,  1));
    //TestDepth(depth, ru.w, ofs, VecI2( 1,  1)); if(all(VecI2(1, 1)==sub_offset))depth_center=ru.w;
   #endif
   }else
   {
      uv+=TemporalOffsetStart;
      UNROLL for(Int y=-1; y<=2; y++) // 4
      UNROLL for(Int x=-1; x<=2; x++) // 4
      {
         VecI2 o=VecI2(x, y);
         Flt d=TexDepthRawPointOfs(uv, o);
         if(x==-1 && y==-1){depth= d; ofs= o;} // first
         else     TestDepth(depth, d, ofs, o);
         if(x>=0 && x<=1 && y>=0 && y<=1 // center pixel can happen only in this range, since this is unrolled then we can check this for free
         && all(o==sub_offset))depth_center=d; // if this is center pixel
      }
   }
   ofs-=sub_offset;
}
/******************************************************************************/
Half Cover(Flt depth, VecH2 screen_delta, Half screen_delta_len2, Half bias, Flt obj_depth, VecH2 obj_uv_motion)
{
   VecH2 obj_screen_motion=UVToScreen(obj_uv_motion);
   Half  move=Dot(obj_screen_motion, screen_delta)+bias, // remember that 'screen_delta' is not normalized
         frac=move/screen_delta_len2, // and calculate as fraction
         in_front=Sat((depth-obj_depth)/DEPTH_TOLERANCE/*+0.5 adding this here would cause self-occlusion, so disable it*/); // if that pixel is in front of this one
   return LerpRS(Sqr(0.5), Sqr(0.75), frac)*in_front;
}
void Temporal_PS
(
   NOPERSP Vec2 uv:UV,
#if SUPER
   NOPERSP PIXEL,
#endif
#if MERGED_ALPHA
   out VecH2 outData :TARGET0,
#else
   out Half  outData :TARGET0,
#endif
#if SEPARATE_ALPHA
   out Half  outAlpha:TARGET1,
#endif
   out VecH4 outCol  :TARGET2
)
{
   // GET DEPTH
   Flt depth_raw; VecI2 ofs;
   if(NEAREST_DEPTH_VEL)NearestDepthRaw4x4(depth_raw, ofs, uv, GATHER); // now we use 4x4 samples (old: need to use 3x3 because 2x2 are not enough)
   else                 depth_raw=TexDepthRawPoint(uv);
   Flt depth=LinearizeDepth(depth_raw);

   // DEBUG
   /*{
      Flt d_max=Z_BACK; Vec2 test=uv+TemporalOffsetStart;
      UNROLL for(Int y=-1; y<=2; y++)
      UNROLL for(Int x=-1; x<=2; x++)d_max=DEPTH_MIN(d_max, TexDepthRawPointOfs(test, VecI2(x, y)));
      Flt depth_raw1; VecI2 ofs1; NearestDepthRaw4x4(depth_raw1, ofs1, uv, false);
      if( depth_raw!=TexDepthRawPoint(uv)
       || depth_raw!=depth_raw1
       || d_max!=TexDepthRawPoint(uv+ofs *ImgSize.xy)
       || d_max!=TexDepthRawPoint(uv+ofs1*ImgSize.xy)
     //|| any(ofs!=ofs1) this might be different because it depends on order of testing
       ){outCol=Vec4(1,0,1,0); return;}
   }*/

   // GET MOTION
   Vec2  depth_motion_uv=NEAREST_DEPTH_VEL ? UVInView(uv+ofs*ImgSize.xy, VIEW_FULL) : uv; // 'depth_motion_uv'=UV that's used for DepthMotion
   VecH2 uv_motion=TexPoint(ImgXY, depth_motion_uv).xy;
   Vec2  cur_uv=uv+TemporalOffset,
         old_uv=uv-uv_motion; // #MotionDir
   Half  old_weight=UVInsideView(old_uv); // use old only if it's inside viewport

   // IGNORE OLD BY MOVEMENT
   Half blend_move=1;

   // check if any object covered old pixel in previous frame
   // this needs to be checked even if current pixel is not moving (uv_motion=0)
   // TODO: for best results this would need a separate dilated motion RT without any MotionScale_2, perhaps it could be smaller than DilatedMotion for Motion Blur
   VecH4 dilated_uv_motion=TexLod(Img2, old_uv); // use filtering because this is very low res, this might be a little problem for DUAL_MOTION since there one pixel can have XY=A, ZW=B, and another can have XY=B, ZW=0 (B vel is in XY), filtering would make values invalid, however it still looks better with filtering than without, because without filtering there are blocky artifacts, with filtering artifacts are smoother

   // if current pixel is moving, or there's any movement at old position, this check improves performance, so keep it
   const Half min_pixel_motion=0.5;
#if 0
 //if(min_pixel_motion<0 ||           any(Abs(uv_motion)+          Abs(dilated_uv_motion.xy) >    ImgSize.xy*min_pixel_motion)) // 4 abs, 2 add, 2 mul, 2 compare, 1 any
 //if(min_pixel_motion<0 || ScreenLength2(    uv_motion)+ScreenLength2(dilated_uv_motion.xy) >Sqr(ImgSize. y*min_pixel_motion)) //               2 mul, 2 dot (4 mul, 2 add), 1 add, 1 mul, 1 mul, 1 compare, less precise because adding squared values
   if(min_pixel_motion<0 || ScreenLength2(Abs(uv_motion)+          Abs(dilated_uv_motion.xy))>Sqr(ImgSize. y*min_pixel_motion)) // 4 abs, 2 add, 1 mul, 1 dot (2 mul, 1 add),        1 mul, 1 mul, 1 compare
   {
#else
   if(min_pixel_motion<0 || ScreenLength2(uv_motion)>Sqr(ImgSize.y*min_pixel_motion) || any(dilated_uv_motion.xy)) // #DilatedMotionZero
   {
#endif
      Half bias=Sqr(ImgSize.y*0.5); // to allow checking zero length motions, fix for div by 0, makes a/b -> (a+bias)/(b+bias)
      Half cover=0; // initialize in case we don't process any
      
      // motion-based detection is not perfect, because motions are in the neighborhood, so if moving from 'old_uv' by 'dilated_uv_motion' we might actually encounter pixels that have no motion, or too small
      // there are plenty of pixels in the neighborhood, and many times we will encounter the good ones, but sometimes we will miss them
      // this helps however in certain cases, when looking at the door, and opening it, it will clear some pixels that belonged to the door but now are the background
      
      // check primary motion
      {
         Vec2 obj_uv=UVInView(old_uv+dilated_uv_motion.xy, VIEW_FULL); // #MotionDir get fastest moving pixel in the neighborhood
         if(0) // slower, higher quality
         {
            UNROLL for(Int y=-1; y<=1; y++)
            UNROLL for(Int x=-1; x<=1; x++)
            {
               VecH2 screen_delta=UVToScreen(dilated_uv_motion.xy+VecI2(x,y)*ImgSize.xy); // calculate 'screen_delta' per-sample
               Half  screen_delta_len2=Length2(screen_delta)+bias; // full distance
               Half  c=Cover(depth, screen_delta, screen_delta_len2, bias, TexDepthPointOfs(obj_uv, VecI2(x,y)), TexPointOfs(ImgXY, obj_uv, VecI2(x,y)));
               cover=Max(cover, c);
            }
         }else
         {
            // here 'screen_delta' is also the delta from current position to object position (distance), so we have to check if 'obj_screen_motion' reaches 'screen_delta'
            VecH2 screen_delta=UVToScreen(dilated_uv_motion.xy); // FIXME for !VIEW_FULL should this be set to delta between old_uv and obj_uv, however what about pixels at the viewport border, they would cover themself?
            Half  screen_delta_len2=Length2(screen_delta)+bias; // full distance
         #if GATHER
            Vec4  d           =TexDepthRawGather(obj_uv);
            VecH4 obj_motion_x=TexGatherR(ImgXY, obj_uv);
            VecH4 obj_motion_y=TexGatherG(ImgXY, obj_uv);
            cover=    Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.x), VecH2(obj_motion_x.x, obj_motion_y.x)); // first pixel can be set instead of Max
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.y), VecH2(obj_motion_x.y, obj_motion_y.y)), cover);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.z), VecH2(obj_motion_x.z, obj_motion_y.z)), cover);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.w), VecH2(obj_motion_x.w, obj_motion_y.w)), cover);
         #else
            obj_uv-=ImgSize.xy/2;
            UNROLL for(Int y=0; y<=1; y++)
            UNROLL for(Int x=0; x<=1; x++)
            {
               Half c=Cover(depth, screen_delta, screen_delta_len2, bias, TexDepthPointOfs(obj_uv, VecI2(x,y)), TexPointOfs(ImgXY, obj_uv, VecI2(x,y)));
               if(x==0 && y==0)cover=c;else cover=Max(cover, c); // first pixel can be set instead of Max
            }
         #endif
         }
      }

      // check secondary motion (check this for both DUAL_MOTION on/off)
      {
         Vec2 obj_uv=UVInView(old_uv+dilated_uv_motion.zw, VIEW_FULL); // #MotionDir get fastest moving pixel in the neighborhood
         if(0) // slower, higher quality
         {
            UNROLL for(Int y=-1; y<=1; y++)
            UNROLL for(Int x=-1; x<=1; x++)
            {
               VecH2 screen_delta=UVToScreen(dilated_uv_motion.zw+VecI2(x,y)*ImgSize.xy); // calculate 'screen_delta' per-sample
               Half  screen_delta_len2=Length2(screen_delta)+bias; // full distance
               Half  c=Cover(depth, screen_delta, screen_delta_len2, bias, TexDepthPointOfs(obj_uv, VecI2(x,y)), TexPointOfs(ImgXY, obj_uv, VecI2(x,y)));
               cover=Max(cover, c);
            }
         }else
         {
            // here 'screen_delta' is also the delta from current position to object position (distance), so we have to check if 'obj_screen_motion' reaches 'screen_delta'
            VecH2 screen_delta=UVToScreen(dilated_uv_motion.zw); // FIXME for !VIEW_FULL should this be set to delta between old_uv and obj_uv, however what about pixels at the viewport border, they would cover themself?
            Half  screen_delta_len2=Length2(screen_delta)+bias; // full distance
         #if GATHER
            Vec4  d           =TexDepthRawGather(obj_uv);
            VecH4 obj_motion_x=TexGatherR(ImgXY, obj_uv);
            VecH4 obj_motion_y=TexGatherG(ImgXY, obj_uv);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.x), VecH2(obj_motion_x.x, obj_motion_y.x)), cover);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.y), VecH2(obj_motion_x.y, obj_motion_y.y)), cover);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.z), VecH2(obj_motion_x.z, obj_motion_y.z)), cover);
            cover=Max(Cover(depth, screen_delta, screen_delta_len2, bias, LinearizeDepth(d.w), VecH2(obj_motion_x.w, obj_motion_y.w)), cover);
         #else
            obj_uv-=ImgSize.xy/2;
            UNROLL for(Int y=0; y<=1; y++)
            UNROLL for(Int x=0; x<=1; x++)
            {
               Half c=Cover(depth, screen_delta, screen_delta_len2, bias, TexDepthPointOfs(obj_uv, VecI2(x,y)), TexPointOfs(ImgXY, obj_uv, VecI2(x,y)));
               cover=Max(cover, c);
            }
         #endif
         }
      }

      blend_move=1-cover;

      // because motion-based detection is not perfect, additionally use another method:
      // expect old position to be moving with the same motion as this pixel, if not then reduce old weight !! TODO: Warning: this ignores VIEW_FULL !!
      if(1) // check 'old_uv' because have to test motions of pixels that we want to use for old color
      {
         Half max_screen_delta_len2=0; // max movement difference between this and samples in old position
      #if GATHER
         // 3x3 samples are needed, 2x2 and 1x1 had ghosting and artifacts
         if(0) // 2x2
         {
            VecH4 r=TexGatherR(ImgXY, old_uv);
            VecH4 g=TexGatherG(ImgXY, old_uv);
            TestMotion(uv_motion, VecH2(r.x, g.x), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.y, g.y), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.z, g.z), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.w, g.w), max_screen_delta_len2);
         }else // 3x3
         {
            Vec2 gather_uv=old_uv;
            TestMotion(uv_motion, TexPointOfs(ImgXY, gather_uv, VecI2(-1,  1)).xy, max_screen_delta_len2); // -1,  1,  left-top
            TestMotion(uv_motion, TexPointOfs(ImgXY, gather_uv, VecI2( 1, -1)).xy, max_screen_delta_len2); //  1, -1, right-bottom
            gather_uv-=(SUPER ? RTSize.xy : ImgSize.xy*0.5); // move to center between -1,-1 and 0,0 texels
            VecH4 r=TexGatherR(ImgXY, gather_uv); // get -1,-1 to 0,0 texels
            VecH4 g=TexGatherG(ImgXY, gather_uv); // get -1,-1 to 0,0 texels
            TestMotion(uv_motion, VecH2(r.x, g.x), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.y, g.y), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.z, g.z), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.w, g.w), max_screen_delta_len2);
            r=TexGatherROfs(ImgXY, gather_uv, VecI2(1, 1)); // get 0,0 to 1,1 texels
            g=TexGatherGOfs(ImgXY, gather_uv, VecI2(1, 1)); // get 0,0 to 1,1 texels
            TestMotion(uv_motion, VecH2(r.x, g.x), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.y, g.y), max_screen_delta_len2);
            TestMotion(uv_motion, VecH2(r.z, g.z), max_screen_delta_len2);
          //TestMotion(uv_motion, VecH2(r.w, g.w), max_screen_delta_len2); already processed
         }
      #else
         UNROLL for(Int y=-1; y<=1; y++)
         UNROLL for(Int x=-1; x<=1; x++)
            TestMotion(uv_motion, TexPointOfs(ImgXY, old_uv, VecI2(x, y)).xy, max_screen_delta_len2);
      #endif

         Half screen_motion_len2=ScreenLength2(uv_motion)+Sqr(ImgSize.y*4); // this pixel movement, add some bias which helps for slowly moving pixels on static background (example FPS view+walking+tree leafs on static sky), "+bias" works better than "Max(, bias)", *4 was the smallest number that disabled flickering on common scenario of walking/running in FPS view
         Half frac=max_screen_delta_len2/screen_motion_len2;
         blend_move=Min(blend_move, LerpRS(Sqr(0.5), Sqr(0.25), frac));
      }

      old_weight*=blend_move;
   }

   // OLD DATA (FLICKER + ALPHA)
#if MERGED_ALPHA
   VecH2 old_data =TexLod(ImgXY1, old_uv);
   Half  old_alpha=old_data.x, old_flicker=old_data.y;
#else
   Half  old_flicker=TexLod(ImgX1, old_uv);
#if ALPHA
   Half  old_alpha=TexLod(ImgX2, old_uv);
#endif
#endif

   // OLD COLOR
   VecH4 old, old1;
#if CUBIC
      CubicFastSampler Sampler;
      Sampler.set(old_uv, RTSize); if(!VIEW_FULL)Sampler.UVClamp(ImgClamp.xy, ImgClamp.zw); // here do clamping because for CUBIC we check many samples around texcoord
      old =Max(VecH4(0,0,0,0), Sampler.tex(Img1)); // use Max(0) because of cubic sharpening potentially giving negative values
   #if DUAL_HISTORY
      old1=Max(VecH4(0,0,0,0), Sampler.tex(Img2)); // use Max(0) because of cubic sharpening potentially giving negative values
   #endif
#else
   // clamping 'old_uv' shouldn't be done, because we already detect if 'old_uv' is outside viewport and zero 'old_weight'
      old =TexLod(Img1, old_uv);
   #if DUAL_HISTORY
      old1=TexLod(Img2, old_uv);
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
   if(SUPER     )Sampler.setSharp(cur_uv, ImgSize, old_weight); // when 'old_weight' is zero and old pixels are ignored, then use blurry weight to avoid flickering (due to sharpening and jitter)
   else          Sampler.set     (cur_uv, ImgSize            );
   if(!VIEW_FULL)Sampler.UVClamp (ImgClamp.xy, ImgClamp.zw);
   #if ALPHA
      Half cur_alpha=Sat(Sampler.texX(ImgX)); // use Sat because of cubic sharpening potentially giving negative values
   #endif
#else
   if(!VIEW_FULL)cur_uv=UVClamp(cur_uv);
   #if ALPHA
      Half cur_alpha=TexLod(ImgX, cur_uv);
   #endif
#endif

#if CUBIC && MERGE_CUBIC_MIN_MAX // merged CUBIC with MIN MAX
{
   const Bool skip_corners=true;
   UNROLL for(Int y=0; y<4; y++)
   UNROLL for(Int x=0; x<4; x++)
      if(!skip_corners || (x!=0 && x!=3) || (y!=0 && y!=3)) // skip corners
   {
   #if VIEW_FULL
      VecH4 col=TexPointOfs(Img, Sampler.tc[0], VecI2(x, y));
   #else
      VecH4 col=TexPoint(Img, Sampler.uv(x, y));
   #endif
      Half weight=Sampler.weight(x, y);
   #if YCOCG
      VecH4 ycocg=RGBToYCoCg4(col);
   #endif
      if(x==skip_corners && y==0) // first
      {
         cur=col*weight;
      #if YCOCG
         ycocg_min=ycocg;
         ycocg_max=ycocg;
      #else
         col_min=col;
         col_max=col;
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
   if(skip_corners)cur/=1-Sampler.cornersWeight(); // we've skipped corners, so adjust by their weight (TotalWeight-CornerWeight)
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
}
#else // this version uses 5 tex reads for CUBIC and 8 (or 9 if FILTER_MIN_MAX unavailable) tex reads for MIN MAX (13 total)
{
   #if CUBIC
      cur=Max(VecH4(0,0,0,0), Sampler.tex(Img)); // use Max(0) because of cubic sharpening potentially giving negative values
   #else
      cur=TexLod(Img, cur_uv);
   #endif

   // MIN MAX
   #if FILTER_MIN_MAX // check 3x3 samples by using 2x2 tex reads using Min/Max filtering (where each tex read gives minimum/maximum out of 2x2 samples)
   {
      Vec2 uv_clamp[2];
   #if VIEW_FULL
      uv_clamp[0]=uv-(SUPER ? RTSize.xy : ImgSize.xy/2);
      uv_clamp[1]=uv+(SUPER ? RTSize.xy : ImgSize.xy/2);
   #else
      uv_clamp[0]=Vec2(Max(uv.x-(SUPER ? RTSize.x : ImgSize.x/2), ImgClamp.x), Max(uv.y-(SUPER ? RTSize.y : ImgSize.y/2), ImgClamp.y));
      uv_clamp[1]=Vec2(Min(uv.x+(SUPER ? RTSize.x : ImgSize.x/2), ImgClamp.z), Min(uv.y+(SUPER ? RTSize.y : ImgSize.y/2), ImgClamp.w));
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
           uv_clamp[0]=Vec2(Max(uv.x-ImgSize.x, ImgClamp.x), Max(uv.y-ImgSize.y, ImgClamp.y)); uv_clamp[1]=uv;
           uv_clamp[2]=Vec2(Min(uv.x+ImgSize.x, ImgClamp.z), Min(uv.y+ImgSize.y, ImgClamp.w));
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
}
#endif

   // NEIGHBOR CLAMP
#if YCOCG
   VecH ycocg_old=RGBToYCoCg4(old.rgb);
   VecH ycocg_cur=RGBToYCoCg4(cur.rgb);
   Half     blend=GetBlend(ycocg_old, ycocg_cur, ycocg_min.xyz, ycocg_max.xyz);
#else
   Half blend=GetBlend(old.rgb, cur.rgb, col_min.rgb, col_max.rgb);
#endif
   blend=Sat(blend);

   // update flicker
   Half new_flicker;
   if(DEPTH_FOREGROUND(depth_raw))
   {
      // calculate difference between 'old' and 'cur'
      Half difference=SHOW_FLICKER ? Dist(LinearToSRGBFast(old.rg ), LinearToSRGBFast(cur.rg )) // it's better to use 'Dist' rather than 'Dist2', because it will prevent smooth changes from particles (like fire effect) being reported as flickering (if fire is reported as flickering then it will look very blurry)
                                   : Dist(LinearToSRGBFast(old.rgb), LinearToSRGBFast(cur.rgb));
           difference=Sat(difference/FLICKER_EPS);

      new_flicker=Lerp(difference, old_flicker, OLD_FLICKER_WEIGHT)*old_weight; // always 0..1, alternative is "Lerp(difference, old_flicker*old_weight, OLD_WEIGHT)" however it makes sense to discard 'difference' too if old is being ignored

    //Half     flicker=Sat(LerpR(1-OLD_WEIGHT, 1, new_flicker)); // cut-off small differences
      Half     flicker=new_flicker; // faster
      Half not_flicker=1-Sqr(flicker); // use Sqr to consider only big flickering

      blend*=not_flicker; // reduce Lerp to 'cur' for flickering pixels
   }else
   {
      new_flicker=0; // always disable flickering for sky because on cam zoom in/out the sky motion vectors don't change, and it could retain some flicker from another object that wouldn't get cleared
   }

   old_weight*=1-blend;

#if !DUAL_HISTORY
#if SUPER
   VecI2 pix=pixel.xy; pix&=1;
   if(all(pix==TemporalCurPixel)) // this pixel has latest data
   {
      if(ANTI_ALIAS) // for anti-alias
      {
         old_weight*=OLD_WEIGHT; // smoothly blend with new
      }else
      {
         old_weight=0; // completely ignore old
      }
   }else
   {
      // try to keep old
   }
#else
   old_weight*=OLD_WEIGHT; // smoothly blend with new
#endif

   Half cur_weight=1-old_weight; // old_weight+cur_weight=1

   #if YCOCG && 0 // not needed, blend with RGB works similar or the same
              outCol=VecH4(YCoCg4ToRGB(ycocg_old*old_weight + ycocg_cur*cur_weight), old.a*old_weight + cur.a*cur_weight);
   #else
              outCol=old      *old_weight + cur      *cur_weight;
   #endif
   #if ALPHA
      Half new_alpha=old_alpha*old_weight + cur_alpha*cur_weight;
   #endif
   #if MERGED_ALPHA
      outData.x=new_alpha; // !! STORE ALPHA IN X CHANNEL SO IT CAN BE USED FOR '_alpha' RT !!
      outData.y=new_flicker;
   #else
      outData=new_flicker;
   #if ALPHA
      outAlpha=new_alpha;
   #endif
   #endif
 //if(T)outCol.r=Lerp(1, outCol.r, blend_move);
   if(SHOW_FLICKER)outCol.b=SRGBToLinearFast(new_flicker); // visualize flicker
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

   // #TemporalDualAlpha
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
