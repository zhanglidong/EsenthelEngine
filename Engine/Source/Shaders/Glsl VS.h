PAR HP Flt VtxHeightmap;
PAR MP Flt VtxSkinning;

#define ATTR attribute
// !! must be in sync with GL_VTX_SEMANTIC !!
ATTR HP Vec4 ATTR0 ; // pos
ATTR MP Vec  ATTR9 ; // hlp
ATTR MP Vec  ATTR1 ; // nrm
ATTR MP Vec4 ATTR2 ; // tan bin
ATTR HP Vec2 ATTR3 ; // tex
ATTR HP Vec2 ATTR4 ; // tex1
ATTR HP Vec2 ATTR11; // tex2
ATTR MP Flt  ATTR10; // size
ATTR MP Vec  ATTR5 ; // bone, should be MP VecI but won't compile
ATTR MP Vec  ATTR6 ; // weight
ATTR MP Vec4 ATTR8 ; // material
ATTR MP Vec4 ATTR7 ; // color

MP Vec  vtx_nrm      () {return ATTR1 ;}
MP Vec  vtx_tan      () {return ATTR2.xyz;}
MP Flt  vtx_tanW     () {return ATTR2.w;}
HP Vec2 vtx_pos2     () {return ATTR0.xy;}
HP Vec  vtx_pos      () {return ATTR0.xyz;}
HP Vec4 vtx_pos4     () {return ATTR0 ;}
MP Vec  vtx_hlp      () {return ATTR9 ;}
HP Vec2 vtx_tex      () {return ATTR3;}
HP Vec2 vtx_texHM    () {return ATTR0.xz*Vec2(VtxHeightmap, -VtxHeightmap);}
HP Vec2 vtx_tex1     () {return ATTR4 ;}
HP Vec2 vtx_tex2     () {return ATTR11;}
MP VecI vtx_bone     () {return Bool(VtxSkinning) ? VecI(ATTR5) : VecI(0, 0, 0);}
MP Vec  vtx_weight   () {return ATTR6;}
MP Vec4 vtx_material () {return ATTR8;}
MP Vec  vtx_material3() {return ATTR8.xyz;}
MP Flt  vtx_size     () {return ATTR10;}

#if LINEAR_GAMMA
MP Vec4 vtx_color     () {return MP Vec4(SRGBToLinear    (ATTR7.rgb), ATTR7.a);} // sRGB vertex color (precise)
MP Vec  vtx_color3    () {return         SRGBToLinear    (ATTR7.rgb)          ;} // sRGB vertex color (precise)
MP Vec4 vtx_colorFast () {return MP Vec4(SRGBToLinearFast(ATTR7.rgb), ATTR7.a);} // sRGB vertex color (fast)
MP Vec  vtx_colorFast3() {return         SRGBToLinearFast(ATTR7.rgb)          ;} // sRGB vertex color (fast)
#else
MP Vec4 vtx_color     () {return ATTR7    ;} // sRGB vertex color (precise)
MP Vec  vtx_color3    () {return ATTR7.rgb;} // sRGB vertex color (precise)
MP Vec4 vtx_colorFast () {return ATTR7    ;} // sRGB vertex color (fast)
MP Vec  vtx_colorFast3() {return ATTR7.rgb;} // sRGB vertex color (fast)
#endif
