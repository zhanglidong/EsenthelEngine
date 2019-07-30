/******************************************************************************/
#include "!Set SP.h"
struct OverlayClass
{
   VecH4  param;
   Matrix mtrx ; // !! define Matrix last to avoid potential alignment issues on Arm Mali !!
};

BUFFER(Overlay)
   OverlayClass OverlayParams;
BUFFER_END
#include "!Set LP.h"

inline Half OverlayOpaqueFrac() {return OverlayParams.param.x;}
inline Half OverlayAlpha     () {return OverlayParams.param.y;}
/******************************************************************************/
