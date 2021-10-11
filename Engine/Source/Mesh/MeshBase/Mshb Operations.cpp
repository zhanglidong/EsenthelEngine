/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
// WELD
/******************************************************************************/
static void Weld(MeshBase &mshb, MESH_FLAG flag, Flt pos_eps, Flt nrm_cos, Flt remove_degenerate_faces_eps, Bool flat)
{
   if(pos_eps>=0)
   {
      if(flat)mshb.setVtxDup2D(flag, pos_eps, nrm_cos);
      else    mshb.setVtxDup  (flag, pos_eps, nrm_cos);

      Int   *vtx_dup=mshb.vtx .dup();
      VecI2 *edg_ind=mshb.edge.ind();
      VecI  *tri_ind=mshb.tri .ind();
      VecI4 *qud_ind=mshb.quad.ind();

      Memt<Bool> is;

      // vtx is
      is.setNum(mshb.vtxs()); REPAO(is)=(vtx_dup[i]==i);
      Int vtxs=CountIs(is);
      if( vtxs==mshb.vtxs())
      {
         mshb.exclude(VTX_DUP|ADJ_ALL);
      }else
      {
         Memt<Int> vtx_remap; SetRemap(vtx_remap, is, mshb.vtxs());
         REPA(vtx_remap)if(!is[i])vtx_remap[i]=vtx_remap[vtx_dup[i]];

         mshb.exclude(VTX_DUP|ADJ_ALL);
         MeshBase temp(vtxs, 0, 0, 0, mshb.flag());

         // vtx
         {
            temp.copyVtxs(mshb, is);
         }

         // quad
         Memb<VecI4> quad_tri;
         {
            is.setNum(mshb.quads());
            FREPA(mshb.quad)
            {
               VecI4 f=qud_ind[i], r=f; r.remapAll(vtx_remap);
               if(!(is[i]=r.allDifferent()))
               {
                  if(r.c[0]!=r.c[1] && r.c[1]!=r.c[3] && r.c[0]!=r.c[3])quad_tri.New().set(f.c[0], f.c[1], f.c[3], i);else // 013
                  if(r.c[0]!=r.c[1] && r.c[1]!=r.c[2] && r.c[0]!=r.c[2])quad_tri.New().set(f.c[0], f.c[1], f.c[2], i);else // 012
                  if(r.c[1]!=r.c[2] && r.c[2]!=r.c[3] && r.c[1]!=r.c[3])quad_tri.New().set(f.c[1], f.c[2], f.c[3], i);else // 123
                  if(r.c[2]!=r.c[3] && r.c[3]!=r.c[0] && r.c[2]!=r.c[0])quad_tri.New().set(f.c[2], f.c[3], f.c[0], i);     // 230
               }
            }
            temp.quad._elms=CountIs(is); temp.include(QUAD_ALL&mshb.flag());
            temp.copyQuads(mshb, is);
            IndRemap(vtx_remap, temp.quad.ind(), temp.quads());
         }

         // tri
         {
            is.setNum(mshb.tris()); FREPA(mshb.tri){VecI f=tri_ind[i]; f.remapAll(vtx_remap); is[i]=f.allDifferent();}
            Int       tris=CountIs(is);
            MESH_FLAG flag=mshb.flag(), f=(flag&TRI_ALL);

            if(quad_tri.elms())
            {
                                 f|=TRI_IND;
               if(flag&QUAD_NRM )f|=TRI_NRM;
               if(flag&QUAD_FLAG)f|=TRI_FLAG;
               if(flag&QUAD_ID  )f|=TRI_ID;
            }

            temp.tri._elms=tris+quad_tri.elms(); temp.include(f);
            temp.copyTris(mshb, is);

            if(temp.tri.nrm () && !mshb.tri.nrm ())ZeroFastN(temp.tri.nrm (), tris);
            if(temp.tri.flag() && !mshb.tri.flag())ZeroFastN(temp.tri.flag(), tris);
            if(temp.tri.id  () && !mshb.tri.id  ())ZeroFastN(temp.tri.id  (), tris);

            REPA(quad_tri)
            {
               VecI4 &v=quad_tri[i];
                                  temp.tri.ind (tris+i)=v.xyz;
               if(temp.tri.nrm ())temp.tri.nrm (tris+i)=(mshb.quad.nrm () ? mshb.quad.nrm (v.w) :  0);
               if(temp.tri.flag())temp.tri.flag(tris+i)=(mshb.quad.flag() ? mshb.quad.flag(v.w) :  0);
               if(temp.tri.id  ())temp.tri.id  (tris+i)=(mshb.quad.id  () ? mshb.quad.id  (v.w) : -1);
            }
            IndRemap(vtx_remap, temp.tri.ind(), temp.tris());
         }

         // edge
         {
            is.setNum(mshb.edges()); FREPA(mshb.edge){Int *p=edg_ind[i].c; is[i]=(vtx_remap[p[0]]!=vtx_remap[p[1]]);}
            temp.edge._elms=CountIs(is); temp.include(EDGE_ALL&mshb.flag());
            temp.copyEdges(mshb, is);
            IndRemap(vtx_remap, temp.edge.ind(), temp.edges());
         }

         // remove degenerate faces
         if(remove_degenerate_faces_eps>=0)temp.removeDegenerateFaces(remove_degenerate_faces_eps);

         // finish
         Swap(mshb, temp);
      }
   }
}
MeshBase& MeshBase::weldVtx2D(MESH_FLAG flag, Flt pos_eps, Flt nrm_cos, Flt remove_degenerate_faces_eps) {Weld(T, flag, pos_eps, nrm_cos, remove_degenerate_faces_eps, true ); return T;}
MeshBase& MeshBase::weldVtx  (MESH_FLAG flag, Flt pos_eps, Flt nrm_cos, Flt remove_degenerate_faces_eps) {Weld(T, flag, pos_eps, nrm_cos, remove_degenerate_faces_eps, false); return T;}
/******************************************************************************/
MeshBase& MeshBase::weldVtxValues(MESH_FLAG flag, Flt pos_eps, Flt nrm_cos, Flt remove_degenerate_faces_eps)
{
   flag&=T.flag(); // can weld only values that we have
   if(flag&(VTX_POS|VTX_NRM_TAN_BIN|VTX_HLP|VTX_TEX_ALL|VTX_COLOR|VTX_MATERIAL|VTX_SKIN|VTX_SIZE)) // if have anything to weld
   {
      setVtxDup(VTX_NRM, pos_eps, nrm_cos);

      // calculate weight for all vertexes based on their face area
      Memt<Flt> vtx_weight; vtx_weight.setNumZero(vtxs());
      if(C Vec *pos=vtx.pos())
      {
         if(tri.ind())REPA(tri)
         {
          C VecI  &ind=tri.ind(i);
            Flt weight=TriArea2(pos[ind.x], pos[ind.y], pos[ind.z]);
            vtx_weight[ind.x]+=weight;
            vtx_weight[ind.y]+=weight;
            vtx_weight[ind.z]+=weight;
         }
         if(quad.ind())REPA(quad)
         {
          C VecI4 &ind=quad.ind(i);
            Flt weight=TriArea2(pos[ind.x], pos[ind.y], pos[ind.w]) + TriArea2(pos[ind.y], pos[ind.z], pos[ind.w]);
            vtx_weight[ind.x]+=weight;
            vtx_weight[ind.y]+=weight;
            vtx_weight[ind.z]+=weight;
            vtx_weight[ind.w]+=weight;
         }
      }

      Memt<Vec4> color; if(flag&VTX_COLOR)color.setNumZero(vtxs());

      // first scale unique vertex values by their weight
      REPA(vtx)
      {
         Int d=vtx.dup(i); if(d==i) // unique
         {
            Flt weight=vtx_weight[i];
            if(flag&VTX_POS     )vtx.pos     (i)*=weight;
          //if(flag&VTX_MATERIAL)vtx.material(i)*=weight; VecB4 !! sum must be equal to 255 !!
          //if(flag&VTX_MATRIX  )vtx.matrix  (i)*=weight; VecB4
          //if(flag&VTX_BLEND   )vtx.blend   (i)*=weight; VecB4 !! sum must be equal to 255 !!
            if(flag&VTX_NRM     )vtx.nrm     (i)*=weight;
            if(flag&VTX_TAN     )vtx.tan     (i)*=weight;
            if(flag&VTX_BIN     )vtx.bin     (i)*=weight;
            if(flag&VTX_HLP     )vtx.hlp     (i)*=weight;
            if(flag&VTX_TEX0    )vtx.tex0    (i)*=weight;
            if(flag&VTX_TEX1    )vtx.tex1    (i)*=weight;
            if(flag&VTX_TEX2    )vtx.tex2    (i)*=weight;
            if(flag&VTX_TEX3    )vtx.tex3    (i)*=weight;
            if(flag&VTX_SIZE    )vtx.size    (i)*=weight;
            if(flag&VTX_COLOR   )color       [i] =weight*vtx.color(i);
         }
      }

      // now add duplicate values to unique
      REPA(vtx)
      {
         Int d=vtx.dup(i); if(d!=i) // duplicate
         {
            Flt weight=vtx_weight[i];
                                 vtx_weight  [d]+=weight;
            if(flag&VTX_POS     )vtx.pos     (d)+=weight*vtx.pos     (i);
          //if(flag&VTX_MATERIAL)vtx.material(d)+=weight*vtx.material(i); VecB4 !! sum must be equal to 255 !!
          //if(flag&VTX_MATRIX  )vtx.matrix  (d)+=weight*vtx.matrix  (i); VecB4
          //if(flag&VTX_BLEND   )vtx.blend   (d)+=weight*vtx.blend   (i); VecB4 !! sum must be equal to 255 !!
            if(flag&VTX_NRM     )vtx.nrm     (d)+=weight*vtx.nrm     (i);
            if(flag&VTX_TAN     )vtx.tan     (d)+=weight*vtx.tan     (i);
            if(flag&VTX_BIN     )vtx.bin     (d)+=weight*vtx.bin     (i);
            if(flag&VTX_HLP     )vtx.hlp     (d)+=weight*vtx.hlp     (i);
            if(flag&VTX_TEX0    )vtx.tex0    (d)+=weight*vtx.tex0    (i);
            if(flag&VTX_TEX1    )vtx.tex1    (d)+=weight*vtx.tex1    (i);
            if(flag&VTX_TEX2    )vtx.tex2    (d)+=weight*vtx.tex2    (i);
            if(flag&VTX_TEX3    )vtx.tex3    (d)+=weight*vtx.tex3    (i);
            if(flag&VTX_SIZE    )vtx.size    (d)+=weight*vtx.size    (i);
            if(flag&VTX_COLOR   )color       [d]+=weight*vtx.color   (i);
         }
      }

      // normalize unique vertex values
      REPA(vtx)
      {
         Int d=vtx.dup(i); if(d==i) // unique
            if(Flt weight=vtx_weight[i])
         {
            weight=1/weight;
            if(flag&VTX_POS     )vtx.pos     (i)*=weight;
          //if(flag&VTX_MATERIAL)vtx.material(i)*=weight; VecB4 !! sum must be equal to 255 !!
          //if(flag&VTX_MATRIX  )vtx.matrix  (i)*=weight; VecB4
          //if(flag&VTX_BLEND   )vtx.blend   (i)*=weight; VecB4 !! sum must be equal to 255 !!
            if(flag&VTX_NRM     )vtx.nrm     (i).normalize();
            if(flag&VTX_TAN     )vtx.tan     (i).normalize();
            if(flag&VTX_BIN     )vtx.bin     (i).normalize();
            if(flag&VTX_HLP     )vtx.hlp     (i)*=weight;
            if(flag&VTX_TEX0    )vtx.tex0    (i)*=weight;
            if(flag&VTX_TEX1    )vtx.tex1    (i)*=weight;
            if(flag&VTX_TEX2    )vtx.tex2    (i)*=weight;
            if(flag&VTX_TEX3    )vtx.tex3    (i)*=weight;
            if(flag&VTX_SIZE    )vtx.size    (i)*=weight;
            if(flag&VTX_COLOR   )vtx.color   (i) =weight*color[i];
         }
      }

      // set duplicate vertex values
      REPA(vtx)
      {
         Int d=vtx.dup(i); if(d!=i) // duplicate
         {
            if(flag&VTX_POS     )vtx.pos     (i)=vtx.pos     (d);
            if(flag&VTX_MATERIAL)vtx.material(i)=vtx.material(d);
            if(flag&VTX_MATRIX  )vtx.matrix  (i)=vtx.matrix  (d);
            if(flag&VTX_BLEND   )vtx.blend   (i)=vtx.blend   (d);
            if(flag&VTX_NRM     )vtx.nrm     (i)=vtx.nrm     (d);
            if(flag&VTX_TAN     )vtx.tan     (i)=vtx.tan     (d);
            if(flag&VTX_BIN     )vtx.bin     (i)=vtx.bin     (d);
            if(flag&VTX_HLP     )vtx.hlp     (i)=vtx.hlp     (d);
            if(flag&VTX_TEX0    )vtx.tex0    (i)=vtx.tex0    (d);
            if(flag&VTX_TEX1    )vtx.tex1    (i)=vtx.tex1    (d);
            if(flag&VTX_TEX2    )vtx.tex2    (i)=vtx.tex2    (d);
            if(flag&VTX_TEX3    )vtx.tex3    (i)=vtx.tex3    (d);
            if(flag&VTX_SIZE    )vtx.size    (i)=vtx.size    (d);
            if(flag&VTX_COLOR   )vtx.color   (i)=vtx.color   (d);
         }
      }

      // remove degenerate faces
      if((flag&VTX_POS) && remove_degenerate_faces_eps>=0)removeDegenerateFaces(remove_degenerate_faces_eps);
   }
   return T;
}
/******************************************************************************/
void MeshBase::copyVtx(Int i, MeshBase &dest, Int dest_i)C
{
   if(InRange(i, vtx) && InRange(dest_i, dest.vtx))
   {
      if(dest.vtx.pos     ())dest.vtx.pos     (dest_i)=(vtx.pos     () ? vtx.pos     (i) : 0);
      if(dest.vtx.nrm     ())dest.vtx.nrm     (dest_i)=(vtx.nrm     () ? vtx.nrm     (i) : 0);
      if(dest.vtx.tan     ())dest.vtx.tan     (dest_i)=(vtx.tan     () ? vtx.tan     (i) : 0);
      if(dest.vtx.bin     ())dest.vtx.bin     (dest_i)=(vtx.bin     () ? vtx.bin     (i) : 0);
      if(dest.vtx.hlp     ())dest.vtx.hlp     (dest_i)=(vtx.hlp     () ? vtx.hlp     (i) : 0);
      if(dest.vtx.tex0    ())dest.vtx.tex0    (dest_i)=(vtx.tex0    () ? vtx.tex0    (i) : 0);
      if(dest.vtx.tex1    ())dest.vtx.tex1    (dest_i)=(vtx.tex1    () ? vtx.tex1    (i) : 0);
      if(dest.vtx.tex2    ())dest.vtx.tex2    (dest_i)=(vtx.tex2    () ? vtx.tex2    (i) : 0);
      if(dest.vtx.tex3    ())dest.vtx.tex3    (dest_i)=(vtx.tex3    () ? vtx.tex3    (i) : 0);
      if(dest.vtx.matrix  ())dest.vtx.matrix  (dest_i)=(vtx.matrix  () ? vtx.matrix  (i) : 0);
      if(dest.vtx.blend   ())dest.vtx.blend   (dest_i)=(vtx.blend   () ? vtx.blend   (i) : VecB4(255, 0, 0, 0)); // !! sum must be equal to 255 !!
      if(dest.vtx.size    ())dest.vtx.size    (dest_i)=(vtx.size    () ? vtx.size    (i) : 0);
      if(dest.vtx.material())dest.vtx.material(dest_i)=(vtx.material() ? vtx.material(i) : 0);
      if(dest.vtx.color   ())dest.vtx.color   (dest_i)=(vtx.color   () ? vtx.color   (i) : TRANSPARENT);
      if(dest.vtx.flag    ())dest.vtx.flag    (dest_i)=(vtx.flag    () ? vtx.flag    (i) : 0);
      if(dest.vtx.dup     ())dest.vtx.dup     (dest_i)=(vtx.dup     () ? vtx.dup     (i) : dest_i);
   }
}
/******************************************************************************/
struct WeldEdge
{
   Int   id;
   UInt  flag;
   VecI2 ind;
   Vec   nrm;
};
static Int CompareWeldEdge(C WeldEdge &e0, C WeldEdge &e1)
{
   if(e0.ind.c[0]<e1.ind.c[0])return -1;
   if(e0.ind.c[0]>e1.ind.c[0])return +1;
   if(e0.ind.c[1]<e1.ind.c[1])return -1;
   if(e0.ind.c[1]>e1.ind.c[1])return +1;
   return 0;
}
MeshBase& MeshBase::weldEdge()
{
   if(edge.ind())
   {
      // copy edges
      Memt<Bool>     edge_is; edge_is.setNum(edges()); SetMem(edge_is.data(), 1, edge_is.elms());
      Memt<WeldEdge> w_edge ; w_edge .setNum(edges());
      FREPA(edge)
      {
                        w_edge[i].ind =edge.ind (i);
         if(edge.id  ())w_edge[i].id  =edge.id  (i);
         if(edge.nrm ())w_edge[i].nrm =edge.nrm (i);
         if(edge.flag())w_edge[i].flag=edge.flag(i);
         if(w_edge[i].ind.c[0]>w_edge[i].ind.c[1])
         {
                            w_edge[i].ind.reverse();
                          //w_edge[i].id .reverse();
            if(!edge.flag())w_edge[i].nrm.chs    ();
                            w_edge[i].flag=EtqFlagSwap(w_edge[i].flag);
         }
      }

      // check for duplicate edges
      w_edge.sort(CompareWeldEdge);
      FREPA(edge)if(edge_is[i])
      {
         Int ind0=w_edge[i].ind.c[0];
         for(Int j=i+1; j<edges(); j++)if(edge_is[j])
         {
            if(w_edge[j].ind.c[0]!=ind0)break;
            if(w_edge[j].ind.c[1]==w_edge[i].ind.c[1])
            {
               w_edge [i].nrm +=w_edge[j].nrm;
               w_edge [i].flag|=w_edge[j].flag;
             //w_edge [i].id OP w_edge[j].id;
               edge_is[j]=0;
            }
         }
      }

      // create
      Int edges =CountIs(edge_is);
      if( edges!=T.edges())
      {
         Bool nrm = (edge.nrm ()!=null),
              flag= (edge.flag()!=null),
              id  = (edge.id  ()!=null);
         Int  num =T.edges();
         exclude(EDGE_ALL|FACE_ADJ_EDGE); edge._elms=edges;
         include(EDGE_IND| (nrm ? EDGE_NRM : MESH_NONE) | (flag ? EDGE_FLAG : MESH_NONE) | (id ? EDGE_ID : MESH_NONE));
         edges=0;         FREP(num)if(edge_is[i])edge.ind (edges++)= w_edge[i].ind;
         edges=0; if(nrm )FREP(num)if(edge_is[i])edge.nrm (edges++)=!w_edge[i].nrm;
         edges=0; if(flag)FREP(num)if(edge_is[i])edge.flag(edges++)= w_edge[i].flag;
         edges=0; if(id  )FREP(num)if(edge_is[i])edge.id  (edges++)= w_edge[i].id;
      }
   }
   return T;
}
/******************************************************************************/
MeshBase& MeshBase::explodeVtxs()
{
   MeshBase temp(edges()*2 + tris()*3 + quads()*4, edges(), tris(), quads(), flag()&~VTX_DUP);
   Int v=0;
   temp.copyEdges(T); FREPA(temp.edge){VecI2 &edge=temp.edge.ind(i); FREPA(edge){copyVtx(edge.c[i], temp, v); edge.c[i]=v++;}}
   temp.copyTris (T); FREPA(temp.tri ){VecI  &tri =temp.tri .ind(i); FREPA(tri ){copyVtx(tri .c[i], temp, v); tri .c[i]=v++;}}
   temp.copyQuads(T); FREPA(temp.quad){VecI4 &quad=temp.quad.ind(i); FREPA(quad){copyVtx(quad.c[i], temp, v); quad.c[i]=v++;}}
   Swap(temp, T);
   return T;
}
/******************************************************************************/
// TESSELATE
/******************************************************************************/
static INLINE Flt Crease(C Vec &n) {Flt l=n.length2(); return l ? 1/Sqrt(l) : 0;} // this function will make the formula work as if 'n' is normalized and then final value scaled by original length, scale is performed to avoid cases where both normal vectors are similar and its cross product has very small length (direction could be incorrect due to precision issues)
struct TriHull
{
   Vec P0, P1, P2,
       N0, N1, N2;

#if 0 // unoptimized original version
   Vec B210, B120, B021, B012, B102, B201, B111,
       N110, N011, N101;

