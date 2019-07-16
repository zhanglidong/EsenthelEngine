/******************************************************************************/
#include "!Header.h"
#include "Sky.h"
/******************************************************************************/
#include "!Set HP.h"
BUFFER(VolLight)
   VecH VolMax=Vec(1, 1, 1);
BUFFER_END
#include "!Set LP.h"
/******************************************************************************/
VecH4 VolDir_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                NOPERSP Vec2 inPosXY:TEXCOORD1,
                NOPERSP PIXEL                 ,
                uniform Int  num              ,
                uniform Bool cloud            ):TARGET
{
   Vec obj   =GetPosLinear(inTex, inPosXY); // use linear filtering because we may be drawing to a smaller RT
   Flt power =0,
       length=Length(obj);
   if( length>ShdRange)
   {
      obj  *=ShdRange/length;
      length=ShdRange;
   }

   Vec2 jitter_value=ShadowJitter(pixel.xy);

   Int  steps=80;
   LOOP for(Int i=0; i<steps; i++)
   {
      Vec pos=ShadowDirTransform(obj*(Flt(i+1)/(steps+1)), num);
      if(cloud)power+=CompareDepth(pos, jitter_value, true)*CompareDepth2(pos);
      else     power+=CompareDepth(pos, jitter_value, true);
   }

   power =Pow(power /steps   , Light_dir.vol_exponent_steam.y);
   power*=Pow(length/ShdRange, Light_dir.vol_exponent_steam.y*(1-Light_dir.vol_exponent_steam.z));
   power*=Light_dir.vol_exponent_steam.x;
   return VecH4(Light_dir.color.rgb*power, 0);
}
/******************************************************************************/
VecH4 VolPoint_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                  NOPERSP Vec2 inPosXY:TEXCOORD1,
                  NOPERSP PIXEL                 ):TARGET
{
   Vec obj   =GetPosLinear(inTex, inPosXY); // use linear filtering because we may be drawing to a smaller RT
   Flt power =0,
       length=Length(obj);
   if( length>Viewport.range)
   {
      obj  *=Viewport.range/length;
      length=Viewport.range;
   }
   Vec from=ShdMatrix[3],
       to  =Transform(obj, ShdMatrix);

   Vec2 jitter_value=ShadowJitter(pixel.xy);

   Int steps=48;

   LOOP for(Int i=0; i<steps; i++)
   {
      // TODO: optimize
      Vec pos=Lerp(from, to, Flt(i)/Flt(steps)); Flt inv_dist2=1/Length2(pos);
      power+=ShadowPointValue(obj*(Flt(i)/steps), jitter_value, true)*LightPointDist(inv_dist2);
   }
   return VecH4(Light_point.color.rgb*Min(Light_point.vol_max, Light_point.vol*power*(length/steps)), 0);
}
/******************************************************************************/
VecH4 VolLinear_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                   NOPERSP Vec2 inPosXY:TEXCOORD1,
                   NOPERSP PIXEL                 ):TARGET
{
   Vec obj   =GetPosLinear(inTex, inPosXY); // use linear filtering because we may be drawing to a smaller RT
   Flt power =0,
       length=Length(obj);
   if( length>Viewport.range)
   {
      obj  *=Viewport.range/length;
      length=Viewport.range;
   }
   Vec from =ShdMatrix[3],
       to   =Transform(obj, ShdMatrix);
   Int steps=48;

   Vec2 jitter_value=ShadowJitter(pixel.xy);

   LOOP for(Int i=0; i<steps; i++)
   {
      // TODO: optimize
      Vec pos=Lerp(from, to, Flt(i)/Flt(steps));
      power+=ShadowPointValue(obj*(Flt(i)/steps), jitter_value, true)*LightLinearDist(Length(pos));
   }
   return VecH4(Light_linear.color.rgb*Min(Light_linear.vol_max, Light_linear.vol*power*(length/steps)), 0);
}
/******************************************************************************/
VecH4 VolCone_PS(NOPERSP Vec2 inTex  :TEXCOORD0,
                 NOPERSP Vec2 inPosXY:TEXCOORD1,
                 NOPERSP PIXEL                 ):TARGET
{
   Vec obj   =GetPosLinear(inTex, inPosXY), // use linear filtering because we may be drawing to a smaller RT
       scale =Vec(Light_cone.scale, Light_cone.scale, 1);
   Flt power =0,
       length=Length(obj);
   if( length>Viewport.range)
   {
      obj  *=Viewport.range/length;
      length=Viewport.range;
   }
   Vec from =ShdMatrix[3],
       to   =Transform(obj, ShdMatrix);
   Int steps=48;

   Vec2 jitter_value=ShadowJitter(pixel.xy);

   LOOP for(Int i=0; i<steps; i++)
   {
      // TODO: optimize
      Vec pos=Lerp(from, to, Flt(i)/Flt(steps));
      Flt cur=Max(Abs(pos));
      if( pos.z>=cur)
      {
         power+=ShadowConeValue(obj*(Flt(i)/steps), jitter_value, true)*LightConeDist(Length(pos*scale))*LightConeAngle(pos.xy/pos.z);
      }
   }
   return VecH4(Light_cone.color.rgb*Min(Light_cone.vol_max, Light_cone.vol*power*(length/steps)), 0);
}
/******************************************************************************/
VecH4 Volumetric_PS(NOPERSP Vec2 inTex:TEXCOORD,
                    uniform Bool add           ,
                    uniform Int  samples=6     ):TARGET
{
   VecH vol=TexLod(Img, inTex).rgb; // use linear filtering because 'Img' may be smaller

   UNROLL for(Int i=0; i<samples; i++)
   {
      Vec2 t;
      if(samples== 4)t=ImgSize.xy*BlendOfs4 [i]+inTex;
    //if(samples== 5)t=ImgSize.xy*BlendOfs5 [i]+inTex;
      if(samples== 6)t=ImgSize.xy*BlendOfs6 [i]+inTex;
      if(samples== 8)t=ImgSize.xy*BlendOfs8 [i]+inTex;
    //if(samples== 9)t=ImgSize.xy*BlendOfs9 [i]+inTex;
      if(samples==12)t=ImgSize.xy*BlendOfs12[i]+inTex;
    //if(samples==13)t=ImgSize.xy*BlendOfs13[i]+inTex;

      vol+=TexLod(Img, t).rgb; // use linear filtering because 'Img' may be smaller and texcoords are not rounded
   }
   vol/=samples+1;
   vol =Min(vol, VolMax);

   if(add)return VecH4(vol, 0);                                       // alpha blending : ALPHA_ADD
   else   {Half max=Max(vol); return VecH4(vol/(HALF_MIN+max), max);} // alpha blending : ALPHA_BLEND_DEC
}
/******************************************************************************/
// TECHNIQUES
/******************************************************************************/
TECHNIQUE(VolDir1, DrawPosXY_VS(), VolDir_PS(1, false));   TECHNIQUE(VolDir1C, DrawPosXY_VS(), VolDir_PS(1, true));
TECHNIQUE(VolDir2, DrawPosXY_VS(), VolDir_PS(2, false));   TECHNIQUE(VolDir2C, DrawPosXY_VS(), VolDir_PS(2, true));
TECHNIQUE(VolDir3, DrawPosXY_VS(), VolDir_PS(3, false));   TECHNIQUE(VolDir3C, DrawPosXY_VS(), VolDir_PS(3, true));
TECHNIQUE(VolDir4, DrawPosXY_VS(), VolDir_PS(4, false));   TECHNIQUE(VolDir4C, DrawPosXY_VS(), VolDir_PS(4, true));
TECHNIQUE(VolDir5, DrawPosXY_VS(), VolDir_PS(5, false));   TECHNIQUE(VolDir5C, DrawPosXY_VS(), VolDir_PS(5, true));
TECHNIQUE(VolDir6, DrawPosXY_VS(), VolDir_PS(6, false));   TECHNIQUE(VolDir6C, DrawPosXY_VS(), VolDir_PS(6, true));

TECHNIQUE(VolPoint , DrawPosXY_VS(), VolPoint_PS ());
TECHNIQUE(VolLinear, DrawPosXY_VS(), VolLinear_PS());
TECHNIQUE(VolCone  , DrawPosXY_VS(), VolCone_PS  ());

TECHNIQUE(Volumetric , Draw_VS(), Volumetric_PS(false));
TECHNIQUE(VolumetricA, Draw_VS(), Volumetric_PS(true ));
/******************************************************************************/
