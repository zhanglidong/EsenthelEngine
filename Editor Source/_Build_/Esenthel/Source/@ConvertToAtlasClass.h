/******************************************************************************/
/******************************************************************************/
class ConvertToAtlasClass : PropWin
{
   class Mtrl
   {
      Button       del;
      UID          id;
      TextLine     name;
      Text       t_original_size, t_scaled_size;
      VecI2        original_size, scaled_size, pos;
      RectI        packed_rect; // not inclusive
      bool         rotated;
      flt          scale;
      Property     prop;
      ImagePtr     base_0;
      EditMaterial edit;

      void setScaleText(C VecI2 &size);
      void setScale    (             );
      void setScale    (flt scale    );
      void posY        (flt y        );
      /*void pack(MtrlImages &atlas)
      { // #MaterialTextureLayout
         ThreadMayUseGPUData();
         VecI2 size=packed_rect.size(); if(rotated)size.swap();
         MtrlImages src; src.fromMaterial(edit, Proj); src.processAlpha(); src.resize(size); src.apply();
         Color normal(128, 128, 255);
         REPD(y, size.y)
         REPD(x, size.x)
         {
            int dest_x=x, dest_y=y; if(rotated)Swap(dest_x, dest_y);
            dest_x+=packed_rect.min.x;
            dest_y+=packed_rect.min.y;

            flt   glow; if(src.glow  .is()){Color c=src.glow.color(x, y); glow=c.lum()*c.a/255.0;}else glow=255;
            Color n   ; if(src.normal.is())
            {
               n=src.normal.color(x, y);
               if(src.flip_normal_y)n.g=255-n.g;
               if(rotated)Swap(n.r, n.g); // in rotated mode, normal XY channels need to be swapped
               if(!Equal(edit.normal, 1))
               {
                  n.r=Mid(Round((n.r-128)*edit.normal+128), 0, 255);
                  n.g=Mid(Round((n.g-128)*edit.normal+128), 0, 255);
               }
            }else n=normal;

            Color c; if(src.color.is())
            {
               Vec4 cf=src.color.colorF(x, y);
               cf.xyz*=edit.color_s.xyz;
               c=cf;
            }else c=WHITE;

            atlas.color  .color(dest_x, dest_y, c);
            atlas.alpha  .pixB (dest_x, dest_y)=          (src.alpha  .is() ? Mid(Round(src.alpha  .pixelF(x, y)*255                      ), 0, 255) : 255);
            atlas.bump   .pixB (dest_x, dest_y)=          (src.bump   .is() ? Mid(Round(src.bump   .pixelF(x, y)*255                      ), 0, 255) : 255);
            atlas.smooth .pixB (dest_x, dest_y)=Mid(Round((src.smooth .is() ?           src.smooth .color (x, y).lum() : 255)*edit.smooth ), 0, 255); // bake
            atlas.reflect.pixB (dest_x, dest_y)=Mid(Round((src.reflect.is() ?           src.reflect.color (x, y).lum() : 255)*edit.reflect), 0, 255); // bake
            atlas.glow   .pixB (dest_x, dest_y)=Mid(Round(                                  glow                             *edit.glow   ), 0, 255); // bake
            atlas.normal .color(dest_x, dest_y, n);
         }
         tex=src.color.is()*BT_COLOR | src.alpha.is()*BT_ALPHA | src.bump.is()*BT_BUMP | src.normal.is()*BT_NORMAL | src.smooth.is()*BT_SMOOTH | src.reflect.is()*BT_REFLECT | src.glow.is()*BT_GLOW;
         AtomicOr(atlas.tex, tex);
      }*/

public:
   Mtrl();
   };
   class Error
   {
      UID mesh_id, mtrl_id;
      flt error;

public:
   Error();
   };
   class Preview : GuiCustom
   {
      virtual void draw(C GuiPC &gpc)override;
   };
   class Warning : ClosableWindow
   {
      Text        text;
      Region      region;
      List<Error> list;
      Button      proceed;
      
      static Str  MeshName(C Error&error);
      static Str  MtrlName(C Error&error);
      static void Proceed (ptr);        

      void create();
      void display();
   };

   enum MODE
   {
      NEW,
      REPLACE_KEEP,
      REPLACE_NO_PUBLISH,
      REPLACE_REMOVE,
   };
   static cchar8 *mode_t[]
;
   static const flt h;

   Memx<Mtrl>  mtrls;
   Memc<Error> errors;
   Region      region;
   Preview     preview;
   Warning     warning;
   bool        force_square, auto_stretch;
   flt         scale;
   MODE        mode;
   VecI2       tex_size;
   Text        columns, t_tex_size;
   Button      convert;

   static Str  TexSize(C VecI2 &size);           
   static void Scale(Mtrl &mtrl, C Str &text);   
   static void Del(Mtrl &mtrl);                  
   static void Refresh(C Property &prop);        
   static void ChangedScale(C Property &prop);   
   static int  CompareData(C Mtrl &a, C Mtrl &b);
   static void Convert(ConvertToAtlasClass &cta);
 //static void PackMaterial(Mtrl &mtrl, MtrlImages &atlas, int thread_index) {mtrl.pack(atlas);}

   void setErrors(Mesh &mesh, C Memc<UID> &mtrl_ids, C UID &mesh_id);
   static bool Create(int &occurence, C UID &id, ptr user);        
   void convertMeshes(C UID &atlas_id);
   static bool AddMap(bool &forced, Str &dest, C Str &src, bool force, C Mtrl &mtrl, bool normal=false, C Vec &mul=1, bool flip_normal_y=false);
   void checkSide(Str &dest, bool filled);
   void convertPerform();
   void convertDo();
   void clearProj();
   void create();
   void autoStretch();
   void refresh();
   void addElms(C MemPtr<UID> &elm_ids);
   void setElms(C MemPtr<UID> &elm_ids);
   void drag(Memc<UID> &elms, GuiObj *focus_obj, C Vec2 &screen_pos);
   virtual ConvertToAtlasClass& hide()override;
   /*virtual void update(C GuiPC &gpc)override
   {
      super.update(gpc);
      if(visible() && gpc.visible)
      {
      }
   }*/

public:
   ConvertToAtlasClass();
};
/******************************************************************************/
/******************************************************************************/
extern ConvertToAtlasClass ConvertToAtlas;
/******************************************************************************/
