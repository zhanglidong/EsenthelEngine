/******************************************************************************

   For AO shader, Depth is linearized to 0 .. Viewport.range

   TODO: the 2d version "!GEOM" operates on 'nrm2' which assumes that depth changes are linear when iterating though pixels,
      however most likely they're not. (it's possible that deltas are linear when using the raw "Delinearized" depth buffer)
   3D version 'GEOM' should be used, however it doesn't work with flipped normals.

/******************************************************************************/
#include "!Header.h"
#include "Ambient Occlusion.h"
/******************************************************************************/
#define AO0Elms 12
#define AO1Elms 28
#define AO2Elms 48
#define AO3Elms 80

// input: MODE, JITTER, NORMALS
#define LINEAR_FILTER 1 // this removes some vertical lines on distant terrain (because multiple samples are clamped together), however introduces extra shadowing under distant objects
#define GEOM (NORMALS && 0) // this is an alternative mode to AO formula which works on 3D space instead of 2D, currently disabled, because has some unresolved issues, doesn't work with flipped normals (leafs/grass), probably would require storing flipped information in Nrm RT W channel, which is currently used for specular
/******************************************************************************/
BUFFER(AOConstants) // z=1/xy.length()
   Vec AO0Vec[]={Vec(-0.707, -0.707, 1.000), Vec(0.000, -0.707, 1.414), Vec(0.707, -0.707, 1.000), Vec(-0.354, -0.354, 1.997), Vec(0.354, -0.354, 1.997), Vec(-0.707, 0.000, 1.414), Vec(0.707, 0.000, 1.414), Vec(-0.354, 0.354, 1.997), Vec(0.354, 0.354, 1.997), Vec(-0.707, 0.707, 1.000), Vec(0.000, 0.707, 1.414), Vec(0.707, 0.707, 1.000)};
   Vec AO1Vec[]={Vec(0.000, -0.943, 1.060), Vec(-0.707, -0.707, 1.000), Vec(-0.236, -0.707, 1.342), Vec(0.236, -0.707, 1.342), Vec(0.707, -0.707, 1.000), Vec(-0.471, -0.471, 1.501), Vec(0.000, -0.471, 2.123), Vec(0.471, -0.471, 1.501), Vec(-0.707, -0.236, 1.342), Vec(-0.236, -0.236, 2.996), Vec(0.236, -0.236, 2.996), Vec(0.707, -0.236, 1.342), Vec(-0.943, 0.000, 1.060), Vec(-0.471, 0.000, 2.123), Vec(0.471, 0.000, 2.123), Vec(0.943, 0.000, 1.060), Vec(-0.707, 0.236, 1.342), Vec(-0.236, 0.236, 2.996), Vec(0.236, 0.236, 2.996), Vec(0.707, 0.236, 1.342), Vec(-0.471, 0.471, 1.501), Vec(0.000, 0.471, 2.123), Vec(0.471, 0.471, 1.501), Vec(-0.707, 0.707, 1.000), Vec(-0.236, 0.707, 1.342), Vec(0.236, 0.707, 1.342), Vec(0.707, 0.707, 1.000), Vec(0.000, 0.943, 1.060)};
   Vec AO2Vec[]={Vec(-0.177, -0.884, 1.109), Vec(0.177, -0.884, 1.109), Vec(-0.707, -0.707, 1.000), Vec(-0.354, -0.707, 1.265), Vec(0.000, -0.707, 1.414), Vec(0.354, -0.707, 1.265), Vec(0.707, -0.707, 1.000), Vec(-0.530, -0.530, 1.334), Vec(-0.177, -0.530, 1.790), Vec(0.177, -0.530, 1.790), Vec(0.530, -0.530, 1.334), Vec(-0.707, -0.354, 1.265), Vec(-0.354, -0.354, 1.997), Vec(0.000, -0.354, 2.825), Vec(0.354, -0.354, 1.997), Vec(0.707, -0.354, 1.265), Vec(-0.884, -0.177, 1.109), Vec(-0.530, -0.177, 1.790), Vec(-0.177, -0.177, 3.995), Vec(0.177, -0.177, 3.995), Vec(0.530, -0.177, 1.790), Vec(0.884, -0.177, 1.109), Vec(-0.707, 0.000, 1.414), Vec(-0.354, 0.000, 2.825), Vec(0.354, 0.000, 2.825), Vec(0.707, 0.000, 1.414), Vec(-0.884, 0.177, 1.109), Vec(-0.530, 0.177, 1.790), Vec(-0.177, 0.177, 3.995), Vec(0.177, 0.177, 3.995), Vec(0.530, 0.177, 1.790), Vec(0.884, 0.177, 1.109), Vec(-0.707, 0.354, 1.265), Vec(-0.354, 0.354, 1.997), Vec(0.000, 0.354, 2.825), Vec(0.354, 0.354, 1.997), Vec(0.707, 0.354, 1.265), Vec(-0.530, 0.530, 1.334), Vec(-0.177, 0.530, 1.790), Vec(0.177, 0.530, 1.790), Vec(0.530, 0.530, 1.334), Vec(-0.707, 0.707, 1.000), Vec(-0.354, 0.707, 1.265), Vec(0.000, 0.707, 1.414), Vec(0.354, 0.707, 1.265), Vec(0.707, 0.707, 1.000), Vec(-0.177, 0.884, 1.109), Vec(0.177, 0.884, 1.109)};
   Vec AO3Vec[]={Vec(-0.141, -0.990, 1.000), Vec(0.141, -0.990, 1.000), Vec(-0.283, -0.849, 1.117), Vec(0.000, -0.849, 1.178), Vec(0.283, -0.849, 1.117), Vec(-0.707, -0.707, 1.000), Vec(-0.424, -0.707, 1.213), Vec(-0.141, -0.707, 1.387), Vec(0.141, -0.707, 1.387), Vec(0.424, -0.707, 1.213), Vec(0.707, -0.707, 1.000), Vec(-0.566, -0.566, 1.249), Vec(-0.283, -0.566, 1.580), Vec(0.000, -0.566, 1.767), Vec(0.283, -0.566, 1.580), Vec(0.566, -0.566, 1.249), Vec(-0.707, -0.424, 1.213), Vec(-0.424, -0.424, 1.668), Vec(-0.141, -0.424, 2.238), Vec(0.141, -0.424, 2.238), Vec(0.424, -0.424, 1.668), Vec(0.707, -0.424, 1.213), Vec(-0.849, -0.283, 1.117), Vec(-0.566, -0.283, 1.580), Vec(-0.283, -0.283, 2.499), Vec(0.000, -0.283, 3.534), Vec(0.283, -0.283, 2.499), Vec(0.566, -0.283, 1.580), Vec(0.849, -0.283, 1.117), Vec(-0.990, -0.141, 1.000), Vec(-0.707, -0.141, 1.387), Vec(-0.424, -0.141, 2.238), Vec(-0.141, -0.141, 5.015), Vec(0.141, -0.141, 5.015), Vec(0.424, -0.141, 2.238), Vec(0.707, -0.141, 1.387), Vec(0.990, -0.141, 1.000), Vec(-0.849, 0.000, 1.178), Vec(-0.566, 0.000, 1.767), Vec(-0.283, 0.000, 3.534), Vec(0.283, 0.000, 3.534), Vec(0.566, 0.000, 1.767), Vec(0.849, 0.000, 1.178), Vec(-0.990, 0.141, 1.000), Vec(-0.707, 0.141, 1.387), Vec(-0.424, 0.141, 2.238), Vec(-0.141, 0.141, 5.015), Vec(0.141, 0.141, 5.015), Vec(0.424, 0.141, 2.238), Vec(0.707, 0.141, 1.387), Vec(0.990, 0.141, 1.000), Vec(-0.849, 0.283, 1.117), Vec(-0.566, 0.283, 1.580), Vec(-0.283, 0.283, 2.499), Vec(0.000, 0.283, 3.534), Vec(0.283, 0.283, 2.499), Vec(0.566, 0.283, 1.580), Vec(0.849, 0.283, 1.117), Vec(-0.707, 0.424, 1.213), Vec(-0.424, 0.424, 1.668), Vec(-0.141, 0.424, 2.238), Vec(0.141, 0.424, 2.238), Vec(0.424, 0.424, 1.668), Vec(0.707, 0.424, 1.213), Vec(-0.566, 0.566, 1.249), Vec(-0.283, 0.566, 1.580), Vec(0.000, 0.566, 1.767), Vec(0.283, 0.566, 1.580), Vec(0.566, 0.566, 1.249), Vec(-0.707, 0.707, 1.000), Vec(-0.424, 0.707, 1.213), Vec(-0.141, 0.707, 1.387), Vec(0.141, 0.707, 1.387), Vec(0.424, 0.707, 1.213), Vec(0.707, 0.707, 1.000), Vec(-0.283, 0.849, 1.117), Vec(0.000, 0.849, 1.178), Vec(0.283, 0.849, 1.117), Vec(-0.141, 0.990, 1.000), Vec(0.141, 0.990, 1.000)};
