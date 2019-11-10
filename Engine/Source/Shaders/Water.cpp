/******************************************************************************/
#include "!Header.h"
#include "Water.h"
/******************************************************************************/
#define DEFAULT_DEPTH 1.0
/******************************************************************************/
// LIGHT, SHADOW, SOFT, REFLECT_ENV, REFLECT_MIRROR, WAVES, RIVER
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

   out Vec2 outTex  :TEXCOORD0,
   out Vec4 outTexN0:TEXCOORD1,
   out Vec4 outTexN1:TEXCOORD2,
#if WAVES
   out Vec4 outTexB :TEXCOORD3,
#endif
#if LIGHT
   out Vec  outPos      :POS,
   out Half outPlaneDist:PLANE_DIST,
#endif
   out Vec4 outVtx:POSITION
)
{
   Vec world_pos=vtx.pos(), view_pos;
   if(WAVES)
   {
      world_pos.y=world_pos.y*WaterYMulAdd.x + WaterYMulAdd.y;

      Vec dir =Normalize(Vec(FracToPosXY(world_pos.xy), 1));
      Flt dist=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm, dir);
      if( dist>0)
      {
         world_pos=dir*dist;
      }else
      {
         world_pos=PointOnPlane(dir*Viewport.range, WaterPlanePos, WaterPlaneNrm);
      }

       view_pos=world_pos;
      world_pos=Transform(world_pos, CamMatrix);
   }else
   {
      view_pos=TransformPos(world_pos);
   }

   Vec2 tex;
   if(RIVER){tex=vtx.tex(); tex.y-=WaterFlow;}
   else     {tex=world_pos.xz;}
   outTex     =tex*  WaterMaterial.scale_color    +     WaterOfs ;
   outTexN0.xy=tex*  WaterMaterial.scale_normal   +     WaterOfs ;
   outTexN0.zw=tex* -WaterMaterial.scale_normal   +     WaterOfs ;
   outTexN1.xy=tex*( WaterMaterial.scale_normal/8)+Perp(WaterOfs);
   outTexN1.zw=tex*(-WaterMaterial.scale_normal/8)+Perp(WaterOfs);
#if WAVES
   outTexB .xy=tex*  WaterMaterial.scale_bump     +     WaterOfs ;
   outTexB .zw=tex* -WaterMaterial.scale_bump     +     WaterOfs ;

   Flt dist_scale=LerpRS(Sqr(150), Sqr(100), Length2(view_pos)),
       bump      =TexLod(Col, outTexB.xy).a+TexLod(Col, outTexB.zw).a; // #WaterMaterialTextureLayout
       bump      =bump-1; // Avg(a,b)*2-1 = (a+b)-1
       view_pos +=WaterPlaneNrm*(WaterMaterial.wave_scale*bump*dist_scale);
#endif

#if LIGHT
   outPos      =view_pos;
   outPlaneDist=DistPointPlane(view_pos, WaterPlanePos, WaterPlaneNrm); // plane distance factor, must be = 1 for dist=0..1 (wave scale)
#endif
   outVtx=Project(view_pos);
}
/******************************************************************************/

// Col, Nrm = water material textures
// ImgXF = background underwater depth
// these must be the same as "Apply" shader - Img1=reflection (2D image), Img2=background underwater

