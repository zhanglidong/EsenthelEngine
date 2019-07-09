/******************************************************************************/
#if EE_PRIVATE
struct MainShaderClass
{
   Str         path;
   ShaderFile *shader;

   // get
   Shader* find(C Str8 &name); // find shader, null on fail
   Shader*  get(C Str8 &name); //  get shader, Exit on fail

   // effects
   static void clear(                C Vec4  &color,                                 C Rect *rect=null);
   static void draw (C Image &image,                                                 C Rect *rect=null);
   static void draw (C Image &image, C Color &color, C Color &color_add=TRANSPARENT, C Rect *rect=null);
   static void draw (C Image &image, C Vec4  &color, C Vec4  &color_add=Vec4Zero   , C Rect *rect=null);

   INLINE void imgSize(C Image &image) {ImgSize->set           (Vec4(1.0f/image.hwSize(), image.hwSize()));} // xy=1/hwSize(), zw=hwSize(), this format is also required for SMAA
   INLINE void  rtSize(C Image &image) {RTSize ->setConditional(Vec4(1.0f/image.hwSize(), image.hwSize()));} // xy=1/hwSize(), zw=hwSize(), this format is also required for SMAA

   // private
   void del           ();
   void createSamplers();
   void create        ();
   void compile       ();
   void getTechniques ();
   void connectRT     ();

   ShaderImage
      *Img  [4], *ImgMS[2],
      *ImgX [4], *ImgXMS,
      *ImgXF[2],
      *ImgXY,
      *Cub[2],
      *Vol,
      *VolXY[2],
      *Depth, *DepthMS,
      *ShdMap[2],

      *Col[4],
      *Nrm[4],
      *Det[4],
      *Mac[4],
      *Rfl[4],
      *Lum;

   ShaderParam
      *ImgSize    ,
      *ImgClamp   ,
      *RTSize     ,
      *Coords     ,
      *Viewport   ,
      *DepthWeightScale,

      *CamAngVel ,
      *ObjAngVel ,
      *ObjVel    ,
      *ViewMatrix,
      *CamMatrix ,
      *ProjMatrix,
      *FurVel    ,
      *ClipPlane ,

      *Material        ,
      *MultiMaterial[4],

      *Light_dir   ,
      *Light_point ,
      *Light_linear,
      *Light_cone  ,

      *Step         ,
      *Color[2]     ,
      *BehindBias   ,
      *AllowBackFlip,

      *VtxSkinning ,
      *VtxHeightmap,

      *FontShadow  ,
      *FontLum     ,
      *FontContrast,
      *FontShade   ,
      *FontDepth   ,

      *LightMapScale,

      *GrassRangeMulAdd,
      *BendFactor,

      *Volume,

      *RippleParams,

      *AmbientMaterial ,
      *AmbientContrast ,
      *AmbientRange    ,
      *AmbientScale    ,
      *AmbientBias     ,
      *AmbientColor_l  , // Vec Linear Gamma
      *AmbientColorNS_l, // Vec Linear Gamma + NightShade
      *NightShadeColor ,

      *HdrBrightness,
      *HdrExp,
      *HdrMaxDark,
      *HdrMaxBright,
      *HdrWeight,

      *TesselationDensity,

      *Sun            ,
      *SkyFracMulAdd  ,
      *SkyDnsMulAdd   ,
      *SkyDnsExp      ,
      *SkyHorExp      ,
      *SkyBoxBlend    ,
      *SkyHorCol      ,
      *SkySkyCol      ,
      *SkyStarOrn     ,
      *SkySunHighlight,
      *SkySunPos      ,

      *FogColor_Density     ,
      *LocalFogColor_Density,
      *LocalFogInside       ,

      *VertexFogMulAdd,
      *VertexFogColor ,

      *ShdJitter     ,
      *ShdRange      ,
      *ShdRangeMulAdd,
      *ShdOpacity    ,
      *ShdStep[6]    ,
      *ShdMatrix     ,
      *ShdMatrix4[6] ,

      *ParticleFrames,

      *DecalParams,
      *OverlayParams,
      
      *SMAAThreshold;

   // SHADERS
   Shader
      *Draw2DFlat          ,
      *Draw3DFlat          ,
      *Draw2DCol           ,
      *Draw3DCol           ,
      *Draw2DTex           ,
      *Draw2DTexC          ,
      *Draw2DTexCol        ,
      *Draw3DTex   [2][2]  , // [AlphaTest] [Fog]
      *Draw3DTexCol[2][2]  , // [AlphaTest] [Fog]
      *Draw2DDepthTex   [2], // [AlphaTest]
      *Draw2DDepthTexCol[2], // [AlphaTest]
      *DrawX               ,
      *DrawXG              ,
      *DrawXC              ,
      *DrawXCD             ,
      *DrawXCG             ,
      *DrawXCDG            ,
      *Simple              ,