   void set01()
   {
		B210=(2*P0 + P1 - Dot(P1-P0, N0)*N0)/3;
		B120=(2*P1 + P0 - Dot(P0-P1, N1)*N1)/3;
   }
   void set12()
   {
		B021=(2*P1 + P2 - Dot(P2-P1, N1)*N1)/3;
		B012=(2*P2 + P1 - Dot(P1-P2, N2)*N2)/3;
   }
   void set20()
   {
		B102=(2*P2 + P0 - Dot(P0-P2, N2)*N2)/3;
		B201=(2*P0 + P2 - Dot(P2-P0, N0)*N0)/3;
   }
   void set01(C Vec &NA, C Vec &NB)
   {
      Vec N=CrossN(N0, NA); B210=P0+(Dot(P1-P0, N)*Crease(N)/3)*N;
          N=CrossN(N1, NB); B120=P1+(Dot(P0-P1, N)*Crease(N)/3)*N;
   }
   void set12(C Vec &NA, C Vec &NB)
   {
      Vec N=CrossN(N1, NA); B021=P1+(Dot(P2-P1, N)*Crease(N)/3)*N;
          N=CrossN(N2, NB); B012=P2+(Dot(P1-P2, N)*Crease(N)/3)*N;
   }
   void set20(C Vec &NA, C Vec &NB)
   {
      Vec N=CrossN(N2, NA); B102=P2+(Dot(P0-P2, N)*Crease(N)/3)*N;
          N=CrossN(N0, NB); B201=P0+(Dot(P2-P0, N)*Crease(N)/3)*N;
   }
   void finalize()
   {
		Vec E=(B210+B120+B021+B012+B102+B201)/6,
		    V=(P0+P1+P2)/3;
      B111=E+(E-V)/2;

		Flt V01=2 * Dot(P1-P0, N0+N1) / Dot(P1-P0, P1-P0); N110=N0 + N1 - V01*(P1-P0); N110.normalize();
		Flt V12=2 * Dot(P2-P1, N1+N2) / Dot(P2-P1, P2-P1); N011=N1 + N2 - V12*(P2-P1); N011.normalize();
		Flt V20=2 * Dot(P0-P2, N2+N0) / Dot(P0-P2, P0-P2); N101=N2 + N0 - V20*(P0-P2); N101.normalize();
   }
   void set(Vec &pos, Vec &nrm, Flt U, Flt V)
   {
      Flt W=1-U-V,
          UU=Sqr(U), VV=Sqr(V), WW=Sqr(W),
          UU3=UU*3,
          VV3=VV*3,
          WW3=WW*3;

      pos=P0   * (WW * W)
        + P1   * (UU * U)
        + P2   * (VV * V)
        + B210 * (WW3 * U)
        + B120 * (W * UU3)
        + B201 * (WW3 * V)
        + B021 * (UU3 * V)
        + B102 * (W * VV3)
        + B012 * (U * VV3)
        + B111 * (6 * W * U * V);
    
      nrm=N0   *  WW
        + N1   *  UU
        + N2   *  VV
        + N110 * (W * U * 2)  // *2 makes sure that sum of all weights is 1 and that normals from tesselated triangles will match normals from tesselated quads (original version didn't have this)
        + N011 * (U * V * 2)  // *2 makes sure that sum of all weights is 1 and that normals from tesselated triangles will match normals from tesselated quads (original version didn't have this)
        + N101 * (W * V * 2); // *2 makes sure that sum of all weights is 1 and that normals from tesselated triangles will match normals from tesselated quads (original version didn't have this)
      nrm.normalize();
   }
#else // optimized, where all B* are 3x bigger, except 'B111' which is 6x bigger, and N*** are 2x bigger
   Vec B210_3, B120_3, B021_3, B012_3, B102_3, B201_3, B111_6,
       N110_2, N011_2, N101_2;

   void set01()
   {
		B210_3=2*P0 + P1 - Dot(P1-P0, N0)*N0;
		B120_3=2*P1 + P0 - Dot(P0-P1, N1)*N1;
   }
   void set12()
   {
		B021_3=2*P1 + P2 - Dot(P2-P1, N1)*N1;
		B012_3=2*P2 + P1 - Dot(P1-P2, N2)*N2;
   }
   void set20()
   {
		B102_3=2*P2 + P0 - Dot(P0-P2, N2)*N2;
		B201_3=2*P0 + P2 - Dot(P2-P0, N0)*N0;
   }
   void set01(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N0, NA); B210_3=P0*3+(Dot(P1-P0, N)*Crease(N))*N;
          N=Cross(N1, NB); B120_3=P1*3+(Dot(P0-P1, N)*Crease(N))*N;
   }
   void set12(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N1, NA); B021_3=P1*3+(Dot(P2-P1, N)*Crease(N))*N;
          N=Cross(N2, NB); B012_3=P2*3+(Dot(P1-P2, N)*Crease(N))*N;
   }
   void set20(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N2, NA); B102_3=P2*3+(Dot(P0-P2, N)*Crease(N))*N;
          N=Cross(N0, NB); B201_3=P0*3+(Dot(P2-P0, N)*Crease(N))*N;
   }
   void finalize()
   {
		Vec E=B210_3+B120_3+B021_3+B012_3+B102_3+B201_3,
		    V=P0+P1+P2;
      B111_6=E*0.5f-V;

		Flt V01=2 * Dot(P1-P0, N0+N1) / Dot(P1-P0, P1-P0); N110_2=N0 + N1 - V01*(P1-P0); N110_2.setLength(2);
		Flt V12=2 * Dot(P2-P1, N1+N2) / Dot(P2-P1, P2-P1); N011_2=N1 + N2 - V12*(P2-P1); N011_2.setLength(2);
		Flt V20=2 * Dot(P0-P2, N2+N0) / Dot(P0-P2, P0-P2); N101_2=N2 + N0 - V20*(P0-P2); N101_2.setLength(2);
   }
   void set(Vec &pos, Vec &nrm, Flt U, Flt V)
   {
      Flt W=1-U-V,
          UU=Sqr(U), VV=Sqr(V), WW=Sqr(W);

      pos=P0     * (WW * W)
        + P1     * (UU * U)
        + P2     * (VV * V)
        + B210_3 * (WW * U)
        + B120_3 * (W * UU)
        + B201_3 * (WW * V)
        + B021_3 * (UU * V)
        + B102_3 * (W * VV)
        + B012_3 * (U * VV)
        + B111_6 * (W * U * V);
    
      nrm=N0     *  WW
        + N1     *  UU
        + N2     *  VV
        + N110_2 * (W * U)
        + N011_2 * (U * V)
        + N101_2 * (W * V);
      nrm.normalize();
   }
