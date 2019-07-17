/******************************************************************************/
#include "!Header.h"
#include "Water.h"
/******************************************************************************/
#define DEFAULT_DEPTH 1.0
/******************************************************************************/
void Surface_VS
(
   VtxInput vtx,

   out Vec  outPos  :TEXCOORD0,
   out Vec2 outTex  :TEXCOORD1,
   out Vec4 outTexN0:TEXCOORD2,
   out Vec4 outTexN1:TEXCOORD3,
   out Vec4 outTexB :TEXCOORD4,
   out Half outPDF  :TEXCOORD5,
   out Vec4 outVtx  :POSITION ,

   uniform Bool waves,
   uniform Bool river,
   uniform Bool light=false
)
{
   Vec pos=vtx.pos();
   if(waves)
   {
      pos.y=pos.y*WaterYMulAdd.x + WaterYMulAdd.y;

      Vec dir =Normalize(Vec(FracToPosXY(pos.xy), 1));
      Flt dist=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlnPos, WaterPlnNrm, dir);
      if( dist>0)
      {
         pos=dir*dist;
      }else
      {
         pos=PointOnPlane(dir*Viewport.range, WaterPlnPos, WaterPlnNrm);
      }

      outPos=pos;
      pos   =Transform(pos, CamMatrix);
   }

   if(river)
   {
      Vec2 tex=vtx.tex(); tex.y-=WaterFlow;
      outTex     =( tex  )/WaterScaleDif  +      WaterOfs ;
      outTexN0.xy=( tex  )/WaterScaleNrm  +      WaterOfs ;
      outTexN0.zw=(-tex  )/WaterScaleNrm  +      WaterOfs ;
      outTexN1.xy=( tex/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexN1.zw=(-tex/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexB .xy=( tex  )/WaterScaleBump +      WaterOfs ;
      outTexB .zw=(-tex  )/WaterScaleBump +      WaterOfs ;
   }else
   {
      outTex     =( pos.xz  )/WaterScaleDif  +      WaterOfs ;
      outTexN0.xy=( pos.xz  )/WaterScaleNrm  +      WaterOfs ;
      outTexN0.zw=(-pos.xz  )/WaterScaleNrm  +      WaterOfs ;
      outTexN1.xy=( pos.xz/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexN1.zw=(-pos.xz/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexB .xy=( pos.xz  )/WaterScaleBump +      WaterOfs ;
      outTexB .zw=(-pos.xz  )/WaterScaleBump +      WaterOfs ;
   }

   if(waves)
   {
      Flt dist_scale=LerpRS(Sqr(150.0f), Sqr(100.0f), Length2(outPos)),
          bump      =TexLod(Col, outTexB.xy).a+TexLod(Col, outTexB.zw).a;
          bump      =bump-1; // Avg(a,b)*2-1 = (a+b)-1
          outPos   +=(WaterPlnNrm*WaterWave)*bump*dist_scale;
   }else
   {
      outPos=TransformPos(pos);
   }
   if(light)outPDF=2-Abs(DistPointPlane(outPos, WaterPlnPos, WaterPlnNrm)); // plane distance factor, must be = 1 for dist=0..1 (wave scale)
   outVtx=Project(outPos);
}
/******************************************************************************/

// Col, Nrm, Rfl = water material textures
// ImgXF = solid underwater depth
// these must be the same as "Apply" shader - Img1=reflection (2D image), Img2=solid underwater

void Surface_PS
(
   Vec  inPos  :TEXCOORD0,
   Vec2 inTex  :TEXCOORD1,
   Vec4 inTexN0:TEXCOORD2,
   Vec4 inTexN1:TEXCOORD3,
   Vec4 inTexB :TEXCOORD4,
   Half inPDF  :TEXCOORD5,
   PIXEL,

   out VecH4 O_col:TARGET0,
   out VecH4 O_nrm:TARGET1,

   uniform Bool waves                ,
   uniform Bool river                ,
   uniform Bool light                ,
   uniform Bool fake_reflection=false,
   uniform Int  shadow         =0    ,
   uniform Bool soft           =false
)
{
   VecH     nrm; // #MaterialTextureChannelOrder
            nrm.xy =(Tex(Nrm, inTexN0.xy).xy - Tex(Nrm, inTexN0.zw).xy + Tex(Nrm, inTexN1.xy).xy - Tex(Nrm, inTexN1.zw).xy)*WaterRgh_2; // (Avg(Tex(Nrm, inTexN0.xy).xy, 1-Tex(Nrm, inTexN0.zw).xy, Tex(Nrm, inTexN1.xy).xy, 1-Tex(Nrm, inTexN1.zw).xy)*2-1)*WaterRgh
   if(waves)nrm.xy+=(Tex(Nrm, inTexB .xy).xy - Tex(Nrm, inTexB .zw).xy                                                    )*WaterWave ; // (Avg(Tex(Nrm, inTexB .xy).xy, 1-Tex(Nrm, inTexB .zw).xy                                                    )*2-1)*WaterWave
            nrm.z  =CalcZ(nrm.xy);

   Matrix3 mtrx;
   mtrx[0]=MatrixX(ViewMatrix[0]);
   mtrx[1]=MatrixZ(ViewMatrix[0]);
   mtrx[2]=MatrixY(ViewMatrix[0]);

   VecH fresnel_nrm    =nrm;
        fresnel_nrm.xy*=WaterFresnelRough;
        fresnel_nrm.z  =CalcZ(fresnel_nrm.xy);
        fresnel_nrm    =Transform(fresnel_nrm, mtrx); // convert to view space
   VecH    view_nrm    =Transform(nrm        , mtrx); // convert to view space

   VecH view=Normalize(inPos);

   VecH4 col=VecH4(WaterCol*Tex(Col, inTex).rgb, 0);
   {
      if(fake_reflection) // add fake reflection
      {
         col.rgb+=TexCube(Rfl, Transform3(reflect(view, view_nrm), CamMatrix)).rgb*WaterRflFake; // #ShaderHalf
      }
      // fresnel
      {
         Half dot_prod=Sat(-Dot(view, fresnel_nrm)),
              fresnel =Pow(1-dot_prod, WaterFresnelPow);
         col.rgb+=fresnel*WaterFresnelColor;
      }
   }
   col.rgb=Sat(col.rgb);

   if(!light)
   {
      O_col=col; // in O_col.w you can encode: reflection, refraction, glow

   #if SIGNED_NRM_RT
      O_nrm.xyz=view_nrm;
   #else
      O_nrm.xyz=view_nrm*0.5+0.5; // -1..1 -> 0..1
   #endif

   #if SIGNED_NRM_RT && FULL_PRECISION_SPEC
      O_nrm.w=WaterSpc*2-1; // 0..1 -> -1..1
   #else
      O_nrm.w=WaterSpc;
   #endif
   }else
   {
           inTex      =PixelToScreen(pixel);
      Flt  water_z    =inPos.z,
           solid_z_raw=(soft ? TexPoint(ImgXF, inTex).x : 0),
           solid_z    =(soft ? LinearizeDepth(solid_z_raw) : water_z+DEFAULT_DEPTH);
      Half alpha=0;

      Vec2    col_tex=inTex;
      VecH4 water_col=col;

      Vec2 refract=nrm.xy*Viewport.size;

      Flt dz   =(soft ? solid_z-water_z : DEFAULT_DEPTH);
          alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);

      Vec2 test_tex=Mid(col_tex+refract*WaterRfr*alpha/Max(1, water_z), WaterClamp.xy, WaterClamp.zw);
      if(soft)
      {
         Flt test_z=LinearizeDepth(TexLodClamp(ImgXF, test_tex).x); // use linear filtering because texcoords are not rounded
         if( test_z>water_z)
         {
            solid_z=test_z;
            col_tex=test_tex;
         }
         if(DEPTH_BACKGROUND(solid_z_raw))alpha=1;else // always force full opacity when there's no solid pixel set to avoid remains in the RenderTarget from previous usage
         {
            dz   =solid_z-water_z;
            alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);
         }
      }else
      {
         col_tex=test_tex;
      }

      // light
      VecH4 lum;
      {
         // shadow
         Half shd; if(shadow)shd=Sat(ShadowDirValue(inPos, ShadowJitter(pixel.xy), true, shadow, false));

         // diffuse
         Half diffuse=LightDiffuse(view_nrm, Light_dir.dir); if(shadow)diffuse*=shd;

         // specular
         Half specular=LightSpecular(view_nrm, WaterSpc, Light_dir.dir, -view); if(shadow)specular*=shd;

         lum=VecH4(Light_dir.color.rgb*diffuse, Light_dir.color.a*specular);
      }

      // col light
      water_col.rgb*=lum.rgb+AmbNSColor;

      // reflection
      Half  rfl=WaterRfl*Sat(inPDF);
          inTex=Mid ((inTex+refract*WaterRfrRfl)*WaterRflMulAdd.xy+WaterRflMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
      water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl); // use LOD to avoid anisotropic going out of clamp region

      // glow and specular
    //water_col.a  +=lum.w*      0.5; // glow
      water_col.rgb+=lum.w*lum.w*0.5; // specular

      if(soft)
      {
              VecH4 solid_col=TexLodClamp(Img2, col_tex);
         O_col=Lerp(solid_col, water_col, alpha);
      }else
      {
         O_col=water_col;
      }
   }
}
/******************************************************************************/

// Img=Water RT Nrm (this is required for 'GetNormal', 'GetNormalMS', which are used for Lights - Dir, Point, etc.), ImgXF=WaterDepth, Img3=Water RT Col, Col=Water RT Lum
// these must be the same as "Surface" shader - Img1=reflection (2D image), Img2=solid underwater

VecH4 Apply_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1,
                   out Flt  depth  :DEPTH    ,
               uniform Bool refract          ,
               uniform Bool set_depth=false  ):TARGET
{
   Flt  water_z    =TexPoint        (ImgXF, inTex).x,
        solid_z_raw=TexDepthRawPoint(       inTex);
   Half alpha=0;

 //if(set_depth) FIXME make this optional depending on macro
      depth=water_z;

   if(refract)
   {
      Vec2  col_tex=inTex;
      VecH4 water_col=0;

      BRANCH if(DEPTH_SMALLER(water_z, solid_z_raw)) // branch works faster when most of the screen is above water
      {
             water_z=LinearizeDepth(water_z    );
         Flt solid_z=LinearizeDepth(solid_z_raw);

         water_col=TexLod(Img3, inTex);
         VecH4 lum=TexLod(Col , inTex); // water surface light
         VecH  nrm=GetNormal(inTex, false).xyz; // water surface normals

         MatrixH3 mtrx;
         mtrx[0]=MatrixX(ViewMatrix[0]);
         mtrx[1]=MatrixZ(ViewMatrix[0]);
         mtrx[2]=MatrixY(ViewMatrix[0]);
         nrm=TransformTP(nrm, mtrx);
         Vec2 refract=nrm.xy*Viewport.size;

         Flt dz   =solid_z-water_z;
             alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);

         Vec2 test_tex=Mid(col_tex+refract*WaterRfr*alpha/Max(1, water_z), WaterClamp.xy, WaterClamp.zw);
         Flt  test_z  =TexDepthLinear(test_tex); // use linear filtering because texcoords are not rounded
         if(  test_z  >water_z)
         {
            solid_z=test_z;
            col_tex=test_tex;
         }

         if(DEPTH_BACKGROUND(solid_z_raw))alpha=1;else // always force full opacity when there's no solid pixel set to avoid remains in the RenderTarget from previous usage
         {
            dz   =solid_z-water_z;
            alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);
         }

         // col light
         water_col.rgb*=lum.rgb;

         // reflection
         Vec   pos=Vec(inPosXY*water_z, water_z);
         Half  pdf=Sat(2-Abs(DistPointPlane(pos, WaterPlnPos, WaterPlnNrm))), // plane distance factor, must be = 1 for dist=0..1 (wave scale)
               rfl=WaterRfl*pdf;
             inTex=Mid ((inTex+refract*WaterRfrRfl)*WaterRflMulAdd.xy+WaterRflMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
         water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl); // use LOD to avoid anisotropic going out of clamp region

         // glow and specular
       //water_col.a  +=lum.w*      0.5; // glow
         water_col.rgb+=lum.w*lum.w*0.5; // specular
      }
            VecH4 solid_col=TexClamp(Img2, col_tex);
      return Lerp(solid_col, water_col, alpha);
   }else
   {
      VecH4 solid_col=TexLod(Img2, inTex);
      BRANCH if(DEPTH_SMALLER(water_z, solid_z_raw)) // branch works faster when most of the screen is above water
      {
             water_z=LinearizeDepth(water_z    );
         Flt solid_z=LinearizeDepth(solid_z_raw);

         if(DEPTH_BACKGROUND(solid_z_raw))alpha=1;else // always force full opacity when there's no solid pixel set to avoid remains in the RenderTarget from previous usage
         {
            Flt dz   =solid_z-water_z;
                alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);
         }

         VecH4 water_col=TexLod(Img3, inTex),
                     lum=TexLod(Col , inTex); // water surface light
         VecH        nrm=GetNormal(inTex, false).xyz; // water surface normals

         MatrixH3 mtrx;
         mtrx[0]=MatrixX(ViewMatrix[0]);
         mtrx[1]=MatrixZ(ViewMatrix[0]);
         mtrx[2]=MatrixY(ViewMatrix[0]);
         nrm=TransformTP(nrm, mtrx);
         Vec2 refract=nrm.xy*Viewport.size;

         // col light
         water_col.rgb*=lum.rgb;

         // reflection
         Vec   pos=Vec(inPosXY*water_z, water_z);
         Half  pdf=Sat(2-Abs(DistPointPlane(pos, WaterPlnPos, WaterPlnNrm))), // plane distance factor, needs to be = 1 for dist=0..1 (wave scale)
               rfl=WaterRfl*pdf;
             inTex=Mid ((inTex+refract*WaterRfrRfl)*WaterRflMulAdd.xy+WaterRflMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
         water_col=Lerp(water_col, TexLod(Img1, inTex), rfl);

         // glow and specular
       //water_col.a  +=lum.w*      0.5; // glow
         water_col.rgb+=lum.w*lum.w*0.5; // specular

         solid_col=Lerp(solid_col, water_col, alpha);
      }
      return solid_col;
   }
}
/******************************************************************************/
VecH4 Under_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1,
               uniform Bool refract          ):TARGET
{
   // underwater refraction
   if(refract)
   {
      Flt dist      =Viewport.range;
      Flt to_surface=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlnPos, WaterPlnNrm, Normalize(Vec(inPosXY, 1)));
      if( to_surface>0)dist=Min(to_surface, dist);

      Flt refract_len=Sat(AccumulatedDensity(WaterDns.x, dist)+WaterDns.y)*WaterUnderRfr;

   #if 1 // viewport size adjusted
      inTex+=Sin(inTex.yx*14/Viewport.size+Step)*refract_len*Viewport.size; // add refraction
      inTex =(inTex-Viewport.center)/(1+2*refract_len)+Viewport.center; // scale texture coordinates to avoid clamping
   #else
      inTex+=Sin(inTex.yx*14+Step)*refract_len; // add refraction
      inTex =(inTex-0.5)/(1+2*refract_len)+0.5; // scale texture coordinates to avoid clamping
   #endif
   }

   Vec pos       =(refract ? GetPosLinear(inTex) : GetPosPoint(inTex, inPosXY));
   Flt dist      =Length(pos);
   Vec ray       =pos/dist; // Normalize(pos); should be no NaN because pos.z should be always >0
   Flt to_surface=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlnPos, WaterPlnNrm, ray);
   if( to_surface>0)dist=Min(to_surface, dist);

   Half opacity=Sat(AccumulatedDensity(WaterDns.x, dist)+WaterDns.y)*WaterUnder;

   Flt depth_0=-DistPointPlane(Vec(0, 0, 0), WaterPlnPos, WaterPlnNrm),
       depth_1=-DistPointPlane(ray*dist    , WaterPlnPos, WaterPlnNrm);

   Half water_density;

