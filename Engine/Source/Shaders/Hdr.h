/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(Hdr)
   Flt  HdrBrightness,
        HdrExp       ,
        HdrMaxDark   ,
        HdrMaxBright ;
   VecH HdrWeight    ;
BUFFER_END

BUFFER(ToneMap)
   Half ToneMapMonitorMaxLum,
        ToneMapTopRange,
        ToneMapDarkenRange,
        ToneMapDarkenExp;
BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