BUFFER_END
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
void AO_VS( VtxInput vtx,
NOPERSP out Vec2 outTex   :TEXCOORD0,
NOPERSP out Vec2 outPosXY :TEXCOORD1,
NOPERSP out Vec2 outPosXY1:TEXCOORD2,
NOPERSP out Vec4 outVtx   :POSITION )
{
   outTex   =vtx.tex();
   outPosXY =ScreenToPosXY(outTex);
   outPosXY1=ScreenToPosXY(outTex+RTSize.xy);
   outVtx   =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
// Img=Nrm, Depth=depth
Half AO_PS(NOPERSP Vec2 inTex   :TEXCOORD ,
           NOPERSP Vec2 inPosXY :TEXCOORD1,
           NOPERSP Vec2 inPosXY1:TEXCOORD2,
           NOPERSP PIXEL                  ):TARGET
{
   Vec2 nrm2;
   Vec  nrm, pos;

   if(NORMALS)
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
      
      if(GEOM)
      {
         nrm=Normalize(nrm);
      }else
      {
         // 'nrm' does not need to be normalized, because following codes don't require that

      #if 0
         Vec dir_right=Normalize(Vec(ScreenToPosXY(inTex+Vec2(RTSize.x, 0)), 1)), // get view space direction that points slightly to the right of 'pos'
             dir_up   =Normalize(Vec(ScreenToPosXY(inTex+Vec2(0, RTSize.y)), 1)); // get view space direction that points slightly to the top   of 'pos'

         Vec pr=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_right), // get intersection point when casting ray from view space camera (0,0,0) to 'pos,nrm' plane using 'dir_right' ray
             pu=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_up   ); // get intersection point when casting ray from view space camera (0,0,0) to 'pos,nrm' plane using 'dir_up'    ray

         nrm2=Vec2(pr.z-pos.z, pu.z-pos.z) // this gives the expected delta between depths (for right-center, and top-center pixels)
             *RTSize.zw; // have to scale because we will multiply 'nrm2' by 'offs' below, and it operates on texture coordinates, so the next pixel uses 'RTSize.xy' offset, however we want 1 offset, so scale it by '1/RTSize.xy' which is 'RTSize.zw'
      #else // optimized
         Vec dir_right=Normalize(Vec(inPosXY1.x, inPosXY .y, 1)),
             dir_up   =Normalize(Vec(inPosXY .x, inPosXY1.y, 1));

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
   if(JITTER)
   {
      Flt a    =Dot(pixel.xy, Vec2(0.5, 0.25)),
          angle=2.0/3; // good results were obtained with 0.666(2/3), 0.8, 1.0, however using smaller value increases performance because it affects texture cache
      a=Frac(a)*angle - 0.5*angle; // (Frac(a)-0.5)*angle;
      CosSin(cos_sin.x, cos_sin.y, a);
   }

   Flt  occl  =0,
        weight=HALF_MIN;
   Vec2 offs_scale=Viewport.size_fov_tan*(0.5*AmbRange/Max(1.0f, pos.z)); // use 0.5 because we're converting from -1..1 to 0..1 scale

#if 0 // don't pack too many samples together, require them to be spread out, don't use for now, because forces big occlusion on distant objects
   this doesn't work with AmbBias already scaled by AmbRange, would have to make AmbBiasChangeable
   Flt pixels=Min(offs_scale*RTSize.zw);
   Flt AmbRangeChangeable=AmbRange;
   #define AmbRange AmbRangeChangeable
   Flt min_pixels;
   if(MODE==0)min_pixels=1/(0.707-0.354);else
   if(MODE==1)min_pixels=1/(0.943-0.707);else
   if(MODE==2)min_pixels=1/(0.884-0.707);else
              min_pixels=1/(0.990-0.849);
   if(W && 0)if(pixels<min_pixels)
   {
      Flt scale=min_pixels/pixels;
      AmbRange*=scale;
      offs_scale*=scale;
   }
#endif

   // add some distance based bias, which reduces flickering for distant pixels
   { // must be in delinearized space, which will not affect much pixels close to camera, or when we have high precision depth (high fov, high view from).
      pos.z=DelinearizeDepth(pos.z); DEPTH_DEC(pos.z, 0.00000007); // value tested on fov 20 deg, 1000 view range
      pos.z=  LinearizeDepth(pos.z);
   }

   // required for later optimizations
   Flt range2, scale;
   if(GEOM)
   {
      range2=Sqr(AmbRange);
   }else
   {
   #if 1 // Optimized
      scale=1/AmbRange;
      pos.z-=AmbBias;
   #endif
   }

   Int        elms;
   if(MODE==0)elms=AO0Elms;else
   if(MODE==1)elms=AO1Elms;else
   if(MODE==2)elms=AO2Elms;else
              elms=AO3Elms;
 //using UNROLL didn't make a performance difference, however it made shader file bigger and compilation slower
   LOOP for(Int i=0; i<elms; i++)
   {
      Vec        pattern;
      if(MODE==0)pattern=AO0Vec[i];else
      if(MODE==1)pattern=AO1Vec[i];else
      if(MODE==2)pattern=AO2Vec[i];else
                 pattern=AO3Vec[i];

      Vec2              offs=pattern.xy*offs_scale; // don't use 'VecH2' here because benefit looks small, and 'offs' has to be added to 'inTex' and multiplied by 'nrm2' which are 'Vec2' so probably there would be no performance benefits
      if(JITTER        )offs=Rotate(offs, cos_sin);
      if(!LINEAR_FILTER)offs=Round(offs*RTSize.zw)*RTSize.xy;

      Vec2 t=inTex+offs;
      Flt  o, w;

      if(all(Abs(t-Viewport.center)<=Viewport.size/2)) // UV inside viewport
      {
         // !! for AO shader depth is already linearized !!
         Flt test_z=(LINEAR_FILTER ? TexDepthRawLinear(t) : TexDepthRawPoint(t)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 't'
         if(GEOM)
         {
            test_z+=AmbBias;
            Vec test_pos=GetPos(test_z, ScreenToPosXY(t)),
                delta=test_pos-pos;
            if(Dot(delta, nrm)>0)
            {
                  w=(Length2(delta)<=range2);
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
               }

               o=Sat(Dot(delta, nrm)/AmbBias-(E?0:1));*/
            }else
            {
               w=1;
               o=0;
            }
         }else
         {
         #if 1 // Optimized
            Flt expected=pos.z+Dot(nrm2, offs); if(test_z<expected)
            {
               w=BlendSqr((pos.z-test_z)*scale);
         #else // Unoptimized
            Flt expected=pos.z+Dot(nrm2, offs); test_z+=AmbBias; if(test_z<expected)
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
         o=0; w=0.5; // set as brightening but use small weight
      }

      w     *=pattern.z; // focus on samples near to the center
      occl  +=w*o;
      weight+=w;
   }
   return 1-AmbContrast*Half(occl/weight); // result is stored in One Channel 1 Byte RT so it doesn't need 'Sat' saturation
}
/******************************************************************************/
