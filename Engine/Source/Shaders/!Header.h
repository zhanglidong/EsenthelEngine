/******************************************************************************
 * Copyright (c) Grzegorz Slazinski. All Rights Reserved.                     *
 * Esenthel Engine (http://esenthel.com) shader file.                         *
/******************************************************************************

   Optimization guidelines:
      -use MAD wherever possible A*B+C
      -Sign(x)*y can be replaced with (x>=0) ? y : -y (if Sign(x)==0 is not important)
      -use Sat(x) instead of Max(0, x) or Min(1, x) where 'x' is expected to be in 0..1 range
      -use Half, VecH2, VecH, VecH4 precision where possible
      -use NOPERSP for flat 2D shaders
      -offload some calculations on the CPU or Vertex Shader
      -use 'TexPoint' or 'TexLod' wherever possible
      -use constants without any suffix (0.0 instead of 0.0f or 0.0h) - https://gpuopen.com/first-steps-implementing-fp16/ - "Using either the h or f suffix will result in a conversion. It is better to use the unadorned literal, such as 0.0, 1.5 and so on."

/******************************************************************************/
#include "!Header CPU.h"
#define LINEAR_GAMMA 1
#define CONCAT(a, b) a##b
/******************************************************************************/
// DATA TYPES
/******************************************************************************/
#pragma pack_matrix(column_major) // always use "Column Major" matrix packing for most efficient 'Matrix' packing (orn+scale+pos) to use only 3x Vec4

#define Bool    bool
#define Int     int
#define UInt    uint
#define Flt     float
#define VecI2   int2
#define VecI    int3
#define VecI4   int4
#define VecU2   uint2
#define VecU    uint3
#define VecU4   uint4
#define Vec2    float2
#define Vec     float3
#define Vec4    float4
#define Matrix3 float3x3
#define Matrix  float4x3
#define Matrix4 float4x4

#define ImageF      Texture2D  <Flt  >
#define ImageH      Texture2D  <Half >
#define ImageH2     Texture2D  <VecH2>
#define Image       Texture2D  <VecH4>
#define Image3D     Texture3D  <VecH4>
#define Image3DH2   Texture3D  <VecH2>
#define ImageCube   TextureCube<VecH4>
#define ImageShadow Texture2D  <Half >

#define        SAMPLER(name, index) sampler                name : register(s##index) //        sampler
#define SHADOW_SAMPLER(name, index) SamplerComparisonState name : register(s##index) // shadow sampler
/******************************************************************************/
// HELPERS
/******************************************************************************/
#define PIXEL               Vec4 pixel :SV_Position               // pixel coordinates, integer based in format Vec4(x, y, 0, 0) ranges from (0, 0) to (RenderTarget.w(), RenderTarget.h())
#define IS_FRONT            Bool front :SV_IsFrontFace            // face front side
#define CLIP_DIST       out Flt  O_clip:SV_ClipDistance           // clip plane distance
#define CLIP_PLANE(pos) O_clip=Dot(Vec4((pos).xyz, 1), ClipPlane) // perform user plane clipping

#define BUFFER(name)          cbuffer name {                      // declare a constant buffer
#define BUFFER_I(name, index) cbuffer name : register(b##index) { // declare a constant buffer with custom buffer index
#define BUFFER_END            }                                   // end constant buffer declaration

#define POSITION SV_Position
#define DEPTH    SV_Depth
#define TARGET   SV_Target
#define TARGET0  SV_Target0
#define TARGET1  SV_Target1
#define TARGET2  SV_Target2
#define TARGET3  SV_Target3

#define NOPERSP noperspective // will disable perspective interpolation

#define FLATTEN [flatten] // will make a conditional statement flattened, use before 'if'        statement
#define BRANCH  [branch ] // will make a conditional statement branched , use before 'if'        statement
#define LOOP    [loop   ] // will make a loop looped                    , use before 'for while' statements
#define UNROLL  [unroll ] // will make a loop unrolled                  , use before 'for while' statements

#if DX_SHADER_COMPILER
   #define LOC(i) [[vk::location(i)]]
#else
   #define LOC(i)
#endif
/******************************************************************************/
// FUNCTIONS
/******************************************************************************/
#define Dot       dot
#define Cross     cross
#define Sign      sign
#define Abs       abs
#define Sat       saturate
#define Mid       clamp
#define Frac      frac
#define Round     round
#define Trunc     trunc
#define Floor     floor
#define Ceil      ceil
#define Sqrt      sqrt
#define Normalize normalize
#define Pow       pow
#define Sin       sin
#define Cos       cos
#define Acos      acos
#define Asin      asin
#define Lerp      lerp
/******************************************************************************/
// CONSTANTS
/******************************************************************************/
#define MAX_MATRIX 256 // maximum number of matrixes
#define HALF_MIN   0.00006103515625                   // Minimum positive value of 16-bit real (Half)
#define HALF_MAX   65504                              // Maximum possible value of 16-bit real (Half)
#define EPS        0.0001                             // float epsilon
#define EPS_COL    (1.0/256)                          // color epsilon
#define EPS_LUM    (LINEAR_GAMMA ? 1.0/512 : EPS_COL) // light epsilon (need a little extra precision for linear gamma)

#define PI_6    0.5235987755982988 // PI/6 ( 30 deg) Flt
#define PI_4    0.7853981633974483 // PI/4 ( 45 deg) Flt
#define PI_3    1.0471975511965977 // PI/3 ( 60 deg) Flt
#define PI_2    1.5707963267948966 // PI/2 ( 90 deg) Flt
#define PI      3.1415926535897932 // PI   (180 deg) Flt
#define PI2     6.2831853071795864 // PI*2 (360 deg) Flt
#define SQRT2   1.4142135623730950 // Sqrt(2)
#define SQRT3   1.7320508075688773 // Sqrt(3)
#define SQRT2_2 0.7071067811865475 // Sqrt(2)/2
#define SQRT3_3 0.5773502691896257 // Sqrt(3)/3
#define TAN     0.5                // tangent calculation constant

#define ColorLumWeight  VecH(0.2126, 0.7152, 0.0722)
#define ColorLumWeight2 VecH(0.2990, 0.5870, 0.1140)

#define TRANSLUCENT_VAL 0.5

#define REFLECT_OCCL 0 // if apply occlusion for reflectivity below 0.02 #SpecularReflectionFromZeroSmoothReflectivity
/******************************************************************************/
// RENDER TARGETS
/******************************************************************************/
#define MS_SAMPLES 4 // number of samples in multi-sampled render targets

#define SIGNED_NRM_RT 0     // if Normal   Render Target is signed, in GL it depends on "GL_EXT_render_snorm", never because we use IMAGE_R10G10B10A2 which is unsigned
#define SIGNED_VEL_RT (!GL) // if Velocity Render Target is signed, in GL it depends on "GL_EXT_render_snorm"

#define REVERSE_DEPTH (!GL) // if Depth Buffer is reversed. Can't enable on GL because for some reason (might be related to #glClipControl) it disables far-plane depth clipping, which can be observed when using func=FUNC_ALWAYS inside D.depthFunc. Even though we clear the depth buffer, there may still be performance hit, because normally geometry would already get clipped due to far plane, but without it, per-pixel depth tests need to be performed.
#if     REVERSE_DEPTH
   #define Z_FRONT             1.0
   #define Z_BACK              0.0
   #define DEPTH_MIN           Max
   #define DEPTH_MAX           Min
   #define DEPTH_FOREGROUND(x) ((x)> Z_BACK)
   #define DEPTH_BACKGROUND(x) ((x)<=Z_BACK)
   #define DEPTH_SMALLER(x, y) ((x)> (y))
   #define DEPTH_DEC(x, y)     ((x)+=(y))
#else
   #define Z_FRONT             0.0
   #define Z_BACK              1.0
   #define DEPTH_MIN           Min
   #define DEPTH_MAX           Max
   #define DEPTH_FOREGROUND(x) ((x)< Z_BACK)
   #define DEPTH_BACKGROUND(x) ((x)>=Z_BACK)
   #define DEPTH_SMALLER(x, y) ((x)< (y))
   #define DEPTH_DEC(x, y)     ((x)-=(y))
#endif
/******************************************************************************/
// TEXTURE ACCESSING
/******************************************************************************/
#define Tex(    image, uv )   image.Sample(SamplerDefault, uv ) // access a 2D   texture
#define Tex3D(  image, uvw)   image.Sample(SamplerDefault, uvw) // access a 3D   texture
#define TexCube(image, uvw)   image.Sample(SamplerDefault, uvw) // access a Cube texture

#define TexLod(     image, uv      )   image.SampleLevel(SamplerDefault, uv ,   0) // access 2D   texture's   0-th MipMap (LOD level=  0)
#define TexLodI(    image, uv , lod)   image.SampleLevel(SamplerDefault, uv , lod) // access 2D   texture's lod-th MipMap (LOD level=lod)
#define Tex3DLod(   image, uvw     )   image.SampleLevel(SamplerDefault, uvw,   0) // access 3D   texture's   0-th MipMap (LOD level=  0)
#define TexCubeLod( image, uvw     )   image.SampleLevel(SamplerDefault, uvw,   0) // access Cube texture's   0-th MipMap (LOD level=  0)
#define TexCubeLodI(image, uvw, lod)   image.SampleLevel(SamplerDefault, uvw, lod) // access Cube texture's lod-th MipMap (LOD level=lod)

#define TexPoint(image, uv)   image.SampleLevel(SamplerPoint, uv, 0)

#define TexGather(image, uv)   image.Gather(SamplerPoint, uv) // gather available since SM_4_1, GL 4.0, GL ES 3.1

#define TexClamp(    image, uv )   image.Sample     (SamplerLinearClamp, uv    )
#define TexLodClamp( image, uv )   image.SampleLevel(SamplerLinearClamp, uv , 0)
#define Tex3DLodWrap(image, uvw)   image.SampleLevel(SamplerLinearWrap , uvw, 0)

#define TexSample(image, pixel, i)   image.Load(pixel, i) // access i-th sample of a multi-sampled texture

#define TexDepthRawPoint( uv)                       TexPoint (Depth  , uv).x
#define TexDepthRawLinear(uv)                       TexLod   (Depth  , uv).x
#define TexDepthPoint(    uv)        LinearizeDepth(TexPoint (Depth  , uv).x)
#define TexDepthLinear(   uv)        LinearizeDepth(TexLod   (Depth  , uv).x)
#define TexDepthGather(   uv)                       TexGather(Depth  , uv)
#define TexDepthMSRaw(pixel, sample)                TexSample(DepthMS, pixel, sample).x
#define TexDepthMS(   pixel, sample) LinearizeDepth(TexSample(DepthMS, pixel, sample).x)

#if !GL
#define TexShadow(image, uvw)   image.SampleCmpLevelZero(SamplerShadowMap, uvw.xy, uvw.z)
#else
#define TexShadow(image, uvw)   image.SampleCmpLevelZero(SamplerShadowMap, uvw.xy, uvw.z*0.5+0.5) // adjust OpenGL depth scale (z' = z*0.5 + 0.5)
#endif
/******************************************************************************/
// CONSTANTS
/******************************************************************************/
#include "!Set Prec Struct.h"

struct ViewportClass
{
   Flt  from, range;
   Vec2 center, size, size_fov_tan;
   Vec4 FracToPosXY, ScreenToPosXY, PosToScreen;
};

BUFFER_I(Viewport, SBI_VIEWPORT)
   Vec4          Coords  ;
   Vec4          RTSize  ; // xy=1/Image.hwSize(), zw=Image.hwSize(), this format is also required for SMAA
   ViewportClass Viewport;
BUFFER_END

BUFFER(Constants)
   const Vec2 BlendOfs4[4]=
   {
      Vec2( 0.5, -1.5),
      Vec2(-1.5, -0.5),
      Vec2( 1.5,  0.5),
      Vec2(-0.5,  1.5),
   };
 /*const Vec2 BlendOfs5[5]=
   {
      Vec2(-0.5, -1.5),

      Vec2(-1.5, -0.5),
      Vec2( 1.5, -0.5),

      Vec2(-0.5,  1.5),
      Vec2( 1.5,  1.5),
   };*/
   const Vec2 BlendOfs6[6]=
   {
      Vec2( 0.5, -2.5),

      Vec2(-0.5, -0.5),
      Vec2( 1.5, -0.5),

      Vec2(-2.5,  0.5),

      Vec2(-0.5,  1.5),
      Vec2( 1.5,  1.5),
   };
   const Vec2 BlendOfs8[8]=
   {
      Vec2(-1.5, -2.5),

      Vec2( 0.5, -1.5),
      Vec2( 2.5, -1.5),

      Vec2(-1.5, -0.5),

      Vec2( 1.5,  0.5),

      Vec2(-2.5,  1.5),
      Vec2(-0.5,  1.5),

      Vec2( 1.5,  2.5),
   };
 /*const Vec2 BlendOfs9[9]=
   {
      Vec2(-2.5, -2.5),
      Vec2(-0.5, -2.5),
      Vec2( 1.5, -2.5),

      Vec2(-2.5, -0.5),
      Vec2(-0.5, -0.5),
      Vec2( 1.5, -0.5),

      Vec2(-2.5,  1.5),
      Vec2(-0.5,  1.5),
      Vec2( 1.5,  1.5),
   };*/
   const Vec2 BlendOfs12[12]=
   {
      Vec2( 0.5, -3.5),

      Vec2(-1.5, -2.5),

      Vec2( 0.5, -1.5),
      Vec2( 2.5, -1.5),

      Vec2(-3.5, -0.5),
      Vec2(-1.5, -0.5),

      Vec2( 1.5,  0.5),
      Vec2( 3.5,  0.5),

      Vec2(-2.5,  1.5),
      Vec2(-0.5,  1.5),

      Vec2( 1.5,  2.5),

      Vec2(-0.5,  3.5),
   };
 /*const Vec2 BlendOfs13[13]=
   {
      Vec2(-0.5, -3.5),

      Vec2( 1.5, -2.5),

      Vec2(-0.5, -1.5),

      Vec2(-3.5, -0.5),
      Vec2(-1.5, -0.5),
      Vec2( 1.5, -0.5),
      Vec2( 3.5, -0.5),

      Vec2(-2.5,  1.5),
      Vec2(-0.5,  1.5),
      Vec2( 1.5,  1.5),
      Vec2( 3.5,  1.5),

      Vec2(-0.5,  3.5),
      Vec2( 1.5,  3.5),
   };*/
   #define V(x) (Flt(x-32)/32/256) // gives -1..1 / 256 range
   const Flt OrderDither[64]=
   {
      V( 0), V(32), V( 8), V(40), V( 2), V(34), V(10), V(42),
      V(48), V(16), V(56), V(24), V(50), V(18), V(58), V(26),
      V(12), V(44), V( 4), V(36), V(14), V(46), V( 6), V(38),
      V(60), V(28), V(52), V(20), V(62), V(30), V(54), V(22),
      V( 3), V(35), V(11), V(43), V( 1), V(33), V( 9), V(41),
      V(51), V(19), V(59), V(27), V(49), V(17), V(57), V(25),
      V(15), V(47), V( 7), V(39), V(13), V(45), V( 5), V(37),
      V(63), V(31), V(55), V(23), V(61), V(29), V(53), V(21),
   };
