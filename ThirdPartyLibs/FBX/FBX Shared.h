/******************************************************************************/
#define FBX_LINK_NONE 0
#define FBX_LINK_LIB  1
#define FBX_LINK_DLL  2

#ifndef FBX_LINK_TYPE
   #if WINDOWS
      #define FBX_LINK_TYPE FBX_LINK_DLL
   #elif MAC || LINUX
      #define FBX_LINK_TYPE FBX_LINK_LIB
   #else
      #define FBX_LINK_TYPE FBX_LINK_NONE
   #endif
#endif
/******************************************************************************/
#ifndef _FBX_H
#define _FBX_H

#define FBX_MESH               0x01
#define FBX_SKEL               0x02
#define FBX_ANIM               0x04
#define FBX_MTRL               0x08
#define FBX_PMI                0x10
#define FBX_XSKEL              0x20
#define FBX_ALL_NODES_AS_BONES 0x40

inline Bool SaveFBXData(File &f, C Mesh *mesh, C Skeleton *skeleton, C MemPtr<XAnimation> &animations, C MemPtr<XMaterial> &materials, C MemPtr<Int> &part_material_index, C XSkeleton *xskeleton)
{
   return
      (!mesh                || mesh              ->save   (f))
   && (!skeleton            || skeleton          ->save   (f))
   && (!animations          || animations         .save   (f))
   && (!materials           || materials          .save   (f))
   && (!xskeleton           || xskeleton         ->save   (f))
   && (!part_material_index || part_material_index.saveRaw(f))
   && f.ok();
}
inline Bool LoadFBXData(File &f, Mesh *mesh, Skeleton *skeleton, MemPtr<XAnimation> animations, MemPtr<XMaterial> materials, MemPtr<Int> part_material_index, XSkeleton *xskeleton) // !! Warning: pointers validity must be the same as for save !!
{
   return
      (!mesh                || mesh              ->load   (f))
   && (!skeleton            || skeleton          ->load   (f))
   && (!animations          || animations         .load   (f))
   && (!materials           || materials          .load   (f))
   && (!xskeleton           || xskeleton         ->load   (f))
   && (!part_material_index || part_material_index.loadRaw(f))
   && f.ok();
}
#endif
/******************************************************************************/
