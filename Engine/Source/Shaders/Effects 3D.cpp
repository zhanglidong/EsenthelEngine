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
   output.smooth      (0);
   output.velocityZero( );
   output.reflect     (0);
}
/******************************************************************************/
// DECAL
/******************************************************************************/
#include "!Set SP.h"
BUFFER(Decal)
   VecH DecalParams; // x=OpaqueFracMul, y=OpaqueFracAdd, z=alpha
BUFFER_END
#include "!Set LP.h"

inline Half DecalOpaqueFracMul() {return DecalParams.x;}
inline Half DecalOpaqueFracAdd() {return DecalParams.y;}
inline Half DecalAlpha        () {return DecalParams.z;}

// FULLSCREEN, MODE 0-default, 1-normals, 2-palette

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
   outVtx=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
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
              ):TARGET
{
   Vec  pos  =GetPosPoint(PixelToScreen(pixel));
        pos  =TransformTP(pos-inMatrix[3], (Matrix3)inMatrix);
   Half alpha=Sat(Half(Abs(pos.z))*DecalOpaqueFracMul()+DecalOpaqueFracAdd());

   clip(Vec(1-Abs(pos.xy), alpha-EPS_COL));
   alpha*=DecalAlpha();

   pos.xy=pos.xy*0.5+0.5;

   VecH4 col=Tex(Col, pos.xy);

#if MODE==2 // palette
   return (col.a*alpha)*Color[0]*Material.color;
#elif MODE==1 // normal
   // #MaterialTextureLayout
   Half  specular=tex_nrm.z*MaterialSpecular(); // specular is in 'nrm.z'

        VecH nrm;
             nrm.xy =Tex(Nrm, pos.xy).xy*Material.normal; // #MaterialTextureLayout
 //if(DETAIL)nrm.xy+=det.xy;
             nrm.z  =CalcZ(nrm.xy);
             nrm    =Transform(nrm, inMatrixN);

   col.a=tex_nrm.w*alpha; // alpha is in 'nrm.w' FIXME make NORMAL/LAYOUT independent
   col *=Color[0]*Material.color;

   #if SIGNED_NRM_RT
      outNrm.xyz=nrm;
   #else
      outNrm.xyz=nrm*0.5+0.5;
   #endif
      outNrm.w=col.a; // alpha

   return col;
#else
   col  *=Color[0]*Material.color;
   col.a*=alpha;
   return col;
#endif
}
/******************************************************************************/
