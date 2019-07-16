/******************************************************************************/
#include "!Header.h"
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
           out Matrix3 outMat:TEXCOORD2,
       uniform Int     inside          )
{
   outMat[0]=Normalize(MatrixX(ViewMatrix[0]));
   outMat[1]=Normalize(MatrixY(ViewMatrix[0]));
   outMat[2]=Normalize(MatrixZ(ViewMatrix[0]));

   // convert to texture space (0..1)
   if(inside)outTex=Volume.inside/(2*Volume.size)+0.5;
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
   out VecH4 mask :TARGET1,

   uniform Int  inside,
   uniform Bool LA=false
)
{
   Flt z  =TexDepthPoint(PixelToScreen(pixel));
   Vec pos=inTex;
   Vec dir=Normalize(inPos); dir*=Min((SQRT3*2)*Max(Volume.size), (z-(inside ? Viewport.from : inPos.z))/dir.z);
       dir=TransformTP(dir, inMat); // convert to box space

   // convert to texture space (0..1)
   dir=dir/(2*Volume.size);

   if(inside==1)
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
      mask.rgb=0; mask.a=color.a;
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
      mask.rgb=0; mask.a=color.a;
   }
}
TECHNIQUE(Volume0, Volume_VS(0), Volume_PS(0));
TECHNIQUE(Volume1, Volume_VS(1), Volume_PS(1));
TECHNIQUE(Volume2, Volume_VS(2), Volume_PS(2));

TECHNIQUE(Volume0LA, Volume_VS(0), Volume_PS(0, true));
TECHNIQUE(Volume1LA, Volume_VS(1), Volume_PS(1, true));
TECHNIQUE(Volume2LA, Volume_VS(2), Volume_PS(2, true));
/******************************************************************************/
// LASER
/******************************************************************************/
void Laser_VS(VtxInput vtx,
          out Vec  outPos:TEXCOORD0,
          out VecH outNrm:TEXCOORD1,
          out Vec4 outVtx:POSITION ,
      uniform Bool normals         )
{
       if(normals)outNrm=TransformDir(vtx.nrm());
   outVtx=Project(outPos=TransformPos(vtx.pos()));
}
void Laser_PS(Vec                 inPos:TEXCOORD0,
              VecH                inNrm:TEXCOORD1,
          out DeferredSolidOutput output         ,
      uniform Bool                normals        )
{
   if(normals)
   {
          inNrm=Normalize(inNrm);
      Half  stp=Max (-Dot(inNrm, Normalize(inPos)), -inNrm.z);
            stp=Sat (stp);
            stp=Pow (stp, Step);
      VecH4 col=Lerp(Color[0], Color[1], stp);
      output.color(col.rgb);
      output.glow (col.a  );
   }else
   {
      output.color(Color[0].rgb);
      output.glow (Color[0].a  );
   }
   output.normal  (0);
   output.specular(0);
   output.velocity(0, inPos);
}
TECHNIQUE(Laser , Laser_VS(false), Laser_PS(false));
TECHNIQUE(LaserN, Laser_VS(true ), Laser_PS(true ));
/******************************************************************************/
// DECAL
/******************************************************************************/
#include "!Set HP.h"
BUFFER(Decal)
   VecH DecalParams; // x=OpaqueFracMul, y=OpaqueFracAdd, z=alpha
BUFFER_END
#include "!Set LP.h"

inline Half DecalOpaqueFracMul() {return DecalParams.x;}
inline Half DecalOpaqueFracAdd() {return DecalParams.y;}
inline Half DecalAlpha        () {return DecalParams.z;}

void Decal_VS(VtxInput vtx,
          out Vec4    outVtx    :POSITION ,
          out Matrix  outMatrix :TEXCOORD0,
          out Matrix3 outMatrixN:TEXCOORD3,
      uniform Bool    fullscreen          ,
      uniform Bool    normal              ,
      uniform Bool    palette             )
{
   outMatrix=ViewMatrix[0];
   outMatrix[0]/=Length2(outMatrix[0]);
   outMatrix[1]/=Length2(outMatrix[1]);
   outMatrix[2]/=Length2(outMatrix[2]);

   if(!palette && normal)
   {
      outMatrixN[0]=Normalize(outMatrix[0]);
      outMatrixN[1]=Normalize(outMatrix[1]);
      outMatrixN[2]=Normalize(outMatrix[2]);
   }

   if(fullscreen)
   {
      outVtx=Vec4(vtx.pos2(), !REVERSE_DEPTH, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
   }else
   {
      outVtx=Project(TransformPos(vtx.pos()));
   }
}
VecH4 Decal_PS(PIXEL,
               Matrix  inMatrix :TEXCOORD0,
               Matrix3 inMatrixN:TEXCOORD3,
           out VecH4   outNrm   :TARGET1  ,
       uniform Bool    normal             ,
       uniform Bool    palette            ):TARGET
{
   Vec  pos  =GetPosPoint(PixelToScreen(pixel));
        pos  =TransformTP(pos-inMatrix[3], (Matrix3)inMatrix);
   Half alpha=Sat(Half(Abs(pos.z))*DecalOpaqueFracMul()+DecalOpaqueFracAdd());
 
   clip(Vec(1-Abs(pos.xy), alpha-EPS_COL));
   alpha*=DecalAlpha();

   pos.xy=pos.xy*0.5+0.5;

   VecH4 col=Tex(Col, pos.xy);

   if(palette)
   {
      return (col.a*alpha)*Color[0]*MaterialColor();
   }else
   {
      if(normal)
      {
         VecH4 tex_nrm =Tex(Nrm, pos.xy); // #MaterialTextureChannelOrder
         Half  specular=tex_nrm.z*MaterialSpecular(); // specular is in 'nrm.z'

              VecH nrm;
                   nrm.xy =(tex_nrm.xy*2-1)*MaterialRough(); // normal is in 'nrm.xy'
       //if(detail)nrm.xy+=det.xy;
                   nrm.z  =CalcZ(nrm.xy);
                   nrm    =Transform(nrm, inMatrixN);

         col.a=tex_nrm.w*alpha; // alpha is in 'nrm.w'
         col *=Color[0]*MaterialColor();

      #if SIGNED_NRM_RT
         outNrm.xyz=nrm;
      #else
         outNrm.xyz=nrm*0.5+0.5;
      #endif
         outNrm.w=col.a; // alpha
      }else
      {
         col  *=Color[0]*MaterialColor();
         col.a*=alpha;
      }
      return col;
   }
}
TECHNIQUE(Decal  , Decal_VS(false, false, false), Decal_PS(false, false));
TECHNIQUE(DecalN , Decal_VS(false, true , false), Decal_PS(true , false));
TECHNIQUE(DecalP , Decal_VS(false, false, true ), Decal_PS(false, true ));
TECHNIQUE(DecalF , Decal_VS(true , false, false), Decal_PS(false, false));
TECHNIQUE(DecalFN, Decal_VS(true , true , false), Decal_PS(true , false));
TECHNIQUE(DecalFP, Decal_VS(true , false, true ), Decal_PS(false, true ));
/******************************************************************************/
