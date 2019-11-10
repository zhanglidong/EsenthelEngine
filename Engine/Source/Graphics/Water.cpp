/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
      WaterClass   Water;
const WaterMtrl   *WaterMtrlLast;
      WaterMtrlPtr WaterMtrlNull;
DEFINE_CACHE(WaterMtrl, WaterMtrls, WaterMtrlPtr, "Water Material");
/******************************************************************************/
// WATER PARAMETERS
/******************************************************************************/
Vec  WaterMtrlParams::colorS(              )C {return        LinearToSRGB(color  ) ;}
void WaterMtrlParams::colorS(C Vec &color_s)  {return colorL(SRGBToLinear(color_s));}

Vec  WaterMtrl::colorUnderwater0S(              )C {return                   LinearToSRGB(color_underwater0) ;}
void WaterMtrl::colorUnderwater0S(C Vec &color_s)  {return colorUnderwater0L(SRGBToLinear(color_s          ));}

Vec  WaterMtrl::colorUnderwater1S(              )C {return                   LinearToSRGB(color_underwater1) ;}
void WaterMtrl::colorUnderwater1S(C Vec &color_s)  {return colorUnderwater1L(SRGBToLinear(color_s          ));}
/******************************************************************************/
WaterMtrl::WaterMtrl()
{
   color=1;
   colorUnderwater0S(Vec(0.26f, 0.35f, 0.42f));
   colorUnderwater1S(Vec(0.10f, 0.20f, 0.30f));

   smooth            =1;
   reflect           =0.02f;
   normal            =1;
   wave_scale        =0;

   scale_color       =1.0f/200;
   scale_normal      =1.0f/10;
   scale_bump        =1.0f/100;

   density           =0.3f;
   density_add       =0;

   refract           =0.10f;
   refract_reflection=0.06f;
   refract_underwater=0.01f;
}
/******************************************************************************/
WaterMtrl& WaterMtrl:: colorMap(C ImagePtr &image) {T. _color_map=image; return T;}
WaterMtrl& WaterMtrl::normalMap(C ImagePtr &image) {T._normal_map=image; return T;}
WaterMtrl& WaterMtrl::reset()
{
   T=WaterMtrl();
   return validate();
}
WaterMtrl& WaterMtrl::validate()
{
   if(WaterMtrlLast==this)WaterMtrlLast=null;
   return T;
}
void WaterMtrl::set()C
{
   if(WaterMtrlLast!=this)
   {
      WaterMtrlLast=this;
      WS.WaterMaterial->set<WaterMtrlParams>(T);
      Sh.Col[0]->set( _color_map());
      Sh.Nrm[0]->set(_normal_map());
    //Sh.Ext[0]->set( _extra_map());
   }
}
/******************************************************************************/
Bool WaterMtrl::save(File &f, CChar *path)C
{
   f.cmpUIntV(1); // version

   f<<SCAST(C WaterMtrlParams, T)
    <<color_underwater0
    <<color_underwater1
    <<refract_underwater;

   // textures
   f.putStr( _color_map.name(path)); // !! can't use 'id' because textures are stored in "Tex/" folder, so there's no point in using 'putAsset' !!
   f.putStr(_normal_map.name(path));

   return f.ok();
}
Bool WaterMtrl::load(File &f, CChar *path)
{
   Char temp[MAX_LONG_PATH];
   switch(f.decUIntV()) // version
   {
      case 1:
      {
         f>>SCAST(WaterMtrlParams, T)
          >>color_underwater0
          >>color_underwater1
          >>refract_underwater;
         f.getStr(temp);  _color_map.require(temp, path);
         f.getStr(temp); _normal_map.require(temp, path);
         if(f.ok())return true;
      }break;

      case 0:
      {
         Flt temp_flt;
         Vec temp_vec;
         reset();
         f>>density>>density_add>>temp_flt>>temp_flt>>scale_color>>scale_normal>>scale_bump>>normal>>temp_flt>>temp_flt>>refract>>refract_reflection>>refract_underwater>>temp_flt>>wave_scale>>temp_flt>>temp_flt;
         f>>temp_vec>>color>>color_underwater0>>color_underwater1;
         colorS           (color);
         colorUnderwater0S(color_underwater0);
         colorUnderwater1S(color_underwater1);
         scale_color =1/scale_color ;
         scale_normal=1/scale_normal;
         scale_bump  =1/scale_bump  ;
          _color_map.require(f._getStr(), path);
         _normal_map.require(f._getStr(), path);
                             f._getStr();
         if(f.ok())return true;
      }break;
   }
   reset(); return false;
}
Bool WaterMtrl::save(C Str &name)C
{
   File f; if(f.writeTry(name)){if(save(f, _GetPath(name)) && f.flush())return true; f.del(); FDelFile(name);}
   return false;
}
Bool WaterMtrl::load(C Str &name)
{
   File f; if(f.readTry(name))return load(f, _GetPath(name));
   reset(); return false;
}
/******************************************************************************/
// WATER PLANE
/******************************************************************************/
WaterClass::WaterClass()
{
   draw                 =false;
   reflection_allow     =!MOBILE;
   reflection_shadows   =false;
   reflection_resolution=2;
   plane.normal.y       =1;
  _max_1_light          =true;
  _draw_plane_surface   =false;
  _use_secondary_rt     =false;
  _began                =false;
  _swapped_ds           =false;
  _reflect_renderer     =(MOBILE ? RT_FORWARD : RT_DEFERRED);
}
void WaterClass::del()
{
  _mshr.del();
}
void WaterClass::create()
{
   del();

   // create mesh
   MeshBase mshb; mshb.createPlane(80, 130).scaleMove(Vec(1, -1, 1), Vec(0, 1, 0));

   REPA(mshb.vtx)
   {
      Vec &pos=mshb.vtx.pos(i);
      if(  pos.x<=EPS || pos.x>=1-EPS
      ||   pos.y<=EPS || pos.y>=1-EPS)pos.xy=(pos.xy-0.5f)*6+0.5f;
   }
  _mshr.create(mshb);
}
/******************************************************************************/
WaterClass& WaterClass::reflectionRenderer(RENDER_TYPE type)
{
   Clamp(type, RENDER_TYPE(0), RENDER_TYPE(RT_NUM-1));
   if(type==RT_DEFERRED && D.deferredUnavailable())return T;
   if(type!=_reflect_renderer)
   {
     _reflect_renderer=type;
      if(type==RT_DEFERRED && D.deferredMSUnavailable())D.samples(1); // disable multi-sampling if we can't support it
      D.setShader(); // needed because shaders are set only for current renderer type
   }
   return T;
}
WaterClass& WaterClass::max1Light(Bool on) {/*if(_max_1_light!=on)*/_max_1_light=on; return T;}
/******************************************************************************/
void WaterClass::prepare() // this is called at the start
{
   WaterMtrlLast=null; // reset 'WaterMtrlLast' because we may have 'set' regular Materials which share the same texture shader images as WaterMaterials, so make sure we will reset them when setting water materials

  _under_mtrl        =null;
  _draw_plane_surface=false;

   // check visibility for water plane
   if(draw)
   {
      Matrix3 matrix; matrix.setUp(plane.normal);
      Flt     size=D.viewRange();
      VecD    p   =PointOnPlane(CamMatrix.pos, plane.pos, plane.normal);

      under(plane, T);
      if(Frustum(Box(size*2, wave_scale, size*2), MatrixM(matrix, p)))
      {
        _draw_plane_surface=true;
        _quad.set(p+size*(matrix.z-matrix.x), p+size*(matrix.x+matrix.z), p+size*(matrix.x-matrix.z), p+size*(-matrix.x-matrix.z));
      }
   }

   // !! it is important to call this as the first thing during rendering, because 'Game.WorldManager.draw' assumes so, if this needs to be changed then rework 'Game.WorldManager.draw' !!
   // test for under-water on all other water elements
   {
     _mode=MODE_UNDERWATER; Renderer.mode(RM_WATER); Renderer._render();
     _mode=MODE_DRAW      ;
   }

   //  set draw plane surface
   if(_draw_plane_surface)
   {
      if(_under_mtrl && _under_step>=1) // totally under water
      {
        _draw_plane_surface=false;
      }else
      {
         VecD left =FullScreenToPos(Vec2(D.viewRect().min.x, 0), D.viewRange()),
              right=FullScreenToPos(Vec2(D.viewRect().max.x, 0), D.viewRange());

         left =PointOnPlaneRay(left , plane.pos, plane.normal, CamMatrix.y);
         right=PointOnPlaneRay(right, plane.pos, plane.normal, CamMatrix.y);

         Vec2 l2=PosToFullScreen(left ),
              r2=PosToFullScreen(right);

         Rect rect=D.viewRect();
         Flt  dot =Dot(CamMatrix.y, plane.normal);

         if(dot>EPS)
         {
               rect.max.y=Max(l2.y, r2.y);
            if(rect.max.y<rect.min.y)_draw_plane_surface=false;else MIN(rect.max.y, D.viewRect().max.y);
         }else
         if(dot<-EPS)
         {
               rect.min.y=Min(l2.y, r2.y);
            if(rect.min.y>rect.max.y)_draw_plane_surface=false;else MAX(rect.min.y, D.viewRect().min.y);
         }

       /*l2.draw(RED);
         r2.draw(RED);
         rect.draw(0x30FFFFFF);*/

         if(_draw_plane_surface)
         {
            // full    range is D.viewRect().min.y .. D.viewRect().max.y
            // drawing range is       rect  .min.y ..       rect  .max.y
            Flt y_min_frac=LerpR(D.viewRect().min.y, D.viewRect().max.y, rect.min.y),
                y_max_frac=LerpR(D.viewRect().min.y, D.viewRect().max.y, rect.max.y);
         #if 1
           _y_mul_add.x=y_max_frac-y_min_frac;
           _y_mul_add.y=1-y_max_frac;
         #else
           _y_mul_add.x=1;
           _y_mul_add.y=0;
         #endif

            WS.load();
            if(reflection_allow && reflect>EPS_COL)Renderer.requestMirror(plane, 1, reflection_shadows, reflection_resolution);
         }
      }
   }

   // test for lake reflections
   if(reflection_allow && reflect>EPS_COL && !Renderer._mirror_want)
      if(!(_under_mtrl && _under_step>=1))
   {
     _mode=MODE_REFLECTION; Renderer.mode(RM_WATER); Renderer._render();
     _mode=MODE_DRAW      ;
   }
}
/******************************************************************************/
void WaterClass::setEyeViewportCam()
{
   Renderer.setEyeViewportCam();
   if(Renderer._mirror_rt)
   {
      Vec2 scale=1,
           offs =0;
      if(Renderer._stereo)
      {
         scale.x=2;
         if(Renderer._eye)offs.x-=1;
         offs.x-=ProjMatrixEyeOffset[Renderer._eye]*0.5f;
      }
      WS.WaterReflectMulAdd->set(Vec4(scale, offs));
   }else
   {
      WS.WaterReflectMulAdd->set(Vec4(1, 1, 0, 0));
   }
}
/******************************************************************************/
void WaterClass::begin()
{
   if(!_began)
   {
     _began=true;

      WS.load();
    //Renderer._has_glow=true;
      D.alpha(ALPHA_NONE);

      if(_use_secondary_rt)
      {
        _swapped_ds=false;
         Renderer._water_col.get  (ImageRTDesc(Renderer._col->w(), Renderer._col->h(),                                   IMAGERT_SRGB)); // here Alpha is unused
         Renderer._water_nrm.get  (ImageRTDesc(Renderer._col->w(), Renderer._col->h(), D.signedNrmRT() ? IMAGERT_RGB_S : IMAGERT_RGB )); // here Alpha is unused
         Renderer._water_ds .getDS(            Renderer._col->w(), Renderer._col->h(), 1, false);

         if(Renderer.stage)switch(Renderer.stage)
         {
            case RS_WATER_COLOR : Renderer._water_col->clearFull(); break;
            case RS_WATER_NORMAL: Renderer._water_nrm->clearFull(); break;
         }

         // set RT's and depth buffer
         if(Shader *shader=Sh.SetDepth) // if we can copy depth buffer from existing solid's, then do it, to prevent drawing water pixels if they're occluded
         {
            Renderer.set(null, Renderer._water_ds, true);
            D.depthLock  (true); D.depthFunc(FUNC_ALWAYS); D.stencil(STENCIL_ALWAYS_SET, 0); shader->draw();
            D.depthUnlock(    ); D.depthFunc(FUNC_LESS  ); D.stencil(STENCIL_NONE         );
          //Renderer.set(Renderer._water_col, Renderer._water_nrm, null, null, Renderer._water_ds, true); don't set, instead swap first and set later
           _swapped_ds=Renderer.swapDS1S(Renderer._water_ds); // try to swap DS to put existing stencil values into '_water_ds' because we will write water depths onto '_water_ds' and we want to use it later instead of '_ds_1s' so we want all stencil values to be kept
            Renderer.set(Renderer._water_col, Renderer._water_nrm, null, null, Renderer._water_ds, true);
         }else // if we can't copy then just clear it
         {
            Renderer.set(Renderer._water_col, Renderer._water_nrm, null, null, Renderer._water_ds, true);
            D.clearDS();
         }
      }else
      {
         if(Lights.elms() && Lights[0].type==LIGHT_DIR)Lights[0].dir.set();else LightDir(Vec(0,-1,0), VecZero, 0).set();
         // we're going to draw water on top of existing RT, including refraction, so we need to have a color copy of what's underwater (solid) for the refraction, also we want to do softing so we need to backup depth because we can't read and write to depth in the same time
         Renderer._water_col.get(ImageRTDesc(Renderer._col->w(), Renderer._col->h(), GetImageRTType(Renderer._col->type()))); // create RT for the copy
         Renderer._col->copyHw(*Renderer._water_col, false, D.viewRect()); // copy
         if(_shader_soft)
         {
            Renderer._water_ds.get(ImageRTDesc(Renderer._col->w(), Renderer._col->h(), IMAGERT_F32));
            Renderer._ds_1s->copyHw(*Renderer._water_ds, false, D.viewRect());
         }
         setImages(Renderer._water_col, Renderer._water_ds);
         Renderer.set(Renderer._col, Renderer._ds, true);
      }

      SetOneMatrix();
      D.wire      (Renderer.wire);
      D.depth     (true);
      D.cull      (true);
      D.sampler3D (    );
      D.stencil   (STENCIL_WATER_SET, STENCIL_REF_WATER);
      WS.WaterFlow->set(Time.time()*3);
      WS.WaterOfs ->set(_offset      );
      Rect uv=D.screenToUV(D.viewRect()); // UV
      if(Renderer._mirror_rt)
      {
         uv=Round(uv*Renderer._mirror_rt->size()); // convert to RectI of RT size
         uv.extend(-0.5f)/=Renderer._mirror_rt->size(); // decrease half pixel to avoid tex filtering issues and convert back to UV
         WS.WaterPlanePos->set(Renderer._mirror_plane.pos   *CamMatrixInv      );
         WS.WaterPlaneNrm->set(Renderer._mirror_plane.normal*CamMatrixInv.orn());
      }
      WS.WaterClamp->set(uv);
      setEyeViewportCam();
   }
}
void WaterClass::end()
{
   if(_began)
   {
     _began=false;

      D.wire     (false);
      D.sampler2D();
      D.stencil  (STENCIL_NONE);

      MaterialClear(); // clear Materials because we've potentially set WaterMaterials which share the same textures

      if(!_use_secondary_rt)
      {
         endImages();
         Renderer._water_col.clear();
         Renderer._water_ds .clear();
         if(Renderer._ds!=Renderer._ds_1s && Renderer._ds_1s) // if we've drawn to MSAA ds, then we need to setup 1S DS
         {
            Renderer.set(null, Renderer._ds_1s, true);
            D.alpha(ALPHA_NONE);
            D.depthLock  (true); Sh.ResolveDepth->draw(); // here we can keep FUNC_LESS because 1S DS was already set before and we just need to apply water on top
            D.depthUnlock(    );
         }
      }
   }
}
/******************************************************************************/
void WaterClass::under(C PlaneM &plane, WaterMtrl &mtrl)
{
   Flt step=(Dist(CamMatrix.pos, plane)-D.viewFromActual())/-WATER_TRANSITION+1;
   if( step>EPS_COL)
   {
      if(!_under_mtrl || step>_under_step){_under_mtrl=&mtrl; _under_step=step; _under_plane=plane; _under_plane.pos+=_under_plane.normal*(D.viewFromActual()+WATER_TRANSITION);}
   }
}
/******************************************************************************/
void WaterClass::setImages(Image *src, Image *depth)
{
   // these are used by both draw surface and apply water shaders
   Sh.Img  [1]->set(Renderer._mirror_rt); Sh.Img[1]->_sampler=&SamplerLinearClamp; // reflection
   Sh.Img  [2]->set(          src      ); Sh.Img[2]->_sampler=&SamplerLinearClamp; // solid underwater
   Sh.ImgXF[0]->set(          depth    );                                          // solid depth
}
void WaterClass::endImages()
{
   Sh.Img[1]->_sampler=null;
   Sh.Img[2]->_sampler=null;
}
/******************************************************************************/
Bool WaterClass::ocean()
{
   #define EPS_WAVE_SCALE 0.001f // 1 mm
   return _color_map && wave_scale>EPS_WAVE_SCALE;
}
Shader* WaterClass::shader()
{
   return _use_secondary_rt ? (ocean() ? WS.Ocean  : WS.Lake )
                            : (ocean() ? WS.OceanL : WS.LakeL)[_shader_shadow][_shader_soft][_shader_reflect_env][_shader_reflect_mirror];
}
/******************************************************************************/
void WaterClass::drawSurfaces()
{
   // these are used only when '_use_secondary_rt' is disabled
  _shader_shadow        =((Lights.elms() && Lights[0].type==LIGHT_DIR && Lights[0].shadow) ? D.shadowMapNumActual() : 0);
  _shader_soft          =Renderer.canReadDepth();
  _shader_reflect_env   =(         D.envMap()!=null);
  _shader_reflect_mirror=(Renderer._mirror_rt!=null);

   REPS(Renderer._eye, Renderer._eye_num)
   {
      setEyeViewportCam();

      if(_draw_plane_surface)
         if(Shader *shader=T.shader())
      {
         begin();
         set  ();

         if(ocean())
         {
            WS.WaterPlanePos->set(plane.pos   *CamMatrixInv      );
            WS.WaterPlaneNrm->set(plane.normal*CamMatrixInv.orn());
            WS.WaterYMulAdd ->set(_y_mul_add                     );
            shader->begin(); _mshr.set().draw();
         }else
         {
            VI.shader(shader);
            VI.cull  (true  );
            VI.quad  (_quad );
            VI.end   (      );
         }
      }

      if(!(_under_mtrl && _under_step>=1)) // don't draw any surfaces when totally under water
      {
         Renderer.mode(RM_WATER); Renderer._render();
      }
   }

   end();
}
/******************************************************************************/
WaterClass& WaterClass::update(C Vec2 &vel)
{
  _offset+=vel*Time.d();
   return T;
}
/******************************************************************************/
// WATER MESH
/******************************************************************************/
void WaterMesh::zero()
{
   depth=0;
  _lake =false;
  _box.zero();
}
WaterMesh::WaterMesh() {zero();}
void WaterMesh::del()
{
  _mshb.del();
  _mshr.del();
  _material.clear();
   zero();
}
void WaterMesh::create(C MeshBase &src, Bool lake, Flt depth, C WaterMtrlPtr &material)
{
   T._lake    =lake;
   T. depth   =depth;
   T._material=material;
  _mshb.create(src, VTX_POS|VTX_TEX0|TRI_IND|QUAD_IND).getBox(_box);
  _mshr.create(_mshb); // 'mshb' is kept for testing 'under'
   WS.load();
}
/******************************************************************************/
Bool WaterMesh::under(C Vec &pos, Flt *depth)C
{
   if(pos.x>=_box.min.x && pos.x<=_box.max.x
   && pos.z>=_box.min.z && pos.z<=_box.max.z
   && pos.y<=_box.max.y && pos.y>=_box.min.y-T.depth)
   {
      Bool flat=(_box.h()<=0.01f);

      // per face precision
      Vec2 pos2=pos.xz();
    C Vec *p   =_mshb.vtx.pos();
      REPA(_mshb.tri)
      {
       C VecI &ind=_mshb.tri.ind(i);
         Tri2  tri(p[ind.x].xz(), p[ind.y].xz(), p[ind.z].xz());
         if(Cuts(pos2, tri))
         {
            if(flat)
            {
               if(depth)*depth=_box.max.y-pos.y;
               return true;
            }else
            {
               Vec blend=TriBlend(pos2, tri);
               Flt water=p[ind.x].y*blend.x + p[ind.y].y*blend.y + p[ind.z].y*blend.z,
                   d    =water-pos.y;
               if( d>=0 && d<T.depth)
               {
                  if(depth)*depth=d;
                  return true;
               }
               return false;
            }
         }
      }
      REPA(_mshb.quad)
      {
       C VecI4 &ind=_mshb.quad.ind(i);
         Quad2  quad(p[ind.x].xz(), p[ind.y].xz(), p[ind.z].xz(), p[ind.w].xz());
         if(Cuts(pos2, quad))
         {
            if(flat)
            {
               if(depth)*depth=_box.max.y-pos.y;
               return true;
            }else
            {
               Vec blend=TriBlend(pos2, quad.tri013());
               Flt water=p[ind.x].y*blend.x + p[ind.y].y*blend.y + p[ind.w].y*blend.z,
                   d    =water-pos.y;
               if( d>=0 && d<T.depth)
               {
                  if(depth)*depth=d;
                  return true;
               }
               return false;
            }
         }
      }
   }
   return false;
}
/******************************************************************************/
Shader* WaterMesh::shader()C
{
   if(WaterMtrl *mtrl=getMaterial())
   {
      return Water._use_secondary_rt ? (_lake ? WS.Lake  : WS.River )
                                     : (_lake ? WS.LakeL : WS.RiverL)[Water._shader_shadow][Water._shader_soft][Water._shader_reflect_env][Water._shader_reflect_mirror];
   }
   return null;
}
/******************************************************************************/
void WaterMesh::draw()C
{
   if(WaterMtrl *mtrl=getMaterial())switch(Water._mode)
   {
      case WaterClass::MODE_DRAW:
      {
         if(Frustum(_box) && _mshr.is())
            if(Shader *shader=T.shader())
         {
            Water  .begin();
            mtrl  ->set  ();
            shader->begin(); _mshr.set().draw();
         }
      }break;

      case WaterClass::MODE_REFLECTION:
      {
         if(_box.h()<=0.01f /*&& mtrl->reflect>EPS_COL*/) // for reflections accept only flat waters, 'reflect' is taken globally
            if(CamMatrix.pos.y>=_box.min.y) // if camera is above surface
               if(Frustum(_box) && _mshr.is())
                  Renderer.requestMirror(PlaneM(VecD(0, _box.max.y, 0), Vec(0, 1, 0)), 0, Water.reflection_shadows, Water.reflection_resolution);
      }break;

      case WaterClass::MODE_UNDERWATER:
      {
         if(_mshb.vtxs())
         {
            Flt  depth, eps=D.viewFromActual()+WATER_TRANSITION;
            VecD test=CamMatrix.pos; test.y-=eps;
            if(under(test, &depth))
            {
               test.y+=depth;
               Water.under(PlaneM(test, Vec(0, 1, 0)), *mtrl);
            }
         }
      }break;
   }
}
/******************************************************************************/
Bool WaterMesh::save(File &f, CChar *path)C
{
   f.cmpUIntV(0); // version
   f<<_lake<<depth<<_box;
   if(_mshb.save(f))
   if(_mshr.save(f))
   {
      f._putStr(_material.name(path));
      return f.ok();
   }
   return false;
}
Bool WaterMesh::load(File &f, CChar *path)
{
   switch(f.decUIntV()) // version
   {
      case 0:
      {
         f>>_lake>>depth>>_box;
         if(_mshb.load(f))
         if(_mshr.load(f))
         {
           _material.require(f._getStr(), path);
            if(f.ok())
            {
               WS.load();
               return true;
            }
         }
      }break;
   }
   del(); return false;
}
/******************************************************************************/
// WATER DROPS
/******************************************************************************
struct WaterDrops
{
   Int    num   ; // number of water drops
   Vec   *pos   , // position
         *nrm   , // normal  (1st random direction)
         *tan   ; // tangent (2nd random direction)
   Flt   *time  , // time offset
         *blend ; // bone blend value
   VecB4 *matrix; // bone matrix index
   Image *image ; // drop texture

   // manage
   WaterDrops& del   (                                              ); // delete
   WaterDrops& create(Image &image,Int num                          ); // create with 'num' drops
   WaterDrops& create(Image &image,Int num,Shape   *shape,Int shapes); // create from shapes
   WaterDrops& create(Image &image,Int num,Ragdoll &ragdoll         ); // create from ragdoll

   // operations
   void setSkin(Skeleton &skeleton); // recalculate bone skinning values according to 'skeleton'

   // draw
   void draw(                           ); // render with  active     matrix
   void draw(Matrix           &matrix   ); // render with 'matrix'    matrix
   void draw(AnimatedSkeleton &anim_skel); // render with 'anim_skel' matrixes

  ~WaterDrops() {del ( );}
   WaterDrops() {Zero(T);}

   NO_COPY_CONSTRUCTOR(WaterDrops);
};
/******************************************************************************
WaterDrops& WaterDrops::del()
{
   Free(pos);
   Free(nrm);
   Free(tan);
   Free(time);
   Free(blend);
   Free(matrix);
   Zero(T); return T;
}
WaterDrops& WaterDrops::create(Image &image,Int num)
{
   del();

   T.image=&image;
   T.num  = num  ;
   Alloc(pos ,num);
   Alloc(nrm ,num);
   Alloc(tan ,num);
   Alloc(time,num);
   return T;
}
WaterDrops& WaterDrops::create(Image &image,Int num,Shape *shape,Int shapes)
{
   create(image,num);

   Box  box;
   Bool box_set=false;
   REP(shapes)
   {
      Box    b;
      Shape &s=shape[i];
      if(s.type==SHAPE_BALL)
      {
         Ball &ball=s.ball;
         b.set(ball.pos-ball.r,ball.pos+ball.r);
      }else
      if(s.type==SHAPE_CAPSULE)
      {
         Capsule &capsule=s.capsule;
         Vec      up     =capsule.up*(capsule.h*0.5f-capsule.r);
         b.from(capsule.pos-up,capsule.pos+up).extend(capsule.r);
      }else continue;
      if(!box_set){box_set=true; box=b;}else box|=b;
   }
   REP(num)
   {
      pos [i].zero();
      nrm [i]=Random  (Ball(1),false)*Random.f(1/1.2f,1.2f)*0.008f;
      tan [i]=Random  (Ball(1),false)*Random.f(1/1.2f,1.2f)*0.008f;
      time[i]=Random.f(10);
      Vec &p=pos[i];
      REPD(a,256)
      {
         p=Random(box);
         Bool cut=false;
         REP(shapes)
         {
            Shape &s=shape[i];
            if(s.type==SHAPE_BALL    && Cuts(p,s.ball   )){cut=true; break;}else
            if(s.type==SHAPE_CAPSULE && Cuts(p,s.capsule)){cut=true; break;}
         }
         if(cut)break;
      }
   }
   return T;
}
WaterDrops& WaterDrops::create(Image &image,Int num,Ragdoll &ragdoll)
{
   return create(image,num,ragdoll.shape,ragdoll.bones);
}
/******************************************************************************
void WaterDrops::setSkin(Skeleton &skeleton)
{
   if(!blend )Alloc(blend ,num);
   if(!matrix)Alloc(matrix,num);
   REP(num)skeleton.getSkin(pos[i],blend[i],matrix[i]);
}
/******************************************************************************
void WaterDrops::draw()
{
   VI.image    (image);
   VI.technique(blend ? Sh.RndrWaterdS : Sh.RndrWaterd);
   REP(num)
   {
      Flt t1=Sin(Time.time()*1.5f+time[i]),
          t2=Sin(Time.time()*1.7f+time[i]);
      Vec p=pos[i]+t1*nrm[i]+t2*tan[i];
      if(blend)VI.image(WHITE,0.020f,p,blend[i],matrix[i]);
      else     VI.image(WHITE,0.020f,p);
   }
   VI.end();
}
void WaterDrops::draw(Matrix &matrix)
{
   SetOneMatrix(matrix);
   draw();
}
void WaterDrops::draw(AnimatedSkeleton &anim_skel)
{
   anim_skel.setMatrix();
   draw();
}
/******************************************************************************/
UInt CreateWaterBaseTextures(Image &base_0, Image &base_1, C Image &col, C Image &alpha, C Image &bump, C Image &normal, C Image &smooth, C Image &reflect, C Image &glow, Bool resize_to_pow2, Bool flip_normal_y, FILTER_TYPE filter)
{
   // #WaterMaterialTextureLayout
   UInt  ret=0;
   Image dest_0, dest_1;
   {
      Image     col_temp; C Image *    col_src=&    col; if(    col_src->compressed())if(    col_src->copyTry(    col_temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))    col_src=&    col_temp;else goto error;
    //Image   alpha_temp; C Image *  alpha_src=&  alpha; if(  alpha_src->compressed())if(  alpha_src->copyTry(  alpha_temp, -1, -1, -1, IMAGE_L8A8         , IMAGE_SOFT, 1))  alpha_src=&  alpha_temp;else goto error;
      Image    bump_temp; C Image *   bump_src=&   bump; if(   bump_src->compressed())if(   bump_src->copyTry(   bump_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1))   bump_src=&   bump_temp;else goto error;
      Image  normal_temp; C Image * normal_src=& normal; if( normal_src->compressed())if( normal_src->copyTry( normal_temp, -1, -1, -1, IMAGE_R8G8         , IMAGE_SOFT, 1)) normal_src=& normal_temp;else goto error;
    //Image  smooth_temp; C Image * smooth_src=& smooth; if( smooth_src->compressed())if( smooth_src->copyTry( smooth_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1)) smooth_src=& smooth_temp;else goto error;
    //Image reflect_temp; C Image *reflect_src=&reflect; if(reflect_src->compressed())if(reflect_src->copyTry(reflect_temp, -1, -1, -1, IMAGE_L8           , IMAGE_SOFT, 1))reflect_src=&reflect_temp;else goto error;
    //Image    glow_temp; C Image *   glow_src=&   glow; if(   glow_src->compressed())if(   glow_src->copyTry(   glow_temp, -1, -1, -1, IMAGE_L8A8         , IMAGE_SOFT, 1))   glow_src=&   glow_temp;else goto error;

      /*// set alpha
      if(!alpha_src->is() && ImageTI[col_src->type()].a) // if there's no alpha map but there is alpha in color map
      {
         Byte min_alpha=255;
         alpha_src=&alpha_temp.create(col_src->w(), col_src->h(), 1, IMAGE_A8, IMAGE_SOFT, 1);
         if(col_src->lockRead())
         {
            REPD(y, col_src->h())
            REPD(x, col_src->w())
            {
               Byte a=col_src->color(x, y).a;
                    alpha_temp.pixel(x, y, a);
               MIN(min_alpha, a);
            }
            col_src->unlock();
         }
         if(min_alpha>=254)alpha_temp.del(); // alpha channel in color map is fully white
      }else
      if(alpha_src->is() && ImageTI[alpha_src->type()].channels>1 && ImageTI[alpha_src->type()].a) // if alpha has both RGB and Alpha channels, then check which one to use
         if(alpha_src->lockRead())
      {
         Byte min_alpha=255, min_lum=255;
         REPD(y, alpha_src->h())
         REPD(x, alpha_src->w())
         {
            Color c=alpha_src->color(x, y);
            MIN(min_alpha, c.a    );
            MIN(min_lum  , c.lum());
         }
         alpha_src->unlock();
         if(min_alpha>=254 && min_lum<254)if(alpha_src->copyTry(alpha_temp, -1, -1, -1, IMAGE_L8, IMAGE_SOFT, 1))alpha_src=&alpha_temp;else goto error; // alpha channel is fully white -> use luminance as alpha
      }*/

      // set what textures do we have (set this before 'normal' is generated from 'bump')
      if(    col_src->is())ret|=BT_COLOR  ;
    //if(  alpha_src->is())ret|=BT_ALPHA  ;
      if(   bump_src->is())ret|=BT_BUMP   ;
      if( normal_src->is())ret|=BT_NORMAL ;
    //if( smooth_src->is())ret|=BT_SMOOTH ;
    //if(reflect_src->is())ret|=BT_REFLECT;
    //if(   glow_src->is())ret|=BT_GLOW   ;

      // base_1 NRM !! do this first before base_0 RGB_Bump which resizes bump !!
    C Image *bump_to_normal=null;
      if(  bump_src->is() && !normal_src->is()           )bump_to_normal=  bump_src;else // if bump available and normal not, then create normal from bump
      if(normal_src->is() &&  normal_src->monochromatic())bump_to_normal=normal_src;     // if normal is provided as monochromatic, then treat it as bump and convert to normal
      if(bump_to_normal) // create normal from bump
      {
         // it's best to resize bump instead of normal
         Int w=bump_to_normal->w(),
             h=bump_to_normal->h(); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
         if(bump_to_normal->w()!=w || bump_to_normal->h()!=h)if(bump_to_normal->copyTry(normal_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))bump_to_normal=&normal_temp;else goto error; // !! convert to 'normal_temp' instead of 'bump_temp' because we still need original bump later !!
         bump_to_normal->bumpToNormal(normal_temp, AvgF(w, h)*BUMP_NORMAL_SCALE); normal_src=&normal_temp;
         flip_normal_y=false; // no need to flip since normal map generated from bump is always correct
      }
      if(normal_src->is())
      {
         Int w=normal_src->w(),
             h=normal_src->h(); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
         if(normal_src->w()!=w || normal_src->h()!=h)if(normal_src->copyTry(normal_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP))normal_src=&normal_temp;else goto error;
         if(normal_src->lockRead())
         {
            dest_1.createSoftTry(w, h, 1, IMAGE_R8G8_SIGN, 1);
            Vec4 c; c.zw=0;
            REPD(y, dest_1.h())
            REPD(x, dest_1.w())
            {
               c.xy=normal_src->colorF(x, y).xy*2-1;
               if(flip_normal_y)CHS(c.y);
               dest_1.colorF(x, y, c);
            }
            normal_src->unlock();
         }
      }

      // base_0, put bump in W channel
      {
         Int w=Max(col_src->w(), bump_src->w()),
             h=Max(col_src->h(), bump_src->h()); if(resize_to_pow2){w=NearestPow2(w); h=NearestPow2(h);}
         if( col_src->is() && ( col_src->w()!=w ||  col_src->h()!=h))if( col_src->copyTry( col_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP|IC_ALPHA_WEIGHT)) col_src=& col_temp;else goto error;
         if(bump_src->is() && (bump_src->w()!=w || bump_src->h()!=h))if(bump_src->copyTry(bump_temp, w, h, -1, -1, IMAGE_SOFT, 1, filter, IC_WRAP                ))bump_src=&bump_temp;else goto error;
         dest_0.createSoftTry(w, h, 1, IMAGE_R8G8B8A8_SRGB);
         if(!col_src->is() || col_src->lockRead())
         {
            if(!bump_src->is() || bump_src->lockRead())
            {
               REPD(y, dest_0.h())
               REPD(x, dest_0.w())
               {
                  Color c=(col_src->is() ? col_src->color(x, y) : WHITE);
                  if(bump_src->is())
                  {
                     Color bump=bump_src->color(x, y); c.a=bump.lum();
                  }else                                c.a=255; // use 255 so BC1 can be selected or BC7 with best quality
                  dest_0.color(x, y, c);
               }
               bump_src->unlock();
            }
            col_src->unlock();
         }
      }
   }

error:
   Swap(dest_0, base_0);
   Swap(dest_1, base_1);
   return ret;
}
/******************************************************************************/
}
/******************************************************************************/
