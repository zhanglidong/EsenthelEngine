@GROUP "Main"

   @SHARED
      #include "Glsl.h"
      #include "Glsl Material.h"
      #include "Glsl Light.h"

         VAR MP Vec4 IO_col;
      #if textures>0
         VAR HP Vec2 IO_tex;
      #endif
      #if rflct!=0
         VAR MP Vec  IO_rfl;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 3D.h"

      void main()
      {
         HP Vec pos=vtx_pos();

      #if textures>0
         IO_tex=vtx_tex();
      #endif
         IO_col=MaterialColor();
      #if COLOR!=0
         IO_col*=vtx_colorFast();
      #endif

      #if skin==0
                   pos=          TransformPos(pos      , gl_InstanceID) ;
         #if rflct!=0
            MP Vec nrm=Normalize(TransformDir(vtx_nrm(), gl_InstanceID));
         #endif
                 O_vtx=          Project     (pos);
      #else
            MP VecI bone  =vtx_bone  ();
            MP Vec  weight=vtx_weight();
                   pos=          TransformPos(pos      , bone, weight) ;
         #if rflct!=0
            MP Vec nrm=Normalize(TransformDir(vtx_nrm(), bone, weight));
         #endif
                 O_vtx=          Project     (pos);
      #endif

         MP Vec mp_pos =pos;
         MP Flt d      =Length(mp_pos);
         MP Flt opacity=Sat(d*SkyFracMulAdd.x + SkyFracMulAdd.y);
         IO_col.a*=opacity;

      #if rflct!=0
         IO_rfl=Normalize(pos); // convert to MP
         IO_rfl=Transform3(reflect(IO_rfl, nrm), CamMatrix);
      #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      void main()
      {
         MP Vec4 col=IO_col;

      #if textures==1
         col*=Tex(Col, IO_tex);
      #elif textures==2
         MP Vec4 tex_nrm=Tex(Nrm, IO_tex); // #MaterialTextureChannelOrder
         MP Vec4 tex_col=Tex(Col, IO_tex); tex_col.a=tex_nrm.a; col*=tex_col;
      #endif

         col.rgb+=Highlight.rgb;

      #if rflct!=0
         #if textures==2
            col.rgb+=TexCube(Rfl, IO_rfl).rgb*(MaterialReflect()*tex_nrm.z);
         #else
            col.rgb+=TexCube(Rfl, IO_rfl).rgb*MaterialReflect();
         #endif
      #endif

         gl_FragColor=col; // set 'gl_FragColor' at end since it's MP
      }
   @PS_END

@GROUP_END