void Surface_PS
(
   Vec2 inTexC :TEXCOORD0,
   Vec4 inTexN0:TEXCOORD1,
   Vec4 inTexN1:TEXCOORD2,
#if WAVES
   Vec4 inTexB :TEXCOORD3,
#endif
#if LIGHT
   Vec  inPos      :POS,
   Half inPlaneDist:PLANE_DIST,
#endif
   PIXEL,

   out VecH4 O_col:TARGET0
#if !LIGHT
 , out VecH  O_nrm:TARGET1
#endif
) // #RTOutput
{
   VecH nrm_flat; // #WaterMaterialTextureLayout
        nrm_flat.xy=(Tex(Nrm, inTexN0.xy).xy - Tex(Nrm, inTexN0.zw).xy + Tex(Nrm, inTexN1.xy).xy - Tex(Nrm, inTexN1.zw).xy)*(WaterMaterial.normal/4); // Avg(Tex(Nrm, inTexN0.xy).xy, -Tex(Nrm, inTexN0.zw).xy, Tex(Nrm, inTexN1.xy).xy, -Tex(Nrm, inTexN1.zw).xy))*WaterMaterial.normal
#if WAVES // FIXME perhaps make it dependent on bump from VS, use some ddx (+delta) and scale by wave_scale
   nrm_flat.xy+=(Tex(Nrm, inTexB.xy).xy - Tex(Nrm, inTexB.zw).xy)*(WaterMaterial.wave_scale*0.5); // Avg(Tex(Nrm, inTexB.xy).xy, -Tex(Nrm, inTexB.zw).xy))*WaterMaterial.wave_scale
#endif
   nrm_flat.z=CalcZ(nrm_flat.xy);

   Matrix3 mtrx; // FIXME precision
   mtrx[0]=ViewMatrixX();
   mtrx[1]=ViewMatrixZ();
   mtrx[2]=ViewMatrixY();

   Vec nrm=Transform(nrm_flat, mtrx); // convert to view space
   // FIXME try to use TransformDir(nrm_flat.xzy) or TransformDir(nrm_flat).xzy

   VecH4 water_col;
   water_col.rgb=Tex(Col, inTexC).rgb*WaterMaterial.color;
   water_col.a  =0;

#if !LIGHT
   O_col=water_col;

   #if SIGNED_NRM_RT
      O_nrm.xyz=nrm;
   #else
      O_nrm.xyz=nrm*0.5+0.5; // -1..1 -> 0..1
   #endif
#else
   Vec eye_dir=Normalize(inPos);

   // shadow
   Half shadow; if(SHADOW)shadow=Sat(ShadowDirValue(inPos, ShadowJitter(pixel.xy), true, SHADOW, false));

   // light
   VecH total_lum     =AmbientNSColor,
        total_specular=0;

   Vec light_dir=LightDir.dir;
   LightParams lp; lp.set(nrm, light_dir);
   Half lum=lp.NdotL; if(SHADOW)lum*=shadow;
   BRANCH if(lum>EPS_LUM)
   {
      // light #1
      lp.set(nrm, light_dir, eye_dir);
            
      // specular
      Half specular=lp.specular(WaterMaterial.smooth, WaterMaterial.reflect, false)*lum;

      // diffuse !! after specular because it adjusts 'lum' !!
      lum*=lp.diffuse(WaterMaterial.smooth);

      total_lum     +=LightDir.color.rgb*lum     ;
      total_specular+=LightDir.color.rgb*specular;
   }
   water_col.rgb*=total_lum;

   Vec2 inTex  =PixelToScreen(pixel);
   Vec2 refract=nrm_flat.xy*Viewport.size; // FIXME

   // reflection
   #if REFLECT_ENV || REFLECT_MIRROR
   {
      water_col.rgb*=1-WaterMaterial.reflect;
      Half reflect_power=ReflectEnv(WaterMaterial.smooth, WaterMaterial.reflect, -Dot(nrm, eye_dir), false);

   #if REFLECT_ENV
      Vec  reflect_dir=ReflectDir(eye_dir, nrm); if(1)reflect_dir.y=Max(0, reflect_dir.y); // don't go below water level (to skip showing ground)
      VecH reflect_env=ReflectTex(reflect_dir, WaterMaterial.smooth)*EnvColor;
   #endif
   #if REFLECT_MIRROR
      Vec2 reflect_tex=Mid((inTex+refract*WaterMaterial.refract_reflection)*WaterReflectMulAdd.xy+WaterReflectMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
      VecH reflect_mirror=TexLodClamp(Img1, reflect_tex).rgb;
      Half reflect_mirror_power=Sat(2-Abs(inPlaneDist)); // can use mirror reflection only if water position is close to water plane
   #endif

   #if REFLECT_ENV && REFLECT_MIRROR // both available
      VecH reflect=Lerp(reflect_env, reflect_mirror, reflect_mirror_power)*reflect_power;
   #elif REFLECT_ENV
      VecH reflect=reflect_env*reflect_power;
   #else
      VecH reflect=reflect_mirror*(reflect_mirror_power*reflect_power);
   #endif

      total_specular+=reflect;
   }
   #endif

   Flt  water_z    =inPos.z,
         back_z_raw=(SOFT ? TexPoint(ImgXF, inTex).x : 0),
         back_z    =(SOFT ? LinearizeDepth(back_z_raw) : water_z+DEFAULT_DEPTH),
             dz    =(SOFT ? back_z-water_z : DEFAULT_DEPTH);
   Half alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);

   if(SOFT)
   {
      Vec2 back_tex=inTex;
      Vec2 test_tex=Mid(back_tex+refract*(WaterMaterial.refract*alpha/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
      Flt  test_z=LinearizeDepth(TexLodClamp(ImgXF, test_tex).x); // use linear filtering because texcoords are not rounded
      if(  test_z>water_z) // only if it's not in front of water (leaking)
      {
         back_z  =test_z;
         back_tex=test_tex;
      }
      if(DEPTH_BACKGROUND(back_z_raw))alpha=1;else // always force full opacity when there's no background pixel set to avoid remains in the RenderTarget from previous usage
      {
         dz   =back_z-water_z;
         alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);
      }
      VecH4 back_col=TexLodClamp(Img2, back_tex);
      O_col=((alpha==1) ? water_col : Lerp(back_col, water_col, alpha));
   }else
   {
      O_col=water_col;
   }
   O_col.rgb+=total_specular; // independent of alpha
#endif
}
/******************************************************************************/

// Img=Water RT Nrm (this is required for 'GetNormal', 'GetNormalMS', which are used for Lights - Dir, Point, etc.), ImgXF=WaterDepth, Img3=Water RT Col, Col=Water RT Lum
// these must be the same as "Surface" shader - Img1=reflection (2D image), Img2=background underwater
// REFRACT, SET_DEPTH

VecH4 Apply_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
               NOPERSP Vec2 inPosXY:TEXCOORD1
            #if SET_DEPTH
                 , out Flt  depth  :DEPTH
            #endif
              ):TARGET
{
   Flt  water_z    =TexPoint        (ImgXF, inTex).x,
         back_z_raw=TexDepthRawPoint(       inTex);
   Half alpha=0;

#if SET_DEPTH
   depth=water_z;
#endif

   if(REFRACT)
   {
      Vec2  back_tex=inTex;
      VecH4 water_col=0;

      BRANCH if(DEPTH_SMALLER(water_z, back_z_raw)) // branch works faster when most of the screen is above water
      {
            water_z=LinearizeDepth(water_z    );
         Flt back_z=LinearizeDepth( back_z_raw);

         water_col.rgb=TexLod(Img3, inTex).rgb; // water surface color
         VecH4 lum=TexLod(Col, inTex); // water surface light
         Vec   nrm=GetNormal(inTex).xyz; // water surface normals

         /*MatrixH3 mtrx;
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
         nrm=TransformTP(nrm, mtrx);*/
         Vec2 refract=nrm.xy*Viewport.size;

         Flt dz   =back_z-water_z;
             alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);

         Vec2 test_tex=Mid(back_tex+refract*(WaterMaterial.refract*alpha/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
         Flt  test_z  =TexDepthLinear(test_tex); // use linear filtering because texcoords are not rounded
         if(  test_z  >water_z)
         {
            back_z  =test_z;
            back_tex=test_tex;
         }

         if(DEPTH_BACKGROUND(back_z_raw))alpha=1;else // always force full opacity when there's no background pixel set to avoid remains in the RenderTarget from previous usage
         {
            dz   =back_z-water_z;
            alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);
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
            VecH4 back_col=TexLodClamp(Img2, back_tex);
      return Lerp(back_col, water_col, alpha);
   }else
   {
      VecH4 back_col=TexLodClamp(Img2, inTex);
      BRANCH if(DEPTH_SMALLER(water_z, back_z_raw)) // branch works faster when most of the screen is above water
      {
            water_z=LinearizeDepth(water_z    );
         Flt back_z=LinearizeDepth( back_z_raw);

         if(DEPTH_BACKGROUND(back_z_raw))alpha=1;else // always force full opacity when there's no background pixel set to avoid remains in the RenderTarget from previous usage
         {
            Flt dz   =back_z-water_z;
                alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);
         }

         VecH4 water_col; water_col.rgb=TexLod(Img3, inTex); water_col.a=0; // water surface color
         VecH4 lum=TexLod(Col, inTex); // water surface light
         Vec   nrm=GetNormal(inTex).xyz; // water surface normals

         /*MatrixH3 mtrx;
         mtrx[0]=ViewMatrixX();
         mtrx[1]=ViewMatrixZ();
         mtrx[2]=ViewMatrixY();
         nrm=TransformTP(nrm, mtrx);*/
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

         back_col=Lerp(back_col, water_col, alpha);
      }
      return back_col;
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
