/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
#define MacroFrom   192.0
#define MacroTo     320.0
#define MacroMax    0.70
#define MacroScale (1.0/32)

#define DEFAULT_TEX_SIZE 1024.0 // 1024x1024

#define PARALLAX_MODE 1 // 1=best

#define RELIEF_STEPS_MAX    32
#define RELIEF_STEPS_BINARY 3 // 3 works good in most cases, 4 could be used for absolute best quality
#define RELIEF_STEPS_MUL    0.75 // 0.75 gets slightly worse quality but better performance, 1.0 gets full quality but slower performance, default=0.75
#define RELIEF_LOD_OFFSET   0.33 // values >0 increase performance (by using fewer steps and smaller LOD's) which also makes results more soft and flat helping to reduce jitter for distant pixels, default=0.33
#define RELIEF_TAN_POS      1 // 0=gets worse quality but better performance (not good for triangles with vertexes with very different normals or for surfaces very close to camera), 1=gets full quality but slower performance, default=1
#define RELIEF_DEC_NRM      1 // if reduce relief bump intensity where there are big differences between vtx normals, tangents and binormals, default=1
#define RELIEF_MODE         1 // 1=best
#define RELIEF_Z_LIMIT      0.4 // smaller values may cause leaking (UV swimming), and higher reduce bump intensity at angles, default=0.4
#define RELIEF_LOD_TEST     0 // close to camera (test enabled=4.76 fps, test disabled=4.99 fps), far from camera (test enabled=9.83 fps, test disabled=9.52 fps), conclusion: this test reduces performance when close to camera by a similar factor to when far away, however since more likely pixels will be close to camera (as for distant LOD's other shaders are used) we prioritize close to camera performance, so this check should be disabled, default=0

