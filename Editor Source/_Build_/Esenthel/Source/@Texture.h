/******************************************************************************/
/******************************************************************************/
class Texture
{
   UID id; // texture id
   bool  uses_alpha, // if uses alpha channel
            dynamic, // if texture is dynamically generated
               srgb , // if sRGB gamma
             normal, // if normal map
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
