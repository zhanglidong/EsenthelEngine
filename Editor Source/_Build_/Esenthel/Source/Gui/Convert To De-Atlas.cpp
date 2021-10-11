﻿/******************************************************************************/
#include "stdafx.h"
/******************************************************************************/
ConvertToDeAtlasClass ConvertToDeAtlas;
/******************************************************************************/

/******************************************************************************/
   cchar8 *ConvertToDeAtlasClass::mode_t[]=
   {
      "Create as new objects (use for testing)",
      "Replace existing objects (and keep Original Elements)",
      "Replace existing objects (and Disable Publishing of Original Elements)",
      "Replace existing objects (and Remove Original Elements)",
   };
/******************************************************************************/
      void ConvertToDeAtlasClass::Preview::draw(C GuiPC &gpc)
{
         if(gpc.visible && visible())
         {
            D.clip(gpc.clip);
            Rect r=rect()+gpc.offset;
            if(ImagePtr img=Proj.texPath(ConvertToDeAtlas.base_0_tex))
            {
               r=img->fit(r);
               ALPHA_MODE alpha=D.alpha(ALPHA_NONE);
               img->draw(r);
               Rect sr=ConvertToDeAtlas.source_rect; sr/=Vec2(ConvertToDeAtlas.tex_size);
               sr.min.y=1-sr.min.y;
               sr.max.y=1-sr.max.y;
               sr.min=r.lerp(sr.min);
               sr.max=r.lerp(sr.max);
               sr.draw(RED, false);
               D.alpha(alpha);
            }
            r.draw(Gui.borderColor(), false);
         }
      }
   Str  ConvertToDeAtlasClass::TexSize(C VecI2 &size) {return S+size.x+'x'+size.y;}
   void ConvertToDeAtlasClass::Convert(ConvertToDeAtlasClass &cta) {cta.convertDo();}
   void ConvertToDeAtlasClass::convertMeshes(Memc<IDReplace> &mtrl_replace, C Rect *frac)
   {
      Vec2 mul, add;  if(frac){mul=1/frac->size(); add=-frac->min*mul;}

      Memc<UID> duplicated; Proj.duplicate(objs, duplicated, (mode==NEW) ? " (De-Atlas)" : " (De-Atlas Backup)");
      //       objs  duplicated
      // NEW              atlas
      // REPLACE         backup
      if(mode==NEW)Swap(duplicated, objs);
      Memc<UID> &deatlased=objs, &non_deatlased=duplicated;

      REPA(deatlased)if(Elm *obj=Proj.findElm(deatlased[i]))if(ElmObj *obj_data=obj->objData())
                     if(Elm *mesh_elm=Proj.findElm(obj_data->mesh_id, ELM_MESH))
      {
         if(ObjEdit.mesh_elm==mesh_elm)ObjEdit.flushMeshSkel();
         Mesh mesh; if(Load(mesh, Proj.editPath(mesh_elm->id), Proj.game_path))
         {
            REPD(l, mesh.lods())
            {
               MeshLod &lod=mesh.lod(l); REPA(lod)
               {
                  MeshPart   &part=lod.parts[i];
                  bool        changed_part=false, changed_multi_mtrl=false;
                  MaterialPtr mtrls[4]; REPA(mtrls)
                  {
                     MaterialPtr &mtrl=mtrls[i];
                     mtrl=part.multiMaterial(i);
                     if(C IDReplace *id=ReplaceID(mtrl.id(), mtrl_replace)){changed_multi_mtrl=true; mtrl=Proj.gamePath(id->to);}
                  }
                  if(changed_multi_mtrl)
                  {
                     changed_part=true;
                     part.multiMaterial(mtrls[0], mtrls[1], mtrls[2], mtrls[3]);
                  }
                  REP(part.variations())if(i)
                     if(C IDReplace *id=ReplaceID(part.variation(i).id(), mtrl_replace)){changed_part=true; part.variation(i, Proj.gamePath(id->to));}
                  if(changed_part)
                  {
                     MeshBase &base=part.base; if(frac && base.vtx.tex0())
                     {
                        base.fixTexOffset();
                        REPA(base.vtx)
                        {
                           Vec2 &t=base.vtx.tex0(i);
                           t*=mul; t+=add;
                        }
                     }
                  }
               }
            }
            if(ElmMesh *mesh_data=mesh_elm->meshData()){mesh_data->newVer(); mesh_data->file_time.getUTC();}
            Save(mesh, Proj.editPath(mesh_elm->id), Proj.game_path);
            Proj.makeGameVer(*mesh_elm);
            Server.setElmLong(mesh_elm->id);
            if(ObjEdit.mesh_elm==mesh_elm)ObjEdit.reloadMeshSkel();
         }
      }

      // adjust parents, publishing and removed status
      TimeStamp time; time.getUTC();

      // process objects
      if(non_deatlased.elms()==deatlased.elms())
      {
         REPA(non_deatlased)if(Elm *elm=Proj.findElm(non_deatlased[i]))
         {
            if(mode==REPLACE_NO_PUBLISH || mode==REPLACE_REMOVE){elm->setParent(deatlased[i], time); Server.setElmParent(*elm);} // move 'non_deatlased' to 'deatlased'
            if(mode==REPLACE_NO_PUBLISH)elm->setNoPublish(true, time);else
            if(mode==REPLACE_REMOVE    )elm->setRemoved  (true, time);
         }
         if(mode==REPLACE_NO_PUBLISH)Server.noPublishElms(non_deatlased, true, time);else
         if(mode==REPLACE_REMOVE    )Server.removeElms   (non_deatlased, true, time);

         /*if(mode!=NEW) // move materials to old ones, from 'deatlased' -> 'non_deatlased'
         {
            REPA(mtrls)if(Elm *mtrl=Proj.findElm(mtrls[i]))
            {
               int index; if(deatlased.binarySearch(mtrl.parent_id, index)) // if is located in one of 'deatlased'
               {
                  mtrl.setParent(non_deatlased[index], time); Server.setElmParent(*mtrl);
               }
            }
         }*/
      }

      if(deatlased.elms()==1) // if created only one object, then move all materials to it
         REPA(mtrl_replace)if(Elm *mtrl=Proj.findElm(mtrl_replace[i].to))
      {
         mtrl->setParent(deatlased[0], time); Server.setElmParent(*mtrl);
      }
      
      if(mode==REPLACE_NO_PUBLISH || mode==REPLACE_REMOVE) // check if we can remove/unpublish old materials
      {
         Proj.setList(); // first we need to reset the list to set 'finalRemoved' and 'finalPublish'
         Memc<UID> used_mtrls_publish, used_mtrls_exist, remove, unpublish;
                                 Proj.getUsedMaterials(used_mtrls_publish, true ); // get list of materials used by publishable elements
         if(mode==REPLACE_REMOVE)Proj.getUsedMaterials(used_mtrls_exist  , false); // get list of materials used by existing    elements
         REPA(mtrls)
         {
          C UID &mtrl_id=mtrls[i];
            if(mode==REPLACE_REMOVE && !used_mtrls_exist  .binaryHas(mtrl_id))remove   .add(mtrl_id);else // first check if we can remove            , no other existing    element uses this material
            if(                        !used_mtrls_publish.binaryHas(mtrl_id))unpublish.add(mtrl_id);     // then  check if at least we can unpublish, no other publishable element uses this material
         }
         Proj.remove        (remove   , false);
         Proj.disablePublish(unpublish, false);
      }
   }
   Str ConvertToDeAtlasClass::Process(C Str &name, C Rect *crop, C VecI2 *resize)
   {
      if(name.is() && (crop || resize)) // don't add params to an empty file name
      {
         Mems<FileParams> files=FileParams::Decode(name); if(files.elms())
         {
            TextParam src_resize; ExtractResize(files, src_resize); // first remove source resize
            if(files.elms()>1)files.New(); // if there's more than 1 file, then add crop/resize parameters globally
            if(crop) // crop first
            {
               Image image; if(Proj.loadImages(image, null, FileParams::Encode(files))) // load image at current state to extract its size
               {
                  RectI r=Round((*crop)*(Vec2)image.size());
                  files.last().params.New().set("crop", S+r.min.x+','+r.min.y+','+r.w()+','+r.h());
               }
            }
            if(resize)files.last().params.New().set("resizeClamp", TextVecI2Ex(*resize)); // then resize
            return FileParams::Encode(files);
         }
      }
      return name;
   }
   void ConvertToDeAtlasClass::convertDo()
   {
      // convert before hiding because that may release resources
      if(mtrls.elms())
      {
         Proj.clearListSel();
         REPA(mtrls)MtrlEdit.flush(mtrls[i]); // flush first

         // load first material and adjust its textures
         EditMaterial first; first.load(Proj.editPath(mtrls[0]));

         Rect   frac=Rect(source_rect)/Vec2(tex_size);
         VecI2  final_size=finalSize();
       C Rect  *crop  =((source_rect.min.any() || source_rect.max!=tex_size) ? &frac       : null);
       C VecI2 *resize=((dest_size.x>0         || dest_size.y>0            ) ? &final_size : null);

         first. color_map=Process(first. color_map, crop, resize);
         first. alpha_map=Process(first. alpha_map, crop, resize);
         first.  bump_map=Process(first.  bump_map, crop, resize);
         first.normal_map=Process(first.normal_map, crop, resize);
         first.smooth_map=Process(first.smooth_map, crop, resize);
         first. metal_map=Process(first. metal_map, crop, resize);
         first.  glow_map=Process(first.  glow_map, crop, resize);

         // create textures
         Image base_0, base_1, base_2; Proj.createBaseTextures(base_0, base_1, base_2, first);
         IMAGE_TYPE ct; ImageProps(base_0, &first.base_0_tex, &ct, MTRL_BASE_0, first.tex_quality); if(Proj.includeTex(first.base_0_tex)){base_0.copyTry(base_0, -1, -1, -1, ct, IMAGE_2D, 0, FILTER_BEST, IC_WRAP); Proj.saveTex(base_0, first.base_0_tex);} Server.setTex(first.base_0_tex);
                        ImageProps(base_1, &first.base_1_tex, &ct, MTRL_BASE_1                   ); if(Proj.includeTex(first.base_1_tex)){base_1.copyTry(base_1, -1, -1, -1, ct, IMAGE_2D, 0, FILTER_BEST, IC_WRAP); Proj.saveTex(base_1, first.base_1_tex);} Server.setTex(first.base_1_tex);
                        ImageProps(base_2, &first.base_2_tex, &ct, MTRL_BASE_2                   ); if(Proj.includeTex(first.base_2_tex)){base_2.copyTry(base_2, -1, -1, -1, ct, IMAGE_2D, 0, FILTER_BEST, IC_WRAP); Proj.saveTex(base_2, first.base_2_tex);} Server.setTex(first.base_2_tex);

         // adjust all materials
         Memc<IDReplace> mtrl_replace;
         TimeStamp time; time.getUTC();
         REPA(mtrls)if(Elm *elm_mtrl=Proj.findElm(mtrls[i]))
         {
            Elm &de_atlas=Proj.Project::newElm(); de_atlas.type=ELM_MTRL;
            de_atlas.copyParams(*elm_mtrl).setName(elm_mtrl->name+" (De-Atlas)").setRemoved(false); // call 'setName' after 'copyParams'

            EditMaterial edit; edit.load(Proj.editPath(elm_mtrl->id));
            edit. color_map=first. color_map; edit. color_map_time=time;
            edit. alpha_map=first. alpha_map; edit. alpha_map_time=time;
            edit.  bump_map=first.  bump_map; edit.  bump_map_time=time;
            edit.normal_map=first.normal_map; edit.normal_map_time=time;
            edit.smooth_map=first.smooth_map; edit.smooth_map_time=time;
            edit. metal_map=first. metal_map; edit. metal_map_time=time;
            edit.  glow_map=first.  glow_map; edit.  glow_map_time=time;
            edit.base_0_tex=first.base_0_tex;
            edit.base_1_tex=first.base_1_tex;
            edit.base_2_tex=first.base_2_tex;
            if(ElmMaterial *mtrl_data=de_atlas.mtrlData())mtrl_data->from(edit);
            Save(edit, Proj.editPath(de_atlas.id));
            Proj.makeGameVer(de_atlas);

            Server.setElmFull(de_atlas.id);
            mtrl_replace.New().set(elm_mtrl->id, de_atlas.id);
         }

         mtrl_replace.sort(IDReplace::Compare);
         convertMeshes(mtrl_replace, crop);
         Proj.setList();
      }
      hide();
   }
   void ConvertToDeAtlasClass::clearProj()
   {
      objs.clear(); mtrls.clear(); base_0_tex.zero(); base_1_tex.zero(); base_2_tex.zero();
   }
   void ConvertToDeAtlasClass::create()
   {
Property &mode=add("De-Atlased Objects", MEMBER(ConvertToDeAtlasClass, mode)).setEnum(mode_t, Elms(mode_t)).desc("Existing object meshes need to have their UV adjusted.\nWith this option you can control if the adjusted objects:\n-Replace the old ones (keeping their Element ID)\nor\n-They are created as new objects (with new Element ID)");
               add("Source Left"       , MEMBER(ConvertToDeAtlasClass, source_rect.min.x));
               add("Source Right"      , MEMBER(ConvertToDeAtlasClass, source_rect.max.x));
               add("Source Top"        , MEMBER(ConvertToDeAtlasClass, source_rect.min.y));
               add("Source Bottom"     , MEMBER(ConvertToDeAtlasClass, source_rect.max.y));
           sw=&add();
           sh=&add();
               add("Force Width"       , MEMBER(ConvertToDeAtlasClass, dest_size.x)).min(-1);
               add("Force Height"      , MEMBER(ConvertToDeAtlasClass, dest_size.y)).min(-1);
            w=&add();
            h=&add();
      Rect r=super::create("Extract from Atlas", Vec2(0.02f, -0.02f), 0.036f, 0.043f, 0.2f); button[2].func(HideProjAct, SCAST(GuiObj, T)).show(); mode.combobox.resize(Vec2(0.73f, 0));
      autoData(this);
      T+=preview.create(Rect_LU(r.ru()+Vec2(0.02f, -0.06f), 1.4f));
      T+=t_tex_size.create(preview.rect().ru()+Vec2(-0.15f, 0.02f));
      T+=convert.create(Rect_U (r.down()-Vec2(0, 0.05f), 0.3f, 0.055f), "Convert").func(Convert, T);
      Vec2 size(preview.rect().max.x, Max(-convert.rect().min.y, -preview.rect().min.y));
      rect(Rect_C(0, size+0.02f+defaultInnerPaddingSize()));
   }
   VecI2 ConvertToDeAtlasClass::finalSize()C {return ImageSize(VecI(source_rect.size(), 1), dest_size, true).xy;}
   void ConvertToDeAtlasClass::setElms(C MemPtr<UID> &elm_ids)
   {
      clearProj();
      Str mtrl_color_map;
      FREPA(elm_ids) // process in order so materials are chosen from object that was selected first
         if(Elm *elm=Proj.findElm(elm_ids[i]))if(ElmObj *obj_data=elm->objData())if(Elm *mesh_elm=Proj.findElm(obj_data->mesh_id))if(ElmMesh *mesh_data=mesh_elm->meshData())
      {
         bool include_obj=false;
         FREPA(mesh_data->mtrl_ids)if(Elm *elm_mtrl=Proj.findElm(mesh_data->mtrl_ids[i]))if(ElmMaterial *mtrl_data=elm_mtrl->mtrlData())
            if(mtrl_data->base_0_tex.valid() || mtrl_data->base_1_tex.valid() || mtrl_data->base_2_tex.valid())
         {
            if(!base_0_tex.valid() && !base_1_tex.valid() && !base_2_tex.valid())
            {
               EditMaterial mtrl; Proj.mtrlGet(elm_mtrl->id, mtrl); mtrl_color_map=mtrl.color_map;
               base_0_tex=mtrl_data->base_0_tex;
               base_1_tex=mtrl_data->base_1_tex;
               base_2_tex=mtrl_data->base_2_tex;
            }
            if(mtrl_data->base_0_tex==base_0_tex
            && mtrl_data->base_1_tex==base_1_tex
            && mtrl_data->base_2_tex==base_2_tex)
            {
               mtrls.binaryInclude(elm_mtrl->id);
               include_obj=true;
            }
         }
         if(include_obj)objs.binaryInclude(elm->id);
      }

      if(!objs.elms())Gui.msgBox(S, "No Objects/Materials to process");else
      {
         // get 'tex_size'
         tex_size=0;
         Image temp; TextParam temp_resize; if(Proj.loadImages(temp, &temp_resize, mtrl_color_map))tex_size=temp.size(); // first try loading from source image (because it may not be pow2) without any resize, we want exact size, can ignore sRGB
         if(!tex_size.all())if(ImagePtr base_0=Proj.texPath(base_0_tex))tex_size=base_0->size(); // if failed to load then use base tex size
         t_tex_size.set(TexSize(tex_size));

         bool used_tex=false;
         Rect used_tex_rect(0, 0);
         REPA(objs)if(Elm *elm=Proj.findElm(objs[i]))if(ElmObj *obj_data=elm->objData())if(Elm *mesh_elm=Proj.findElm(obj_data->mesh_id))if(ElmMesh *mesh_data=mesh_elm->meshData())
         {
            Mesh mesh; if(ObjEdit.mesh_elm==mesh_elm)mesh.create(ObjEdit.mesh);else Load(mesh, Proj.editPath(mesh_elm->id), Proj.game_path); // load edit so we can have access to MeshBase
            REPD(l, mesh.lods())
            {
             C MeshLod &lod=mesh.lod(l); REPA(lod)
               {
                C MeshPart &part=lod.parts[i];
                C MeshBase &base=part.base; if(base.vtx.tex0())REP(4)
                  {
                     UID mtrl_id=part.multiMaterial(i).id(); if(mtrl_id.valid() && mtrls.binaryHas(mtrl_id))
                     {
                        REPA(base.tri)
                        {
                         C VecI &ind=base.tri.ind(i);
                           Rect face_tex=Tri2(base.vtx.tex0(ind.x), base.vtx.tex0(ind.y), base.vtx.tex0(ind.z));
                           face_tex-=Floor(face_tex.min);
                           Include(used_tex_rect, used_tex, face_tex);
                        }
                        REPA(base.quad)
                        {
                         C VecI4 &ind=base.quad.ind(i);
                           Rect face_tex=Quad2(base.vtx.tex0(ind.x), base.vtx.tex0(ind.y), base.vtx.tex0(ind.z), base.vtx.tex0(ind.w));
                           face_tex-=Floor(face_tex.min);
                           Include(used_tex_rect, used_tex, face_tex);
                        }
                        break;
                     }
                  }
               }
            }
         }

         if(used_tex)source_rect.set(Floor(used_tex_rect.min*(Vec2)tex_size), Ceil(used_tex_rect.max*(Vec2)tex_size));else source_rect.zero();
         toGui();

         activate();
      }
   }
   void ConvertToDeAtlasClass::drag(Memc<UID> &elms, GuiObj *focus_obj, C Vec2 &screen_pos)
   {
   }
   ConvertToDeAtlasClass& ConvertToDeAtlasClass::hide()
{
      super::hide();
      clearProj(); // release memory
      return T;
   }
   void ConvertToDeAtlasClass::update(C GuiPC &gpc)
{
      super::update(gpc);
      if(gpc.visible && visible())
      {
         VecI2 size=finalSize();
         if(sw)sw->name.set(S+"Source Width: " +source_rect.w());
         if(sh)sh->name.set(S+"Source Height: "+source_rect.h());
         if(w)w->name.set(S+"Width: " +size.x);
         if(h)h->name.set(S+"Height: "+size.y);
      }
   }
ConvertToDeAtlasClass::ConvertToDeAtlasClass() : base_0_tex(UIDZero), base_1_tex(UIDZero), base_2_tex(UIDZero), mode(NEW), source_rect(0, 0, 0, 0), dest_size(0), tex_size(0), w(null), h(null), sw(null), sh(null) {}

/******************************************************************************/
