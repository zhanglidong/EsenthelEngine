/******************************************************************************/
struct CloudLayer
{
   VecH4 color;
   Vec2  scale, position;
};

BUFFER(CloudLayer)
   Flt  LCScaleY;
   Vec2 LCRange;

   CloudLayer CL[4];
BUFFER_END
/******************************************************************************/
inline Flt CloudAlpha(Flt y) {return y*8-0.15;}
/******************************************************************************/