/* Proper function:
   {
      water_density=0;

      Int steps        =Mid(dist, 1, 255);
      Flt opacity      =1,
          total_opacity=0;
      LOOP for(Int i=0; i<steps; i++)
      {
         Flt depth=Lerp(depth_0, depth_1, i/Flt(steps-1));
         Flt d    =AccumulatedDensity(WaterDns.x, depth);
         opacity      *=1-WaterDns.x;
         water_density+=opacity*d;
         total_opacity+=opacity;
      }
      water_density/=total_opacity;
   }
   if(BOOL)
/**/
   // approximation:
   {
      Half density_0=AccumulatedDensity(WaterDns.x, depth_0),
           density_1=AccumulatedDensity(WaterDns.x, depth_1),
           blend    =0.5/(1+dist*(WaterDns.x/3));
      water_density=Lerp(density_0, density_1, blend);
   }

   VecH water_col=Lerp(WaterUnderCol0, WaterUnderCol1, water_density);

   return refract ? Lerp(TexLod(Img, inTex), VecH4(water_col, 0), opacity)
                  :                          VecH4(water_col    , opacity);
}
/******************************************************************************/
TECHNIQUE(Apply , DrawPosXY_VS(), Apply_PS(false));
TECHNIQUE(ApplyR, DrawPosXY_VS(), Apply_PS(true ));