BUFFER_END

BUFFER(Step)
   Flt Step;
BUFFER_END

BUFFER(ImgSize)
   Vec4 ImgSize; // xy=1/Image.hwSize(), zw=Image.hwSize(), this format is also required for SMAA
BUFFER_END

BUFFER(ImgClamp)
   Vec4 ImgClamp; // xy=min.xy, zw=max.xy
BUFFER_END

BUFFER_I(Color, SBI_COLOR)
   VecH4 Color[2];
BUFFER_END
/******************************************************************************/
BUFFER_I(Frame, SBI_FRAME) // once per-frame
   Vec   CamAngVel                 ; // camera angular velocity, pre-multiplied by 'D.motionScale' (don't put to Camera Buffer because this is set once per-frame and ignored for shadow maps)
   Flt   TesselationDensity        ; // tesselation density
   Vec4  ClipPlane=Vec4(0, 0, 0, 1); // clipping plane
   Vec2  GrassRangeMulAdd          ; // factors used for grass opacity calculation
   Bool  FirstPass=true            ; // if first pass (apply Material Ambient)
   VecH  AmbientNSColor            ; // ambient combined with night shade
   VecH  EnvColor                  ; // environment map color
   Half  EnvMipMaps                ; // environment map mip-maps
   VecH4 BendFactor                ; // factors used for grass/leaf bending calculation
BUFFER_END

BUFFER_I(Camera, SBI_CAMERA) // this gets changed when drawing shadow maps
   Matrix4 ProjMatrix; // projection matrix
   Matrix  CamMatrix ; // camera     matrix
BUFFER_END

struct Velocity
{
   VecH lin, //  linear, pre-multiplied by 'D.motionScale', use VecH to allow velocity functions work faster
        ang; // angular, pre-multiplied by 'D.motionScale', use VecH to allow velocity functions work faster
};

BUFFER_I(ObjVel, SBI_OBJ_VEL) // !! WARNING: this CB is dynamically resized, do not add other members !!
   Velocity ObjVel[MAX_MATRIX]; // object velocities
BUFFER_END

BUFFER_I(Mesh, SBI_MESH)
   Flt   VtxHeightmap;
   Bool  VtxSkinning;
   VecH4 Highlight; // this can be modified by engine's 'SetHighlight' function
BUFFER_END

BUFFER(Behind)
   Half BehindBias; // this can be modified by engine's 'SetBehindBias' function
BUFFER_END
/******************************************************************************/
// MATERIALS
/******************************************************************************/
struct MaterialClass // this is used when a MeshPart has only one material
{
   VecH4 color; // !! color must be listed first because ShaderParam handle for setting 'Material.color' is set from the entire Material object pointer !!
   VecH  ambient;
   Half  smooth,
         reflect,
         glow,
         normal,
         bump,
         det_power;
   Flt   det_scale,
         tex_scale;
};
BUFFER_I(Material, SBI_MATERIAL)
   MaterialClass Material;
BUFFER_END
/******************************************************************************/
struct MultiMaterialClass // this is used when a MeshPart has multiple materials
{
   VecH4 color;
   VecH2 base2_mul, base2_add;
   Half  glow, normal, bump, det_mul, det_add, det_inv, macro;
   Flt   tex_scale, det_scale;
};
BUFFER(MultiMaterial0) MultiMaterialClass MultiMaterial0; BUFFER_END
BUFFER(MultiMaterial1) MultiMaterialClass MultiMaterial1; BUFFER_END
BUFFER(MultiMaterial2) MultiMaterialClass MultiMaterial2; BUFFER_END
BUFFER(MultiMaterial3) MultiMaterialClass MultiMaterial3; BUFFER_END
/******************************************************************************/
#include "!Set Prec Default.h"
/******************************************************************************/
// IMAGES
/******************************************************************************/
#include "!Set Prec Image.h"
// #MaterialTextureLayout
Image     Col, Col1, Col2, Col3;
ImageH2   Nrm, Nrm1, Nrm2, Nrm3;
Image     Ext, Ext1, Ext2, Ext3,
          Det, Det1, Det2, Det3,
          Mac, Mac1, Mac2, Mac3,
          Lum;

#define    BUMP_IMAGE   Ext
#define  SMOOTH_CHANNEL x
#define REFLECT_CHANNEL y
#define    BUMP_CHANNEL z
#define   ALPHA_CHANNEL w

Image     Img, Img1, Img2, Img3;
ImageH    ImgX, ImgX1, ImgX2, ImgX3;
ImageF    ImgXF, ImgXF1, Depth;
ImageH2   ImgXY, EnvDFG;
ImageCube Env, Cub, Cub1;
Image3D   Vol;
Image3DH2 VolXY, VolXY1;

Texture2DMS<VecH4, MS_SAMPLES> ImgMS, ImgMS1, ImgMS2;
Texture2DMS<Half , MS_SAMPLES> ImgXMS;
Texture2DMS<VecH2, MS_SAMPLES> ImgXYMS;
Texture2DMS<Flt  , MS_SAMPLES> DepthMS;
#include "!Set Prec Default.h"

       SAMPLER(SamplerDefault    , SSI_DEFAULT     );
       SAMPLER(SamplerPoint      , SSI_POINT       );
       SAMPLER(SamplerLinearClamp, SSI_LINEAR_CLAMP);
       SAMPLER(SamplerLinearWrap , SSI_LINEAR_WRAP );
       SAMPLER(SamplerLinearCWW  , SSI_LINEAR_CWW  );
SHADOW_SAMPLER(SamplerShadowMap  , SSI_SHADOW      );
       SAMPLER(SamplerFont       , SSI_FONT        );
#if GL // use default sampler on GL because it would create a secondary "sampler2D" in GLSL and we would have to set 2 ShaderImage's
   #define SamplerPoint       SamplerDefault
   #define SamplerLinearClamp SamplerDefault
   #define SamplerLinearWrap  SamplerDefault
#endif
/******************************************************************************/
// force convert to Half (can be used for testing Precisions)
inline Half  HALF(Flt  x) {return f16tof32(f32tof16(x));}
inline VecH2 HALF(Vec2 x) {return f16tof32(f32tof16(x));}
inline VecH  HALF(Vec  x) {return f16tof32(f32tof16(x));}
inline VecH4 HALF(Vec4 x) {return f16tof32(f32tof16(x));}

inline Int   Min(Int   x, Int   y                  ) {return min(x, y);}
inline Half  Min(Half  x, Half  y                  ) {return min(x, y);}
inline Flt   Min(Flt   x, Flt   y                  ) {return min(x, y);}
inline Flt   Min(Int   x, Flt   y                  ) {return min(x, y);}
inline Flt   Min(Flt   x, Int   y                  ) {return min(x, y);}
inline VecH2 Min(VecH2 x, VecH2 y                  ) {return min(x, y);}
inline Vec2  Min(Vec2  x, Vec2  y                  ) {return min(x, y);}
inline VecH  Min(VecH  x, VecH  y                  ) {return min(x, y);}
inline Vec   Min(Vec   x, Vec   y                  ) {return min(x, y);}
inline VecH4 Min(VecH4 x, VecH4 y                  ) {return min(x, y);}
inline Vec4  Min(Vec4  x, Vec4  y                  ) {return min(x, y);}
inline Int   Min(Int   x, Int   y, Int   z         ) {return min(x, min(y, z));}
inline Half  Min(Half  x, Half  y, Half  z         ) {return min(x, min(y, z));}
inline Flt   Min(Flt   x, Flt   y, Flt   z         ) {return min(x, min(y, z));}
inline VecH2 Min(VecH2 x, VecH2 y, VecH2 z         ) {return min(x, min(y, z));}
inline Vec2  Min(Vec2  x, Vec2  y, Vec2  z         ) {return min(x, min(y, z));}
inline VecH  Min(VecH  x, VecH  y, VecH  z         ) {return min(x, min(y, z));}
inline Vec   Min(Vec   x, Vec   y, Vec   z         ) {return min(x, min(y, z));}
inline VecH4 Min(VecH4 x, VecH4 y, VecH4 z         ) {return min(x, min(y, z));}
inline Vec4  Min(Vec4  x, Vec4  y, Vec4  z         ) {return min(x, min(y, z));}
inline Int   Min(Int   x, Int   y, Int   z, Int   w) {return min(x, min(y, min(z, w)));}
inline Half  Min(Half  x, Half  y, Half  z, Half  w) {return min(x, min(y, min(z, w)));}
inline Flt   Min(Flt   x, Flt   y, Flt   z, Flt   w) {return min(x, min(y, min(z, w)));}
inline VecH2 Min(VecH2 x, VecH2 y, VecH2 z, VecH2 w) {return min(x, min(y, min(z, w)));}
inline Vec2  Min(Vec2  x, Vec2  y, Vec2  z, Vec2  w) {return min(x, min(y, min(z, w)));}
inline VecH  Min(VecH  x, VecH  y, VecH  z, VecH  w) {return min(x, min(y, min(z, w)));}
inline Vec   Min(Vec   x, Vec   y, Vec   z, Vec   w) {return min(x, min(y, min(z, w)));}
inline VecH4 Min(VecH4 x, VecH4 y, VecH4 z, VecH4 w) {return min(x, min(y, min(z, w)));}
inline Vec4  Min(Vec4  x, Vec4  y, Vec4  z, Vec4  w) {return min(x, min(y, min(z, w)));}

inline Int   Max(Int   x, Int   y                  ) {return max(x, y);}
inline Half  Max(Half  x, Half  y                  ) {return max(x, y);}
inline Flt   Max(Flt   x, Flt   y                  ) {return max(x, y);}
inline Flt   Max(Int   x, Flt   y                  ) {return max(x, y);}
inline Flt   Max(Flt   x, Int   y                  ) {return max(x, y);}
inline VecH2 Max(VecH2 x, VecH2 y                  ) {return max(x, y);}
inline Vec2  Max(Vec2  x, Vec2  y                  ) {return max(x, y);}
inline VecH  Max(VecH  x, VecH  y                  ) {return max(x, y);}
inline Vec   Max(Vec   x, Vec   y                  ) {return max(x, y);}
inline VecH4 Max(VecH4 x, VecH4 y                  ) {return max(x, y);}
inline Vec4  Max(Vec4  x, Vec4  y                  ) {return max(x, y);}
inline Int   Max(Int   x, Int   y, Int   z         ) {return max(x, max(y, z));}
inline Half  Max(Half  x, Half  y, Half  z         ) {return max(x, max(y, z));}
inline Flt   Max(Flt   x, Flt   y, Flt   z         ) {return max(x, max(y, z));}
inline VecH2 Max(VecH2 x, VecH2 y, VecH2 z         ) {return max(x, max(y, z));}
inline Vec2  Max(Vec2  x, Vec2  y, Vec2  z         ) {return max(x, max(y, z));}
inline VecH  Max(VecH  x, VecH  y, VecH  z         ) {return max(x, max(y, z));}
inline Vec   Max(Vec   x, Vec   y, Vec   z         ) {return max(x, max(y, z));}
inline VecH4 Max(VecH4 x, VecH4 y, VecH4 z         ) {return max(x, max(y, z));}
inline Vec4  Max(Vec4  x, Vec4  y, Vec4  z         ) {return max(x, max(y, z));}
inline Int   Max(Int   x, Int   y, Int   z, Int   w) {return max(x, max(y, max(z, w)));}
inline Half  Max(Half  x, Half  y, Half  z, Half  w) {return max(x, max(y, max(z, w)));}
inline Flt   Max(Flt   x, Flt   y, Flt   z, Flt   w) {return max(x, max(y, max(z, w)));}
inline VecH2 Max(VecH2 x, VecH2 y, VecH2 z, VecH2 w) {return max(x, max(y, max(z, w)));}
inline Vec2  Max(Vec2  x, Vec2  y, Vec2  z, Vec2  w) {return max(x, max(y, max(z, w)));}
inline VecH  Max(VecH  x, VecH  y, VecH  z, VecH  w) {return max(x, max(y, max(z, w)));}
inline Vec   Max(Vec   x, Vec   y, Vec   z, Vec   w) {return max(x, max(y, max(z, w)));}
inline VecH4 Max(VecH4 x, VecH4 y, VecH4 z, VecH4 w) {return max(x, max(y, max(z, w)));}
inline Vec4  Max(Vec4  x, Vec4  y, Vec4  z, Vec4  w) {return max(x, max(y, max(z, w)));}

inline Half  Avg(Half  x, Half  y                  ) {return (x+y    )/2   ;}
inline Flt   Avg(Flt   x, Flt   y                  ) {return (x+y    )*0.50;}
inline VecH2 Avg(VecH2 x, VecH2 y                  ) {return (x+y    )/2   ;}
inline Vec2  Avg(Vec2  x, Vec2  y                  ) {return (x+y    )*0.50;}
inline VecH  Avg(VecH  x, VecH  y                  ) {return (x+y    )/2   ;}
inline Vec   Avg(Vec   x, Vec   y                  ) {return (x+y    )*0.50;}
inline VecH4 Avg(VecH4 x, VecH4 y                  ) {return (x+y    )/2   ;}
inline Vec4  Avg(Vec4  x, Vec4  y                  ) {return (x+y    )*0.50;}
inline Half  Avg(Half  x, Half  y, Half  z         ) {return (x+y+z  )/3   ;}
inline Flt   Avg(Flt   x, Flt   y, Flt   z         ) {return (x+y+z  )/3.00;}
inline VecH2 Avg(VecH2 x, VecH2 y, VecH2 z         ) {return (x+y+z  )/3   ;}
inline Vec2  Avg(Vec2  x, Vec2  y, Vec2  z         ) {return (x+y+z  )/3.00;}
inline VecH  Avg(VecH  x, VecH  y, VecH  z         ) {return (x+y+z  )/3   ;}
inline Vec   Avg(Vec   x, Vec   y, Vec   z         ) {return (x+y+z  )/3.00;}
inline VecH4 Avg(VecH4 x, VecH4 y, VecH4 z         ) {return (x+y+z  )/3   ;}
inline Vec4  Avg(Vec4  x, Vec4  y, Vec4  z         ) {return (x+y+z  )/3.00;}
inline Half  Avg(Half  x, Half  y, Half  z, Half  w) {return (x+y+z+w)/4   ;}
inline Flt   Avg(Flt   x, Flt   y, Flt   z, Flt   w) {return (x+y+z+w)*0.25;}
inline VecH2 Avg(VecH2 x, VecH2 y, VecH2 z, VecH2 w) {return (x+y+z+w)/4   ;}
inline Vec2  Avg(Vec2  x, Vec2  y, Vec2  z, Vec2  w) {return (x+y+z+w)*0.25;}
inline VecH  Avg(VecH  x, VecH  y, VecH  z, VecH  w) {return (x+y+z+w)/4   ;}
inline Vec   Avg(Vec   x, Vec   y, Vec   z, Vec   w) {return (x+y+z+w)*0.25;}
inline VecH4 Avg(VecH4 x, VecH4 y, VecH4 z, VecH4 w) {return (x+y+z+w)/4   ;}
inline Vec4  Avg(Vec4  x, Vec4  y, Vec4  z, Vec4  w) {return (x+y+z+w)*0.25;}