#define FAST_TPOS ((BUMP_MODE>=SBUMP_PARALLAX_MIN && BUMP_MODE<=SBUMP_PARALLAX_MAX) || (BUMP_MODE==SBUMP_RELIEF && !RELIEF_TAN_POS))
/******************************************************************************
SKIN, MATERIALS, LAYOUT, BUMP_MODE, ALPHA_TEST, DETAIL, MACRO, REFLECT, COLORS, MTRL_BLEND, HEIGHTMAP, FX, TESSELATE
/******************************************************************************/
struct VS_PS
{
   Vec2 tex:TEXCOORD;
   Vec  pos:POS; // always needed for velocity
   Vec  vel:VELOCITY;

#if   BUMP_MODE> SBUMP_FLAT
   MatrixH3 mtrx:MATRIX; // !! may not be Normalized !!
   VecH Nrm() {return mtrx[2];}
#elif BUMP_MODE==SBUMP_FLAT
   VecH nrm:NORMAL; // !! may not be Normalized !!
   VecH Nrm() {return nrm;}
#else
   VecH Nrm() {return 0;}
#endif

#if MATERIALS>1
   VecH4 material:MATERIAL;
#endif

#if COLORS
   VecH col:COLOR;
#endif

#if FAST_TPOS
   Vec _tpos:TPOS;
   Vec  tpos() {return Normalize(_tpos);}
#elif BUMP_MODE>SBUMP_FLAT
   Vec  tpos() {return Normalize(TransformTP(-pos, mtrx));} // need high precision here for 'TransformTP'
#else
   Vec  tpos() {return 0;}
#endif

#if FX==FX_GRASS
   Half fade_out:FADE_OUT;
#endif
};
/******************************************************************************/
// VS
/******************************************************************************/
void VS
(
   VtxInput vtx,

   out VS_PS O,
   out Vec4  O_vtx:POSITION,

   CLIP_DIST
)
{
   Vec  pos=vtx.pos();
   VecH nrm, tan; if(BUMP_MODE>=SBUMP_FLAT)nrm=vtx.nrm(); if(BUMP_MODE>SBUMP_FLAT)tan=vtx.tan(nrm, HEIGHTMAP);

#if LAYOUT || DETAIL
   O.tex=vtx.tex(HEIGHTMAP);
   if(HEIGHTMAP && MATERIALS==1)O.tex*=MaterialTexScale();
#endif

#if MATERIALS>1
   O.material=vtx.material();
#endif

#if COLORS
   if(MATERIALS<=1)O.col=vtx.colorFast3()*MaterialColor3();
   else            O.col=vtx.colorFast3();
#endif

   if(FX==FX_LEAF)
   {
      if(BUMP_MODE> SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm, tan);else
      if(BUMP_MODE==SBUMP_FLAT)BendLeaf(vtx.hlp(), pos, nrm     );else
                               BendLeaf(vtx.hlp(), pos          );
   }
   if(FX==FX_LEAFS)
   {
      if(BUMP_MODE> SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm, tan);else
      if(BUMP_MODE==SBUMP_FLAT)BendLeafs(vtx.hlp(), vtx.size(), pos, nrm     );else
                               BendLeafs(vtx.hlp(), vtx.size(), pos          );
   }

   if(!SKIN)
   {
      if(true) // instance
      {
         O.pos=TransformPos(pos,        vtx.instance());
         O.vel=GetObjVel   (pos, O.pos, vtx.instance());

      #if   BUMP_MODE> SBUMP_FLAT
         O.mtrx[2]=TransformDir(nrm, vtx.instance());
         O.mtrx[0]=TransformDir(tan, vtx.instance());
      #elif BUMP_MODE==SBUMP_FLAT
         O.nrm    =TransformDir(nrm, vtx.instance());
      #endif

      #if FX==FX_GRASS
           BendGrass(pos, O.pos, vtx.instance());
         O.fade_out=GrassFadeOut(vtx.instance());
      #endif
      }else
      {
         O.pos=TransformPos(pos);
         O.vel=GetObjVel   (pos, O.pos);

      #if   BUMP_MODE> SBUMP_FLAT
         O.mtrx[2]=TransformDir(nrm);
         O.mtrx[0]=TransformDir(tan);
      #elif BUMP_MODE==SBUMP_FLAT
         O.nrm    =TransformDir(nrm);
      #endif

      #if FX==FX_GRASS
         BendGrass(pos, O.pos);
         O.fade_out=GrassFadeOut();
      #endif
      }
   }else
   {
      VecU bone=vtx.bone();
      O.pos=TransformPos(pos,        bone, vtx.weight());
      O.vel=GetBoneVel  (pos, O.pos, bone, vtx.weight());

   #if   BUMP_MODE> SBUMP_FLAT
      O.mtrx[2]=TransformDir(nrm, bone, vtx.weight());
      O.mtrx[0]=TransformDir(tan, bone, vtx.weight());
   #elif BUMP_MODE==SBUMP_FLAT
      O.nrm    =TransformDir(nrm, bone, vtx.weight());
   #endif
   }

   // normalize (have to do all at the same time, so all have the same lengths)
   if(BUMP_MODE> SBUMP_FLAT // calculating binormal (this also covers the case when we have tangent from heightmap which is not Normalized)
   || BUMP_MODE==SBUMP_RELIEF && RELIEF_DEC_NRM // needed for RELIEF_DEC_NRM effect
   || TESSELATE) // needed for tesselation
   {
   #if   BUMP_MODE> SBUMP_FLAT
      O.mtrx[2]=Normalize(O.mtrx[2]);
      O.mtrx[0]=Normalize(O.mtrx[0]);
   #elif BUMP_MODE==SBUMP_FLAT
      O.nrm    =Normalize(O.nrm);
   #endif
   }

#if BUMP_MODE>SBUMP_FLAT
   O.mtrx[1]=vtx.bin(O.mtrx[2], O.mtrx[0], HEIGHTMAP);
#endif

#if FAST_TPOS
   O._tpos=TransformTP(-O.pos, O.mtrx); // need high precision here, we can't Normalize because it's important to keep distances
#endif

   O_vtx=Project(O.pos); CLIP_PLANE(O.pos);
}
/******************************************************************************/
// PS
/******************************************************************************/
void PS
(
   VS_PS I,

#if FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   IS_FRONT,
#endif

   out DeferredSolidOutput output
)
{
   VecH col, nrm;
   Half glow, specular;

#if COLORS
   col=I.col;
#else
   if(MATERIALS<=1)col=MaterialColor3();
#endif

#if BUMP_MODE==SBUMP_ZERO
   glow    =MaterialGlow    ();
   specular=MaterialSpecular();
   nrm     =0;
#elif MATERIALS==1
   // apply tex coord bump offset
   #if LAYOUT==2
   {
      #if BUMP_MODE>=SBUMP_PARALLAX_MIN && BUMP_MODE<=SBUMP_PARALLAX_MAX // Parallax
      {
         const Int steps=BUMP_MODE-SBUMP_PARALLAX0;

         VecH tpos=I.tpos();
      #if   PARALLAX_MODE==0 // too flat
         Half scale=MaterialBump()/steps;
      #elif PARALLAX_MODE==1 // best results (not as flat, and not much aliasing)
         Half scale=MaterialBump()/(steps*Lerp(1, tpos.z, tpos.z)); // MaterialBump()/steps/Lerp(1, tpos.z, tpos.z);
      #elif PARALLAX_MODE==2 // generates too steep walls (not good for parallax)
         Half scale=MaterialBump()/(steps*Lerp(1, tpos.z, Sat(tpos.z/0.5))); // MaterialBump()/steps/Lerp(1, tpos.z, tpos.z);
      #elif PARALLAX_MODE==3 // introduces a bit too much aliasing/artifacts on surfaces perpendicular to view direction
         Half scale=MaterialBump()/steps*(2-tpos.z); // MaterialBump()/steps*Lerp(1, 1/tpos.z, tpos.z)
      #else // correct however introduces way too much aliasing/artifacts on surfaces perpendicular to view direction
         Half scale=MaterialBump()/steps/tpos.z;
      #endif
         tpos.xy*=scale; VecH2 add=-0.5*tpos.xy;
         UNROLL for(Int i=0; i<steps; i++)I.tex+=Tex(Col, I.tex).w*tpos.xy+add; // (tex-0.5)*tpos.xy = tex*tpos.xy + -0.5*tpos.xy
      }
      #elif BUMP_MODE==SBUMP_RELIEF // Relief
      {
      #if RELIEF_LOD_TEST
         BRANCH if(GetLod(I.tex, DEFAULT_TEX_SIZE)<=4)
      #endif
         {
         #if !GL
            Vec2 TexSize; Col.GetDimensions(TexSize.x, TexSize.y);
         #else
            Flt TexSize=DEFAULT_TEX_SIZE; // on GL using 'GetDimensions' would force create a secondary sampler for 'Col'
         #endif
            Flt lod=Max(0, GetLod(I.tex, TexSize)+RELIEF_LOD_OFFSET); // yes, can be negative, so use Max(0) to avoid increasing number of steps when surface is close to camera
            //lod=Trunc(lod); don't do this as it would reduce performance and generate more artifacts, with this disabled, we generate fewer steps gradually, and blend with the next MIP level softening results

            VecH tpos=I.tpos();
         #if   RELIEF_MODE==0
            Half scale=MaterialBump();
         #elif RELIEF_MODE==1 // best
            Half scale=MaterialBump()/Lerp(1, tpos.z, Sat(tpos.z/RELIEF_Z_LIMIT));
         #elif RELIEF_MODE==2 // produces slight aliasing/artifacts on surfaces perpendicular to view direction
            Half scale=MaterialBump()/Max(tpos.z, RELIEF_Z_LIMIT);
         #else // correct however introduces way too much aliasing/artifacts on surfaces perpendicular to view direction
            Half scale=MaterialBump()/tpos.z;
         #endif

         #if RELIEF_DEC_NRM
            scale*=Length2(I.mtrx[0])*Length2(I.mtrx[1])*Length2(I.mtrx[2]); // vtx matrix vectors are interpolated linearly, which means that if there are big differences between vtx vectors, then their length will be smaller and smaller, for example if #0 vtx normal is (1,0), and #1 vtx normal is (0,1), then interpolated value between them will be (0.5, 0.5)
         #endif
            tpos.xy*=-scale;

            Flt length=Length(tpos.xy) * TexSize.x / Pow(2, lod); // how many pixels
            if(RELIEF_STEPS_MUL!=1)if(lod>0)length*=RELIEF_STEPS_MUL; // don't use this for first LOD

            I.tex-=tpos.xy*0.5;

            Int  steps   =Mid(length, 0, RELIEF_STEPS_MAX);
            Half stp     =1.0/(steps+1),
                 ray     =1;
            Vec2 tex_step=tpos.xy*stp; // keep as HP to avoid conversions several times in the loop below

         #if 1 // linear + interval search (faster)
            // linear search
            Half height_next, height_prev=0.5; // use 0.5 as approximate average value, we could do "TexLodI(Col, I.tex, lod).w", however in tests that wasn't needed but only reduced performance
            LOOP for(Int i=0; ; i++)
            {
               ray  -=stp;
               I.tex+=tex_step;
               height_next=TexLodI(Col, I.tex, lod).w;
               if(i>=steps || height_next>=ray)break;
               height_prev=height_next;
            }

            // interval search
            if(1)
            {
               Half ray_prev=ray+stp;
               // prev pos: I.tex-tex_step, height_prev-ray_prev
               // next pos: I.tex         , height_next-ray
               Half hn=height_next-ray,
                    hp=height_prev-ray_prev,
                    frac=Sat(hn/(hn-hp));
               I.tex-=tex_step*frac;

               BRANCH if(lod<=0) // extra step (needed only for closeup)
               {
                  Half ray_cur=ray+stp*frac,
                       height_cur=TexLodI(Col, I.tex, lod).w;
                  if(  height_cur>=ray_cur) // if still below, then have to go back more, lerp between this position and prev pos
                  {
                     // prev pos: I.tex-tex_step (BUT I.tex before adjustment), height_prev-ray_prev
                     // next pos: I.tex                                       , height_cur -ray_cur
                     tex_step*=1-frac; // we've just travelled "tex_step*frac", so to go to the prev point, we need what's left, "tex_step*(1-frac)"
                  }else // we went back too far, go forward, lerp between next pos and this position
                  {
                     // prev pos: I.tex                              , height_cur -ray_cur
                     // next pos: I.tex (BUT I.tex before adjustment), height_next-ray
                     hp=hn;
                     tex_step*=-frac; // we've just travelled "tex_step*frac", so to go to the next point, we need the same way but other direction, "tex_step*-frac"
                  }
                  hn=height_cur-ray_cur;
                  frac=Sat(hn/(hn-hp));
                  I.tex-=tex_step*frac;
               }
            }
         #else // linear + binary search (slower because requires 3 tex reads in binary to get the same results as with only 0-1 tex reads in interval)
            // linear search
            LOOP for(Int i=0; ; i++)
            {
               ray  -=stp;
               I.tex+=tex_step;
               if(i>=steps || TexLodI(Col, I.tex, lod).w>=ray)break;
            }

            // binary search
            {
               Half ray_prev=ray+stp,
                    l=0, r=1, m=0.5;
               UNROLL for(Int i=0; i<RELIEF_STEPS_BINARY; i++)
               {
                  Half height=TexLodI(Col, I.tex-tex_step*m, lod).w;
                  if(  height>Lerp(ray, ray_prev, m))l=m;else r=m;
                  m=Avg(l, r);
               }
               I.tex-=tex_step*m;
            }
         #endif
         }
      }
      #endif
   }
   #endif

   VecH4 tex_nrm; // #MaterialTextureChannelOrder
   if(LAYOUT==0)
   {
      if(DETAIL)col+=GetDetail(I.tex).z;
      nrm     =Normalize(I.Nrm());
      glow    =MaterialGlow    ();
      specular=MaterialSpecular();
   }else
   if(LAYOUT==1)
   {
      VecH4 tex_col=Tex(Col, I.tex);
      if(ALPHA_TEST)
      {
      #if FX==FX_GRASS
         tex_col.a-=I.fade_out;
      #endif
         AlphaTest(tex_col.a);
      }
      col    *=tex_col.rgb; if(DETAIL)col+=GetDetail(I.tex).z;
      nrm     =Normalize(I.Nrm());
      glow    =MaterialGlow    ();
      specular=MaterialSpecular();
   }else
   if(LAYOUT==2)
   {
      tex_nrm=Tex(Nrm, I.tex); // #MaterialTextureChannelOrder
      if(ALPHA_TEST)
      {
      #if FX==FX_GRASS
         tex_nrm.w-=I.fade_out;
      #endif
         AlphaTest(tex_nrm.w);
      }
      glow    =MaterialGlow    (); if(!ALPHA_TEST)glow*=tex_nrm.a;
      specular=MaterialSpecular()*tex_nrm.z;

   #if BUMP_MODE==SBUMP_FLAT
      nrm =Normalize(I.Nrm());
      col*=Tex(Col, I.tex).rgb; if(DETAIL)col+=GetDetail(I.tex).z;
   #elif BUMP_MODE>SBUMP_FLAT // normal mapping
      VecH      det;
      if(DETAIL)det=GetDetail(I.tex);

                nrm.xy =(tex_nrm.xy*2-1)*MaterialRough();
      if(DETAIL)nrm.xy+=det.xy;
                nrm.z  =CalcZ(nrm.xy);
                nrm    =Normalize(Transform(nrm, I.mtrx));

                col*=Tex(Col, I.tex).rgb;
      if(DETAIL)col+=det.z;
   #endif
   }

   if(MACRO)col=Lerp(col, Tex(Mac, I.tex*MacroScale).rgb, LerpRS(MacroFrom, MacroTo, I.pos.z)*MacroMax);

   // reflection
   if(REFLECT)
   {
      Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
      col+=TexCube(Rfl, rfl).rgb*((LAYOUT==2) ? MaterialReflect()*tex_nrm.z : MaterialReflect());
   }
#else // MATERIALS>1
   // assuming that in multi materials LAYOUT!=0
   Vec2 tex0, tex1, tex2, tex3;
                   tex0=I.tex*MultiMaterial0TexScale();
                   tex1=I.tex*MultiMaterial1TexScale();
   if(MATERIALS>=3)tex2=I.tex*MultiMaterial2TexScale();
   if(MATERIALS>=4)tex3=I.tex*MultiMaterial3TexScale();

   // apply tex coord bump offset
   if(LAYOUT==2)
   {
      #if BUMP_MODE>=SBUMP_PARALLAX_MIN && BUMP_MODE<=SBUMP_PARALLAX_MAX // Parallax
      {
         const Int steps=BUMP_MODE-SBUMP_PARALLAX0;

         VecH tpos=I.tpos();
      #if   PARALLAX_MODE==0 // too flat
         Half scale=1/steps;
      #elif PARALLAX_MODE==1 // best results (not as flat, and not much aliasing)
         Half scale=1/(steps*Lerp(1, tpos.z, tpos.z)); // 1/steps/Lerp(1, tpos.z, tpos.z);
      #elif PARALLAX_MODE==2 // generates too steep walls (not good for parallax)
         Half scale=1/(steps*Lerp(1, tpos.z, Sat(tpos.z/0.5)));
      #elif PARALLAX_MODE==3 // introduces a bit too much aliasing/artifacts on surfaces perpendicular to view direction
         Half scale=1/steps*(2-tpos.z); // 1/steps*Lerp(1, 1/tpos.z, tpos.z)
      #else // correct however introduces way too much aliasing/artifacts on surfaces perpendicular to view direction
         Half scale=1/steps/tpos.z;
      #endif
         tpos.xy*=scale;

         // h=(tex-0.5)*bump_mul = x*bump_mul - 0.5*bump_mul
         VecH4 bump_mul; bump_mul.x=MultiMaterial0Bump();
         VecH4 bump_add; bump_mul.y=MultiMaterial1Bump(); if(MATERIALS==2){bump_mul.xy  *=I.material.xy  ; bump_add.xy  =bump_mul.xy  *-0.5;}
         if(MATERIALS>=3)bump_mul.z=MultiMaterial2Bump(); if(MATERIALS==3){bump_mul.xyz *=I.material.xyz ; bump_add.xyz =bump_mul.xyz *-0.5;}
         if(MATERIALS>=4)bump_mul.w=MultiMaterial3Bump(); if(MATERIALS==4){bump_mul.xyzw*=I.material.xyzw; bump_add.xyzw=bump_mul.xyzw*-0.5;}

         UNROLL for(Int i=0; i<steps; i++) // I.tex+=h*tpos.xy;
         {
                       Half h =Tex(Col , tex0).w*bump_mul.x+bump_add.x
                              +Tex(Col1, tex1).w*bump_mul.y+bump_add.y;
            if(MATERIALS>=3)h+=Tex(Col2, tex2).w*bump_mul.z+bump_add.z;
            if(MATERIALS>=4)h+=Tex(Col3, tex3).w*bump_mul.w+bump_add.w;

            Vec2 offset=h*tpos.xy; // keep as HP to avoid multiple conversions below

                            tex0+=offset;
                            tex1+=offset;
            if(MATERIALS>=3)tex2+=offset;
            if(MATERIALS>=4)tex3+=offset;
         }
      }
      #elif BUMP_MODE==SBUMP_RELIEF // Relief
      {
      #if RELIEF_LOD_TEST
         BRANCH if(GetLod(I.tex, DEFAULT_TEX_SIZE)<=4)
      #endif
         {
            VecH4 bump_mul; bump_mul.x=MultiMaterial0Bump(); Half avg_bump;
                            bump_mul.y=MultiMaterial1Bump(); if(MATERIALS==2){bump_mul.xy  *=I.material.xy  ; avg_bump=Sum(bump_mul.xy  );} // use 'Sum' because they're premultipled by 'I.material'
            if(MATERIALS>=3)bump_mul.z=MultiMaterial2Bump(); if(MATERIALS==3){bump_mul.xyz *=I.material.xyz ; avg_bump=Sum(bump_mul.xyz );}
            if(MATERIALS>=4)bump_mul.w=MultiMaterial3Bump(); if(MATERIALS==4){bump_mul.xyzw*=I.material.xyzw; avg_bump=Sum(bump_mul.xyzw);}

            Flt TexSize=DEFAULT_TEX_SIZE, // here we have 2..4 textures, so use a default value
                    lod=Max(0, GetLod(I.tex, TexSize)+RELIEF_LOD_OFFSET); // yes, can be negative, so use Max(0) to avoid increasing number of steps when surface is close to camera
            //lod=Trunc(lod); don't do this as it would reduce performance and generate more artifacts, with this disabled, we generate fewer steps gradually, and blend with the next MIP level softening results

            VecH tpos=I.tpos();
         #if   RELIEF_MODE==0
            Half scale=avg_bump;
         #elif RELIEF_MODE==1 // best
            Half scale=avg_bump/Lerp(1, tpos.z, Sat(tpos.z/RELIEF_Z_LIMIT));
         #elif RELIEF_MODE==2 // produces slight aliasing/artifacts on surfaces perpendicular to view direction
            Half scale=avg_bump/Max(tpos.z, RELIEF_Z_LIMIT);
         #else // correct however introduces way too much aliasing/artifacts on surfaces perpendicular to view direction
            Half scale=avg_bump/tpos.z;
         #endif

         #if RELIEF_DEC_NRM
            scale*=Length2(I.mtrx[0])*Length2(I.mtrx[1])*Length2(I.mtrx[2]); // vtx matrix vectors are interpolated linearly, which means that if there are big differences between vtx vectors, then their length will be smaller and smaller, for example if #0 vtx normal is (1,0), and #1 vtx normal is (0,1), then interpolated value between them will be (0.5, 0.5)
         #endif
            tpos.xy*=-scale;

            Flt length=Length(tpos.xy) * TexSize.x / Pow(2, lod); // how many pixels
            if(RELIEF_STEPS_MUL!=1)if(lod>0)length*=RELIEF_STEPS_MUL; // don't use this for first LOD

            //I.tex-=tpos.xy*0.5;
            Vec2 offset=tpos.xy*0.5; // keep as HP to avoid multiple conversions below
                            tex0-=offset;
                            tex1-=offset;
            if(MATERIALS>=3)tex2-=offset;
            if(MATERIALS>=4)tex3-=offset;

            Int  steps   =Mid(length, 0, RELIEF_STEPS_MAX);
            Half stp     =1.0/(steps+1),
                 ray     =1;
            Vec2 tex_step=tpos.xy*stp; // keep as HP to avoid conversions several times in the loop below

         #if 1 // linear + interval search (faster)
            // linear search
            Half height_next, height_prev=0.5; // use 0.5 as approximate average value, we could do "TexLodI(Col, I.tex, lod).w", however in tests that wasn't needed but only reduced performance
            LOOP for(Int i=0; ; i++)
            {
               ray-=stp;

               //I.tex+=tex_step;
                               tex0+=tex_step;
                               tex1+=tex_step;
               if(MATERIALS>=3)tex2+=tex_step;
               if(MATERIALS>=4)tex3+=tex_step;

               //height_next=TexLodI(Col, I.tex, lod).w;
                               height_next =TexLodI(Col , tex0, lod).w*I.material.x
                                           +TexLodI(Col1, tex1, lod).w*I.material.y;
               if(MATERIALS>=3)height_next+=TexLodI(Col2, tex2, lod).w*I.material.z;
               if(MATERIALS>=4)height_next+=TexLodI(Col3, tex3, lod).w*I.material.w;

               if(i>=steps || height_next>=ray)break;
               height_prev=height_next;
            }

            // interval search
            if(1)
            {
               Half ray_prev=ray+stp;
               // prev pos: I.tex-tex_step, height_prev-ray_prev
               // next pos: I.tex         , height_next-ray
               Half hn=height_next-ray, hp=height_prev-ray_prev,
                  frac=Sat(hn/(hn-hp));

               //I.tex-=tex_step*frac;
               offset=tex_step*frac;
                               tex0-=offset;
                               tex1-=offset;
               if(MATERIALS>=3)tex2-=offset;
               if(MATERIALS>=4)tex3-=offset;

               BRANCH if(lod<=0) // extra step (needed only for closeup)
               {
                  Half ray_cur=ray+stp*frac,
                     //height_cur=TexLodI(Col, I.tex, lod).w;
                                  height_cur =TexLodI(Col , tex0, lod).w*I.material.x
                                             +TexLodI(Col1, tex1, lod).w*I.material.y;
                  if(MATERIALS>=3)height_cur+=TexLodI(Col2, tex2, lod).w*I.material.z;
                  if(MATERIALS>=4)height_cur+=TexLodI(Col3, tex3, lod).w*I.material.w;

                  if(height_cur>=ray_cur) // if still below, then have to go back more, lerp between this position and prev pos
                  {
                     // prev pos: I.tex-tex_step (BUT I.tex before adjustment), height_prev-ray_prev
                     // next pos: I.tex                                       , height_cur -ray_cur
                     tex_step*=1-frac; // we've just travelled "tex_step*frac", so to go to the prev point, we need what's left, "tex_step*(1-frac)"
                  }else // we went back too far, go forward, lerp between next pos and this position
                  {
                     // prev pos: I.tex                              , height_cur -ray_cur
                     // next pos: I.tex (BUT I.tex before adjustment), height_next-ray
                     hp=hn;
                     tex_step*=-frac; // we've just travelled "tex_step*frac", so to go to the next point, we need the same way but other direction, "tex_step*-frac"
                  }
                  hn=height_cur-ray_cur;
                  frac=Sat(hn/(hn-hp));

                  //I.tex-=tex_step*frac;
                  offset=tex_step*frac;
                                    tex0-=offset;
                                    tex1-=offset;
                  if(MATERIALS>=3)tex2-=offset;
                  if(MATERIALS>=4)tex3-=offset;
               }
            }
         #else // linear + binary search (slower because requires 3 tex reads in binary to get the same results as with only 0-1 tex reads in interval)
            this needs to be updated for 4 materials

            // linear search
            LOOP for(Int i=0; ; i++)
            {
               ray  -=stp;
               I.tex+=tex_step;
               if(i>=steps || TexLodI(Col, I.tex, lod).w>=ray)break;
            }

            // binary search
            {
               Half ray_prev=ray+stp,
                    l=0, r=1, m=0.5;
               UNROLL for(Int i=0; i<RELIEF_STEPS_BINARY; i++)
               {
                  Half height=TexLodI(Col, I.tex-tex_step*m, lod).w;
                  if(  height>Lerp(ray, ray_prev, m))l=m;else r=m;
                  m=Avg(l, r);
               }
               I.tex-=tex_step*m;
            }
         #endif
         }
      }
      #endif // Relief
   }

   // detail texture
   VecH det0, det1, det2, det3;
   if(DETAIL) // #MaterialTextureChannelOrder
   {
                      det0=Tex(Det , tex0*MultiMaterial0DetScale()).xyz*MultiMaterial0DetMul()+MultiMaterial0DetAdd();
                      det1=Tex(Det1, tex1*MultiMaterial1DetScale()).xyz*MultiMaterial1DetMul()+MultiMaterial1DetAdd();
      if(MATERIALS>=3)det2=Tex(Det2, tex2*MultiMaterial2DetScale()).xyz*MultiMaterial2DetMul()+MultiMaterial2DetAdd();
      if(MATERIALS>=4)det3=Tex(Det3, tex3*MultiMaterial3DetScale()).xyz*MultiMaterial3DetMul()+MultiMaterial3DetAdd();
   }

   // macro texture
   Half mac_blend; if(MACRO)mac_blend=LerpRS(MacroFrom, MacroTo, I.pos.z)*MacroMax;

   // combine color + detail + macro !! do this first because it may modify 'I.material' which affects secondary texture !!
   VecH tex;
   if(MTRL_BLEND)
   {
      VecH4 col0, col1, col2, col3;
                       col0=Tex(Col , tex0); col0.rgb*=MultiMaterial0Color3(); I.material.x=MultiMaterialWeight(I.material.x, col0.a);
                       col1=Tex(Col1, tex1); col1.rgb*=MultiMaterial1Color3(); I.material.y=MultiMaterialWeight(I.material.y, col1.a); if(MATERIALS==2)I.material.xy  /=I.material.x+I.material.y;
      if(MATERIALS>=3){col2=Tex(Col2, tex2); col2.rgb*=MultiMaterial2Color3(); I.material.z=MultiMaterialWeight(I.material.z, col2.a); if(MATERIALS==3)I.material.xyz /=I.material.x+I.material.y+I.material.z;}
      if(MATERIALS>=4){col3=Tex(Col3, tex3); col3.rgb*=MultiMaterial3Color3(); I.material.w=MultiMaterialWeight(I.material.w, col3.a); if(MATERIALS==4)I.material.xyzw/=I.material.x+I.material.y+I.material.z+I.material.w;}

                      {if(DETAIL)col0.rgb+=det0.z; if(MACRO)col0.rgb=Lerp(col0.rgb, Tex(Mac , tex0*MacroScale).rgb, MultiMaterial0Macro()*mac_blend); tex =I.material.x*col0.rgb;}
                      {if(DETAIL)col1.rgb+=det1.z; if(MACRO)col1.rgb=Lerp(col1.rgb, Tex(Mac1, tex1*MacroScale).rgb, MultiMaterial1Macro()*mac_blend); tex+=I.material.y*col1.rgb;}
      if(MATERIALS>=3){if(DETAIL)col2.rgb+=det2.z; if(MACRO)col2.rgb=Lerp(col2.rgb, Tex(Mac2, tex2*MacroScale).rgb, MultiMaterial2Macro()*mac_blend); tex+=I.material.z*col2.rgb;}
      if(MATERIALS>=4){if(DETAIL)col3.rgb+=det3.z; if(MACRO)col3.rgb=Lerp(col3.rgb, Tex(Mac3, tex3*MacroScale).rgb, MultiMaterial3Macro()*mac_blend); tex+=I.material.w*col3.rgb;}
   }else
   {
                      {VecH col0=Tex(Col , tex0).rgb; if(DETAIL)col0+=det0.z; if(MACRO)col0=Lerp(col0, Tex(Mac , tex0*MacroScale).rgb, MultiMaterial0Macro()*mac_blend); tex =I.material.x*col0*MultiMaterial0Color3();}
                      {VecH col1=Tex(Col1, tex1).rgb; if(DETAIL)col1+=det1.z; if(MACRO)col1=Lerp(col1, Tex(Mac1, tex1*MacroScale).rgb, MultiMaterial1Macro()*mac_blend); tex+=I.material.y*col1*MultiMaterial1Color3();}
      if(MATERIALS>=3){VecH col2=Tex(Col2, tex2).rgb; if(DETAIL)col2+=det2.z; if(MACRO)col2=Lerp(col2, Tex(Mac2, tex2*MacroScale).rgb, MultiMaterial2Macro()*mac_blend); tex+=I.material.z*col2*MultiMaterial2Color3();}
      if(MATERIALS>=4){VecH col3=Tex(Col3, tex3).rgb; if(DETAIL)col3+=det3.z; if(MACRO)col3=Lerp(col3, Tex(Mac3, tex3*MacroScale).rgb, MultiMaterial3Macro()*mac_blend); tex+=I.material.w*col3*MultiMaterial3Color3();}
   }
#if COLORS
   col*=tex;
#else
   col =tex;
#endif

   // normal, specular, glow !! do this second after modifying 'I.material' !!
   if(LAYOUT<=1)
   {
      nrm=Normalize(I.Nrm());

                VecH2 tex =I.material.x*MultiMaterial0NormalAdd().zw // #MaterialTextureChannelOrder
                          +I.material.y*MultiMaterial1NormalAdd().zw;
      if(MATERIALS>=3)tex+=I.material.z*MultiMaterial2NormalAdd().zw;
      if(MATERIALS>=4)tex+=I.material.w*MultiMaterial3NormalAdd().zw;

      specular=tex.x;
      glow    =tex.y;

      // reflection
      if(REFLECT)
      {
         Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
                         col+=TexCube(Rfl , rfl).rgb*(MultiMaterial0Reflect()*I.material.x);
                         col+=TexCube(Rfl1, rfl).rgb*(MultiMaterial1Reflect()*I.material.y);
         if(MATERIALS>=3)col+=TexCube(Rfl2, rfl).rgb*(MultiMaterial2Reflect()*I.material.z);
         if(MATERIALS>=4)col+=TexCube(Rfl3, rfl).rgb*(MultiMaterial3Reflect()*I.material.w);
      }
   }else
   {
      Half tex_spec[4];

      #if BUMP_MODE==SBUMP_FLAT
      {
         nrm=Normalize(I.Nrm());

                          VecH2 tex; // #MaterialTextureChannelOrder
                         {VecH2 nrm0; nrm0=Tex(Nrm , tex0).zw; if(REFLECT)tex_spec[0]=nrm0.x; nrm0=nrm0*MultiMaterial0NormalMul().zw+MultiMaterial0NormalAdd().zw; tex =I.material.x*nrm0;}
                         {VecH2 nrm1; nrm1=Tex(Nrm1, tex1).zw; if(REFLECT)tex_spec[1]=nrm1.x; nrm1=nrm1*MultiMaterial1NormalMul().zw+MultiMaterial1NormalAdd().zw; tex+=I.material.y*nrm1;}
         if(MATERIALS>=3){VecH2 nrm2; nrm2=Tex(Nrm2, tex2).zw; if(REFLECT)tex_spec[2]=nrm2.x; nrm2=nrm2*MultiMaterial2NormalMul().zw+MultiMaterial2NormalAdd().zw; tex+=I.material.z*nrm2;}
         if(MATERIALS>=4){VecH2 nrm3; nrm3=Tex(Nrm3, tex3).zw; if(REFLECT)tex_spec[3]=nrm3.x; nrm3=nrm3*MultiMaterial3NormalMul().zw+MultiMaterial3NormalAdd().zw; tex+=I.material.w*nrm3;}

         specular=tex.x;
         glow    =tex.y;
      }
      #elif BUMP_MODE>SBUMP_FLAT // normal mapping
      {
                          VecH4 tex; // #MaterialTextureChannelOrder
                         {VecH4 nrm0=Tex(Nrm , tex0); if(REFLECT)tex_spec[0]=nrm0.z; nrm0=nrm0*MultiMaterial0NormalMul()+MultiMaterial0NormalAdd(); if(DETAIL)nrm0.xy+=det0.xy; tex =I.material.x*nrm0;}
                         {VecH4 nrm1=Tex(Nrm1, tex1); if(REFLECT)tex_spec[1]=nrm1.z; nrm1=nrm1*MultiMaterial1NormalMul()+MultiMaterial1NormalAdd(); if(DETAIL)nrm1.xy+=det1.xy; tex+=I.material.y*nrm1;}
         if(MATERIALS>=3){VecH4 nrm2=Tex(Nrm2, tex2); if(REFLECT)tex_spec[2]=nrm2.z; nrm2=nrm2*MultiMaterial2NormalMul()+MultiMaterial2NormalAdd(); if(DETAIL)nrm2.xy+=det2.xy; tex+=I.material.z*nrm2;}
         if(MATERIALS>=4){VecH4 nrm3=Tex(Nrm3, tex3); if(REFLECT)tex_spec[3]=nrm3.z; nrm3=nrm3*MultiMaterial3NormalMul()+MultiMaterial3NormalAdd(); if(DETAIL)nrm3.xy+=det3.xy; tex+=I.material.w*nrm3;}

         nrm=VecH(tex.xy, CalcZ(tex.xy));
         nrm=Normalize(Transform(nrm, I.mtrx));

         specular=tex.z;
         glow    =tex.w;
      }
      #endif

      // reflection
      if(REFLECT)
      {
         Vec rfl=Transform3(reflect(I.pos, nrm), CamMatrix); // #ShaderHalf
                         col+=TexCube(Rfl , rfl).rgb*(MultiMaterial0Reflect()*I.material.x*tex_spec[0]);
                         col+=TexCube(Rfl1, rfl).rgb*(MultiMaterial1Reflect()*I.material.y*tex_spec[1]);
         if(MATERIALS>=3)col+=TexCube(Rfl2, rfl).rgb*(MultiMaterial2Reflect()*I.material.z*tex_spec[2]);
         if(MATERIALS>=4)col+=TexCube(Rfl3, rfl).rgb*(MultiMaterial3Reflect()*I.material.w*tex_spec[3]);
      }
   }
#endif

   col+=Highlight.rgb;

#if FX!=FX_GRASS && FX!=FX_LEAF && FX!=FX_LEAFS
   BackFlip(nrm, front);
#endif

   output.color   (col         );
   output.glow    (glow        );
   output.normal  (nrm         );
   output.specular(specular    );
   output.velocity(I.vel, I.pos);
}
/******************************************************************************/
// HULL / DOMAIN
/******************************************************************************/
#if TESSELATE
HSData HSConstant(InputPatch<VS_PS,3> I) {return GetHSData(I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm());}
[maxtessfactor(5.0)]
[domain("tri")]
[partitioning("fractional_odd")] // use 'odd' because it supports range from 1.0 ('even' supports range from 2.0)
[outputtopology("triangle_cw")]
[patchconstantfunc("HSConstant")]
[outputcontrolpoints(3)]
VS_PS HS
(
   InputPatch<VS_PS,3> I, UInt cp_id:SV_OutputControlPointID
)
{
   VS_PS O;
   O.pos=I[cp_id].pos;
   O.vel=I[cp_id].vel;

#if LAYOUT || DETAIL
   O.tex=I[cp_id].tex;
#endif

#if   BUMP_MODE> SBUMP_FLAT
   O.mtrx=I[cp_id].mtrx;
#elif BUMP_MODE==SBUMP_FLAT
   O.nrm =I[cp_id].nrm ;
#endif

#if MATERIALS>1
   O.material=I[cp_id].material;
#endif

#if COLORS
   O.col=I[cp_id].col;
#endif

#if FAST_TPOS
   O._tpos=I[cp_id]._tpos;
#endif

#if FX==FX_GRASS
   O.fade_out=I[cp_id].fade_out;
#endif

   return O;
}
/******************************************************************************/
[domain("tri")]
void DS
(
   HSData hs_data, const OutputPatch<VS_PS,3> I, Vec B:SV_DomainLocation,

   out VS_PS O,
   out Vec4  O_vtx:POSITION
)
{
#if   BUMP_MODE> SBUMP_FLAT
   O.mtrx[0]=I[0].mtrx[0]*B.z + I[1].mtrx[0]*B.x + I[2].mtrx[0]*B.y;
   O.mtrx[1]=I[0].mtrx[1]*B.z + I[1].mtrx[1]*B.x + I[2].mtrx[1]*B.y;
   SetDSPosNrm(O.pos, O.mtrx[2], I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#elif BUMP_MODE==SBUMP_FLAT
   SetDSPosNrm(O.pos, O.nrm    , I[0].pos, I[1].pos, I[2].pos, I[0].Nrm(), I[1].Nrm(), I[2].Nrm(), B, hs_data, false, 0);
#endif

   O.vel=I[0].vel*B.z + I[1].vel*B.x + I[2].vel*B.y;

#if LAYOUT || DETAIL
   O.tex=I[0].tex*B.z + I[1].tex*B.x + I[2].tex*B.y;
#endif

#if MATERIALS>1
   O.material=I[0].material*B.z + I[1].material*B.x + I[2].material*B.y;
#endif

#if COLORS
   O.col=I[0].col*B.z + I[1].col*B.x + I[2].col*B.y;
#endif

#if FAST_TPOS
   O._tpos=I[0]._tpos*B.z + I[1]._tpos*B.x + I[2]._tpos*B.y;
#endif

#if FX==FX_GRASS
   O.fade_out=I[0].fade_out*B.z + I[1].fade_out*B.x + I[2].fade_out*B.y;
#endif

   O_vtx=Project(O.pos);
}
#endif
/******************************************************************************/
