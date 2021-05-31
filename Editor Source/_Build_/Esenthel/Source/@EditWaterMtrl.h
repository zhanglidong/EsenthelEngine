/******************************************************************************/
/******************************************************************************/
class EditWaterMtrl : EditMaterial
{
   flt density, density_add,
       scale_color, scale_normal, scale_bump,
       refract, refract_reflection, refract_underwater,
       wave_scale;
   Vec color_underwater0,
       color_underwater1;

   TimeStamp density_time,
             scale_color_time, scale_normal_time, scale_bump_time,
             refract_time, refract_reflection_time, refract_underwater_time,
             wave_scale_time,
             color_underwater_time;

   EditWaterMtrl();

   // get
   bool usesTexBump()C;

   bool equal(C EditWaterMtrl &src)C;
   bool newer(C EditWaterMtrl &src)C;

   // operations
   void reset();  
   void newData();
   void create(C WaterMtrl &src, C TimeStamp &time=TimeStamp().getUTC());
   void copyTo(WaterMtrl &dest, C Project &proj)C;
   uint sync(C EditMaterial  &src);            
   uint sync(C EditWaterMtrl &src);
   uint undo(C EditWaterMtrl &src);
   void adjustParams(uint old_base_tex, uint new_base_tex, bool old_light_map);

   // io
   bool save(File &f)C;
   bool load(File &f);
   bool load(C Str &name);
};
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