inline Half Min(VecH2 v) {return Min(v.x, v.y);}
inline Half Max(VecH2 v) {return Max(v.x, v.y);}
inline Half Avg(VecH2 v) {return Avg(v.x, v.y);}
inline Flt  Min(Vec2  v) {return Min(v.x, v.y);}
inline Flt  Max(Vec2  v) {return Max(v.x, v.y);}
inline Flt  Avg(Vec2  v) {return Avg(v.x, v.y);}
inline Half Min(VecH  v) {return Min(v.x, v.y, v.z);}
inline Half Max(VecH  v) {return Max(v.x, v.y, v.z);}
inline Half Avg(VecH  v) {return Avg(v.x, v.y, v.z);}
inline Flt  Min(Vec   v) {return Min(v.x, v.y, v.z);}
inline Flt  Max(Vec   v) {return Max(v.x, v.y, v.z);}
inline Flt  Avg(Vec   v) {return Avg(v.x, v.y, v.z);}
inline Half Min(VecH4 v) {return Min(v.x, v.y, v.z, v.w);}
inline Half Max(VecH4 v) {return Max(v.x, v.y, v.z, v.w);}
inline Half Avg(VecH4 v) {return Avg(v.x, v.y, v.z, v.w);}
inline Flt  Min(Vec4  v) {return Min(v.x, v.y, v.z, v.w);}
inline Flt  Max(Vec4  v) {return Max(v.x, v.y, v.z, v.w);}
inline Flt  Avg(Vec4  v) {return Avg(v.x, v.y, v.z, v.w);}

inline Half Sum(VecH2 v) {return v.x+v.y        ;}
inline Flt  Sum(Vec2  v) {return v.x+v.y        ;}
inline Half Sum(VecH  v) {return v.x+v.y+v.z    ;}
inline Flt  Sum(Vec   v) {return v.x+v.y+v.z    ;}
inline Half Sum(VecH4 v) {return v.x+v.y+v.z+v.w;}
inline Flt  Sum(Vec4  v) {return v.x+v.y+v.z+v.w;}

inline Int   Sqr(Int   x) {return x*x;}
inline Half  Sqr(Half  x) {return x*x;}
inline Flt   Sqr(Flt   x) {return x*x;}
inline VecH2 Sqr(VecH2 x) {return x*x;}
inline Vec2  Sqr(Vec2  x) {return x*x;}
inline VecH  Sqr(VecH  x) {return x*x;}
inline Vec   Sqr(Vec   x) {return x*x;}
inline VecH4 Sqr(VecH4 x) {return x*x;}
inline Vec4  Sqr(Vec4  x) {return x*x;}

inline Int   Cube(Int   x) {return x*x*x;}
inline Half  Cube(Half  x) {return x*x*x;}
inline Flt   Cube(Flt   x) {return x*x*x;}
inline VecH2 Cube(VecH2 x) {return x*x*x;}
inline Vec2  Cube(Vec2  x) {return x*x*x;}
inline VecH  Cube(VecH  x) {return x*x*x;}
inline Vec   Cube(Vec   x) {return x*x*x;}
inline VecH4 Cube(VecH4 x) {return x*x*x;}
inline Vec4  Cube(Vec4  x) {return x*x*x;}

inline Int   Quart(Int   x) {return Sqr(x*x);}
inline Half  Quart(Half  x) {return Sqr(x*x);}
inline Flt   Quart(Flt   x) {return Sqr(x*x);}
inline VecH2 Quart(VecH2 x) {return Sqr(x*x);}
inline Vec2  Quart(Vec2  x) {return Sqr(x*x);}
inline VecH  Quart(VecH  x) {return Sqr(x*x);}
inline Vec   Quart(Vec   x) {return Sqr(x*x);}
inline VecH4 Quart(VecH4 x) {return Sqr(x*x);}
inline Vec4  Quart(Vec4  x) {return Sqr(x*x);}

inline Int   Quint(Int   x) {return Quart(x)*x;}
inline Half  Quint(Half  x) {return Quart(x)*x;}
inline Flt   Quint(Flt   x) {return Quart(x)*x;}
inline VecH2 Quint(VecH2 x) {return Quart(x)*x;}
inline Vec2  Quint(Vec2  x) {return Quart(x)*x;}
inline VecH  Quint(VecH  x) {return Quart(x)*x;}
inline Vec   Quint(Vec   x) {return Quart(x)*x;}
inline VecH4 Quint(VecH4 x) {return Quart(x)*x;}
inline Vec4  Quint(Vec4  x) {return Quart(x)*x;}

inline Half Length(VecH2 v) {return length(v);}
inline Flt  Length(Vec2  v) {return length(v);}
inline Half Length(VecH  v) {return length(v);}
inline Flt  Length(Vec   v) {return length(v);}
inline Half Length(VecH4 v) {return length(v);}
inline Flt  Length(Vec4  v) {return length(v);}

inline Half Length2(VecH2 v) {return Dot(v, v);}
inline Flt  Length2(Vec2  v) {return Dot(v, v);}
inline Half Length2(VecH  v) {return Dot(v, v);}
inline Flt  Length2(Vec   v) {return Dot(v, v);}
inline Half Length2(VecH4 v) {return Dot(v, v);}
inline Flt  Length2(Vec4  v) {return Dot(v, v);}

inline Flt  Dist(Int   a, Int   b) {return Sqrt(Flt(a*a + b*b));}
inline Half Dist(Half  a, Half  b) {return Sqrt(    a*a + b*b );}
inline Flt  Dist(Flt   a, Flt   b) {return Sqrt(    a*a + b*b );}
inline Half Dist(VecH2 a, VecH2 b) {return distance(a, b );}
inline Flt  Dist(Vec2  a, Vec2  b) {return distance(a, b );}
inline Half Dist(VecH  a, VecH  b) {return distance(a, b );}
inline Flt  Dist(Vec   a, Vec   b) {return distance(a, b );}

inline Half DistH(Int a, Int b) {return Sqrt(Half(a*a + b*b));}

inline Int  Dist2(Int   a, Int   b) {return a*a + b*b;}
inline Half Dist2(Half  a, Half  b) {return a*a + b*b;}
inline Flt  Dist2(Flt   a, Flt   b) {return a*a + b*b;}
inline Half Dist2(VecH2 a, VecH2 b) {return Length2(a-b);}
inline Flt  Dist2(Vec2  a, Vec2  b) {return Length2(a-b);}
inline Half Dist2(VecH  a, VecH  b) {return Length2(a-b);}
inline Flt  Dist2(Vec   a, Vec   b) {return Length2(a-b);}
inline Half Dist2(VecH4 a, VecH4 b) {return Length2(a-b);}
inline Flt  Dist2(Vec4  a, Vec4  b) {return Length2(a-b);}

inline Half DistPointPlane(VecH2 pos,                  VecH2 plane_normal) {return Dot(pos          , plane_normal);}
inline Flt  DistPointPlane(Vec2  pos,                  Vec2  plane_normal) {return Dot(pos          , plane_normal);}
inline Half DistPointPlane(VecH  pos,                  VecH  plane_normal) {return Dot(pos          , plane_normal);}
inline Flt  DistPointPlane(Vec   pos,                  Vec   plane_normal) {return Dot(pos          , plane_normal);}
inline Half DistPointPlane(VecH2 pos, VecH2 plane_pos, VecH2 plane_normal) {return Dot(pos-plane_pos, plane_normal);}
inline Flt  DistPointPlane(Vec2  pos, Vec2  plane_pos, Vec2  plane_normal) {return Dot(pos-plane_pos, plane_normal);}
inline Half DistPointPlane(VecH  pos, VecH  plane_pos, VecH  plane_normal) {return Dot(pos-plane_pos, plane_normal);}
inline Flt  DistPointPlane(Vec   pos, Vec   plane_pos, Vec   plane_normal) {return Dot(pos-plane_pos, plane_normal);}

inline VecH2 PointOnPlane(VecH2 pos,                  VecH2 plane_normal) {return pos-plane_normal*DistPointPlane(pos,            plane_normal);}
inline Vec2  PointOnPlane(Vec2  pos,                  Vec2  plane_normal) {return pos-plane_normal*DistPointPlane(pos,            plane_normal);}
inline VecH  PointOnPlane(VecH  pos,                  VecH  plane_normal) {return pos-plane_normal*DistPointPlane(pos,            plane_normal);}
inline Vec   PointOnPlane(Vec   pos,                  Vec   plane_normal) {return pos-plane_normal*DistPointPlane(pos,            plane_normal);}
inline VecH2 PointOnPlane(VecH2 pos, VecH2 plane_pos, VecH2 plane_normal) {return pos-plane_normal*DistPointPlane(pos, plane_pos, plane_normal);}
inline Vec2  PointOnPlane(Vec2  pos, Vec2  plane_pos, Vec2  plane_normal) {return pos-plane_normal*DistPointPlane(pos, plane_pos, plane_normal);}
inline VecH  PointOnPlane(VecH  pos, VecH  plane_pos, VecH  plane_normal) {return pos-plane_normal*DistPointPlane(pos, plane_pos, plane_normal);}
inline Vec   PointOnPlane(Vec   pos, Vec   plane_pos, Vec   plane_normal) {return pos-plane_normal*DistPointPlane(pos, plane_pos, plane_normal);}

inline Half Angle (Half  x, Half y) {return atan2(  y,   x);}
inline Flt  Angle (Flt   x, Flt  y) {return atan2(  y,   x);}
inline Half Angle (VecH2 v        ) {return atan2(v.y, v.x);}
inline Flt  Angle (Vec2  v        ) {return atan2(v.y, v.x);}
inline Half CosSin(Half  cs       ) {return Sqrt (1-cs*cs );} // NaN
inline Flt  CosSin(Flt   cs       ) {return Sqrt (1-cs*cs );} // NaN
inline void CosSin(out Half cos, out Half sin, Half angle) {sincos(angle, sin, cos);}
inline void CosSin(out Flt  cos, out Flt  sin, Flt  angle) {sincos(angle, sin, cos);}

inline Half Cross2D(VecH2 a, VecH2 b) {return a.x*b.y - a.y*b.x;}
inline Flt  Cross2D(Vec2  a, Vec2  b) {return a.x*b.y - a.y*b.x;}

inline Half CalcZ(VecH2 v) {return Sqrt(Sat(1-Dot(v, v)));} // 1 - v.x*v.x - v.y*v.y, NaN
inline Flt  CalcZ(Vec2  v) {return Sqrt(Sat(1-Dot(v, v)));} // 1 - v.x*v.x - v.y*v.y, NaN

inline Half SignFast(Half x) {return (x>=0) ? 1 : -1;} // ignores 0
inline Flt  SignFast(Flt  x) {return (x>=0) ? 1 : -1;} // ignores 0

inline Half AngleFull     (Half  angle         ) {return Frac(angle/PI2         )*PI2   ;} // normalize angle to   0..PI2
inline Flt  AngleFull     (Flt   angle         ) {return Frac(angle/PI2         )*PI2   ;} // normalize angle to   0..PI2
inline Half AngleNormalize(Half  angle         ) {return Frac(angle/PI2 + PI/PI2)*PI2-PI;} // normalize angle to -PI..PI
inline Flt  AngleNormalize(Flt   angle         ) {return Frac(angle/PI2 + PI/PI2)*PI2-PI;} // normalize angle to -PI..PI
inline Half AngleDelta    (Half  from, Half  to) {return AngleNormalize(to-from)        ;} // get angle delta    -PI..PI
inline Flt  AngleDelta    (Flt   from, Flt   to) {return AngleNormalize(to-from)        ;} // get angle delta    -PI..PI
inline Half AngleBetween  (VecH2 a   , VecH2 b ) {return AngleDelta(Angle(a), Angle(b)) ;}
inline Flt  AngleBetween  (Vec2  a   , Vec2  b ) {return AngleDelta(Angle(a), Angle(b)) ;}

inline VecH2 Perp(VecH2 vec) {return VecH2(vec.y, -vec.x);} // get perpendicular vector
inline Vec2  Perp(Vec2  vec) {return Vec2 (vec.y, -vec.x);} // get perpendicular vector

inline VecH2 Rotate(VecH2 vec, VecH2 cos_sin) // rotate vector by cos and sin values obtained from a custom angle
{
   return VecH2(vec.x*cos_sin.x - vec.y*cos_sin.y,
                vec.x*cos_sin.y + vec.y*cos_sin.x);
}
inline Vec2 Rotate(Vec2 vec, Vec2 cos_sin) // rotate vector by cos and sin values obtained from a custom angle
{
   return Vec2(vec.x*cos_sin.x - vec.y*cos_sin.y,
               vec.x*cos_sin.y + vec.y*cos_sin.x);
}

inline Half LerpR (Half from, Half to, Half v) {return     (v-from)/(to-from) ;}
inline Flt  LerpR (Flt  from, Flt  to, Flt  v) {return     (v-from)/(to-from) ;}
inline Half LerpRS(Half from, Half to, Half v) {return Sat((v-from)/(to-from));}
inline Flt  LerpRS(Flt  from, Flt  to, Flt  v) {return Sat((v-from)/(to-from));}
/******************************************************************************/
#if 1 // faster (1.6 fps) tested on GeForce 1050 Ti
inline Vec  Transform(Vec  v, Matrix3  m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2]));} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform(VecH v, MatrixH3 m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2]));} // transform 'v' vector by 'm' orientation-scale matrix
#else // slower (1.0 fps)
inline Vec  Transform(Vec  v, Matrix3  m) {return mul(v, m);} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform(VecH v, MatrixH3 m) {return mul(v, m);} // transform 'v' vector by 'm' orientation-scale matrix
#endif

