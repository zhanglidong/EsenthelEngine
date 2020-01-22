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

// input: MODE, JITTER, NORMALS
#ifndef NORMALS
#define NORMALS 1
#endif

#define LINEAR_FILTER 1 // this removes some vertical lines on distant terrain (because multiple samples are clamped together), however introduces extra shadowing under distant objects
#define GEOM 1 // this is an alternative mode to AO formula which works on 3D space instead of 2D
#define PRECISION 0 // 1=operate on delinearized depth which will give a little more precise position calculations for expected depth, disable beacuse not much noticable
//#define THICKNESS 0.05 // assume all pixels are at least 5 cm thick, slightly improves quality
/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(AOConstants) // z=1/xy.length()
   Vec AO0Vec[]={Vec(-0.707, -0.707, 1.000), Vec(0.000, -0.707, 1.414), Vec(0.707, -0.707, 1.000), Vec(-0.354, -0.354, 1.997), Vec(0.354, -0.354, 1.997), Vec(-0.707, 0.000, 1.414), Vec(0.707, 0.000, 1.414), Vec(-0.354, 0.354, 1.997), Vec(0.354, 0.354, 1.997), Vec(-0.707, 0.707, 1.000), Vec(0.000, 0.707, 1.414), Vec(0.707, 0.707, 1.000)};
   Vec AO1Vec[]={Vec(0.000, -0.943, 1.060), Vec(-0.707, -0.707, 1.000), Vec(-0.236, -0.707, 1.342), Vec(0.236, -0.707, 1.342), Vec(0.707, -0.707, 1.000), Vec(-0.471, -0.471, 1.501), Vec(0.000, -0.471, 2.123), Vec(0.471, -0.471, 1.501), Vec(-0.707, -0.236, 1.342), Vec(-0.236, -0.236, 2.996), Vec(0.236, -0.236, 2.996), Vec(0.707, -0.236, 1.342), Vec(-0.943, 0.000, 1.060), Vec(-0.471, 0.000, 2.123), Vec(0.471, 0.000, 2.123), Vec(0.943, 0.000, 1.060), Vec(-0.707, 0.236, 1.342), Vec(-0.236, 0.236, 2.996), Vec(0.236, 0.236, 2.996), Vec(0.707, 0.236, 1.342), Vec(-0.471, 0.471, 1.501), Vec(0.000, 0.471, 2.123), Vec(0.471, 0.471, 1.501), Vec(-0.707, 0.707, 1.000), Vec(-0.236, 0.707, 1.342), Vec(0.236, 0.707, 1.342), Vec(0.707, 0.707, 1.000), Vec(0.000, 0.943, 1.060)};
   Vec AO2Vec[]={Vec(-0.177, -0.884, 1.109), Vec(0.177, -0.884, 1.109), Vec(-0.707, -0.707, 1.000), Vec(-0.354, -0.707, 1.265), Vec(0.000, -0.707, 1.414), Vec(0.354, -0.707, 1.265), Vec(0.707, -0.707, 1.000), Vec(-0.530, -0.530, 1.334), Vec(-0.177, -0.530, 1.790), Vec(0.177, -0.530, 1.790), Vec(0.530, -0.530, 1.334), Vec(-0.707, -0.354, 1.265), Vec(-0.354, -0.354, 1.997), Vec(0.000, -0.354, 2.825), Vec(0.354, -0.354, 1.997), Vec(0.707, -0.354, 1.265), Vec(-0.884, -0.177, 1.109), Vec(-0.530, -0.177, 1.790), Vec(-0.177, -0.177, 3.995), Vec(0.177, -0.177, 3.995), Vec(0.530, -0.177, 1.790), Vec(0.884, -0.177, 1.109), Vec(-0.707, 0.000, 1.414), Vec(-0.354, 0.000, 2.825), Vec(0.354, 0.000, 2.825), Vec(0.707, 0.000, 1.414), Vec(-0.884, 0.177, 1.109), Vec(-0.530, 0.177, 1.790), Vec(-0.177, 0.177, 3.995), Vec(0.177, 0.177, 3.995), Vec(0.530, 0.177, 1.790), Vec(0.884, 0.177, 1.109), Vec(-0.707, 0.354, 1.265), Vec(-0.354, 0.354, 1.997), Vec(0.000, 0.354, 2.825), Vec(0.354, 0.354, 1.997), Vec(0.707, 0.354, 1.265), Vec(-0.530, 0.530, 1.334), Vec(-0.177, 0.530, 1.790), Vec(0.177, 0.530, 1.790), Vec(0.530, 0.530, 1.334), Vec(-0.707, 0.707, 1.000), Vec(-0.354, 0.707, 1.265), Vec(0.000, 0.707, 1.414), Vec(0.354, 0.707, 1.265), Vec(0.707, 0.707, 1.000), Vec(-0.177, 0.884, 1.109), Vec(0.177, 0.884, 1.109)};
   Vec AO3Vec[]={Vec(-0.141, -0.990, 1.000), Vec(0.141, -0.990, 1.000), Vec(-0.283, -0.849, 1.117), Vec(0.000, -0.849, 1.178), Vec(0.283, -0.849, 1.117), Vec(-0.707, -0.707, 1.000), Vec(-0.424, -0.707, 1.213), Vec(-0.141, -0.707, 1.387), Vec(0.141, -0.707, 1.387), Vec(0.424, -0.707, 1.213), Vec(0.707, -0.707, 1.000), Vec(-0.566, -0.566, 1.249), Vec(-0.283, -0.566, 1.580), Vec(0.000, -0.566, 1.767), Vec(0.283, -0.566, 1.580), Vec(0.566, -0.566, 1.249), Vec(-0.707, -0.424, 1.213), Vec(-0.424, -0.424, 1.668), Vec(-0.141, -0.424, 2.238), Vec(0.141, -0.424, 2.238), Vec(0.424, -0.424, 1.668), Vec(0.707, -0.424, 1.213), Vec(-0.849, -0.283, 1.117), Vec(-0.566, -0.283, 1.580), Vec(-0.283, -0.283, 2.499), Vec(0.000, -0.283, 3.534), Vec(0.283, -0.283, 2.499), Vec(0.566, -0.283, 1.580), Vec(0.849, -0.283, 1.117), Vec(-0.990, -0.141, 1.000), Vec(-0.707, -0.141, 1.387), Vec(-0.424, -0.141, 2.238), Vec(-0.141, -0.141, 5.015), Vec(0.141, -0.141, 5.015), Vec(0.424, -0.141, 2.238), Vec(0.707, -0.141, 1.387), Vec(0.990, -0.141, 1.000), Vec(-0.849, 0.000, 1.178), Vec(-0.566, 0.000, 1.767), Vec(-0.283, 0.000, 3.534), Vec(0.283, 0.000, 3.534), Vec(0.566, 0.000, 1.767), Vec(0.849, 0.000, 1.178), Vec(-0.990, 0.141, 1.000), Vec(-0.707, 0.141, 1.387), Vec(-0.424, 0.141, 2.238), Vec(-0.141, 0.141, 5.015), Vec(0.141, 0.141, 5.015), Vec(0.424, 0.141, 2.238), Vec(0.707, 0.141, 1.387), Vec(0.990, 0.141, 1.000), Vec(-0.849, 0.283, 1.117), Vec(-0.566, 0.283, 1.580), Vec(-0.283, 0.283, 2.499), Vec(0.000, 0.283, 3.534), Vec(0.283, 0.283, 2.499), Vec(0.566, 0.283, 1.580), Vec(0.849, 0.283, 1.117), Vec(-0.707, 0.424, 1.213), Vec(-0.424, 0.424, 1.668), Vec(-0.141, 0.424, 2.238), Vec(0.141, 0.424, 2.238), Vec(0.424, 0.424, 1.668), Vec(0.707, 0.424, 1.213), Vec(-0.566, 0.566, 1.249), Vec(-0.283, 0.566, 1.580), Vec(0.000, 0.566, 1.767), Vec(0.283, 0.566, 1.580), Vec(0.566, 0.566, 1.249), Vec(-0.707, 0.707, 1.000), Vec(-0.424, 0.707, 1.213), Vec(-0.141, 0.707, 1.387), Vec(0.141, 0.707, 1.387), Vec(0.424, 0.707, 1.213), Vec(0.707, 0.707, 1.000), Vec(-0.283, 0.849, 1.117), Vec(0.000, 0.849, 1.178), Vec(0.283, 0.849, 1.117), Vec(-0.141, 0.990, 1.000), Vec(0.141, 0.990, 1.000)};
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************
static Flt ObstacleDotToLight(Flt dot) // calculate amount of received light based on obstacle dot product
{
#if 0 // test method
   Flt  angle=Asin(dot); // angle relative to Vec2(1,0)
   Dbl  v=0, total=0;
   Vec2 n(0, 1);
   const Int res=1024; REP(res+1) // iterate light samples
   {
      Flt  a=i/Flt(res)*PI_2; // angle of this light sample
      Vec2 cs(Cos(a), Sin(a)); // position of this light sample
      Flt  l, d=Dot(cs, n); l=d; // light amount (based on angle-dot), d == cs.y
      total+=l; // total light of all samples (needed for normalization to 0..1 range)
      // if sample is above obstacle, then contribute its light to received light, all 3 methods below are the same:
      if(d   >=dot  )v+=l;
    //if(cs.y>=dot  )v+=l;
    //if(a   >=angle)v+=l;
   }
   return total ? v/total : 0;
#else // optimized result based on test method
   return CosSin(dot);
#endif
}
Occlusion = 1-Light
/******************************************************************************/
// can use 'RTSize' instead of 'ImgSize' since there's no scale
void AO_VS
(
   VtxInput vtx,
   NOPERSP out Vec2 outTex   :TEXCOORD0,
   NOPERSP out Vec2 outPosXY :TEXCOORD1,
#if (NORMALS && !GEOM) || (!NORMALS && GEOM)
   NOPERSP out Vec2 outPosXY1:TEXCOORD2,
#endif
   NOPERSP out Vec4 outVtx   :POSITION
)
{
   outTex   =vtx.tex();
   outPosXY =UVToPosXY(outTex);
#if (NORMALS && !GEOM) || (!NORMALS && GEOM)
   outPosXY1=UVToPosXY(outTex+RTSize.xy);
#endif
   outVtx   =Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
}

