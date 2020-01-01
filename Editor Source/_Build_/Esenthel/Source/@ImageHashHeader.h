/******************************************************************************/
class ImageHashHeader // !! try to don't make any changes to this class layout, because doing so will require a new hash for every texture !!
{
   VecI       size;
   byte       flags;
   IMAGE_TYPE type;

   ImageHashHeader(C Image &image, IMAGE_TYPE type);
};
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
