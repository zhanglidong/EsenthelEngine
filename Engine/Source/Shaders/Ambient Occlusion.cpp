/******************************************************************************

   For AO shader, Depth is linearized to 0 .. Viewport.range

/******************************************************************************/
#include "!Header.h"
#include "Ambient Occlusion.h"
/******************************************************************************/
#define AO0Elms 12
#define AO1Elms 28
#define AO2Elms 48
#define AO3Elms 80
/******************************************************************************/
BUFFER(AOConstants) // z=1/xy.length()
   Vec AO0Vec[]={Vec(-0.707, -0.707, 1.000), Vec(0.000, -0.707, 1.414), Vec(0.707, -0.707, 1.000), Vec(-0.354, -0.354, 1.997), Vec(0.354, -0.354, 1.997), Vec(-0.707, 0.000, 1.414), Vec(0.707, 0.000, 1.414), Vec(-0.354, 0.354, 1.997), Vec(0.354, 0.354, 1.997), Vec(-0.707, 0.707, 1.000), Vec(0.000, 0.707, 1.414), Vec(0.707, 0.707, 1.000)};
   Vec AO1Vec[]={Vec(0.000, -0.943, 1.060), Vec(-0.707, -0.707, 1.000), Vec(-0.236, -0.707, 1.342), Vec(0.236, -0.707, 1.342), Vec(0.707, -0.707, 1.000), Vec(-0.471, -0.471, 1.501), Vec(0.000, -0.471, 2.123), Vec(0.471, -0.471, 1.501), Vec(-0.707, -0.236, 1.342), Vec(-0.236, -0.236, 2.996), Vec(0.236, -0.236, 2.996), Vec(0.707, -0.236, 1.342), Vec(-0.943, 0.000, 1.060), Vec(-0.471, 0.000, 2.123), Vec(0.471, 0.000, 2.123), Vec(0.943, 0.000, 1.060), Vec(-0.707, 0.236, 1.342), Vec(-0.236, 0.236, 2.996), Vec(0.236, 0.236, 2.996), Vec(0.707, 0.236, 1.342), Vec(-0.471, 0.471, 1.501), Vec(0.000, 0.471, 2.123), Vec(0.471, 0.471, 1.501), Vec(-0.707, 0.707, 1.000), Vec(-0.236, 0.707, 1.342), Vec(0.236, 0.707, 1.342), Vec(0.707, 0.707, 1.000), Vec(0.000, 0.943, 1.060)};
   Vec AO2Vec[]={Vec(-0.177, -0.884, 1.109), Vec(0.177, -0.884, 1.109), Vec(-0.707, -0.707, 1.000), Vec(-0.354, -0.707, 1.265), Vec(0.000, -0.707, 1.414), Vec(0.354, -0.707, 1.265), Vec(0.707, -0.707, 1.000), Vec(-0.530, -0.530, 1.334), Vec(-0.177, -0.530, 1.790), Vec(0.177, -0.530, 1.790), Vec(0.530, -0.530, 1.334), Vec(-0.707, -0.354, 1.265), Vec(-0.354, -0.354, 1.997), Vec(0.000, -0.354, 2.825), Vec(0.354, -0.354, 1.997), Vec(0.707, -0.354, 1.265), Vec(-0.884, -0.177, 1.109), Vec(-0.530, -0.177, 1.790), Vec(-0.177, -0.177, 3.995), Vec(0.177, -0.177, 3.995), Vec(0.530, -0.177, 1.790), Vec(0.884, -0.177, 1.109), Vec(-0.707, 0.000, 1.414), Vec(-0.354, 0.000, 2.825), Vec(0.354, 0.000, 2.825), Vec(0.707, 0.000, 1.414), Vec(-0.884, 0.177, 1.109), Vec(-0.530, 0.177, 1.790), Vec(-0.177, 0.177, 3.995), Vec(0.177, 0.177, 3.995), Vec(0.530, 0.177, 1.790), Vec(0.884, 0.177, 1.109), Vec(-0.707, 0.354, 1.265), Vec(-0.354, 0.354, 1.997), Vec(0.000, 0.354, 2.825), Vec(0.354, 0.354, 1.997), Vec(0.707, 0.354, 1.265), Vec(-0.530, 0.530, 1.334), Vec(-0.177, 0.530, 1.790), Vec(0.177, 0.530, 1.790), Vec(0.530, 0.530, 1.334), Vec(-0.707, 0.707, 1.000), Vec(-0.354, 0.707, 1.265), Vec(0.000, 0.707, 1.414), Vec(0.354, 0.707, 1.265), Vec(0.707, 0.707, 1.000), Vec(-0.177, 0.884, 1.109), Vec(0.177, 0.884, 1.109)};
   Vec AO3Vec[]={Vec(-0.141, -0.990, 1.000), Vec(0.141, -0.990, 1.000), Vec(-0.283, -0.849, 1.117), Vec(0.000, -0.849, 1.178), Vec(0.283, -0.849, 1.117), Vec(-0.707, -0.707, 1.000), Vec(-0.424, -0.707, 1.213), Vec(-0.141, -0.707, 1.387), Vec(0.141, -0.707, 1.387), Vec(0.424, -0.707, 1.213), Vec(0.707, -0.707, 1.000), Vec(-0.566, -0.566, 1.249), Vec(-0.283, -0.566, 1.580), Vec(0.000, -0.566, 1.767), Vec(0.283, -0.566, 1.580), Vec(0.566, -0.566, 1.249), Vec(-0.707, -0.424, 1.213), Vec(-0.424, -0.424, 1.668), Vec(-0.141, -0.424, 2.238), Vec(0.141, -0.424, 2.238), Vec(0.424, -0.424, 1.668), Vec(0.707, -0.424, 1.213), Vec(-0.849, -0.283, 1.117), Vec(-0.566, -0.283, 1.580), Vec(-0.283, -0.283, 2.499), Vec(0.000, -0.283, 3.534), Vec(0.283, -0.283, 2.499), Vec(0.566, -0.283, 1.580), Vec(0.849, -0.283, 1.117), Vec(-0.990, -0.141, 1.000), Vec(-0.707, -0.141, 1.387), Vec(-0.424, -0.141, 2.238), Vec(-0.141, -0.141, 5.015), Vec(0.141, -0.141, 5.015), Vec(0.424, -0.141, 2.238), Vec(0.707, -0.141, 1.387), Vec(0.990, -0.141, 1.000), Vec(-0.849, 0.000, 1.178), Vec(-0.566, 0.000, 1.767), Vec(-0.283, 0.000, 3.534), Vec(0.283, 0.000, 3.534), Vec(0.566, 0.000, 1.767), Vec(0.849, 0.000, 1.178), Vec(-0.990, 0.141, 1.000), Vec(-0.707, 0.141, 1.387), Vec(-0.424, 0.141, 2.238), Vec(-0.141, 0.141, 5.015), Vec(0.141, 0.141, 5.015), Vec(0.424, 0.141, 2.238), Vec(0.707, 0.141, 1.387), Vec(0.990, 0.141, 1.000), Vec(-0.849, 0.283, 1.117), Vec(-0.566, 0.283, 1.580), Vec(-0.283, 0.283, 2.499), Vec(0.000, 0.283, 3.534), Vec(0.283, 0.283, 2.499), Vec(0.566, 0.283, 1.580), Vec(0.849, 0.283, 1.117), Vec(-0.707, 0.424, 1.213), Vec(-0.424, 0.424, 1.668), Vec(-0.141, 0.424, 2.238), Vec(0.141, 0.424, 2.238), Vec(0.424, 0.424, 1.668), Vec(0.707, 0.424, 1.213), Vec(-0.566, 0.566, 1.249), Vec(-0.283, 0.566, 1.580), Vec(0.000, 0.566, 1.767), Vec(0.283, 0.566, 1.580), Vec(0.566, 0.566, 1.249), Vec(-0.707, 0.707, 1.000), Vec(-0.424, 0.707, 1.213), Vec(-0.141, 0.707, 1.387), Vec(0.141, 0.707, 1.387), Vec(0.424, 0.707, 1.213), Vec(0.707, 0.707, 1.000), Vec(-0.283, 0.849, 1.117), Vec(0.000, 0.849, 1.178), Vec(0.283, 0.849, 1.117), Vec(-0.141, 0.990, 1.000), Vec(0.141, 0.990, 1.000)};
