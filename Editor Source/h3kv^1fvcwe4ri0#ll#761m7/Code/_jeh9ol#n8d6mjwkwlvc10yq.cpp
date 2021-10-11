/******************************************************************************

   Lake and River can be saved on MainThread and loaded on BuilderThread.
      Use 'WorldAreaSync' for that.

/******************************************************************************/
class LakeBase
{
   bool            removed=false;
   flt             depth=2, uv_scale=1;
   UID             material=UIDZero;
   Memc<Memc<Vec>> polys;
   TimeStamp       removed_time, depth_time, uv_scale_time, material_time, polys_time;

   // get
   bool equal(C LakeBase &src)C {return removed_time==src.removed_time && depth_time==src.depth_time && uv_scale_time==src.uv_scale_time && material_time==src.material_time && polys_time==src.polys_time;}
   bool newer(C LakeBase &src)C {return removed_time> src.removed_time || depth_time> src.depth_time || uv_scale_time> src.uv_scale_time || material_time> src.material_time || polys_time> src.polys_time;}

   bool getRect(Rect &rect)C
   {
      bool   found=false; REPA(polys){C Memc<Vec> &poly=polys[i]; REPA(poly)Include(rect, found, poly[i].xz());}
      return found;
   }

   uint memUsage()C
   {
      uint   size=0; REPA(polys)size+=polys[i].memUsage();
      return size;
   }

   // operations
   void setDepth(flt depth) {T.depth=depth; T.depth_time.getUTC();}
   bool sync(C LakeBase &src)
   {
      bool changed=false;
      changed|=Sync( removed_time, src. removed_time, removed , src.removed );
      changed|=Sync(   depth_time, src.   depth_time, depth   , src.depth   );
      changed|=Sync(uv_scale_time, src.uv_scale_time, uv_scale, src.uv_scale);
      changed|=Sync(material_time, src.material_time, material, src.material);
      changed|=Sync(   polys_time, src.   polys_time, polys   , src.polys   );
      return changed;
   }
   bool undo(C LakeBase &src)
   {
      bool changed=false;
      changed|=Undo      ( removed_time, src. removed_time, removed , src.removed );
      changed|=Undo      (   depth_time, src.   depth_time, depth   , src.depth   );
      changed|=Undo      (uv_scale_time, src.uv_scale_time, uv_scale, src.uv_scale);
      changed|=Undo      (material_time, src.material_time, material, src.material);
      changed|=UndoByTime(   polys_time, src.   polys_time, polys   , src.polys   );
      return changed;
   }

   // draw
   void draw(C Color &color, C Color &depth_color=TRANSPARENT)C
   {
      if(depth_color.a)
      {
         VI.color(depth_color);
         VI.cull (true);
         Vec d(0, -depth, 0);
         REPA(polys)
         {
          C Memc<Vec> &poly=polys[i];
            Vec        prev=poly.first();
            REPA(poly)
            {
             C Vec &next=poly[i];
                    VI.quad(prev, prev+d, next+d, next);
                    prev=next;
            }
         }
         VI.end();
      }
    /*if(side_width)
      {
         Color c0=ColorAlpha(color, 0.5),
               c1=ColorAlpha(color, 0.0);
         REPA(polys)
         {
            Memc<Vec> &poly=polys[i];
            Vec        prev=poly.first();
            REPA(poly)
            {
               Vec &next=poly[i],
                    perp=PerpN(prev.xz()-next.xz()).x0y()*side_width; VI.quad(c0, c1, prev, prev+perp, next+perp, next);
                    prev=next;
            }
         }
         VI.end();
      }*/
      REPA(polys)DrawPoly(ConstCast(polys[i]), color, color);
   }

