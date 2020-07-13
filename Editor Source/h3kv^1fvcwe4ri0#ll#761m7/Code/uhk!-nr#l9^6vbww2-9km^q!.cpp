/******************************************************************************/
class LeafRegion : Region
{
   class Texture : ImageSkin
   {
      virtual void update(C GuiPC &gpc)override
      {
         super.update(gpc);
         if(gpc.visible)
         {
            image=(MtrlEdit.game ? MtrlEdit.game->base_0 : ImagePtr());
            if(Gui.ms()==this)
            {
               bool set=false;
               Memc<LeafAttachment> &attachments=ObjEdit.leaf.attachments;
               if(Ms.bp(1) && attachments.elms()){attachments.removeLast(); set=true;}
               if(Ms.bp(0) || Ms.b(0))
               {
                  Vec2 screen_pos=pos()+gpc.offset;
                  Vec2 tex=(Ms.pos()-screen_pos)/size(); CHS(tex.y); tex.sat();
                  if(Ms.bp(0))
                  {
                     if(!Kb.ctrlCmd())attachments.clear();
                     LeafAttachment &at=attachments.New();
                     at.center=at.attachment=tex;
                     set=true;
                  }
                  if(attachments.elms())
                  {
                     LeafAttachment &at=attachments.last();
                     if(at.attachment!=tex)
                     {
                        at.attachment=tex;
                        set=true;
                     }
                  }
               }
               if(set)
               {
                  bool changed=false;
                  REPD(i, ObjEdit.mesh.lods())if(ObjEdit.selLod()==i || !ObjEdit.visibleLodSelection())
                  {
                     MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(ObjEdit.partOp(i) || !ObjEdit.mesh_parts.visibleOnActiveDesktop())
                     {
                        MeshPart &part=lod.parts[i]; if(HasMaterial(part, MtrlEdit.game)){ObjEdit.mesh_undos.set("leaf"); part.setLeafAttachment(attachments); changed=true;}
                     }
                  }
                  if(changed)ObjEdit.setChangedMesh(true, false);
               }
            }
         }
      }
      virtual void draw(C GuiPC &gpc)override
      {
         super.draw(gpc);
         Memc<LeafAttachment> &attachments=ObjEdit.leaf.attachments; if(attachments.elms())
         {
            Rect r=T.rect()+gpc.offset; r.swapY();
            FREPA(attachments)
            {
               LeafAttachment &at=attachments[i];
               Edge2 e;
               e.p[0]=r.lerp(at.center);
               e.p[1]=r.lerp(at.attachment);
               e.p[0].draw(PURPLE);
               e.draw(PURPLE);
            }
         }
      }
   }
   TextWhite ts;
   Text      leaf_attachment;
   Texture   texture;
   Button    remove_attachment, set_attachment_cam, random_bending, same_random_bending, remove_bending, random_color, remove_color;
   TextLine  color_value;
   Memc<LeafAttachment> attachments;