      *DrawMask,
      *DrawCubeFace,

      *FontCur,
      *FontCurSP,
      *Font[2][2], // [Depth][Gamma]
      *FontSP [2], //        [Gamma]

      *Laser[2],

      *PaletteDraw,

      // BASIC 2D
      *SetCol ,
      *Draw   ,
      *DrawC  ,
      *DrawCG ,
      *DrawA  ,
      *DrawMs1,
      *DrawMsN,
      *DrawMsM,

      // BLUR
      #define SHADER_BLUR_RANGE 5 // 5 pixel range in both directions
      *BlurX[2], // [High]
      *BlurY[2], // [High]
    /**BlurX_X,
      *BlurY_X,*/

      // MAX
      *MaxX,
      *MaxY,

      // VIDEO
      *YUV [2], // [Gamma]
      *YUVA[2], // [Gamma]

      // 2D FX
      *ColTrans       ,
      *ColTransHB     ,
      *ColTransHSB    ,
      *Ripple         ,
      *Titles         ,
      *Fade           ,
      *Wave           ,
      *RadialBlur     ,
      *Outline        ,
      *OutlineDS      ,
      *OutlineClip    ,
      *OutlineApply   ,
      *EdgeDetect     ,
      *EdgeDetectApply,
      *DetectMSCol    ,
    //*DetectMSNrm    ,

      *LinearizeDepth[2][3], // [Perspective] [MultiSample]
      *ResolveDepth,
      *SetDepth,
      *Dither,
      *Combine,
      *CombineMS,
      *CombineSS,
      *CombineSSAlpha,

      // FOG
      *Fog[3]  , // [MultiSample]
      *FogBox  ,
      *FogBox0 ,
      *FogBox1 ,
      *FogHgt  ,
      *FogHgt0 ,
      *FogHgt1 ,
      *FogBall ,
      *FogBall0,
      *FogBall1;
   void initFogBoxShaders ();   INLINE void loadFogBoxShaders () {if(SLOW_SHADER_LOAD)initFogBoxShaders ();}
   void initFogHgtShaders ();   INLINE void loadFogHgtShaders () {if(SLOW_SHADER_LOAD)initFogHgtShaders ();}
   void initFogBallShaders();   INLINE void loadFogBallShaders() {if(SLOW_SHADER_LOAD)initFogBallShaders();}

   Shader
      // VOLUME
      *Volume0[2], // [RedGreen as LumAlpha]
      *Volume1[2], // [RedGreen as LumAlpha]
      *Volume2[2], // [RedGreen as LumAlpha]

      // EDGE SOFTEN
      *FXAA[2], // [Gamma]
   #if SUPPORT_MLAA
      *MLAAEdge ,
      *MLAABlend,
      *MLAA     ,
   #endif
      *SMAAEdge[2], // [Gamma]
      *SMAABlend  ,
      *SMAA       ,

      // PARTICLE
      *Bilb                ,
      *Particle[2][2][3][2], // [Palette] [Soft] [Anim] [Motion stretch affects opacity]

      // POINT
      *DrawTexPoint ,
      *DrawTexPointC,

      // CUBIC
      *DrawTexCubicFast    ,
      *DrawTexCubicFastC   ,
      *DrawTexCubicFast1   ,
      *DrawTexCubicFastD   ,
      *DrawTexCubicFastRGB ,
      *DrawTexCubicFastRGBD,
      *DrawTexCubic        ,
      *DrawTexCubicC       ,
      *DrawTexCubic1       ,
      *DrawTexCubicD       ,
      *DrawTexCubicRGB     ,
      *DrawTexCubicRGBD    ;
   void initCubicShaders();   INLINE void loadCubicShaders() {if(SLOW_SHADER_LOAD)initCubicShaders();}

   // SHADOWS
   Shader
      *ShdDir[6][2][2], // [NumberOfMaps] [Clouds] [MultiSample]
      *ShdPoint    [2], //                         [MultiSample]
      *ShdCone     [2], //                         [MultiSample]
      *ShdBlur     [4], // [Quality]
      *ShdBlurX       ,
      *ShdBlurY       ;
   Shader* getShdDir  (Int map_num, Bool clouds, Bool multi_sample);
   Shader* getShdPoint(                          Bool multi_sample);
   Shader* getShdCone (                          Bool multi_sample);