#endif
   inline void set(Vec &pos, Vec &nrm, C Vec2 &uv) {set(    pos,     nrm, uv.x, uv.y);}
   inline void set(VtxFull &vtx      , C Vec2 &uv) {set(vtx.pos, vtx.nrm, uv.x, uv.y);}

   Vec2 uv(Int edge)C // get UV based on edge index in triangle in clockwise order
   {
      switch(edge)
      {
         default: return Vec2(0.5f, 0   ); // 0
         case  1: return Vec2(0.5f, 0.5f);
         case  2: return Vec2(0   , 0.5f);
      }
   }
};
struct QuadHull
{
   Vec P0, P1, P2, P3,   B01, B10, B12, B21, B23, B32, B30, B03,   B02, B13, B20, B31,
       N0, N1, N2, N3,   N01, N12, N23, N30, N0123;

   void set01()
   {
      B01=(2*P0 + P1 - Dot(P1 - P0, N0)*N0)/3;
      B10=(2*P1 + P0 - Dot(P0 - P1, N1)*N1)/3;
   }
   void set12()
   {
      B12=(2*P1 + P2 - Dot(P2 - P1, N1)*N1)/3;
      B21=(2*P2 + P1 - Dot(P1 - P2, N2)*N2)/3;
   }
   void set23()
   {
      B23=(2*P2 + P3 - Dot(P3 - P2, N2)*N2)/3;
      B32=(2*P3 + P2 - Dot(P2 - P3, N3)*N3)/3;
   }
   void set30()
   {
      B30=(2*P3 + P0 - Dot(P0 - P3, N3)*N3)/3;
      B03=(2*P0 + P3 - Dot(P3 - P0, N0)*N0)/3;
   }
   void set01(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N0, NA); B01=P0+(Dot(P1-P0, N)*Crease(N)/3)*N;
          N=Cross(N1, NB); B10=P1+(Dot(P0-P1, N)*Crease(N)/3)*N;
   }
   void set12(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N1, NA); B12=P1+(Dot(P2-P1, N)*Crease(N)/3)*N;
          N=Cross(N2, NB); B21=P2+(Dot(P1-P2, N)*Crease(N)/3)*N;
   }
   void set23(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N2, NA); B23=P2+(Dot(P3-P2, N)*Crease(N)/3)*N;
          N=Cross(N3, NB); B32=P3+(Dot(P2-P3, N)*Crease(N)/3)*N;
   }
   void set30(C Vec &NA, C Vec &NB)
   {
      Vec N=Cross(N3, NA); B30=P3+(Dot(P0-P3, N)*Crease(N)/3)*N;
          N=Cross(N0, NB); B03=P0+(Dot(P3-P0, N)*Crease(N)/3)*N;
   }
   void finalize()
   {
      Vec Q=B01+B10+B12+B21+B23+B32+B30+B03;

      Vec E0=(2*(B01+B03+Q) - (B21+B23))/18, V0=(4*P0 + 2*(P3+P1) + P2)/9; B02=E0+(E0-V0)/2;
      Vec E1=(2*(B12+B10+Q) - (B32+B30))/18, V1=(4*P1 + 2*(P0+P2) + P3)/9; B13=E1+(E1-V1)/2;
      Vec E2=(2*(B23+B21+Q) - (B03+B01))/18, V2=(4*P2 + 2*(P1+P3) + P0)/9; B20=E2+(E2-V2)/2;
      Vec E3=(2*(B30+B32+Q) - (B10+B12))/18, V3=(4*P3 + 2*(P2+P0) + P1)/9; B31=E3+(E3-V3)/2;

      Flt V01=2*(Dot(P1-P0, N0+N1) / Dot(P1-P0, P1-P0)); N01=N0 + N1 - V01*(P1-P0); N01.normalize();
      Flt V12=2*(Dot(P2-P1, N1+N2) / Dot(P2-P1, P2-P1)); N12=N1 + N2 - V12*(P2-P1); N12.normalize();
      Flt V23=2*(Dot(P3-P2, N2+N3) / Dot(P3-P2, P3-P2)); N23=N2 + N3 - V23*(P3-P2); N23.normalize();
      Flt V30=2*(Dot(P0-P3, N3+N0) / Dot(P0-P3, P0-P3)); N30=N3 + N0 - V30*(P0-P3); N30.normalize();
      N0123=2*(N01+N12+N23+N30) + (N0+N1+N2+N3); N0123.normalize();
   }
   void set(Vec &pos, Vec &nrm, Flt U, Flt V)
   {
      Flt BU0=(1-U) * (1-U) * (1-U),
          BU1=3 * U * (1-U) * (1-U),
          BU2=3 * U * U * (1-U),
          BU3=U * U * U,
          BV0=(1-V) * (1-V) * (1-V),
          BV1=3 * V * (1-V) * (1-V),
          BV2=3 * V * V * (1-V),
          BV3=V * V * V;

      pos=BV0*(BU0*P0  + BU1*B01 + BU2*B10 + BU3*P1 )
        + BV1*(BU0*B03 + BU1*B02 + BU2*B13 + BU3*B12)
        + BV2*(BU0*B30 + BU1*B31 + BU2*B20 + BU3*B21)
        + BV3*(BU0*P3  + BU1*B32 + BU2*B23 + BU3*P2 );
    
      Flt NU0=Sqr(1-U),
          NU2=Sqr(  U),
          NU1=2*U*(1-U), // 1-NU0-NU2
          NV0=Sqr(1-V),
          NV2=Sqr(  V),
          NV1=2*V*(1-V); // 1-NV0-NV2

      nrm=NV0*(NU0*N0  + NU1*N01   + NU2*N1 )
        + NV1*(NU0*N30 + NU1*N0123 + NU2*N12)
        + NV2*(NU0*N3  + NU1*N23   + NU2*N2 );

      nrm.normalize();
   }
   inline void set(Vec &pos, Vec &nrm, C Vec2 &uv) {set(    pos,     nrm, uv.x, uv.y);}
   inline void set(VtxFull &vtx      , C Vec2 &uv) {set(vtx.pos, vtx.nrm, uv.x, uv.y);}

   Vec2 uvCenter(        )C {return Vec2(0.5f, 0.5f);} // get UV for quad center
   Vec2 uv      (Int edge)C // get UV based on edge index in quad in clockwise order
   {
      switch(edge)
      {
         default: return Vec2(0.5f, 0   ); // 0
         case  1: return Vec2(1   , 0.5f);
         case  2: return Vec2(0.5f, 1   );
         case  3: return Vec2(0   , 0.5f);
      }
   }
};
static Bool SameNrm(C MeshBase &mesh, Int face, Int v0, Int v1, C Vec &test_n0, C Vec &test_n1, Vec &out_n0, Vec &out_n1) // check if the neighboring 'face' face has the same vtx normals for 'v0,v1' vertexes
{
   if(face==-1)return true; // compare to -1 and not >=0 because it can have SIGN_BIT, if there's no adjacent face, then return true so we can use default version
   Int   face_vtxs;
   VecI4 face_ind;
   if(face&SIGN_BIT) // quad
   {
      face_vtxs=4;
      face_ind =mesh.quad.ind(face^SIGN_BIT);
   }else
   {
      face_vtxs   =3;
      face_ind.xyz=mesh.tri.ind(face);
   }
   REP(face_vtxs) // find the same edge
   {
      Int test_v1=face_ind.c[i]; if((mesh.vtx.dup() ? mesh.vtx.dup(test_v1) : test_v1)==v1) // first check for v1, because we're checking the neighbor, so it should have vertexes in rotated order
      {
         Int test_v0=face_ind.c[(i+1)%face_vtxs]; if((mesh.vtx.dup() ? mesh.vtx.dup(test_v0) : test_v0)==v0) // now check for v0
         {
            out_n0=mesh.vtx.nrm(test_v0);
            out_n1=mesh.vtx.nrm(test_v1);
            return Dot(test_n0, out_n0)>=EPS_COL_COS && Dot(test_n1, out_n1)>=EPS_COL_COS; // check if both normals are the same
         }
         break;
      }
   }
   return true;
}
MeshBase& MeshBase::tesselate(Flt weld_pos_eps)
{
   if(vtx.pos())
   {
      setVtxDup(MESH_NONE, weld_pos_eps).setAdjacencies(true);

      // vtxs are created in following order [original_vtxs, tri edges, quad edges & centers]
      Int vtxs=T.vtxs (),
          tris=T.tris (),
         quads=T.quads();
      MeshBase temp; temp.create(vtxs + tris*3 + quads*5, 0, tris*4, quads*4, flag()&~(VTX_DUP|EDGE_ALL|ADJ_ALL|FACE_NRM));
      MemtN<IndexWeight, 256> skin;

      Int  * vtx_dup=     vtx.dup ();
      Vec  * pos_src=temp.vtx.pos (),
           * pos_dst=temp.vtx.pos (),
           * nrm_src=temp.vtx.nrm (),
           * nrm_dst=temp.vtx.nrm (),
           * tan_src=temp.vtx.tan (),
           * tan_dst=temp.vtx.tan (),
           * bin_src=temp.vtx.bin (),
           * bin_dst=temp.vtx.bin (),
           * hlp_src=temp.vtx.hlp (),
           * hlp_dst=temp.vtx.hlp ();
      Vec2 *tex0_src=temp.vtx.tex0(),
           *tex0_dst=temp.vtx.tex0(),
           *tex1_src=temp.vtx.tex1(),
           *tex1_dst=temp.vtx.tex1(),
           *tex2_src=temp.vtx.tex2(),
           *tex2_dst=temp.vtx.tex2(),
           *tex3_src=temp.vtx.tex3(),
           *tex3_dst=temp.vtx.tex3();

      VecB4 *mtrx_src=temp.vtx.matrix  (),
            *mtrx_dst=temp.vtx.matrix  ();
      VecB4 *blnd_src=temp.vtx.blend   (),
            *blnd_dst=temp.vtx.blend   ();
      Flt   *size_src=temp.vtx.size    (),
            *size_dst=temp.vtx.size    ();
      VecB4 *mtrl_src=temp.vtx.material(),
            *mtrl_dst=temp.vtx.material();
      Color * col_src=temp.vtx.color   (),
            * col_dst=temp.vtx.color   ();
      Byte  *flag_src=temp.vtx.flag    (),
            *flag_dst=temp.vtx.flag    ();

      VecI  *tri_src   =     tri .ind    (),
            *tri_dst   =temp.tri .ind    (),
            *tri_adjf  =     tri .adjFace();
      Byte  *tri_fl_src=     tri .flag   (),
            *tri_fl_dst=temp.tri .flag   ();
      Int   *tri_id_src=     tri .id     (),
            *tri_id_dst=temp.tri .id     ();
      VecI4 *qud_src   =     quad.ind    (),
            *qud_dst   =temp.quad.ind    (),
            *qud_adjf  =     quad.adjFace();
      Byte  *qud_fl_src=     quad.flag   (),
            *qud_fl_dst=temp.quad.flag   ();
      Int   *qud_id_src=     quad.id     (),
            *qud_id_dst=temp.quad.id     ();

      // original vtxs
      if( pos_dst){CopyN( pos_dst, vtx.pos     (), vtxs);  pos_dst+=vtxs;}
      if( nrm_dst){CopyN( nrm_dst, vtx.nrm     (), vtxs);  nrm_dst+=vtxs;}
      if( tan_dst){CopyN( tan_dst, vtx.tan     (), vtxs);  tan_dst+=vtxs;}
      if( bin_dst){CopyN( bin_dst, vtx.bin     (), vtxs);  bin_dst+=vtxs;}
      if( hlp_dst){CopyN( hlp_dst, vtx.hlp     (), vtxs);  hlp_dst+=vtxs;}
      if(tex0_dst){CopyN(tex0_dst, vtx.tex0    (), vtxs); tex0_dst+=vtxs;}
      if(tex1_dst){CopyN(tex1_dst, vtx.tex1    (), vtxs); tex1_dst+=vtxs;}
      if(tex2_dst){CopyN(tex2_dst, vtx.tex2    (), vtxs); tex2_dst+=vtxs;}
      if(tex3_dst){CopyN(tex3_dst, vtx.tex3    (), vtxs); tex3_dst+=vtxs;}
      if(mtrx_dst){CopyN(mtrx_dst, vtx.matrix  (), vtxs); mtrx_dst+=vtxs;}
      if(blnd_dst){CopyN(blnd_dst, vtx.blend   (), vtxs); blnd_dst+=vtxs;}
      if(size_dst){CopyN(size_dst, vtx.size    (), vtxs); size_dst+=vtxs;}
      if(mtrl_dst){CopyN(mtrl_dst, vtx.material(), vtxs); mtrl_dst+=vtxs;}
      if( col_dst){CopyN( col_dst, vtx.color   (), vtxs);  col_dst+=vtxs;}
      if(flag_dst){CopyN(flag_dst, vtx.flag    (), vtxs); flag_dst+=vtxs;}

      // set faces
      FREP(tris)
      {
         VecI ind=tri_src[i];

         tri_dst[0].set(ind.x , vtxs+0, vtxs+2);
         tri_dst[1].set(vtxs+0, ind.y , vtxs+1);
         tri_dst[2].set(vtxs+0, vtxs+1, vtxs+2);
         tri_dst[3].set(vtxs+1, ind.z , vtxs+2);
         tri_dst+=4;

         if(tri_fl_dst)
         {
            tri_fl_dst[0]=tri_fl_dst[1]=tri_fl_dst[2]=tri_fl_dst[3]=*tri_fl_src++;
            tri_fl_dst+=4;
         }
         if(tri_id_dst)
         {
            tri_id_dst[0]=tri_id_dst[1]=tri_id_dst[2]=tri_id_dst[3]=*tri_id_src++;
            tri_id_dst+=4;
         }

         if(nrm_src)
         {
            TriHull hull;
            hull.P0=pos_src[ind.x];
            hull.P1=pos_src[ind.y];
            hull.P2=pos_src[ind.z];
            hull.N0=nrm_src[ind.x];
		      hull.N1=nrm_src[ind.y];
		      hull.N2=nrm_src[ind.z];

            VecI af=tri_adjf[i], ind_dup=ind; ind_dup.remap(vtx_dup); Vec NA, NB;
            if(SameNrm(T, af.x, ind_dup.x, ind_dup.y, hull.N0, hull.N1, NA, NB))hull.set01();else hull.set01(NA, NB);
            if(SameNrm(T, af.y, ind_dup.y, ind_dup.z, hull.N1, hull.N2, NA, NB))hull.set12();else hull.set12(NA, NB);
            if(SameNrm(T, af.z, ind_dup.z, ind_dup.x, hull.N2, hull.N0, NA, NB))hull.set20();else hull.set20(NA, NB);

            hull.finalize();

            hull.set(pos_dst[0], nrm_dst[0], 0.5f,    0);
            hull.set(pos_dst[1], nrm_dst[1], 0.5f, 0.5f);
            hull.set(pos_dst[2], nrm_dst[2],    0, 0.5f);
            pos_dst+=3;
            nrm_dst+=3;
         }else
         {
            Vec P0=pos_src[ind.x], P1=pos_src[ind.y], P2=pos_src[ind.z];
            pos_dst[0]=Avg(P0, P1);
            pos_dst[1]=Avg(P1, P2);
            pos_dst[2]=Avg(P2, P0);
            pos_dst+=3;
         }
         if(tan_dst)
         {
            Vec t0=tan_src[ind.x], t1=tan_src[ind.y], t2=tan_src[ind.z];
            tan_dst[0]=!(t0+t1);
            tan_dst[1]=!(t1+t2);
            tan_dst[2]=!(t2+t0);
            tan_dst+=3;
         }
         if(bin_dst)
         {
            Vec b0=bin_src[ind.x], b1=bin_src[ind.y], b2=bin_src[ind.z];
            bin_dst[0]=!(b0+b1);
            bin_dst[1]=!(b1+b2);
            bin_dst[2]=!(b2+b0);
            bin_dst+=3;
         }
         if(hlp_dst)
         {
            Vec h0=hlp_src[ind.x], h1=hlp_src[ind.y], h2=hlp_src[ind.z];
            hlp_dst[0]=Avg(h0, h1);
            hlp_dst[1]=Avg(h1, h2);
            hlp_dst[2]=Avg(h2, h0);
            hlp_dst+=3;
         }
         if(tex0_dst)
         {
            Vec2 t0=tex0_src[ind.x], t1=tex0_src[ind.y], t2=tex0_src[ind.z];
            tex0_dst[0]=Avg(t0, t1);
            tex0_dst[1]=Avg(t1, t2);
            tex0_dst[2]=Avg(t2, t0);
            tex0_dst+=3;
         }
         if(tex1_dst)
         {
            Vec2 t0=tex1_src[ind.x], t1=tex1_src[ind.y], t2=tex1_src[ind.z];
            tex1_dst[0]=Avg(t0, t1);
            tex1_dst[1]=Avg(t1, t2);
            tex1_dst[2]=Avg(t2, t0);
            tex1_dst+=3;
         }
         if(tex2_dst)
         {
            Vec2 t0=tex2_src[ind.x], t1=tex2_src[ind.y], t2=tex2_src[ind.z];
            tex2_dst[0]=Avg(t0, t1);
            tex2_dst[1]=Avg(t1, t2);
            tex2_dst[2]=Avg(t2, t0);
            tex2_dst+=3;
         }
         if(tex3_dst)
         {
            Vec2 t0=tex3_src[ind.x], t1=tex3_src[ind.y], t2=tex3_src[ind.z];
            tex3_dst[0]=Avg(t0, t1);
            tex3_dst[1]=Avg(t1, t2);
            tex3_dst[2]=Avg(t2, t0);
            tex3_dst+=3;
         }
         if(size_dst)
         {
            Flt s0=size_src[ind.x], s1=size_src[ind.y], s2=size_src[ind.z];
            size_dst[0]=Avg(s0, s1);
            size_dst[1]=Avg(s1, s2);
            size_dst[2]=Avg(s2, s0);
            size_dst+=3;
         }
         if(mtrl_dst)
         {
            VecB4 m0=mtrl_src[ind.x], m1=mtrl_src[ind.y], m2=mtrl_src[ind.z];
            mtrl_dst[0]=AvgI(m0, m1);
            mtrl_dst[1]=AvgI(m1, m2);
            mtrl_dst[2]=AvgI(m2, m0);
            mtrl_dst+=3;
         }
         if(col_dst)
         {
            Color c0=col_src[ind.x], c1=col_src[ind.y], c2=col_src[ind.z];
            col_dst[0]=Avg(c0, c1);
            col_dst[1]=Avg(c1, c2);
            col_dst[2]=Avg(c2, c0);
            col_dst+=3;
         }
         if(flag_dst)
         {
            Byte f0=flag_src[ind.x], f1=flag_src[ind.y], f2=flag_src[ind.z];
            flag_dst[0]=(f0|f1);
            flag_dst[1]=(f1|f2);
            flag_dst[2]=(f2|f0);
            flag_dst+=3;
         }
         if(mtrx_dst && blnd_dst)
         {
            VecB4 m0=mtrx_src[ind.x], m1=mtrx_src[ind.y], m2=mtrx_src[ind.z];
            VecB4 b0=blnd_src[ind.x], b1=blnd_src[ind.y], b2=blnd_src[ind.z];

            REP(4)skin.New().set(m0.c[i], b0.c[i]);
            REP(4)skin.New().set(m1.c[i], b1.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m1.c[i], b1.c[i]);
            REP(4)skin.New().set(m2.c[i], b2.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m2.c[i], b2.c[i]);
            REP(4)skin.New().set(m0.c[i], b0.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();
         }else
         {
            if(mtrx_dst)
            {
               mtrx_dst[0]=mtrx_src[ind.x]; // just copy from first point, because we need correct data and blending with sum of 255
               mtrx_dst[1]=mtrx_src[ind.y];
               mtrx_dst[2]=mtrx_src[ind.z];
               mtrx_dst+=3;
            }
            if(blnd_dst)
            {
               blnd_dst[0]=blnd_src[ind.x]; // just copy from first point, because we need correct data and blending with sum of 255
               blnd_dst[1]=blnd_src[ind.y];
               blnd_dst[2]=blnd_src[ind.z];
               blnd_dst+=3;
            }
         }
         vtxs+=3;
      }
      FREP(quads)
      {
         VecI4 ind=qud_src[i];

         qud_dst[0].set(ind.x, vtxs+0, vtxs+4, vtxs+3);
         qud_dst[1].set(vtxs+0, ind.y, vtxs+1, vtxs+4);
         qud_dst[2].set(vtxs+3, vtxs+4, vtxs+2, ind.w);
         qud_dst[3].set(vtxs+4, vtxs+1, ind.z, vtxs+2);
         qud_dst+=4;

         if(qud_fl_dst)
         {
            qud_fl_dst[0]=qud_fl_dst[1]=qud_fl_dst[2]=qud_fl_dst[3]=*qud_fl_src++;
            qud_fl_dst+=4;
         }
         if(qud_id_dst)
         {
            qud_id_dst[0]=qud_id_dst[1]=qud_id_dst[2]=qud_id_dst[3]=*qud_id_src++;
            qud_id_dst+=4;
         }

         if(nrm_src)
         {
            QuadHull hull;
            hull.P0=pos_src[ind.x];
            hull.P1=pos_src[ind.y];
            hull.P2=pos_src[ind.z];
            hull.P3=pos_src[ind.w];
            hull.N0=nrm_src[ind.x];
		      hull.N1=nrm_src[ind.y];
		      hull.N2=nrm_src[ind.z];
		      hull.N3=nrm_src[ind.w];

            VecI4 af=qud_adjf[i], ind_dup=ind; ind_dup.remap(vtx_dup); Vec NA, NB;
            if(SameNrm(T, af.x, ind_dup.x, ind_dup.y, hull.N0, hull.N1, NA, NB))hull.set01();else hull.set01(NA, NB);
            if(SameNrm(T, af.y, ind_dup.y, ind_dup.z, hull.N1, hull.N2, NA, NB))hull.set12();else hull.set12(NA, NB);
            if(SameNrm(T, af.z, ind_dup.z, ind_dup.w, hull.N2, hull.N3, NA, NB))hull.set23();else hull.set23(NA, NB);
            if(SameNrm(T, af.w, ind_dup.w, ind_dup.x, hull.N3, hull.N0, NA, NB))hull.set30();else hull.set30(NA, NB);

            hull.finalize();

            hull.set(pos_dst[0], nrm_dst[0], 0.5f, 0   );
            hull.set(pos_dst[1], nrm_dst[1], 1   , 0.5f);
            hull.set(pos_dst[2], nrm_dst[2], 0.5f, 1   );
            hull.set(pos_dst[3], nrm_dst[3], 0   , 0.5f);
            hull.set(pos_dst[4], nrm_dst[4], 0.5f, 0.5f);
            pos_dst+=5;
            nrm_dst+=5;
         }else
         {
            Vec P0=pos_src[ind.x], P1=pos_src[ind.y], P2=pos_src[ind.z], P3=pos_src[ind.w];
            pos_dst[0]=Avg(P0, P1);
            pos_dst[1]=Avg(P1, P2);
            pos_dst[2]=Avg(P2, P3);
            pos_dst[3]=Avg(P3, P0);
            pos_dst[4]=Avg(P0, P1, P2, P3);
            pos_dst+=5;
         }
         if(tan_dst)
         {
            Vec t0=tan_src[ind.x], t1=tan_src[ind.y], t2=tan_src[ind.z], t3=tan_src[ind.w];
            tan_dst[0]=!(t0+t1);
            tan_dst[1]=!(t1+t2);
            tan_dst[2]=!(t2+t3);
            tan_dst[3]=!(t3+t0);
            tan_dst[4]=!(t0+t1+t2+t3);
            tan_dst+=5;
         }
         if(bin_dst)
         {
            Vec b0=bin_src[ind.x], b1=bin_src[ind.y], b2=bin_src[ind.z], b3=bin_src[ind.w];
            bin_dst[0]=!(b0+b1);
            bin_dst[1]=!(b1+b2);
            bin_dst[2]=!(b2+b3);
            bin_dst[3]=!(b3+b0);
            bin_dst[4]=!(b0+b1+b2+b3);
            bin_dst+=5;
         }
         if(hlp_dst)
         {
            Vec h0=hlp_src[ind.x], h1=hlp_src[ind.y], h2=hlp_src[ind.z], h3=hlp_src[ind.w];
            hlp_dst[0]=Avg(h0, h1);
            hlp_dst[1]=Avg(h1, h2);
            hlp_dst[2]=Avg(h2, h3);
            hlp_dst[3]=Avg(h3, h0);
            hlp_dst[4]=Avg(h0, h1, h2, h3);
            hlp_dst+=5;
         }
         if(tex0_dst)
         {
            Vec2 t0=tex0_src[ind.x], t1=tex0_src[ind.y], t2=tex0_src[ind.z], t3=tex0_src[ind.w];
            tex0_dst[0]=Avg(t0, t1);
            tex0_dst[1]=Avg(t1, t2);
            tex0_dst[2]=Avg(t2, t3);
            tex0_dst[3]=Avg(t3, t0);
            tex0_dst[4]=Avg(t0, t1, t2, t3);
            tex0_dst+=5;
         }
         if(tex1_dst)
         {
            Vec2 t0=tex1_src[ind.x], t1=tex1_src[ind.y], t2=tex1_src[ind.z], t3=tex1_src[ind.w];
            tex1_dst[0]=Avg(t0, t1);
            tex1_dst[1]=Avg(t1, t2);
            tex1_dst[2]=Avg(t2, t3);
            tex1_dst[3]=Avg(t3, t0);
            tex1_dst[4]=Avg(t0, t1, t2, t3);
            tex1_dst+=5;
         }
         if(tex2_dst)
         {
            Vec2 t0=tex2_src[ind.x], t1=tex2_src[ind.y], t2=tex2_src[ind.z], t3=tex2_src[ind.w];
            tex2_dst[0]=Avg(t0, t1);
            tex2_dst[1]=Avg(t1, t2);
            tex2_dst[2]=Avg(t2, t3);
            tex2_dst[3]=Avg(t3, t0);
            tex2_dst[4]=Avg(t0, t1, t2, t3);
            tex2_dst+=5;
         }
         if(tex3_dst)
         {
            Vec2 t0=tex3_src[ind.x], t1=tex3_src[ind.y], t2=tex3_src[ind.z], t3=tex3_src[ind.w];
            tex3_dst[0]=Avg(t0, t1);
            tex3_dst[1]=Avg(t1, t2);
            tex3_dst[2]=Avg(t2, t3);
            tex3_dst[3]=Avg(t3, t0);
            tex3_dst[4]=Avg(t0, t1, t2, t3);
            tex3_dst+=5;
         }
         if(size_dst)
         {
            Flt s0=size_src[ind.x], s1=size_src[ind.y], s2=size_src[ind.z], s3=size_src[ind.w];
            size_dst[0]=Avg(s0, s1);
            size_dst[1]=Avg(s1, s2);
            size_dst[2]=Avg(s2, s3);
            size_dst[3]=Avg(s3, s0);
            size_dst[4]=Avg(s0, s1, s2, s3);
            size_dst+=5;
         }
         if(mtrl_dst)
         {
            VecB4 m0=mtrl_src[ind.x], m1=mtrl_src[ind.y], m2=mtrl_src[ind.z], m3=mtrl_src[ind.w];
            mtrl_dst[0]=AvgI(m0, m1);
            mtrl_dst[1]=AvgI(m1, m2);
            mtrl_dst[2]=AvgI(m2, m3);
            mtrl_dst[3]=AvgI(m3, m0);
            mtrl_dst[4]=AvgI(m0, m1, m2, m3);
            mtrl_dst+=5;
         }
         if(col_dst)
         {
            Color c0=col_src[ind.x], c1=col_src[ind.y], c2=col_src[ind.z], c3=col_src[ind.w];
            col_dst[0]=Avg(c0, c1);
            col_dst[1]=Avg(c1, c2);
            col_dst[2]=Avg(c2, c3);
            col_dst[3]=Avg(c3, c0);
            col_dst[4]=Avg(c0, c1, c2, c3);
            col_dst+=5;
         }
         if(flag_dst)
         {
            Byte f0=flag_src[ind.x], f1=flag_src[ind.y], f2=flag_src[ind.z], f3=flag_src[ind.w];
            flag_dst[0]=(f0|f1);
            flag_dst[1]=(f1|f2);
            flag_dst[2]=(f2|f3);
            flag_dst[3]=(f3|f0);
            flag_dst[4]=(f0|f1|f2|f3);
            flag_dst+=5;
         }
         if(mtrx_dst && blnd_dst)
         {
            VecB4 m0=mtrx_src[ind.x], m1=mtrx_src[ind.y], m2=mtrx_src[ind.z], m3=mtrx_src[ind.w];
            VecB4 b0=blnd_src[ind.x], b1=blnd_src[ind.y], b2=blnd_src[ind.z], b3=blnd_src[ind.w];

            REP(4)skin.New().set(m0.c[i], b0.c[i]);
            REP(4)skin.New().set(m1.c[i], b1.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m1.c[i], b1.c[i]);
            REP(4)skin.New().set(m2.c[i], b2.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m2.c[i], b2.c[i]);
            REP(4)skin.New().set(m3.c[i], b3.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m3.c[i], b3.c[i]);
            REP(4)skin.New().set(m0.c[i], b0.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();

            REP(4)skin.New().set(m0.c[i], b0.c[i]);
            REP(4)skin.New().set(m1.c[i], b1.c[i]);
            REP(4)skin.New().set(m2.c[i], b2.c[i]);
            REP(4)skin.New().set(m3.c[i], b3.c[i]);
            SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();
         }else
         {
            if(mtrx_dst)
            {
               mtrx_dst[0]=mtrx_src[ind.x]; // just copy from first point, because we need correct data and blending with sum of 255
               mtrx_dst[1]=mtrx_src[ind.y];
               mtrx_dst[2]=mtrx_src[ind.z];
               mtrx_dst[3]=mtrx_src[ind.w];
               mtrx_dst[4]=mtrx_src[ind.x];
               mtrx_dst+=5;
            }
            if(blnd_dst)
            {
               blnd_dst[0]=blnd_src[ind.x]; // just copy from first point, because we need correct data and blending with sum of 255
               blnd_dst[1]=blnd_src[ind.y];
               blnd_dst[2]=blnd_src[ind.z];
               blnd_dst[3]=blnd_src[ind.w];
               blnd_dst[4]=blnd_src[ind.x];
               blnd_dst+=5;
            }
         }
         vtxs+=5;
      }

      temp.weldVtx(VTX_ALL, weld_pos_eps, EPS_COL_COS, -1);
      Swap(temp, T);
   }
   return T;
}
MeshBase& MeshBase::tesselate(C CMemPtr<Int > &vtx_sel, Flt weld_pos_eps) {Memt<Bool> vtx_is; CreateIs(vtx_is, vtx_sel, vtxs()); return tesselate(vtx_is, weld_pos_eps);}
MeshBase& MeshBase::tesselate(C CMemPtr<Bool> &vtx_sel, Flt weld_pos_eps)
{
   if(C Vec *vtx_pos=vtx.pos())
   {
      setVtxDup(MESH_NONE, weld_pos_eps).setAdjacencies(true);

    C Vec   *vtx_nrm =vtx .nrm    ();
    C Int   *vtx_dup =vtx .dup    ();
    C VecI  *tri_adjf=tri .adjFace();
    C VecI4 *qud_adjf=quad.adjFace();

      Memc<VtxFull>  vtxs;
      Memc<VecI   >  tris;
      Memc<VecI4  > quads;
      Bool vs[4]; // cached copy of selected vertexes in a face

       vtxs.reserve(T. vtxs() + T.tris()*3 + T.quads()*5);
       tris.reserve(T. tris()*4);
      quads.reserve(T.quads()*4);

      FREPA(tri)
      {
         VecI ind=tri.ind(i);
         Int  tess_vtxs=0; REPA(ind)if(vs[i]=vtx_sel[ind.c[i]])tess_vtxs++; // how many vertexes want to be tesselated
         Int  ofs=vtxs.elms(); // vertex offset - how many vtxs created so far
         FREPA(ind)vtxs.New().from(T, ind.c[i]); // copy original vtxs
         if(tess_vtxs<2)tris.New().set(ofs, ofs+1, ofs+2);else // set as original
         {
            TriHull hull;
            if(vtx_nrm)
            {
               hull.P0=vtx_pos[ind.x];
               hull.P1=vtx_pos[ind.y];
               hull.P2=vtx_pos[ind.z];
               hull.N0=vtx_nrm[ind.x];
		         hull.N1=vtx_nrm[ind.y];
		         hull.N2=vtx_nrm[ind.z];

               VecI af=tri_adjf[i], ind_dup=ind; ind_dup.remap(vtx_dup); Vec NA, NB;
               if(SameNrm(T, af.x, ind_dup.x, ind_dup.y, hull.N0, hull.N1, NA, NB))hull.set01();else hull.set01(NA, NB);
               if(SameNrm(T, af.y, ind_dup.y, ind_dup.z, hull.N1, hull.N2, NA, NB))hull.set12();else hull.set12(NA, NB);
               if(SameNrm(T, af.z, ind_dup.z, ind_dup.x, hull.N2, hull.N0, NA, NB))hull.set20();else hull.set20(NA, NB);

               hull.finalize();
            }
            if(tess_vtxs==3) // if all want to be tesselated
            {
               vtxs.addNum(3); // make room for 3 vtxs at edges !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
               vtxs[ofs+3].avg(vtxs[ofs+0], vtxs[ofs+1]);
               vtxs[ofs+4].avg(vtxs[ofs+1], vtxs[ofs+2]);
               vtxs[ofs+5].avg(vtxs[ofs+2], vtxs[ofs+0]);
               if(vtx_nrm)
               {
                  hull.set(vtxs[ofs+3], hull.uv(0));
                  hull.set(vtxs[ofs+4], hull.uv(1));
                  hull.set(vtxs[ofs+5], hull.uv(2));
               }
               tris.New().set(ofs+0, ofs+3, ofs+5);
               tris.New().set(ofs+3, ofs+4, ofs+5);
               tris.New().set(ofs+5, ofs+4, ofs+2);
               tris.New().set(ofs+4, ofs+3, ofs+1);
            }else
            REPAD(v0, ind) // check which face vtxs want to be tesselated
               if(vs[v0]) // if 'v0' wants
            {
               Int v1=(v0+1)%Elms(ind); if(vs[v1]) // and 'v1' wants
               {
                  Int v2=(v0+2)%Elms(ind);
                  vtxs.addNum(1); // create 1 vtx at v0-v1 edge !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
                  vtxs[ofs+3].avg(vtxs[ofs+v0], vtxs[ofs+v1]);
                  if(vtx_nrm)hull.set(vtxs[ofs+3], hull.uv(v0));
                  tris.New().set(ofs+v0, ofs+3, ofs+v2);
                  tris.New().set(ofs+v2, ofs+3, ofs+v1);
                  break;
               }
            }
         }
      }

      FREPA(quad)
      {
         VecI4 ind=quad.ind(i);
         Int   tess_vtxs=0; REPA(ind)if(vs[i]=vtx_sel[ind.c[i]])tess_vtxs++; // how many vtxs want to be tesselated
         Int   ofs=vtxs.elms(); // vertex offset - how many vtxs created so far
         FREPA(ind)vtxs.New().from(T, ind.c[i]); // copy original vtxs
         if(tess_vtxs<2)quads.New().set(ofs, ofs+1, ofs+2, ofs+3);else // set as original
         {
            QuadHull hull;
            if(vtx_nrm)
            {
               hull.P0=vtx_pos[ind.x];
               hull.P1=vtx_pos[ind.y];
               hull.P2=vtx_pos[ind.z];
               hull.P3=vtx_pos[ind.w];
               hull.N0=vtx_nrm[ind.x];
		         hull.N1=vtx_nrm[ind.y];
		         hull.N2=vtx_nrm[ind.z];
		         hull.N3=vtx_nrm[ind.w];

               VecI4 af=qud_adjf[i], ind_dup=ind; ind_dup.remap(vtx_dup); Vec NA, NB;
               if(SameNrm(T, af.x, ind_dup.x, ind_dup.y, hull.N0, hull.N1, NA, NB))hull.set01();else hull.set01(NA, NB);
               if(SameNrm(T, af.y, ind_dup.y, ind_dup.z, hull.N1, hull.N2, NA, NB))hull.set12();else hull.set12(NA, NB);
               if(SameNrm(T, af.z, ind_dup.z, ind_dup.w, hull.N2, hull.N3, NA, NB))hull.set23();else hull.set23(NA, NB);
               if(SameNrm(T, af.w, ind_dup.w, ind_dup.x, hull.N3, hull.N0, NA, NB))hull.set30();else hull.set30(NA, NB);

               hull.finalize();
            }
            if(tess_vtxs==4) // if all want to be tesselated
            {
               vtxs.addNum(5); // make room for 4 vtxs at edges and 1 at center !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
               vtxs[ofs+4].avg(vtxs[ofs+0], vtxs[ofs+1]);
               vtxs[ofs+5].avg(vtxs[ofs+1], vtxs[ofs+2]);
               vtxs[ofs+6].avg(vtxs[ofs+2], vtxs[ofs+3]);
               vtxs[ofs+7].avg(vtxs[ofs+3], vtxs[ofs+0]);
               vtxs[ofs+8].avg(vtxs[ofs+0], vtxs[ofs+1], vtxs[ofs+2], vtxs[ofs+3]);
               if(vtx_nrm)
               {
                  hull.set(vtxs[ofs+4], hull.uv(0));
                  hull.set(vtxs[ofs+5], hull.uv(1));
                  hull.set(vtxs[ofs+6], hull.uv(2));
                  hull.set(vtxs[ofs+7], hull.uv(3));
                  hull.set(vtxs[ofs+8], hull.uvCenter());
               }
               quads.New().set(ofs+0, ofs+4, ofs+8, ofs+7);
               quads.New().set(ofs+1, ofs+5, ofs+8, ofs+4);
               quads.New().set(ofs+2, ofs+6, ofs+8, ofs+5);
               quads.New().set(ofs+3, ofs+7, ofs+8, ofs+6);
            }else
            REPAD(v0, ind) // check which face vtxs want to be tesselated
               if(vs[v0]) // if 'v0' wants
            {
               Int v1=(v0+1)%Elms(ind);
               Int v2=(v0+2)%Elms(ind);
               Int v3=(v0+3)%Elms(ind);
               if(tess_vtxs==3)
               {
                  if(vs[v1] && vs[v2]) // and 'v1' 'v2' want
                  {
                     vtxs.addNum(3); // create vtx at v0-v1 edge, v1-v2 edge, center !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
                     vtxs[ofs+4].avg(vtxs[ofs+v0], vtxs[ofs+v1]); // v0-v1
                     vtxs[ofs+5].avg(vtxs[ofs+v1], vtxs[ofs+v2]); // v1-v2
                     vtxs[ofs+6].avg(vtxs[ofs+ 0], vtxs[ofs+ 1], vtxs[ofs+2], vtxs[ofs+3]); // center
                     if(vtx_nrm)
                     {
                        hull.set(vtxs[ofs+4], hull.uv(v0));
                        hull.set(vtxs[ofs+5], hull.uv(v1));
                        hull.set(vtxs[ofs+6], hull.uvCenter());
                     }
                      tris.New().set(ofs+v3, ofs+v0, ofs+6); // big
                      tris.New().set(ofs+v2, ofs+v3, ofs+6); // big
                      tris.New().set(ofs+v0, ofs+ 4, ofs+6); // small
                      tris.New().set(ofs+ 5, ofs+v2, ofs+6); // small
                     quads.New().set(ofs+v1, ofs+ 5, ofs+6, ofs+4);
                     break;
                  }
               }else
             //if(tess_vtxs==2)
               {
                  if(vs[v1]) // and 'v1' wants
                  {
                     vtxs.addNum(1); // create vtx at v0-v1 edge !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
                     vtxs[ofs+4].avg(vtxs[ofs+v0], vtxs[ofs+v1]); // v0-v1
                     if(vtx_nrm)hull.set(vtxs[ofs+4], hull.uv(v0));
                     tris.New().set(ofs+v0, ofs+ 4, ofs+v3);
                     tris.New().set(ofs+ 4, ofs+v1, ofs+v2);
                     tris.New().set(ofs+v2, ofs+v3, ofs+ 4);
                     break;
                  }
                  if(vs[v2]) // and 'v2' wants
                  {
                     vtxs.addNum(1); // create vtx at center !! important to call this instead of "vtxs.New().avg(vtxs..)" because 'New' might change memory address !!
                     vtxs[ofs+4].avg(vtxs[ofs+0], vtxs[ofs+1], vtxs[ofs+2], vtxs[ofs+3]); // center
                     if(vtx_nrm)hull.set(vtxs[ofs+4], hull.uvCenter());
                     tris.New().set(ofs+v0, ofs+v1, ofs+4);
                     tris.New().set(ofs+v1, ofs+v2, ofs+4);
                     tris.New().set(ofs+v2, ofs+v3, ofs+4);
                     tris.New().set(ofs+v3, ofs+v0, ofs+4);
                     break;
                  }
               }
            }
         }
      }

      create(vtxs.elms(), 0, tris.elms(), quads.elms(), flag()&~(VTX_DUP|EDGE_ALL|ADJ_ALL|FACE_NRM));
      REPA( vtxs)vtxs[i].to(T, i);
      REPA( tris) tri.ind(i)= tris[i];
      REPA(quads)quad.ind(i)=quads[i];
      weldVtx(VTX_ALL, weld_pos_eps, EPS_COL_COS, -1);
   }
   return T;
}
/******************************************************************************/
// SUBDIVIDE
/******************************************************************************/
MeshBase& MeshBase::subdivide()
{
   const Bool tri_as_quad=false;
   // tri_as_quad:false -> vtxs are created in following order [original_vtxs,              quads_center, edges_center]
   // tri_as_quad:true  -> vtxs are created in following order [original_vtxs, tris_center, quads_center, edges_center]

   setAdjacencies(true, true);

   Int   *dup_vtx     =null;
   Int   *dup_edge_edg=null;
   Bool  *dup_edge_add=null;
   VecI2 *dup_edge_adj=null;

   if(vtx.dup())
   {
             dup_vtx        =vtx .dup    (); vtx ._dup     =null;
             dup_edge_adj   =edge.adjFace(); edge._adj_face=null;
      VecI  *dup_tri_adj_edg=tri .adjEdge(); tri ._adj_edge=null;
      VecI4 *dup_qud_adj_edg=quad.adjEdge(); quad._adj_edge=null;
      AllocZero     (dup_edge_add, edges());
      exclude       (VTX_DUP|EDGE_ALL|ADJ_ALL);
      setAdjacencies(true, true);
      Alloc         (dup_edge_edg, edges());
      REPA(edge)
      {
         Int *ef=edge.adjFace(i).c;
         REPD(f, 2)
         {
            Int face =ef[f];
            if( face!=-1) // compare to -1 and not >=0 because it can have SIGN_BIT
            {
               if(face&SIGN_BIT){Int *fe=quad.adjEdge(face^SIGN_BIT).c; REPD(e, 4)if(fe[e]==i){dup_edge_edg[i]=dup_qud_adj_edg[face^SIGN_BIT].c[e]; goto found;}}
               else             {Int *fe=tri .adjEdge(face         ).c; REPD(e, 3)if(fe[e]==i){dup_edge_edg[i]=dup_tri_adj_edg[face         ].c[e]; goto found;}}
            }
         }
         dup_edge_edg[i]=i;
      found:;
      }
      Free(dup_tri_adj_edg);
      Free(dup_qud_adj_edg);
   }

   Int     *p, p0, p1, p2, p3,
            vtxs=T.vtxs (),
           edges=T.edges(),
            tris=T.tris (),
           quads=T.quads(),
             tri_vtx_offset=vtxs,
            quad_vtx_offset= tri_vtx_offset+(tri_as_quad ? tris : 0),
            edge_vtx_offset=quad_vtx_offset+quads;
   MeshBase temp(vtxs + (tri_as_quad ? tris : 0)+quads + edges, 0, tri_as_quad ? 0 : tris*4, (tri_as_quad ? tris*3 : 0)+quads*4, (VTX_NRM_TAN_BIN|VTX_HLP|VTX_TEX0|VTX_TEX1|VTX_TEX2|VTX_TEX3|VTX_MATRIX|VTX_BLEND|VTX_SIZE|VTX_MATERIAL|VTX_COLOR|VTX_FLAG)&flag());
   MemtN<IndexWeight, 256> skin;

   Vec  * pos_src=temp.vtx.pos (),
        * pos_dst=temp.vtx.pos (),
        * nrm_src=temp.vtx.nrm (),
        * nrm_dst=temp.vtx.nrm (),
        * tan_src=temp.vtx.tan (),
        * tan_dst=temp.vtx.tan (),
        * bin_src=temp.vtx.bin (),
        * bin_dst=temp.vtx.bin (),
        * hlp_src=temp.vtx.hlp (),
        * hlp_dst=temp.vtx.hlp ();
   Vec2 *tex0_src=temp.vtx.tex0(),
        *tex0_dst=temp.vtx.tex0(),
        *tex1_src=temp.vtx.tex1(),
        *tex1_dst=temp.vtx.tex1(),
        *tex2_src=temp.vtx.tex2(),
        *tex2_dst=temp.vtx.tex2(),
        *tex3_src=temp.vtx.tex3(),
        *tex3_dst=temp.vtx.tex3();

   VecB4 *mtrx_src=temp.vtx.matrix  (),
         *mtrx_dst=temp.vtx.matrix  ();
   VecB4 *blnd_src=temp.vtx.blend   (),
         *blnd_dst=temp.vtx.blend   ();
   Flt   *size_src=temp.vtx.size    (),
         *size_dst=temp.vtx.size    ();
   VecB4 *mtrl_src=temp.vtx.material(),
         *mtrl_dst=temp.vtx.material();
   Color * col_src=temp.vtx.color   (),
         * col_dst=temp.vtx.color   ();
   Byte  *flag_src=temp.vtx.flag    (),
         *flag_dst=temp.vtx.flag    ();

   VecI2 *edg_src=     edge.ind    (),
         *edg_adj=     edge.adjFace();
   VecI  *tri_src=     tri .ind    (),
         *tri_dst=temp.tri .ind    ();
   VecI4 *qud_src=     quad.ind    (),
         *qud_dst=temp.quad.ind    ();

   // original vtxs
   if( pos_dst){CopyN( pos_dst, vtx.pos     (), vtxs);  pos_dst+=vtxs;}
   if( nrm_dst){CopyN( nrm_dst, vtx.nrm     (), vtxs);  nrm_dst+=vtxs;}
   if( tan_dst){CopyN( tan_dst, vtx.tan     (), vtxs);  tan_dst+=vtxs;}
   if( bin_dst){CopyN( bin_dst, vtx.bin     (), vtxs);  bin_dst+=vtxs;}
   if( hlp_dst){CopyN( hlp_dst, vtx.hlp     (), vtxs);  hlp_dst+=vtxs;}
   if(tex0_dst){CopyN(tex0_dst, vtx.tex0    (), vtxs); tex0_dst+=vtxs;}
   if(tex1_dst){CopyN(tex1_dst, vtx.tex1    (), vtxs); tex1_dst+=vtxs;}
   if(tex2_dst){CopyN(tex2_dst, vtx.tex2    (), vtxs); tex2_dst+=vtxs;}
   if(tex3_dst){CopyN(tex3_dst, vtx.tex3    (), vtxs); tex3_dst+=vtxs;}
   if(mtrx_dst){CopyN(mtrx_dst, vtx.matrix  (), vtxs); mtrx_dst+=vtxs;}
   if(blnd_dst){CopyN(blnd_dst, vtx.blend   (), vtxs); blnd_dst+=vtxs;}
   if(size_dst){CopyN(size_dst, vtx.size    (), vtxs); size_dst+=vtxs;}
   if(mtrl_dst){CopyN(mtrl_dst, vtx.material(), vtxs); mtrl_dst+=vtxs;}
   if( col_dst){CopyN( col_dst, vtx.color   (), vtxs);  col_dst+=vtxs;}
   if(flag_dst){CopyN(flag_dst, vtx.flag    (), vtxs); flag_dst+=vtxs;}

   // reposition original points
   Int *vtx_edg_num  =AllocZero<Int>(vtxs),
       *vtx_edg_num_b=AllocZero<Int>(vtxs), // border
       *vtx_fac_num  =AllocZero<Int>(vtxs);
   Vec *vtx_edg_avg  =AllocZero<Vec>(vtxs),
       *vtx_edg_avg_b=AllocZero<Vec>(vtxs), // border
       *vtx_fac_avg  =AllocZero<Vec>(vtxs),
       * tri_center=(tri_as_quad ? pos_src+ tri_vtx_offset : Alloc<Vec>(tris)), // if we're storing tris as quads then we store their center in the dest mesh, and we can access those centers from there, otherwise allocate an array
       *quad_center=               pos_src+quad_vtx_offset;

   // face points
   if(tri_as_quad)FREP(tris)
   {
      p=tri_src[i].c; p0=p[0]; p1=p[1]; p2=p[2];
      if( pos_dst)* pos_dst++=Avg ( pos_src[p0], pos_src[p1], pos_src[p2]);
      if( nrm_dst)* nrm_dst++=   !( nrm_src[p0]+ nrm_src[p1]+ nrm_src[p2]);
      if( tan_dst)* tan_dst++=   !( tan_src[p0]+ tan_src[p1]+ tan_src[p2]);
      if( bin_dst)* bin_dst++=   !( bin_src[p0]+ bin_src[p1]+ bin_src[p2]);
      if( hlp_dst)* hlp_dst++=Avg ( hlp_src[p0], hlp_src[p1], hlp_src[p2]);
      if(tex0_dst)*tex0_dst++=Avg (tex0_src[p0],tex0_src[p1],tex0_src[p2]);
      if(tex1_dst)*tex1_dst++=Avg (tex1_src[p0],tex1_src[p1],tex1_src[p2]);
      if(tex2_dst)*tex2_dst++=Avg (tex2_src[p0],tex2_src[p1],tex2_src[p2]);
      if(tex3_dst)*tex3_dst++=Avg (tex3_src[p0],tex3_src[p1],tex3_src[p2]);
      if(size_dst)*size_dst++=Avg (size_src[p0],size_src[p1],size_src[p2]);
      if(mtrl_dst)*mtrl_dst++=AvgI(mtrl_src[p0],mtrl_src[p1],mtrl_src[p2]);
      if( col_dst)* col_dst++=Avg ( col_src[p0], col_src[p1], col_src[p2]);
      if(flag_dst)*flag_dst++=    (flag_src[p0]|flag_src[p1]|flag_src[p2]);
      if(mtrx_dst && blnd_dst)
      {
         REP(3){Int point=p[i]; REP(4)skin.New().set(mtrx_src[point].c[i], blnd_src[point].c[i]);}
         SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();
      }else
      {
         if(mtrx_dst)*mtrx_dst++=mtrx_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
         if(blnd_dst)*blnd_dst++=blnd_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
      }
   }else FREP(tris)
   {
      p=tri_src[i].c; p0=p[0]; p1=p[1]; p2=p[2];
      tri_center[i]=Avg(pos_src[p0], pos_src[p1], pos_src[p2]);
   }

   FREP(quads)
   {
      p=qud_src[i].c; p0=p[0]; p1=p[1]; p2=p[2]; p3=p[3];
      if( pos_dst)* pos_dst++=Avg ( pos_src[p0], pos_src[p1], pos_src[p2], pos_src[p3]);
      if( nrm_dst)* nrm_dst++=   !( nrm_src[p0]+ nrm_src[p1]+ nrm_src[p2]+ nrm_src[p3]);
      if( tan_dst)* tan_dst++=   !( tan_src[p0]+ tan_src[p1]+ tan_src[p2]+ tan_src[p3]);
      if( bin_dst)* bin_dst++=   !( bin_src[p0]+ bin_src[p1]+ bin_src[p2]+ bin_src[p3]);
      if( hlp_dst)* hlp_dst++=Avg ( hlp_src[p0], hlp_src[p1], hlp_src[p2], hlp_src[p3]);
      if(tex0_dst)*tex0_dst++=Avg (tex0_src[p0],tex0_src[p1],tex0_src[p2],tex0_src[p3]);
      if(tex1_dst)*tex1_dst++=Avg (tex1_src[p0],tex1_src[p1],tex1_src[p2],tex1_src[p3]);
      if(tex2_dst)*tex2_dst++=Avg (tex2_src[p0],tex2_src[p1],tex2_src[p2],tex2_src[p3]);
      if(tex3_dst)*tex3_dst++=Avg (tex3_src[p0],tex3_src[p1],tex3_src[p2],tex3_src[p3]);
      if(size_dst)*size_dst++=Avg (size_src[p0],size_src[p1],size_src[p2],size_src[p3]);
      if(mtrl_dst)*mtrl_dst++=AvgI(mtrl_src[p0],mtrl_src[p1],mtrl_src[p2],mtrl_src[p3]);
      if( col_dst)* col_dst++=Avg ( col_src[p0], col_src[p1], col_src[p2], col_src[p3]);
      if(flag_dst)*flag_dst++=    (flag_src[p0]|flag_src[p1]|flag_src[p2]|flag_src[p3]);
      if(mtrx_dst && blnd_dst)
      {
         REP(4){Int point=p[i]; REP(4)skin.New().set(mtrx_src[point].c[i], blnd_src[point].c[i]);}
         SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();
      }else
      {
         if(mtrx_dst)*mtrx_dst++=mtrx_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
         if(blnd_dst)*blnd_dst++=blnd_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
      }
   }

   // edge points
   FREP(edges)
   {
      p=edg_src[i].c; p0=p[0]; p1=p[1];
      if( pos_dst) pos_dst[i]=Avg ( pos_src[p0], pos_src[p1]);
      if( nrm_dst) nrm_dst[i]=   !( nrm_src[p0]+ nrm_src[p1]);
      if( tan_dst) tan_dst[i]=   !( tan_src[p0]+ tan_src[p1]);
      if( bin_dst) bin_dst[i]=   !( bin_src[p0]+ bin_src[p1]);
      if( hlp_dst) hlp_dst[i]=Avg ( hlp_src[p0], hlp_src[p1]);
      if(tex0_dst)tex0_dst[i]=Avg (tex0_src[p0],tex0_src[p1]);
      if(tex1_dst)tex1_dst[i]=Avg (tex1_src[p0],tex1_src[p1]);
      if(tex2_dst)tex2_dst[i]=Avg (tex2_src[p0],tex2_src[p1]);
      if(tex3_dst)tex3_dst[i]=Avg (tex3_src[p0],tex3_src[p1]);
      if(size_dst)size_dst[i]=Avg (size_src[p0],size_src[p1]);
      if(mtrl_dst)mtrl_dst[i]=AvgI(mtrl_src[p0],mtrl_src[p1]);
      if( col_dst) col_dst[i]=Avg ( col_src[p0], col_src[p1]);
      if(flag_dst)flag_dst[i]=    (flag_src[p0]|flag_src[p1]);
      if(mtrx_dst && blnd_dst)
      {
         REP(2){Int point=p[i]; REP(4)skin.New().set(mtrx_src[point].c[i], blnd_src[point].c[i]);}
         SetSkin(skin, *mtrx_dst++, *blnd_dst++, null); skin.clear();
      }else
      {
         if(mtrx_dst)*mtrx_dst++=mtrx_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
         if(blnd_dst)*blnd_dst++=blnd_src[p0]; // just copy from first point, because we need correct data and blending with sum of 255
      }

      p=(dup_vtx ? dup_edge_adj[dup_edge_edg[i]] : edg_adj[i]).c; p0=p[0]; p1=p[1];

      // edge point average (must be based on edge mid-points, before adjusting it by face centers)
      if(dup_vtx){Bool &add=dup_edge_add[dup_edge_edg[i]]; if(add)goto skip_edge;else add=true;}
      {
         VecI2 f=edg_src[i]; f.remap(dup_vtx);
         Vec  &v=pos_dst[i];
         if(p0!=-1 && p1!=-1) // have both faces, compare to -1 and not >=0 because it can have SIGN_BIT
         {
            vtx_edg_avg[f.c[0]]+=v;
            vtx_edg_avg[f.c[1]]+=v;
            vtx_edg_num[f.c[0]]++;
            vtx_edg_num[f.c[1]]++;
         }else
         {
            vtx_edg_avg_b[f.c[0]]+=v;
            vtx_edg_avg_b[f.c[1]]+=v;
            vtx_edg_num_b[f.c[0]]++;
            vtx_edg_num_b[f.c[1]]++;
         }
      }
   skip_edge:;

      // adjust point by adj_facs center
      if(p0!=-1 && p1!=-1) // have both faces, compare to -1 and not >=0 because it can have SIGN_BIT
      {
         Vec fp=Avg((p0&SIGN_BIT) ? quad_center[p0^SIGN_BIT] : tri_center[p0],
                    (p1&SIGN_BIT) ? quad_center[p1^SIGN_BIT] : tri_center[p1]);
         pos_dst[i]=Avg(pos_dst[i], fp); // make edge point to be average of edge points and nearby face centers
      }
   }

   // reposition original points
   {
      // face point average
      FREP(tris)
      {
         VecI f=tri_src   [i]; f.remap(dup_vtx);
         Vec &v=tri_center[i];
         vtx_fac_avg[f.c[0]]+=v;
         vtx_fac_avg[f.c[1]]+=v;
         vtx_fac_avg[f.c[2]]+=v;
         vtx_fac_num[f.c[0]]++;
         vtx_fac_num[f.c[1]]++;
         vtx_fac_num[f.c[2]]++;
      }
      FREP(quads)
      {
         VecI4 f= qud_src   [i]; f.remap(dup_vtx);
         Vec  &v=quad_center[i];
         vtx_fac_avg[f.c[0]]+=v;
         vtx_fac_avg[f.c[1]]+=v;
         vtx_fac_avg[f.c[2]]+=v;
         vtx_fac_avg[f.c[3]]+=v;
         vtx_fac_num[f.c[0]]++;
         vtx_fac_num[f.c[1]]++;
         vtx_fac_num[f.c[2]]++;
         vtx_fac_num[f.c[3]]++;
      }

      FREP(vtxs)
      {
         Int   n=vtx_edg_num  [i];
         if(n && vtx_fac_num  [i]==n)pos_src[i]=((vtx_fac_avg[i] + 2*vtx_edg_avg[i])/n + (n-3)*pos_src[i])/n;else 
         if(   n=vtx_edg_num_b[i]   )pos_src[i]=Avg(vtx_edg_avg_b[i]/n, pos_src[i]);
      }
   }

   if(!tri_as_quad)Free(tri_center);
   Free(vtx_fac_avg  );
   Free(vtx_edg_avg_b);
   Free(vtx_edg_avg  );
   Free(vtx_fac_num  );
   Free(vtx_edg_num_b);
   Free(vtx_edg_num  );

   // create faces
   VecI *tri_adj_edg=tri.adjEdge(); FREP(tris)
   {
      p=tri_src[i].c;
      if(tri_as_quad)
      {
         qud_dst[0].set(p[0], edge_vtx_offset+tri_adj_edg[i].c[0],
                               tri_vtx_offset+i,
                              edge_vtx_offset+tri_adj_edg[i].c[2]);
         qud_dst[1].set(p[1], edge_vtx_offset+tri_adj_edg[i].c[1],
                               tri_vtx_offset+i,
                              edge_vtx_offset+tri_adj_edg[i].c[0]);
         qud_dst[2].set(p[2], edge_vtx_offset+tri_adj_edg[i].c[2],
                               tri_vtx_offset+i,
                              edge_vtx_offset+tri_adj_edg[i].c[1]);
         qud_dst+=3;
      }else
      {
         tri_dst[0].set(p[0], edge_vtx_offset+tri_adj_edg[i].c[0],
                              edge_vtx_offset+tri_adj_edg[i].c[2]);
         tri_dst[1].set(p[1], edge_vtx_offset+tri_adj_edg[i].c[1],
                              edge_vtx_offset+tri_adj_edg[i].c[0]);
         tri_dst[2].set(p[2], edge_vtx_offset+tri_adj_edg[i].c[2],
                              edge_vtx_offset+tri_adj_edg[i].c[1]);
         tri_dst[3].set(      edge_vtx_offset+tri_adj_edg[i].c[0],
                              edge_vtx_offset+tri_adj_edg[i].c[1],
                              edge_vtx_offset+tri_adj_edg[i].c[2]);
         tri_dst+=4;
      }
   }
   VecI4 *qud_adj_edg=quad.adjEdge(); FREP(quads)
   {
      p=qud_src[i].c;
      qud_dst[0].set(p[0], edge_vtx_offset+qud_adj_edg[i].c[0],
                           quad_vtx_offset+i,
                           edge_vtx_offset+qud_adj_edg[i].c[3]);
      qud_dst[1].set(p[1], edge_vtx_offset+qud_adj_edg[i].c[1],
                           quad_vtx_offset+i,
                           edge_vtx_offset+qud_adj_edg[i].c[0]);
      qud_dst[2].set(p[2], edge_vtx_offset+qud_adj_edg[i].c[2],
                           quad_vtx_offset+i,
                           edge_vtx_offset+qud_adj_edg[i].c[1]);
      qud_dst[3].set(p[3], edge_vtx_offset+qud_adj_edg[i].c[3],
                           quad_vtx_offset+i,
                           edge_vtx_offset+qud_adj_edg[i].c[2]);
      qud_dst+=4;
   }

   if(dup_vtx)
   {
      REP(vtxs)pos_src[i]=pos_src[dup_vtx[i]];
      Free(dup_vtx     );
      Free(dup_edge_edg);
      Free(dup_edge_add);
      Free(dup_edge_adj);
      temp.setVtxDup();
   }

   Swap(T, temp); return T;
}
/******************************************************************************/
MeshBase& MeshBase::subdivideEdge(Bool freeze_z, C CMemPtr<Bool> &is)
{
   Int     *p, p0, p1;
   Int      vtxs =T.vtxs (),
            edges=T.edges(),
            num  =(is ? CountIs(is) : edges);
   MeshBase temp(vtxs+num, edges+num, 0, 0, (VTX_NRM_TAN_BIN|VTX_HLP|VTX_TEX0|VTX_TEX1|VTX_TEX2|VTX_TEX3|VTX_MATRIX|VTX_BLEND|VTX_SIZE|VTX_MATERIAL|VTX_COLOR|VTX_FLAG|EDGE_FLAG|EDGE_ID)&flag());
   MemtN<IndexWeight, 256> skin;

   // original points
   CopyN(temp.vtx.nrm     (), vtx.nrm     (), vtxs);
   CopyN(temp.vtx.tan     (), vtx.tan     (), vtxs);
   CopyN(temp.vtx.bin     (), vtx.bin     (), vtxs);
   CopyN(temp.vtx.hlp     (), vtx.hlp     (), vtxs);
   CopyN(temp.vtx.tex0    (), vtx.tex0    (), vtxs);
   CopyN(temp.vtx.tex1    (), vtx.tex1    (), vtxs);
   CopyN(temp.vtx.tex2    (), vtx.tex2    (), vtxs);
   CopyN(temp.vtx.tex3    (), vtx.tex3    (), vtxs);
   CopyN(temp.vtx.matrix  (), vtx.matrix  (), vtxs);
   CopyN(temp.vtx.blend   (), vtx.blend   (), vtxs);
   CopyN(temp.vtx.size    (), vtx.size    (), vtxs);
   CopyN(temp.vtx.material(), vtx.material(), vtxs);
   CopyN(temp.vtx.color   (), vtx.color   (), vtxs);
   CopyN(temp.vtx.flag    (), vtx.flag    (), vtxs);

   CopyN(temp.edge.ind (), edge.ind (), edges);
   CopyN(temp.edge.flag(), edge.flag(), edges);
   CopyN(temp.edge.id  (), edge.id  (), edges);

   Int j=0; FREP(edges)if(!is || ElmIs(is, i))
   {
      p=edge.ind(i).c; p0=p[0]; p1=p[1];

      // edge points
                             temp.vtx.pos     (vtxs+j)=Avg (vtx.pos     (p0),vtx.pos     (p1));
      if(temp.vtx.nrm     ())temp.vtx.nrm     (vtxs+j)=   !(vtx.nrm     (p0)+vtx.nrm     (p1));
      if(temp.vtx.tan     ())temp.vtx.tan     (vtxs+j)=   !(vtx.tan     (p0)+vtx.tan     (p1));
      if(temp.vtx.bin     ())temp.vtx.bin     (vtxs+j)=   !(vtx.bin     (p0)+vtx.bin     (p1));
      if(temp.vtx.hlp     ())temp.vtx.hlp     (vtxs+j)=Avg (vtx.hlp     (p0),vtx.hlp     (p1));
      if(temp.vtx.tex0    ())temp.vtx.tex0    (vtxs+j)=Avg (vtx.tex0    (p0),vtx.tex0    (p1));
      if(temp.vtx.tex1    ())temp.vtx.tex1    (vtxs+j)=Avg (vtx.tex1    (p0),vtx.tex1    (p1));
      if(temp.vtx.tex2    ())temp.vtx.tex2    (vtxs+j)=Avg (vtx.tex2    (p0),vtx.tex2    (p1));
      if(temp.vtx.tex3    ())temp.vtx.tex3    (vtxs+j)=Avg (vtx.tex3    (p0),vtx.tex3    (p1));
      if(temp.vtx.size    ())temp.vtx.size    (vtxs+j)=Avg (vtx.size    (p0),vtx.size    (p1));
      if(temp.vtx.material())temp.vtx.material(vtxs+j)=AvgI(vtx.material(p0),vtx.material(p1));
      if(temp.vtx.color   ())temp.vtx.color   (vtxs+j)=Avg (vtx.color   (p0),vtx.color   (p1));
      if(temp.vtx.flag    ())temp.vtx.flag    (vtxs+j)=    (vtx.flag    (p0)|vtx.flag    (p1));
      if(temp.vtx.matrix  () && temp.vtx.blend())
      {
         REP(2){Int point=p[i]; REP(4)skin.New().set(vtx.matrix(point).c[i], vtx.blend(point).c[i]);}
         SetSkin(skin, temp.vtx.matrix(vtxs+j), temp.vtx.blend(vtxs+j), null); skin.clear();
      }else
      {
         if(temp.vtx.matrix())temp.vtx.matrix(vtxs+j)=vtx.matrix(p0); // just copy from first point, because we need correct data and blending with sum of 255
         if(temp.vtx.blend ())temp.vtx.blend (vtxs+j)=vtx.blend (p0); // just copy from first point, because we need correct data and blending with sum of 255
      }

      // edges
                          temp.edge.ind (i      ).set(p0, vtxs+j    );
                          temp.edge.ind (edges+j).set(    vtxs+j, p1);
      if(temp.edge.flag())temp.edge.flag(edges+j)=edge.flag(i);
      if(temp.edge.id  ())temp.edge.id  (edges+j)=edge.id  (i);
      j++;
   }

   // move original points
   Memt<Int> vtx_num_mem; Int *vtx_num=vtx_num_mem.setNumZero(vtxs).data();
   Bool     *vtx_freeze=(freeze_z ? AllocZero<Bool>(vtxs) : null);
   ZeroN(temp.vtx.pos(), vtxs);
   j=0; FREP(edges)if(!is || ElmIs(is, i))
   {
      p=edge.ind(i).c; p0=p[0]; p1=p[1];
      if(edge.flag() && (edge.flag(i)&ETQ_LR)==ETQ_LR)
      {
         if(freeze_z)
         {
            vtx_freeze[p0]=true;
            vtx_freeze[p1]=true;
         }
      }else
      {
         vtx_num[p0]++; temp.vtx.pos(p0)+=temp.vtx.pos(vtxs+j);
         vtx_num[p1]++; temp.vtx.pos(p1)+=temp.vtx.pos(vtxs+j);
      }
      j++;
   }
   FREP(vtxs)
   {
      if(vtx_num[i]>=2)
      {
                                      temp.vtx.pos(i)  =Avg(vtx.pos(i), temp.vtx.pos(i)/vtx_num[i]);
         if(freeze_z && vtx_freeze[i])temp.vtx.pos(i).z=    vtx.pos(i).z;
      }else
      {
         temp.vtx.pos(i)=vtx.pos(i);
      }
   }

   // free
   Free(vtx_freeze);
   Swap(T, temp); return T;
}
/******************************************************************************/
//
/******************************************************************************/
struct SplitEdge
{
   VecI2 ind;
   Int   id;
   UInt  flag;
};
MeshBase& MeshBase::splitEdges(Flt length, Bool *is)
{
   Memb<VtxFull  > mvtx (128);
   Memb<SplitEdge> medge(128);
   {
      // copy original vtx
      FREPA(vtx)mvtx.New().from(T, i);

      // create edges
      FREPA(edge)
      {
         Int     *p  =edge.ind(i).c, p0=p[0], p1=p[1];
         VtxFull &v0 =mvtx[p0],
                 &v1 =mvtx[p1];
         Int     next=p1, prev;
         if(!is || is[i])
         {
            Flt l=Dist(v0.pos.xy, v1.pos.xy);
            Int c=Round(l/length);
            REPD(j, c-1)
            {
               prev=mvtx.elms(); mvtx.New().lerp(v0, v1, Flt(j+1)/c);
               SplitEdge &e=medge.New();
                              e.ind.set(prev, next);
               if(edge.id  ())e.id  =edge.id  (i);
               if(edge.flag())e.flag=edge.flag(i);
               next=prev;
            }
         }
         prev=p0;
         SplitEdge &e=medge.New();
                        e.ind.set(prev, next);
         if(edge.id  ())e.id  =edge.id  (i);
         if(edge.flag())e.flag=edge.flag(i);
      }
   }

   // create mshb
   {
      MESH_FLAG f=((VTX_ALL|EDGE_ID|EDGE_FLAG)&flag());
      create(mvtx.elms(), medge.elms(), 0, 0, f);
      REPA(vtx )mvtx[i].to(T, i);
      REPA(edge)
      {
         SplitEdge &e=medge[i];
                        edge.ind (i)=e.ind ;
         if(edge.id  ())edge.id  (i)=e.id  ;
         if(edge.flag())edge.flag(i)=e.flag;
      }
   }

   return T;
}
MeshBase& MeshBase::splitEdges(Flt length, Flt d, Bool *is)
{
   Memb<VtxFull  > mvtx (128);
   Memb<SplitEdge> medge(128);
   {
      // copy original vtx
      FREPA(vtx)mvtx.New().from(T, i);

      // create edges
      d=Min(d, length/3);
      FREPA(edge)
      {
         Int     *p   =edge.ind(i).c, p0=p[0], p1=p[1];
         VtxFull &v0  =mvtx[p0],
                 &v1  =mvtx[p1];
         Int      next=p1, prev;
         if(!is || is[i])
         {
            Flt l=Dist(v0.pos.xy, v1.pos.xy);
            if(l>=length)
            {
               // first edge
               {
                  prev=mvtx.elms(); mvtx.New().lerp(v0, v1, 1-d/l);
                  SplitEdge &e=medge.New();
                                 e.ind.set(prev, next);
                  if(edge.id  ())e.id  =edge.id  (i);
                  if(edge.flag())e.flag=edge.flag(i);
                  next=prev;
               }

               // second edge
               {
                  prev=mvtx.elms(); mvtx.New().lerp(v0, v1, d/l);
                  SplitEdge &e=medge.New();
                                 e.ind.set(prev, next);
                  if(edge.id  ())e.id  =edge.id  (i);
                  if(edge.flag())e.flag=edge.flag(i);
                  next=prev;
               }
            }
         }
         prev=p0;
         SplitEdge &e=medge.New();
                        e.ind.set(prev, next);
         if(edge.id  ())e.id  =edge.id  (i);
         if(edge.flag())e.flag=edge.flag(i);
      }
   }

   // create mshb
   {
      MESH_FLAG f=((VTX_ALL|EDGE_ID|EDGE_FLAG)&flag());
      create(mvtx.elms(), medge.elms(), 0, 0, f);
      REPA(vtx )mvtx[i].to(T, i);
      REPA(edge)
      {
         SplitEdge &e=medge[i];
                        edge.ind (i)=e.ind ;
         if(edge.id  ())edge.id  (i)=e.id  ;
         if(edge.flag())edge.flag(i)=e.flag;
      }
   }

   return T;
}
/******************************************************************************/
// EXTRUDE
/******************************************************************************
   MeshBase& extrEdge  (Flt  length, Bool keep_src=false  ); // extrude edges !! beta !!
   MeshBase& extr      (Flt  length                       ); // extrude
/******************************************************************************
static void MSHBCreate(MeshBase &dest, MeshBase &src, Poly &poly)
{
   dest.create(poly.vtx.elms(), poly.vtx.elms(), 0, 0, (src.flag()&VTX_ALL)|EDGE_FLAG);
   Int j=0;
   MFREP(poly.vtx)
   {
      PVTX &vtx=poly.vtx[i];
      VtxSet(dest, src, j, vtx.ind);
      dest.edge.ind [j].set(j, (j+1)%dest.vtx.num);
      dest.edge.flag[j]=ETQ_R;
      j++;
   }
   dest.setNormal2();
}
/******************************************************************************
static void MSHBCreate(MeshBase &dest, MeshBase &src, Poly &poly, Flt length, Bool inf)
{
   MeshBase temp;
   // create extruded temp
   {
      Memb<VtxFull> vtxs(poly.vtx.elms());
      Vec *nrm=src.vtx.nrm;
      poly.setAngle();
      MFREP(poly.vtx)
      {
         PVTX &pvtx =poly.vtx[i];
         Int   ind  =pvtx.ind;
         Flt   angle=pvtx.angle;
         //if(angle>=0 && angle<=PI)
         {
            VtxFull &v=Zero(vtxs.New());
            VtxSet(v, src, ind);
          //v.pos+=nrm[ind]*length/Sin(angle/2);
            v.pos+=nrm[ind]*length/Max(0.25f, Sin(angle/2));
         }/*else
         {
            VtxFull &v=Zero(vtxs.New());
            VtxSet(v, src, ind);
            v.pos+=nrm[ind]*length;
         }*
      }
      temp.create(vtxs.elms(), vtxs.elms(), 0, 0, (src.flag()&VTX_ALL)|EDGE_FLAG);
      FREPA(vtxs)
      {
         VtxSet(temp, vtxs[i], i);
         temp.edge.ind [i].set(i, (i+1)%temp.vtx.num);
         temp.edge.flag[i]=ETQ_R;
      }
   }
   // get non-infinite parts
   {
      temp.cut2D(); temp.weldVtx(0, 0.001f);
      Meml<Poly> polys; temp.edgToPoly(polys);
      MeshBase      *mshb=AllocN(MeshBase, polys.elms()); Int mshbs=0;
      MFREP(polys)
      {
         Poly &poly=polys[i];
         if(poly.inf()==inf)MSHBCreate(mshb[mshbs++], temp, poly);
      }
      dest.create(mshb, mshbs); REP(mshbs)mshb[i].del(); Free(mshb);
   }
}
/******************************************************************************
MeshBase& MeshBase::extrEdge(Flt length, Bool keep_src)
{
   MeshBase *mshb;
   Int   mshbs;
   // extrude
   {
      setNormal2(true);
      Meml<Poly> polys; edgToPoly(polys);
      AllocZero(mshb, polys.elms()+1); mshbs=0;
      MFREP(polys)
      {
         Poly &poly=polys[i];
         MSHBCreate(mshb[mshbs], T, poly, length, poly.inf());
         if(mshb[mshbs].is2())mshbs++;
      }
   }
   // solid up
   /*{
      REP(mshbs)
      {
         MeshBase &m=mshb[i];
         Byte *flag=m.edge.flag; REP(m.edge.num)(*flag++)|=ETQ_LR;
      }
   }*
   // create
   if(mshbs)
   {
      if(keep_src)copy(mshb[mshbs++]); del();
      create(mshb, mshbs); REP(mshbs)mshb[i].del();
   }
   Free(mshb);
   return T;
}
/******************************************************************************
MeshBase& MeshBase::extr(Flt length)
{
   if(!vtx .nrm     )setNormals    ();
   if(!edge.adj_face)setAdjacencies();

   Int edges=0; REPA(edge)if(edge.adj_face[i].x==-1 || edge.adj_face[i].y==-1)edges++; // compare to -1 and not <0 because it can have SIGN_BIT

   MeshBase temp(vtx.num*2, 0, tri.num*2, quad.num*2+edges, flag()&(VTX_NRM|VTX_TEX0));
   if(temp.vtx.nrm)
   {
      CopyN(temp.vtx.nrm        , vtx.nrm, vtx.num);
      CopyN(temp.vtx.nrm+vtx.num, vtx.nrm, vtx.num);
      Chs  (temp.vtx.nrm+vtx.num,          vtx.num);
   }
   CopyN(temp.vtx .pos         , vtx .pos , vtx .num);
   CopyN(temp.vtx .tex0        , vtx .tex0, vtx .num);
   CopyN(temp.vtx .tex0+vtx.num, vtx .tex0, vtx .num);
   CopyN(temp.tri .ind         , tri .ind , tri .num);
   CopyN(temp.quad.ind         , quad.ind , quad.num);
   {
      Vec *dest=temp.vtx.pos+vtx.num;
      REPA(vtx)
      {
         dest[i]=vtx.pos[i]-vtx.nrm[i]*length;
      }
   }
   {
      VecI *dest=temp.tri.ind+tri.num;
      REPA(tri)
      {
         dest[i]=tri.ind[i]+vtx.num;
         dest[i].reverse();
      }
   }
   {
      VecI4 *dest=temp.quad.ind+quad.num;
      REPA(quad)
      {
         dest[i]=quad.ind[i]+vtx.num;
         dest[i].reverse();
      }
      dest+=quad.num;
      REPA(edge)
      if(edge.adj_face[i].x==-1) // compare to -1 and not <0 because it can have SIGN_BIT
      {
         (dest++)->set(edge.ind[i].x, edge.ind[i].y, edge.ind[i].y+vtx.num, edge.ind[i].x+vtx.num);
      }else
      if(edge.adj_face[i].y==-1) // compare to -1 and not <0 because it can have SIGN_BIT
      {
         (dest++)->set(edge.ind[i].y, edge.ind[i].x, edge.ind[i].x+vtx.num, edge.ind[i].y+vtx.num);
      }
   }
   Swap(T, temp); return T;
}
/******************************************************************************/
static void BoneRemap(VecB4 &matrix, C CMemPtr<Byte, 256> &old_to_new, VecB4 *blend=null)
{
   REPA(matrix.c)
   {
      Byte &m=matrix.c[i];
   #if VIRTUAL_ROOT_BONE
      if(m)
      {
         if(InRange(m-1, old_to_new))
         {
            m=old_to_new[m-1];
            if(m==0xFF)m=0;else m++;
         }else
         {
            m=0;
         }
      }
   #else
      if(InRange(m, old_to_new))
      {
         m=old_to_new[m];
         if(m==0xFF)m=0;
      }else
      {
         m=0;
      }
   #endif
   }
   if(blend)FixMatrixWeight(matrix, *blend);
}
MeshBase& MeshBase::boneRemap(C CMemPtr<Byte, 256> &old_to_new)
{
   if(VecB4 *matrix=vtx.matrix()){VecB4 *blend=vtx.blend(); REP(vtxs())BoneRemap(matrix[i], old_to_new, blend ? &blend[i] : null);}
   return T;
}
void MeshRender::boneRemap(C CMemPtr<Byte, 256> &old_to_new)
{
   Int matrix_ofs =vtxOfs(VTX_MATRIX);
   if( matrix_ofs>=0)if(Byte *vtx=vtxLock())
   {
      Int    blend_ofs=vtxOfs(VTX_BLEND);
      Byte *vtx_matrix=vtx+matrix_ofs, *vtx_blend=((blend_ofs>=0) ? vtx+blend_ofs : null);
      REP(vtxs())
      {
         BoneRemap(*(VecB4*)vtx_matrix, old_to_new, (VecB4*)vtx_blend);
                      vtx_matrix+=vtxSize();
         if(vtx_blend)vtx_blend +=vtxSize();
      }
      vtxUnlock();
   }
}
void MeshBase  ::setUsedBones(Bool (&bones)[256])C {Zero(bones); includeUsedBones(bones);}
void MeshRender::setUsedBones(Bool (&bones)[256])C {Zero(bones); includeUsedBones(bones);}

void MeshBase::includeUsedBones(Bool (&bones)[256])C
{
   if(C VecB4 *matrix=vtx.matrix())
   {
    C VecB4 *blend=vtx.blend(); // this is optional
      REP(vtxs())
      {
         VecB4 m=matrix[i], b; if(blend)b=blend[i];
         REPA(m.c)
         {
            Byte bone=m.c[i];
         #if VIRTUAL_ROOT_BONE
            if(bone)
            {
               bone--;
               if(blend ? b.c[i] : true)bones[bone]=true;
            }
         #else
            if(blend ? b.c[i] : true)bones[bone]=true;
         #endif
         }
      }
   }
}
void MeshRender::includeUsedBones(Bool (&bones)[256])C
{
   Int matrix_ofs =vtxOfs(VTX_MATRIX);
   if( matrix_ofs>=0)if(C Byte *vtx=vtxLockRead())
   {
      Int    blend_ofs=vtxOfs(VTX_BLEND);
    C Byte *vtx_matrix=vtx+matrix_ofs, *vtx_blend=((blend_ofs>=0) ? vtx+blend_ofs : null);
    /*if(_bone_split && storageBoneSplit())FREP(_bone_splits)
      {
       C MeshRender::BoneSplit &split=_bone_split[i];
         FREP(split.vtxs)
         {
            REP(4) // 4 bytes in VecB4
            {
               Byte bone=split.split_to_real[vtx_matrix[i]];
               if(  bone)
               {
                  bone--;
                  if(vtx_blend ? vtx_blend[i] : true)bones[bone]=true;
               }
            }
                         vtx_matrix+=vtxSize();
            if(vtx_blend)vtx_blend +=vtxSize();
         }
      }else*/
      REP(vtxs())
      {
         REP(4) // 4 bytes in VecB4
         {
            Byte bone=vtx_matrix[i];
         #if VIRTUAL_ROOT_BONE
            if(bone)
            {
               bone--;
               if(vtx_blend ? vtx_blend[i] : true)bones[bone]=true;
            }
         #else
            if(vtx_blend ? vtx_blend[i] : true)bones[bone]=true;
         #endif
         }
                      vtx_matrix+=vtxSize();
         if(vtx_blend)vtx_blend +=vtxSize();
      }
      vtxUnlock();
   }
}
/******************************************************************************/
}
/******************************************************************************/
