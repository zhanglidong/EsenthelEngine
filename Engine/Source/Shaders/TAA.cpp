/******************************************************************************/
#include "!Header.h"
/******************************************************************************
CLAMP, ALPHA, DUAL_HISTORY, GATHER

ALPHA=0
   ImgX=Weight
ALPHA=1
   ImgXY2=AlphaWeight, ImgX=CurAlpha
   
DUAL_HISTORY=1
   Img2=Old1

Img=Cur, Img1=Old, ImgXY=CurVel, ImgXY1=OldVel
/******************************************************************************/
#define CUR_WEIGHT (1.0/8)

#define YCOCG 0 // didn't improve quality much

#define NEAREST_DEPTH_VEL 1

#define VEL_EPS 0.003h

#define CUBIC 1
#if     CUBIC
#include "Cubic.h"
#endif

#if ALPHA
   #undef DUAL_HISTORY // #TAADualAlpha
#endif

#define MERGE_CUBIC_MIN_MAX 0 // Actually disable since it causes ghosting (visible when rotating camera around a character in the dungeons, perhaps range for MIN MAX can't be big), enable since this version is slightly better because: uses 12 tex reads (4*4 -4 corners), uses 12 samples for MIN/MAX which reduces flickering a bit, however has a lot more arithmetic calculations because of min/max x12 and each sample color is multiplied by weight separately

