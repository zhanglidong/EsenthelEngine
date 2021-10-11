/******************************************************************************/
class ImportTerrainClass : ClosableWindow
{
   class GuiImage2 : ImageSkin
   {
      bool     mono=false;
      int      channel=-1;
      Image    image_sw, image_hw;
      WindowIO wio;

      static void Load(C Str &name, GuiImage2 &img) {img.load(name);}
             void load(C Str &name)
      {
         image_sw.ImportTry(name, mono ? IMAGE_F32 : IMAGE_R8G8B8A8, IMAGE_SOFT, 1);
         image_sw.copyTry(image_hw, 128, 128, 1, IMAGE_R8G8B8A8_SRGB, IMAGE_2D, 1, FILTER_LINEAR, IC_CLAMP|IC_IGNORE_GAMMA); // we need sRGB preview, so ignore gamma always
      }

      GuiImage2& create(C Rect &rect, bool mono=false)
      {
         T.mono=mono;
         super.create(rect, &image_hw).desc("Drag and drop an image here,\nor click to select one.");
         wio.create(S, S, S, Load, Load, T).ext(SUPPORTED_IMAGE_EXT, "Image");
         return T;
      }
      virtual void update(C GuiPC &gpc)override
      {
         super.update(gpc);
         REPA(MT)if(MT.bp(i) && MT.guiObj(i)==this)wio.activate();
      }
      virtual void draw(C GuiPC &gpc)override
      {
         if(gpc.visible && visible())
         {
            D.clip(gpc.clip);
            Rect rect=T.rect()+gpc.offset;
            if(image_hw.is())
            {
               if(channel>=0)switch(channel)
               {
                  case 0: VI.shader(ShaderFiles("Main")->get("DrawTexX" )); break; // 'image_hw' is sRGB
                  case 1: VI.shader(ShaderFiles("Main")->get("DrawTexY" )); break; // 'image_hw' is sRGB
                  case 2: VI.shader(ShaderFiles("Main")->get("DrawTexZ" )); break; // 'image_hw' is sRGB
                  case 3: VI.shader(ShaderFiles("Main")->get("DrawTexWG")); break;
               }
               ALPHA_MODE alpha=D.alpha(ALPHA_NONE); image_hw.drawFit(rect);
                                D.alpha(alpha     );
            }
            TextStyleParams ts(true); if(image_hw.is())ts.resetColors(false); D.text(ts, rect.center(), "Image");
            if(rect_color.a)rect.draw(rect_color, false);
         }
      }
   }
   class MaterialChannel : GuiImage
   {
      Button      remove;
      MaterialPtr mtrl;
      UID         mtrl_id=UIDZero;

      static void Clear(MaterialChannel &mc) {mc.clear();}
             void clear()
      {
         image=null;
         mtrl =null;
         mtrl_id.zero();
      }
      void set(Elm &elm)
      {
         if(elm.type==ELM_MTRL)
         {
            mtrl_id=elm.id;
            mtrl   =Proj.gamePath(elm.id);
            image  =MaterialImage(mtrl);
            color  =MaterialColor(mtrl);
         }
      }
      MaterialChannel& create(C Rect &rect, Color rect_color, C Str &desc)
      {
         super.create(rect).desc(desc);
         T.rect_color=rect_color;
         remove.create(Rect_RU(rect.ru(), 0.035, 0.035)).func(Clear, T); remove.image="Gui/close.img";
         alpha_mode=ALPHA_NONE;
         return T;
      }
      virtual void update(C GuiPC &gpc)override
      {
         super.update(gpc);
         remove.visible(mtrl_id.valid() && (Gui.ms()==this || Gui.ms()==&remove));
      }
   }

   Tabs  pos_mode, height, mtrl, color, mtrl_channel, height_mode, mtrl_mode, color_mode;
   bool  color_img_alpha=false;
   flt   mtrl_blend=0.5, color_blend=0.5;
   VecI2 area=0, radius=5, size=5, target=5;
   Vec2  height_range(-16, 16);
   GuiImage2 height_image, mtrl_image, color_image;
   Memx<Property> area_p, radius_p, size_p, target_p, height_prop, mtrl_prop, color_prop;
   Button import_b;
   TextBlack ts, ts_l, ts_r;
   Text            mtrl_channels_t, select_t, world_area_coords_t;
   MaterialChannel mtrl_channels[4];

