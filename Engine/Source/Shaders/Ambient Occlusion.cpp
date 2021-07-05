/******************************************************************************

   For AO shader, Depth is linearized to 0 .. Viewport.range

Input: QUALITY, JITTER, NORMALS
Img=Nrm, Depth=depth
/******************************************************************************/
#include "!Header.h"
#include "Ambient Occlusion.h"
/******************************************************************************/
#ifndef NORMALS
#define NORMALS 1
#endif

#define TEMPORAL 0 // this would require using TAA AO RT's, old, new, and process velocities
//Flt AmbientTemporalAngle, AmbientTemporalStep;

#define LINEAR_FILTER 0

#define AO_MAX     0
#define AO_AVG     1
#define AO_PATTERN 2

#define AO_MODE AO_AVG
/******************************************************************************/
#if AO_MODE==AO_PATTERN
#include "!Set Prec Struct.h"
BUFFER(AOConstants) // z=1/xy.length()
   Vec2 AO0Vec[]={Vec2(0.000, -0.750), Vec2(-0.650, -0.375), Vec2(-0.125, -0.217), Vec2(0.125, -0.217), Vec2(0.650, -0.375), Vec2(-0.250, 0.000), Vec2(0.250, 0.000), Vec2(-0.650, 0.375), Vec2(-0.125, 0.217), Vec2(0.125, 0.217), Vec2(0.650, 0.375), Vec2(0.000, 0.750)};
   Vec2 AO1Vec[]={Vec2(-0.147, -0.764), Vec2(0.147, -0.764), Vec2(-0.588, -0.509), Vec2(-0.222, -0.385), Vec2(0.000, -0.333), Vec2(0.222, -0.385), Vec2(0.588, -0.509), Vec2(-0.735, -0.255), Vec2(-0.289, -0.167), Vec2(-0.056, -0.096), Vec2(0.056, -0.096), Vec2(0.289, -0.167), Vec2(0.735, -0.255), Vec2(-0.444, 0.000), Vec2(-0.111, 0.000), Vec2(0.111, 0.000), Vec2(0.444, 0.000), Vec2(-0.735, 0.255), Vec2(-0.289, 0.167), Vec2(-0.056, 0.096), Vec2(0.056, 0.096), Vec2(0.289, 0.167), Vec2(0.735, 0.255), Vec2(-0.588, 0.509), Vec2(-0.222, 0.385), Vec2(0.000, 0.333), Vec2(0.222, 0.385), Vec2(0.588, 0.509), Vec2(-0.147, 0.764), Vec2(0.147, 0.764)};
   Vec2 AO2Vec[]={Vec2(-0.225, -0.781), Vec2(0.000, -0.750), Vec2(0.225, -0.781), Vec2(-0.563, -0.585), Vec2(-0.281, -0.487), Vec2(-0.083, -0.430), Vec2(0.083, -0.430), Vec2(0.281, -0.487), Vec2(0.563, -0.585), Vec2(-0.650, -0.375), Vec2(-0.331, -0.286), Vec2(-0.125, -0.217), Vec2(0.000, -0.188), Vec2(0.125, -0.217), Vec2(0.331, -0.286), Vec2(0.650, -0.375), Vec2(-0.789, -0.195), Vec2(-0.413, -0.143), Vec2(-0.162, -0.094), Vec2(-0.031, -0.054), Vec2(0.031, -0.054), Vec2(0.162, -0.094), Vec2(0.413, -0.143), Vec2(0.789, -0.195), Vec2(-0.563, 0.000), Vec2(-0.250, 0.000), Vec2(-0.063, 0.000), Vec2(0.063, 0.000), Vec2(0.250, 0.000), Vec2(0.563, 0.000), Vec2(-0.789, 0.195), Vec2(-0.413, 0.143), Vec2(-0.162, 0.094), Vec2(-0.031, 0.054), Vec2(0.031, 0.054), Vec2(0.162, 0.094), Vec2(0.413, 0.143), Vec2(0.789, 0.195), Vec2(-0.650, 0.375), Vec2(-0.331, 0.286), Vec2(-0.125, 0.217), Vec2(0.000, 0.188), Vec2(0.125, 0.217), Vec2(0.331, 0.286), Vec2(0.650, 0.375), Vec2(-0.563, 0.585), Vec2(-0.281, 0.487), Vec2(-0.083, 0.430), Vec2(0.083, 0.430), Vec2(0.281, 0.487), Vec2(0.563, 0.585), Vec2(-0.225, 0.781), Vec2(0.000, 0.750), Vec2(0.225, 0.781)};
   Vec2 AO3Vec[]={Vec2(-0.275, -0.794), Vec2(-0.087, -0.755), Vec2(0.087, -0.755), Vec2(0.275, -0.794), Vec2(-0.550, -0.635), Vec2(-0.320, -0.554), Vec2(-0.144, -0.500), Vec2(0.000, -0.480), Vec2(0.144, -0.500), Vec2(0.320, -0.554), Vec2(0.550, -0.635), Vec2(-0.610, -0.453), Vec2(-0.361, -0.375), Vec2(-0.180, -0.312), Vec2(-0.053, -0.275), Vec2(0.053, -0.275), Vec2(0.180, -0.312), Vec2(0.361, -0.375), Vec2(0.610, -0.453), Vec2(-0.697, -0.302), Vec2(-0.416, -0.240), Vec2(-0.212, -0.183), Vec2(-0.080, -0.139), Vec2(0.000, -0.120), Vec2(0.080, -0.139), Vec2(0.212, -0.183), Vec2(0.416, -0.240), Vec2(0.697, -0.302), Vec2(-0.825, -0.159), Vec2(-0.505, -0.125), Vec2(-0.265, -0.092), Vec2(-0.104, -0.060), Vec2(-0.020, -0.035), Vec2(0.020, -0.035), Vec2(0.104, -0.060), Vec2(0.265, -0.092), Vec2(0.505, -0.125), Vec2(0.825, -0.159), Vec2(-0.640, 0.000), Vec2(-0.360, 0.000), Vec2(-0.160, 0.000), Vec2(-0.040, 0.000), Vec2(0.040, 0.000), Vec2(0.160, 0.000), Vec2(0.360, 0.000), Vec2(0.640, 0.000), Vec2(-0.825, 0.159), Vec2(-0.505, 0.125), Vec2(-0.265, 0.092), Vec2(-0.104, 0.060), Vec2(-0.020, 0.035), Vec2(0.020, 0.035), Vec2(0.104, 0.060), Vec2(0.265, 0.092), Vec2(0.505, 0.125), Vec2(0.825, 0.159), Vec2(-0.697, 0.302), Vec2(-0.416, 0.240), Vec2(-0.212, 0.183), Vec2(-0.080, 0.139), Vec2(0.000, 0.120), Vec2(0.080, 0.139), Vec2(0.212, 0.183), Vec2(0.416, 0.240), Vec2(0.697, 0.302), Vec2(-0.610, 0.453), Vec2(-0.361, 0.375), Vec2(-0.180, 0.312), Vec2(-0.053, 0.275), Vec2(0.053, 0.275), Vec2(0.180, 0.312), Vec2(0.361, 0.375), Vec2(0.610, 0.453), Vec2(-0.550, 0.635), Vec2(-0.320, 0.554), Vec2(-0.144, 0.500), Vec2(0.000, 0.480), Vec2(0.144, 0.500), Vec2(0.320, 0.554), Vec2(0.550, 0.635), Vec2(-0.275, 0.794), Vec2(-0.087, 0.755), Vec2(0.087, 0.755), Vec2(0.275, 0.794)};

   #define AO0Elms 12
   #define AO1Elms 30
   #define AO2Elms 54
   #define AO3Elms 84

   #define AO0Spacing 0.500
   #define AO1Spacing 0.333
   #define AO2Spacing 0.250
   #define AO3Spacing 0.200