#if 1 // was faster on GeForce 650m, but on GeForce 1050 Ti performance is the same, however keep this version as in other cases 'mul' is slower
inline Vec  Transform(Vec  v, Matrix  m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2] + m[3]));} // transform 'v' vector by 'm' orientation-scale-translation matrix, faster version of "mul(Vec4 (v, 1), m)"
inline VecH Transform(VecH v, Matrix  m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2] + m[3]));} // transform 'v' vector by 'm' orientation-scale-translation matrix, faster version of "mul(Vec4 (v, 1), m)"
inline VecH Transform(VecH v, MatrixH m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2] + m[3]));} // transform 'v' vector by 'm' orientation-scale-translation matrix, faster version of "mul(VecH4(v, 1), m)"
inline Vec4 Transform(Vec  v, Matrix4 m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2] + m[3]));} // transform 'v' vector by 'm' 4x4                           matrix, faster version of "mul(Vec4 (v, 1), m)"
#else
inline Vec  Transform(Vec  v, Matrix  m) {return mul(Vec4 (v, 1), m);} // transform 'v' vector by 'm' orientation-scale-translation matrix
inline VecH Transform(VecH v, Matrix  m) {return mul(VecH4(v, 1), m);} // transform 'v' vector by 'm' orientation-scale-translation matrix
inline VecH Transform(VecH v, MatrixH m) {return mul(VecH4(v, 1), m);} // transform 'v' vector by 'm' orientation-scale-translation matrix
inline Vec4 Transform(Vec  v, Matrix4 m) {return mul(Vec4 (v, 1), m);} // transform 'v' vector by 'm' 4x4                           matrix
#endif

#if 1 // faster (1.6 fps) tested on GeForce 1050 Ti
inline Vec  Transform3(Vec  v, Matrix  m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2]));} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform3(VecH v, MatrixH m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2]));} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform3(VecH v, Matrix  m) {return v.x*m[0] + (v.y*m[1] + (v.z*m[2]));} // transform 'v' vector by 'm' orientation-scale matrix, TODO: #ShaderHalf would it be faster to cast 'v' to 'Vec' first? Mixing precisions is not perfect however alternative would require to store matrixes in additional half precision but that would slow down (calculating on CPU side and uploading to GPU)
#else // slower (1.0 fps)
inline Vec  Transform3(Vec  v, Matrix  m) {return mul(v, (Matrix3 )m);} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform3(VecH v, MatrixH m) {return mul(v, (MatrixH3)m);} // transform 'v' vector by 'm' orientation-scale matrix
inline VecH Transform3(VecH v, Matrix  m) {return mul(v, (MatrixH3)m);} // transform 'v' vector by 'm' orientation-scale matrix
#endif

#if 1 // faster 4.3 fps
inline Vec  TransformTP(Vec  v, Matrix3  m) {return mul(m, v);} // transform 'v' vector by transposed 'm' orientation-scale matrix
inline VecH TransformTP(VecH v, MatrixH3 m) {return mul(m, v);} // transform 'v' vector by transposed 'm' orientation-scale matrix
inline Vec  TransformTP(Vec  v, MatrixH3 m) {return mul(m, v);} // transform 'v' vector by transposed 'm' orientation-scale matrix
#else // slower 3.2 fps
inline Vec  TransformTP(Vec  v, Matrix3  m) {return Vec(Dot(v, m[0]), Dot(v, m[1]), Dot(v, m[2]));} // transform 'v' vector by transposed 'm' orientation-scale matrix
inline VecH TransformTP(VecH v, MatrixH3 m) {return Vec(Dot(v, m[0]), Dot(v, m[1]), Dot(v, m[2]));} // transform 'v' vector by transposed 'm' orientation-scale matrix
inline Vec  TransformTP(Vec  v, MatrixH3 m) {return Vec(Dot(v, m[0]), Dot(v, m[1]), Dot(v, m[2]));} // transform 'v' vector by transposed 'm' orientation-scale matrix
#endif
/******************************************************************************/
#if !GL // FIXME broken for Mac - https://feedbackassistant.apple.com/feedback/7116525
#include "!Set Prec Struct.h"
BUFFER_I(ObjMatrix, SBI_OBJ_MATRIX) // !! WARNING: this CB is dynamically resized, do not add other members !!
   Matrix ViewMatrix[MAX_MATRIX]; // object transformation matrixes relative to view space (this is object matrix * inversed camera matrix = object matrix / camera matrix)
BUFFER_END
#include "!Set Prec Default.h"

inline Vec  TransformPos(Vec  pos, UInt mtrx=0) {return Transform (pos, ViewMatrix[mtrx]);}
inline VecH TransformDir(VecH dir, UInt mtrx=0) {return Transform3(dir, ViewMatrix[mtrx]);}

inline Vec  TransformPos(Vec  pos, VecU bone, Vec  weight) {return weight.x*Transform (pos, ViewMatrix[bone.x]) + weight.y*Transform (pos, ViewMatrix[bone.y]) + weight.z*Transform (pos, ViewMatrix[bone.z]);}
inline VecH TransformDir(VecH dir, VecU bone, VecH weight) {return weight.x*Transform3(dir, ViewMatrix[bone.x]) + weight.y*Transform3(dir, ViewMatrix[bone.y]) + weight.z*Transform3(dir, ViewMatrix[bone.z]);}

inline Vec ViewMatrixX  (UInt mtrx=0) {return ViewMatrix[mtrx][0];}
inline Vec ViewMatrixY  (UInt mtrx=0) {return ViewMatrix[mtrx][1];}
inline Vec ViewMatrixZ  (UInt mtrx=0) {return ViewMatrix[mtrx][2];}
inline Vec ViewMatrixPos(UInt mtrx=0) {return ViewMatrix[mtrx][3];}

inline Matrix GetViewMatrix() {return ViewMatrix[0];}
#else // Mac currently has no known workaround and must use this. Arm Mali has a bug on Android GL ES, where it expects 4xVec4 array stride for 'Matrix' - https://community.arm.com/developer/tools-software/graphics/f/discussions/43743/serious-problems-with-handling-of-mat4x3 however a workaround was found to declare layout additionally for struct instead of just member.
#include "!Set Prec Struct.h"
BUFFER_I(ObjMatrix, SBI_OBJ_MATRIX) // !! WARNING: this CB is dynamically resized, do not add other members !!
   Vec4 ViewMatrix[MAX_MATRIX*3]; // object transformation matrixes relative to view space (this is object matrix * inversed camera matrix = object matrix / camera matrix)
BUFFER_END
#include "!Set Prec Default.h"

inline Vec TransformPos(Vec pos)
{
   return Vec(Dot(pos, ViewMatrix[0].xyz) + ViewMatrix[0].w,
              Dot(pos, ViewMatrix[1].xyz) + ViewMatrix[1].w,
              Dot(pos, ViewMatrix[2].xyz) + ViewMatrix[2].w);
}
inline VecH TransformDir(VecH dir)
{
   return VecH(Dot(dir, ViewMatrix[0].xyz),
               Dot(dir, ViewMatrix[1].xyz),
               Dot(dir, ViewMatrix[2].xyz));
}

inline Vec TransformPos(Vec pos, UInt mtrx)
{
   mtrx*=3;
   return Vec(Dot(pos, ViewMatrix[mtrx  ].xyz) + ViewMatrix[mtrx  ].w,
              Dot(pos, ViewMatrix[mtrx+1].xyz) + ViewMatrix[mtrx+1].w,
              Dot(pos, ViewMatrix[mtrx+2].xyz) + ViewMatrix[mtrx+2].w);
}
inline VecH TransformDir(VecH dir, UInt mtrx)
{
   mtrx*=3;
   return VecH(Dot(dir, ViewMatrix[mtrx  ].xyz),
               Dot(dir, ViewMatrix[mtrx+1].xyz),
               Dot(dir, ViewMatrix[mtrx+2].xyz));
}

inline Vec  TransformPos(Vec  pos, VecU bone, Vec  weight) {return weight.x*TransformPos(pos, bone.x) + weight.y*TransformPos(pos, bone.y) + weight.z*TransformPos(pos, bone.z);}
inline VecH TransformDir(VecH dir, VecU bone, VecH weight) {return weight.x*TransformDir(dir, bone.x) + weight.y*TransformDir(dir, bone.y) + weight.z*TransformDir(dir, bone.z);}

inline Vec ViewMatrixX  () {return Vec(ViewMatrix[0].x, ViewMatrix[1].x, ViewMatrix[2].x);}
inline Vec ViewMatrixY  () {return Vec(ViewMatrix[0].y, ViewMatrix[1].y, ViewMatrix[2].y);}
inline Vec ViewMatrixZ  () {return Vec(ViewMatrix[0].z, ViewMatrix[1].z, ViewMatrix[2].z);}
inline Vec ViewMatrixPos() {return Vec(ViewMatrix[0].w, ViewMatrix[1].w, ViewMatrix[2].w);}

inline Vec ViewMatrixY  (UInt mtrx) {mtrx*=3; return Vec(ViewMatrix[mtrx].y, ViewMatrix[mtrx+1].y, ViewMatrix[mtrx+2].y);}
inline Vec ViewMatrixPos(UInt mtrx) {mtrx*=3; return Vec(ViewMatrix[mtrx].w, ViewMatrix[mtrx+1].w, ViewMatrix[mtrx+2].w);}

inline Matrix GetViewMatrix() {Matrix m; m[0]=ViewMatrixX(); m[1]=ViewMatrixY(); m[2]=ViewMatrixZ(); m[3]=ViewMatrixPos(); return m;}
#endif
/******************************************************************************/
// VELOCITIES
/******************************************************************************/
inline VecH GetVel(VecH local_pos, UInt mtrx=0) // does not include velocity from Camera Angle Vel
{
   return             ObjVel[mtrx].lin
  +TransformDir(Cross(ObjVel[mtrx].ang, local_pos), mtrx);
}
inline Vec GetCamAngVel(Vec view_space_pos)
{
   return -Cross(CamAngVel, view_space_pos);
}
inline Vec GetObjVel(VecH local_pos, Vec view_space_pos, UInt mtrx=0)
{
   return GetVel      (local_pos, mtrx)
         +GetCamAngVel(view_space_pos);
}
inline Vec GetBoneVel(VecH local_pos, Vec view_space_pos, VecU bone, VecH weight)
{
   return (GetVel      (local_pos, bone.x)*weight.x
          +GetVel      (local_pos, bone.y)*weight.y
          +GetVel      (local_pos, bone.z)*weight.z)
          +GetCamAngVel(view_space_pos);
}
/******************************************************************************/
inline VecH GetVelocity_PS(Vec vel, Vec view_space_pos)
{
   // divide by distance to camera (there is no NaN because in PixelShader view_space_pos.z>0)
 //if(ORTHO_SUPPORT && !Viewport.ortho)
   VecH vel_ps=vel/view_space_pos.z;

#if !SIGNED_VEL_RT
   vel_ps=vel_ps*0.5+0.5; // scale from signed to unsigned (-1..1 -> 0..1)
#endif

   return vel_ps;
}
inline Vec GetVelocitiesCameraOnly(Vec view_space_pos)
{
   Vec vel=ObjVel[0].lin // set to object linear velocity in view space
          +GetCamAngVel(view_space_pos); // add camera angular velocity

   // divide by distance to camera (there is no NaN because in PixelShader view_space_pos.z>0)
 //if(ORTHO_SUPPORT && !Viewport.ortho)
      vel/=view_space_pos.z;

   vel=Mid(vel, Vec(-1, -1, -1), Vec(1, 1, 1)); // clamp to get the same results as if stored in a RT

   return vel; // this function always returns signed -1..1 version
}
/******************************************************************************/
inline Vec4 Project(Vec pos)
{
#if 1 // 2x faster on Intel (made no difference for GeForce)
   return Vec4(pos.x*ProjMatrix[0].x + pos.z*ProjMatrix[2].x, pos.y*ProjMatrix[1].y, pos.z*ProjMatrix[2].z + ProjMatrix[3].z, pos.z*ProjMatrix[2].w + ProjMatrix[3].w);
#else // slower
   return Transform(pos, ProjMatrix);
#endif
}
/******************************************************************************/
inline Vec  MatrixX(Matrix3  m) {return m[0];}
inline VecH MatrixX(MatrixH3 m) {return m[0];}
inline Vec  MatrixY(Matrix3  m) {return m[1];}
inline VecH MatrixY(MatrixH3 m) {return m[1];}
inline Vec  MatrixZ(Matrix3  m) {return m[2];}
inline VecH MatrixZ(MatrixH3 m) {return m[2];}

inline Vec  MatrixX  (Matrix  m) {return m[0];}
inline VecH MatrixX  (MatrixH m) {return m[0];}
inline Vec  MatrixY  (Matrix  m) {return m[1];}
inline VecH MatrixY  (MatrixH m) {return m[1];}
inline Vec  MatrixZ  (Matrix  m) {return m[2];}
inline VecH MatrixZ  (MatrixH m) {return m[2];}
inline Vec  MatrixPos(Matrix  m) {return m[3];}
inline VecH MatrixPos(MatrixH m) {return m[3];}

inline Vec ObjWorldPos(UInt mtrx=0) {return Transform(ViewMatrixPos(mtrx), CamMatrix);} // get the world position of the object matrix
/******************************************************************************/
inline Vec2 UVClamp(Vec2 screen, Bool do_clamp=true)
{
   return do_clamp ? Mid(screen, ImgClamp.xy, ImgClamp.zw) : screen;
}
/******************************************************************************/
inline Vec2 FracToPosXY(Vec2 frac) // return view space xy position at z=1
{
   return frac * Viewport.FracToPosXY.xy + Viewport.FracToPosXY.zw;
}
inline Vec2 ScreenToPosXY(Vec2 screen) // return view space xy position at z=1
{
   return screen * Viewport.ScreenToPosXY.xy + Viewport.ScreenToPosXY.zw;
}
inline Vec2 ScreenToPosXY(Vec2 screen, Flt z) // return view space xy position at z='z'
{
   return ScreenToPosXY(screen)*z;
}
/******************************************************************************/
inline Vec2 PosToScreen(Vec4 pos) // prefer using 'PixelToScreen' if possible, returns (0,0)..(1,1) range
{
   return (pos.xy/pos.w) * Viewport.PosToScreen.xy + Viewport.PosToScreen.zw;
}
inline Vec2 PixelToScreen(Vec4 pixel) // faster and more accurate than 'PosToScreen', returns (0,0)..(1,1) range
{
   return pixel.xy*RTSize.xy;
}
/******************************************************************************/
// DEPTH
/******************************************************************************/
inline Flt DelinearizeDepth(Flt z, Bool perspective=true)
{
   Flt a=ProjMatrix[2][2], // ProjMatrix.z  .z
       b=ProjMatrix[3][2], // ProjMatrix.pos.z
       w=(perspective ? (z*a+b)/z : (z*a+REVERSE_DEPTH));

#if GL
   w=w*0.5+0.5; // #glClipControl
#endif

   return w;
}
inline Flt LinearizeDepth(Flt w, Bool perspective=true)
{
   Flt a=ProjMatrix[2][2], // ProjMatrix.z  .z
       b=ProjMatrix[3][2]; // ProjMatrix.pos.z

#if GL
   w=w*2-1; // #glClipControl
#endif

   // w   = (z*a+b)/z
   // w*z = a*z + b
   // a*z - w*z + b = 0
   // (a-w)*z + b = 0
   // z = -b/(a-w)
   // z =  b/(w-a)
   // precise: if(perspective && w>=1)return Viewport.range;
   return perspective ? b/(w-a) : (w-REVERSE_DEPTH)/a;
}
/******************************************************************************/
// Perspective: pos.xy=pos_xy*z
// Orthogonal : pos.xy=pos_xy
inline Vec GetPos(Flt z, Vec2 pos_xy) // Get Viewspace Position at 'z' depth, 'pos_xy'=known xy position at depth=1
{
   Vec pos;         pos.z =z;
 //if(ORTHO_SUPPORT)pos.xy=pos_xy*(Viewport.ortho ? 1 : pos.z);else
                    pos.xy=pos_xy*pos.z;
   return pos;
}
inline Vec GetPosPoint (Vec2 tex             ) {return GetPos(TexDepthPoint (tex), ScreenToPosXY(tex));} // Get Viewspace Position at 'tex' screen coordinates
inline Vec GetPosPoint (Vec2 tex, Vec2 pos_xy) {return GetPos(TexDepthPoint (tex), pos_xy            );} // Get Viewspace Position at 'tex' screen coordinates, 'pos_xy'=known xy position at depth=1
inline Vec GetPosLinear(Vec2 tex             ) {return GetPos(TexDepthLinear(tex), ScreenToPosXY(tex));} // Get Viewspace Position at 'tex' screen coordinates
inline Vec GetPosLinear(Vec2 tex, Vec2 pos_xy) {return GetPos(TexDepthLinear(tex), pos_xy            );} // Get Viewspace Position at 'tex' screen coordinates, 'pos_xy'=known xy position at depth=1

