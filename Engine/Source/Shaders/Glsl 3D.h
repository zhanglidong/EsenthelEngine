#include "Glsl Matrix.h"
PAR HP Matrix(CamMatrix);
MP Flt VisibleOpacity(MP Flt density, MP Flt range) {return Pow(1.0-density, range);}