BUFFER_END
#include "!Set Prec Default.h"
#endif
/******************************************************************************
static Flt ObstacleSinToLight(Flt sin) // calculate amount of received light based on obstacle sine
{
#if 0 // test method
   Flt  angle=Asin(sin); // angle relative to Vec2(1,0)
   Dbl  v=0, total=0;
   Vec2 n(0, 1);
   const Int res=1024; REP(res+1) // iterate light samples
   {
      Flt  a=i/Flt(res)*PI_2; // angle of this light sample
      Vec2 cs(Cos(a), Sin(a)); // position of this light sample
      Flt  l, d=Dot(cs, n); l=d; // light amount (based on angle-dot), d == cs.y == Sin(a)
      total+=l; // total light of all samples (needed for normalization to 0..1 range)
      // if sample is above obstacle, then contribute its light to received light, all 3 methods below are the same:
      if(d   >sin  )v+=l;
    //if(cs.y>sin  )v+=l;
    //if(a   >angle)v+=l;
   }
   return total ? v/total : 0;
#else // optimized result based on test method
   return CosSin(sin);
 //return   Acos(sin)/PI_2; // if using 'l=1'
 //return 1-Asin(sin)/PI_2; // if using 'l=1'
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
#if !NORMALS
   NOPERSP out Vec2 outPosXY1:TEXCOORD2,
#endif
   NOPERSP out Vec4 outVtx   :POSITION
)
{
   outTex   =vtx.tex();
   outPosXY =UVToPosXY(outTex);
#if !NORMALS
   outPosXY1=UVToPosXY(outTex+RTSize.xy);
#endif
   outVtx   =Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
}
/******************************************************************************
#define NUM_DIRECTIONS 16
#define NUM_STEPS      12
Vec FetchEyePos(Vec2 UV) {return GetPos(TexDepthRawLinear(UV), UVToPosXY(UV));}
Flt ComputeAO(Vec P, Vec N, Vec S)
{
   Vec delta = S - P;
   Flt len2=Length2(delta);
   Flt NdotV = dot(N, delta) * rsqrt(len2);
   return Sat(NdotV) * Sat(1 - len2 * AmbientRangeInvSqr);
}
Half HBAO(Vec2 uv, Vec nrm, Vec pos, Vec2 g_fRadiusToScreen)
{
   Flt AO=0;
   for(Int DirectionIndex=0; DirectionIndex<NUM_DIRECTIONS; DirectionIndex++)
   {
      Flt  Angle=DirectionIndex*(PI2/NUM_DIRECTIONS);
      Vec2 dir2 =Vec2(cos(Angle), sin(Angle))*g_fRadiusToScreen/NUM_STEPS;
      for(Int StepIndex=0; StepIndex<NUM_STEPS; StepIndex++)
      {
         Vec S=FetchEyePos(uv + dir2 * (StepIndex+1));
         AO+=ComputeAO(pos, nrm, S);
      }
   }
   AO/=(NUM_DIRECTIONS*NUM_STEPS);
   return 1-AO*2;
}
/******************************************************************************/
Half GTAOIntegrateArc(VecH2 h, Half n)
{
   VecH2 Arc = -cos(2*h-n) + cos(n) + 2*h*sin(n); return 0.25*(Arc.x+Arc.y);
}
Flt FadeOut(Flt dist2)
{
 /*Flt dist=Sqrt(dist2);
   Flt f=Sat(dist/(AmbientRange_2*2));
   if(W)return Sat(2-f*f*2);
   if(E)return Sat(4-f  *4);
   if(R)return f<1;*/
   return Sat(2-dist2*AmbientRangeInvSqr2); // 2-f*f*2
}
/******************************************************************************/
Half AO_PS
(
   NOPERSP Vec2 inTex   :TEXCOORD ,
   NOPERSP Vec2 inPosXY :TEXCOORD1,
#if !NORMALS
   NOPERSP Vec2 inPosXY1:TEXCOORD2,
#endif
   NOPERSP PIXEL
):TARGET
{
   Vec2 nrm2;
   Vec  nrm, pos=GetPos(TexDepthRawPoint(inTex), inPosXY), eye_dir=Normalize(pos); // !! for AO shader depth is already linearized !!

   #if NORMALS
   {
      nrm=TexLod(Img, inTex).xyz; // use filtering because 'Img' may be bigger, especially important for pixels in the distance (there are some cases however when point filtering improves quality, although not always)
   #if !SIGNED_NRM_RT
      nrm-=0.5; // normally it should be "nrm=nrm*2-1", however we normalize it below, so we can just do -0.5
   #endif
   }
   #else // NORMALS
   {
      // !! for AO shader depth is already linearized !!
      Flt zl=TexDepthRawPointOfs(inTex, VecI2(-1,  0)), // TexDepthRawPoint(inTex-Vec2(RTSize.x, 0)),
          zr=TexDepthRawPointOfs(inTex, VecI2( 1,  0)), // TexDepthRawPoint(inTex+Vec2(RTSize.x, 0)),
          zd=TexDepthRawPointOfs(inTex, VecI2( 0, -1)), // TexDepthRawPoint(inTex-Vec2(0, RTSize.y)),
          zu=TexDepthRawPointOfs(inTex, VecI2( 0,  1)), // TexDepthRawPoint(inTex+Vec2(0, RTSize.y)),
          dl=pos.z-zl, dr=zr-pos.z,
          dd=pos.z-zd, du=zu-pos.z;

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
   #endif // NORMALS

   nrm=Normalize(nrm);
   Vec nrm_clamp=nrm; nrm_clamp.z=Min(nrm_clamp.z, -1.0/255); nrm_clamp=Normalize(nrm_clamp); // normal that's always facing the camera, this is needed for normals facing away from the camera

   if(0) // FIXME is this still needed?
   {
      pos.z=DelinearizeDepth(pos.z);
      DEPTH_DEC(pos.z, 0.00000007); // value tested on fov 20 deg, 1000 view range
      pos.z=LinearizeDepth(pos.z); // convert back to linear
   }

   Vec2  cos_sin;
   Flt   jitter_angle, jitter_step, jitter_half;
   VecI2 pix;
   if(JITTER)
   {
      pix=pixel.xy;
      if(JITTER_RANGE==3)
      {
         pix%=3;
         jitter_angle=((((pix.x+pix.y)%3)*3)+pix.x)*(1.0/9); // 0 .. 0.888
         jitter_step =((  pix.y-pix.x)%3          )*(1.0/3); // 0 .. 0.666
         jitter_half =1; // because values are 0..2 inclusive
      }else
      {
         pix&=3;
         jitter_angle=((((pix.x+pix.y)&3)<<2)+pix.x)*(1.0/16); // 0 .. 0.9375
         jitter_step =((  pix.y-pix.x)&3           )*(1.0/ 4); // 0 .. 0.75
         jitter_half =1.5; // because values are 0..3 inclusive
      }
   #if TEMPORAL
      jitter_angle+=AmbientTemporalAngle;
      jitter_step -=AmbientTemporalStep ; // subtract because later we're subtracting 'jitter_step' instead of adding
   #endif
   }

   Flt  occl  =0,
        weight=0;
   Vec2 offs_scale=Viewport.size_fov_tan*(AmbientRange_2/Max(1.0f, pos.z)); // use /2 because we're converting from -1..1 to 0..1 scale

   //if(R)return HBAO(inTex, nrm, pos, offs_scale);

   #if AO_MODE==AO_MAX
   {
      Int angles, max_steps;
      if(QUALITY==0){angles= 4; max_steps=3;}else // 12 (Better than angles= 3; max_steps=4;)
      if(QUALITY==1){angles= 6; max_steps=5;}else // 30
      if(QUALITY==2){angles= 8; max_steps=7;}else // 56 (Better than angles= 9; max_steps=6; AND angles=8; max_steps=6;)
                    {angles=10; max_steps=9;}     // 90 (Better than angles=10; max_steps=8; AND angles=9; max_steps=9;)
      angles/=2; // because we process 2 at the same time
      LOOP for(Int a=0; a<angles; a++)
      {
         Flt  angle=a; if(JITTER)angle+=jitter_angle; angle*=PI/angles; // this is best for cache
         Vec2 dir2; CosSin(dir2.x, dir2.y, angle);
         Vec  dir=PointOnPlaneRay(Vec(dir2.x, -dir2.y, 0), nrm_clamp, eye_dir); // this is nrm tangent, doesn't need to be normalized
         dir2*=offs_scale;
         Vec2 max_sin=0;
         Int  steps=max_steps;
         LOOP for(Int s=1; s<=steps; s++) // start from 1 to skip this pixel
         {
            Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
            if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
            Vec2 uv0=inTex+d; // TODO: may need to do UVClamp
            Vec2 uv1=inTex-d; // TODO: may need to do UVClamp
            Flt  test_z0=(LINEAR_FILTER ? TexDepthRawLinear(uv0) : TexDepthRawPoint(uv0)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Flt  test_z1=(LINEAR_FILTER ? TexDepthRawLinear(uv1) : TexDepthRawPoint(uv1)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Vec  test_pos0=GetPos(test_z0, UVToPosXY(uv0)), delta0=test_pos0-pos;
            Vec  test_pos1=GetPos(test_z1, UVToPosXY(uv1)), delta1=test_pos1-pos;
            Flt  y0=Dot(delta0, nrm); if(y0>0.5/255){Flt delta0_len2=Length2(delta0); Flt w0=FadeOut(delta0_len2); Flt sin0=y0*rsqrt(delta0_len2); Flt x0=Dot(delta0, dir); if(x0<0)sin0=1; max_sin.x=Max(max_sin.x, sin0*w0);} // small bias needed for walls perpendicular to camera at a distance
            Flt  y1=Dot(delta1, nrm); if(y1>0.5/255){Flt delta1_len2=Length2(delta1); Flt w1=FadeOut(delta1_len2); Flt sin1=y1*rsqrt(delta1_len2); Flt x1=Dot(delta1, dir); if(x1>0)sin1=1; max_sin.y=Max(max_sin.y, sin1*w1);} // small bias needed for walls perpendicular to camera at a distance
         }
      #if 0
         alternative with steps range clamp:
         {
            Flt frac=ViewportClamp(inTex+dir2, dir2);
            Int steps=Floor(max_steps*(1-frac)+HALF_MIN+(JITTER?jitter_step:0)); // this will have the same effect as if ignoring samples outside of viewport
            LOOP for(Int s=steps; s>=1; s--) // end at 1 to skip this pixel
            {
               Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
               if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
               Vec2 uv0=inTex+d;
               Flt  test_z0=(LINEAR_FILTER ? TexDepthRawLinear(uv0) : TexDepthRawPoint(uv0)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
               Vec  test_pos0=GetPos(test_z0, UVToPosXY(uv0)), delta0=test_pos0-pos;
               Flt  y0=Dot(delta0, nrm); if(y0>0.5/255){Flt delta0_len2=Length2(delta0); Flt w0=FadeOut(delta0_len2); Flt sin0=y0*rsqrt(delta0_len2); Flt x0=Dot(delta0, dir); if(x0<0)sin0=1; max_sin.x=Max(max_sin.x, sin0*w0);} // small bias needed for walls perpendicular to camera at a distance
            }
            frac=ViewportClamp(inTex-dir2, dir2);
            steps=Floor(max_steps*(1-frac)+HALF_MIN+(JITTER?jitter_step:0)); // this will have the same effect as if ignoring samples outside of viewport
            LOOP for(Int s=1; s<=steps; s++) // start from 1 to skip this pixel
            {
               Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
               if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
               Vec2 uv1=inTex-d;
               Flt  test_z1=(LINEAR_FILTER ? TexDepthRawLinear(uv1) : TexDepthRawPoint(uv1)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
               Vec  test_pos1=GetPos(test_z1, UVToPosXY(uv1)), delta1=test_pos1-pos;
               Flt  y1=Dot(delta1, nrm); if(y1>0.5/255){Flt delta1_len2=Length2(delta1); Flt w1=FadeOut(delta1_len2); Flt sin1=y1*rsqrt(delta1_len2); Flt x1=Dot(delta1, dir); if(x1>0)sin1=1; max_sin.y=Max(max_sin.y, sin1*w1);} // small bias needed for walls perpendicular to camera at a distance
            }
         }
      #endif
      #if 0
         if(0) // GTAO
         {
            if(R){weight+=1; occl+=GTAOIntegrateArc(asin(max_sin), 0);}else
                 {weight+=2; occl+=Sqr(max_sin.x)+Sqr(max_sin.y);}
         }else
      #endif
         {
            occl  +=2-CosSin(max_sin.x)-CosSin(max_sin.y); // (1-CosSin(max_sin.x)) + (1-CosSin(max_sin.y))
            weight+=2;
         }
      }
   }
   #endif

   #if AO_MODE==AO_AVG
   {   
      Int angles, max_steps;
      if(QUALITY==0){angles= 4; max_steps=3;}else // 12 (Better than angles= 3; max_steps=4;)
      if(QUALITY==1){angles= 6; max_steps=5;}else // 30
      if(QUALITY==2){angles= 8; max_steps=7;}else // 56 (Better than angles= 9; max_steps=6; AND angles=8; max_steps=6;)
                    {angles=10; max_steps=9;}     // 90 (Better than angles=10; max_steps=8; AND angles=9; max_steps=9;)
      LOOP for(Int a=0; a<angles; a++)
      {
         Flt  angle=a; if(JITTER)angle+=jitter_angle; angle*=PI2/angles; // this is best for cache
         Vec2 dir2; CosSin(dir2.x, dir2.y, angle);
         Vec  dir=PointOnPlaneRay(Vec(dir2.x, -dir2.y, 0), nrm_clamp, eye_dir); // this is nrm tangent, doesn't need to be normalized
         dir2*=offs_scale;

         Flt frac=ViewportClamp(inTex+dir2, dir2);
         // instead of reducing movement "dir2*=1-frac;" limit number of steps, because reduced movement would change weights for samples
         Int steps=Floor(max_steps*(1-frac)+HALF_MIN+(JITTER?jitter_step:0)); // this will have the same effect as if ignoring samples outside of viewport
         weight+=(max_steps-steps)*0.5; // add 0.5 weight for each step skipped

         LOOP for(Int s=1; s<=steps; s++) // start from 1 to skip this pixel
         {
            Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
            if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
            Vec2 uv=inTex+d;
            Flt  test_z  =(LINEAR_FILTER ? TexDepthRawLinear(uv) : TexDepthRawPoint(uv)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Vec  test_pos=GetPos(test_z, UVToPosXY(uv)),
                 delta   =test_pos-pos;
            Flt  delta_len2=Length2(delta);
            Flt  o, w=FadeOut(delta_len2);
            Flt  y=Dot(delta, nrm); if(y>0.5/255) // small bias needed for walls perpendicular to camera at a distance
            {
               Flt sin=y*rsqrt(delta_len2);
               Flt x=Dot(delta, dir); if(x<0)sin=1;
               o =1-CosSin(sin);
               o*=w; // fix artifacts (occlusion can be strong only as weight)
            }else o=0;
            w=w*0.5+0.5;   // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(0.5, w); // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(1, 1/Sqrt(delta_len2));
            occl  +=w*o;
            weight+=w;
         }
      }
      occl*=2; // multiply by 2 to match AO_MAX mode

   }
   #endif

   #if 0 // alternative AO_AVG
      Int angles, max_steps;
      if(QUALITY==0){angles= 4; max_steps=3;}else // 12 (Better than angles= 3; max_steps=4;)
      if(QUALITY==1){angles= 6; max_steps=5;}else // 30
      if(QUALITY==2){angles= 8; max_steps=7;}else // 56 (Better than angles= 9; max_steps=6; AND angles=8; max_steps=6;)
                    {angles=10; max_steps=9;}     // 90 (Better than angles=10; max_steps=8; AND angles=9; max_steps=9;)
      angles/=2; // because we process 2 at the same time
      LOOP for(Int a=0; a<angles; a++)
      {
         Flt  angle=a; if(JITTER)angle+=jitter_angle; angle*=PI/angles; // this is best for cache
         Vec2 dir2; CosSin(dir2.x, dir2.y, angle);
         Vec  dir=PointOnPlaneRay(Vec(dir2.x, -dir2.y, 0), nrm_clamp, eye_dir); // this is nrm tangent, doesn't need to be normalized
         dir2*=offs_scale;
         Int steps=max_steps;
         LOOP for(Int s=1; s<=steps; s++) // start from 1 to skip this pixel
         {
            Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
            if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
            Vec2 uv0=inTex+d;
            Vec2 uv1=inTex-d;
            Flt  test_z0=(LINEAR_FILTER ? TexDepthRawLinear(uv0) : TexDepthRawPoint(uv0)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Flt  test_z1=(LINEAR_FILTER ? TexDepthRawLinear(uv1) : TexDepthRawPoint(uv1)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Vec  test_pos0=GetPos(test_z0, UVToPosXY(uv0)), delta0=test_pos0-pos;
            Vec  test_pos1=GetPos(test_z1, UVToPosXY(uv1)), delta1=test_pos1-pos;
            Flt  delta0_len2=Length2(delta0);
            Flt  delta1_len2=Length2(delta1);
            Flt  o0, w0=FadeOut(delta0_len2);
            Flt  o1, w1=FadeOut(delta1_len2);
            Flt  y0=Dot(delta0, nrm); if(y0>0.5/255){Flt sin=y0*rsqrt(delta0_len2); Flt x=Dot(delta0, dir); if(x<0)sin=1; o0=1-CosSin(sin); o0*=w0;}else o0=0;
            Flt  y1=Dot(delta1, nrm); if(y1>0.5/255){Flt sin=y1*rsqrt(delta1_len2); Flt x=Dot(delta1, dir); if(x>0)sin=1; o1=1-CosSin(sin); o1*=w1;}else o1=0;
            w0=w0*0.5+0.5;
            w1=w1*0.5+0.5;
            occl+=w0*o0; weight+=w0;
            occl+=w1*o1; weight+=w1;
         }
      }
      occl*=2; // multiply by 2 to match AO_MAX mode
   #endif
   #if 0 // alternative AO_AVG
      Int angles, max_steps;
      if(QUALITY==0){angles= 4; max_steps=3;}else // 12 (Better than angles= 3; max_steps=4;)
      if(QUALITY==1){angles= 6; max_steps=5;}else // 30
      if(QUALITY==2){angles= 8; max_steps=7;}else // 56 (Better than angles= 9; max_steps=6; AND angles=8; max_steps=6;)
                    {angles=10; max_steps=9;}     // 90 (Better than angles=10; max_steps=8; AND angles=9; max_steps=9;)
      angles/=2; // because we process 2 at the same time
      LOOP for(Int a=0; a<angles; a++)
      {
         Flt  angle=a; if(JITTER)angle+=jitter_angle; angle*=PI/angles; // this is best for cache
         Vec2 dir2; CosSin(dir2.x, dir2.y, angle);
         Vec  dir=PointOnPlaneRay(Vec(dir2.x, -dir2.y, 0), nrm_clamp, eye_dir); // this is nrm tangent, doesn't need to be normalized
         dir2*=offs_scale;

         Flt frac=ViewportClamp(inTex+dir2, dir2);
         // instead of reducing movement "dir2*=1-frac;" limit number of steps, because reduced movement would change weights for samples
         Int steps=Floor(max_steps*(1-frac)+HALF_MIN+(JITTER?jitter_step:0)); // this will have the same effect as if ignoring samples outside of viewport
         weight+=(max_steps-steps)*0.5; // add 0.5 weight for each step skipped

         LOOP for(Int s=steps; s>=1; s--) // end at 1 to skip this pixel
         {
            Vec2 d=dir2*((JITTER ? s-jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
            if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
            Vec2 uv=inTex+d;

            Flt test_z  =(LINEAR_FILTER ? TexDepthRawLinear(uv) : TexDepthRawPoint(uv)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Vec test_pos=GetPos(test_z, UVToPosXY(uv)),
                delta   =test_pos-pos;
            Flt delta_len2=Length2(delta);
            Flt o, w=FadeOut(delta_len2);
            Flt y=Dot(delta, nrm); if(y>0.5/255) // small bias needed for walls perpendicular to camera at a distance
            {
               Flt sin=y*rsqrt(delta_len2);
               Flt x=Dot(delta, dir); if(x<0)sin=1;
               o =1-CosSin(sin);
               o*=w; // fix artifacts (occlusion can be strong only as weight)
            }else o=0;
            w=w*0.5+0.5;   // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(0.5, w); // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(1, 1/Sqrt(delta_len2));
            occl  +=w*o;
            weight+=w;
         }

         frac=ViewportClamp(inTex-dir2, dir2);
         // instead of reducing movement "dir2*=1-frac;" limit number of steps, because reduced movement would change weights for samples
         steps=Floor(max_steps*(1-frac)+HALF_MIN+(JITTER?jitter_step:0)); // this will have the same effect as if ignoring samples outside of viewport
         weight+=(max_steps-steps)*0.5; // add 0.5 weight for each step skipped

         LOOP for(Int s=-1; s>=-steps; s--) // start from -1 to skip this pixel
         {
            Vec2 d=dir2*((JITTER ? s+jitter_step : s)/Flt(max_steps)); // subtract 'jitter_step' because we start from step 's=1' and subtracting 0 .. 0.75 jitter allows us to still skip step 0 and start from 0.25
            if(!LINEAR_FILTER){d=Round(d*RTSize.zw)*RTSize.xy; if(!any(d)){weight++; continue;}}
            Vec2 uv=inTex+d;

            Flt test_z  =(LINEAR_FILTER ? TexDepthRawLinear(uv) : TexDepthRawPoint(uv)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 'uv'
            Vec test_pos=GetPos(test_z, UVToPosXY(uv)),
                delta   =test_pos-pos;
            Flt delta_len2=Length2(delta);
            Flt o, w=FadeOut(delta_len2);
            Flt y=Dot(delta, nrm); if(y>0.5/255) // small bias needed for walls perpendicular to camera at a distance
            {
               Flt sin=y*rsqrt(delta_len2);
               Flt x=Dot(delta, dir); if(x>0)sin=1;
               o =1-CosSin(sin);
               o*=w; // fix artifacts (occlusion can be strong only as weight)
            }else o=0;
            w=w*0.5+0.5;   // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(0.5, w); // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(1, 1/Sqrt(delta_len2));
            occl  +=w*o;
            weight+=w;
         }
      }
      occl*=2; // multiply by 2 to match AO_MAX mode
   #endif

   #if AO_MODE==AO_PATTERN
   {
      Int            elms; Flt spacing;
      if(QUALITY==0){elms=AO0Elms; spacing=AO0Spacing;}else
      if(QUALITY==1){elms=AO1Elms; spacing=AO1Spacing;}else
      if(QUALITY==2){elms=AO2Elms; spacing=AO2Spacing;}else
                    {elms=AO3Elms; spacing=AO3Spacing;}
      Vec2 jitter_offs; if(JITTER){CosSin(cos_sin.x, cos_sin.y, jitter_angle); jitter_offs=(pix.yx-jitter_half)*(spacing*0.215);} // using higher values may affect cache performance, so use smallest possible
      LOOP for(Int i=0; i<elms; i++) // using UNROLL didn't make a performance difference, however it made shader file bigger and compilation slower
      {
         Vec2          dir2; // don't use 'VecH2' here because benefit looks small, and 'dir2' has to be added to 'inTex' and multiplied by 'nrm2' which are 'Vec2' so probably there would be no performance benefits
         if(QUALITY==0)dir2=AO0Vec[i];else
         if(QUALITY==1)dir2=AO1Vec[i];else
         if(QUALITY==2)dir2=AO2Vec[i];else
                       dir2=AO3Vec[i];

         if(JITTER        )dir2=Rotate(dir2, cos_sin)+jitter_offs;
         Vec2              uv_delta=dir2*offs_scale;
         if(!LINEAR_FILTER)uv_delta=Round(uv_delta*RTSize.zw)*RTSize.xy;

         Vec2 t=inTex+uv_delta;
         Flt  o, w;

         if(!LINEAR_FILTER && !any(uv_delta))
         {
            o=0; w=1;
         }else
         if(all(Abs(t-Viewport.center)<=Viewport.size/2)) // UV inside viewport
         {
            // !! for AO shader depth is already linearized !!
            Flt test_z  =(LINEAR_FILTER ? TexDepthRawLinear(t) : TexDepthRawPoint(t)); // !! for AO shader depth is already linearized !! can use point filtering because we've rounded 't'
            Vec test_pos=GetPos(test_z, UVToPosXY(t)),
                delta   =test_pos-pos;
            Flt delta_len2=Length2(delta);
            w=FadeOut(delta_len2);
            Flt y=Dot(delta, nrm); if(y>0)
            {
               Flt sin=y*rsqrt(delta_len2); // "/Length(delta)" -> "/Sqrt(delta_len2)"
               Vec dir=PointOnPlaneRay(Vec(dir2.x, -dir2.y, 0), nrm_clamp, eye_dir); // this is nrm tangent, doesn't need to be normalized
               Flt x=Dot(delta, dir); if(x<0)sin=1;
               o=1-CosSin(sin); // precise, calculated based on 'ObstacleSinToLight'
               o*=w; // fix artifacts (occlusion can be strong only as weight)
            }else o=0;
            w=w*0.5+0.5;   // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(0.5, w); // fix artifacts, this increases weight if it's small, which results in brightening because we don't touch occlusion
          //w=Max(1, 1/Sqrt(delta_len2));
         }else // UV outside viewport
         {
            o=0; w=0.5; // set as brightening but use small weight
         }

         occl  +=w*o;
         weight+=w;
      }
      occl*=2; // multiply by 2 to match AO_MAX mode
   }
   #endif

   return Max(AmbientMin, 1-AmbientContrast*occl/weight);
}
/******************************************************************************/
