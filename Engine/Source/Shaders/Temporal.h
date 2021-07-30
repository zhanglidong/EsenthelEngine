/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(Temporal)
   Vec2  TemporalOffset,
         TemporalOffsetPrev;
   VecI2 TemporalCurPixel;
   UInt  TemporalOffsetGatherIndex; // this is index to be used for TexGather(uv+TemporalOffset) that allows to get the same values as if accessing TexPoint(uv)
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
