/******************************************************************************/
#include "!Set SP.h"
struct WaterMaterialClass
{
   VecH color;
   Half smooth,
        reflect,
        normal;
   Flt  wave_scale,
        scale_color,
        scale_normal,
        scale_bump,

        density,
        density_add,

        refract,
        refract_reflection;
};
BUFFER(WaterMaterial)
   WaterMaterialClass WaterMaterial;
BUFFER_END

BUFFER(Water)
   VecH Water_color_underwater0,
        Water_color_underwater1;
   Flt  Water_refract_underwater;

   Half WaterUnderStep;
   Vec2 WaterOfs;
   Vec2 WaterYMulAdd;
   Vec  WaterPlanePos,
        WaterPlaneNrm;
   Flt  WaterFlow;
   Vec4 WaterReflectMulAdd,
        WaterClamp;
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
