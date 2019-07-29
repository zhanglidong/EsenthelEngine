/******************************************************************************/
#include "!Header.h"
/******************************************************************************/
VecH4 TexCubicFast(Vec2 inTex)
{
#if 0 // original
   Vec2  tex =inTex*ImgSize.zw-0.5,
         texi=Floor(tex);
   VecH2 texf=tex-texi;
         texi-=0.5; texi*=ImgSize.xy;

   VecH4 c00=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 0)), c10=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 0)), c20=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 0)), c30=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 0)),
         c01=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 1)), c11=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 1)), c21=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 1)), c31=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 1)),
         c02=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 2)), c12=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 2)), c22=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 2)), c32=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 2)),
         c03=TexPoint(Img, texi+ImgSize.xy*Vec2(0, 3)), c13=TexPoint(Img, texi+ImgSize.xy*Vec2(1, 3)), c23=TexPoint(Img, texi+ImgSize.xy*Vec2(2, 3)), c33=TexPoint(Img, texi+ImgSize.xy*Vec2(3, 3));

   VecH4 c0=Lerp4(c00, c10, c20, c30, texf.x),
         c1=Lerp4(c01, c11, c21, c31, texf.x),
         c2=Lerp4(c02, c12, c22, c32, texf.x),
         c3=Lerp4(c03, c13, c23, c33, texf.x);

   return Lerp4(c0, c1, c2, c3, texf.y);
#else // optimized
   inTex*=ImgSize.zw;
   Vec2 tc=Floor(inTex-0.5)+0.5;
   VecH2 f=inTex-tc, f2=f*f, f3=f2*f,
        w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
   #if 0
        w2=-1.5*f3+2*f2+0.5*f, w3=0.5*(f3-f2);
   #else
        w3=0.5*(f3-f2), w2=1.0-w0-w1-w3;
   #endif

   tc*=ImgSize.xy;
   Vec2 tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
#if 0 // 16 tex reads
   Vec2 tc2=tc+ImgSize.xy;

 /*Flt w[4][4]={(w0.x*w0.y), (w1.x*w0.y), (w2.x*w0.y), (w3.x*w0.y),
                (w0.x*w1.y), (w1.x*w1.y), (w2.x*w1.y), (w3.x*w1.y),
                (w0.x*w2.y), (w1.x*w2.y), (w2.x*w2.y), (w3.x*w2.y),
                (w0.x*w3.y), (w1.x*w3.y), (w2.x*w3.y), (w3.x*w3.y)};*/

   return TexPoint(Img, Vec2(tc0.x, tc0.y))*(w0.x*w0.y)
         +TexPoint(Img, Vec2(tc .x, tc0.y))*(w1.x*w0.y)
         +TexPoint(Img, Vec2(tc0.x, tc .y))*(w0.x*w1.y)
         +TexPoint(Img, Vec2(tc .x, tc .y))*(w1.x*w1.y)

         +TexPoint(Img, Vec2(tc2.x, tc0.y))*(w2.x*w0.y)
         +TexPoint(Img, Vec2(tc3.x, tc0.y))*(w3.x*w0.y)
         +TexPoint(Img, Vec2(tc2.x, tc .y))*(w2.x*w1.y)
         +TexPoint(Img, Vec2(tc3.x, tc .y))*(w3.x*w1.y)
 
         +TexPoint(Img, Vec2(tc0.x, tc2.y))*(w0.x*w2.y)
         +TexPoint(Img, Vec2(tc .x, tc2.y))*(w1.x*w2.y)
         +TexPoint(Img, Vec2(tc0.x, tc3.y))*(w0.x*w3.y)
         +TexPoint(Img, Vec2(tc .x, tc3.y))*(w1.x*w3.y)

         +TexPoint(Img, Vec2(tc2.x, tc2.y))*(w2.x*w2.y)
         +TexPoint(Img, Vec2(tc3.x, tc2.y))*(w3.x*w2.y)
         +TexPoint(Img, Vec2(tc2.x, tc3.y))*(w2.x*w3.y)
         +TexPoint(Img, Vec2(tc3.x, tc3.y))*(w3.x*w3.y);
