/******************************************************************************/
#include "!Set HP.h"
BUFFER(WaterSurface)
   Half WaterRgh_2,
        WaterFresnelRough,
        WaterFresnelPow,
        WaterRflFake,
        WaterSpc;
   Flt  WaterScaleDif,
        WaterScaleNrm,
        WaterScaleBump;
   VecH WaterCol,
        WaterFresnelColor;
BUFFER_END

BUFFER(Water)
   Half WaterRfl;
   Flt  WaterRfr,
        WaterRfrRfl,
        WaterUnder,
        WaterUnderRfr,
        WaterFlow;
   Vec2 WaterOfs;
   Vec2 WaterDns;
   Vec  WaterUnderCol0,
        WaterUnderCol1;
   Vec4 WaterRflMulAdd,
        WaterClamp;

   Half WaterWave;
   Vec2 WaterYMulAdd;
   Vec  WaterPlnPos,
        WaterPlnNrm;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
