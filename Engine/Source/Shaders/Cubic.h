/******************************************************************************/
struct CubicFastSampler
{
/*#if 0 // original
   Vec2  tex =uv*ImgSize.zw-0.5,
         texi=Floor(tex);
   VecH2 texf=tex-texi;
         texi-=0.5; texi*=ImgSize.xy;

   VecH4 c00=TexPoint(img, texi+ImgSize.xy*Vec2(0, 0)), c10=TexPoint(img, texi+ImgSize.xy*Vec2(1, 0)), c20=TexPoint(img, texi+ImgSize.xy*Vec2(2, 0)), c30=TexPoint(img, texi+ImgSize.xy*Vec2(3, 0)),
         c01=TexPoint(img, texi+ImgSize.xy*Vec2(0, 1)), c11=TexPoint(img, texi+ImgSize.xy*Vec2(1, 1)), c21=TexPoint(img, texi+ImgSize.xy*Vec2(2, 1)), c31=TexPoint(img, texi+ImgSize.xy*Vec2(3, 1)),
         c02=TexPoint(img, texi+ImgSize.xy*Vec2(0, 2)), c12=TexPoint(img, texi+ImgSize.xy*Vec2(1, 2)), c22=TexPoint(img, texi+ImgSize.xy*Vec2(2, 2)), c32=TexPoint(img, texi+ImgSize.xy*Vec2(3, 2)),
         c03=TexPoint(img, texi+ImgSize.xy*Vec2(0, 3)), c13=TexPoint(img, texi+ImgSize.xy*Vec2(1, 3)), c23=TexPoint(img, texi+ImgSize.xy*Vec2(2, 3)), c33=TexPoint(img, texi+ImgSize.xy*Vec2(3, 3));

   VecH4 c0=Lerp4(c00, c10, c20, c30, texf.x),
         c1=Lerp4(c01, c11, c21, c31, texf.x),
         c2=Lerp4(c02, c12, c22, c32, texf.x),
         c3=Lerp4(c03, c13, c23, c33, texf.x);

   return Lerp4(c0, c1, c2, c3, texf.y);
#else // optimized
   uv*=ImgSize.zw;
   Vec2 tc=Floor(uv-0.5)+0.5;
   VecH2 f=uv-tc, f2=f*f, f3=f2*f,
        w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
   #if 0
        w2=-1.5*f3+2*f2+0.5*f, w3=0.5*(f3-f2);
   #else
        w3=0.5*(f3-f2), w2=1-w0-w1-w3;
   #endif

   tc*=ImgSize.xy;
   Vec2 tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
#if 0 // 16 tex reads
   Vec2 tc2=tc+ImgSize.xy;

 //Flt w[4][4]={(w0.x*w0.y), (w1.x*w0.y), (w2.x*w0.y), (w3.x*w0.y),
 //             (w0.x*w1.y), (w1.x*w1.y), (w2.x*w1.y), (w3.x*w1.y),
 //             (w0.x*w2.y), (w1.x*w2.y), (w2.x*w2.y), (w3.x*w2.y),
 //             (w0.x*w3.y), (w1.x*w3.y), (w2.x*w3.y), (w3.x*w3.y)};

   return TexPoint(img, Vec2(tc0.x, tc0.y))*(w0.x*w0.y)
         +TexPoint(img, Vec2(tc .x, tc0.y))*(w1.x*w0.y)
         +TexPoint(img, Vec2(tc0.x, tc .y))*(w0.x*w1.y)
         +TexPoint(img, Vec2(tc .x, tc .y))*(w1.x*w1.y)

         +TexPoint(img, Vec2(tc2.x, tc0.y))*(w2.x*w0.y)
         +TexPoint(img, Vec2(tc3.x, tc0.y))*(w3.x*w0.y)
         +TexPoint(img, Vec2(tc2.x, tc .y))*(w2.x*w1.y)
         +TexPoint(img, Vec2(tc3.x, tc .y))*(w3.x*w1.y)
 
         +TexPoint(img, Vec2(tc0.x, tc2.y))*(w0.x*w2.y)
         +TexPoint(img, Vec2(tc .x, tc2.y))*(w1.x*w2.y)
         +TexPoint(img, Vec2(tc0.x, tc3.y))*(w0.x*w3.y)
         +TexPoint(img, Vec2(tc .x, tc3.y))*(w1.x*w3.y)

         +TexPoint(img, Vec2(tc2.x, tc2.y))*(w2.x*w2.y)
         +TexPoint(img, Vec2(tc3.x, tc2.y))*(w3.x*w2.y)
         +TexPoint(img, Vec2(tc2.x, tc3.y))*(w2.x*w3.y)
         +TexPoint(img, Vec2(tc3.x, tc3.y))*(w3.x*w3.y);
#else // 5 tex reads, corners are ignored because they're insignificant
   VecH2 w12=w1+w2; Vec2 c=tc+(w2/w12)*ImgSize.xy;
   Half  wu=w12.x*w0.y, wd=w12.x*w3.y, wl=w12.y*w0.x, wr=w12.y*w3.x, wc=w12.x*w12.y;
   return(TexLod(img, Vec2(  c.x, tc0.y))*wu // sample upper edge (2 texels), both weights are negative
         +TexLod(img, Vec2(  c.x, tc3.y))*wd // sample lower edge (2 texels), both weights are negative
         +TexLod(img, Vec2(tc0.x,   c.y))*wl // sample left  edge (2 texels), both weights are negative
         +TexLod(img, Vec2(tc3.x,   c.y))*wr // sample right edge (2 texels), both weights are negative
         +TexLod(img, Vec2(  c.x,   c.y))*wc // sample center     (4 texels), all  weights are positive
         )/(wu+wd+wl+wr+wc);
#endif
#endif*/

