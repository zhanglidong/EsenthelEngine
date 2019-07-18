@GROUP "Main"

   @SHARED
      #include "Glsl.h"
      #include "Glsl Material.h"
      #include "Glsl MultiMaterial.h"
      #include "Glsl Light.h"

      #define MULTI_TEXCOORD 1

      #if materials<=1 /*|| mtrl_blend==0*/ || COLOR!=0 || per_pixel==0
         VAR MP Vec IO_col;
      #endif
      #if per_pixel!=0
         VAR MP Vec IO_nrm;
      #endif
      #if fx==FX_GRASS
         VAR MP Flt IO_fade_out;
      #endif
      #if materials>1
         VAR MP Vec4 IO_material;
      #endif
      #if textures>=1
         VAR HP Vec2 IO_tex;
         #if MULTI_TEXCOORD!=0
            #if materials>=2
               VAR HP Vec2 IO_tex1;
            #endif
            #if materials>=3
               VAR HP Vec2 IO_tex2;
            #endif
            #if materials>=4
               VAR HP Vec2 IO_tex3;
            #endif
         #endif
      #endif
      #if rflct!=0
         VAR MP Vec IO_rfl;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 3D.h"

      void main()
      {
         HP Vec pos=vtx_pos();
         MP Vec nrm=vtx_nrm();
         HP Vec O_pos;

      #if textures>=1
         IO_tex=((heightmap!=0) ? vtx_texHM() : vtx_tex());
         if(heightmap!=0 && textures>=1)
         {
            #if MULTI_TEXCOORD!=0
               #if materials>=2
                  IO_tex1=IO_tex*MultiMaterial1TexScale();
               #endif
               #if materials>=3
                  IO_tex2=IO_tex*MultiMaterial2TexScale();
               #endif
               #if materials>=4
                  IO_tex3=IO_tex*MultiMaterial3TexScale();
               #endif
               if(materials>=2)IO_tex*=MultiMaterial0TexScale();else
               if(materials==1)IO_tex*=     MaterialTexScale ();
            #else
               if(materials==1)IO_tex*=MaterialTexScale();
            #endif
         }
      #endif
      #if materials<=1
         IO_col.rgb=MaterialColor3();
      #else
         IO_material=vtx_material();
         /*#if mtrl_blend==0
                            IO_col.rgb =IO_material.x*MultiMaterial0Color3()
                                       +IO_material.y*MultiMaterial1Color3();
            if(materials>=3)IO_col.rgb+=IO_material.z*MultiMaterial2Color3();
            if(materials>=4)IO_col.rgb+=IO_material.w*MultiMaterial3Color3();
         #endif*/
      #endif
      #if COLOR!=0
         if(materials<=1/* || mtrl_blend==0*/)IO_col.rgb*=vtx_colorFast3();
         else                                 IO_col.rgb =vtx_colorFast3();
      #endif

      #if fx==FX_GRASS
         IO_fade_out=GrassFadeOut();
      #endif
         if(fx==FX_LEAF )pos=BendLeaf (vtx_hlp(),             pos);
         if(fx==FX_LEAFS)pos=BendLeafs(vtx_hlp(), vtx_size(), pos);

         #if skin==0
         {
                                   O_pos=          TransformPos(pos, gl_InstanceID) ; if(fx==FX_GRASS)O_pos+=BendGrass(pos);
            if(bump_mode>=SBUMP_FLAT)nrm=Normalize(TransformDir(nrm, gl_InstanceID));
         }
         #else
         {
            MP VecI bone  =vtx_bone  ();
            MP Vec  weight=vtx_weight();
                                   O_pos=          TransformPos(pos, bone, weight) ;
            if(bump_mode>=SBUMP_FLAT)nrm=Normalize(TransformDir(nrm, bone, weight));
         }
         #endif
         O_vtx=Project(O_pos);
         gl_ClipDistance[0]=Dot(Vec4(O_pos, 1), ClipPlane);

         #if rflct!=0
            IO_rfl=Normalize(O_pos); // convert to MP
            IO_rfl=Transform3(reflect(IO_rfl, nrm), CamMatrix);
         #endif

         #if per_pixel==0
         {
            if(!(materials<=1 /*|| mtrl_blend==0*/ || COLOR!=0))IO_col.rgb=Vec(1.0, 1.0, 1.0);

            #if bump_mode>=SBUMP_FLAT
            {
               MP Flt d  =Max(Dot(nrm, Light_dir.dir), 0.0);
               MP Vec lum=Light_dir.color.rgb*d + AmbNSColor;
               if(materials<=1 && fx!=FX_BONE)lum+=MaterialAmbient();
               IO_col.rgb*=lum;
            }
            #endif
         }
         #else
            IO_nrm=nrm;
         #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      void main()
      {
         MP Vec col;
      #if materials<=1 /*|| mtrl_blend==0*/ || COLOR!=0 || per_pixel==0
         col=IO_col.rgb;
      #endif
         MP Flt glow;
      #if materials<=1
         #if textures==0
            glow=MaterialGlow();
         #else
            #if textures==1
               MP Vec4 tex_col=Tex(Col, IO_tex);
               #if alpha_test!=0
                  MP Flt alpha=tex_col.a;
                  #if fx==FX_GRASS
                     alpha-=IO_fade_out;
                  #endif
                  AlphaTest(alpha);
               #endif
               glow=MaterialGlow();
            #elif textures==2
               MP Vec4 tex_nrm=Tex(Nrm, IO_tex); // #MaterialTextureChannelOrder
               #if alpha_test!=0
                  MP Flt alpha=tex_nrm.a;
                  #if fx==FX_GRASS
                     alpha-=IO_fade_out;
                  #endif
                  AlphaTest(alpha);
               #endif
               glow=MaterialGlow();
               #if alpha_test==0
                  glow*=tex_nrm.a;
               #endif
            #endif

            #if textures==1
               col*=tex_col.rgb;
            #elif textures==2
               col*=Tex(Col, IO_tex).rgb;
            #endif

            #if rflct!=0
               #if textures==2
                  col.rgb+=TexCube(Rfl, IO_rfl).rgb*(MaterialReflect()*tex_nrm.z);
               #else
                  col.rgb+=TexCube(Rfl, IO_rfl).rgb*MaterialReflect();
               #endif
            #endif
         #endif
      #else // materials>1
         glow=0.0;
         MP Vec tex;
         #if mtrl_blend!=0
            MP Vec4 col0, col1, col2, col3;
            #if MULTI_TEXCOORD!=0
                  col0=Tex(Col , IO_tex );
                  col1=Tex(Col1, IO_tex1);
               #if materials>=3
                  col2=Tex(Col2, IO_tex2);
               #endif
               #if materials>=4
                  col3=Tex(Col3, IO_tex3);
               #endif
            #else
                               col0=Tex(Col , IO_tex*MultiMaterial0TexScale());
                               col1=Tex(Col1, IO_tex*MultiMaterial1TexScale());
               if(materials>=3)col2=Tex(Col2, IO_tex*MultiMaterial2TexScale());
               if(materials>=4)col3=Tex(Col3, IO_tex*MultiMaterial3TexScale());
            #endif
                            col0.rgb*=MultiMaterial0Color3();
                            col1.rgb*=MultiMaterial1Color3();
            if(materials>=3)col2.rgb*=MultiMaterial2Color3();
            if(materials>=4)col3.rgb*=MultiMaterial3Color3();
            MP Vec4 material;
                             material.x=MultiMaterialWeight(IO_material.x, col0.a);
                             material.y=MultiMaterialWeight(IO_material.y, col1.a); if(materials==2)material.xy  /=material.x+material.y;
            if(materials>=3){material.z=MultiMaterialWeight(IO_material.z, col2.a); if(materials==3)material.xyz /=material.x+material.y+material.z;}
            if(materials>=4){material.w=MultiMaterialWeight(IO_material.w, col3.a); if(materials==4)material.xyzw/=material.x+material.y+material.z+material.w;}
                            tex =material.x*col0.rgb
                                +material.y*col1.rgb;
            if(materials>=3)tex+=material.z*col2.rgb;
            if(materials>=4)tex+=material.w*col3.rgb;
         #else
            #if MULTI_TEXCOORD!=0
                  tex =IO_material.x*Tex(Col , IO_tex ).rgb*MultiMaterial0Color3()
                      +IO_material.y*Tex(Col1, IO_tex1).rgb*MultiMaterial1Color3();
               #if materials>=3
                  tex+=IO_material.z*Tex(Col2, IO_tex2).rgb*MultiMaterial2Color3();
               #endif
               #if materials>=4
                  tex+=IO_material.w*Tex(Col3, IO_tex3).rgb*MultiMaterial3Color3();
               #endif
            #else
                               tex =IO_material.x*Tex(Col , IO_tex*MultiMaterial0TexScale()).rgb
                                   +IO_material.y*Tex(Col1, IO_tex*MultiMaterial1TexScale()).rgb;
               if(materials>=3)tex+=IO_material.z*Tex(Col2, IO_tex*MultiMaterial2TexScale()).rgb;
               if(materials>=4)tex+=IO_material.w*Tex(Col3, IO_tex*MultiMaterial3TexScale()).rgb;
            #endif
         #endif
         #if materials<=1 /*|| mtrl_blend==0*/ || COLOR!=0 || per_pixel==0
            col*=tex;
         #else
            col=tex;
         #endif
      #endif
         col+=Highlight.rgb;
         // lighting
         #if per_pixel!=0
         {
            #if bump_mode>=SBUMP_FLAT
            {
               MP Vec nrm=Normalize(IO_nrm); if(fx!=FX_GRASS && fx!=FX_LEAF && fx!=FX_LEAFS)BackFlip(nrm);
               MP Flt d  =Max(Dot(nrm, Light_dir.dir), 0.0);
               MP Vec lum=Light_dir.color.rgb*d + AmbNSColor;
               if(materials<=1 && fx!=FX_BONE)
               {
               #if light_map==1
                  lum+=MaterialAmbient()*Tex(Lum, IO_tex).rgb;
               #else
                  lum+=MaterialAmbient();
               #endif
               }
               col*=lum;
            }
            #endif
         }
         #endif

         gl_FragColor.rgb=col; // set 'gl_FragColor' at end since it's MP
         gl_FragColor.a  =glow;
      }
   @PS_END

@GROUP_END
