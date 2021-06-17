/******************************************************************************/
#include "!Header.h"
/******************************************************************************
CLAMP, ALPHA, DUAL_HISTORY, GATHER, FILTER_MIN_MAX

Img=Cur, Img1=Old, ImgXY=CurVel, ImgXY1=OldVel

ALPHA=0
   ImgX=Weight
ALPHA=1
   ImgXY2=AlphaWeight, ImgX=CurAlpha
   
DUAL_HISTORY=1
   Img2=Old1
/******************************************************************************/
#define CUR_WEIGHT (1.0/8)

#define YCOCG 0 // didn't improve quality much

#define NEAREST_DEPTH_VEL 1

#define VEL_EPS 0.006

#define CUBIC 1
#if     CUBIC
   #include "Cubic.h"
#endif

#if ALPHA
   #undef DUAL_HISTORY // #TAADualAlpha
#endif

#define MERGE_CUBIC_MIN_MAX 0 // Actually disable since it causes ghosting (visible when rotating camera around a character in the dungeons, perhaps range for MIN MAX can't be big), enable since this version is slightly better because: uses 12 tex reads (4*4 -4 corners), uses 12 samples for MIN/MAX which reduces flickering a bit, however has a lot more arithmetic calculations because of min/max x12 and each sample color is multiplied by weight separately

#define DUAL_ADJUST_OLD 0 // disable because didn't make any significant difference (in some tests it increased flickering)
/******************************************************************************/
BUFFER(TAA)
   Vec2 TAAOffset,
        TAAOffsetCurToPrev;
   Half TAAAspectRatio;
BUFFER_END
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
VecH RGBToYCoCg4(VecH col) // faster but scales *4
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
VecH4 RGBToYCoCg4(VecH4 col) // faster but scales *4
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
Half GetBlend(VecH old, VecH cur, VecH col_min, VecH col_max)
{
   VecH dir=cur-old,
    inv_dir=rcp(dir),
   min_step=(col_min-old)*inv_dir,
   max_step=(col_max-old)*inv_dir;
   return Max(Min(min_step, max_step)); // don't 'Sat' to allow reporting high >1 differences (Warning: <0 can also be reported)
}
Half GetBlend(VecH4 old, VecH4 cur, VecH4 col_min, VecH4 col_max)
{
   VecH4 dir=cur-old,
     inv_dir=rcp(dir),
    min_step=(col_min-old)*inv_dir,
    max_step=(col_max-old)*inv_dir;
   return Max(Min(min_step, max_step)); // don't 'Sat' to allow reporting high >1 differences (Warning: <0 can also be reported)
}
/******************************************************************************/
void TestVel(VecH2 vel, VecH2 test_vel, inout Half max_delta_vel_len2)
{
   VecH2 delta_vel=vel-test_vel;
   delta_vel.x*=TAAAspectRatio; // 'delta_vel' is in UV 0..1 for both XY so mul X by aspect ratio
   Half delta_vel_len2=Length2(delta_vel);
   if(  delta_vel_len2>max_delta_vel_len2)max_delta_vel_len2=delta_vel_len2;
}
void TestDepth(inout Flt depth, Flt d, inout VecI2 ofs, Int x, Int y)
{
   if(DEPTH_SMALLER(d, depth)){depth=d; ofs.x=x; ofs.y=y;}
}
void TAA_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
          //NOPERSP Vec2 inPosXY:TEXCOORD1,
          //NOPERSP PIXEL                 ,
             #if ALPHA
                out VecH2 outWeight   :TARGET0,
             #else
                out Half  outWeight   :TARGET0,
             #endif
                out VecH4 outNext     :TARGET1,
                out VecH4 outOld      :TARGET2
             #if ALPHA && TAA_SEPARATE_ALPHA
              , out Half  outNextAlpha:TARGET3 // #TAADualAlpha
             #endif
             #if DUAL_HISTORY
              , out VecH4 outOld1     :TARGET3
             #endif
            )
{
   VecI2 ofs=0;

   Flt depth;
   // NEAREST_DEPTH_VEL - get velocity for depth nearest to camera
   if(NEAREST_DEPTH_VEL) // !! TODO: Warning: this ignores CLAMP, if this is fixed then 'UVClamp' below for 'vel' can be removed !!
   {
   #if GATHER
      ofs=VecI2(-1, 1); depth=TexDepthRawPointOfs(inTex, ofs         );              // -1,  1,  left-top
              TestDepth(depth,TexDepthRawPointOfs(inTex, VecI2(1, -1)), ofs, 1, -1); //  1, -1, right-bottom
      Vec2 tex=inTex-ImgSize.xy*0.5; // move to center between -1,-1 and 0,0 texels
      Vec4 d=TexDepthGather(tex); // get -1,-1 to 0,0 texels
      TestDepth(depth, d.x, ofs, -1,  0);
      TestDepth(depth, d.y, ofs,  0,  0);
      TestDepth(depth, d.z, ofs,  0, -1);
      TestDepth(depth, d.w, ofs, -1, -1);
      d=TexDepthGatherOfs(tex, VecI2(1, 1)); // get 0,0 to 1,1 texels
      TestDepth(depth, d.x, ofs,  0,  1);
      TestDepth(depth, d.y, ofs,  1,  1);
      TestDepth(depth, d.z, ofs,  1,  0);
    //TestDepth(depth, d.w, ofs,  0,  0); already processed
   #else
      depth=TexDepthRawPoint(inTex);
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)if(x || y)TestDepth(depth, TexDepthRawPointOfs(inTex, VecI2(x, y)), ofs, x, y);
   #endif
   }else
   {
      depth=TexDepthRawPoint(inTex);
   }

   // GET VEL
   VecH2 vel=TexPoint(ImgXY, UVClamp(inTex+ofs*ImgSize.xy, CLAMP)).xy;

   Vec2 cur_tex=inTex+TAAOffset,
        old_tex=inTex+vel;

   // OLD WEIGHT + OLD ALPHA