   // io
   bool save(File &f)C
   {
      f.cmpUIntV(0);
      f<<removed<<depth<<uv_scale<<material;
      f.cmpUIntV(polys.elms()); FREPA(polys){C Memc<Vec> &poly=polys[i]; poly.saveRaw(f);}
      f<<removed_time<<depth_time<<uv_scale_time<<material_time<<polys_time;
      return f.ok();
   }
   bool load(File &f)
   {
      switch(f.decUIntV())
      {
         case 0:
         {
            f>>removed>>depth>>uv_scale>>material;
            polys.clear().setNum(f.decUIntV()); FREPA(polys){Memc<Vec> &poly=polys[i]; poly.loadRaw(f);}
            f>>removed_time>>depth_time>>uv_scale_time>>material_time>>polys_time;
            if(f.ok())return true;
         }break;
      }
      return false;
   }
   bool load(C Str &name)
   {
      ReadLock rl(WorldAreaSync);
      File f; if(f.readTry(name))return load(f); return false;
   }
}
/******************************************************************************/
class RiverBase
{
   class Vtx
   {
      flt radius=2;
      Vec pos=0;

      void set(flt radius, C Vec &pos) {T.radius=radius; T.pos=pos;}
   }

   bool        removed=false;
   byte        smooth=0;
   flt         depth=3;
   Vec2        uv_scale=1;
   UID         material=UIDZero;
   Memc<Vtx  > vtxs;
   Memc<VecI2> edges;
   TimeStamp   removed_time, smooth_time, depth_time, uv_scale_time, material_time, vtx_edge_time;

   // get
   bool equal(C RiverBase &src)C {return removed_time==src.removed_time && smooth_time==src.smooth_time && depth_time==src.depth_time && uv_scale_time==src.uv_scale_time && material_time==src.material_time && vtx_edge_time==src.vtx_edge_time;}
   bool newer(C RiverBase &src)C {return removed_time> src.removed_time || smooth_time> src.smooth_time || depth_time> src.depth_time || uv_scale_time> src.uv_scale_time || material_time> src.material_time || vtx_edge_time> src.vtx_edge_time;}

   bool getRect(Rect &rect)C
   {
      bool   found=false; REPA(vtxs){C Vtx &v=vtxs[i]; Include(rect, found, Rect_C(v.pos.xz(), v.radius));}
      return found;
   }

   uint memUsage()C
   {
      uint   size=vtxs.memUsage()+edges.memUsage();
      return size;
   }

   // operations
   void setSmooth (byte    smooth  ) {T.smooth  =smooth  ; T.  smooth_time.getUTC();}
   void setDepth  (flt     depth   ) {T.depth   =depth   ; T.   depth_time.getUTC();}
   void setUVScale(C Vec2 &uv_scale) {T.uv_scale=uv_scale; T.uv_scale_time.getUTC();}

   bool sync(C RiverBase &src)
   {
      bool changed=false;
      changed|=Sync( removed_time, src. removed_time, removed , src.removed );
      changed|=Sync(  smooth_time, src.  smooth_time, smooth  , src.smooth  );
      changed|=Sync(   depth_time, src.   depth_time, depth   , src.depth   );
      changed|=Sync(uv_scale_time, src.uv_scale_time, uv_scale, src.uv_scale);
      changed|=Sync(material_time, src.material_time, material, src.material);
      if(Sync(vtx_edge_time, src.vtx_edge_time))
      {
         changed=true;
         vtxs =src.vtxs;
         edges=src.edges;
      }
      return changed;
   }
   bool undo(C RiverBase &src)
   {
      bool changed=false;
      changed|=Undo( removed_time, src. removed_time, removed , src.removed );
      changed|=Undo(  smooth_time, src.  smooth_time, smooth  , src.smooth  );
      changed|=Undo(   depth_time, src.   depth_time, depth   , src.depth   );
      changed|=Undo(uv_scale_time, src.uv_scale_time, uv_scale, src.uv_scale);
      changed|=Undo(material_time, src.material_time, material, src.material);
      if(Undo(vtx_edge_time, src.vtx_edge_time))
      {
         changed=true;
         vtxs =src.vtxs;
         edges=src.edges;
      }
      return changed;
   }

