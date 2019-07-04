struct LIGHT_DIR
{
   MP Vec  dir;
   MP Vec4 color; // a=spec
 //MP Vec  vol_exponent_steam; not used by GLSL
};
PAR LIGHT_DIR Light_dir;
PAR MP Vec AmbColor, AmbNSColor;