   static void HeightModeChanged(ImportTerrainClass &it) {if(it.height_prop.elms())it.height_prop[0].visible(it.height_mode()==0);}
   static void    ChannelChanged(ImportTerrainClass &it) {it.mtrl_image.channel=it.mtrl_channel()-1;}
   static void            Import(ImportTerrainClass &it) {it.import();}
          void            import()
   {
      if(height()<0 && mtrl()<0 && color()<0){Gui.msgBox(S, "No elements selected to import"); return;}
      if(!areaRectValid()){Gui.msgBox(S, "Import Area Size is empty"); return;}
      if(!WorldEdit.elm_id.valid()){Gui.msgBox(S, "No world opened"); return;}
      // check if any material is selected
      if(mtrl()>=0)
      {
         REPA(mtrl_channels)if(mtrl_channels[i].mtrl_id.valid())goto have_mtrl;
         Gui.msgBox(S, "Please drag and drop at least one material\nto the \"Materials for image channels\" slots"); return;
      }else
      if(!Proj.hm_mtrl_id.valid())
      {
         WorldEdit.highlightHmMtrl();
         Gui.msgBox(S, "Please drag and drop a material to the terrain material slot"); return;
      }
   have_mtrl:
      ImportTerrainTasks.New().create(T);
      hide();
   }

