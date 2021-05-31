/******************************************************************************

   This header is visible on both CPU and GPU

   !! If any change is made here then all shaders need to be recompiled !!

/******************************************************************************/
// Shader Bump Mapping Modes
#define SBUMP_ZERO      0 // no vertex normal
#define SBUMP_FLAT      1
#define SBUMP_NORMAL    2
#define SBUMP_PARALLAX0 2 // !! this is on purpose the same as SBUMP_NORMAL because it's never used anyway, it's used just for calculations
#define SBUMP_PARALLAX1 3
#define SBUMP_PARALLAX2 4
#define SBUMP_PARALLAX3 5
#define SBUMP_PARALLAX4 6
#define SBUMP_RELIEF    7

#define SBUMP_PARALLAX_MIN SBUMP_PARALLAX2 // ignore SBUMP_PARALLAX1 because just 1 sample is not worth to keep as separate shader
#define SBUMP_PARALLAX_MAX SBUMP_PARALLAX4

// Shader Diffuse Modes
#define SDIFFUSE_LAMBERT    0 // set Lambert as 0 to be the default mode for Forward/Blend shaders which don't have DIFFUSE_MODE specified
#define SDIFFUSE_OREN_NAYAR 1
#define SDIFFUSE_BURLEY     2
#define SDIFFUSE_NUM        3

// Effects
#define FX_NONE     0
#define FX_GRASS_2D 1
#define FX_GRASS_3D 2
#define FX_LEAF_2D  3
#define FX_LEAF_3D  4
#define FX_LEAFS_2D 5
#define FX_LEAFS_3D 6

// Motion Blur
#define MAX_MOTION_BLUR_PIXEL_RANGE 48 // max range of pixels to blur for 2160p resolution, Warning: increasing this value will slow down performance because we need to do more dilation steps, also it would decrease precision for blur direction coordinates because currently 10-bit are used per channel
#define MOTION_BLUR_PREDICTIVE 1 // 1=assumes movement will continue along velocity and blur future movements too, enable because even though this mode is not correct (it blurs both ways, including the future) it helps prevent "leaking" when rotating camera around an object, where one side doesn't get blurred because it wants to get samples behind the object however they're not avaialble since the object covers them, so when blurring in both ways, we can just use samples from the other side

// Temporal Anti-Aliasing
#define TAA_SEPARATE_ALPHA 0 // 0=is faster and uses less memory however 'Renderer._alpha' may point to 'taa_new_weight' (X=alpha, Y=weight), 1=will create a separate Alpha RT just for '_alpha'
#define TAA_OLD_VEL        1

// Buffer Indexes
#define SBI_FRAME           0
#define SBI_CAMERA          1
#define SBI_OBJ_MATRIX      2
#define SBI_OBJ_MATRIX_PREV 3
#define SBI_MESH            4
#define SBI_MATERIAL        5
#define SBI_VIEWPORT        6
#define SBI_COLOR           7
#define SBI_NUM             8

// Sampler Indexes
#define SSI_DEFAULT      0
#define SSI_POINT        1
#define SSI_LINEAR_CLAMP 2
#define SSI_LINEAR_WRAP  3
#define SSI_SHADOW       4
#define SSI_FONT         5
#define SSI_LINEAR_CWW   6
#define SSI_NUM          7

// Material Textures
#define TEX_IS_ROUGH 1 // if texture has roughness (if 0 then it has smoothness)

// #MaterialTextureLayout
// base_0
#define BASE_CHANNEL_ALPHA w
// base_1
#define BASE_CHANNEL_NORMAL   xy
#define BASE_CHANNEL_NORMAL_X x
#define BASE_CHANNEL_NORMAL_Y y
// base_2
#define BASE_CHANNEL_ROUGH y
#define BASE_CHANNEL_METAL x
#define BASE_CHANNEL_BUMP  z
#define BASE_CHANNEL_GLOW  w

// #MaterialTextureLayoutDetail
#define DETAIL_CHANNEL_NORMAL   xy
#define DETAIL_CHANNEL_NORMAL_X x
#define DETAIL_CHANNEL_NORMAL_Y y
#define DETAIL_CHANNEL_ROUGH    z
#define DETAIL_CHANNEL_COLOR    w

// Other
#define MAX_MTRLS 4 // 3 or 4 (3 to make shaders smaller, 4 to support more materials per-triangle)

#define LCScale 0.2
/******************************************************************************/