#else // 5 tex reads, corners are ignored because they're insignificant
   VecH2 w12=w1+w2; Vec2 p=tc+(w2/w12)*ImgSize.xy;
   Half  wu=w12.x*w0.y, wd=w12.x*w3.y, wl=w12.y*w0.x, wr=w12.y*w3.x, wc=w12.x*w12.y;
   return(TexLod(Img, Vec2(  p.x, tc0.y))*wu // sample upper edge (2 texels), both weights are negative
         +TexLod(Img, Vec2(  p.x, tc3.y))*wd // sample lower edge (2 texels), both weights are negative
         +TexLod(Img, Vec2(tc0.x,   p.y))*wl // sample left  edge (2 texels), both weights are negative
         +TexLod(Img, Vec2(tc3.x,   p.y))*wr // sample right edge (2 texels), both weights are negative
         +TexLod(Img, Vec2(  p.x,   p.y))*wc // sample center     (4 texels), all  weights are positive
         )/(wu+wd+wl+wr+wc);
#endif
#endif
}
VecH TexCubicFastRGB(Vec2 inTex) // ignores alpha channel
{
   inTex*=ImgSize.zw;
   Vec2 tc=Floor(inTex-0.5)+0.5;
   VecH2 f=inTex-tc, f2=f*f, f3=f2*f,
        w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
        w3=0.5*(f3-f2), w2=1.0-w0-w1-w3;
   tc*=ImgSize.xy;
   Vec2  tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
   VecH2 w12=w1+w2; Vec2 p=tc+(w2/w12)*ImgSize.xy;
   Half  wu =w12.x*w0.y, wd=w12.x*w3.y, wl=w12.y*w0.x, wr=w12.y*w3.x, wc=w12.x*w12.y;
   return(TexLod(Img, Vec2(  p.x, tc0.y)).rgb*wu
         +TexLod(Img, Vec2(  p.x, tc3.y)).rgb*wd
         +TexLod(Img, Vec2(tc0.x,   p.y)).rgb*wl
         +TexLod(Img, Vec2(tc3.x,   p.y)).rgb*wr
         +TexLod(Img, Vec2(  p.x,   p.y)).rgb*wc)/(wu+wd+wl+wr+wc);
}
/******************************************************************************/
#define CUBIC_SAMPLES     3
#define CUBIC_RANGE       2
#define CUBIC_SHARPNESS   (2/2.5)
#define CUBIC_QUALITY     2 // 0..2, 0=3.748 fps, 1=3.242 fps, 2=3.075 fps (however when using CUBIC_SKIP_SAMPLE it's now much faster)
#define CUBIC_SKIP_SAMPLE (1 && CUBIC_QUALITY==2) // because the actual range is 2.5 then it means we need to process 5x5 samples (and not 6x6), this optimization can work only if actual range <= 2.5, also we can enable this only for CUBIC_QUALITY==2 because only this mode operates on single 1x1 pixels and not 2x2 blocks)

inline Flt Cubic(Flt x, Flt blur, Flt sharpen)
{
   Flt x2=x*x,
       x3=x*x*x;
   return (x<=1) ? ((12-9*blur-6*sharpen)/6*x3 + (-18+12*blur+6*sharpen)/6*x2 +                             (6-2*blur         )/6)
                 : ((-blur-6*sharpen    )/6*x3 + (6*blur+30*sharpen    )/6*x2 + (-12*blur-48*sharpen)/6*x + (8*blur+24*sharpen)/6);
}
inline Half Cubic(Half x, Half blur, Half sharpen)
{
   Half x2=x*x,
        x3=x*x*x;
   return (x<=1) ? ((12-9*blur-6*sharpen)/6*x3 + (-18+12*blur+6*sharpen)/6*x2 +                             (6-2*blur         )/6)
                 : ((-blur-6*sharpen    )/6*x3 + (6*blur+30*sharpen    )/6*x2 + (-12*blur-48*sharpen)/6*x + (8*blur+24*sharpen)/6);
}
Flt  CubicMed(Flt  x) {return Cubic(x, 0.0, 0.375);}
Half CubicMed(Half x) {return Cubic(x, 0.0, 0.375);}

