/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(Bloom)
   VecH4 BloomParams; // x=original, y=scale, z=cut, w=glow/(res*res)
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
VecH BloomColor(VecH color, Bool precomputed=false)
{
   if(precomputed)return color;
   Half col_lum=Max(color), lum=col_lum*BloomParams.y+BloomParams.z;
   return (lum>0) ? color*(Sqr(lum)/col_lum) : VecH(0, 0, 0);
}
/******************************************************************************/