#define DUAL_ADJUST_OLD 0 // disable because didn't make any significant difference
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
	return Sat(Max(Min(min_step, max_step)));
}
Half GetBlend(VecH4 old, VecH4 cur, VecH4 col_min, VecH4 col_max)
{
	VecH4 dir=cur-old,
     inv_dir=rcp(dir),
	 min_step=(col_min-old)*inv_dir,
	 max_step=(col_max-old)*inv_dir;
	return Sat(Max(Min(min_step, max_step)));
}
/******************************************************************************/
void TestVel(VecH2 vel, VecH2 test_vel, in out Half old_weight)
{
   VecH2 delta_vel=vel-test_vel;
   delta_vel.x*=TAAAspectRatio; // 'delta_vel' is in UV 0..1 for both XY so mul X by aspect ratio
   if(Length2(delta_vel)>Sqr(VEL_EPS))old_weight=0;
}
void TestDepth(in out Flt depth, Flt d, in out VecI2 ofs, Int x, Int y)
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

   // NEAREST_DEPTH_VEL - get velocity for depth nearest to camera
   if(NEAREST_DEPTH_VEL) // !! TODO: Warning: this ignores CLAMP, if this is fixed then 'UVClamp' below for 'vel' can be removed !!
   {
   #if GATHER
      ofs=VecI2(-1, 1); Flt depth=TexDepthRawPointOfs(inTex, ofs         );              // -1,  1,  left-top
                 TestDepth(depth, TexDepthRawPointOfs(inTex, VecI2(1, -1)), ofs, 1, -1); //  1, -1, right-bottom
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
      Flt depth=TexDepthRawPoint(inTex);
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)if(x || y)TestDepth(depth, TexDepthRawPointOfs(inTex, VecI2(x, y)), ofs, x, y);
   #endif
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
   #if GATHER
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(-1,  1)).xy, old_weight); // -1,  1,  left-top
      TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2( 1, -1)).xy, old_weight); //  1, -1, right-bottom
      old_tex_vel-=ImgSize.xy*0.5; // move to center between -1,-1 and 0,0 texels
      VecH4 r=ImgXY1.GatherRed  (SamplerPoint, old_tex_vel); // get -1,-1 to 0,0 texels
      VecH4 g=ImgXY1.GatherGreen(SamplerPoint, old_tex_vel); // get -1,-1 to 0,0 texels
      TestVel(vel, VecH2(r.x, g.x), old_weight);
      TestVel(vel, VecH2(r.y, g.y), old_weight);
      TestVel(vel, VecH2(r.z, g.z), old_weight);
      TestVel(vel, VecH2(r.w, g.w), old_weight);
      r=ImgXY1.GatherRed  (SamplerPoint, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
      g=ImgXY1.GatherGreen(SamplerPoint, old_tex_vel, VecI2(1, 1)); // get 0,0 to 1,1 texels
      TestVel(vel, VecH2(r.x, g.x), old_weight);
      TestVel(vel, VecH2(r.y, g.y), old_weight);
      TestVel(vel, VecH2(r.z, g.z), old_weight);
    //TestVel(vel, VecH2(r.w, g.w), old_weight); already processed
   #else
      UNROLL for(Int y=-1; y<=1; y++)
      UNROLL for(Int x=-1; x<=1; x++)
         TestVel(vel, TexPointOfs(ImgXY1, old_tex_vel, VecI2(x, y)).xy, old_weight);
   #endif
#endif

 /* test current velocities, skip because didn't help
   UNROLL for(Int y=-1; y<=1; y++)
   UNROLL for(Int x=-1; x<=1; x++)if(x || y)
      TestVel(vel, TexPointOfs(ImgXY, inTex, VecI2(x, y)).xy, old_weight);*/

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
#else // this version uses 5 tex reads for CUBIC and 9 tex reads for MIN MAX (14 total), because it has only 9 samples for MIN MAX the flickering is a bit stronger
   #if CUBIC
      cur=Max(VecH4(0,0,0,0), cs.tex(Img)); // use Max(0) because of cubic sharpening potentially giving negative values
   #else
      cur=TexLod(Img, cur_tex);
   #endif

   // MIN MAX
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
         if(y==-1 && x==-1)
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
#endif

   // NEIGHBOR CLAMP
   if(YCOCG)
   {
                  old.rgb=RGBToYCoCg4(old.rgb);
      VecH4 ycocg_cur    =RGBToYCoCg4(cur);
      old=Lerp(old, ycocg_cur, GetBlend(old.rgb, ycocg_cur.rgb, ycocg_min.rgb, ycocg_max.rgb));
      old.rgb=YCoCg4ToRGB(old.rgb);
   }

#if 1 // alpha used for glow #RTOutput
   Half blend=GetBlend(old, cur, col_min, col_max);
#else
   Half blend=GetBlend(old.rgb, cur.rgb, col_min.rgb, col_max.rgb);
#endif

#if 1
   old=Lerp(old, cur, blend);
   #if ALPHA
      old_alpha=Lerp(old_alpha, cur_alpha, blend);
   #endif
#else // this increases flickering, since 'old_weight' gets smaller
   /* instead of adjusting 'old', adjust 'old_weight'
      (old*old_weight + cur*cur_weight)/total_weight
      (Lerp(old, cur, blend)*old_weight + cur*cur_weight)/total_weight
      ((old*(1-blend) + cur*blend)*old_weight + cur*cur_weight)/total_weight
      (old*(1-blend)*old_weight + cur*blend*old_weight + cur*cur_weight)/total_weight
      (old*(1-blend)*old_weight + cur*(blend*old_weight + cur_weight))/total_weight */
   old_weight*=(1-blend) // 'old_weight' gets smaller, multiplied by "1-blend"
              *CUR_WEIGHT/(CUR_WEIGHT+blend*old_weight); // 'cur_weight' gets bigger, starting from 'CUR_WEIGHT' adding "blend*old_weight" = "CUR_WEIGHT+blend*old_weight". However we want 'cur_weight' below to be always CUR_WEIGHT, so scale down both 'old_weight' and 'cur_weight' so that 'cur_weight' gets to be CUR_WEIGHT. scaling factor is just "CUR_WEIGHT/cur_weight" which is "CUR_WEIGHT/(CUR_WEIGHT+blend*old_weight)"
#endif

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
   Half cur_weight=CUR_WEIGHT/2, total_weight=old_weight+cur_weight;
   if(old_weight<0.5 - cur_weight/2) // fill 1st history RT
   {
      outWeight=total_weight;
      outOld1=outOld=outNext=old*(old_weight/total_weight) + cur*(cur_weight/total_weight);
   }else // fill 2nd history RT
   {
      outWeight=total_weight;
      if(DUAL_ADJUST_OLD)
      {
            Half ow=1, cw=CUR_WEIGHT, tw=ow+cw;
            outOld=old*(ow/tw) + cur*(cw/tw);
      }else outOld=old;

      Half old_weight1=old_weight-0.5, total=old_weight1+cur_weight;
      outOld1=old1*(old_weight1/total) + cur*(cur_weight/total);

      outNext=Lerp(outOld, outOld1, DUAL_ADJUST_OLD ? Sqr(total*2) : total*2);

      if(total>=0.5 - cur_weight/2) // filled all RT's
      {
         outOld=outOld1; // move Old1 to Old
         outWeight=0.5; // mark Old1 as empty
      }
   }
#endif
}
/******************************************************************************/