   // draw
   void draw(C Color &color, C Color &depth_color=TRANSPARENT)C
   {
      if(depth_color.a)
      {
         Vec d(0, -depth, 0);
                    VI.color(depth_color);
       //REPA(vtxs )VI.line (vtxs[i].pos, vtxs[i].pos+d);
         REPA(edges)VI.quad (vtxs[edges[i].x].pos, vtxs[edges[i].x].pos+d, vtxs[edges[i].y].pos+d, vtxs[edges[i].y].pos);
                    VI.end  ();
      }
      if(color.a)
      {
                    VI.color(color);
         REPA(edges)VI.line (vtxs[edges[i].x].pos, vtxs[edges[i].y].pos);
                    VI.end  ();

                    VI.color(color);
         REPA(vtxs )VI.dot  (vtxs[i].pos);
                    VI.end  ();
      }
   }

   // io
   bool save(File &f)C
   {
      f.cmpUIntV(0);
      f<<smooth<<depth<<uv_scale<<material;
      vtxs .saveRaw(f);
      edges.saveRaw(f);
      f<<removed_time<<depth_time<<uv_scale_time<<material_time<<vtx_edge_time;
      return f.ok();
   }
   bool load(File &f)
   {
      switch(f.decUIntV())
      {
         case 0:
         {
            f>>smooth>>depth>>uv_scale>>material;
            vtxs .loadRaw(f);
            edges.loadRaw(f);
            f>>removed_time>>depth_time>>uv_scale_time>>material_time>>vtx_edge_time;
            if(f.ok())return true;
         }break;
      }
      return false;
   }
   bool load(C Str &name)
   {
      ReadLock rl(WorldAreaSync);
      File f; if(f.readTry(name))return load(f); return false;
   }
}
/******************************************************************************/
class Lake : LakeBase
{
   WaterMesh water_mesh;

   void setMesh(C Project &proj)
   {
      // set mesh 2D
      int vtxs=0; FREPA(polys)if(polys[i].elms()>=3)vtxs+=polys[i].elms();
      MeshBase mesh; mesh.create(vtxs, vtxs, 0, 0, EDGE_FLAG);
      vtxs=0;
      FREPA(polys)
      {
         Memc<Vec> &poly=polys[i];
         if(poly.elms()>=3)
         {
            FREPA(poly)
            {
               mesh.vtx .pos (vtxs+i)=poly[i].xzy();
               mesh.edge.ind (vtxs+i).set(vtxs+i, vtxs+(i+1)%poly.elms());
               mesh.edge.flag(vtxs+i)=ETQ_R;
            }
            vtxs+=poly.elms();
         }
      }

      // convert mesh 2D to 3D
      mesh.edgeToTri(false);
      REPA(mesh.vtx)Swap(mesh.vtx.pos(i).y, mesh.vtx.pos(i).z);

      // create water
      water_mesh.create(mesh, true, depth, proj.gamePath(material));
   }
   Lake& checkMesh(C Project &proj) // set mesh if not set yet
   {
      if(!water_mesh.is())setMesh(proj);
      return T;
   }
   void render()
   {
      water_mesh.draw();
   }
}
/******************************************************************************/
class River : RiverBase
{
   WaterMesh water_mesh;

   void setMesh(C Project &proj)
   {
      // set mesh 2D
      MeshBase mesh; mesh.create(vtxs.elms(), edges.elms(), 0, 0, VTX_SIZE);
      REPA(vtxs)
      {
         mesh.vtx.pos (i)=vtxs[i].pos.xzy();
         mesh.vtx.size(i)=vtxs[i].radius;
      }
      REPA(edges)mesh.edge.ind(i)=edges[i];

      // convert mesh 2D to 3D
      REP(Min(8, smooth))mesh.subdivideEdge();
      mesh.inflateEdges();
      mesh.quadToTri   (0.9993  ); // convert quads to tris because not flat rivers can have quads built of 2 non-coplanar tris
      mesh.texScale    (uv_scale);
      REPA(mesh.vtx)Swap(mesh.vtx.pos(i).y, mesh.vtx.pos(i).z);

      // create water
      water_mesh.create(mesh, false, depth, proj.gamePath(material));
   }
   River& checkMesh(C Project &proj) // set mesh if not set yet
   {
      if(!water_mesh.is())setMesh(proj);
      return T;
   }
   void render()
   {
      water_mesh.draw();
   }
}
/******************************************************************************/
