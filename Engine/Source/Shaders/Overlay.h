/******************************************************************************/
#include "!Set Prec Struct.h"
struct OverlayClass
{
   VecH4  param;
   Matrix mtrx ;
};

BUFFER(Overlay)
   OverlayClass OverlayParams;
BUFFER_END
#include "!Set Prec Default.h"

inline Half OverlayOpaqueFrac() {return OverlayParams.param.x;}
inline Half OverlayAlpha     () {return OverlayParams.param.y;}
/******************************************************************************/