   // get
   Vec2 heightRange() {return height_mode() ? Vec2(0, height_range.y) : height_range;}
   bool  areaRectValid()C
   {
      if(pos_mode()==1)if(!size.all())return false;
      return true;
   }
   RectI areaRect()C
   {
      switch(pos_mode())
      {
         default: return RectI(area).extend(radius);
         case  1: {RectI r(area); r.max.x+=size.x; r.max.y+=size.y; if(size.x>0)r.max.x--;else if(size.x<0)r.min.x--; if(size.y>0)r.max.y--;else if(size.y<0)r.min.y--; return r;}
         case  2: return RectI(area, target);
      }
   }
   // manage
   void create()
   {
      ts.reset().size=0.04; ts_r=ts_l=ts; ts_r.align.set(1, 0); ts_l.align.set(-1, 0); ts.align.set(0, -1);
      flt x=0, y=-0.04, h=0.05, height_w=0.4, mtrl_w=0.5, color_w=0.4;
      Gui+=super.create(Rect_C(0, 0, height_w+mtrl_w+color_w, 1.06), "Import Heightmap").hide(); button[2].show();
      cchar8 *pos_mode_t[]={"Radius", "Size", "Target"};
      T+=world_area_coords_t.create(Vec2(0.25, y), "World area coordinates:", &ts);
      T+=pos_mode.create(Rect_LU(0.04, y, 0.4, 0.055), 0, pos_mode_t, Elms(pos_mode_t), true).valid(true).set(0);
        area_p.New().create(   "AreaX", MEMBER(ImportTerrainClass,   area.x)).autoData(this).mouseEditSpeed(5);
        area_p.New().create(   "AreaY", MEMBER(ImportTerrainClass,   area.y)).autoData(this).mouseEditSpeed(5);        AddProperties(  area_p,               T, Vec2(0.50, y), h, 0.15, &ts_l);
      radius_p.New().create( "RadiusX", MEMBER(ImportTerrainClass, radius.x)).autoData(this).mouseEditSpeed(5).min(0);
      radius_p.New().create( "RadiusY", MEMBER(ImportTerrainClass, radius.y)).autoData(this).mouseEditSpeed(5).min(0); AddProperties(radius_p, pos_mode.tab(0), Vec2(0.85, y), h, 0.15, &ts_l);
        size_p.New().create(   "SizeX", MEMBER(ImportTerrainClass,   size.x)).autoData(this).mouseEditSpeed(5);
        size_p.New().create(   "SizeY", MEMBER(ImportTerrainClass,   size.y)).autoData(this).mouseEditSpeed(5);        AddProperties(  size_p, pos_mode.tab(1), Vec2(0.85, y), h, 0.15, &ts_l);
      target_p.New().create( "TargetX", MEMBER(ImportTerrainClass, target.x)).autoData(this).mouseEditSpeed(5);
      target_p.New().create( "TargetY", MEMBER(ImportTerrainClass, target.y)).autoData(this).mouseEditSpeed(5);        AddProperties(target_p, pos_mode.tab(2), Vec2(0.85, y), h, 0.15, &ts_l);
      y-=h*2.5;
      T+=select_t.create(Vec2(clientWidth()/2, y), "Select elements to import:", &ts);
      y-=h*1.5;

      cchar8 *height_t[]={"Height"}, *height_mode_t[]={"Set", "Add", "Sub"},
             *  mtrl_t[]={"Material"}, *mtrl_channel_t[]={"RGB", "Red", "Green", "Blue", "Alpha"},
             * color_t[]={"Color"},
             *set_blend_t[]={"Set", "Blend"};
      T+=height.create(Rect_C(x+height_w/2, y, 0.2, 0.055), 0, height_t, Elms(height_t));
      {
         flt xx=x+0.01, yy=y-h;
         height.tab(0)+=height_image.create(Rect_LU(xx, yy, 0.3, 0.3), true);
         height.tab(0)+=height_mode .create(Rect_LU(xx, height_image.rect().min.y-0.01, 0.3, 0.055), 0, height_mode_t, Elms(height_mode_t)).func(HeightModeChanged, T).valid(true).set(0).desc("Mode controlling how the height will be applied");
         height_prop.New().create("MinHeight", MEMBER(ImportTerrainClass, height_range.x)).autoData(this).mouseEditSpeed(10);
         height_prop.New().create("MaxHeight", MEMBER(ImportTerrainClass, height_range.y)).autoData(this).mouseEditSpeed(10);
         AddProperties(height_prop, height.tab(0), height_mode.rect().ld()-Vec2(0, 0.01), h, 0.15, &ts_l);
      }
      x+=height_w;
      T+=mtrl.create(Rect_C(x+mtrl_w/2, y, 0.2, 0.055), 0, mtrl_t, Elms(mtrl_t));
      {
         flt xx=x+0.01, yy=y-h;
         mtrl.tab(0)+=mtrl_image.create(Rect_LU(xx, yy, 0.3, 0.3));
         mtrl.tab(0)+=mtrl_channel.create(Rect_LU(mtrl_image.rect().ru(), 0.16, mtrl_image.rect().h()), 0, mtrl_channel_t, Elms(mtrl_channel_t)).valid(true).set(0).func(ChannelChanged, T).desc("Preview selected channel of the image");
         mtrl.tab(0)+=mtrl_mode.create(Rect_LU(xx, mtrl_image.rect().min.y-0.01, 0.3, 0.055), 0, set_blend_t, Elms(set_blend_t)).valid(true).set(0).desc("Mode controlling how the materials will be applied");
         mtrl_prop.New().create("Blend", MEMBER(ImportTerrainClass, mtrl_blend)).range(0, 1).autoData(this);
         Rect rect=AddProperties(mtrl_prop, mtrl_mode.tab(1), mtrl_mode.rect().ld()-Vec2(0, 0.01), h, 0.15, &ts_l);
         mtrl.tab(0)+=mtrl_channels_t.create(Vec2(x+mtrl_w/2, rect.min.y-0.01), "Materials for image channels:", &ts);
         mtrl.tab(0)+=mtrl_channels[0].create(Rect_LU(rect                   .ld()+Vec2(0, -0.05), 0.108, 0.108), RED  , "This slot sets the material for red channel intensity of the source image.\nDrag and drop a material here.");
         mtrl.tab(0)+=mtrl_channels[1].create(Rect_LU(mtrl_channels[0].rect().ru()+Vec2( 0.01, 0), 0.108, 0.108), GREEN, "This slot sets the material for green channel intensity of the source image.\nDrag and drop a material here.");
         mtrl.tab(0)+=mtrl_channels[2].create(Rect_LU(mtrl_channels[1].rect().ru()+Vec2( 0.01, 0), 0.108, 0.108), BLUE , "This slot sets the material for blue channel intensity of the source image.\nDrag and drop a material here.");
         mtrl.tab(0)+=mtrl_channels[3].create(Rect_LU(mtrl_channels[2].rect().ru()+Vec2( 0.01, 0), 0.108, 0.108), GREY , "This slot sets the material for alpha channel intensity of the source image.\nDrag and drop a material here.");
         REPA(mtrl_channels)mtrl.tab(0)+=mtrl_channels[i].remove;
      }
      x+=mtrl_w;
      T+=color.create(Rect_C(x+color_w/2, y, 0.2, 0.055), 0, color_t, Elms(color_t));
      {
         flt xx=x+0.01, yy=y-h;
         color.tab(0)+=color_image.create(Rect_LU(xx, yy, 0.3, 0.3));
         color.tab(0)+=color_mode.create(Rect_LU(xx, color_image.rect().min.y-0.01, 0.3, 0.055), 0, set_blend_t, Elms(set_blend_t)).valid(true).set(0).desc("Mode controlling how the colors will be applied");
         color_prop.New().create("Blend"      , MEMBER(ImportTerrainClass, color_blend    )).range(0, 1).autoData(this);
         color_prop.New().create("Image Alpha", MEMBER(ImportTerrainClass, color_img_alpha)).autoData(this);
         AddProperties(color_prop, color_mode.tab(1), color_mode.rect().ld()-Vec2(0, 0.01), h, 0.15, &ts_r);
      }
      T+=import_b.create(Rect_D(rect().w()/2, -clientHeight()+0.03, 0.3, 0.055), "Import").func(Import, T);
   }
   // operations
   void clearProj()
   {
      ImportTerrainTasks.del();
      REPAO(mtrl_channels).clear();
      hide();
   }
   void erasing(C UID &elm_id)
   {
      REPA(ImportTerrainTasks)if(ImportTerrainTasks[i].world_id==elm_id)ImportTerrainTasks.removeValid(i, true);
   }
   void drag(Memc<UID> &elms, GuiObj *obj, C Vec2 &screen_pos)
   {
      REPAD(c, mtrl_channels)if(obj==&mtrl_channels[c])
      {
         REPA(elms)if(Elm *elm=Proj.findElm(elms[i], ELM_MTRL))
         {
            mtrl_channels[c].set(*elm);
            break;
         }
         break;
      }
   }
   void drop(Memc<Str> &names, GuiObj *obj, C Vec2 &screen_pos)
   {
      GuiImage2 *img[]={&height_image, &mtrl_image, &color_image};
      REPA(img)if(obj==img[i])
      {
         REPAD(n, names)if(ExtType(GetExt(names[n]))==EXT_IMAGE)
         {
            img[i].load(names[n]);
            break;
         }
         break;
      }
   }
   virtual void update(C GuiPC &gpc)override
   {
      super.update(gpc);
      if(ImportTerrainTasks.elms())
      {
         const uint time=Time.curTimeMs(), delay=16; // 1000ms/60fps
         for(int i=0; i<ImportTerrainTasks.elms(); )
         {
            if(!ImportTerrainTasks[i].step())ImportTerrainTasks.removeValid(i, true);
            if(Time.curTimeMs()-time>=delay)break;
         }
      }
   }
}
ImportTerrainClass ImportTerrain;
/******************************************************************************/
class ImportTerrainTask : Window
{
   bool        height_do=false, mtrl_do=false, color_do=false, color_img_alpha=false;
   int         height_mode=0, mtrl_mode=0, color_mode=0, res=0;
   flt         mtrl_blend=0, color_blend=0, grid_level=0;
   Vec2        height=0;
   VecI2       area_xy=0;
   RectI       area_rect=0;
   UID         world_id=UIDZero;
   WorldVer   *world_ver=null;
   Image       height_image, mtrl_image, color_image;
   MaterialPtr mtrl, mtrl_channels[4];
   Progress    progress;