/*#define NUM_DIRECTIONS 16
#define NUM_STEPS      12
Vec FetchEyePos(Vec2 UV) {return GetPos(TexDepthRawLinear(UV), UVToPosXY(UV));}
Flt ComputeAO(Vec P, Vec N, Vec S)
{
   Vec delta = S - P;
   Flt len2=Length2(delta);
   Flt NdotV = dot(N, delta) * rsqrt(len2);
   return Sat(NdotV) * Sat(2 - len2 * AmbientRangeInvSqr);
}
Half HBAO(Vec2 uv, Vec nrm, Vec pos, Vec2 g_fRadiusToScreen)
{
   Flt AO=0;
   for(Int DirectionIndex=0; DirectionIndex<NUM_DIRECTIONS; DirectionIndex++)
   {
      Flt  Angle=DirectionIndex*(PI2/NUM_DIRECTIONS);
      Vec2 dir  =Vec2(cos(Angle), sin(Angle))*g_fRadiusToScreen/NUM_STEPS;
      for(Int StepIndex=0; StepIndex<NUM_STEPS; StepIndex++)
      {
         Vec S=FetchEyePos(uv + dir * (StepIndex+1));
         AO+=ComputeAO(pos, nrm, S);
      }
   }
   AO/=(NUM_DIRECTIONS*NUM_STEPS);
   return 1-AO*2;
}*/

