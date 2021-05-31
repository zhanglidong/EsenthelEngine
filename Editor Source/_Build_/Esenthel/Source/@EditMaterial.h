/******************************************************************************/
/******************************************************************************/
class EditMaterial
{
   MATERIAL_TECHNIQUE        tech;
   Edit::Material::TEX_QUALITY tex_quality;
   bool                      flip_normal_y, smooth_is_rough, cull;
   byte                      downsize_tex_mobile;
   Vec4                      color_s;
   Vec                       emissive;
   flt                       smooth, // 0..1 without 'smooth_map', and -1..1 with 'smooth_map'
                             reflect_min, reflect_max, glow, normal, bump,
                             uv_scale, det_scale, det_power;
   UID                       base_0_tex, base_1_tex, base_2_tex, detail_tex, macro_tex, light_tex;
   Str                       color_map, alpha_map, bump_map, normal_map, smooth_map, metal_map, glow_map,
                             detail_color, detail_bump, detail_normal, detail_smooth,
                             macro_map,
                             light_map;
   TimeStamp                 flip_normal_y_time, smooth_is_rough_time, tex_quality_time,
                             color_map_time, alpha_map_time, bump_map_time, normal_map_time, smooth_map_time, metal_map_time, glow_map_time,
                             detail_map_time, macro_map_time, light_map_time,
                             cull_time, tech_time, downsize_tex_mobile_time,
                             color_time, emissive_time, smooth_time, reflect_time, normal_time, bump_time, glow_time, uv_scale_time, detail_time;

   // get
   flt roughMul()C;
   flt roughAdd()C;
   bool hasBumpMap     ()C;
   bool hasNormalMap   ()C;
   bool hasDetailMap   ()C;
   bool hasLightMap    ()C;
   bool hasBase1Tex    ()C; // #MaterialTextureLayout
   bool hasBase2Tex    ()C; // #MaterialTextureLayout
   uint baseTex        ()C;
   uint baseTexUsed    ()C;
   bool usesTexColAlpha()C; // alpha may come from color
   bool usesTexAlpha   ()C; // check only alpha
   bool usesTexBump    ()C; // always keep bump map because it can be used for multi-material per-pixel blending
   bool usesTexNormal  ()C;
   bool usesTexSmooth  ()C;
   bool usesTexMetal   ()C;
   bool usesTexGlow    ()C;
   bool usesTexDetail  ()C;
   bool needTanBin     ()C;

   bool equal(C EditMaterial &src)C;
   bool newer(C EditMaterial &src)C;

   // operations
   void reset();                   
   void resetAlpha();                                            
   void separateNormalMap(C TimeStamp&time=TimeStamp().getUTC());
   void cleanupMaps();
   void expandMap(Str &map, C MemPtr<FileParams> &color, C MemPtr<FileParams> &smooth, C MemPtr<FileParams> &bump);
   void expandMaps();

   void newData();
   void create(C Material &src, C TimeStamp &time=TimeStamp().getUTC());
   void copyTo(Material &dest, C Project &proj)C;
   void copyTo(Edit::Material &dest)C;
   enum
   {
      CHANGED_PARAM=1<<0,
      CHANGED_BASE =1<<1,
      CHANGED_DET  =1<<2,
      CHANGED_MACRO=1<<3,
      CHANGED_LIGHT=1<<4,
      CHANGED_FNY  =1<<5,
      CHANGED_SIR  =1<<6,
   };
   uint sync(C Edit::Material &src);
   uint sync(C EditMaterial &src);
   uint undo(C EditMaterial &src);
   void adjustParams(uint old_base_tex, uint new_base_tex, bool old_light_map);

   static void FixOldFileParams(Str &name);
   static void ChangeMulToSet(Mems<FileParams> &fps);
   static void ChangeMulToSet(Str &name);
   void fixOldFileParams();
   void fixOldReflect(flt reflect);
   void fixOldSmooth();

   // io
   bool save(File &f)C;
   bool load(File &f);
   bool load(C Str &name);

public:
   EditMaterial();
};
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
