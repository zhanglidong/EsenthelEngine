@GROUP "Draw3DTex" // params: COLOR, alpha_test
   @SHARED
      #include "Glsl.h"

         VAR HP Vec2 IO_tex;
      #if COLOR!=0
         VAR MP Vec4 IO_col;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 3D.h"

      void main()
      {
          O_vtx=Project(TransformPos(vtx_pos()));
         IO_tex=vtx_tex();
      #if COLOR!=0
         IO_col=vtx_color();
      #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      void main()
      {
         MP Vec4 col=Tex(Img, IO_tex);
      #if alpha_test!=0
         if(col.a<0.5)discard;
      #endif
      #if COLOR!=0
         col*=IO_col;
      #endif
         gl_FragColor=col;
      }
   @PS_END

@GROUP_END


@GROUP "Draw2DTex" // params: COLOR
   @SHARED
      #include "Glsl.h"

      VAR HP Vec2 IO_tex;
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 2D.h"

      void main()
      {
          O_vtx=Vec4(vtx_pos2()*Coords.xy+Coords.zw, Flt(REVERSE_DEPTH), 1.0);
         IO_tex=vtx_tex();
      }
   @VS_END

   @PS
      #include "Glsl PS.h"
      #include "Glsl PS 2D.h"

      void main()
      {
      #if COLOR!=0
         gl_FragColor=Tex(Img, IO_tex)*Color[0]+Color[1];
      #else
         gl_FragColor=Tex(Img, IO_tex);
      #endif
      }
   @PS_END

@GROUP_END


@GROUP "Font"
   @SHARED
      #include "Glsl.h"

      VAR HP Vec2 IO_tex;
      VAR MP Flt  IO_shade;
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 2D.h"

      void main()
      {
          O_vtx  =Vec4(vtx_pos2()*Coords.xy+Coords.zw, Flt(REVERSE_DEPTH), 1.0);
         IO_tex  =vtx_tex ();
         IO_shade=vtx_size();
      }
   @VS_END

   @PS
      #include "Glsl PS.h"
      #include "Glsl PS 2D.h"

      PAR MP Flt FontShadow, FontShade;

      void main()
      {
         MP Vec4 tex=Tex(ImgXY, IO_tex);
         MP Flt  a  =tex.r,            // #FontImageLayout
                 s  =tex.g*FontShadow, // #FontImageLayout
                 final_alpha=a+s-s*a;
         MP Flt  final_color=Lerp(FontShade, 1.0, Sat(IO_shade))*a/(final_alpha+HALF_MIN);

         gl_FragColor.rgb=Color[0].rgb*final_color;
         gl_FragColor.a  =Color[0].a  *final_alpha;
      }
   @PS_END

@GROUP_END


// can use 'RTSize' instead of 'ImgSize' since there's no scale
@GROUP "Blur" // params: axis, high

   @SHARED
      #include "Glsl.h"

      #define WEIGHT4_0 0.250000000
      #define WEIGHT4_1 0.213388354
      #define WEIGHT4_2 0.124999993
      #define WEIGHT4_3 0.036611654
      // WEIGHT4_0 + WEIGHT4_1*2 + WEIGHT4_2*2 + WEIGHT4_3*2 = 1

      #define WEIGHT6_0 0.166666668
      #define WEIGHT6_1 0.155502122
      #define WEIGHT6_2 0.125000001
      #define WEIGHT6_3 0.083333329
      #define WEIGHT6_4 0.041666662
      #define WEIGHT6_5 0.011164551
      // WEIGHT6_0 + WEIGHT6_1*2 + WEIGHT6_2*2 + WEIGHT6_3*2 + WEIGHT6_4*2 + WEIGHT6_5*2 = 1

         VAR HP Vec2 IO_tex0, IO_tex1, IO_tex2, IO_tex3;
      #if high!=0
         VAR HP Vec2 IO_tex4, IO_tex5;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 2D.h"

      void main()
      {
         O_vtx=vtx_pos4();
         HP Vec2 tex=vtx_tex();
      #if high==0
         #if axis==0 // X
            IO_tex0.y=tex.y; IO_tex0.x=tex.x+RTSize.x*( 0.0+WEIGHT4_1/(WEIGHT4_0/2.0+WEIGHT4_1));
            IO_tex1.y=tex.y; IO_tex1.x=tex.x+RTSize.x*(-0.0-WEIGHT4_1/(WEIGHT4_0/2.0+WEIGHT4_1));
            IO_tex2.y=tex.y; IO_tex2.x=tex.x+RTSize.x*( 2.0+WEIGHT4_3/(WEIGHT4_2    +WEIGHT4_3));
            IO_tex3.y=tex.y; IO_tex3.x=tex.x+RTSize.x*(-2.0-WEIGHT4_3/(WEIGHT4_2    +WEIGHT4_3));
         #else
            IO_tex0.x=tex.x; IO_tex0.y=tex.y+RTSize.y*( 0.0+WEIGHT4_1/(WEIGHT4_0/2.0+WEIGHT4_1));
            IO_tex1.x=tex.x; IO_tex1.y=tex.y+RTSize.y*(-0.0-WEIGHT4_1/(WEIGHT4_0/2.0+WEIGHT4_1));
            IO_tex2.x=tex.x; IO_tex2.y=tex.y+RTSize.y*( 2.0+WEIGHT4_3/(WEIGHT4_2    +WEIGHT4_3));
            IO_tex3.x=tex.x; IO_tex3.y=tex.y+RTSize.y*(-2.0-WEIGHT4_3/(WEIGHT4_2    +WEIGHT4_3));
         #endif
      #else
         #if axis==0 // X
            IO_tex0.y=tex.y; IO_tex0.x=tex.x+RTSize.x*( 0.0+WEIGHT6_1/(WEIGHT6_0/2.0+WEIGHT6_1));
            IO_tex1.y=tex.y; IO_tex1.x=tex.x+RTSize.x*(-0.0-WEIGHT6_1/(WEIGHT6_0/2.0+WEIGHT6_1));
            IO_tex2.y=tex.y; IO_tex2.x=tex.x+RTSize.x*( 2.0+WEIGHT6_3/(WEIGHT6_2    +WEIGHT6_3));
            IO_tex3.y=tex.y; IO_tex3.x=tex.x+RTSize.x*(-2.0-WEIGHT6_3/(WEIGHT6_2    +WEIGHT6_3));
            IO_tex4.y=tex.y; IO_tex4.x=tex.x+RTSize.x*( 4.0+WEIGHT6_5/(WEIGHT6_4    +WEIGHT6_5));
            IO_tex5.y=tex.y; IO_tex5.x=tex.x+RTSize.x*(-4.0-WEIGHT6_5/(WEIGHT6_4    +WEIGHT6_5));
         #else
            IO_tex0.x=tex.x; IO_tex0.y=tex.y+RTSize.y*( 0.0+WEIGHT6_1/(WEIGHT6_0/2.0+WEIGHT6_1));
            IO_tex1.x=tex.x; IO_tex1.y=tex.y+RTSize.y*(-0.0-WEIGHT6_1/(WEIGHT6_0/2.0+WEIGHT6_1));
            IO_tex2.x=tex.x; IO_tex2.y=tex.y+RTSize.y*( 2.0+WEIGHT6_3/(WEIGHT6_2    +WEIGHT6_3));
            IO_tex3.x=tex.x; IO_tex3.y=tex.y+RTSize.y*(-2.0-WEIGHT6_3/(WEIGHT6_2    +WEIGHT6_3));
            IO_tex4.x=tex.x; IO_tex4.y=tex.y+RTSize.y*( 4.0+WEIGHT6_5/(WEIGHT6_4    +WEIGHT6_5));
            IO_tex5.x=tex.x; IO_tex5.y=tex.y+RTSize.y*(-4.0-WEIGHT6_5/(WEIGHT6_4    +WEIGHT6_5));
         #endif
      #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      void main()
      {
      #if high==0
         gl_FragColor.rgb=texture2DLod(Img, IO_tex0, 0.0).rgb*(WEIGHT4_0/2.0+WEIGHT4_1)
                         +texture2DLod(Img, IO_tex1, 0.0).rgb*(WEIGHT4_0/2.0+WEIGHT4_1)
                         +texture2DLod(Img, IO_tex2, 0.0).rgb*(WEIGHT4_2    +WEIGHT4_3)
                         +texture2DLod(Img, IO_tex3, 0.0).rgb*(WEIGHT4_2    +WEIGHT4_3);
      #else
         gl_FragColor.rgb=texture2DLod(Img, IO_tex0, 0.0).rgb*(WEIGHT6_0/2.0+WEIGHT6_1)
                         +texture2DLod(Img, IO_tex1, 0.0).rgb*(WEIGHT6_0/2.0+WEIGHT6_1)
                         +texture2DLod(Img, IO_tex2, 0.0).rgb*(WEIGHT6_2    +WEIGHT6_3)
                         +texture2DLod(Img, IO_tex3, 0.0).rgb*(WEIGHT6_2    +WEIGHT6_3)
                         +texture2DLod(Img, IO_tex4, 0.0).rgb*(WEIGHT6_4    +WEIGHT6_5)
                         +texture2DLod(Img, IO_tex5, 0.0).rgb*(WEIGHT6_4    +WEIGHT6_5);
      #endif
         gl_FragColor.a=0.0;
      }
   @PS_END

@GROUP_END


@GROUP "BloomDS" // params: DoGlow, DoClamp, half, saturate

   @SHARED
      #include "Glsl.h"

      PAR MP Vec BloomParams;

      #if DoGlow!=0
         #define res ((half!=0) ? 2 : 4)
         VAR HP Vec2 IO_tex;
      #elif half!=0
         VAR HP Vec2 IO_tex;
      #else
         VAR HP Vec2 IO_tex0, IO_tex1, IO_tex2, IO_tex3;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 2D.h"

      void main()
      {
         O_vtx=vtx_pos4();
         HP Vec2 tex=vtx_tex();
      #if DoGlow!=0
         IO_tex=tex-ImgSize.xy*Vec2((half!=0) ? 0.5 : 1.5, (half!=0) ? 0.5 : 1.5);
      #elif half!=0
         IO_tex=tex;
      #else
         #if DoClamp!=0
            IO_tex0=tex-ImgSize.xy;
            IO_tex1=tex+ImgSize.xy;
         #else
            IO_tex0=tex+ImgSize.xy*Vec2( 1.0,  1.0);
            IO_tex1=tex+ImgSize.xy*Vec2(-1.0,  1.0);
            IO_tex2=tex+ImgSize.xy*Vec2( 1.0, -1.0);
            IO_tex3=tex+ImgSize.xy*Vec2(-1.0, -1.0);
         #endif
      #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"
      #include "Glsl PS 2D.h"
      #include "Glsl VS 2D.h"

      MP Vec BloomColor(MP Vec color)
      {
      #if saturate!=0
         return color*BloomParams.y+BloomParams.z;
      #else
         MP Flt col_lum=Max(Max(color.r, color.g), color.b), lum=col_lum*BloomParams.y+BloomParams.z;
         return (lum>0.0) ? color*(lum/col_lum) : Vec(0.0, 0.0, 0.0);
      #endif
      }

      void main()
      {
      #if DoGlow!=0
         MP Vec  color=Vec (0.0, 0.0, 0.0);
         MP Vec4 glow =Vec4(0.0, 0.0, 0.0, 0.0);
         for(Int y=0; y<res; y++)
         for(Int x=0; x<res; x++)
         {
            MP Vec4 c=texture2DLod(Img, UVClamp(IO_tex+ImgSize.xy*Vec2(x, y), DoClamp!=0), 0.0);
            color   +=c.rgb;
            glow.rgb+=c.rgb*c.a;
            glow.a   =Max(glow.a, c.a);
         }
         MP Flt eps=HALF_MIN;
         glow.rgb*=2.0*glow.a/Max(Max(Max(glow.r, glow.g), glow.b), eps); // NaN (increase by 2 because normally it's too small)
         gl_FragColor.rgb=Max(BloomColor(color), glow.rgb);
      #elif half!=0
         gl_FragColor.rgb=BloomColor(texture2DLod(Img, UVClamp(IO_tex, DoClamp!=0), 0.0).rgb);
      #else
         #if DoClamp!=0
            HP Vec2 tex_min=UVClamp(IO_tex0, true),
                    tex_max=UVClamp(IO_tex1, true);
            MP Vec col=texture2DLod(Img, Vec2(tex_min.x, tex_min.y), 0.0).rgb // keep this outside of Sat(..) if used, because compilation will fail on Galaxy Tab 2 and some others
                      +texture2DLod(Img, Vec2(tex_max.x, tex_min.y), 0.0).rgb
                      +texture2DLod(Img, Vec2(tex_min.x, tex_max.y), 0.0).rgb
                      +texture2DLod(Img, Vec2(tex_max.x, tex_max.y), 0.0).rgb;
         #else
            MP Vec col=texture2DLod(Img, IO_tex0, 0.0).rgb
                      +texture2DLod(Img, IO_tex1, 0.0).rgb
                      +texture2DLod(Img, IO_tex2, 0.0).rgb
                      +texture2DLod(Img, IO_tex3, 0.0).rgb;
         #endif
            gl_FragColor.rgb=BloomColor(col);
      #endif
         gl_FragColor.a=0.0;
      }
   @PS_END

@GROUP_END


@GROUP "Bloom"
   @SHARED
      #include "Glsl.h"

      VAR HP Vec2 IO_tex;
   @SHARED_END

   @VS
      #include "Glsl VS.h"

      void main()
      {
          O_vtx=vtx_pos4();
         IO_tex=vtx_tex ();
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      PAR MP Vec BloomParams;

      void main()
      {
         gl_FragColor.rgb=texture2DLod(Img, IO_tex, 0.0).rgb*BloomParams.x + texture2DLod(Img1, IO_tex, 0.0).rgb;
         gl_FragColor.a  =1.0; // force full alpha so back buffer effects can work ok
      }
   @PS_END

@GROUP_END


// can use 'RTSize' instead of 'ImgSize' since there's no scale
@GROUP "ShdBlurX" // params: range
   @SHARED
      #include "Glsl.h"

      VAR HP Vec2 IO_tex;
   @SHARED_END

   @VS
      #include "Glsl VS.h"

      void main()
      {
          O_vtx=Vec4(vtx_pos2(), Flt(!REVERSE_DEPTH), 1.0);
         IO_tex=vtx_tex();
      }
   @VS_END

   @PS
      #include "Glsl VS 2D.h"
      #include "Glsl PS.h"
      #include "Glsl Matrix.h"
      PAR HP Matrix4(ProjMatrix);
      #include "Glsl Depth.h"
      PAR HP Flt DepthWeightScale;

      inline HP Vec2 DepthWeightMAD(HP Flt depth) {return Vec2(-1.0/(depth*DepthWeightScale+0.004), 2.0);}
      inline MP Flt  DepthWeight   (MP Flt delta, HP Vec2 dw_mad) {return Sat(Abs(delta)*dw_mad.x + dw_mad.y);}

      void main()
      {
         MP Flt  weight=0.5,
                 color =               texture2DLod(ImgX , IO_tex, 0.0).x*weight;
         HP Flt  z     =LinearizeDepth(texture2DLod(Depth, IO_tex, 0.0).x, true);
         HP Vec2 dw_mad=DepthWeightMAD(z), t; t.y=IO_tex.y;
         for(MP Int i=-range; i<=range; i++)if(i!=0)
         {
            t.x=RTSize.x*(Flt(2*i)+((i>0) ? -0.5 : 0.5))+IO_tex.x;
            MP Flt w=DepthWeight(z-LinearizeDepth(texture2DLod(Depth, t, 0.0).x, true), dw_mad);
            color +=w*texture2DLod(ImgX, t, 0.0).x;
            weight+=w;
         }
         gl_FragColor=Vec4(color/weight, 0.0, 0.0, 0.0);
      }
   @PS_END

@GROUP_END


// can use 'RTSize' instead of 'ImgSize' since there's no scale
@GROUP "ShdBlurY" // params: range
   @SHARED
      #include "Glsl.h"

      VAR HP Vec2 IO_tex;
   @SHARED_END

   @VS
      #include "Glsl VS.h"

      void main()
      {
          O_vtx=Vec4(vtx_pos2(), Flt(!REVERSE_DEPTH), 1.0);
         IO_tex=vtx_tex();
      }
   @VS_END

   @PS
      #include "Glsl VS 2D.h"
      #include "Glsl PS.h"
      #include "Glsl Matrix.h"
      PAR HP Matrix4(ProjMatrix);
      #include "Glsl Depth.h"
      PAR HP Flt DepthWeightScale;

      inline HP Vec2 DepthWeightMAD(HP Flt depth) {return Vec2(-1.0/(depth*DepthWeightScale+0.004), 2.0);}
      inline MP Flt  DepthWeight   (MP Flt delta, HP Vec2 dw_mad) {return Sat(Abs(delta)*dw_mad.x + dw_mad.y);}

      void main()
      {
         MP Flt  weight=0.5,
                 color =               texture2DLod(ImgX , IO_tex, 0.0).x*weight;
         HP Flt  z     =LinearizeDepth(texture2DLod(Depth, IO_tex, 0.0).x, true);
         HP Vec2 dw_mad=DepthWeightMAD(z), t; t.x=IO_tex.x;
         for(MP Int i=-range; i<=range; i++)if(i!=0)
         {
            t.y=RTSize.y*(Flt(2*i)+((i>0) ? -0.5 : 0.5))+IO_tex.y;
            MP Flt w=DepthWeight(z-LinearizeDepth(texture2DLod(Depth, t, 0.0).x, true), dw_mad);
            color +=w*texture2DLod(ImgX, t, 0.0).x;
            weight+=w;
         }
         gl_FragColor=Vec4(color/weight, 0.0, 0.0, 0.0);
      }
   @PS_END

@GROUP_END


@GROUP "Particle" // params: palette, anim, motion_stretch, stretch_alpha

   @SHARED
      #include "Glsl.h"

      #define ANIM_NONE   0
      #define ANIM_YES    1
      #define ANIM_SMOOTH 2

      VAR MP Vec4 IO_col;
      VAR HP Vec2 IO_tex;
      #if anim==ANIM_SMOOTH
         VAR HP Vec2 IO_tex1;
         VAR MP Flt  IO_tex_blend;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 2D.h"
      #include "Glsl VS 3D.h"

      PAR MP Vec2 ParticleFrames;

      void main()
      {
         IO_tex=vtx_tex();
         IO_col=((palette!=0) ? vtx_colorF() : vtx_colorFast());

         MP Flt  size  =vtx_size(),
                 angle =vtx_tanW();
         HP Vec  pos   =TransformPos(vtx_pos());
         MP Vec2 offset=IO_tex; offset=offset*Vec2(2.0, -2.0)+Vec2(-1.0, 1.0); offset=Rotate(offset, Vec2(Cos(angle), Sin(angle)));

         #if motion_stretch!=0
         if(pos.z>0.0)
         {
            #define PARTICLE_PROJECT 100.0
            MP Vec  vel =TransformDir(vtx_tan()); if(vel.z<0.0)vel=-vel;
            HP Vec  pos1=pos+vel/PARTICLE_PROJECT;
            MP Vec2 vel2=(pos1.xy/pos1.z - pos.xy/pos.z)*PARTICLE_PROJECT;
            MP Flt  len =Length(vel2)+HALF_MIN;
            {
               MP Vec2 x=vel2*(vel2.x/len),
                       y=vel2*(vel2.y/len);
               offset=Vec2(offset.x*(x.x+1.0) + offset.y*y.x, offset.x*x.y + offset.y*(y.y+1.0));
               if(stretch_alpha!=0)
               {
                  if(palette!=0)IO_col  /=1.0+len; // in RM_PALETTE each component
                  else          IO_col.a/=1.0+len; // in RM_BLEND   only alpha
               }
            }
         }
         #endif

         pos.xy+=offset*size;

         // sky
         MP Vec mp_pos =pos;
         MP Flt d      =Length(mp_pos);
         MP Flt opacity=Sat(d*SkyFracMulAdd.x + SkyFracMulAdd.y);
         if(palette!=0)IO_col  *=opacity; // in RM_PALETTE each component
         else          IO_col.a*=opacity; // in RM_BLEND   only alpha

         #if anim!=ANIM_NONE
         {
            MP Flt frames=ParticleFrames.x*ParticleFrames.y,
                   frame =Frac(vtx_tex1().x/frames)*frames; // frame=[0..frames)
            HP Flt f     =Floor(frame); // keep this as HP because flickering can occur if MP is used (for example when ParticleFrames is 5x5)
            #if anim==ANIM_SMOOTH // frame blending
            {
               MP Flt f1=f+1.0; if(f1+0.5>=frames)f1=0.0; // f1=(f+1)%frames;
               IO_tex1     =IO_tex;
               IO_tex_blend=frame-f; // [0..1) frac

               f1/=ParticleFrames.x;
               MP Flt y=Floor(f1);
               IO_tex1.y+=y;
               IO_tex1  /=ParticleFrames;
               IO_tex1.x+=f1-y;
            }
            #endif
            f/=ParticleFrames.x;
            MP Flt y=Floor(f);
            IO_tex.y+=y;
            IO_tex  /=ParticleFrames;
            IO_tex.x+=f-y;
         }
         #endif
         O_vtx=Project(pos);
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      void main()
      {
         MP Vec4 tex=          Tex(Img, IO_tex );
      #if anim==ANIM_SMOOTH
                 tex=Lerp(tex, Tex(Img, IO_tex1), IO_tex_blend);
      #endif
         if(palette!=0)gl_FragColor=IO_col*tex.a;
         else          gl_FragColor=IO_col*tex  ;
      }
   @PS_END

@GROUP_END


@GROUP "Sky" // params: per_vertex, DENSITY, textures, stars, clouds

   @SHARED
      #include "Glsl.h"

      PAR HP Flt  SkyDnsExp      ; // need high precision
      PAR MP Flt  SkyHorExp      ,
                  SkyBoxBlend    ;
      PAR MP Vec4 SkyHorCol      ,
                  SkySkyCol      ;
      PAR MP Vec2 SkyDnsMulAdd   ,
                  SkySunHighlight;
      PAR MP Vec  SkySunPos      ;

      #define LCScale 0.2

      struct CloudLayer
      {
         MP Vec4 color;
         HP Vec2 scale;
         HP Vec2 position;
      };

      PAR HP Flt LCScaleY;

      PAR CloudLayer CL[1];

      inline MP Vec4 SkyColor(MP Flt y)
      {
         MP Flt hor=Pow(1.0-Sat(y), SkyHorExp);
         return Lerp(SkySkyCol, SkyHorCol, hor);
      }

         VAR MP Vec  IO_tex;
      #if stars!=0
         VAR MP Vec  IO_tex_star;
      #endif
      #if per_vertex!=0
         VAR MP Vec4 IO_col;
      #endif
      #if clouds!=0
         VAR HP Vec  IO_tex_cloud;
         VAR MP Vec4 IO_col_cloud;
      #endif
   @SHARED_END

   @VS
      #include "Glsl VS.h"
      #include "Glsl VS 3D.h"

      PAR MP Matrix3(SkyStarOrn);

      void main()
      {
          O_vtx=Project(TransformPos(vtx_pos()));
         IO_tex=vtx_pos();
      #if stars!=0
         IO_tex_star=Transform(vtx_pos(), SkyStarOrn);
      #endif
      #if per_vertex!=0
         IO_col=SkyColor(vtx_pos().y);
      #endif
         #if clouds!=0
         {
            MP Vec pos=vtx_pos(); pos*=Vec(LCScale, 1.0, LCScale);
            IO_col_cloud=CL[0].color; IO_col_cloud.a*=Sat(pos.y*8.0-0.15); // CloudAlpha
            IO_tex_cloud=pos;
         }
         #endif
      }
   @VS_END

   @PS
      #include "Glsl PS.h"

      inline MP Vec SkyTex()
      {
         if(textures==2)return Vec(Lerp(TexCube(Cub, IO_tex).rgb, TexCube(Cub1, IO_tex).rgb, SkyBoxBlend));else
         if(textures==1)return Vec(     TexCube(Cub, IO_tex).rgb                                         );else
         {
            MP Vec4 col;
            #if per_vertex==0
            {
               MP Vec tex=Normalize(IO_tex);
                      col=SkyColor (tex.y );

               MP Flt cos      =Dot(SkySunPos, tex);
               MP Flt highlight=1.0+Sqr(cos)*((cos>0.0) ? SkySunHighlight.x : SkySunHighlight.y); // rayleigh
               col.rgb*=highlight;
            }
            #else
               col=IO_col;
            #endif

            #if stars!=0
               col.rgb=Lerp(TexCube(Cub, IO_tex_star).rgb, col.rgb, col.a);
            #endif

            return col.rgb;
         }
      }

      void main()
      {
         MP Vec col=SkyTex();
         #if clouds!=0
         {
            HP Vec2 uv=Normalize(IO_tex_cloud).xz;
            MP Vec4 tex=Tex(Img, uv*CL[0].scale+CL[0].position)*IO_col_cloud;
            col.rgb=Lerp(col.rgb, tex.rgb, tex.a);
         }
         #endif
         gl_FragColor.rgb=col;
         gl_FragColor.a  =0.0;
      }
   @PS_END

@GROUP_END


@GROUP "SMAAEdge" // params: GAMMA
   @SHARED
      #define SMAA_GLSL_3 1
      #include "Glsl.h"
      #include "Glsl VS 2D.h"
      PAR MP Flt SMAAThreshold;
      #include "SMAA_config.h"
      VAR HP Vec2 texcoord;
      VAR HP Vec4 offset[3];
   @SHARED_END

   @VS
      #define SMAA_INCLUDE_PS 0
      #include "Glsl VS.h"
      #include "SMAA.h"
      void main()
      {
         O_vtx=vtx_pos4();
         texcoord=vtx_tex();
         SMAAEdgeDetectionVS(texcoord, offset);
      }
   @VS_END

   @PS
      #define SMAA_INCLUDE_VS 0
      #include "Glsl PS.h"
      #include "SMAA.h"
      void main()
      {
         gl_FragColor.rg=SMAAColorEdgeDetectionPS(texcoord, offset, Img, GAMMA);
         gl_FragColor.b=0;
         gl_FragColor.a=1;
      }
   @PS_END
@GROUP_END


@GROUP "SMAABlend"
   @SHARED
      #define SMAA_GLSL_3 1
      #include "Glsl.h"
      #include "Glsl VS 2D.h"
      #include "SMAA_config.h"
      VAR HP Vec2 texcoord, pixcoord;
      VAR HP Vec4 offset[3];
   @SHARED_END

   @VS
      #define SMAA_INCLUDE_PS 0
      #include "Glsl VS.h"
      #include "SMAA.h"
      void main()
      {
         O_vtx=vtx_pos4();
         texcoord=vtx_tex();
         SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
      }
   @VS_END

   @PS
      #define SMAA_INCLUDE_VS 0
      #include "Glsl PS.h"
      #include "SMAA.h"
      void main()
      {
         gl_FragColor=SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, Img, Img1, Img2, 0);
      }
   @PS_END
@GROUP_END


@GROUP "SMAA"
   @SHARED
      #define SMAA_GLSL_3 1
      #include "Glsl.h"
      #include "Glsl VS 2D.h"
      #include "SMAA_config.h"
      VAR HP Vec2 texcoord;
      VAR HP Vec4 offset;
   @SHARED_END

   @VS
      #define SMAA_INCLUDE_PS 0
      #include "Glsl VS.h"
      #include "SMAA.h"
      void main()
      {
         O_vtx=vtx_pos4();
         texcoord=vtx_tex();
         SMAANeighborhoodBlendingVS(texcoord, offset);
      }
   @VS_END

   @PS
      #define SMAA_INCLUDE_VS 0
      #include "Glsl PS.h"
      #include "SMAA.h"
      void main()
      {
         gl_FragColor=SMAANeighborhoodBlendingPS(texcoord, offset, Img, Img1);
      }
   @PS_END
@GROUP_END
