/******************************************************************************/
// MATH
/******************************************************************************/
struct Half;
struct VecH2;
struct Vec2;
struct VecD2;
struct VecI2;
struct VecB2;
struct VecSB2;
struct VecUS2;
struct VecH;
struct Vec;
struct VecD;
struct VecI;
struct VecB;
struct VecSB;
struct VecUS;
struct VecH4;
struct Vec4;
struct VecD4;
struct VecI4;
struct VecB4;
struct VecSB4;

struct Plane2;
struct Plane;
struct PlaneM;
struct PlaneD;

struct Edge2;
struct EdgeD2;
struct Edge;
struct EdgeD;

struct Tri2;
struct TriD2;
struct Tri;
struct TriN;
struct TriD;
struct TriND;

struct Quad2;
struct QuadD2;
struct Quad;
struct QuadN;
struct QuadD;
struct QuadND;

struct Rect;
struct RectD;
struct RectI;
struct Box;
struct BoxD;
struct BoxI;
struct OBox;
struct Extent;
struct Circle;
struct CircleM;
struct CircleD;
struct Ball;
struct BallM;
struct BallD;
struct Capsule;
struct Tube;
struct Torus;
struct Cone;
struct Pyramid;
struct Shape;
struct Poly;

struct Matrix2;
struct Matrix3;
struct MatrixD3;
struct Matrix2P;
struct Matrix;
struct MatrixM;
struct MatrixD;
struct Matrix4;
struct GpuMatrix;
struct RevMatrix3;
struct RevMatrix;

struct Orient;
struct OrientD;
struct OrientP;
struct OrientM;
struct OrientPD;

struct Quaternion;
struct QuaternionD;

struct FrustumClass;

struct Randomizer;
extern Randomizer Random;

struct InterpolatorTemp;
/******************************************************************************/
// ANIMATION
/******************************************************************************/
struct SkeletonBone;
struct SkeletonSlot;
struct Skeleton;
struct AnimatedSkeletonBone;
struct AnimatedSkeleton;
struct AnimationKeys;
struct AnimationBone;
struct AnimationEvent;
struct Animation;
struct SkelAnim;
struct Motion;
/******************************************************************************/
// GRAPHICS
/******************************************************************************/
struct Color;
struct Image;
struct ImageRT;
struct ImageHeader;
struct Material;
struct Video;
struct Font;
struct DisplayClass;
struct MainShaderClass;
struct RendererClass;
struct RenderTargets;
struct VtxFormatGL;
struct VtxFormat;
struct VtxBuf;
struct IndBuf;
struct VtxIndBuf;
struct VtxFull;
struct Light;
struct RippleFx;
struct TextStyleParams;
struct TextStyle;
struct TextInput;
struct ShaderParam;
struct ShaderParamChange;
struct ShaderImage;
struct ShaderBase;
struct Shader;
struct ShaderFile;
struct FRST;
struct BLST;
/******************************************************************************/
// GUI
/******************************************************************************/
struct  GUI;
struct  GuiObj;
struct  GuiObjs;
struct  GuiObjChildren;
struct  Button;
struct  CheckBox;
struct  ColorPicker;
struct  ComboBox;
struct  Dialog;
struct  Desktop;
struct  GuiCustom;
struct  TextCodeData;
struct  Text;
struct  Viewport;
struct  GuiImage;
struct _List;
struct  ListColumn;
struct  MenuElm;
struct  Menu;
struct  MenuBar;
struct  Progress;
struct  Region;
struct  SlideBar;
struct  Slider;
struct  Tab;
struct  Tabs;
struct  TextBox;
struct  TextLine;
struct  Window;
struct  WindowIO;
/******************************************************************************/
// MESH
/******************************************************************************/
struct Blocks;
struct BlocksMap;
struct MeshBase;
struct MeshBaseIndex;
struct MeshRender;
struct MeshPart;
struct MeshLod;
struct Mesh;
struct MeshGroup;
/******************************************************************************/
// INPUT
/******************************************************************************/
struct KbSc;
struct KeyboardClass;
struct MouseClass;
struct Touch;
struct VirtualRealityApi;
/******************************************************************************/
// MISC
/******************************************************************************/
struct Str8;
struct Str;
struct BStr;
struct File;
struct FileText;
struct PakFile;
struct Pak;
struct PakSet;
struct PakProgress;
struct PakInPlace;
struct PakNode;
struct PakFileData;
struct TextNode;
struct TextData;
struct XmlNode;
struct XmlData;
struct TextEdit;
struct CalcValue;
struct DateTime;
struct Cipher;
struct PathWorld;
struct UID;
struct _Memc;
struct _Memb;
struct _Memx;
struct  MemlNode;
T1(TYPE) struct Mems;
T1(TYPE) struct FixedMems;
T1(TYPE) struct Memc;
T1(TYPE) struct Memb;
T1(TYPE) struct Memx;
T1(TYPE) struct Meml;
template<typename TYPE, Int size=64*1024>   struct  Memt;
template<typename TYPE, Int size=64*1024>   struct CMemPtr;
template<typename TYPE, Int size=64*1024>   struct  MemPtr;
struct _Grid;
struct _Map;
struct _Cache;
T1(TYPE    ) struct Cache;
T2(KEY,DATA) struct MapEx;
template<typename TYPE              , Cache<TYPE    > &CACHE>   struct CacheElmPtr;
template<typename KEY, typename DATA, MapEx<KEY,DATA> &MAP  >   struct   MapElmPtr;
struct Object;
struct DataCallback;
struct Notification;
struct IndexWeight;
struct SysWindow;
/******************************************************************************/
// SOUND
/******************************************************************************/
struct _Sound;
struct _SoundRecord;
struct SoundBuffer;
struct SoundStream;
struct SoundDataCallback;
/******************************************************************************/
// PHYSICS
/******************************************************************************/
struct PhysHitBasic;
struct PhysHit;
struct PhysCutsCallback;
struct PhysHitCallback;
struct Joint;
struct Actor;
struct ActorInfo;
struct PhysMtrl;
struct Ragdoll;
struct Controller;
struct Grab;
struct PhysPart;
struct PhysBody;
struct PhysGroup;
struct PhysicsClass;
struct RigidBody;
/******************************************************************************/
// NET
/******************************************************************************/
struct Socket;
/******************************************************************************/
// GAME
/******************************************************************************/
namespace Game
{
   struct Obj;
   struct Item;
   struct Chr;
   struct Area;
   struct WorldSettings;
   struct WorldManager;
   T1(TYPE) struct ObjMap;
}
/******************************************************************************/
// EDIT
/******************************************************************************/
namespace Edit
{
   struct Symbol;
   struct Line;
   struct Source;
   struct Token;
   struct Macro;
   struct Expr;
   struct Command;
   struct Message;
   struct Compiler;
   struct CompilerContext;
}
/******************************************************************************/