/*VecH4 TexLerp(Flt x0, Flt x1, Flt y, Flt l, Flt r)
{
   Flt w=l+r;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, Vec2((x0*l + x1*r)/w, y))*Half(w);
}
VecH TexLerpRGB(Flt x0, Flt x1, Flt y, Flt l, Flt r) // ignores alpha channel
{
   Flt w=l+r;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, Vec2((x0*l + x1*r)/w, y)).rgb*Half(w);
}

VecH4 TexLerp(Vec2 t0, Vec2 t1, Flt lu, Flt ru, Flt lb, Flt rb)
{
#if 0 // slower
   return TexPoint(Img, Vec2(t0.x, t0.y))*lu
         +TexPoint(Img, Vec2(t1.x, t0.y))*ru
         +TexPoint(Img, Vec2(t0.x, t1.y))*lb
         +TexPoint(Img, Vec2(t1.x, t1.y))*rb;
#else
   Vec2 t=Vec2(t0.x, t0.y)*lu
         +Vec2(t1.x, t0.y)*ru
         +Vec2(t0.x, t1.y)*lb
         +Vec2(t1.x, t1.y)*rb;
   Flt w=lu+ru+lb+rb;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, t/w)*Half(w);
#endif
}
VecH TexLerpRGB(Vec2 t0, Vec2 t1, Flt lu, Flt ru, Flt lb, Flt rb) // ignores alpha channel
{
   Vec2 t=Vec2(t0.x, t0.y)*lu
         +Vec2(t1.x, t0.y)*ru
         +Vec2(t0.x, t1.y)*lb
         +Vec2(t1.x, t1.y)*rb;
   Flt w=lu+ru+lb+rb;
   // keep 'Tex' in case we need LOD's (for example stretching in 1 dimension but shrinking in another)
   return Tex(Img, t/w).rgb*Half(w);
}*/

VecH4 TexCubic(Vec2 inTex)
{
   Vec2  pixel =inTex*ImgSize.zw-0.5,
         pixeli=Floor(pixel),
         offset       [CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];
   VecH2 offset_weight[CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];

#if CUBIC_SKIP_SAMPLE
   pixeli+=(pixel-pixeli>=CUBIC_RANGE/CUBIC_SHARPNESS-(CUBIC_SAMPLES-1)); // if the left/top coordinate is completely out of range, then process the next pixel (the >= returns Vec2, so it modifies X and Y independently)
#endif

   UNROLL for(int i=0; i<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; i++)
   {
      offset       [i]=     pixeli+(i-(CUBIC_SAMPLES-1));
      offset_weight[i]=Sqr((pixel -offset[i])*CUBIC_SHARPNESS);
      offset       [i]=           (offset[i]+0.5)*ImgSize.xy;
   }
   VecH4 color =0;
   Half  weight=0;
#if CUBIC_QUALITY>=2
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; x++)
   {
      Half w=offset_weight[x].x+offset_weight[y].y; if(w<Sqr(CUBIC_RANGE))
      {
         w=CubicMed(Sqrt(w));
         color +=w*TexPoint(Img, Vec2(offset[x].x, offset[y].y)); // don't use "color+=w*Img.Load(VecI(offset[x].x without scale, offset[y].y without scale, 0)); because it is slower and doesn't support wrap/clamp
         weight+=w;
      }
   }
#else
   Flt weights[CUBIC_SAMPLES*2][CUBIC_SAMPLES*2]; // [y][x]
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x++)
   {
      Flt w=offset_weight[x].x+offset_weight[y].y;
          w=(w<Sqr(CUBIC_RANGE) ? CubicMed(Sqrt(w)) : 0);
      weights[y][x]=w;
      weight      +=w;
   }
   #if CUBIC_QUALITY>=1 // medium quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++ )
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerp(offset[x].x, offset[x+1].x, offset[y].y, weights[y][x], weights[y][x+1]);
   #else // low quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y+=2)
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerp(Vec2(offset[x].x, offset[y].y), Vec2(offset[x+1].x, offset[y+1].y), weights[y][x], weights[y][x+1], weights[y+1][x], weights[y+1][x+1]);
   #endif