BUFFER_END
/******************************************************************************/
Flt Q,W,E,R; // FIXME
// can use 'RTSize' instead of 'ImgSize' since there's no scale
// Img=Nrm, Depth=depth
Half AO_PS(NOPERSP Vec2 inTex  :TEXCOORD ,
           NOPERSP Vec2 inPosXY:TEXCOORD1,
           NOPERSP PIXEL                 ,
   uniform Int  mode                     ,
   uniform Bool jitter                   ,
   uniform Bool normals                  ):COLOR
{
   Bool geom=(normals && Q); // FIXME

   Vec2 nrm2;
   Vec  nrm, pos;

   if(normals)
   {
      pos=GetPos(TexDepthRawPoint(inTex), inPosXY); // !! for AO shader depth is already linearized !!
   #if 1 // sharp normal, looks better
      nrm=TexLod(Img, inTex).xyz; // use filtering because 'Img' may be bigger, especially important for pixels in the distance (there are some cases however when point filtering improves quality, although not always)
      #if !SIGNED_NRM_RT
         nrm-=0.5; // normally it should be "nrm=nrm*2-1", however since the normal doesn't need to be normalized, we can just do -0.5
      #endif
   #else // smoothened normal, less detail
      Flt zl=TexDepthRawPoint(inTex+RTSize.xy*Vec2(-1, 0)),
          zr=TexDepthRawPoint(inTex+RTSize.xy*Vec2( 1, 0)),
          zd=TexDepthRawPoint(inTex+RTSize.xy*Vec2( 0,-1)),
          zu=TexDepthRawPoint(inTex+RTSize.xy*Vec2( 0, 1)),
          dl=z-zl, dr=zr-z, adr=Abs(dr), adl=Abs(dl),
          dd=z-zd, du=zu-z, adu=Abs(du), add=Abs(dd);

      Vec2 smooth_ru=Vec2(Sat(1-adr*2), Sat(1-adu*2)),
           smooth_ld=Vec2(Sat(1-adl*2), Sat(1-add*2));

      // read two normals from texture, convert to -1..1 scale and average them
      // ((n1*2-1) + (n2*2-1))/2 = n1+n2-1
      nrm=TexLod(Img, inTex+RTSize.xy*0.5*smooth_ru).xyz  // apply 0.5 scale so we get a smooth texel from 4 values using bilinear filtering
         +TexLod(Img, inTex-RTSize.xy*0.5*smooth_ld).xyz; // apply 0.5 scale so we get a smooth texel from 4 values using bilinear filtering
      #if !SIGNED_NRM_RT
         nrm-=2*0.5; // same as above (0.5) but for 2 Tex reads
      #endif
   #endif
      
      if(geom)
      {
         nrm=Normalize(nrm);
      }else
      {
         // 'nrm' does not need to be normalized, because following codes don't require that

         Vec dir_right=Normalize(Vec(ScreenToPosXY(inTex+Vec2(RTSize.x, 0)), 1)),
             dir_up   =Normalize(Vec(ScreenToPosXY(inTex+Vec2(0, RTSize.y)), 1));
      #if 0
         Vec pr=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_right),
             pu=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_up   );

         nrm2=Vec2(pr.z-pos.z, pu.z-pos.z)*RTSize.zw;
      #else // optimized
         Flt pr_z=dir_right.z*Dot(pos, nrm)/Dot(dir_right, nrm),
             pu_z=dir_up   .z*Dot(pos, nrm)/Dot(dir_up   , nrm);

         nrm2=Vec2(pr_z-pos.z, pu_z-pos.z)*RTSize.zw;
      #endif
      }
   }else
   {
      // !! for AO shader depth is already linearized !!
      pos.z =TexDepthRawPoint(inTex);
      Flt zl=TexDepthRawPoint(inTex-Vec2(RTSize.x, 0)),
          zr=TexDepthRawPoint(inTex+Vec2(RTSize.x, 0)),
          zd=TexDepthRawPoint(inTex-Vec2(0, RTSize.y)),
          zu=TexDepthRawPoint(inTex+Vec2(0, RTSize.y)),
          dl=pos.z-zl, dr=zr-pos.z,
          dd=pos.z-zd, du=zu-pos.z;

      nrm2=Vec2((Abs(dl)<Abs(dr)) ? dl : dr,
                (Abs(dd)<Abs(du)) ? dd : du)*RTSize.zw;
   }

   Vec2 cos_sin;
   if(jitter)
   {
      Flt a    =Dot(pixel.xy, Vec2(0.5, 0.25)),
          angle=0.8;
      a=Frac(a)*angle - 0.5*angle; // (Frac(a)-0.5)*angle;
      CosSin(cos_sin.x, cos_sin.y, a);
   }

   Flt  occl  =0,
        weight=EPS;
   Vec2 offs_scale=Viewport.size_fov_tan*(0.5*AmbRange/Max(1.0, pos.z)); // use 0.5 because we're converting from -1..1 to 0..1 scale

   Flt pixels=Min(offs_scale*RTSize.zw);
   Flt rrr=AmbRange;//, max_length=1;
   #define AmbRange rrr
   Flt min_pixels;
   if(mode==0)min_pixels=1/(0.707-0.354);else
   if(mode==1)min_pixels=1/(0.943-0.707);else
   if(mode==2)min_pixels=1/(0.884-0.707);else
              min_pixels=1/(0.990-0.849);
   if(W && 0)if(pixels<min_pixels)
   {
      Flt scale=min_pixels/pixels;
      AmbRange*=scale;
      offs_scale*=scale;
    //max_length/=scale;
   }

   Flt bias=AmbBias*AmbRange;
   if(!geom)bias+=pos.z/(1024/* *Viewport.size_fov_tan.y*/); // add some distance based bias, which reduces flickering for distant pixels, the formula should depend on size_fov_tan, however when using it, then we're getting artifacts in low fov and high distance, so have to keep it disabled

   // required for later optimizations
   Flt range2, scale;
   if(geom)
   {
      range2=Sqr(AmbRange);
   }else
   {
   #if 1
      scale=1/AmbRange;
      nrm2*=scale;
      pos.z=(pos.z-bias)*scale;
   #endif
   }

   Int        elms;
   if(mode==0)elms=AO0Elms;else
   if(mode==1)elms=AO1Elms;else
   if(mode==2)elms=AO2Elms;else
              elms=AO3Elms;
   // FIXME UNROLL?
   UNROLL for(Int i=0; i<elms; i++)
   {
      Vec        pattern;
      if(mode==0)pattern=AO0Vec[i];else
      if(mode==1)pattern=AO1Vec[i];else
      if(mode==2)pattern=AO2Vec[i];else
                 pattern=AO3Vec[i];

      //if(1/pattern.z>max_length)continue; not working well

      Vec2      offs=pattern.xy*offs_scale;
      if(jitter)offs=Rotate(offs, cos_sin);
                offs=Round (offs*RTSize.zw)*RTSize.xy; // doesn't make a big difference for pixels close to camera, but makes a HUGE difference for pixels far away, keep !! otherwise distant terrain gets unnaturally shaded

      Vec2 t=inTex+offs;
      Flt  o, w;

      if(all(Abs(t-Viewport.center)<=Viewport.size/2)) // UV inside viewport
      {
         // !! for AO shader depth is already linearized !!
         Flt test_z=TexDepthRawPoint(t); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 't'
         if(geom)
         {
            test_z+=bias;
            Vec test_pos=GetPos(test_z, ScreenToPosXY(t)),
                delta=test_pos-pos;
            if(Dot(delta, nrm)>0)
            {
                  w=(Length2(delta)<=range2);
                  //if(W)w=Sat(1-Length2(delta)/range2);
                  //if(E)w*=BlendSqr(Dot(delta, nrm)/AmbRange);
                  //if(!E && R && !W)w=(Length(delta)<=AmbRange+bias);
                  o=1;
                  if(w<=0)
                  {
                     w=1.0/3;
                     o=0;
                  }
               /*if(W)
               {
                  Flt f=Length(delta)/AmbRange;
                  //if(E)w=Sat(1-Length2(delta)/range2);else
                  w=Sat(1.5-Sqr(f));
                  o=1;
               }else
               {
                  w=(Length2(delta)<=(W ? 2 : 1.0)*range2);
                  o=1;
               }*/

               //if(E)
               //if(R)
               //o=Sat(Dot(delta, nrm)/bias-(E?0:1));
            }else
            {
               w=1;
               o=0;
            }
         }else
         {
         #if 1 // Optimized
            Flt expected=pos.z+Dot(nrm2, offs); test_z*=scale; if(test_z<expected)
            {
               w=BlendSqr(pos.z-test_z);
         #else // Unoptimized
            Flt expected=pos.z+Dot(nrm2, offs); test_z+=AmbBias*AmbRange; if(test_z<expected)
            {
               w=BlendSqr((pos.z-test_z)/AmbRange);
         #endif
               o=1;
               if(w<=0)
               {
                  w=1.0/3;
                  o=0;
               }
            }else
            {
               w=1;
               o=0;
            }
         }
      }else // UV outside viewport
      {
         o=0; w=0.5; // set as brightening but only half of weight
      }

      w     *=pattern.z; // focus on samples near to the center
      occl  +=w*o;
      weight+=w;
   }
   return 1-AmbContrast*occl/weight; // result is stored in One Channel 1 Byte RT so it doesn't need 'Sat' saturation
}
/******************************************************************************/
/*FIXME
TECHNIQUE(AO0  , DrawPosXY_VS(), AO_PS(0, false, false));
TECHNIQUE(AO1  , DrawPosXY_VS(), AO_PS(1, false, false));
TECHNIQUE(AO2  , DrawPosXY_VS(), AO_PS(2, false, false));
TECHNIQUE(AO3  , DrawPosXY_VS(), AO_PS(3, false, false));
TECHNIQUE(AO0J , DrawPosXY_VS(), AO_PS(0, true , false));
TECHNIQUE(AO1J , DrawPosXY_VS(), AO_PS(1, true , false));
TECHNIQUE(AO2J , DrawPosXY_VS(), AO_PS(2, true , false));
TECHNIQUE(AO3J , DrawPosXY_VS(), AO_PS(3, true , false));
TECHNIQUE(AO0N , DrawPosXY_VS(), AO_PS(0, false, true ));
TECHNIQUE(AO1N , DrawPosXY_VS(), AO_PS(1, false, true ));
TECHNIQUE(AO2N , DrawPosXY_VS(), AO_PS(2, false, true ));
TECHNIQUE(AO3N , DrawPosXY_VS(), AO_PS(3, false, true ));*/
TECHNIQUE(AO0JN, DrawPosXY_VS(), AO_PS(0, true , true ));
TECHNIQUE(AO1JN, DrawPosXY_VS(), AO_PS(1, true , true ));
TECHNIQUE(AO2JN, DrawPosXY_VS(), AO_PS(2, true , true ));
TECHNIQUE(AO3JN, DrawPosXY_VS(), AO_PS(3, true , true ));
/******************************************************************************/
