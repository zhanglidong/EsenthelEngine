/******************************************************************************/
#include "!Header.h"
#include "Water.h"
/******************************************************************************
This code calculates lighting by taking samples along the view ray, which is refracted by water
-calculate view direction
-refract   view direction
-keep moving along view direction
-calculate total density at that point (based on travelled distance inside water)
-calculate light intensity at that point (based on distance to surface along refracted 'light_dir'
{
   static Flt min=FLT_MAX, max=0;
   Vec light_dir=-Sun.pos;
   Flt d=Dot(light_dir, Water.plane.normal);
   //if(d<0)
      Vec under_water_light_dir=Refract(light_dir, Water.plane.normal, 1.33f);
   
   //Water.density=Mid(Ms.pos().y, 0.1f, 0.99f);
   Int res=128;
   REPD(x, res)
   REPD(y, res)
   {
      Flt fx=x/Flt(res-1), fy=y/Flt(res-1);
      Vec2 screen=D.rect().lerp(fx, fy);
      Vec pos, dir; ScreenToPosDir(screen, pos, dir);
      Vec nrm, hit_pos; if(SweepPointPlane(pos, dir*D.viewRange(), Water.plane, null, &nrm, &hit_pos))
      {
         Vec view=Refract(dir, nrm, 1.33f);
         Vec pos=0;
         Flt density=0;
         Flt light=0;
         for(;;)
         {
            pos+=view/16;
            Flt step_density=AccumulatedDensity(Water.density, pos.length());
            Flt gained_density=step_density-density;
            Flt dist_plane_ray=Abs(DistPointPlaneRay(pos, Water.plane.normal, under_water_light_dir));
            Flt l=VisibleOpacity(Water.density, dist_plane_ray);
            light+=gained_density*l;
            density=step_density;
            if(density>=0.99f)break;
         }
         MIN(min, light);
         MAX(max, light);
         light/=0.582f;
         if(Kb.shift()) // approximation
         {
            Flt p=Sat(1+Dot(dir, nrm));
            p=1-Sqr(1-p);
            light=Lerp(0.815f, 1.0f, p);
         }
         light*=Sat(-Dot(light_dir, nrm));
         if(Kb.ctrl())light=LinearToSRGB(light);
         Color c=ColorBrightness(AZURE, light);
         VI.dot(c, screen, 0.014f);
      }
   }
   VI.end();
   D.text(0,0.8f,S+min/max+' '+max);
}
/******************************************************************************/
// LIGHT, SHADOW, SOFT, REFLECT_ENV, REFLECT_MIRROR, GATHER, WAVES, RIVER
// Col, Nrm, Ext = water material textures
// ImgXF = background underwater depth
// these must be the same as "Apply" shader - Img1=reflection (2D image), Img2=background underwater
#ifndef WAVES
#define WAVES 0
#endif
#ifndef RIVER
#define RIVER 0
#endif
#ifndef REFRACT
#define REFRACT 0
#endif
#define DUAL_NORMAL 0
/******************************************************************************/
Half Wave(Vec2 world_pos)
{
   return Avg(RTexLod(Ext, (WaterOfsBump+world_pos)*WaterMaterial.scale_bump).x,  // it's better to scale 'WaterOfsBump' too #MaterialTextureLayoutWater
              RTexLod(Ext, (WaterOfsBump-world_pos)*WaterMaterial.scale_bump).x); // it's better to scale 'WaterOfsBump' too
}
/******************************************************************************/
void Surface_VS
(
   VtxInput vtx,

   out Vec2 uv_col:UV_COL,
   out Vec4 uv_nrm:UV_NRM,
#if DUAL_NORMAL
   out Vec4 uv_nrm1:UV_NRM1,
#endif
#if WAVES
   out VecH outWaveN:NORMAL,
#endif
#if LIGHT
   out Vec  outPos      :POS,
   out Half outPlaneDist:PLANE_DIST,
#endif
   out Vec4 vpos:POSITION
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

   Vec2 uv;
   if(RIVER){uv=vtx.uv(); uv.y-=WaterFlow;}
   else     {uv=world_pos.xz;}
   uv_col   =(WaterOfsCol+uv)*WaterMaterial.scale_color ; // it's better to scale 'WaterOfsCol' too
   uv_nrm.xy=(WaterOfsNrm+uv)*WaterMaterial.scale_normal;
   uv_nrm.zw=(WaterOfsNrm-uv)*WaterMaterial.scale_normal;
#if DUAL_NORMAL
   uv_nrm1.xy=(WaterOfsNrm+uv/8)*WaterMaterial.scale_normal;
   uv_nrm1.zw=(WaterOfsNrm-uv/8)*WaterMaterial.scale_normal;
#endif

#if WAVES
   Flt  dist=Length(view_pos);
   Half dist_scale;
   dist_scale=Sat(16/dist-0.25 )*WaterMaterial.wave_scale;
 //dist_scale=Sat(12/dist-0.125)*WaterMaterial.wave_scale;
 //dist_scale=Sat( 6/dist      )*WaterMaterial.wave_scale;

   #define DERIVATIVE 0.125
   Half wave  =Wave(uv),
        wave_r=Wave(uv+Vec2(DERIVATIVE, 0)),
        wave_f=Wave(uv+Vec2(0, DERIVATIVE));

   outWaveN.x=wave-wave_r;
   outWaveN.y=wave-wave_f;
   outWaveN.z=DERIVATIVE/dist_scale;
   outWaveN=Normalize(outWaveN);

   view_pos+=WaterPlaneNrm*(wave*dist_scale);
#endif

#if LIGHT
   outPos      =view_pos;
   outPlaneDist=DistPointPlane(view_pos, WaterPlanePos, WaterPlaneNrm); // plane distance factor, must be = 1 for dist=0..1 (wave scale)
#endif
   vpos=Project(view_pos);
}
/******************************************************************************/
void WaterReflectColor(inout VecH total_specular, Vec nrm, Vec eye_dir, Vec2 uv, Vec2 refract, Half plane_dist)
{
   #if REFLECT_ENV || REFLECT_MIRROR
   {
      VecH reflect_power=ReflectEnv(WaterMaterial.rough, WaterMaterial.reflect, WaterMaterial.reflect, -Dot(nrm, eye_dir), false);

   #if REFLECT_ENV
      Vec  reflect_dir=ReflectDir(eye_dir, nrm); if(1)reflect_dir.y=Max(0, reflect_dir.y); // don't go below water level (to skip showing ground)
      VecH reflect_env=ReflectTex(reflect_dir, WaterMaterial.rough)*EnvColor;
   #endif
   #if REFLECT_MIRROR
      Vec2 reflect_tex=Mid((uv+refract*WaterMaterial.refract_reflection)*WaterReflectMulAdd.xy+WaterReflectMulAdd.zw, WaterClamp.xy, WaterClamp.zw);
      VecH reflect_mirror=TexLod(Img1, reflect_tex).rgb; // need UV clamp
      Half reflect_mirror_power=Sat(2-Abs(plane_dist)); // can use mirror reflection only if water position is close to water plane
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
}
void Surface_PS
(
   Vec2 uv_col:UV_COL,
   Vec4 uv_nrm:UV_NRM,
#if DUAL_NORMAL
   Vec4 uv_nrm1:UV_NRM1,
#endif
#if WAVES
   VecH inWaveN:NORMAL,
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
   VecH nrm_flat; // #MaterialTextureLayoutWater
#if DUAL_NORMAL
   nrm_flat.xy=(RTex(Nrm, uv_nrm.xy).xy - RTex(Nrm, uv_nrm.zw).xy + RTex(Nrm, uv_nrm1.xy).xy - RTex(Nrm, uv_nrm1.zw).xy)*(WaterMaterial.normal/4); // Avg(RTex(Nrm, uv_nrm.xy).xy, -RTex(Nrm, uv_nrm.zw).xy, RTex(Nrm, uv_nrm1.xy).xy, -RTex(Nrm, uv_nrm1.zw).xy)*WaterMaterial.normal; normals from mirrored tex coordinates must be subtracted
#else
   nrm_flat.xy=(RTex(Nrm, uv_nrm.xy).xy - RTex(Nrm, uv_nrm.zw).xy)*(WaterMaterial.normal/2); // Avg(RTex(Nrm, uv_nrm.xy).xy, -RTex(Nrm, uv_nrm.zw).xy)*WaterMaterial.normal; normals from mirrored tex coordinates must be subtracted
#endif
#if WAVES
   nrm_flat.xy+=inWaveN.xy;
#endif
   nrm_flat.z=CalcZ(nrm_flat.xy);
   Vec nrm=Normalize(Vec(TransformDir(nrm_flat.xzy))); // convert to view space, convert to HP before Normalize

   VecH4 water_col;
   water_col.rgb=RTex(Col, uv_col).rgb*WaterMaterial.color;
   water_col.a  =0;

#if !LIGHT
   O_col=water_col;

   #if SIGNED_NRM_RT
      O_nrm.xyz=nrm;
   #else
      O_nrm.xyz=nrm*0.5+0.5; // -1..1 -> 0..1
   #endif
#else
   Vec2 uv     =PixelToUV(pixel);
   Vec2 refract=nrm_flat.xy*Viewport.size; // TODO: this could be improved
   Vec  eye_dir=Normalize(inPos);

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
            
      VecH            lum_rgb=LightDir.color.rgb*lum;
      total_lum     +=lum_rgb*lp.diffuseWater(                                                                                              ); // diffuse
      total_specular+=lum_rgb*lp.specular    (WaterMaterial.rough, WaterMaterial.reflect, WaterMaterial.reflect, false, LightDir.radius_frac); // specular
   }
   water_col.rgb*=total_lum;

   // reflection
   WaterReflectColor(total_specular, nrm, eye_dir, uv, refract, inPlaneDist);

   if(SOFT)
   {
      Flt water_z    =inPos.z;
      Flt water_z_raw=pixel.z;
   #if REFRACT
      Vec2 back_uv=Mid(uv+refract*(WaterMaterial.refract/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
   #if GATHER
      // potentially could use FilterMinMax instead of TexGather, however we need both DEPTH_MIN/DEPTH_MAX, so that would need 2 tex reads, so better skip
      Vec4 back_z_raw4=TexGather(ImgXF, back_uv);
   #else // simulate gather
      Vec2 pixel  =back_uv*RTSize.zw+0.5,
           pixeli =Floor(pixel),
           tex_min=(pixeli-0.5)*RTSize.xy,
           tex_max=(pixeli+0.5)*RTSize.xy;
      Vec4 back_z_raw4=Vec4(TexPoint(ImgXF, Vec2(tex_min.x, tex_min.y)),
                            TexPoint(ImgXF, Vec2(tex_max.x, tex_min.y)),
                            TexPoint(ImgXF, Vec2(tex_min.x, tex_max.y)),
                            TexPoint(ImgXF, Vec2(tex_max.x, tex_max.y)));
   #endif // GATHER
      Flt back_z_raw=DEPTH_MAX(back_z_raw4); // use DEPTH_MAX to check if any depth sample is Z_BACK (not set), we will use linear filtering so have to check all 4 pixels for depth
      if(DEPTH_SMALLER(DEPTH_MIN(back_z_raw4), water_z_raw)) // if refracted sample is in front of water (leaking), use DEPTH_MIN to check if all samples are in front
      { // skip refracted sample
         back_uv   =uv;
         back_z_raw=TexPoint(ImgXF, uv).x;
      }
   #else // NO REFRACT
      Vec2 back_uv   =uv;
      Flt  back_z_raw=TexPoint(ImgXF, uv).x;
   #endif // REFRACT
      if(DEPTH_BACKGROUND(back_z_raw))O_col=water_col;else // always force full opacity when there's no background pixel set to ignore discarded pixels in RenderTarget (they could cause artifacts)
      {
         Flt   back_z=LinearizeDepth(back_z_raw);
         Flt   dz=back_z-water_z;
         Half  alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);
         VecH4 back_col=TexLod(Img2, back_uv); // need UV clamp
         O_col=Lerp(back_col, water_col, alpha);
      }
   }else
   {
      O_col=water_col;
   }
   O_col.rgb+=total_specular; // independent of alpha
#endif
}
/******************************************************************************/
// Img=Water RT Nrm (this is required for 'GetNormal', 'GetNormalMS', which are used for Lights - Dir, Point, etc.), ImgXF=WaterDepth, Img3=Water RT Col, Img4=Water RT Lum, Img5=Water RT Lum Spec
// these must be the same as "Surface" shader - Img1=reflection (2D image), Img2=background underwater
// REFRACT, SET_DEPTH, REFLECT_ENV, REFLECT_MIRROR, GATHER
VecH4 Apply_PS(NOPERSP Vec2 uv   :UV,
               NOPERSP Vec2 posXY:POS_XY,
               NOPERSP PIXEL
            #if SET_DEPTH
                 , out Flt  depth:DEPTH
            #endif
              ):TARGET
{
   VecI2 pix=pixel.xy;
   Vec2  back_uv=uv;
   Flt   back_z_raw=Depth[pix],
        water_z_raw=ImgXF[pix];

#if SET_DEPTH
   depth=water_z_raw;
#endif

   BRANCH if(DEPTH_SMALLER(water_z_raw, back_z_raw)) // branch works faster when most of the screen is above water
   {
      Flt water_z=LinearizeDepth(water_z_raw);
      Flt  back_z=LinearizeDepth( back_z_raw);

      Vec pos=Vec(posXY*water_z, water_z);
      Vec eye_dir=Normalize(pos);

      VecH4 water_col; water_col.rgb=Img3[pix].rgb; water_col.a=0; // water surface color
      VecH  lum =   Img4  [pix]; // water surface light
      VecH  spec=   Img5  [pix]; // water surface light specular
      Vec   nrm =GetNormal(pix).xyz; // water surface normals
      Vec   nrm_flat=TransformTP(nrm, (Matrix3)GetViewMatrix()).xzy;
      Vec2  refract=nrm_flat.xy*Viewport.size; // TODO: this could be improved

      water_col.rgb*=lum;
      WaterReflectColor(spec, nrm, eye_dir, uv, refract, DistPointPlane(pos, WaterPlanePos, WaterPlaneNrm));

   #if REFRACT
      Vec2 test_tex=Mid(uv+refract*(WaterMaterial.refract/Max(1, water_z)), WaterClamp.xy, WaterClamp.zw);
   #if GATHER
      // potentially could use FilterMinMax instead of 'TexDepthRawGather', however we need both DEPTH_MIN/DEPTH_MAX, so that would need 2 tex reads, so better skip
      Vec4 test_z_raw4=TexDepthRawGather(test_tex);
   #else // simulate gather
      Vec2 pixel  =test_tex*RTSize.zw+0.5,
           pixeli =Floor(pixel),
           tex_min=(pixeli-0.5)*RTSize.xy,
           tex_max=(pixeli+0.5)*RTSize.xy;
      Vec4 test_z_raw4=Vec4(TexPoint(Depth, Vec2(tex_min.x, tex_min.y)),
                            TexPoint(Depth, Vec2(tex_max.x, tex_min.y)),
                            TexPoint(Depth, Vec2(tex_min.x, tex_max.y)),
                            TexPoint(Depth, Vec2(tex_max.x, tex_max.y)));
   #endif // GATHER
      if(DEPTH_SMALLER(water_z_raw, DEPTH_MIN(test_z_raw4))) // if refracted sample is behind water (not leaking), use DEPTH_MIN to check if all samples are behind
      { // use refracted sample
         back_uv   =test_tex;
         back_z_raw=DEPTH_MAX(test_z_raw4); // use DEPTH_MAX to check if any depth sample is Z_BACK (not set) to force full opacity
      }
   #endif // REFRACT
      if(DEPTH_FOREGROUND(back_z_raw)) // always force full opacity when there's no background pixel set to ignore discarded pixels in RenderTarget (they could cause artifacts)
      {
         Flt   back_z=LinearizeDepth(back_z_raw);
         Flt   dz=back_z-water_z;
         Half  alpha=Sat(AccumulatedDensity(WaterMaterial.density, dz) + WaterMaterial.density_add);
         VecH4 back_col=TexLod(Img2, back_uv); // need UV clamp
         water_col=Lerp(back_col, water_col, alpha);
      }
      water_col.rgb+=spec; // independent of alpha
      return water_col;
   }
   return Img2[pix];
}
/******************************************************************************/
// REFRACT
VecH4 Under_PS
(
#if REFRACT
   NOPERSP Vec2 uv:UV,
#endif
   NOPERSP Vec2 posXY:POS_XY,
   NOPERSP PIXEL
):TARGET
{
   // underwater refraction
   #if 0 && REFRACT
   {
      Flt dist      =Viewport.range;
      Flt to_surface=-DistPointPlaneRay(Vec(0, 0, 0), WaterPlanePos, WaterPlaneNrm, Normalize(Vec(posXY, 1)));
      if( to_surface>0)dist=Min(to_surface, dist);

      Flt refract_len=Sat(AccumulatedDensity(WaterMaterial.density, dist)+WaterMaterial.density_add)*Water_refract_underwater;

   #if 1 // viewport size adjusted
      uv+=Sin(uv.yx*14/Viewport.size+Step)*refract_len*Viewport.size; // add refraction
      uv =(uv-Viewport.center)/(1+2*refract_len)+Viewport.center; // scale texture coordinates to avoid clamping
   #else
      uv+=Sin(uv.yx*14+Step)*refract_len; // add refraction
      uv =(uv-0.5)/(1+2*refract_len)+0.5; // scale texture coordinates to avoid clamping
   #endif
   }
   #endif

#if 0 && REFRACT
   Vec pos=GetPosLinear(uv);
#else
   Vec pos=GetPosPix(pixel.xy, posXY);
#endif
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

#if 0 && REFRACT
   return Lerp(TexLod(Img, uv), VecH4(water_col, 0), opacity);
#else
   return                       VecH4(water_col    , opacity);
#endif
}
/******************************************************************************/
