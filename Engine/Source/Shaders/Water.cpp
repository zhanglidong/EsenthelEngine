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
      Flt dist=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm, dir);
      if( dist>0)
      {
         pos=dir*dist;
      }else
      {
         pos=PointOnPlane(dir*Viewport.range, WaterPlanePos, WaterPlaneNrm);
      }

      outPos=pos;
      pos   =Transform(pos, CamMatrix);
   }

   Vec2 tex;
   if(RIVER){tex=vtx.tex(); tex.y-=WaterFlow;}
   else     {tex=pos.xz;}
   outTex     =tex*  WaterMaterial.scale_color    +     WaterOfs ;
   outTexN0.xy=tex*  WaterMaterial.scale_normal   +     WaterOfs ;
   outTexN0.zw=tex* -WaterMaterial.scale_normal   +     WaterOfs ;
   outTexN1.xy=tex*( WaterMaterial.scale_normal/8)+Perp(WaterOfs);
   outTexN1.zw=tex*(-WaterMaterial.scale_normal/8)+Perp(WaterOfs);
#if WAVES
   outTexB .xy=tex*  WaterMaterial.scale_bump     +     WaterOfs ;
   outTexB .zw=tex* -WaterMaterial.scale_bump     +     WaterOfs ;
#endif

#if WAVES // #WaterMaterialTextureLayout
   Flt dist_scale=LerpRS(Sqr(150), Sqr(100), Length2(outPos)),
       bump      =TexLod(Col, outTexB.xy).a+TexLod(Col, outTexB.zw).a;
       bump      =bump-1; // Avg(a,b)*2-1 = (a+b)-1
       outPos   +=WaterPlaneNrm*(WaterMaterial.wave_scale*bump*dist_scale);
#else
   outPos=TransformPos(pos);
#endif

#if LIGHT
   outPDF=2-Abs(DistPointPlane(outPos, WaterPlanePos, WaterPlaneNrm)); // plane distance factor, must be = 1 for dist=0..1 (wave scale)
#endif

   outVtx=Project(outPos);
}
/******************************************************************************/

// Col, Nrm = water material textures
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
 , out VecH  O_nrm:TARGET1