   Vec2  c,  l,  r,  u,  d;
   Half wc, wl, wr, wu, wd;

   void set(Vec2 uv)
   {
      uv*=ImgSize.zw;
      Vec2 tc=Floor(uv-0.5)+0.5;
      VecH2 f=uv-tc, f2=f*f, f3=f2*f,
           w0=f2-0.5*(f3+f), w1=1.5*f3-2.5*f2+1.0,
      #if 0
           w2=-1.5*f3+2*f2+0.5*f, w3=0.5*(f3-f2);
      #else
           w3=0.5*(f3-f2), w2=1-w0-w1-w3;
      #endif

      tc*=ImgSize.xy;
      Vec2 tc0=tc-ImgSize.xy, tc3=tc+ImgSize.xy*2;
      VecH2 w12=w1+w2; c=tc+(w2/w12)*ImgSize.xy;
      wu=w12.x*w0.y; wd=w12.x*w3.y; wl=w12.y*w0.x; wr=w12.y*w3.x; wc=w12.x*w12.y;
      Half sum=wc+wl+wr+wu+wd;
      wc/=sum;
      wl/=sum; l=Vec2(tc0.x,   c.y);
      wr/=sum; r=Vec2(tc3.x,   c.y);
      wu/=sum; u=Vec2(  c.x, tc0.y);
      wd/=sum; d=Vec2(  c.x, tc3.y);
   }
   void UVClamp(Vec2 min, Vec2 max)
   {
      c=Mid(c, min, max);
      l=Mid(l, min, max);
      r=Mid(r, min, max);
      u=Mid(u, min, max);
      d=Mid(d, min, max);
   }
   VecH4 tex(Image img)
   {
      return TexLod(img, u)*wu  // sample upper edge (2 texels), both weights are negative
            +TexLod(img, l)*wl  // sample left  edge (2 texels), both weights are negative
            +TexLod(img, c)*wc  // sample center     (4 texels), all  weights are positive
            +TexLod(img, r)*wr  // sample right edge (2 texels), both weights are negative
            +TexLod(img, d)*wd; // sample lower edge (2 texels), both weights are negative
   }
   VecH texRGB(Image img) // ignores alpha channel
   {
      return TexLod(img, u).rgb*wu  // sample upper edge (2 texels), both weights are negative
            +TexLod(img, l).rgb*wl  // sample left  edge (2 texels), both weights are negative
            +TexLod(img, c).rgb*wc  // sample center     (4 texels), all  weights are positive
            +TexLod(img, r).rgb*wr  // sample right edge (2 texels), both weights are negative
            +TexLod(img, d).rgb*wd; // sample lower edge (2 texels), both weights are negative
   }
   Half texX(ImageH img)
   {
      return TexLod(img, u).x*wu  // sample upper edge (2 texels), both weights are negative
            +TexLod(img, l).x*wl  // sample left  edge (2 texels), both weights are negative
            +TexLod(img, c).x*wc  // sample center     (4 texels), all  weights are positive
            +TexLod(img, r).x*wr  // sample right edge (2 texels), both weights are negative
            +TexLod(img, d).x*wd; // sample lower edge (2 texels), both weights are negative
   }
};
/******************************************************************************/
VecH4 TexCubicFast   (Image img, Vec2 uv) {CubicFastSampler cs; cs.set(uv); return cs.tex   (img);}
VecH  TexCubicFastRGB(Image img, Vec2 uv) {CubicFastSampler cs; cs.set(uv); return cs.texRGB(img);} // ignores alpha channel
/******************************************************************************/
