PAR MP Vec4 Highlight;

struct MaterialClass
{
   MP Vec4 _color;
   MP Vec4 _ambient_specular;
   MP Vec4 _sss_glow_rough_bump;
   HP Vec4 _texscale_detscale_detpower_reflect;
};

PAR MaterialClass Material;

MP Vec4 MaterialColor   () {return Material._color;}
MP Vec  MaterialColor3  () {return Material._color.rgb;}
MP Flt  MaterialAlpha   () {return Material._color.a;}
MP Vec  MaterialAmbient () {return Material._ambient_specular.xyz;}
MP Flt  MaterialGlow    () {return Material._sss_glow_rough_bump.y;}
HP Flt  MaterialTexScale() {return Material._texscale_detscale_detpower_reflect.x;}
MP Flt  MaterialReflect () {return Material._texscale_detscale_detpower_reflect.w;}