   static void RemoveAttachment(LeafRegion &leaf) {ObjEdit.remVtx(VTX_HLP  , true, MtrlEdit.game);}
   static void RemoveBending   (LeafRegion &leaf) {ObjEdit.remVtx(VTX_SIZE , true, MtrlEdit.game);}
   static void RemoveColor     (LeafRegion &leaf) {ObjEdit.remVtx(VTX_COLOR, true, MtrlEdit.game);}
   static void SetAttachmentCam(LeafRegion &leaf)
   {
      ObjEdit.mesh_undos.set("leaf");
      bool changed=false;
      Vec  pos=ObjEdit.v4.last().camera.at/ObjEdit.mesh_matrix;
      if(ObjEdit.mesh_parts.edit_selected())
      {
         MeshLod &lod=ObjEdit.getLod();
         Memt<int  > changed_parts;
         Memt<VecI2> vtxs; ObjEdit.getSelectedVtxs(vtxs);
         REPA(vtxs)
         {
          C VecI2 &v=vtxs[i]; if(MeshPart *part=lod.parts.addr(v.x))if(HasMaterial(*part, MtrlEdit.game))
            {
               MeshBase &base=part.base; if(InRange(v.y, base.vtx))
               {
                  if(base.vtx.hlp())
                  {
                     base.vtx.hlp(v.y)=pos;
                     changed_parts.binaryInclude(v.x);
                  }
               }
            }
         }
         REPA(changed_parts)
         {
            MeshPart &part=lod.parts[changed_parts[i]];
            part.setRender();
         }
         if(changed_parts.elms())changed=true;
      }else
      REPD(i, ObjEdit.mesh.lods())if(ObjEdit.selLod()==i || !ObjEdit.visibleLodSelection())
      {
         MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(ObjEdit.partOp(i) || !ObjEdit.mesh_parts.visibleOnActiveDesktop())
         {
            MeshPart &part=lod.parts[i]; if(HasMaterial(part, MtrlEdit.game)){part.setLeafAttachment(pos); changed=true;}
         }
      }
      if(changed)ObjEdit.setChangedMesh(true, false);
   }
   static void RandomBending(LeafRegion &leaf)
   {
      ObjEdit.mesh_undos.set("bending");
      bool changed=false;
      REPD(i, ObjEdit.mesh.lods())if(ObjEdit.selLod()==i || !ObjEdit.visibleLodSelection())
      {
         MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(ObjEdit.partOp(i) || !ObjEdit.mesh_parts.visibleOnActiveDesktop())
         {
            MeshPart &part=lod.parts[i]; if(HasMaterial(part, MtrlEdit.game) && (part.flag()&VTX_HLP)) // set only for parts that already have leaf attachment
            {
               part.setRandomLeafBending(); changed=true;
            }
         }
      }
      if(changed)ObjEdit.setChangedMesh(true, false);
   }
   static void SameRandomBending(LeafRegion &leaf)
   {
      ObjEdit.mesh_undos.set("bending");
      bool changed=false;
      flt  random=Random.f(1024);
      if(ObjEdit.mesh_parts.edit_selected())
      {
         MeshLod &lod=ObjEdit.getLod();
         Memt<int  > changed_parts;
         Memt<VecI2> vtxs; ObjEdit.getSelectedVtxs(vtxs);
         REPA(vtxs)
         {
          C VecI2 &v=vtxs[i]; if(MeshPart *part=lod.parts.addr(v.x))if(HasMaterial(*part, MtrlEdit.game) && (part.flag()&VTX_HLP)) // set only for parts that already have leaf attachment
            {
               MeshBase &base=part.base; if(InRange(v.y, base.vtx))
               {
                  if(!base.vtx.size())
                  {
                     base.include(VTX_SIZE);
                     ZeroN(base.vtx.size(), base.vtxs());
                  }
                  if(base.vtx.size())base.vtx.size(v.y)=random;
                  changed_parts.binaryInclude(v.x);
               }
            }
         }
         REPA(changed_parts)
         {
            MeshPart &part=lod.parts[changed_parts[i]];
            part.setRender();
         }
         if(changed_parts.elms())changed=true;
      }else
      REPD(i, ObjEdit.mesh.lods())if(ObjEdit.selLod()==i || !ObjEdit.visibleLodSelection())
      {
         MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(ObjEdit.partOp(i) || !ObjEdit.mesh_parts.visibleOnActiveDesktop())
         {
            MeshPart &part=lod.parts[i]; if(HasMaterial(part, MtrlEdit.game) && (part.flag()&VTX_HLP)) // set only for parts that already have leaf attachment
            {
               part.setRandomLeafBending(random); changed=true;
            }
         }
      }
      if(changed)ObjEdit.setChangedMesh(true, false);
   }
   static void RandomColor(LeafRegion &leaf)
   {
      ObjEdit.mesh_undos.set("color");
      flt  variation=TextFlt(leaf.color_value());
      bool changed=false;
      REPD(i, ObjEdit.mesh.lods())if(ObjEdit.selLod()==i || !ObjEdit.visibleLodSelection())
      {
         MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(ObjEdit.partOp(i) || !ObjEdit.mesh_parts.visibleOnActiveDesktop())
         {
            MeshPart &part=lod.parts[i]; if(HasMaterial(part, MtrlEdit.game)){part.setRandomLeafColor(variation); changed=true;}
         }
      }
      if(changed)ObjEdit.setChangedMesh(true, false);
   }
   bool meshHasMtrl(C MaterialPtr &mtrl)
   {
      REPD(i, ObjEdit.mesh.lods()){MeshLod &lod=ObjEdit.mesh.lod(i); REPA(lod.parts)if(HasMaterial(lod.parts[i], mtrl))return true;}
      return false;
   }