inline Vec GetPosMS(VecI2 pixel, UInt sample, Vec2 pos_xy) {return GetPos(TexDepthMS(pixel, sample), pos_xy);}
/******************************************************************************/
// sRGB
/******************************************************************************/
#define SRGBToLinearFast Sqr  // simple and fast sRGB -> Linear conversion
#define LinearToSRGBFast Sqrt // simple and fast Linear -> sRGB conversion

Half SRGBToLinear(Half s) {return (s<=0.04045  ) ? s/12.92 : Pow((s+0.055)/1.055, 2.4);} // convert 0..1 srgb   to 0..1 linear
Half LinearToSRGB(Half l) {return (l<=0.0031308) ? l*12.92 : Pow(l, 1/2.4)*1.055-0.055;} // convert 0..1 linear to 0..1 srgb

VecH2 SRGBToLinear(VecH2 s) {return VecH2(SRGBToLinear(s.x), SRGBToLinear(s.y));}
VecH2 LinearToSRGB(VecH2 l) {return VecH2(LinearToSRGB(l.x), LinearToSRGB(l.y));}

VecH SRGBToLinear(VecH s) {return VecH(SRGBToLinear(s.x), SRGBToLinear(s.y), SRGBToLinear(s.z));}
VecH LinearToSRGB(VecH l) {return VecH(LinearToSRGB(l.x), LinearToSRGB(l.y), LinearToSRGB(l.z));}

VecH4 SRGBToLinear(VecH4 s) {return VecH4(SRGBToLinear(s.x), SRGBToLinear(s.y), SRGBToLinear(s.z), s.w);}
VecH4 LinearToSRGB(VecH4 l) {return VecH4(LinearToSRGB(l.x), LinearToSRGB(l.y), LinearToSRGB(l.z), l.w);}

Half LinearLumOfLinearColor(VecH l) {return                  Dot(                 l , ColorLumWeight2) ;}
Half LinearLumOfSRGBColor  (VecH s) {return                  Dot(SRGBToLinearFast(s), ColorLumWeight2) ;}
Half   SRGBLumOfSRGBColor  (VecH s) {return LinearToSRGBFast(Dot(SRGBToLinearFast(s), ColorLumWeight2));}
/******************************************************************************/
struct VtxInput // Vertex Input, use this class to access vertex data in vertex shaders
{
#include "!Set Prec Struct.h"
#if GL
   // !! LOC, ATTR numbers AND list order, must be in sync with GL_VTX_SEMANTIC !!
   LOC( 0) Vec4  _pos     :ATTR0 ;
   LOC( 1) VecH  _hlp     :ATTR1 ;
   LOC( 2) VecH  _nrm     :ATTR2 ;
   LOC( 3) VecH4 _tan     :ATTR3 ;
   LOC( 4) Vec2  _tex     :ATTR4 ;
   LOC( 5) Vec2  _tex1    :ATTR5 ;
   LOC( 6) Vec2  _tex2    :ATTR6 ;
   LOC( 7) Half  _size    :ATTR7 ;
   LOC( 8) Vec4  _bone    :ATTR8 ; // this has to be Vec4 because VecI4 and VecU4 don't work for some reason
   LOC( 9) Vec4  _weight  :ATTR9 ; // this has to be Vec4 instead of VecH4 because of 2 reasons, we need sum of weights to be equal to 1.0 (half's can't do that), also when converting to GLSL the explicit casts to "Vec weight" precision are optimized away and perhaps some GLSL compilers may want to perform optimizations where Half*Vec is converted to VecH which would destroy precision for skinned characters
   LOC(10) VecH4 _material:ATTR10;
   LOC(11) VecH4 _color   :ATTR11;
#else
   // !! IF MAKING ANY CHANGE (EVEN PRECISION) THEN DON'T FORGET TO RE-CREATE 'VS_Code' FOR 'CreateInputLayout', see #VTX_INPUT_LAYOUT !!
   Vec4  _pos     :POSITION0   ;
   VecH  _hlp     :POSITION1   ;
   VecH  _nrm     :NORMAL      ;
   VecH4 _tan     :TANGENT     ;
   Vec2  _tex     :TEXCOORD0   ;
   Vec2  _tex1    :TEXCOORD1   ;
   Vec2  _tex2    :TEXCOORD2   ;
   Half  _size    :PSIZE       ;
   VecU4 _bone    :BLENDINDICES;
   Vec4  _weight  :BLENDWEIGHT ; // this has to be Vec4 instead of VecH4 because of 2 reasons, we need sum of weights to be equal to 1.0 (half's can't do that), also when converting to GLSL the explicit casts to "Vec weight" precision are optimized away and perhaps some GLSL compilers may want to perform optimizations where Half*Vec is converted to VecH which would destroy precision for skinned characters
   VecH4 _material:COLOR0      ;
   VecH4 _color   :COLOR1      ;
#endif
   UInt  _instance:SV_InstanceID;
#include "!Set Prec Default.h"

   VecH  nrm      (                                        ) {return _nrm                                                                  ;} // vertex normal
   VecH  tan      (VecH nrm          , Bool heightmap=false) {return heightmap ? VecH(1-nrm.x*nrm.x, -nrm.y*nrm.x, -nrm.z*nrm.x) : _tan.xyz;} // vertex tangent, for heightmap: PointOnPlane(Vec(1,0,0), nrm()), Vec(1,0,0)-nrm*nrm.x, which gives a perpendicular however not Normalized !!
   VecH  bin      (VecH nrm, VecH tan, Bool heightmap=false) {return heightmap ? Cross(nrm, tan) : Cross(nrm, tan)*_tan.w                  ;} // binormal from transformed normal and tangent
   Vec2  pos2     (                                        ) {return _pos.xy                                                               ;} // vertex position
   Vec   pos      (                                        ) {return _pos.xyz                                                              ;} // vertex position
   Vec4  pos4     (                                        ) {return _pos                                                                  ;} // vertex position in Vec4(pos.xyz, 1) format
   Flt   posZ     (                                        ) {return _pos.z                                                                ;} // vertex position Z
   VecH  hlp      (                                        ) {return _hlp                                                                  ;} // helper position
   VecH  tan      (                                        ) {return _tan.xyz                                                              ;} // helper position
   Vec2  tex      (                    Bool heightmap=false) {return heightmap ? _pos.xz*Vec2(VtxHeightmap, -VtxHeightmap) : _tex          ;} // tex coords 0
   Vec2  tex1     (                                        ) {return                                                         _tex1         ;} // tex coords 1
   Vec2  tex2     (                                        ) {return                                                         _tex2         ;} // tex coords 2
#if GL
   VecU  bone     (                                        ) {return VtxSkinning ? VecU(_bone.xyz) : VecU(0, 0, 0)                         ;} // bone matrix indexes
#else
   VecU  bone     (                                        ) {return VtxSkinning ?      _bone.xyz  : VecU(0, 0, 0)                         ;} // bone matrix indexes
#endif
   Vec   weight   (                                        ) {return _weight.xyz                                                           ;} // bone matrix weights
   VecH4 material (                                        ) {return _material                                                             ;} // material    weights
   VecH  material3(                                        ) {return _material.xyz                                                         ;} // material    weights
   Half  size     (                                        ) {return _size                                                                 ;} // point size

   // need to apply gamma correction in the shader because R8G8B8A8_SRGB can't be specified in vertex buffer
#if LINEAR_GAMMA
   VecH4 color     () {return VecH4(SRGBToLinear    (_color.rgb), _color.a);} // sRGB vertex color (precise)
   VecH  color3    () {return       SRGBToLinear    (_color.rgb)           ;} // sRGB vertex color (precise)
   VecH4 colorFast () {return VecH4(SRGBToLinearFast(_color.rgb), _color.a);} // sRGB vertex color (fast)
   VecH  colorFast3() {return       SRGBToLinearFast(_color.rgb)           ;} // sRGB vertex color (fast)
#else
   VecH4 color     () {return _color                                       ;} // sRGB vertex color (precise)
   VecH  color3    () {return _color.rgb                                   ;} // sRGB vertex color (precise)
   VecH4 colorFast () {return _color                                       ;} // sRGB vertex color (fast)
   VecH  colorFast3() {return _color.rgb                                   ;} // sRGB vertex color (fast)
#endif
   VecH4 colorF    () {return _color                                       ;} // linear vertex color
   VecH  colorF3   () {return _color.rgb                                   ;} // linear vertex color

