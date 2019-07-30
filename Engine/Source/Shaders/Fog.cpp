/******************************************************************************/
#include "!Header.h"
#include "Fog.h"
/******************************************************************************/
// GLOBAL
/******************************************************************************/
VecH4 Fog_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
             NOPERSP Vec2 inPosXY:TEXCOORD1
         #if MULTI_SAMPLE
           , NOPERSP PIXEL                 
         #endif
         #if MULTI_SAMPLE==2
           ,         UInt index  :SV_SampleIndex
         #endif
            ):TARGET
{
#if MULTI_SAMPLE!=1
   #if MULTI_SAMPLE==0 // 0: 1s->1s
      Vec pos=GetPosPoint(inTex, inPosXY);
   #else // 2: ms->ms
      Vec pos=GetPosMS(pixel.xy, index, inPosXY);
   #endif
   Half dns=AccumulatedDensity(FogDensity, Length(pos));
#else // 1: ms->1s
   Half dns=0, valid=HALF_MIN;
   UNROLL for(Int i=0; i<MS_SAMPLES; i++)
   {
      Flt depth=TexDepthMSRaw(pixel.xy, i); if(DEPTH_FOREGROUND(depth))
      {
         Vec pos =GetPos(LinearizeDepth(depth), inPosXY);
             dns+=AccumulatedDensity(FogDensity, Length(pos));
         valid++;
      }
   }
   dns/=valid;
#endif
   return VecH4(FogColor, dns);
}
/******************************************************************************/
// LOCAL
/******************************************************************************/
// TODO: optimize fog shaders
void FogBox_VS(VtxInput vtx,
           out Vec4    outVtx :POSITION ,
           out Vec     outPos :TEXCOORD0,
           out Vec     outTex :TEXCOORD1,
           out Vec4    outSize:TEXCOORD2,
           out Matrix3 outMat :TEXCOORD3)
{
   outMat[0]=ViewMatrixX(); outSize.x=Length(outMat[0]); outMat[0]/=outSize.x;
   outMat[1]=ViewMatrixY(); outSize.y=Length(outMat[1]); outMat[1]/=outSize.y;
   outMat[2]=ViewMatrixZ(); outSize.z=Length(outMat[2]); outMat[2]/=outSize.z;
                            outSize.w=Max(outSize.xyz);

   // convert to texture space (0..1)
   outTex=vtx.pos()*0.5+0.5;
   outVtx=Project(outPos=TransformPos(vtx.pos()));
}
void FogBox_PS
(
   PIXEL,

   Vec     inPos :TEXCOORD0,
   Vec     inTex :TEXCOORD1,
   Vec4    inSize:TEXCOORD2,
   Matrix3 inMat :TEXCOORD3,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=inTex,
       dir=Normalize(inPos); dir*=Min((SQRT3*2)*inSize.w, (z-inPos.z)/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   dir=dir/(2*inSize.xyz);

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir =end-pos;
       dir*=inSize.xyz;
   Flt len =Length(dir)/inSize.w;

   Flt dns=LocalFogDensity;
#if HEIGHT
   dns*=1-Avg(pos.y, end.y); len*=3;
#endif

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBoxI_VS(VtxInput vtx,
            out Vec2    outTex  :TEXCOORD0,
            out Vec2    outPosXY:TEXCOORD1,
            out Vec4    outSize :TEXCOORD2,
            out Matrix3 outMat  :TEXCOORD3,
            out Vec4    outVtx  :POSITION )
{
   outMat[0]=ViewMatrixX(); outSize.x=Length(outMat[0]); outMat[0]/=outSize.x;
   outMat[1]=ViewMatrixY(); outSize.y=Length(outMat[1]); outMat[1]/=outSize.y;
   outMat[2]=ViewMatrixZ(); outSize.z=Length(outMat[2]); outMat[2]/=outSize.z;
                            outSize.w=Max(outSize.xyz);

   outVtx  =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
   outTex  =vtx.tex();
   outPosXY=ScreenToPosXY(outTex);
}
void FogBoxI_PS
(
   NOPERSP Vec2    inTex  :TEXCOORD0,
   NOPERSP Vec2    inPosXY:TEXCOORD1,
   NOPERSP Vec4    inSize :TEXCOORD2,
   NOPERSP Matrix3 inMat  :TEXCOORD3,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Vec pos=GetPosPoint(inTex, inPosXY),
       dir=Normalize(pos); dir*=Min((SQRT3*2)*inSize.w, (pos.z-Viewport.from)/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   pos=LocalFogInside/(2*inSize.xyz)+0.5;
   dir=dir           /(2*inSize.xyz);

#if INSIDE==0
   if(pos.x<0)pos+=(0-pos.x)/dir.x*dir;
   if(pos.x>1)pos+=(1-pos.x)/dir.x*dir;
   if(pos.y<0)pos+=(0-pos.y)/dir.y*dir;
   if(pos.y>1)pos+=(1-pos.y)/dir.y*dir;
   if(pos.z<0)pos+=(0-pos.z)/dir.z*dir;
   if(pos.z>1)pos+=(1-pos.z)/dir.z*dir;
#endif

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir =end-pos;
       dir*=inSize.xyz;
   Flt len =Length(dir)/inSize.w;

   Flt dns=LocalFogDensity;
#if HEIGHT
   dns*=1-Avg(pos.y, end.y); len*=3;
#endif

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBall_VS(VtxInput vtx,
            out Vec4 outVtx :POSITION ,
            out Vec  outPos :TEXCOORD0,
            out Vec  outTex :TEXCOORD1,
            out Flt  outSize:TEXCOORD2)
{
   outTex =vtx.pos();
   outSize=Length(ViewMatrixX());
   outVtx =Project(outPos=TransformPos(vtx.pos()));
}
void FogBall_PS
(
   PIXEL,

   Vec inPos :TEXCOORD0,
   Vec inTex :TEXCOORD1,
   Flt inSize:TEXCOORD2,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=Normalize    (inTex),
       dir=Normalize    (inPos); Flt max_length=(z-inPos.z)/(dir.z*inSize);
       dir=Transform3   (dir, CamMatrix); // convert to ball space

   // collision detection
   Vec p  =PointOnPlane(pos, dir);
   Flt s  =Length      (p       );
       s  =Sat         (1-s*s   );
   Vec end=p+Sqrt(s)*dir;

   Flt len=Min(Dist(pos, end), max_length);

   Flt dns=LocalFogDensity*s;

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
void FogBallI_VS(VtxInput vtx,
             out Vec2 outTex  :TEXCOORD0,
             out Vec2 outPosXY:TEXCOORD1,
             out Flt  outSize :TEXCOORD2,
             out Vec4 outVtx  :POSITION )
{
   outVtx  =Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
   outTex  =vtx.tex();
   outPosXY=ScreenToPosXY(outTex);
   outSize =Length(ViewMatrixX());
}
void FogBallI_PS
(
   NOPERSP Vec2 inTex  :TEXCOORD0,
   NOPERSP Vec2 inPosXY:TEXCOORD1,
   NOPERSP Flt  inSize :TEXCOORD2,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Vec pos=GetPosPoint(inTex, inPosXY),
       dir=Normalize (pos); Flt max_length=(pos.z-Viewport.from)/(dir.z*inSize);
       dir=Transform3(dir, CamMatrix); // convert to ball space

   pos=LocalFogInside/inSize;

   // collision detection
   Vec p  =PointOnPlane(pos, dir);
   Flt s  =Length      (p       );
       s  =Sat         (1-s*s   );
   Vec end=p+dir*Sqrt(s);

   Flt len=Min(Dist(pos, end), max_length);

   Flt dns=LocalFogDensity*s;

   color.rgb=LocalFogColor;
   color.a  =AccumulatedDensity(dns, len);
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