#if ALPHA
   VecH2 tex=TexLod(ImgXY2, old_tex).xy;
#else
   Half  tex=TexLod(ImgX  , old_tex).x;
#endif

#if !ALPHA
   Half old_weight=tex.x;
#else
   Half old_weight=tex.y, old_alpha=tex.x;
#endif

   // VIEWPORT TEST - if 'old_tex' is outside viewport then ignore it
   if(any(old_tex!=UVClamp(old_tex)))old_weight=0; // if(any(old_tex<ImgClamp.xy || old_tex>ImgClamp.zw))old_weight=0;

   // OLD VEL TEST
#if TAA_OLD_VEL // if old velocity is different then ignore old, !! TODO: Warning: this ignores CLAMP !!
   Vec2 old_tex_vel=old_tex+TAAOffsetCurToPrev;
   Half max_delta_vel_len2=0;
   #if GATHER
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(-1,  1)).xy, max_delta_vel_len2); // -1,  1,  left-top
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2( 1, -1)).xy, max_delta_vel_len2); //  1, -1, right-bottom
      old_tex_vel-=ImgSize.xy*0.5; // move to center between -1,-1 and 0,0 texels
      VecH4 r=ImgXY1.GatherRed  (SamplerPoint, old_tex_vel); // get -1,-1 to 0,0 texels
      VecH4 g=ImgXY1.GatherGreen(SamplerPoint, old_tex_vel); // get -1,-1 to 0,0 texels
      TestVel(vel, VecH2(r.x, g.x), max_delta_vel_len2);
      TestVel(vel, VecH2(r.y, g.y), max_delta_vel_len2);
      TestVel(vel, VecH2(r.z, g.z), max_delta_vel_len2);
      TestVel(vel, VecH2(r.w, g.w), max_delta_vel_len2);
      r=ImgXY1.GatherRed  (SamplerPoint, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
      g=ImgXY1.GatherGreen(SamplerPoint, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
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
      TestVel(vel, TexPointOfs(ImgXY, inTex, VecI2(x, y)).xy, max_delta_vel_len2);*/

   // OLD COLOR
#if CUBIC
      CubicFastSampler cs;
      cs.set(old_tex); if(CLAMP)cs.UVClamp(ImgClamp.xy, ImgClamp.zw); // here do clamping because for CUBIC we check many samples around texcoord
      VecH4 old =Max(VecH4(0,0,0,0), cs.tex(Img1)); // use Max(0) because of cubic sharpening potentially giving negative values
   #if DUAL_HISTORY
      VecH4 old1=Max(VecH4(0,0,0,0), cs.tex(Img2)); // use Max(0) because of cubic sharpening potentially giving negative values
   #endif
#else
   // clamping 'old_tex' shouldn't be done, because we already detect if 'old_tex' is outside viewport and zero 'old_weight'
      VecH4 old =TexLod(Img1, old_tex);
   #if DUAL_HISTORY
      VecH4 old1=TexLod(Img2, old_tex);
   #endif
#endif

   VecH4 col_min, col_max, cur;
   VecH  ycocg_min, ycocg_max;

   // CUR COLOR + CUR ALPHA
#if CUBIC
   cs.set(cur_tex); if(CLAMP)cs.UVClamp(ImgClamp.xy, ImgClamp.zw);
   #if ALPHA
      Half cur_alpha=Sat(cs.texX(ImgX)); // use Sat because of cubic sharpening potentially giving negative values
   #endif
#else
   if(CLAMP)cur_tex=UVClamp(cur_tex);
   #if ALPHA
      Half cur_alpha=TexLod(ImgX, cur_tex);
   #endif
#endif

#if CUBIC && MERGE_CUBIC_MIN_MAX // merged CUBIC with MIN MAX
   UNROLL for(Int y=0; y<4; y++)
   UNROLL for(Int x=0; x<4; x++)
      if((x!=0 && x!=3) || (y!=0 && y!=3)) // skip corners
   {
   #if !CLAMP
      VecH4 col=TexPointOfs(Img, cs.tc[0], VecI2(x, y));
   #else
      VecH4 col=TexPoint(Img, cs.uv(x, y));
   #endif
      VecH ycocg; if(YCOCG)ycocg=RGBToYCoCg4(col.rgb);
      Half weight=cs.weight(x, y);
      if(x==1 && y==0) // first is (1,0) because corners are skipped
      {
         cur=col*weight;
                    col_min=  col_max=col;
         if(YCOCG)ycocg_min=ycocg_max=ycocg;
      }else
      {
         cur+=col*weight;
         col_min=Min(col_min, col);
         col_max=Max(col_max, col);
         if(YCOCG)
         {
            ycocg_min=Min(ycocg_min, ycocg);
            ycocg_max=Max(ycocg_max, ycocg);
         }
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
      Vec2 uv[2];
   #if CLAMP
      uv[0]=Vec2(Max(inTex.x-ImgSize.x/2, ImgClamp.x), Max(inTex.y-ImgSize.y/2, ImgClamp.y));
      uv[1]=Vec2(Min(inTex.x+ImgSize.x/2, ImgClamp.z), Min(inTex.y+ImgSize.y/2, ImgClamp.w));
   #else
      uv[0]=inTex-ImgSize/2;
      uv[1]=inTex+ImgSize/2;
   #endif
      UNROLL for(Int y=0; y<=1; y++)
      UNROLL for(Int x=0; x<=1; x++)
      {
         VecH4 min=TexMin(Img, Vec2(uv[x].x, uv[y].y));
         VecH4 max=TexMax(Img, Vec2(uv[x].x, uv[y].y));
         VecH  ycocg_lo, ycocg_hi; if(YCOCG){ycocg_lo=RGBToYCoCg4(min.rgb); ycocg_hi=RGBToYCoCg4(max.rgb);}
         if(y==0 && x==0) // first
         {
                        col_min=     min;   col_max=     max;
            if(YCOCG){ycocg_min=ycocg_lo; ycocg_max=ycocg_hi;}
         }else
         {
            col_min=Min(col_min, min);
            col_max=Max(col_max, max);
            if(YCOCG)
            {
               ycocg_min=Min(ycocg_min, ycocg_lo);
               ycocg_max=Max(ycocg_max, ycocg_hi);
            }
         }
      }
   }
   #else // check all 3x3 samples individually
   {
   #if CLAMP
      Vec2 tex_clamp[3];
      tex_clamp[0]=Vec2(Max(inTex.x-ImgSize.x, ImgClamp.x), Max(inTex.y-ImgSize.y, ImgClamp.y)); tex_clamp[1]=inTex;
      tex_clamp[2]=Vec2(Min(inTex.x+ImgSize.x, ImgClamp.z), Min(inTex.y+ImgSize.y, ImgClamp.w));
   #endif
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
      {
         VecH4 col;
      #if !CLAMP
         col=TexPointOfs(Img, inTex, VecI2(x, y));
      #else
         col=TexPoint(Img, Vec2(tex_clamp[x+1].x, tex_clamp[y+1].y));
      #endif
         VecH ycocg; if(YCOCG)ycocg=RGBToYCoCg4(col.rgb);
         if(y==-1 && x==-1) // first
         {
                       col_min=  col_max=col;
            if(YCOCG)ycocg_min=ycocg_max=ycocg;
         }else
         {
            col_min=Min(col_min, col);
            col_max=Max(col_max, col);
            if(YCOCG)
            {
               ycocg_min=Min(ycocg_min, ycocg);
               ycocg_max=Max(ycocg_max, ycocg);
            }
         }
      }
   }
   #endif
#endif

   // NEIGHBOR CLAMP
   if(YCOCG)
   {
                  old.rgb=RGBToYCoCg4(old.rgb);
      VecH4 ycocg_cur    =RGBToYCoCg4(cur);
      old=Lerp(old, ycocg_cur, Sat(GetBlend(old.rgb, ycocg_cur.rgb, ycocg_min.rgb, ycocg_max.rgb)));
      old.rgb=YCoCg4ToRGB(old.rgb);
   }else
   {
   #if 1 // alpha used for glow #RTOutput
      Half blend=GetBlend(old, cur, col_min, col_max);
   #else
      Half blend=GetBlend(old.rgb, cur.rgb, col_min.rgb, col_max.rgb);
   #endif

      /*if(Q)
      {
         // to get average value closer to median, values should be Sqrt before adding, and Sqr after average:
         // Sqr ((Sqrt(2)+Sqrt(2)+Sqrt(2)+Sqrt(10))/4)=3.42
         //     ((     2 +     2 +     2 +     10 )/4)=4
         // Sqrt((Sqr (2)+Sqr (2)+Sqr (2)+Sqr (10))/4)=5.29
         Half dist_sum=0;
         UNROLL for(Int y=-1; y<=1; y++)
         UNROLL for(Int x=-1; x<=1; x++)if(x || y)
         {
            Vec2 ofs=Vec2(x, y)*ImgSize.xy;
            VecH cur=TexLod(Img , cur_tex+ofs).rgb;
            VecH old=TexLod(Img1, old_tex+ofs).rgb;
            if(E)
            {
               cur=LinearToSRGBFast(cur);
               old=LinearToSRGBFast(old);
            }
            Half dist=Dist(cur, old);
            if(W)dist=Sqrt(dist);
            dist_sum+=dist;
         }
         Half dist=dist_sum/(3*3-1);
         if(W)dist=Sqr(dist);

         //  low distance = move blend to 0
         // high distance = keep blend
         blend*=Sat(dist*X*100-Y*100);
      }else
      if(W)
      {
      }else
      if(E)
      {
         blend=0;
      }else*/
      if(DEPTH_FOREGROUND(depth)) // make sky unaffected by movement, because when cam zoom in/out or cam move then sky doesn't change, it only has some values when rotating camera. In tests it was better to use nearest 'depth' instead of depth at 'inTex'.
      {
         Half max_delta_vel_len=Sqrt(max_delta_vel_len2),
              blend_move=max_delta_vel_len/VEL_EPS;
         //Half blend_min=1.0/32; // make sure there's some blend even for static pixels, this will increase flickering but will help boost lighting changes and potential material/texture animations. 1/32 was chosen, 1/16 and 1/8 allowed faster changes but had bigger flickering.
         //blend*=blend_move+blend_min; // works better than "blend*=Sat(blend_move+blend_min);"
      #if !DUAL_HISTORY
         old_weight*=1-Sat(blend_move); // optional boost based on movement FIXME: broken for DUAL_HISTORY
      #endif
      }

      blend=Sat(blend);

   #if 1 // better
      old=Lerp(old, cur, blend);
      #if ALPHA
         old_alpha=Lerp(old_alpha, cur_alpha, blend);
      #endif
      #if DUAL_HISTORY
         old1=Lerp(old1, cur, blend);
      #endif
   #else // don't do this, because it will cause jittered ghosting (some pixels will look brighter and some look darker, depending on the color difference between old and new), above code works much better
      old_weight*=(1-blend); // 'old_weight' gets smaller, multiplied by "1-blend"
   #endif
   }

#if !DUAL_HISTORY
      Half cur_weight=CUR_WEIGHT, total_weight=old_weight+cur_weight;
      old_weight/=total_weight;
      cur_weight/=total_weight;
      outOld=outNext=old*old_weight + cur*cur_weight;
   #if !ALPHA
      outWeight=total_weight;
   #else
      outWeight.y=total_weight;
      outWeight.x=old_alpha*old_weight + cur_alpha*cur_weight; // !! store alpha in X so it can be used for '_alpha' RT !!
      #if TAA_SEPARATE_ALPHA
         outNextAlpha=outWeight.x;
      #endif
   #endif
#else
   // #TAADualAlpha
   // old_weight<0.5 means 'old' is being filled from 0 (empty) .. 0.5 (full) and 'old1' is empty, old_weight>0.5 means 'old' is full and 'old1' is being filled from 0.5 (empty) .. 1 (full)
   Half cur_weight=CUR_WEIGHT/2, // since we operate on 0 (empty) .. 0.5 (full) we need to make cur weight 2x smaller
      total_weight=old_weight+cur_weight;
   outWeight=total_weight;
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT (since 'old_weight' is stored in 8-bit RT, then it won't be exactly 0.5, so we must use some epsilon, the best choice is half of 'cur_weight' step which is "cur_weight/2")
   {
      outOld1=outOld=outNext=old*(old_weight/total_weight) + cur*(cur_weight/total_weight);
   }else // old_weight>0.5 = 1st history RT is full, fill 2nd history RT
   {
      if(DUAL_ADJUST_OLD)
      {
            Half ow=1, cw=CUR_WEIGHT, tw=ow+cw; // here we know 1st history RT is full, so we can use constants to make calculations faster
            outOld=old*(ow/tw) + cur*(cw/tw); // apply new color onto 'old'
      }else outOld=old;

      Half old_weight1=old_weight-0.5, // weight of the 2nd history RT calculated from 'old_weight', gives range 0 .. 0.5
         total_weight1=old_weight1+cur_weight; // 'total_weight1' means 'old_weight1' after applying new data, in range 0 .. 0.5
      outOld1=old1*(old_weight1/total_weight1) + cur*(cur_weight/total_weight1); // apply new color onto 'old1'

      outNext=Lerp(outOld, outOld1, DUAL_ADJUST_OLD ? Sqr(total_weight1*2) : total_weight1*2); // 'old1' is more recent, but may not be fully set yet, so use it based on its weight and remaining values take from 'old'. If we're adjusting DUAL_ADJUST_OLD then 'old' gets updated with latest color, so we can make 'old1' less significant. *2 because here range is 0 .. 0.5

      if(total_weight1>=0.5 - cur_weight/2) // filled all history RT's (1st history RT is full, 2nd history RT is full)
      {
         outOld=outOld1; // move Old1 to Old
         outWeight=0.5; // mark Old1 as empty
      }
   }
#endif
}
/******************************************************************************/