   UInt instance() {return _instance;}
};
/******************************************************************************/
void DrawPixel_VS(VtxInput vtx,
      NOPERSP out Vec4 outVtx:POSITION)
{
   outVtx=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
void Draw_VS(VtxInput vtx,
 NOPERSP out Vec2 outTex:TEXCOORD0,
 NOPERSP out Vec4 outVtx:POSITION )
{
   outTex=vtx.tex();
   outVtx=Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
void DrawPosXY_VS(VtxInput vtx,
      NOPERSP out Vec2 outTex  :TEXCOORD0,
      NOPERSP out Vec2 outPosXY:TEXCOORD1,
      NOPERSP out Vec4 outVtx  :POSITION )
{
   outTex  =vtx.tex();
   outPosXY=ScreenToPosXY(outTex);
   outVtx  =Vec4(vtx.pos2(), Z_BACK, 1); // set Z to be at the end of the viewport, this enables optimizations by optional applying lighting only on solid pixels (no sky/background)
}
void Draw2DTex_VS(VtxInput vtx,
      NOPERSP out Vec2 outTex:TEXCOORD,
      NOPERSP out Vec4 outVtx:POSITION)
{
   outTex=vtx.tex();
   outVtx=Vec4(vtx.pos2()*Coords.xy+Coords.zw, Z_FRONT, 1);
}
/******************************************************************************/
inline Half DistPointPlaneRay(VecH2 p,                  VecH2 plane_normal, VecH2 ray) {Half rd=Dot(ray, plane_normal); return rd ? Dot           (p,            plane_normal)/rd : 0;}
inline Flt  DistPointPlaneRay(Vec2  p,                  Vec2  plane_normal, Vec2  ray) {Flt  rd=Dot(ray, plane_normal); return rd ? Dot           (p,            plane_normal)/rd : 0;}
inline Half DistPointPlaneRay(VecH  p,                  VecH  plane_normal, VecH  ray) {Half rd=Dot(ray, plane_normal); return rd ? Dot           (p,            plane_normal)/rd : 0;}
inline Flt  DistPointPlaneRay(Vec   p,                  Vec   plane_normal, Vec   ray) {Flt  rd=Dot(ray, plane_normal); return rd ? Dot           (p,            plane_normal)/rd : 0;}
inline Half DistPointPlaneRay(VecH2 p, VecH2 plane_pos, VecH2 plane_normal, VecH2 ray) {Half rd=Dot(ray, plane_normal); return rd ? DistPointPlane(p, plane_pos, plane_normal)/rd : 0;}
inline Flt  DistPointPlaneRay(Vec2  p, Vec2  plane_pos, Vec2  plane_normal, Vec2  ray) {Flt  rd=Dot(ray, plane_normal); return rd ? DistPointPlane(p, plane_pos, plane_normal)/rd : 0;}
inline Half DistPointPlaneRay(VecH  p, VecH  plane_pos, VecH  plane_normal, VecH  ray) {Half rd=Dot(ray, plane_normal); return rd ? DistPointPlane(p, plane_pos, plane_normal)/rd : 0;}
inline Flt  DistPointPlaneRay(Vec   p, Vec   plane_pos, Vec   plane_normal, Vec   ray) {Flt  rd=Dot(ray, plane_normal); return rd ? DistPointPlane(p, plane_pos, plane_normal)/rd : 0;}

inline VecH2 PointOnPlaneRay(VecH2 p,                  VecH2 plane_normal, VecH2 ray) {return p-ray*DistPointPlaneRay(p,            plane_normal, ray);}
inline Vec2  PointOnPlaneRay(Vec2  p,                  Vec2  plane_normal, Vec2  ray) {return p-ray*DistPointPlaneRay(p,            plane_normal, ray);}
inline VecH  PointOnPlaneRay(VecH  p,                  VecH  plane_normal, VecH  ray) {return p-ray*DistPointPlaneRay(p,            plane_normal, ray);}
inline Vec   PointOnPlaneRay(Vec   p,                  Vec   plane_normal, Vec   ray) {return p-ray*DistPointPlaneRay(p,            plane_normal, ray);}
inline VecH2 PointOnPlaneRay(VecH2 p, VecH2 plane_pos, VecH2 plane_normal, VecH2 ray) {return p-ray*DistPointPlaneRay(p, plane_pos, plane_normal, ray);}
inline Vec2  PointOnPlaneRay(Vec2  p, Vec2  plane_pos, Vec2  plane_normal, Vec2  ray) {return p-ray*DistPointPlaneRay(p, plane_pos, plane_normal, ray);}
inline VecH  PointOnPlaneRay(VecH  p, VecH  plane_pos, VecH  plane_normal, VecH  ray) {return p-ray*DistPointPlaneRay(p, plane_pos, plane_normal, ray);}
inline Vec   PointOnPlaneRay(Vec   p, Vec   plane_pos, Vec   plane_normal, Vec   ray) {return p-ray*DistPointPlaneRay(p, plane_pos, plane_normal, ray);}
/******************************************************************************/
inline Matrix3 Inverse(Matrix3 m, Bool normalized)
{
   m=transpose(m);
   if(!normalized)
   {
      m[0]/=Length2(m[0]);
      m[1]/=Length2(m[1]);
      m[2]/=Length2(m[2]);
   }
   return m;
}
/******************************************************************************
inline Flt Lerp4(Flt v0, Flt v1, Flt v2, Flt v3, Flt s)
{
   return s*s*s * ((2-TAN)*(v1-v2) + TAN*(v3-v0)                            )
        + s*s   * ((TAN-3)*(v1   ) - TAN*(v3   ) - (2*TAN-3)*v2 + (2*TAN)*v0)
        + s     * ( TAN   *(v2-v0)                                          )
        + v1;
}
inline Half Lerp4(Half v0, Half v1, Half v2, Half v3, Half s)
{
   return s*s*s * ((2-TAN)*(v1-v2) + TAN*(v3-v0)                            )
        + s*s   * ((TAN-3)*(v1   ) - TAN*(v3   ) - (2*TAN-3)*v2 + (2*TAN)*v0)
        + s     * ( TAN   *(v2-v0)                                          )
        + v1;
}

inline Vec Lerp4(Vec v0, Vec v1, Vec v2, Vec v3, Flt s)
{
   Flt s2=s*s,
       s3=s*s*s;

   return v1 * ((2-TAN)*s3     + (  TAN-3)*s2         + 1)
        - v2 * ((2-TAN)*s3     + (2*TAN-3)*s2 - TAN*s    )
        - v0 * (   TAN*(s3+s ) - (2*TAN  )*s2            )
        + v3 * (   TAN*(s3-s2)                           );
}
inline VecH Lerp4(VecH v0, VecH v1, VecH v2, VecH v3, Half s)
{
   Half s2=s*s,
        s3=s*s*s;

   return v1 * ((2-TAN)*s3     + (  TAN-3)*s2         + 1)
        - v2 * ((2-TAN)*s3     + (2*TAN-3)*s2 - TAN*s    )
        - v0 * (   TAN*(s3+s ) - (2*TAN  )*s2            )
        + v3 * (   TAN*(s3-s2)                           );
}

inline Vec4 Lerp4(Vec4 v0, Vec4 v1, Vec4 v2, Vec4 v3, Flt s)
{
   Flt s2=s*s,
       s3=s*s*s;

   return v1 * ((2-TAN)*s3     + (  TAN-3)*s2         + 1)
        - v2 * ((2-TAN)*s3     + (2*TAN-3)*s2 - TAN*s    )
        - v0 * (   TAN*(s3+s ) - (2*TAN  )*s2            )
        + v3 * (   TAN*(s3-s2)                           );
}
inline VecH4 Lerp4(VecH4 v0, VecH4 v1, VecH4 v2, VecH4 v3, Half s)
{
   Half s2=s*s,
        s3=s*s*s;

   return v1 * ((2-TAN)*s3     + (  TAN-3)*s2         + 1)
        - v2 * ((2-TAN)*s3     + (2*TAN-3)*s2 - TAN*s    )
        - v0 * (   TAN*(s3+s ) - (2*TAN  )*s2            )
        + v3 * (   TAN*(s3-s2)                           );
}
/******************************************************************************/
inline Half LerpCube(Half s) {return (3-2*s)*s*s;}
inline Flt  LerpCube(Flt  s) {return (3-2*s)*s*s;}

inline Half LerpCube(Half from, Half to, Half s) {return Lerp(from, to, LerpCube(s));}
inline Flt  LerpCube(Flt  from, Flt  to, Flt  s) {return Lerp(from, to, LerpCube(s));}

/*inline Flt LerpSmoothPow(Flt s, Flt p)
{
   s=Sat(s);
   if(s<=0.5)return   0.5*Pow(  2*s, p);
             return 1-0.5*Pow(2-2*s, p);
}*/

inline Half BlendSqr(Half x) {return Sat(1-x*x);}
inline Flt  BlendSqr(Flt  x) {return Sat(1-x*x);}

inline Half BlendSmoothCube(Half x) {x=Sat(Abs(x)); return 1-LerpCube(x);}
inline Flt  BlendSmoothCube(Flt  x) {x=Sat(Abs(x)); return 1-LerpCube(x);}

inline Half BlendSmoothSin(Half x) {x=Sat(Abs(x)); return Cos(x*PI)*0.5+0.5;}
inline Flt  BlendSmoothSin(Flt  x) {x=Sat(Abs(x)); return Cos(x*PI)*0.5+0.5;}

inline Half Gaussian(Half x) {return exp(-x*x);}
inline Flt  Gaussian(Flt  x) {return exp(-x*x);}
/******************************************************************************/
inline Half     VisibleOpacity(Flt density, Flt range) {return   Pow(1-density, range);} // calculate visible     opacity (0..1) having 'density' environment density (0..1), and 'range' (0..Inf)
inline Half AccumulatedDensity(Flt density, Flt range) {return 1-Pow(1-density, range);} // calculate accumulated density (0..1) having 'density' environment density (0..1), and 'range' (0..Inf)
/******************************************************************************/
inline Half DitherValue(Vec2 pixel)
{
#if 0 // low
   return Frac(Dot(pixel, Vec2(1.0, 0.5)/3))-0.5;
#elif 0 // medium
   return Frac(Dot(pixel, Vec2(3, 1)/8))-0.5;
#elif 1 // good
   return Frac(Dot(pixel, Vec2(1.6, 1)*0.25))-0.5; // -0.5 .. 0.5 range
#else
   VecI2 xy=Trunc(pixel)%8; return OrderDither[xy.x + xy.y*8]; // -1..1 / 256 range
#endif
}
inline void ApplyDither(inout VecH col, Vec2 pixel, Bool linear_gamma=LINEAR_GAMMA)
{
   if(linear_gamma)col=LinearToSRGBFast(col);
   col+=DitherValue(pixel)*(1.5/255);
   if(linear_gamma)col=SRGBToLinearFast(col);
}
/******************************************************************************/
// RGB <-> HSB
/******************************************************************************/
inline Vec RgbToHsb(Vec rgb)
{
   Flt max=Max(rgb),
       min=Min(rgb),
       d  =max-min;
   Vec hsb;

   if(d    <=  0)hsb.x=0;else
   if(rgb.x>=max)
   {
      if(rgb.y>=rgb.z)hsb.x=(rgb.y-rgb.z)/d;
      else            hsb.x=(rgb.y-rgb.z)/d+6;
   }else
   if(rgb.y>=max)hsb.x=(rgb.z-rgb.x)/d+2;
   else          hsb.x=(rgb.x-rgb.y)/d+4;

   hsb.x/=6;
   hsb.y =(max ? 1-min/max : 1);
   hsb.z = max;

   return hsb;
}
inline Vec HsbToRgb(Vec hsb)
{
   Flt h=Frac(hsb.x)*6,
       s=Sat (hsb.y),
       f=Frac(h),
       v=hsb.z,
       p=hsb.z*(1-s      ),
       q=hsb.z*(1-s*(  f)),
       t=hsb.z*(1-s*(1-f));
   if(h<1)return Vec(v, t, p);
   if(h<2)return Vec(q, v, p);
   if(h<3)return Vec(p, v, t);
   if(h<4)return Vec(p, q, v);
   if(h<5)return Vec(t, p, v);
          return Vec(v, p, q);
}
/******************************************************************************/
// ALPHA TEST
/******************************************************************************/
inline void AlphaTest(Half alpha)
{
   clip(alpha+Material.color.a-1);
}
/******************************************************************************/
// NORMAL
/******************************************************************************/
inline Vec4 UnpackNormal(VecH4 nrm)
{
#if !SIGNED_NRM_RT
   nrm.xyz=nrm.xyz*2-1;
#endif
   Vec4 nrm_hp=nrm; // convert to HP before normalization
   nrm_hp.xyz=Normalize(nrm_hp.xyz); // normalize needed even if source was F16 format because it improves quality for specular
   return nrm_hp;
}
inline Vec4 GetNormal  (Vec2  tex               ) {return UnpackNormal(TexPoint (Img  , tex          ));}
inline Vec4 GetNormalMS(VecI2 pixel, UInt sample) {return UnpackNormal(TexSample(ImgMS, pixel, sample));}
/******************************************************************************/
// EXT
/******************************************************************************/
inline VecH2 GetExt  (Vec2  tex               ) {return TexPoint (ImgXY  , tex          );}
inline VecH2 GetExtMS(VecI2 pixel, UInt sample) {return TexSample(ImgXYMS, pixel, sample);}
/******************************************************************************/
// LOD INDEX
/******************************************************************************/
inline Flt GetLod(Vec2 tex_coord, Flt tex_size)
{
   Vec2 tex=tex_coord*tex_size;
   return 0.5*log2(Max(Length2(ddx(tex)) , Length2(ddy(tex)))); // NVIDIA
 //return 0.5*log2(Max(Sqr    (ddx(tex)) + Sqr    (ddy(tex)))); // ATI
}
inline Flt GetLod(Vec2 tex_coord, Vec2 tex_size)
{
   Vec2 tex=tex_coord*tex_size;
   return 0.5*log2(Max(Length2(ddx(tex)) , Length2(ddy(tex)))); // NVIDIA
 //return 0.5*log2(Max(Sqr    (ddx(tex)) + Sqr    (ddy(tex)))); // ATI
}
/******************************************************************************/
// GRASS AND LEAF
/******************************************************************************/
#define GrassBendFreq  1.0
#define GrassBendScale 0.18

#define LeafBendFreq   2.0
#define LeafBendScale  0.13
#define LeafsBendScale (LeafBendScale/2)
/******************************************************************************/
inline Vec2 GetGrassBend(Vec world_pos)
{
   Flt offset=Dot(world_pos.xz, Vec2(0.7, 0.9)*GrassBendFreq);
   return Vec2((0.28*GrassBendScale)*Sin(offset+BendFactor.x) + (0.32*GrassBendScale)*Sin(offset+BendFactor.y),
               (0.18*GrassBendScale)*Sin(offset+BendFactor.z) + (0.24*GrassBendScale)*Sin(offset+BendFactor.w));
}
inline VecH2 GetLeafBend(VecH center)
{
   Half offset=Dot(center.xy, VecH2(0.7, 0.8)*LeafBendFreq);
   return VecH2((0.28*LeafBendScale)*(Half)Sin(offset+BendFactor.x) + (0.32*LeafBendScale)*(Half)Sin(offset+BendFactor.y),
                (0.18*LeafBendScale)*(Half)Sin(offset+BendFactor.z) + (0.24*LeafBendScale)*(Half)Sin(offset+BendFactor.w));
}
inline VecH2 GetLeafsBend(VecH center)
{
   Half offset=Dot(center.xy, VecH2(0.7, 0.8)*LeafBendFreq);
   return VecH2((0.28*LeafsBendScale)*(Half)Sin(offset+BendFactor.x) + (0.32*LeafsBendScale)*(Half)Sin(offset+BendFactor.y),
                (0.18*LeafsBendScale)*(Half)Sin(offset+BendFactor.z) + (0.24*LeafsBendScale)*(Half)Sin(offset+BendFactor.w));
}
/******************************************************************************/
inline Half GrassFadeOut(UInt mtrx=0)
{
   return Sat(Length2(ViewMatrixPos(mtrx))*GrassRangeMulAdd.x+GrassRangeMulAdd.y);
}
inline void BendGrass(Vec local_pos, in out Vec view_pos, UInt mtrx=0)
{
   Flt  b   =Cube(Sat(local_pos.y));
   Vec2 bend=GetGrassBend(ObjWorldPos(mtrx))*(b*Length(ViewMatrixY(mtrx)));

   view_pos+=Vec(CamMatrix[0].x, CamMatrix[1].x, CamMatrix[2].x)*bend.x;
   view_pos+=Vec(CamMatrix[0].y, CamMatrix[1].y, CamMatrix[2].y)*bend.y;
}
/******************************************************************************/
inline void BendLeaf(VecH center, in out Vec pos)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafBend(center);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin);
   pos=center+delta;
}
inline void BendLeaf(VecH center, in out Vec pos, in out VecH nrm)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafBend(center);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin); nrm.xy=Rotate(nrm.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin); nrm.zy=Rotate(nrm.zy, cos_sin);
   pos=center+delta;
}
inline void BendLeaf(VecH center, in out Vec pos, in out VecH nrm, in out VecH tan)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafBend(center);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin); nrm.xy=Rotate(nrm.xy, cos_sin); tan.xy=Rotate(tan.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin); nrm.zy=Rotate(nrm.zy, cos_sin); tan.zy=Rotate(tan.zy, cos_sin);
   pos=center+delta;
}
/******************************************************************************/
inline void BendLeafs(VecH center, Half offset, in out Vec pos)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafsBend(center+offset);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin);
   pos=center+delta;
}
inline void BendLeafs(VecH center, Half offset, in out Vec pos, in out VecH nrm)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafsBend(center+offset);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin); nrm.xy=Rotate(nrm.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin); nrm.zy=Rotate(nrm.zy, cos_sin);
   pos=center+delta;
}
inline void BendLeafs(VecH center, Half offset, in out Vec pos, in out VecH nrm, in out VecH tan)
{
   VecH   delta=(VecH)pos-center;
   VecH2  cos_sin, bend=GetLeafsBend(center+offset);
   CosSin(cos_sin.x, cos_sin.y, bend.x); delta.xy=Rotate(delta.xy, cos_sin); nrm.xy=Rotate(nrm.xy, cos_sin); tan.xy=Rotate(tan.xy, cos_sin);
   CosSin(cos_sin.x, cos_sin.y, bend.y); delta.zy=Rotate(delta.zy, cos_sin); nrm.zy=Rotate(nrm.zy, cos_sin); tan.zy=Rotate(tan.zy, cos_sin);
   pos=center+delta;
}
/******************************************************************************/
// DEPTH WEIGHT
/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(DepthWeight)
   Flt DepthWeightScale=0.005;
BUFFER_END
#include "!Set Prec Default.h"

