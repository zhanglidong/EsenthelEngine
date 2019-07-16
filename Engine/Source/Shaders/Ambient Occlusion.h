/******************************************************************************/
#include "!Set HP.h"
BUFFER(AO)
   Flt  AmbRange   =0.4,
        AmbBias    =0.1*0.4; // BiasFrac*AmbRange
   Half AmbContrast=1.0;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