TECHNIQUE(ApplyD , DrawPosXY_VS(), Apply_PS(false, true));
TECHNIQUE(ApplyRD, DrawPosXY_VS(), Apply_PS(true , true));

TECHNIQUE(Under , DrawPosXY_VS(), Under_PS(false));
TECHNIQUE(UnderR, DrawPosXY_VS(), Under_PS(true ));

TECHNIQUE(River , Surface_VS(false, true ), Surface_PS(false, true , false, false));
TECHNIQUE(RiverF, Surface_VS(false, true ), Surface_PS(false, true , false, true ));
TECHNIQUE(Lake  , Surface_VS(false, false), Surface_PS(false, false, false, false));
TECHNIQUE(LakeF , Surface_VS(false, false), Surface_PS(false, false, false, true ));
TECHNIQUE(Ocean , Surface_VS(true , false), Surface_PS(true , false, false, false));
TECHNIQUE(OceanF, Surface_VS(true , false), Surface_PS(true , false, false, true ));

TECHNIQUE(RiverL0  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 0, false));
TECHNIQUE(RiverL0F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 0, false));
TECHNIQUE(RiverL1  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 1, false));
TECHNIQUE(RiverL1F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 1, false));
TECHNIQUE(RiverL2  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 2, false));
TECHNIQUE(RiverL2F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 2, false));
TECHNIQUE(RiverL3  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 3, false));
TECHNIQUE(RiverL3F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 3, false));
TECHNIQUE(RiverL4  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 4, false));
TECHNIQUE(RiverL4F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 4, false));
TECHNIQUE(RiverL5  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 5, false));
TECHNIQUE(RiverL5F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 5, false));
TECHNIQUE(RiverL6  , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 6, false));
TECHNIQUE(RiverL6F , Surface_VS(false, true , true), Surface_PS(false, true , true, true , 6, false));
TECHNIQUE(RiverL0S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 0, true ));
TECHNIQUE(RiverL0SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 0, true ));
TECHNIQUE(RiverL1S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 1, true ));
TECHNIQUE(RiverL1SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 1, true ));
TECHNIQUE(RiverL2S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 2, true ));
TECHNIQUE(RiverL2SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 2, true ));
TECHNIQUE(RiverL3S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 3, true ));
TECHNIQUE(RiverL3SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 3, true ));
TECHNIQUE(RiverL4S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 4, true ));
TECHNIQUE(RiverL4SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 4, true ));
TECHNIQUE(RiverL5S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 5, true ));
TECHNIQUE(RiverL5SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 5, true ));
TECHNIQUE(RiverL6S , Surface_VS(false, true , true), Surface_PS(false, true , true, false, 6, true ));
TECHNIQUE(RiverL6SF, Surface_VS(false, true , true), Surface_PS(false, true , true, true , 6, true ));
TECHNIQUE(LakeL0   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 0, false));
TECHNIQUE(LakeL0F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 0, false));
TECHNIQUE(LakeL1   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 1, false));
TECHNIQUE(LakeL1F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 1, false));
TECHNIQUE(LakeL2   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 2, false));
TECHNIQUE(LakeL2F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 2, false));
TECHNIQUE(LakeL3   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 3, false));
TECHNIQUE(LakeL3F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 3, false));
TECHNIQUE(LakeL4   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 4, false));
TECHNIQUE(LakeL4F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 4, false));
TECHNIQUE(LakeL5   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 5, false));
TECHNIQUE(LakeL5F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 5, false));
TECHNIQUE(LakeL6   , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 6, false));
TECHNIQUE(LakeL6F  , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 6, false));
TECHNIQUE(LakeL0S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 0, true ));
TECHNIQUE(LakeL0SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 0, true ));
TECHNIQUE(LakeL1S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 1, true ));
TECHNIQUE(LakeL1SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 1, true ));
TECHNIQUE(LakeL2S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 2, true ));
TECHNIQUE(LakeL2SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 2, true ));
TECHNIQUE(LakeL3S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 3, true ));
TECHNIQUE(LakeL3SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 3, true ));
TECHNIQUE(LakeL4S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 4, true ));
TECHNIQUE(LakeL4SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 4, true ));
TECHNIQUE(LakeL5S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 5, true ));
TECHNIQUE(LakeL5SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 5, true ));
TECHNIQUE(LakeL6S  , Surface_VS(false, false, true), Surface_PS(false, false, true, false, 6, true ));
TECHNIQUE(LakeL6SF , Surface_VS(false, false, true), Surface_PS(false, false, true, true , 6, true ));
TECHNIQUE(OceanL0  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 0, false));
TECHNIQUE(OceanL0F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 0, false));
TECHNIQUE(OceanL1  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 1, false));
TECHNIQUE(OceanL1F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 1, false));
TECHNIQUE(OceanL2  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 2, false));
TECHNIQUE(OceanL2F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 2, false));
TECHNIQUE(OceanL3  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 3, false));
TECHNIQUE(OceanL3F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 3, false));
TECHNIQUE(OceanL4  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 4, false));
TECHNIQUE(OceanL4F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 4, false));
TECHNIQUE(OceanL5  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 5, false));
TECHNIQUE(OceanL5F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 5, false));
TECHNIQUE(OceanL6  , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 6, false));
TECHNIQUE(OceanL6F , Surface_VS(true , false, true), Surface_PS(true , false, true, true , 6, false));
TECHNIQUE(OceanL0S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 0, true ));
TECHNIQUE(OceanL0SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 0, true ));
TECHNIQUE(OceanL1S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 1, true ));
TECHNIQUE(OceanL1SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 1, true ));
TECHNIQUE(OceanL2S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 2, true ));
TECHNIQUE(OceanL2SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 2, true ));
TECHNIQUE(OceanL3S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 3, true ));
TECHNIQUE(OceanL3SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 3, true ));
TECHNIQUE(OceanL4S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 4, true ));
TECHNIQUE(OceanL4SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 4, true ));
TECHNIQUE(OceanL5S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 5, true ));
TECHNIQUE(OceanL5SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 5, true ));
TECHNIQUE(OceanL6S , Surface_VS(true , false, true), Surface_PS(true , false, true, false, 6, true ));
TECHNIQUE(OceanL6SF, Surface_VS(true , false, true), Surface_PS(true , false, true, true , 6, true ));
/******************************************************************************/
