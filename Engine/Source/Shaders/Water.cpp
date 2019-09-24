/******************************************************************************/
#include "!Header.h"
#include "Water.h"
/******************************************************************************/
#define DEFAULT_DEPTH 1.0
/******************************************************************************/
// LIGHT, SHADOW, SOFT, FAKE_REFLECTION, WAVES, RIVER
#ifndef WAVES
#define WAVES 0
#endif
#ifndef RIVER
#define RIVER 0
#endif
#ifndef REFRACT
#define REFRACT 0
#endif
void Surface_VS
(
   VtxInput vtx,

   out Vec  outPos  :TEXCOORD0,
   out Vec2 outTex  :TEXCOORD1,
   out Vec4 outTexN0:TEXCOORD2,
   out Vec4 outTexN1:TEXCOORD3,
#if WAVES
   out Vec4 outTexB :TEXCOORD4,
#endif
#if LIGHT
   out Half outPDF  :TEXCOORD5,
#endif
   out Vec4 outVtx  :POSITION
)
{
   Vec pos=vtx.pos();
   if(WAVES)
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

   if(RIVER)
   {
      Vec2 tex=vtx.tex(); tex.y-=WaterFlow;
      outTex     =( tex  )/WaterScaleDif  +      WaterOfs ;
      outTexN0.xy=( tex  )/WaterScaleNrm  +      WaterOfs ;
      outTexN0.zw=(-tex  )/WaterScaleNrm  +      WaterOfs ;
      outTexN1.xy=( tex/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexN1.zw=(-tex/8)/WaterScaleNrm  + Perp(WaterOfs);
   #if WAVES
      outTexB .xy=( tex  )/WaterScaleBump +      WaterOfs ;
      outTexB .zw=(-tex  )/WaterScaleBump +      WaterOfs ;
   #endif
   }else
   {
      outTex     =( pos.xz  )/WaterScaleDif  +      WaterOfs ;
      outTexN0.xy=( pos.xz  )/WaterScaleNrm  +      WaterOfs ;
      outTexN0.zw=(-pos.xz  )/WaterScaleNrm  +      WaterOfs ;
      outTexN1.xy=( pos.xz/8)/WaterScaleNrm  + Perp(WaterOfs);
      outTexN1.zw=(-pos.xz/8)/WaterScaleNrm  + Perp(WaterOfs);
   #if WAVES
      outTexB .xy=( pos.xz  )/WaterScaleBump +      WaterOfs ;
      outTexB .zw=(-pos.xz  )/WaterScaleBump +      WaterOfs ;
   #endif
   }

#if WAVES
   Flt dist_scale=LerpRS(Sqr(150.0f), Sqr(100.0f), Length2(outPos)),
       bump      =TexLod(Col, outTexB.xy).a+TexLod(Col, outTexB.zw).a;
       bump      =bump-1; // Avg(a,b)*2-1 = (a+b)-1
       outPos   +=(WaterPlnNrm*WaterWave)*bump*dist_scale;
#else
   outPos=TransformPos(pos);
#endif

#if LIGHT
   outPDF=2-Abs(DistPointPlane(outPos, WaterPlnPos, WaterPlnNrm)); // plane distance factor, must be = 1 for dist=0..1 (wave scale)
#endif

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
#if WAVES
   Vec4 inTexB :TEXCOORD4,
#endif
#if LIGHT
   Half inPDF  :TEXCOORD5,
#endif
   PIXEL,

   out VecH4 O_col:TARGET0
#if !LIGHT
 , out VecH4 O_nrm:TARGET1
#endif
) // #RTOutput
{
   VecH nrm; // #MaterialTextureLayout
        nrm.xy=(Tex(Nrm, inTexN0.xy).xy - Tex(Nrm, inTexN0.zw).xy + Tex(Nrm, inTexN1.xy).xy - Tex(Nrm, inTexN1.zw).xy)*WaterRgh_4; // Avg(Tex(Nrm, inTexN0.xy).xy, -Tex(Nrm, inTexN0.zw).xy, Tex(Nrm, inTexN1.xy).xy, -Tex(Nrm, inTexN1.zw).xy))*WaterRgh
#if WAVES
   nrm.xy+=(Tex(Nrm, inTexB.xy).xy - Tex(Nrm, inTexB.zw).xy)*(WaterWave*0.5f); // Avg(Tex(Nrm, inTexB.xy).xy, -Tex(Nrm, inTexB.zw).xy))*WaterWave
#endif
   nrm.z=CalcZ(nrm.xy);

   Matrix3 mtrx;
   mtrx[0]=ViewMatrixX();
   mtrx[1]=ViewMatrixZ();
   mtrx[2]=ViewMatrixY();

   VecH fresnel_nrm    =nrm;
        fresnel_nrm.xy*=WaterFresnelRough;
        fresnel_nrm.z  =CalcZ(fresnel_nrm.xy);
        fresnel_nrm    =Transform(fresnel_nrm, mtrx); // convert to view space
   VecH    view_nrm    =Transform(nrm        , mtrx); // convert to view space

   VecH view=Normalize(inPos);

   VecH4 col=VecH4(WaterCol*Tex(Col, inTex).rgb, 0);
   {
   #if FAKE_REFLECTION // add fake reflection
      col.rgb=Lerp(col.rgb, TexCube(Env, Transform3(reflect(view, view_nrm), CamMatrix)).rgb*EnvColor, WaterRflFake); // #ShaderHalf
   #endif
      // fresnel
      {
         Half dot_prod=Sat(-Dot(view, fresnel_nrm)),
              fresnel =Pow(1-dot_prod, WaterFresnelPow);
         col.rgb+=fresnel*WaterFresnelColor;
      }
   }
   col.rgb=Sat(col.rgb);

#if !LIGHT
   O_col=col; // in O_col.w you can encode: reflection, refraction, glow

   #if SIGNED_NRM_RT
      O_nrm.xyz=view_nrm;
   #else
      O_nrm.xyz=view_nrm*0.5+0.5; // -1..1 -> 0..1
   #endif

   #if SIGNED_NRM_RT && FULL_PRECISION_SMOOTH
      O_nrm.w=WaterSpc*2-1; // 0..1 -> -1..1
   #else
      O_nrm.w=WaterSpc;
   #endif
#else
        inTex      =PixelToScreen(pixel);
   Flt  water_z    =inPos.z,
        solid_z_raw=(SOFT ? TexPoint(ImgXF, inTex).x : 0),
        solid_z    =(SOFT ? LinearizeDepth(solid_z_raw) : water_z+DEFAULT_DEPTH);
   Half alpha=0;

   Vec2    col_tex=inTex;
   VecH4 water_col=col;

   Vec2 refract=nrm.xy*Viewport.size;

   Flt dz   =(SOFT ? solid_z-water_z : DEFAULT_DEPTH);
       alpha=Sat(AccumulatedDensity(WaterDns.x, dz) + WaterDns.y)*Sat(dz/0.03);

   Vec2 test_tex=Mid(col_tex+refract*WaterRfr*alpha/Max(1, water_z), WaterClamp.xy, WaterClamp.zw);
   if(SOFT)
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
      Half shd; if(SHADOW)shd=Sat(ShadowDirValue(inPos, ShadowJitter(pixel.xy), true, SHADOW, false));

      // diffuse
      Half diffuse=LightDiffuse(view_nrm, LightDir.dir); if(SHADOW)diffuse*=shd;

      // specular
      Half specular=LightSpecular(view_nrm, WaterSpc, LightDir.dir, -view); if(SHADOW)specular*=shd;

      lum=VecH4(LightDir.color.rgb*diffuse, LightDir.color.a*specular);
   }

   // col light
   water_col.rgb*=lum.rgb+AmbientNSColor;

   // reflection
   Half  rfl=WaterRfl*Sat(inPDF);
       inTex=Mid ((inTex+refract*WaterRfrRfl)*WaterRflMulAdd.xy+WaterRflMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
   water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl); // use LOD to avoid anisotropic going out of clamp region

   // glow and specular
   //water_col.a  +=lum.w*      0.5; // glow
   water_col.rgb+=lum.w*lum.w*0.5; // specular

   if(SOFT)
   {
           VecH4 solid_col=TexLodClamp(Img2, col_tex);
      O_col=Lerp(solid_col, water_col, alpha);
   }else
   {
      O_col=water_col;
   }
#endif
}
/******************************************************************************/

// Img=Water RT Nrm (this is required for 'GetNormal', 'GetNormalMS', which are used for Lights - Dir, Point, etc.), ImgXF=WaterDepth, Img3=Water RT Col, Col=Water RT Lum
// these must be the same as "Surface" shader - Img1=reflection (2D image), Img2=solid underwater
// REFRACT, SET_DEPTH

VecH4 Apply_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1
            #if SET_DEPTH
                 , out Flt  depth  :DEPTH
            #endif
              ):TARGET
{
   Flt  water_z    =TexPoint        (ImgXF, inTex).x,
        solid_z_raw=TexDepthRawPoint(       inTex);
   Half alpha=0;

#if SET_DEPTH
   depth=water_z;
#endif

   if(REFRACT)
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
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
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
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
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
// REFRACT
VecH4 Under_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1):TARGET
{
   // underwater refraction
   if(REFRACT)
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

   Vec pos       =(REFRACT ? GetPosLinear(inTex) : GetPosPoint(inTex, inPosXY));
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

   return REFRACT ? Lerp(TexLod(Img, inTex), VecH4(water_col, 0), opacity)
                  :                          VecH4(water_col    , opacity);
}
/******************************************************************************/
