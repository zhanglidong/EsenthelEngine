/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
ColorMatrix& ColorMatrix::setRGB () {x.set(1, 0, 0); y.set(0, 1, 0); z.set(0, 0, 1); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setRBG () {x.set(1, 0, 0); y.set(0, 0, 1); z.set(0, 1, 0); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setGRB () {x.set(0, 1, 0); y.set(1, 0, 0); z.set(0, 0, 1); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setGBR () {x.set(0, 1, 0); y.set(0, 0, 1); z.set(1, 0, 0); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setBRG () {x.set(0, 0, 1); y.set(1, 0, 0); z.set(0, 1, 0); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setBGR () {x.set(0, 0, 1); y.set(0, 1, 0); z.set(1, 0, 0); pos.zero(); return T;}
ColorMatrix& ColorMatrix::setMono()
{
   Flt f=1.0f/3;
   x=f;
   y=f;
   z=f;
   pos.zero();
   return T;
}
ColorMatrix& ColorMatrix::setHue(Flt h)
{
       h=3*Frac(h);
   Flt a=2-Sat(h)-Sat(3-h),
       b=1-Sat(Abs(h-1)),
       c=1-Sat(Abs(h-2));
   x.set(a, b, c);
   y.set(c, a, b);
   z.set(b, c, a);
   pos.zero();
   return T;
}
void ColorMatrix::draw(Flt alpha)
{
   if(alpha>0)
      if(C ImageRTPtr &back=Renderer.getBackBuffer())
   {
      SPSet("ColTransMatrix", T);
      Sh.Step->set(alpha);
      if(!Sh.ColTrans)Sh.ColTrans=Sh.get("ColTrans"); Sh.ColTrans->draw(back);
   }
}
void ColorTransHB(Flt hue, Flt brightness, Flt alpha)
{
   if(alpha>0)
      if(C ImageRTPtr &back=Renderer.getBackBuffer())
   {
      SPSet("ColTransMatrix", ColorMatrix().setHue(hue));
      SPSet("ColTransHsb"   , Vec(0, 0, brightness));
      Sh.Step->set(alpha);
      if(!Sh.ColTransHB)Sh.ColTransHB=Sh.get("ColTransHB"); Sh.ColTransHB->draw(back);
   }
}
void ColorTransHSB(Flt hue, Flt saturation, Flt brightness, Flt alpha)
{
   if(alpha>0)
      if(C ImageRTPtr &back=Renderer.getBackBuffer())
   {
      SPSet("ColTransHsb", Vec(hue, saturation, brightness));
      Sh.Step->set(alpha);
      if(!Sh.ColTransHSB)Sh.ColTransHSB=Sh.get("ColTransHSB"); Sh.ColTransHSB->draw(back);
   }
}
/******************************************************************************/
void DisplayDraw::fxBegin()
{
   if(!Renderer._back   )Renderer._back   .get  (ImageRTDesc(Renderer._gui->w(), Renderer._gui->h(), IMAGERT_SRGBA)); // Alpha is used for transparency
   if(!Renderer._back_ds)Renderer._back_ds.getDS(Renderer._back->w(), Renderer._back->h(), Renderer._back->samples());
   Renderer._cur_main   =Renderer._back   ;
   Renderer._cur_main_ds=Renderer._back_ds;
   Renderer.set(Renderer._cur_main, Renderer._cur_main_ds, false);
}
ImageRTPtr DisplayDraw::fxEnd()
{
   ImageRTPtr cur; Swap(cur, Renderer._back); Renderer._back_ds.clear();
   Renderer._cur_main   =Renderer._gui   ;
   Renderer._cur_main_ds=Renderer._gui_ds;
   Renderer.set(Renderer._cur_main, Renderer._cur_main_ds, true);
   return cur;
}
/******************************************************************************/
RippleFx& RippleFx::reset()
{
   xx=60;
   xy=40;
   yx=60;
   yy=35;
   step       =Time.appTime();
   power      =0.005f;
   alpha_scale=0;
   alpha_add  =1;
   center=0.5f;
   return T;
}
void RippleFx::draw(C Image &image, C Rect &rect)
{
   if(!Sh.Ripple)
   {
      Sh.Ripple      =Sh.get("Ripple");
      Sh.RippleParams=GetShaderParam("Rppl");
   }
   Sh.RippleParams->set(T);
   VI.shader(Sh.Ripple); image.draw(rect);
}
/******************************************************************************/
TitlesFx& TitlesFx::reset()
{
   step  =Time.appTime()*2;
   center=0.5f;
   range =0.4f;
   smooth=0.1f;
   swirl =0.015f;
   return T;
}
void TitlesFx::draw(C Image &image)
{
   SPSet("Ttls", T);
   Sh.imgSize(image);
   if(!Sh.Titles)Sh.Titles=Sh.get("Titles"); Sh.Titles->draw(image);
}
/******************************************************************************/
void FadeFx(C Image &image, Flt time, Image *fade_modifier)
{
   Sh.Img[1]->set(fade_modifier);
   Sh.Step  ->set(time         );
   if(!Sh.Fade)Sh.Fade=Sh.get("Fade"); Sh.Fade->draw(image);
}
/******************************************************************************/
void WaveFx(Flt time, Flt scale)
{
   if(scale>0 && scale<1)
      if(C ImageRTPtr &back=Renderer.getBackBuffer())
   {
      Matrix m;
      m.setPos(Vec2(-0.5f)).scale(Vec(Cos(time), Sin(time), 0), scale)
       .move  (Vec2( 0.5f));
      Sh.Color[0]->set(Vec(m.x.x, m.x.y, m.pos.x));
      Sh.Color[1]->set(Vec(m.y.x, m.y.y, m.pos.y));
      ALPHA_MODE alpha=D.alpha(ALPHA_NONE); // disable alpha blending
      if(!Sh.Wave)Sh.Wave=Sh.get("Wave"); Sh.Wave->draw(back);
      D.alpha(alpha);
   }
}
/******************************************************************************/
void RadialBlurFx(Flt scale, Flt alpha, C Vec2 &center)
{
   if(scale>0 && alpha>0)
      if(C ImageRTPtr &back=Renderer.getBackBuffer())
   {
      Sh.Color[0]->set(Vec4(D.screenToUV(center), 1+Abs(scale), alpha));
      if(!Sh.RadialBlur)Sh.RadialBlur=Sh.get("RadialBlur"); Sh.RadialBlur->draw(back);
   }
}
/******************************************************************************/
}
/******************************************************************************/
