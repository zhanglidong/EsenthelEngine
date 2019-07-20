/******************************************************************************/
#include "!Set HP.h"
struct CloudLayer
{
   VecH4 color;
   Vec2  scale, position;
};

BUFFER(CloudLayer)
   CloudLayer CL[4];

   Vec2 LCRange;
   Flt  LCScaleY;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
inline Flt CloudAlpha(Flt y) {return y*8-0.15;}
/******************************************************************************/