#endif
) // #RTOutput
{
   VecH nrm; // #WaterMaterialTextureLayout
        nrm.xy=(Tex(Nrm, inTexN0.xy).xy - Tex(Nrm, inTexN0.zw).xy + Tex(Nrm, inTexN1.xy).xy - Tex(Nrm, inTexN1.zw).xy)*(WaterMaterial.normal/4); // Avg(Tex(Nrm, inTexN0.xy).xy, -Tex(Nrm, inTexN0.zw).xy, Tex(Nrm, inTexN1.xy).xy, -Tex(Nrm, inTexN1.zw).xy))*WaterMaterial.normal
#if WAVES // FIXME perhaps make it dependent on bump from VS, use some ddx (+delta) and scale by wave_scale
   nrm.xy+=(Tex(Nrm, inTexB.xy).xy - Tex(Nrm, inTexB.zw).xy)*(WaterMaterial.wave_scale*0.5); // Avg(Tex(Nrm, inTexB.xy).xy, -Tex(Nrm, inTexB.zw).xy))*WaterMaterial.wave_scale
#endif
   nrm.z=CalcZ(nrm.xy);

   Matrix3 mtrx;
   mtrx[0]=ViewMatrixX();
   mtrx[1]=ViewMatrixZ();
   mtrx[2]=ViewMatrixY();

   /*VecH fresnel_nrm    =nrm;
        fresnel_nrm.xy*=WaterFresnelRough;
        fresnel_nrm.z  =CalcZ(fresnel_nrm.xy);
        fresnel_nrm    =Transform(fresnel_nrm, mtrx); // convert to view space*/
   VecH    view_nrm    =Transform(nrm        , mtrx); // convert to view space

   VecH view=Normalize(inPos);

   VecH4 col=VecH4(Tex(Col, inTex).rgb*WaterMaterial.color, 0);
   /*{
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
   col.rgb=Sat(col.rgb);*/

#if !LIGHT
   O_col=col; // in O_col.w you can encode: reflection, refraction, glow

   #if SIGNED_NRM_RT
      O_nrm.xyz=view_nrm;
   #else
      O_nrm.xyz=view_nrm*0.5+0.5; // -1..1 -> 0..1
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
       alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add)*Sat(dz/0.03);

   Vec2 test_tex=Mid(col_tex+refract*(WaterMaterial.refract*alpha/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
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
         alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add)*Sat(dz/0.03);
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
      Half specular=LightSpecular(view_nrm, WaterMaterial.smooth, WaterMaterial.reflect, LightDir.dir, view); if(SHADOW)specular*=shd;

      lum=VecH4(LightDir.color.rgb*diffuse, LightDir.color.a*specular);
   }

   // col light
   water_col.rgb*=lum.rgb+AmbientNSColor;

   // reflection
   //Half  rfl=WaterRfl*Sat(inPDF);
       inTex=Mid ((inTex+refract*WaterMaterial.refract_reflection)*WaterReflectMulAdd.xy+WaterReflectMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
   //FIXME water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl); // use LOD to avoid anisotropic going out of clamp region

   // specular
   water_col.rgb+=lum.w*lum.w*0.5;

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

         water_col.rgb=TexLod(Img3, inTex).rgb; // water surface color
         VecH4 lum=TexLod(Col, inTex); // water surface light
         VecH  nrm=GetNormal(inTex).xyz; // water surface normals

         MatrixH3 mtrx;
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
         nrm=TransformTP(nrm, mtrx);
         Vec2 refract=nrm.xy*Viewport.size;

         Flt dz   =solid_z-water_z;
             alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add)*Sat(dz/0.03);

         Vec2 test_tex=Mid(col_tex+refract*(WaterMaterial.refract*alpha/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
         Flt  test_z  =TexDepthLinear(test_tex); // use linear filtering because texcoords are not rounded
         if(  test_z  >water_z)
         {
            solid_z=test_z;
            col_tex=test_tex;
         }

         if(DEPTH_BACKGROUND(solid_z_raw))alpha=1;else // always force full opacity when there's no solid pixel set to avoid remains in the RenderTarget from previous usage
         {
            dz   =solid_z-water_z;
            alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add)*Sat(dz/0.03);
         }

         // col light
         water_col.rgb*=lum.rgb;

         // reflection FIXME
         /*Vec   pos=Vec(inPosXY*water_z, water_z);
         Half  pdf=Sat(2-Abs(DistPointPlane(pos, WaterPlanePos, WaterPlaneNrm))), // plane distance factor, must be = 1 for dist=0..1 (wave scale)
               rfl=WaterRfl*pdf;
             inTex=Mid ((inTex+refract*WaterMaterial.refract_reflection)*WaterReflectMulAdd.xy+WaterReflectMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
         water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl); // use LOD to avoid anisotropic going out of clamp region*/

         // specular
         water_col.rgb+=lum.w*lum.w*0.5;
      }
            VecH4 solid_col=TexLodClamp(Img2, col_tex);
      return Lerp(solid_col, water_col, alpha);
   }else
   {
      VecH4 solid_col=TexLodClamp(Img2, inTex);
      BRANCH if(DEPTH_SMALLER(water_z, solid_z_raw)) // branch works faster when most of the screen is above water
      {
             water_z=LinearizeDepth(water_z    );
         Flt solid_z=LinearizeDepth(solid_z_raw);

         if(DEPTH_BACKGROUND(solid_z_raw))alpha=1;else // always force full opacity when there's no solid pixel set to avoid remains in the RenderTarget from previous usage
         {
            Flt dz   =solid_z-water_z;
                alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add)*Sat(dz/0.03);
         }

         VecH4 water_col; water_col.rgb=TexLod(Img3, inTex); water_col.a=0; // water surface color
         VecH4 lum=TexLod(Col, inTex); // water surface light
         VecH  nrm=GetNormal(inTex).xyz; // water surface normals

         MatrixH3 mtrx;
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
         nrm=TransformTP(nrm, mtrx);
         Vec2 refract=nrm.xy*Viewport.size;

         // col light
         water_col.rgb*=lum.rgb;

         // reflection FIXME
         /*Vec   pos=Vec(inPosXY*water_z, water_z);
         Half  pdf=Sat(2-Abs(DistPointPlane(pos, WaterPlanePos, WaterPlaneNrm))), // plane distance factor, needs to be = 1 for dist=0..1 (wave scale)
               rfl=WaterRfl*pdf;
             inTex=Mid ((inTex+refract*WaterMaterial.refract_reflection)*WaterReflectMulAdd.xy+WaterReflectMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
         water_col=Lerp(water_col, TexLodClamp(Img1, inTex), rfl);*/

         // specular
         water_col.rgb+=lum.w*lum.w*0.5;

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
      Flt to_surface=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm, Normalize(Vec(inPosXY, 1)));
      if( to_surface>0)dist=Min(to_surface, dist);

      Flt refract_len=Sat(AccumulatedDensity(WaterMaterial.density, dist)+WaterMaterial.density_add)*Water_refract_underwater;

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
   Flt to_surface=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm, ray);
   if( to_surface>0)dist=Min(to_surface, dist);

   Half opacity=Sat(AccumulatedDensity(WaterMaterial.density, dist)+WaterMaterial.density_add)*WaterUnderStep;

   Flt depth_0=-DistPointPlane(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm),
       depth_1=-DistPointPlane(ray*dist    , WaterPlanePos, WaterPlaneNrm);

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
         Flt d    =AccumulatedDensity(WaterMaterial.density, depth);
         opacity      *=1-WaterMaterial.density;
         water_density+=opacity*d;
         total_opacity+=opacity;
      }
      water_density/=total_opacity;
   }
   if(BOOL)
/**/
   // approximation:
   {
      Half density_0=AccumulatedDensity(WaterMaterial.density, depth_0),
           density_1=AccumulatedDensity(WaterMaterial.density, depth_1),
           blend    =0.5/(1+dist*(WaterMaterial.density/3));
      water_density=Lerp(density_0, density_1, blend);
   }

   VecH water_col=Lerp(Water_color_underwater0, Water_color_underwater1, water_density);

   return REFRACT ? Lerp(TexLod(Img, inTex), VecH4(water_col, 0), opacity)
                  :                          VecH4(water_col    , opacity);
}
/******************************************************************************/
