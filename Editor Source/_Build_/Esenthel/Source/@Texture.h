/******************************************************************************/
/******************************************************************************/
class Texture
{
   UID id, // texture id
       src_tex_id; // if this is a dynamically generated texture, then 'src_tex_id' points to the original texture from which it was created
   bool  uses_alpha, // if uses alpha channel
               srgb , // if sRGB gamma
     non_perceptual, // if use non-perceptual compression
         regenerate; // if this texture needs to be regenerated
   sbyte    quality; // -1=PVRTC1_2, 0=default, 1=BC7
   byte    downsize; // downsize

   Texture& downSize(int size);

   static int CompareTex(C Texture &tex, C UID &tex_id);

public:
   Texture();
};
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
