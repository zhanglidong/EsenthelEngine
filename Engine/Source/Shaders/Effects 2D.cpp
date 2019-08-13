/******************************************************************************/
#include "!Header.h"
/******************************************************************************

   Keep here effects that are rarely used.

/******************************************************************************/
#include "!Set SP.h"
BUFFER(ColTrans)
   Vec     ColTransHsb;
   MatrixH ColTransMatrix;
BUFFER_END
#include "!Set LP.h"

VecH4 ColTrans_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return VecH4(Transform(Tex(Img, inTex).rgb, ColTransMatrix), Step);
}
VecH4 ColTransHB_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH color     =Tex(Img, inTex).rgb;
   Half brightness=Max(color); color=Transform(color, ColTransMatrix);
   Half max       =Max(color);
   if(  max)color*=Half(ColTransHsb.z)*brightness/max;
   return VecH4(color, Step);
}
VecH4 ColTransHSB_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Vec hsb=RgbToHsb(Tex(Img, inTex).rgb);
   return VecH4(HsbToRgb(Vec(hsb.x+ColTransHsb.x, hsb.y*ColTransHsb.y, hsb.z*ColTransHsb.z)), Step);
}
/******************************************************************************/
struct RippleClass
{
   Flt  xx, xy,
        yx, yy,
        stp,
        power,
        alpha_scale,
        alpha_add;
   Vec2 center;
};

BUFFER(Ripple)
   RippleClass Rppl;
BUFFER_END

VecH4 Ripple_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   inTex+=Vec2(Sin(inTex.x*Rppl.xx + inTex.y*Rppl.xy + Rppl.stp),
               Sin(inTex.x*Rppl.yx + inTex.y*Rppl.yy + Rppl.stp))*Rppl.power;

   VecH4  c=Tex(Img, inTex); c.a*=Sat((Rppl.alpha_scale*2)*Length(inTex-Rppl.center)+Rppl.alpha_add);
   return c;
}
/******************************************************************************
struct TitlesClass
{
   Flt stp;
   Flt center;
   Flt range;
   Flt smooth;
   Flt swirl;
};

BUFFER(Titles)
   TitlesClass Ttls;
BUFFER_END

VecH4 Titles_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   Flt s=Sat((Abs(inTex.y-Ttls.center)-Ttls.range)/Ttls.smooth);
   inTex.x+=Sin(s*s*s*s*(PI*6)+Ttls.stp)*s*s*Ttls.swirl;
   VecH4 color=0; UNROLL for(Int i=-4; i<=4; i++)color+=Tex(Img, inTex+s*ImgSize.xy*Vec2(i, 0));
   color/=9;
   color.a*=Pow(1-s, 0.6);
   return color;
}
TECHNIQUE(Titles, Draw_VS(), Titles_PS());
/******************************************************************************/
VecH4 Fade_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   VecH4  c=Tex(Img, inTex); c.a*=Sat(Step*3+(inTex.x+inTex.y)*0.5-1-Tex(Img1, inTex).a);
   return c;
}
/******************************************************************************/
void Wave_VS(VtxInput vtx,
 NOPERSP out Vec2 outTex:TEXCOORD,
 NOPERSP out Vec4 outVtx:POSITION)
{
   outTex.x=vtx.tex().x*Color[0].x + vtx.tex().y*Color[0].y + Color[0].z;
   outTex.y=vtx.tex().x*Color[1].x + vtx.tex().y*Color[1].y + Color[1].z;
   outVtx  =vtx.pos4();
}
VecH4 Wave_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
   return Tex(Img, inTex);
}
/******************************************************************************/
VecH4 RadialBlur_PS(NOPERSP Vec2 inTex:TEXCOORD):TARGET
{
         Vec2 tex  =inTex/Color[0].z + (Color[0].xy-Color[0].xy/Color[0].z); // (inTex-RadialBlurPos)/step+RadialBlurPos
         Vec  color=0; // use HP
   const Int  steps=32;
   for(Int i=0; i<steps; i++)color+=Tex(Img, Lerp(inTex, tex, i/Flt(steps-1))).rgb;
   return VecH4(color/steps, Color[0].w);
}
/******************************************************************************/