   static void Stop(ImportTerrainTask &it) {ImportTerrainTasks.removeData(&it, true);}

   void create(ImportTerrainClass &it)
   {
      Gui+=super.create(Rect_C(0, 0, 1, 0.10), "Import Heightmap Task"); button[2].show().func(Stop, T);
      T+=progress.create(Rect_LU(0, 0, clientWidth(), clientHeight()).extend(-0.01));

      area_rect=it.areaRect();
      if(!area_rect.validX())Swap(area_rect.min.x, area_rect.max.x);
      if(!area_rect.validY())Swap(area_rect.min.y, area_rect.max.y);
      area_xy=area_rect.min;

      if(height_do=(it.height()>=0))it.height_image.image_sw.copyTry(height_image);
      if(  mtrl_do=(it.mtrl  ()>=0))it.  mtrl_image.image_sw.copyTry(  mtrl_image);
      if( color_do=(it.color ()>=0))it. color_image.image_sw.copyTry( color_image);

      height_mode=it.height_mode();
        mtrl_mode=it.  mtrl_mode();
       color_mode=it. color_mode();

      height=it.height_range/WorldEdit.areaSize();
      mtrl_blend=it.mtrl_blend;
      color_img_alpha=it.color_img_alpha;
      color_blend=it.color_blend;

      mtrl=WorldEdit.hm_mtrl;
      REPAO(mtrl_channels)=it.mtrl_channels[i].mtrl;

      res       =WorldEdit.hmRes();
      grid_level=WorldEdit.grid_plane_level;
      world_id  =WorldEdit.elm_id;
      world_ver =Proj.worldVerRequire(world_id);
   }