   // LIGHT
   Shader
      *LightDir   [2]   [2][2], // [Shadow]         [MultiSample] [QualityUnpack]
      *LightPoint [2]   [2][2], // [Shadow]         [MultiSample] [QualityUnpack]
      *LightLinear[2]   [2][2], // [Shadow]         [MultiSample] [QualityUnpack]
      *LightCone  [2][2][2][2]; // [Shadow] [Image] [MultiSample] [QualityUnpack]
   Shader* getLightDir   (Bool shadow,             Bool multi_sample, Bool quality);
   Shader* getLightPoint (Bool shadow,             Bool multi_sample, Bool quality);
   Shader* getLightLinear(Bool shadow,             Bool multi_sample, Bool quality);
   Shader* getLightCone  (Bool shadow, Bool image, Bool multi_sample, Bool quality);

   // COL LIGHT
   Shader
      *ColLight[3][2][2][2]; // [Multisample] [AmbientOcclusion] [CelShade] [NightShade]
   Shader* getColLight(Int multi_sample, Bool ao, Bool cel_shade, Bool night_shade);

   // BLOOM
   ShaderParam
      *BloomParams;
   Shader
      *BloomDS[2][2][2][2][2], // [Glow] [UVClamp] [HalfRes] [Saturate] [Gamma]
      *Bloom  [2][2]         ; // [Dither] [Gamma]
   Shader* getBloomDS(Bool glow, Bool viewport_clamp, Bool half, Bool saturate, Bool gamma);
   Shader* getBloom  (Bool dither, Bool gamma);

   // SKY
   Shader
      *SunRaysMask[2]      , // [Mask]
      *SunRays [2][2][2][2], // [High] [Dither] [Jitter] [Gamma]
      *SkyTF[2]   [2]   [2], // [Textures(0->1, 1->2)]         [Cloud  ]               [Dither] (Textures   +Flat)
      *SkyT [2]      [3][2], // [Textures(0->1, 1->2)]                   [MultiSample] [Dither] (Textures        )
      *SkyAF[2][2][2]   [2], // [PerVertex           ] [Stars] [Cloud  ]               [Dither] (Atmospheric+Flat)
      *SkyA [2][2][2][3][2]; // [PerVertex           ] [Stars] [Density] [MultiSample] [Dither] (Atmospheric     )
   Shader* getSunRaysMask(Bool mask);
   Shader* getSunRays    (Bool high, Bool dither, Bool jitter, Bool gamma);
   Shader* getSkyTF(Int textures,                Bool cloud  ,                   Bool dither);
   Shader* getSkyT (Int textures,                              Int multi_sample, Bool dither);
   Shader* getSkyAF(Bool per_vertex, Bool stars, Bool cloud  ,                   Bool dither);
   Shader* getSkyA (Bool per_vertex, Bool stars, Bool density, Int multi_sample, Bool dither);
}extern
   Sh;

struct AmbientOcclusion
{
   ShaderFile *shader;
   Shader     *AO[4][2][2]; // [Quality] [Jitter] [Normal]

   Shader* get(Int quality, Bool jitter, Bool normal);
}extern
   AO;

struct LayeredCloudsFx
{
   ShaderFile  *shader;
   ShaderParam *CL[4], *range;
   Shader      *Clouds[4][2][2]; // [#Layers] [Blend] [Draw Mask to 2nd RT]

   void    load();
   Shader* get(Int layers, Bool blend, Bool mask);
}extern
   LC;

struct VolumetricCloudsFx
{
   ShaderFile  *shader;
   ShaderParam *Cloud, *CloudMap;
   Shader      *Clouds, *CloudsMap,
               *CloudsDraw[2]; // [Gamma]

   void load();
}extern
   VolCloud;

struct VolumetricLights
{
   ShaderFile  *shader;
   ShaderParam *Light_point_range;
   Shader      *VolDir[6][2], // [ShdMapNum] [Clouds]
               *VolPoint    ,
               *VolLinear   ,
               *VolCone     ,
               *Volumetric  ,
               *VolumetricA ;

   void load();
}extern
   VL;

struct HDR
{
   ShaderFile *shader;
   Shader     *HdrDS[2], // [Step]
              *HdrUpdate,
              *Hdr[2]; // [Dither]

   void load();
}extern
   Hdr;

struct MotionBlur
{
   ShaderFile  *shader;
   ShaderParam *MotionUVMulAdd     ,
               *MotionVelScaleLimit,
               *MotionPixelSize    ;
   Shader      *Explosion          ,
               *ClearSkyVel        ,
               *Convert      [2][2], // [High][Clamp]
               *Dilate             ,
               *SetDirs      [2]   , // [Clamp]
               *Blur         [2]   ; // [Dither]