// Img=Nrm, Depth=depth
Half AO_PS
(
   NOPERSP Vec2 inTex   :TEXCOORD ,
   NOPERSP Vec2 inPosXY :TEXCOORD1,
#if (NORMALS && !GEOM) || (!NORMALS && GEOM)
   NOPERSP Vec2 inPosXY1:TEXCOORD2,
#endif
   NOPERSP PIXEL
):TARGET
{
   Vec2 nrm2;
   Vec  nrm, pos;

   if(GEOM)pos  =GetPos(TexDepthRawPoint(inTex), inPosXY); // !! for AO shader depth is already linearized !!
   else    pos.z=       TexDepthRawPoint(inTex);

   #if NORMALS
   {
      nrm=TexLod(Img, inTex).xyz; // use filtering because 'Img' may be bigger, especially important for pixels in the distance (there are some cases however when point filtering improves quality, although not always)
   #if !SIGNED_NRM_RT
      nrm-=0.5; // normally it should be "nrm=nrm*2-1", however since the normal doesn't need to be normalized, we can just do -0.5
   #endif
      
      #if !GEOM
      {
         // 'nrm' does not need to be normalized, because following codes don't require that
      #if 0
         Vec dir_right=Normalize(Vec(UVToPosXY(inTex+Vec2(RTSize.x, 0)), 1)), // get view space direction that points slightly to the right of 'pos'
             dir_up   =Normalize(Vec(UVToPosXY(inTex+Vec2(0, RTSize.y)), 1)); // get view space direction that points slightly to the top   of 'pos'

         Vec pr=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_right), // get intersection point when casting ray from view space camera (0,0,0) to 'pos,nrm' plane using 'dir_right' ray
             pu=PointOnPlaneRay(Vec(0, 0, 0), pos, nrm, dir_up   ); // get intersection point when casting ray from view space camera (0,0,0) to 'pos,nrm' plane using 'dir_up'    ray

         nrm2=Vec2(pr.z-pos.z, pu.z-pos.z); // this gives the expected delta between depths (for right-center, and top-center pixels)
      #else // optimized
         Vec dir_right=Normalize(Vec(inPosXY1.x, inPosXY .y, 1)),
             dir_up   =Normalize(Vec(inPosXY .x, inPosXY1.y, 1));

         Flt pr_z=dir_right.z*Dot(pos, nrm)/Dot(dir_right, nrm),
             pu_z=dir_up   .z*Dot(pos, nrm)/Dot(dir_up   , nrm);

         nrm2=Vec2(pr_z-pos.z, pu_z-pos.z);
      #endif
      }
      #endif
   }
   #else // NORMALS
   {
      // !! for AO shader depth is already linearized !!
      Flt zl=TexDepthRawPoint(inTex-Vec2(RTSize.x, 0)),
          zr=TexDepthRawPoint(inTex+Vec2(RTSize.x, 0)),
          zd=TexDepthRawPoint(inTex-Vec2(0, RTSize.y)),
          zu=TexDepthRawPoint(inTex+Vec2(0, RTSize.y)),
          dl=pos.z-zl, dr=zr-pos.z,
          dd=pos.z-zd, du=zu-pos.z;

      #if GEOM
      {
         #if 0 // made no difference
            Vec up=((Abs(dd)<Abs(du)) ? pos-GetPos(zd, Vec2(inPosXY.x                   , UVToPosXY(inTex-RTSize.xy).y)) : GetPos(zu, Vec2(inPosXY .x, inPosXY1.y))-pos),
                rg=((Abs(dl)<Abs(dr)) ? pos-GetPos(zl, Vec2(UVToPosXY(inTex-RTSize.xy).x, inPosXY.y                   )) : GetPos(zr, Vec2(inPosXY1.x, inPosXY .y))-pos);
            nrm=Cross(rg, up);
         #else
            Vec up=GetPos((Abs(dd)<Abs(du)) ? pos.z+dd : zu, Vec2(inPosXY .x, inPosXY1.y)),
                rg=GetPos((Abs(dl)<Abs(dr)) ? pos.z+dl : zr, Vec2(inPosXY1.x, inPosXY .y));
            nrm=Cross(rg-pos, up-pos);
         #endif
      }
      #else
      {
         nrm2=Vec2((Abs(dl)<Abs(dr)) ? dl : dr,
                   (Abs(dd)<Abs(du)) ? dd : du);
      }
      #endif
   }
   #endif // NORMALS

   // required for later optimizations
   #if GEOM
   {
      nrm=Normalize(nrm);
      pos.z=DelinearizeDepth(pos.z);
      DEPTH_DEC(pos.z, 0.00000007); // value tested on fov 20 deg, 1000 view range
      pos.z=LinearizeDepth(pos.z); // convert back to linear
   }
   #else
   Flt pos_z_bias, pos_w_bias;
   {
      pos_z_bias=pos.z-AmbientBias;
      // add some distance based bias, which reduces flickering for distant pixels
      // must be in delinearized space, which will not affect much pixels close to camera, or when we have high precision depth (high fov, high view from).
      pos_w_bias=DelinearizeDepth(pos_z_bias);
      if(PRECISION)
      {
         Flt pos_wx=DelinearizeDepth(pos_z_bias+nrm2.x);
         Flt pos_wy=DelinearizeDepth(pos_z_bias+nrm2.y);
         nrm2=Vec2(pos_wx-pos_w_bias, pos_wy-pos_w_bias);
      }
      DEPTH_DEC(pos_w_bias, 0.00000007); // value tested on fov 20 deg, 1000 view range
      if(!PRECISION)pos_z_bias=LinearizeDepth(pos_w_bias); // convert back to linear
      nrm2*=RTSize.zw; // have to scale because we will multiply 'nrm2' by 'offs' below, and it operates on texture coordinates, so the next pixel uses 'RTSize.xy' offset, however we want 1 offset, so scale it by '1/RTSize.xy' which is 'RTSize.zw'
   }
   #endif

   Vec2 cos_sin;
   if(JITTER)
   {
      Flt a    =Dot(pixel.xy, Vec2(0.5, 0.25)),
          angle=2.0/3; // good results were obtained with 0.666(2/3), 0.8, 1.0, however using smaller value increases performance because it affects texture cache
      a=Frac(a)*angle - 0.5*angle; // (Frac(a)-0.5)*angle;
      CosSin(cos_sin.x, cos_sin.y, a);
   }

   Flt  occl  =0,
        weight=0;
   Vec2 offs_scale=Viewport.size_fov_tan*(AmbientRange_2/Max(1.0f, pos.z)); // use /2 because we're converting from -1..1 to 0..1 scale

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
         #if GEOM
         {
            Vec test_pos=GetPos(test_z, UVToPosXY(t)),
                delta   =test_pos-pos;

         #ifdef THICKNESS
            Vec d=delta; if(d.z<0)d.z=Min(0, d.z+THICKNESS); // move pixels in front closer to center, to simulate thickness
            w=Sat(2-Length2(d)*AmbientRangeInvSqr); // alternative "Length2(delta)<=AmbientRangeSqr"
         #else
            w=Sat(2-Length2(delta)*AmbientRangeInvSqr); // alternative "Length2(delta)<=AmbientRangeSqr"
         #endif

            Flt y=Dot(delta, nrm); if(y>0)
            {
               Flt dot=y/Length(delta);
            #if 1 // precise, calculated based on 'ObstacleDotToLight'
               o=1-CosSin(dot);
            #else // faster approximation, requires AmbientContrast to be 1.25 smaller
               o=Sqr(dot);
            #endif
               o*=w; // fix artifacts (occlusion can be strong only as weight)
            }else o=0;
            w=Max((delta.z>0) ? 1 : 0.25, w); // fix artifacts, fully brighten if test sample is behind us, but if in front then brighten only a little. this increases weight if it's small, which results in brightening because we don't touch occlusion
         }
         #else
         {
         #if 1 // Optimized
            Flt expected=(PRECISION ? pos_w_bias : pos_z_bias)+Dot(nrm2, offs); if(PRECISION)expected=LinearizeDepth(expected); if(test_z<expected)
            {
               w=BlendSqr((pos.z-test_z)/AmbientRange);
         #else // Unoptimized
            Flt expected=pos.z-AmbientBias+Dot(nrm2, offs); if(test_z<expected)
            {
               w=BlendSqr((pos.z-test_z)/AmbientRange);
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
         #endif
      }else // UV outside viewport
      {
         o=0; w=0.25; // set as brightening but use small weight
      }

    //w     *=pattern.z; // focus on samples near to the center
      occl  +=w*o;
      weight+=w;
   }
   return Max(AmbientMin, 1-AmbientContrast*Half(occl/weight));
}
/******************************************************************************/