   class MaterialIntensities
   {
      class Mtrl
      {
         byte index;
         flt  intensity;
      }

      static int Compare(C Mtrl &a, C Mtrl &b) {return .Compare(b.intensity, a.intensity);} // we sort from higher to lower order, so swap 'a' 'b' order

      Mtrl mtrl[8]; int mtrls=0;

      void set(int i, byte &index, flt &intensity)
      {
         if(InRange(i, mtrls))
         {
            index    =mtrl[i].index;
            intensity=mtrl[i].intensity;
         }else
         {
            index    =0;
            intensity=0;
         }
      }
      void addMtrl(byte index, flt intensity)
      {
         if(intensity>0)
         {
            REP(mtrls)if(mtrl[i].index==index){mtrl[i].intensity+=intensity; return;} // find existing
            if(InRange(mtrls, mtrl)){Mtrl &m=mtrl[mtrls++]; m.index=index; m.intensity=intensity;} // add new one
         }
      }
      void sort() {Sort(mtrl, mtrls, Compare);}
   }
   void import(Heightmap &hm)
   {
      if(AreaVer *area_ver=world_ver.areas.get(area_xy))
      {
         world_ver.changed=true;
         uint changed=0;
         bool empty=!hm.is();
         if(  empty) // create if doesn't exist yet
         {
            MaterialPtr m=mtrl; if(mtrl_do)FREPA(mtrl_channels)if(mtrl_channels[i]){m=mtrl_channels[i]; break;}
            hm.create(res, grid_level, m, false, null, null, null, null, null, null, null, null);
            area_ver.hm_height_time.getUTC();
            area_ver.hm_mtrl_time  .getUTC();
            changed|=AREA_SYNC_HEIGHT|AREA_SYNC_MTRL;
         }
         if(height_do)
         {
            REPD(y, res)
            REPD(x, res)
            {
               flt fx=  (area_xy.x-area_rect.min.x + x/flt(res-1))/(area_rect.w()+1),
                   fy=1-(area_xy.y-area_rect.min.y + y/flt(res-1))/(area_rect.h()+1),
                   h =height_image.pixelFCubicPlus(fx*height_image.w(), fy*height_image.h(), true);
               switch(height_mode)
               {
                  default: hm.height(x, y, Lerp(height.x, height.y, h)); break; // set
                  case  1: hm.height(x, y, hm.height(x, y)+height.y*h); break; // add
                  case  2: hm.height(x, y, hm.height(x, y)-height.y*h); break; // sub
               }
            }
            area_ver.hm_height_time.getUTC();
            changed|=AREA_SYNC_HEIGHT;
         }
         if(mtrl_do)
         {
            VecB4 mtrl_channel_index; REPAO(mtrl_channel_index.c)=hm.getMaterialIndex0(mtrl_channels[i]);
            REPD(y, res)
            REPD(x, res)
            {
               flt  fx=  (area_xy.x-area_rect.min.x + x/flt(res-1))/(area_rect.w()+1),
                    fy=1-(area_xy.y-area_rect.min.y + y/flt(res-1))/(area_rect.h()+1);
               Vec4 c =mtrl_image.colorFCubicPlus(fx*mtrl_image.w(), fy*mtrl_image.h(), true);
               REPA(mtrl_channel_index)if(!mtrl_channel_index.c[i])c.c[i]=0; // if material wasn't set, then clear its intensity

               if(mtrl_mode==0 || empty) // set
               {
                  if(c.any())hm.setMaterial(x, y, mtrl_channel_index, c); // if any material then set it, if not then skip
               }else // blend
               {
                  c*=mtrl_blend;
                  VecB4 src_m; Vec4 src_i; hm.getMaterial(x, y, src_m, src_i);
                  src_i*=1-c.max(); // decrease src intensities
                  MaterialIntensities mi;
                         mi.addMtrl(src_m.x, src_i.x);
                         mi.addMtrl(src_m.y, src_i.y);
                         mi.addMtrl(src_m.z, src_i.z);
                         mi.addMtrl(src_m.w, src_i.w);
                  REPA(c)mi.addMtrl(mtrl_channel_index.c[i], c.c[i]);
                         mi.sort();
                  if(mi.mtrls)
                  {
                     VecB4 m; Vec4 i;
                     mi.set(0, m.x, i.x);
                     mi.set(1, m.y, i.y);
                     mi.set(2, m.z, i.z);
                     mi.set(3, m.w, i.w);
                     hm.setMaterial(x, y, m, i);
                  }
               }
            }
            hm.cleanMaterials();
            area_ver.hm_mtrl_time.getUTC();
            changed|=AREA_SYNC_MTRL;
         }
         if(color_do)
         {
            REPD(y, res)
            REPD(x, res)
            {
               flt  fx=  (area_xy.x-area_rect.min.x + x/flt(res-1))/(area_rect.w()+1),
                    fy=1-(area_xy.y-area_rect.min.y + y/flt(res-1))/(area_rect.h()+1);
               Vec4 c =color_image.colorFCubicPlus(fx*color_image.w(), fy*color_image.h(), true);
               switch(color_mode)
               {
                  default: hm.color(x, y, Color(c)); break; // set
                  case  1: // blend
                  {
                     if(color_img_alpha)c.w*=color_blend;else c.w=color_blend; // set alpha
                     Vec src=hm.colorF(x, y); // get previous color
                             hm.color (x, y, Color(Lerp(src, c.xyz, c.w))); // set new color
                  }break;
               }
            }
            hm.cleanColor();
            area_ver.hm_color_time.getUTC();
            changed|=AREA_SYNC_COLOR;
         }
         world_ver.rebuildArea(area_xy, changed);
         Synchronizer.setArea(world_id, area_xy);
      }
   }
   void update()
   {
      // opened world
      if(world_id.valid() && WorldEdit.elm_id==world_id)
         if(Cell<Area> *cell=WorldEdit.grid.find(area_xy))
      {
         Area &area=*cell.data();
         if(area.loaded)
         {
            bool loaded=area.loaded;
            area.load();
            if(!area.hm)New(area.hm);
            import(*area.hm);
            area.setChanged();
            if(!loaded)area.unload();
            return;
         }
      }

      // non-opened world
      Heightmap hm;
      Str       name=Proj.editAreaPath(world_id, area_xy);
      Chunks    chunks; chunks.load(name, WorldAreaSync);
      Chunk    *chunk=chunks.findChunk("Heightmap"); if(!chunk)chunk=&chunks.chunks.New();else if(chunk.ver==0)hm.load(File().readMem(chunk.data(), chunk.elms()), Proj.game_path);

      import(hm);
      File temp; hm.save(temp.writeMem(), Proj.game_path); temp.pos(0); chunk.create("Heightmap", 0, temp);

      Proj.createWorldPaths(world_id);
      chunks.save(name, WorldAreaSync);
   }
   bool step()
   {
      if(!area_rect.includesX(area_xy.x))
      {
         area_xy.x=area_rect.min.x;
         if(!area_rect.includesY(++area_xy.y))return false;
      }
      update();
      int areas=(area_rect.w()+1)*(area_rect.h()+1);
      progress.set(progress()+1.0/areas);
      // step
      area_xy.x++; return true;
   }
}
Memx<ImportTerrainTask> ImportTerrainTasks;
/******************************************************************************/