#endif
   return color/weight;
}
VecH TexCubicRGB(Vec2 inTex) // ignores alpha channel
{
   Vec2  pixel =inTex*ImgSize.zw-0.5,
         pixeli=Floor(pixel),
         offset       [CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];
   VecH2 offset_weight[CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE];

#if CUBIC_SKIP_SAMPLE
   pixeli+=(pixel-pixeli>=CUBIC_RANGE/CUBIC_SHARPNESS-(CUBIC_SAMPLES-1)); // if the left/top coordinate is completely out of range, then process the next pixel (the >= returns Vec2, so it modifies X and Y independently)
#endif

   UNROLL for(int i=0; i<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; i++)
   {
      offset       [i]=     pixeli+(i-(CUBIC_SAMPLES-1));
      offset_weight[i]=Sqr((pixel -offset[i])*CUBIC_SHARPNESS);
      offset       [i]=           (offset[i]+0.5)*ImgSize.xy;
   }
   VecH color =0;
   Half weight=0;
#if CUBIC_QUALITY>=2 // high quality
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2-CUBIC_SKIP_SAMPLE; x++)
   {
      Half w=offset_weight[x].x+offset_weight[y].y; if(w<Sqr(CUBIC_RANGE))
      {
         w=CubicMed(Sqrt(w));
         color +=w*TexPoint(Img, Vec2(offset[x].x, offset[y].y)).rgb; // don't use "color+=w*Img.Load(VecI(offset[x].x without scale, offset[y].y without scale, 0)).rgb; because it is slower and doesn't support wrap/clamp
         weight+=w;
      }
   }
#else
   Flt weights[CUBIC_SAMPLES*2][CUBIC_SAMPLES*2]; // [y][x]
   UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++)
   UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x++)
   {
      Flt w=offset_weight[x].x+offset_weight[y].y;
          w=(w<Sqr(CUBIC_RANGE) ? CubicMed(Sqrt(w)) : 0);
      weights[y][x]=w;
      weight      +=w;
   }
   #if CUBIC_QUALITY>=1 // medium quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y++ )
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerpRGB(offset[x].x, offset[x+1].x, offset[y].y, weights[y][x], weights[y][x+1]);
   #else // low quality
      UNROLL for(int y=0; y<CUBIC_SAMPLES*2; y+=2)
      UNROLL for(int x=0; x<CUBIC_SAMPLES*2; x+=2)color+=TexLerpRGB(Vec2(offset[x].x, offset[y].y), Vec2(offset[x+1].x, offset[y+1].y), weights[y][x], weights[y][x+1], weights[y+1][x], weights[y+1][x+1]);
   #endif
#endif
   return color/weight;
}
/******************************************************************************/
VecH4 DrawTexCubicFast_PS(NOPERSP Vec2 inTex:TEXCOORD,
                          NOPERSP PIXEL              ):TARGET
{
   VecH4 col=TexCubicFast(inTex);
#if COLORS
   col=col*Color[0]+Color[1];
#endif
#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
VecH4 DrawTexCubicFastRGB_PS(NOPERSP Vec2 inTex:TEXCOORD,
                             NOPERSP PIXEL              ):TARGET
{
   VecH4 col=VecH4(TexCubicFastRGB(inTex), 1);
#if COLORS
   col=col*Color[0]+Color[1];
#endif
#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
VecH4 DrawTexCubic_PS(NOPERSP Vec2 inTex:TEXCOORD,
                      NOPERSP PIXEL              ):TARGET
{
   VecH4 col=TexCubic(inTex);
#if COLORS
   col=col*Color[0]+Color[1];
#endif
#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
VecH4 DrawTexCubicRGB_PS(NOPERSP Vec2 inTex:TEXCOORD,
                         NOPERSP PIXEL              ):TARGET
{
   VecH4 col=VecH4(TexCubicRGB(inTex), 1);
#if COLORS
   col=col*Color[0]+Color[1];
#endif
#if DITHER
   ApplyDither(col.rgb, pixel.xy);
#endif
   return col;
}
/******************************************************************************/
