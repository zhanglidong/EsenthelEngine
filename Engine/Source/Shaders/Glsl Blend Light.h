@GROUP "Main"

   @SHARED
      #include "Glsl.h"
      #include "Glsl Light.h"
      #include "Glsl Material.h"

      #define ALPHA_CLIP 0.5
      #define use_vel alpha_test

         VAR MP Vec4 IO_col;
         VAR MP Vec  IO_col_add;
      #if textures>=1
         VAR HP Vec2 IO_tex;
      #endif
      #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
         VAR MP Vec  IO_nrm;
      #endif
      #if rflct!=0
         VAR MP Vec  IO_rfl;
      #endif
      #if use_vel!=0
         VAR HP Vec  IO_pos;
         VAR MP Vec  IO_vel;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 3D.h"

      void main()
      {
         HP Vec pos=vtx_pos();
         MP Vec nrm=vtx_nrm();
      #if use_vel==0
         HP Vec IO_pos;
      #endif

      #if textures>=1
         IO_tex=vtx_tex();
      #endif

                     IO_col =MaterialColor    ();
         if(COLOR!=0)IO_col*=    vtx_colorFast();
      #if fx==FX_GRASS
         IO_col.a*=1.0-GrassFadeOut();
      #elif fx==FX_LEAF
         pos=BendLeaf(vtx_hlp(), pos);
      #elif fx==FX_LEAFS
         pos=BendLeafs(vtx_hlp(), vtx_size(), pos);
      #endif

         #if skin==0
         {
                                  IO_pos=          TransformPos(pos, gl_InstanceID) ; if(fx==FX_GRASS)IO_pos+=BendGrass(pos);
            if(bump_mode>=SBUMP_FLAT)nrm=Normalize(TransformDir(nrm, gl_InstanceID));
                                   O_vtx=          Project     (IO_pos);
         #if use_vel!=0
            IO_vel=ObjVel[gl_InstanceID]; // #PER_INSTANCE_VEL
         #endif
         }
         #else
         {
            MP VecI bone  =vtx_bone  ();
            MP Vec  weight=vtx_weight();
                                  IO_pos=          TransformPos(pos, bone, weight) ;
            if(bump_mode>=SBUMP_FLAT)nrm=Normalize(TransformDir(nrm, bone, weight));
                                   O_vtx=          Project     (IO_pos);
         #if use_vel!=0
            IO_vel=GetBoneVel(bone, weight);
         #endif
         }
         #endif

         // sky
         MP Vec mp_pos =IO_pos;
         MP Flt d      =Length(mp_pos);
         MP Flt opacity=Sat(d*SkyFracMulAdd.x + SkyFracMulAdd.y);
         IO_col.a*=opacity;

         // fog
         MP Flt fog_rev=      VisibleOpacity(FogDensity, d);
         IO_col.rgb*=                              fog_rev ;
         IO_col_add =Lerp(FogColor, Highlight.rgb, fog_rev);

         // per-vertex light
         #if per_pixel==0 && bump_mode>=SBUMP_FLAT
         {
            MP Flt d  =Max(Dot(nrm, Light_dir.dir), 0.0);
            MP Vec lum=Light_dir.color.rgb*d + AmbNSColor;
            IO_col.rgb*=lum;
         }
         #endif

         #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
            IO_nrm=nrm;
         #endif

         #if rflct!=0
         {
            IO_rfl=Normalize(IO_pos); // convert to MP
            #if !(per_pixel!=0 && bump_mode>=SBUMP_FLAT)
               IO_rfl=Transform3(reflect(IO_rfl, nrm), CamMatrix);
            #endif
         }
         #endif
         #if use_vel!=0
            UpdateVelocities_VS(IO_vel, pos, IO_pos);
         #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"
      #include "Glsl 3D.h"

      void main()
      {
         MP Vec  nrm;
         MP Vec4 col=IO_col;

         #if   textures==0
            #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
               nrm=Normalize(IO_nrm);
            #endif
         #elif textures==1
            MP Vec4 tex_col=Tex(Col, IO_tex);
            #if alpha_test
               if(tex_col.a<ALPHA_CLIP)discard;
            #endif
            #if ALPHA
               col*=tex_col;
            #else
               col.rgb*=tex_col.rgb;
            #endif
            #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
               nrm=Normalize(IO_nrm);
            #endif
         #elif textures==2
            MP Vec4 tex_nrm=Tex(Nrm, IO_tex); // #MaterialTextureChannelOrder
            #if alpha_test
               if(tex_nrm.a<ALPHA_CLIP)discard;
            #endif
               col.rgb*=Tex(Col, IO_tex).rgb;
            #if ALPHA
               col.a  *=tex_nrm.a;
            #endif
            #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
               nrm=Normalize(IO_nrm);
            #endif
         #endif

         #if rflct!=0
            MP Vec rfl=IO_rfl;
            #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
               rfl=Transform3(reflect(rfl, nrm), CamMatrix);
            #endif
            #if textures==2
               col.rgb+=TexCube(Rfl, rfl).rgb*(MaterialReflect()*tex_nrm.z);
            #else
               col.rgb+=TexCube(Rfl, rfl).rgb*MaterialReflect();
            #endif
         #endif

         #if per_pixel!=0 && bump_mode>=SBUMP_FLAT
         {
            MP Vec total_lum=AmbNSColor;
            if(fx!=FX_GRASS && fx!=FX_LEAF && fx!=FX_LEAFS)BackFlip(nrm);

            // ambient
       /*#if light_map==1
            total_lum+=AmbMaterial*MaterialAmbient()*Tex(Lum, IO_tex).rgb;
         #else
            total_lum+=AmbMaterial*MaterialAmbient();
         #endif*/

            // directional light
            MP Flt d  =Max(Dot(nrm, Light_dir.dir), 0.0);
            total_lum+=Light_dir.color.rgb*d;

            col.rgb*=total_lum;
         }
         #endif
         col.rgb+=IO_col_add;
         gl_FragData[0]=col;
      #if use_vel!=0
         gl_FragData[1].xyz=UpdateVelocities_PS(IO_vel, IO_pos);
         gl_FragData[1].w  =col.a; // alpha needed because of blending
      #endif
      }
   @PS_END

@GROUP_END
