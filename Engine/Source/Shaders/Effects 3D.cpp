/******************************************************************************/
#include "!Header.h"
#ifndef INSIDE
#define INSIDE 0
#endif
#ifndef LA
#define LA 0
#endif
#ifndef NORMALS
#define NORMALS 0
#endif
/******************************************************************************

   Keep here effects that are rarely used.

/******************************************************************************/
// VOLUME
/******************************************************************************/
#include "!Set Prec Struct.h"
struct VolumeClass
{
   Flt min_steps,
       max_steps,
       density_factor,
       precision;
   Vec size  ,
       pixels,
       inside;
};

BUFFER(Volume)
   VolumeClass Volume;
BUFFER_END
#include "!Set Prec Default.h"

void Volume_VS(VtxInput vtx,
           out Vec4    outVtx:POSITION ,
           out Vec     outPos:TEXCOORD0,
           out Vec     outTex:TEXCOORD1,
           out Matrix3 outMat:TEXCOORD2)
{
   outMat[0]=Normalize(ViewMatrixX());
   outMat[1]=Normalize(ViewMatrixY());
   outMat[2]=Normalize(ViewMatrixZ());

   // convert to texture space (0..1)
   if(INSIDE)outTex=Volume.inside/(2*Volume.size)+0.5;
   else      outTex=vtx.pos()*0.5+0.5;

   outVtx=Project(outPos=TransformPos(vtx.pos()));
}
void Volume_PS
(
   PIXEL,

   Vec     inPos:TEXCOORD0,
   Vec     inTex:TEXCOORD1,
   Matrix3 inMat:TEXCOORD2,

   out VecH4 color:TARGET0,
   out VecH4 mask :TARGET1
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=inTex;
   Vec dir=Normalize(inPos); dir*=Min((SQRT3*2)*Max(Volume.size), (z-(INSIDE ? Viewport.from : inPos.z))/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   dir=dir/(2*Volume.size);

   if(INSIDE==1)
   {
      if(pos.x<0)pos+=(0-pos.x)/dir.x*dir;
      if(pos.x>1)pos+=(1-pos.x)/dir.x*dir;
      if(pos.y<0)pos+=(0-pos.y)/dir.y*dir;
      if(pos.y>1)pos+=(1-pos.y)/dir.y*dir;
      if(pos.z<0)pos+=(0-pos.z)/dir.z*dir;
      if(pos.z>1)pos+=(1-pos.z)/dir.z*dir;
   }

   Vec end=pos+dir;

   if(end.x<0)end+=(0-end.x)/dir.x*dir;
   if(end.x>1)end+=(1-end.x)/dir.x*dir;
   if(end.y<0)end+=(0-end.y)/dir.y*dir;
   if(end.y>1)end+=(1-end.y)/dir.y*dir;
   if(end.z<0)end+=(0-end.z)/dir.z*dir;
   if(end.z>1)end+=(1-end.z)/dir.z*dir;

       dir        =end-pos;
   Flt pixels     =Length(dir   *Volume.pixels);
   Int steps      =Mid   (pixels*Volume.precision, Volume.min_steps, Volume.max_steps);
   Flt steps_speed=       pixels/steps;

   Flt density_factor=Volume.density_factor;
       density_factor=1-Pow(1-density_factor, steps_speed); // modify by steps speed

   dir/=steps;

   // !! Use high precision here because iterating many samples !!

   if(LA)
   {
      Vec2 col=0;

      LOOP for(Int i=0; i<steps; i++)
      {
         Vec2 sample=Tex3DLod(VolXY, pos).rg; // HP
         Flt  alpha =sample.g*density_factor*(1-col.g);

         col.r+=alpha*sample.r;
         col.g+=alpha;

         pos+=dir;
      }

      col.r/=col.g+HALF_MIN; // NaN

      if(LINEAR_GAMMA)col.r=SRGBToLinearFast(col.r);

      color=col.rrrg*Color[0]+Color[1];
   }else
   {
      Vec4 col=0;

      LOOP for(Int i=0; i<steps; i++)
      {
         Vec4 sample=Tex3DLod(Vol, pos); // HP
         Flt  alpha =sample.a*density_factor*(1-col.a);

         col.rgb+=alpha*sample.rgb;
         col.a  +=alpha;

         pos+=dir;
      }

      col.rgb/=col.a+HALF_MIN; // NaN

      color=col*Color[0]+Color[1];
   }
   mask.rgb=0; mask.a=color.a;
}
/******************************************************************************/
// LASER
/******************************************************************************/
void Laser_VS(VtxInput vtx,
          out Vec  outPos:TEXCOORD0,
         #if NORMALS
          out VecH outNrm:TEXCOORD1,
         #endif
          out Vec4 outVtx:POSITION )
{
#if NORMALS
   outNrm=TransformDir(vtx.nrm());
#endif
   outVtx=Project(outPos=TransformPos(vtx.pos()));
}
void Laser_PS(Vec                 inPos:TEXCOORD0,
           #if NORMALS
              VecH                inNrm:TEXCOORD1,
           #endif
          out DeferredSolidOutput output         )
{
#if NORMALS
         inNrm=Normalize(inNrm);
   Half  stp=Max (-Dot(inNrm, Normalize(inPos)), -inNrm.z);
         stp=Sat (stp);
         stp=Pow (stp, Step);
   VecH4 col=Lerp(Color[0], Color[1], stp);
   output.color(col.rgb);
   output.glow (col.a  );
#else
   output.color(Color[0].rgb);
   output.glow (Color[0].a  );
#endif
   output.normal      (0);
   output.translucent (0);
   output.smooth      (0);
   output.reflect     (0);
   output.velocityZero( );
}
/******************************************************************************/
// DECAL
/******************************************************************************/
// FULLSCREEN, LAYOUT, MODE 0-default, 1-normals, 2-palette
#include "!Set Prec Struct.h"
BUFFER(Decal)
   VecH2 DecalParams; // x=OpaqueFracMul, y=OpaqueFracAdd
BUFFER_END
#include "!Set Prec Default.h"

Half DecalOpaqueFracMul() {return DecalParams.x;}
Half DecalOpaqueFracAdd() {return DecalParams.y;}

void Decal_VS(VtxInput vtx,
          out Vec4    outVtx    :POSITION,
          out Matrix  outMatrix :TEXCOORD0
       #if MODE==1
        , out Matrix3 outMatrixN:TEXCOORD3
       #endif
)
{
   outMatrix=GetViewMatrix();
   outMatrix[0]/=Length2(outMatrix[0]);
   outMatrix[1]/=Length2(outMatrix[1]);
   outMatrix[2]/=Length2(outMatrix[2]);

#if MODE==1 // normal
   outMatrixN[0]=Normalize(outMatrix[0]);
   outMatrixN[1]=Normalize(outMatrix[1]);
   outMatrixN[2]=Normalize(outMatrix[2]);
#endif

#if FULLSCREEN
   outVtx=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by processing only solid pixels (no sky/background)
#else
   outVtx=Project(TransformPos(vtx.pos()));
#endif
}
VecH4 Decal_PS(PIXEL,
               Matrix  inMatrix :TEXCOORD0
            #if MODE==1
         ,     Matrix3 inMatrixN:TEXCOORD3
         , out VecH4   outNrm   :TARGET1
            #endif
              ):TARGET // #RTOutput
{
   Vec  pos  =GetPosPoint(PixelToScreen(pixel));
        pos  =TransformTP(pos-inMatrix[3], (Matrix3)inMatrix);
   Half alpha=Sat(Abs(pos.z)*DecalOpaqueFracMul()+DecalOpaqueFracAdd());

   clip(Vec(1-Abs(pos.xy), alpha-EPS_COL));

   pos.xy=pos.xy*0.5+0.5;

   VecH4 col=Tex(Col, pos.xy);
#if LAYOUT==2
       col.a=Tex(Ext, pos.xy).a; // #MaterialTextureLayout
#endif
   col.a*=alpha;

#if MODE==2 // palette
   return (col.a*Material.color.a)*Color[0];
#else
   col*=Material.color*Color[0];

   #if MODE==1 // normal
           VecH nrm;
                nrm.xy =Tex(Nrm, pos.xy).xy*Material.normal; // #MaterialTextureLayout
    //if(DETAIL)nrm.xy+=det.xy;
                nrm.z  =CalcZ(nrm.xy);
                nrm    =Transform(nrm, inMatrixN);

      #if SIGNED_NRM_RT
         outNrm.xyz=nrm;
      #else
         outNrm.xyz=nrm*0.5+0.5;
      #endif
         outNrm.w=col.a; // alpha
   #endif

   return col;
#endif
}
/******************************************************************************/
