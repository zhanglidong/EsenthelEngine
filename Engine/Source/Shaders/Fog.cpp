/******************************************************************************/
#include "!Header.h"
#include "Fog.h"
/******************************************************************************/
// GLOBAL
/******************************************************************************/
VecH4 Fog_PS(NOPERSP Vec2 uv   :UV,
             NOPERSP Vec2 posXY:POS_XY
         #if MULTI_SAMPLE
           , NOPERSP PIXEL                 
         #endif
         #if MULTI_SAMPLE==2
           ,         UInt index:SV_SampleIndex
         #endif
            ):TARGET
{
#if MULTI_SAMPLE!=1
   #if MULTI_SAMPLE==0 // 0: 1s->1s
      Vec pos=GetPosPoint(uv, posXY);
   #else // 2: ms->ms
      Vec pos=GetPosMS(pixel.xy, index, posXY);
   #endif
   Half dns=AccumulatedDensity(FogDensity, Length(pos));
#else // 1: ms->1s
   Half dns=0, valid=HALF_MIN;
   UNROLL for(Int i=0; i<MS_SAMPLES; i++)
   {
      Flt depth=TexDepthRawMS(pixel.xy, i); if(DEPTH_FOREGROUND(depth))
      {
         Vec pos =GetPos(LinearizeDepth(depth), posXY);
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
           out Vec4    pixel :POSITION ,
           out Vec     outPos:TEXCOORD0,
           out Vec     uv    :TEXCOORD1,
           out Vec4    size  :SIZE,
           out Matrix3 outMat:MATRIX)
{
   outMat[0]=ViewMatrixX(); size.x=Length(outMat[0]); outMat[0]/=size.x;
   outMat[1]=ViewMatrixY(); size.y=Length(outMat[1]); outMat[1]/=size.y;
   outMat[2]=ViewMatrixZ(); size.z=Length(outMat[2]); outMat[2]/=size.z;
                            size.w=Max(size.xyz);

   // convert to texture space (0..1)
   uv   =vtx.pos()*0.5+0.5;
   pixel=Project(outPos=TransformPos(vtx.pos()));
}
void FogBox_PS
(
   PIXEL,

   Vec     inPos:TEXCOORD0,
   Vec     inTex:TEXCOORD1,
   Vec4    size :SIZE,
   Matrix3 mtrx :MATRIX,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToUV(pixel));
   Vec pos=inTex,
       dir=Normalize(inPos); dir*=Min((SQRT3*2)*size.w, (z-inPos.z)/dir.z);
       dir=TransformTP(dir, mtrx); // convert to box space

   // convert to texture space (0..1)
   dir=dir/(2*size.xyz);

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir =end-pos;
       dir*=size.xyz;
   Flt len =Length(dir)/size.w;

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
            out Vec2    uv    :UV,
            out Vec2    posXY :POS_XY,
            out Vec4    size  :SIZE,
            out Matrix3 outMat:MATRIX,
            out Vec4    pixel :POSITION)
{
   outMat[0]=ViewMatrixX(); size.x=Length(outMat[0]); outMat[0]/=size.x;
   outMat[1]=ViewMatrixY(); size.y=Length(outMat[1]); outMat[1]/=size.y;
   outMat[2]=ViewMatrixZ(); size.z=Length(outMat[2]); outMat[2]/=size.z;
                            size.w=Max(size.xyz);

   pixel=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
   uv   =vtx.uv();
   posXY=UVToPosXY(uv);
}
void FogBoxI_PS
(
   NOPERSP Vec2    uv   :UV,
   NOPERSP Vec2    posXY:POS_XY,
   NOPERSP Vec4    size :SIZE,
   NOPERSP Matrix3 mtrx :MATRIX,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Vec pos=GetPosPoint(uv, posXY),
       dir=Normalize(pos); dir*=Min((SQRT3*2)*size.w, (pos.z-Viewport.from)/dir.z);
       dir=TransformTP(dir, mtrx); // convert to box space

   // convert to texture space (0..1)
   pos=LocalFogInside/(2*size.xyz)+0.5;
   dir=dir           /(2*size.xyz);

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
       dir*=size.xyz;
   Flt len =Length(dir)/size.w;

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
            out Vec4 pixel :POSITION ,
            out Vec  outPos:TEXCOORD0,
            out Vec  uv    :TEXCOORD1,
            out Flt  size  :SIZE)
{
   uv   =vtx.pos();
   size =Length(ViewMatrixX());
   pixel=Project(outPos=TransformPos(vtx.pos()));
}
void FogBall_PS
(
   PIXEL,

   Vec inPos:TEXCOORD0,
   Vec inTex:TEXCOORD1,
   Flt size :SIZE,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToUV(pixel));
   Vec pos=Normalize    (inTex),
       dir=Normalize    (inPos); Flt max_length=(z-inPos.z)/(dir.z*size);
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
             out Vec2 uv   :UV,
             out Vec2 posXY:POS_XY,
             out Flt  size :SIZE,
             out Vec4 pixel:POSITION)
{
   pixel=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
   uv   =vtx.uv();
   posXY=UVToPosXY(uv);
   size =Length(ViewMatrixX());
}
void FogBallI_PS
(
   NOPERSP Vec2 uv   :UV,
   NOPERSP Vec2 posXY:POS_XY,
   NOPERSP Flt  size :SIZE,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Vec pos=GetPosPoint(uv, posXY),
       dir=Normalize (pos); Flt max_length=(pos.z-Viewport.from)/(dir.z*size);
       dir=Transform3(dir, CamMatrix); // convert to ball space

   pos=LocalFogInside/size;

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