   LeafRegion& create()
   {
      super.create(Rect_LU(0, 0, 0.44+0.12, 0.793+0.12)).skin(&TransparentSkin, false); kb_lit=false;
      ts.reset(); ts.size=0.038; ts.align.set(1, 0);
      flt h=0.044, p=0.005, vh=h*0.97, y=-0.025, w=rect().w()-0.02;
      T+=leaf_attachment.create(Vec2(0.01, y), "Set Leaf Attachment", &ts); y-=h/2;
      T+=texture.create(Rect(0.01, y-w, rect().w()-0.01, y)).desc("Click on the image to set leaf attachment according to selected texture position.\nClick on approximate leaf/branch center then drag into attachment point.\nHold Ctrl to add new attachments.\nClick RMB to remove last attachment"); y-=w+h/2; texture.alpha_mode=ALPHA_NONE;
      T+=set_attachment_cam .create(Rect_L(0.01, y, w     , vh), "Set At Cam Target"      ).func(SetAttachmentCam , T).desc("This function will set Leaf Attachment at current Camera Target"); y-=h;
      T+=remove_attachment  .create(Rect_L(0.01, y, w     , vh), "Del Leaf Attachment"    ).func(RemoveAttachment , T); y-=h+p;
      T+=     random_bending.create(Rect_L(0.01, y, w     , vh), "Set Random Bending"     ).func(    RandomBending, T).desc("This function will force each leaf to bend differently"); y-=h;
      T+=same_random_bending.create(Rect_L(0.01, y, w     , vh), "Set Same Random Bending").func(SameRandomBending, T).desc("This function will set the same random bending value for selected mesh parts"); y-=h;
      T+=     remove_bending.create(Rect_L(0.01, y, w     , vh), "Del Random Bending"     ).func(    RemoveBending, T); y-=h+p;
      T+=random_color       .create(Rect_L(0.01, y, w-0.09, vh), "Set Random Color"       ).func(RandomColor      , T).desc("This function will randomize color of each leaf"); y-=h;
      T+=remove_color       .create(Rect_L(0.01, y, w-0.09, vh), "Del Random Color"       ).func(RemoveColor      , T); y-=h;
      T+=color_value        .create(Rect_L(random_color.rect().right()+Vec2(0.01, 0), 0.08, vh), "0.3").desc("Random color variation (0..1)");
      return T;
   }
   void clear() {attachments.clear();}
   virtual void update(C GuiPC &gpc)override
   {
      visible(MtrlEdit.visible() && (MtrlEdit.edit.tech==MTECH_LEAF_2D || MtrlEdit.edit.tech==MTECH_LEAF || MtrlEdit.edit.tech==MTECH_BLEND_LIGHT_LEAF || MtrlEdit.edit.tech==MTECH_TEST_BLEND_LIGHT_LEAF) && meshHasMtrl(MtrlEdit.game));
      super.update(gpc);
      if(Ms.bp(2) && contains(Gui.ms()))MtrlEdit.set(null);
   }
}
/******************************************************************************/
