/******************************************************************************/
/******************************************************************************/
class MtrlImages
{
   class ImageResize : Image
   {
      VecI2       size; // if >0 then image should be resized
      FILTER_TYPE filter;
      bool        clamp ;

      ImageResize& clearParams();
      ImageResize& del();
      bool createTry(C VecI2 &size, IMAGE_TYPE type);
      ImageResize& resize(C VecI2 &size);
      ImageResize& setFrom(C TextParam &param);
      operator ImageSource()C;               

public:
   ImageResize();
   };
   bool        flip_normal_y;
   int         tex;
   ImageResize color, alpha, bump, normal, smooth, reflect, glow;
   
   MtrlImages& del();
   bool create(C VecI2 &size);
   void clear();
   void compact();
   void Export(C Str &name, C Str &ext)C;
   /*static void Crop(ImageResize &image, C Rect &frac)
   {
      if(image.is())
      {
         RectI rect=Round(frac*(Vec2)image.size());
         Image temp; if(temp.createSoftTry(rect.w(), rect.h(), 1, ImageTypeUncompressed(image.type()))) // crop manually because we need to use Mod
         {
            if(image.lockRead())
            {
               REPD(y, temp.h())
               REPD(x, temp.w())temp.color(x, y, image.color(Mod(x+rect.min.x, image.w()), Mod(y+rect.min.y, image.h())));
               image.unlock();
               Swap(temp, SCAST(Image, image));
            }
         }
      }
   }
   void crop(C Rect &frac)
   {
      Crop(color  , frac);
      Crop(alpha  , frac);
      Crop(bump   , frac);
      Crop(normal , frac);
      Crop(smooth , frac);
      Crop(reflect, frac);
      Crop(glow   , frac);
   }*/
   void resize(C VecI2 &size);
   void fromMaterial(C EditMaterial &material, C Project &proj, bool changed_flip_normal_y=false);
   void fromMaterial(C EditWaterMtrl &material, C Project &proj, bool changed_flip_normal_y=false);
   uint createBaseTextures(Image &base_0, Image &base_1, Image &base_2)C;
   uint createWaterBaseTextures(Image &base_0, Image &base_1, Image &base_2)C;
   void baseTextureSizes(VecI2 *size0, VecI2 *size1, VecI2 *size2);
   void waterBaseTextureSizes(VecI2 *size0, VecI2 *size1, VecI2 *size2);
   void processAlpha();

public:
   MtrlImages();
};
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
