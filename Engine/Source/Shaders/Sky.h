/******************************************************************************/
#include "!Set SP.h"
BUFFER(Sky)
   Flt      SkyDnsExp      ,
            SkyHorExp      ;
   Half     SkyBoxBlend    ;
   VecH4    SkyHorCol      ,
            SkySkyCol      ;
   Vec2     SkyFracMulAdd  ,
            SkyDnsMulAdd   ;
   VecH2    SkySunHighlight;
   Vec      SkySunPos      ;
   MatrixH3 SkyStarOrn     ; // !! define Matrix last to avoid potential alignment issues on Arm Mali !!
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