#if 0
Flt P1=0.004, P2=2;
inline Vec2 DepthWeightMAD(Flt depth) {return Vec2(-1.0/(depth*DepthWeightScale+P1), P2);}
#else
inline Vec2 DepthWeightMAD(Flt depth) {return Vec2(-1.0/(depth*DepthWeightScale+0.004), 2);}
#endif
inline Half DepthWeight(Flt delta, Vec2 dw_mad)
{
   return Sat(Abs(delta)*dw_mad.x + dw_mad.y);
}
/******************************************************************************/
// DETAIL
/******************************************************************************/
inline VecH4 GetDetail(Vec2 tex)
{
   VecH4 det=Tex(Det, tex*Material.det_scale); // XY=nrm.xy, Z=color, W=smooth #MaterialTextureLayout
   det.xy=(det.xy-0.5)*Material.det_power;
   det.zw= det.zw     *Material.det_power+(1-Material.det_power); // Lerp(1, det.zw, Material.det_power) = 1*(1-Material.det_power) + det.zw*Material.det_power
   return det;
}
inline VecH4 GetDetail0(Vec2 tex) {VecH4 det=Tex(Det , tex*MultiMaterial0.det_scale); det.xy=det.xy*MultiMaterial0.det_mul+MultiMaterial0.det_add; det.zw=det.zw*MultiMaterial0.det_mul+MultiMaterial0.det_inv; return det;}
inline VecH4 GetDetail1(Vec2 tex) {VecH4 det=Tex(Det1, tex*MultiMaterial1.det_scale); det.xy=det.xy*MultiMaterial1.det_mul+MultiMaterial1.det_add; det.zw=det.zw*MultiMaterial1.det_mul+MultiMaterial1.det_inv; return det;}
inline VecH4 GetDetail2(Vec2 tex) {VecH4 det=Tex(Det2, tex*MultiMaterial2.det_scale); det.xy=det.xy*MultiMaterial2.det_mul+MultiMaterial2.det_add; det.zw=det.zw*MultiMaterial2.det_mul+MultiMaterial2.det_inv; return det;}
inline VecH4 GetDetail3(Vec2 tex) {VecH4 det=Tex(Det3, tex*MultiMaterial3.det_scale); det.xy=det.xy*MultiMaterial3.det_mul+MultiMaterial3.det_add; det.zw=det.zw*MultiMaterial3.det_mul+MultiMaterial3.det_inv; return det;}
/******************************************************************************/
// FACE NORMAL HANDLING
/******************************************************************************/
inline void BackFlip(in out VecH dir, Bool front) {if(!front)dir=-dir;}
/******************************************************************************/
inline Half MultiMaterialWeight(Half weight, Half alpha) // 'weight'=weight of this material, 'alpha'=color texture alpha (opacity or bump)
{
   // sharpen alpha
#if 0 // not needed when ALPHA_POWER is big
   #if 0 // good but slow
      if(alpha<=0.5)alpha=  2*Sqr(  alpha);
      else          alpha=1-2*Sqr(1-alpha);
   #else // fast approximation
      alpha=Sat(alpha*1.5 + (-0.5*1.5+0.5)); // Sat((alpha-0.5)*1.5+0.5)
   #endif
#endif

#if 1 // works best
   #define ALPHA_POWER 10.0
   Half w=weight // base
         +weight*(1-weight)*(alpha*ALPHA_POWER - 0.5*ALPHA_POWER); // "weight"=ignore alpha at start "1-weight" ignore alpha at end, "(alpha-0.5)*ALPHA_POWER" alpha
   if(ALPHA_POWER>2)w=Max(w, weight/16); // if ALPHA_POWER>2 then this formula could go below zero which could result in artifacts, because weights could be negative or all zero, so maximize with a small slope (has to be zero at start, and small after that)
   return w;
#elif 1
   #define ALPHA_POWER 0.5
   return Sat(weight*(1+ALPHA_POWER) + alpha*ALPHA_POWER - ALPHA_POWER); // start at "-ALPHA_POWER" so it can clear out any alpha we have at the start (we always want up to zero weight at the start even if we have high alpha) and always want at least 1 weight at the end, even if we have low alpha
#else
   return Lerp(weight, alpha, weight*(1-weight)*2);
#endif
}
/******************************************************************************/
// LIGHTS
/******************************************************************************/
#include "!Set Prec Struct.h"
struct GpuLightDir
{
   Vec   dir; // high precision needed for HQ specular
   VecH4 color; // a=spec
   VecH  vol_exponent_steam;
};
struct GpuLightPoint
{
   Flt   power;
   Half  lum_max, vol, vol_max;
   Vec   pos;
   VecH4 color; // a=spec
};
struct GpuLightLinear
{
   Flt   neg_inv_range;
   Half  vol, vol_max;
   Vec   pos;
   VecH4 color; // a=spec
};
struct GpuLightCone
{
   Flt      neg_inv_range;
   Half     scale, vol, vol_max;
   Vec2     falloff;
   Vec      pos;
   VecH4    color; // a=spec
   MatrixH3 mtrx;
};

BUFFER(LightDir   ) GpuLightDir    LightDir   ; BUFFER_END
BUFFER(LightPoint ) GpuLightPoint  LightPoint ; BUFFER_END
BUFFER(LightLinear) GpuLightLinear LightLinear; BUFFER_END
BUFFER(LightCone  ) GpuLightCone   LightCone  ; BUFFER_END
#include "!Set Prec Default.h"
/******************************************************************************/
inline Half LightPointDist (Flt  inv_dist2) {return Min(Half(inv_dist2*LightPoint .power    ), LightPoint.lum_max);} // LightPoint.power/Length2(pos), NaN
inline Half LightLinearDist(Flt  dist     ) {return Sat(         dist *LightLinear.neg_inv_range + 1             );} // 1-Length(pos)/LightLinear.range
inline Half LightConeDist  (Flt  dist     ) {return Sat(         dist *LightCone  .neg_inv_range + 1             );} // 1-Length(pos)/LightCone  .range
inline Half LightConeAngle (Vec2 pos      ) {Half v=Sat(  Length(pos) *LightCone  .falloff.x+LightCone.falloff.y ); return v;} // alternative is Sqr(v)

Half F_Schlick(Half f0, Half f90, Half c) // High Precision not needed
{
   return (f90-f0)*Quint(1-c) + f0; // Quint(1-c) = ~exp2(-9.28*c)
}
Half Vis_SmithR2Inv(Half roughness2, Half NdotL, Half NdotV) // High Precision not needed, "roughness2=Sqr(roughness)", result is inversed 1/x
{
#if 1
   Half view =NdotV+Sqrt((-NdotV*roughness2+NdotV)*NdotV+roughness2);
	Half light=NdotL+Sqrt((-NdotL*roughness2+NdotL)*NdotL+roughness2);
	return view*light;
#else // gives same results but has 2 MUL and 1 ADD, instead of 2 ADD 1 MUL, don't use in case MUL is slower than ADD
   // Warning: "NdotL*" and "NdotV*" are exchanged on purpose
   Half view =NdotL*Sqrt((-NdotV*roughness2+NdotV)*NdotV+roughness2);
   Half light=NdotV*Sqrt((-NdotL*roughness2+NdotL)*NdotL+roughness2);
   return (view+light)*2;
#endif
}
Half Vis_SmithFastInv(Half roughness, Half NdotL, Half NdotV) // fast approximation of 'Vis_Smith', High Precision not needed, result is inversed 1/x
{
	Half view =NdotL*(NdotV*(1-roughness)+roughness);
	Half light=NdotV*(NdotL*(1-roughness)+roughness);
	return (view+light)*2;
}
/*Half D_GGX(Half roughness, Flt NdotH) // Trowbridge-Reitz, High Precision required
{
   Flt roughness2=Sqr(roughness);
   Flt f=(NdotH*roughness2-NdotH)*NdotH+1;
   return roughness2/(f*f); // NaN
}*/
Half D_GGX_Vis_Smith(Half roughness, Flt NdotH, Half NdotL, Half NdotV, Bool quality) // D_GGX and Vis_Smith combined together
{
   Half roughness2=Sqr(roughness);
   Flt  f=(NdotH*roughness2-NdotH)*NdotH+1;
   Flt  div=f*f*(quality ? Vis_SmithR2Inv(roughness2, NdotL, NdotV) : Vis_SmithFastInv(roughness, NdotL, NdotV));
   return div ? roughness2/div : HALF_MAX;
}
struct LightParams
{
   Flt                                       NdotH_HP, // High Precision needed for 'D_GGX'
        NdotL_HP, NdotV_HP, VdotL_HP                 ; // High Precision needed for 'NdotH, VdotH' calculation
   Half NdotL   , NdotV   , VdotL   , VdotH          ; // VdotH=LdotH, because H is in the middle between L and V

   void set(Vec N, Vec L) // operate on High Precision since it's needed below anyway
   {
      NdotL=NdotL_HP=Dot(N, L); // 'Sat' not needed !! instead do "NdotL>0" checks !!
   }
   void set(Vec N, Vec L, Vec nV) // nV=-V, High Precision required for all 3 vectors (this was tested)
   {
    /*if(Q)N=HALF(N);
      if(W)L=HALF(L);
      if(E)nV=HALF(nV);*/
	   NdotV=NdotV_HP=-Dot(N, nV);
	   VdotL=VdotL_HP=-Dot(nV, L);
   #if 0
      Vec H=Normalize(L-nV); // L+V
      NdotH_HP=Dot(N, H);
      VdotH   =Dot(L, H);
   #else // faster
    //VdotL=2*VdotH*VdotH-1
	   Flt VL=rsqrt(VdotL_HP*2+2);
	   NdotH_HP=(NdotL_HP+NdotV_HP)*VL;
	   VdotH   =       VL*VdotL_HP +VL;
   #endif
    /*if(Q)NdotV=HALF(NdotV);
      if(W)VdotL=HALF(VdotL);
      if(E)NdotV_HP=HALF(NdotV_HP);
      if(R)NdotH_HP=HALF(NdotH_HP);*/
   }

   // High Precision not needed for Diffuse
   Half diffuseOrenNayar(Half smooth) // highlights edges starting from smooth = 1 -> 0
   {
      Half roughness=1-smooth;
	   Half a=Sqr(roughness), a2=Sqr(a); // it's better to square roughness for 'a' too, because it provides smoother transitions between 0..1
	   Half s=VdotL - NdotV*NdotL;
	   Half A=1-0.5*a2/(a2+0.33);
	   Half B= 0.45*a2/(a2+0.09)*s; if(s>=0)B/=Max(NdotL, NdotV);
	   return (A+B)*(a*0.5+1);
   }
   Half diffuseBurley(Half smooth) // aka Disney, highlights edges starting from smooth = 0.5 -> 0 and darkens starting from smooth = 0.5 -> 1.0
   {
      Half roughness=1-smooth;
    //Half f90=0.5+(2*VdotH*VdotH)*roughness; 2*VdotH*VdotH=1+VdotL;
      Half f90=0.5+roughness+roughness*VdotL;
      Half light_scatter=F_Schlick(1, f90,     NdotL );
      Half  view_scatter=F_Schlick(1, f90, Abs(NdotV));
      return 0.965521237*light_scatter*view_scatter;
   }
   Half diffuseWater() // simulate light intensity based on angle between view and water surface normal as described in "Water.cpp"
   {
      const Half min=0.25;
   #if 0 // original
      Half d=Sat(1-NdotV);
      d=1-Sqr(1-d);
      return Lerp(min, 1.0, d);
   #else // optimized
      return Lerp(1.0, min, Sqr(NdotV));
   #endif
   }
   Half diffuse(Half smooth)
   {
   #if WATER
      return diffuseWater();
   #elif DIFFUSE_MODE==SDIFFUSE_OREN_NAYAR
      return diffuseOrenNayar(smooth);
   #elif DIFFUSE_MODE==SDIFFUSE_BURLEY
      return diffuseBurley(smooth);
   #else
      return 1;
   #endif
   }