   struct Pixel
   {
      Int     pixels;
      Shader *DilateX[2], // [Diagonal]
             *DilateY[2]; // [Diagonal]
   }pixels[9];

   void load();
 C Pixel* pixel(Int pixel, Bool diagonal);

}extern
   Mtn;

struct DepthOfField
{
   ShaderFile  *shader;
   ShaderParam *DofParams;
   Shader      *DofDS[2][2][2], // [Clamp ][Realistic][Half]
               *Dof  [2][2]   ; // [Dither][Realistic]

   struct Pixel
   {
      Int     pixels;
      Shader *BlurX,
             *BlurY;
   }pixels[11];

   void load();
   Shader* getDS(Bool clamp , Bool realistic, Bool half);
   Shader* get  (Bool dither, Bool realistic);
 C Pixel& pixel(Int pixel);

}extern
   Dof;

struct WaterShader
{
   ShaderFile *shader;
   Shader     *Ocean [2]      , // [FakeReflect]
              *Lake  [2]      , // [FakeReflect]
              *River [2]      , // [FakeReflect]
              *OceanL[2][7][2], // [FakeReflect] [Shadows] [Soft]
              *LakeL [2][7][2], // [FakeReflect] [Shadows] [Soft]
              *RiverL[2][7][2], // [FakeReflect] [Shadows] [Soft]
              *Apply [2][2]   , // [Refract] [Depth]
              *Under [2]      ; // [Refract]

   void load();
}extern
   WS;

extern ShaderImage::Sampler SamplerPoint, SamplerLinearWrap, SamplerLinearWCC, SamplerLinearCWC, SamplerLinearCWW, SamplerLinearClamp, SamplerFont, SamplerAnisotropic, SamplerShadowMap;

void CreateAnisotropicSampler();
void CreateFontSampler       ();

Str8 TechNameSimple    (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int per_pixel, Int tess);
Str8 TechNameDeferred  (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int macro, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx, Int tess);
Str8 TechNameForward   (Int skin, Int materials, Int textures, Int bump_mode, Int alpha_test, Int light_map, Int detail, Int rflct, Int color, Int mtrl_blend, Int heightmap, Int fx,   Int light_dir, Int light_dir_shd, Int light_dir_shd_num,   Int light_point, Int light_point_shd,   Int light_linear, Int light_linear_shd,   Int light_cone, Int light_cone_shd,   Int tess);
Str8 TechNameBlendLight(Int skin, Int color    , Int textures, Int bump_mode, Int alpha_test, Int alpha, Int light_map, Int rflct, Int fx, Int per_pixel, Int shadow_maps);
Str8 TechNamePosition  (Int skin, Int textures, Int test_blend, Int fx, Int tess);
Str8 TechNameBlend     (Int skin, Int color, Int rflct, Int textures, Int light_map);
Str8 TechNameSetColor  (Int skin, Int textures, Int tess);
Str8 TechNameBehind    (Int skin, Int textures);
Str8 TechNameEarlyZ    (Int skin);
Str8 TechNameAmbient   (Int skin, Int alpha_test);
Str8 TechNameOverlay   (Int skin, Int normal);
Str8 TechNameFurBase   (Int skin, Int size, Int diffuse);
Str8 TechNameFurSoft   (Int skin, Int size, Int diffuse);
Str8 TechNameTattoo    (Int skin, Int tess);

struct DefaultShaders
{
   Bool valid,
        detail, macro, reflect, light_map,
        mtrl_blend,
        heightmap,
        normal, color, size,
        fur, blend, grass, leaf, ambient,
        alpha, alpha_test, alpha_blend, alpha_blend_light,
        skin,
        tess;
   Byte materials, textures, bump, fx;

   void      init(C Material *material[4], UInt mesh_base_flag, Int lod_index, Bool heightmap);
   DefaultShaders(C Material *material[4], UInt mesh_base_flag, Int lod_index, Bool heightmap) {init(material, mesh_base_flag, lod_index, heightmap);}
   DefaultShaders(C Material *material   , UInt mesh_base_flag, Int lod_index, Bool heightmap);

   Shader* EarlyZ ()C;
   Shader* Simple ()C;
   Shader* Solid  (Bool mirror=false)C;
   Shader* Ambient()C;
   Shader* Outline()C;
   Shader* Behind ()C;
   Shader* Fur    ()C;
   Shader* Shadow ()C;
   Shader* Blend  ()C;
   Shader* Overlay()C;
   Shader* get    (RENDER_MODE mode)C;
   FRST  * Frst   ()C;
   BLST  * Blst   ()C;

   void set(Shader *shader[RM_SHADER_NUM], FRST **frst, BLST **blst);
};
#endif
/******************************************************************************/