   Half specular(Half smooth, Half reflectivity, Bool quality)
   {
      // currently specular can be generated even for smooth=0 and reflectivity=0 #SpecularReflectionFromZeroSmoothReflectivity
      const Half light_radius=0.0036;

      Half roughness=1-smooth;
   #if 0
      if( Q)roughness=Lerp(Pow(light_radius, E ? 1.0/4 : 1.0/2), Pow(1, E ? 1.0/4 : 1.0/2), roughness);
      roughness=(E ? Quart(roughness) : Sqr(roughness));
      if(!Q)roughness=Lerp(light_radius, 1, roughness);
   #else
      roughness =Sqr(roughness);
      roughness+=light_radius*(1-roughness); // roughness=Lerp(light_radius, 1, roughness);
   #endif

      Half F=F_Schlick(reflectivity, REFLECT_OCCL ? Sat(reflectivity*50) : 1, VdotH);
   #if 0
      Half D=D_GGX(roughness, NdotH_HP);
      Half Vis=(quality ? Vis_Smith(roughness, NdotL, Abs(NdotV)) : Vis_SmithFast(roughness, NdotL, Abs(NdotV))); // use "Abs(NdotV)" as it helps greatly with faces away from the camera
      return F*D*Vis/PI;
   #else
      Half D_Vis=D_GGX_Vis_Smith(roughness, NdotH_HP, NdotL, Abs(NdotV), quality); // use "Abs(NdotV)" as it helps greatly with faces away from the camera
      return F*D_Vis/PI;
   #endif
   }
};
/******************************************************************************/
// PBR REFLECTION
/******************************************************************************/
inline VecH2 EnvDFGTex(Half smooth, Half NdotV) // uses precomputed texture
{
   return TexLodClamp(EnvDFG, VecH2(smooth, NdotV)).xy;
}
/* https://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
originally developed by Lazarov, modified by Karis */
inline VecH2 EnvDFGLazarovKaris(Half smooth, Half NdotV) 
{
   const VecH4 m={-1, -0.0275, -0.572,  0.022},
               a={ 1,  0.0425,  1.04 , -0.04 };
#if 0
   VecH4 r=roughness*m+a;
#else
   // roughness=1-smooth
   VecH4 r=smooth*(-m)+(a+m);
#endif
   Half mul=Min(r.x*r.x, Quint(1-NdotV))*r.x + r.y;
   return VecH2(-1.04, 1.04)*mul + r.zw;
}
/*
http://miciwan.com/SIGGRAPH2015/course_notes_wip.pdf
NO because has overshots in low reflectivity
inline VecH2 EnvDFGIwanicki(Half roughness, Half NdotV)
{
   Half bias=exp2(-(7*NdotV+4*roughness));
   Half scale=1-bias-roughness*Max(bias, Min(Sqrt(roughness), 0.739 + 0.323*NdotV)-0.434);
   return VecH2(scale, bias);
}

https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
NO because for low reflect and high smooth, 'EnvDFGLazarovKaris' looks better
inline VecH2 EnvDFGLazarovNarkowicz(Half smooth, Half NdotV)
{
   smooth=Sqr(smooth);
   VecH4 p0 = VecH4( 0.5745, 1.548, -0.02397, 1.301 );
   VecH4 p1 = VecH4( 0.5753, -0.2511, -0.02066, 0.4755 );

   VecH4 t = smooth * p0 + p1;

   Half bias  = Sat( t.x * Min( t.y, exp2( -7.672 * NdotV ) ) + t.z );
   Half delta = Sat( t.w );
   Half scale = delta - bias;

   return VecH2(scale, bias);
}

https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
NO because for low reflect, and high smooth it looks the same for long smooth range
inline VecH2 EnvDFGNarkowicz(Half smooth, Half NdotV)
{
   smooth=Sqr(smooth);

   Half x = smooth;
   Half y = NdotV;

   Half b1 = -0.1688;
   Half b2 = 1.895;
   Half b3 = 0.9903;
   Half b4 = -4.853;
   Half b5 = 8.404;
   Half b6 = -5.069;
   Half bias = Sat( Min( b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y ) );
 
   Half d0 = 0.6045;
   Half d1 = 1.699;
   Half d2 = -0.5228;
   Half d3 = -3.603;
   Half d4 = 1.404;
   Half d5 = 0.1939;
   Half d6 = 2.661;
   Half delta = Sat( d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x );
   Half scale = delta - bias;
 
   return VecH2(scale, bias);
}*/
inline Half ReflectEnv(Half smooth, Half reflectivity, Half NdotV, Bool quality)
{
   // currently reflection can be generated even for smooth=0 and reflectivity=0 #SpecularReflectionFromZeroSmoothReflectivity
   VecH2 mad;
   if(quality)mad=EnvDFGTex         (smooth, NdotV);
   else       mad=EnvDFGLazarovKaris(smooth, NdotV);

   // energy compensation, increase reflectivity if it's close to 1 to account for multi-bounce https://google.github.io/filament/Filament.html#materialsystem/improvingthebrdfs/energylossinspecularreflectance
#if 1 // Esenthel version
   mad.x+=(1-mad.y-mad.x)*reflectivity; // mad.x=Lerp(mad.x, 1-mad.y, reflectivity);
   return reflectivity*mad.x + mad.y*(REFLECT_OCCL ? Sat(reflectivity*50) : 1);
#else // same results but slower
   return (reflectivity*mad.x + mad.y*(REFLECT_OCCL ? Sat(reflectivity*50) : 1))*(1+reflectivity*(1/(mad.x+mad.y)-1));
#endif
}
inline VecH ReflectCol(VecH unlit_col, Half reflectivity) // non-metals (with low reflectivity) have white reflection and metals (with high reflectivity) have colored reflection
{
#if 0 // linear version, looks like has sharp transitions near reflectivity=1, and in the middle (reflectivity=0.5) the object is too bright
   return unlit_col*reflectivity + (1-reflectivity); // Lerp(VecH(1,1,1), unlit_col, reflectivity)
#else // square version, better
   Half inv_reflect2=Sqr(1-reflectivity); return unlit_col*(1-inv_reflect2) + inv_reflect2;
#endif
}
inline Vec ReflectDir(Vec eye_dir, Vec nrm) // High Precision needed for high resolution texture coordinates
{
   return Transform3(reflect(eye_dir, nrm), CamMatrix);
}
inline VecH ReflectTex(Vec reflect_dir, Half smooth)
{
   return TexCubeLodI(Env, reflect_dir, (1-smooth)*EnvMipMaps).rgb;
}
inline VecH PBR1(VecH unlit_col, VecH lit_col, Half smooth, Half reflectivity, VecH spec, Half NdotV, Vec reflect_dir, Bool quality)
{
   return lit_col*(1-reflectivity)
         +ReflectCol(unlit_col, reflectivity)*
            (ReflectTex(reflect_dir, smooth)*(EnvColor*ReflectEnv(smooth, reflectivity, NdotV, quality))
            +spec);
}
inline VecH PBR(VecH unlit_col, VecH lit_col, Vec nrm, Half smooth, Half reflectivity, Vec eye_dir, VecH spec)
{
   return PBR1(unlit_col, lit_col, smooth, reflectivity, spec, -Dot(nrm, eye_dir), ReflectDir(eye_dir, nrm), true);
}
/******************************************************************************/
// SHADOWS
/******************************************************************************/
#include "!Set Prec Struct.h"
BUFFER(Shadow)
   Flt     ShdRange      ,
           ShdStep[6]    ;
   Vec2    ShdRangeMulAdd;
   VecH2   ShdOpacity    ;
   Vec4    ShdJitter     ;
   Matrix  ShdMatrix     ;
   Matrix4 ShdMatrix4[6] ;
BUFFER_END

#include "!Set Prec Image.h"

ImageShadow ShdMap;
ImageH      ShdMap1;

#include "!Set Prec Default.h"

inline Half ShadowFinal(Half shadow) {return shadow*ShdOpacity.x+ShdOpacity.y;}

inline Vec ShadowDirTransform(Vec pos, Int num)
{  // using "Int/UInt matrix_index" and "matrix_index=.."  and "ShdMatrix4[matrix_index]" was slower 
   // using "Matrix4  m"            and "m=ShdMatrix4[..]" and "p=Transform(pos, m)"      had the same performance as version below
   // imitate binary search
   Vec4 p;
   if(num<=4)
   {
      BRANCH if(num<=2 || pos.z<=ShdStep[1])
      {
         BRANCH if(num<=1 || pos.z<=ShdStep[0])p=Transform(pos, ShdMatrix4[0]);else
                                               p=Transform(pos, ShdMatrix4[1]);
      }else
      {
         BRANCH if(num<=3 || pos.z<=ShdStep[2])p=Transform(pos, ShdMatrix4[2]);else
                                               p=Transform(pos, ShdMatrix4[3]);
      }
   }else
   {
      BRANCH if(num<=3 || pos.z<=ShdStep[2])
      {
         BRANCH if(num<=1 || pos.z<=ShdStep[0])p=Transform(pos, ShdMatrix4[0]);else
         BRANCH if(num<=2 || pos.z<=ShdStep[1])p=Transform(pos, ShdMatrix4[1]);else
                                               p=Transform(pos, ShdMatrix4[2]);
      }else
      {
         BRANCH if(num<=4 || pos.z<=ShdStep[3])p=Transform(pos, ShdMatrix4[3]);else
         BRANCH if(num<=5 || pos.z<=ShdStep[4])p=Transform(pos, ShdMatrix4[4]);else
                                               p=Transform(pos, ShdMatrix4[5]);
      }
   }
   return p.xyz/p.w;
}
inline Vec ShadowPointTransform(Vec pos)
{
   Vec  local=Transform(pos, ShdMatrix);
   Flt  a=Max(Abs(local));
   Vec4 p;
   // order of matrixes is based on probability of their usage (most frequently light is hanged upwards, making lower texture most used, then sides -x +x -z +z, and last is top y+)
   BRANCH if(-local.y>=a)p=Transform(pos, ShdMatrix4[3]);else
   BRANCH if( local.x>=a)p=Transform(pos, ShdMatrix4[0]);else
   BRANCH if(-local.x>=a)p=Transform(pos, ShdMatrix4[1]);else
   BRANCH if( local.z>=a)p=Transform(pos, ShdMatrix4[4]);else
   BRANCH if(-local.z>=a)p=Transform(pos, ShdMatrix4[5]);else
                         p=Transform(pos, ShdMatrix4[2]);
   return p.xyz/p.w;
}
/******************************************************************************/
inline Vec2 ShadowJitter(Vec2 pixel)
{
     Vec2 offset=Frac(pixel*0.5)*2 - 0.5;
          offset.y+=   offset.x;
       if(offset.y>1.1)offset.y=0;
   return offset*ShdJitter.xy+ShdJitter.zw;
}
/******************************************************************************/
inline Half CompareDepth(Vec pos, Vec2 jitter_value, Bool jitter)
{
   if(jitter)pos.xy+=jitter_value;
   return TexShadow(ShdMap, pos);
}
inline Half CompareDepth2(Vec pos) // 'ShdMap1' is not a Shadow Map Depth Buffer but a Shadow Intensity Color RT
{
   return Tex(ShdMap1, pos.xy).x;
}
/******************************************************************************/
inline Half ShadowDirValue(Vec pos, Vec2 jitter_value, Bool jitter, Int num, Bool cloud)
{
   Half fade=Sat(Length2(pos)*ShdRangeMulAdd.x+ShdRangeMulAdd.y);
   Vec  p=ShadowDirTransform(pos, num);
   if(cloud)return fade+CompareDepth(p, jitter_value, jitter)*CompareDepth2(p);
   else     return fade+CompareDepth(p, jitter_value, jitter);
}
inline Half ShadowPointValue(Vec pos, Vec2 jitter_value, Bool jitter)
{
   Vec p=ShadowPointTransform(pos);
   return CompareDepth(p, jitter_value, jitter);
}
inline Half ShadowConeValue(Vec pos, Vec2 jitter_value, Bool jitter)
{
   Vec4 p=Transform(pos, ShdMatrix4[0]); p.xyz/=p.w;
   return CompareDepth(p.xyz, jitter_value, jitter);
}
/******************************************************************************/
struct DeferredSolidOutput // use this structure in Pixel Shader for setting the output of RT_DEFERRED solid modes
{
   // #RTOutput
   VecH4 out0:TARGET0; // Col, Glow
   VecH4 out1:TARGET1; // Nrm XYZ, Translucent
   VecH2 out2:TARGET2; // Smooth, Reflect
   VecH  out3:TARGET3; // Vel XYZ

   // set components
   inline void color (VecH color ) {out0.rgb=color;}
   inline void glow  (Half glow  ) {out0.w  =glow ;}
   inline void normal(VecH normal)
   {
   #if SIGNED_NRM_RT
      out1.xyz=normal;
   #else
      out1.xyz=normal*0.5+0.5; // -1..1 -> 0..1
   #endif
   }
   inline void translucent(Half translucent) {out1.w=translucent;}

   inline void smooth (Half smooth ) {out2.x=smooth ;}
   inline void reflect(Half reflect) {out2.y=reflect;}

   inline void velocity    (Vec  vel, Vec view_space_pos) {out3.xyz=GetVelocity_PS(vel, view_space_pos);}
   inline void velocityRaw (VecH vel                    ) {out3.xyz=vel;}
   inline void velocityZero(                            ) {out3.xyz=(SIGNED_VEL_RT ? 0 : 0.5);}
};
/******************************************************************************/
// TESSELATION
/******************************************************************************/
struct HSData
{
   Flt TessFactor[3]   :SV_TessFactor,
       InsideTessFactor:SV_InsideTessFactor;

   Vec B210:TEXCOORD0,
       B120:TEXCOORD1,
       B021:TEXCOORD2,
       B012:TEXCOORD3,
       B102:TEXCOORD4,
       B201:TEXCOORD5,
       B111:TEXCOORD6;

   Vec N110:NORMAL0,
       N011:NORMAL1,
       N101:NORMAL2;
};
inline Vec2 ToScreen(Vec pos)
{
   return pos.xy/Max(0.1, pos.z);
}
inline HSData GetHSData(Vec pos0, Vec pos1, Vec pos2, Vec nrm0, Vec nrm1, Vec nrm2, Bool shadow_map=false)
{
   HSData O;

 //BRANCH if(AdaptiveTesselation)
   {
      Vec2 p0=ToScreen(shadow_map ? Transform(pos0, ShdMatrix) : pos0),
           p1=ToScreen(shadow_map ? Transform(pos1, ShdMatrix) : pos1),
           p2=ToScreen(shadow_map ? Transform(pos2, ShdMatrix) : pos2);
      O.TessFactor[0]=Dist(p2, p0)*TesselationDensity;
      O.TessFactor[1]=Dist(p0, p1)*TesselationDensity;
      O.TessFactor[2]=Dist(p1, p2)*TesselationDensity;
      O.InsideTessFactor=Avg(O.TessFactor[0], O.TessFactor[1], O.TessFactor[2]);
   }/*else
   {
      O.TessFactor[0]=O.TessFactor[1]=O.TessFactor[2]=Tess;
      O.InsideTessFactor=Tess;
   }*/

   BRANCH if(O.InsideTessFactor>1)
   {
      Vec B003=pos0,
          B030=pos1,
          B300=pos2;

      Vec N002=nrm0,
          N020=nrm1,
          N200=nrm2;

      O.B210=2*B003+B030-Dot(B030-B003,N002)*N002;
      O.B120=2*B030+B003-Dot(B003-B030,N020)*N020;
      O.B021=2*B030+B300-Dot(B300-B030,N020)*N020;
      O.B012=2*B300+B030-Dot(B030-B300,N200)*N200;
      O.B102=2*B300+B003-Dot(B003-B300,N200)*N200;
      O.B201=2*B003+B300-Dot(B300-B003,N002)*N002;

      Vec E=O.B210+O.B120+O.B021+O.B012+O.B102+O.B201,
          V=B003+B030+B300;
      O.B111=E*0.5-V;
   }

#if 0 // cubic normal interpolation
   Flt V12=2*Dot(B030-B003, N002+N020)/Dot(B030-B003, B030-B003); O.N110=Normalize(N002+N020-V12*(B030-B003));
   Flt V23=2*Dot(B300-B030, N020+N200)/Dot(B300-B030, B300-B030); O.N011=Normalize(N020+N200-V23*(B300-B030));
   Flt V31=2*Dot(B003-B300, N200+N002)/Dot(B003-B300, B003-B300); O.N101=Normalize(N200+N002-V31*(B003-B300));
#endif

   return O;
}
/******************************************************************************/
inline void SetDSPosNrm(out Vec pos, out Vec nrm, Vec pos0, Vec pos1, Vec pos2, Vec nrm0, Vec nrm1, Vec nrm2, Vec B, HSData hs_data, Bool clamp_tess, Flt clamp_tess_factor)
{
   // TODO: we could encode 'clamp_tess_factor' in vtx.nrm.w

   Flt U=B.x, UU=U*U,
       V=B.y, VV=V*V,
       W=B.z, WW=W*W;

#if 0 // cubic normal interpolation
   nrm=Normalize(nrm0*WW
                +nrm1*UU
                +nrm2*VV
                +hs_data.N110*W*U
                +hs_data.N011*U*V
                +hs_data.N101*W*V);
#else // linear normal interpolation
   nrm=Normalize(nrm0*W + nrm1*U + nrm2*V);
#endif

   BRANCH if(hs_data.InsideTessFactor>1)
   {
      pos=pos0*(WW*W)
         +pos1*(UU*U)
         +pos2*(VV*V)
         +hs_data.B210*(WW*U)
         +hs_data.B120*(W*UU)
         +hs_data.B201*(WW*V)
         +hs_data.B021*(UU*V)
         +hs_data.B102*(W*VV)
         +hs_data.B012*(U*VV)
         +hs_data.B111*(W*U*V);
      if(clamp_tess)pos=Lerp(pos0*W + pos1*U + pos2*V, pos, clamp_tess_factor);
   }else
   {
      pos=pos0*W + pos1*U + pos2*V;
   }
}
/******************************************************************************/
