/******************************************************************************

   'SoundRecord' methods on DirectSound always require usage of 'SoundAPILock' locks,
      not because of the API thread-safety, but because of:
      -objects are added/removed to 'SoundRecords' list
      -objects can be processed on the main thread by the user, and on the sound thread by the engine

/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
static inline void Set(I16 &sample, Flt value)
{
   Int v=Round(value);
   sample=Mid(v, SHORT_MIN, SHORT_MAX);
}
Int SoundResampler::set()
{
   if(speed==1) // no resample needed
   {
      Int samples=Min(src_samples, dest_samples), samples_left=samples;
      dest_samples-=samples;
      switch(dest_channels)
      {
         case 1: switch(src_channels) // DEST MONO
         {
            case 1: // SRC MONO
         #if 1
            REP(samples_left>>2)
            {
               Set(dest_mono[0], src_mono[0]*volume[0]);
               Set(dest_mono[1], src_mono[1]*volume[0]);
               Set(dest_mono[2], src_mono[2]*volume[0]);
               Set(dest_mono[3], src_mono[3]*volume[0]);
               src_mono+=4; dest_mono+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Set(*dest_mono++, *src_mono++ * volume[0]);
            }
            break;

            case 2: // SRC STEREO
         #if 1
            REP(samples_left>>2)
            {
               Set(dest_mono[0], src_stereo[0].l*volume[0] + src_stereo[0].r*volume[1]);
               Set(dest_mono[1], src_stereo[1].l*volume[0] + src_stereo[1].r*volume[1]);
               Set(dest_mono[2], src_stereo[2].l*volume[0] + src_stereo[2].r*volume[1]);
               Set(dest_mono[3], src_stereo[3].l*volume[0] + src_stereo[3].r*volume[1]);
               src_stereo+=4; dest_mono+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Set(*dest_mono++, src_stereo->l*volume[0] + src_stereo->r*volume[1]);
               src_stereo++;
            }
            break;
         }break;

         case 2: switch(src_channels) // DEST STEREO
         {
            case 1: // SRC MONO
         #if 1
            REP(samples_left>>2)
            {
               I16 sample=src_mono[0]; Set(dest_stereo[0].l, sample*volume[0]); Set(dest_stereo[0].r, sample*volume[1]);
                   sample=src_mono[1]; Set(dest_stereo[1].l, sample*volume[0]); Set(dest_stereo[1].r, sample*volume[1]);
                   sample=src_mono[2]; Set(dest_stereo[2].l, sample*volume[0]); Set(dest_stereo[2].r, sample*volume[1]);
                   sample=src_mono[3]; Set(dest_stereo[3].l, sample*volume[0]); Set(dest_stereo[3].r, sample*volume[1]);
               src_mono+=4; dest_stereo+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               I16 sample=*src_mono++;
               Set(dest_stereo->l, sample*volume[0]);
               Set(dest_stereo->r, sample*volume[1]);
               dest_stereo++;
            }
            break;

            case 2: // SRC STEREO
         #if 1
            REP(samples_left>>2)
            {
               Set(dest_stereo[0].l, src_stereo[0].l*volume[0]); Set(dest_stereo[0].r, src_stereo[0].r*volume[1]);
               Set(dest_stereo[1].l, src_stereo[1].l*volume[0]); Set(dest_stereo[1].r, src_stereo[1].r*volume[1]);
               Set(dest_stereo[2].l, src_stereo[2].l*volume[0]); Set(dest_stereo[2].r, src_stereo[2].r*volume[1]);
               Set(dest_stereo[3].l, src_stereo[3].l*volume[0]); Set(dest_stereo[3].r, src_stereo[3].r*volume[1]);
               src_stereo+=4; dest_stereo+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Set(dest_stereo->l, src_stereo->l*volume[0]);
               Set(dest_stereo->r, src_stereo->r*volume[1]);
               src_stereo++; dest_stereo++;
            }
            break;
         }break;
      }
      return samples;
   }else // resample
   {
      Int dest_sample_pos=0,
           src_sample_pos;
      for(; ; dest_sample_pos++)
      {
         Flt src_sample_posf=dest_sample_pos*speed ; if( SUPPORT_SAMPLE_OFFSET)src_sample_posf+=src_sample_offset; // add leftover offset from previous operations
             src_sample_pos =Trunc(src_sample_posf); if(!SUPPORT_SAMPLE_OFFSET && (src_sample_pos>=src_samples || dest_sample_pos>=dest_samples))break; // if reached the end of any buffer then stop !! this has to be done after calculating 'src_sample_pos' which is used later !!
         Flt frac=src_sample_posf-src_sample_pos; // calculate sample position fraction
         if(SUPPORT_SAMPLE_OFFSET && (src_sample_pos>=src_samples || dest_sample_pos>=dest_samples)){src_sample_offset=frac; break;} // if reached the end of any buffer then stop and remember current 'src_sample_offset' for future operations !! this has to be done after calculating 'src_sample_pos' which is used later !!

         if(speed<1)
         { // cubic
            Int src_sample_posP=Max(src_sample_pos-1, 0);
            Int src_sample_pos1=Min(src_sample_pos+1, src_samples-1);
            Int src_sample_pos2=Min(src_sample_pos+2, src_samples-1);
            Vec4 w; Lerp4Weights(w, frac);
            switch(dest_channels)
            {
               case 1: switch(src_channels) // DEST MONO
               {
                  case 1: // SRC MONO
                  {
                     Set(*dest_mono++, (src_mono[src_sample_posP]*w.x + src_mono[src_sample_pos]*w.y + src_mono[src_sample_pos1]*w.z + src_mono[src_sample_pos2]*w.w)*volume[0]);
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &sP=src_stereo[src_sample_posP],
                            &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1],
                            &s2=src_stereo[src_sample_pos2];
                     Set(*dest_mono++, (sP.l*w.x + s0.l*w.y + s1.l*w.z + s2.l*w.w)*volume[0]   // Lerp(s0.l, s1.l, frac)
                                      +(sP.r*w.x + s0.r*w.y + s1.r*w.z + s2.r*w.w)*volume[1]); // Lerp(s0.r, s1.r, frac)
                  }break;
               }break;

               case 2: switch(src_channels) // DEST STEREO
               {
                  case 1: // SRC MONO
                  {
                     Flt sample=src_mono[src_sample_posP]*w.x + src_mono[src_sample_pos]*w.y + src_mono[src_sample_pos1]*w.z + src_mono[src_sample_pos2]*w.w;
                     Set(dest_stereo->l, sample*volume[0]);
                     Set(dest_stereo->r, sample*volume[1]);
                     dest_stereo++;
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &sP=src_stereo[src_sample_posP],
                            &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1],
                            &s2=src_stereo[src_sample_pos2];
                     Set(dest_stereo->l, (sP.l*w.x + s0.l*w.y + s1.l*w.z + s2.l*w.w)*volume[0]); // Lerp(s0.l, s1.l, frac)
                     Set(dest_stereo->r, (sP.r*w.x + s0.r*w.y + s1.r*w.z + s2.r*w.w)*volume[1]); // Lerp(s0.r, s1.r, frac)
                     dest_stereo++;
                  }break;
               }break;
            }
         }else // speed>1
         { // linear
            Int src_sample_pos1=Min(src_sample_pos+1, src_samples-1);
            Flt frac1=1-frac;
            switch(dest_channels)
            {
               case 1: switch(src_channels) // DEST MONO
               {
                  case 1: // SRC MONO
                  {
                     Set(*dest_mono++, (src_mono[src_sample_pos]*frac1 + src_mono[src_sample_pos1]*frac)*volume[0]); // Lerp(src_mono[src_sample_pos], src_mono[src_sample_pos1], frac)
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1];
                     Set(*dest_mono++, (s0.l*frac1 + s1.l*frac)*volume[0]   // Lerp(s0.l, s1.l, frac)
                                      +(s0.r*frac1 + s1.r*frac)*volume[1]); // Lerp(s0.r, s1.r, frac)
                  }break;
               }break;

               case 2: switch(src_channels) // DEST STEREO
               {
                  case 1: // SRC MONO
                  {
                     Flt sample=src_mono[src_sample_pos]*frac1 + src_mono[src_sample_pos1]*frac; // Lerp(src_mono[src_sample_pos], src_mono[src_sample_pos1], frac)
                     Set(dest_stereo->l, sample*volume[0]);
                     Set(dest_stereo->r, sample*volume[1]);
                     dest_stereo++;
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1];
                     Set(dest_stereo->l, (s0.l*frac1 + s1.l*frac)*volume[0]); // Lerp(s0.l, s1.l, frac)
                     Set(dest_stereo->r, (s0.r*frac1 + s1.r*frac)*volume[1]); // Lerp(s0.r, s1.r, frac)
                     dest_stereo++;
                  }break;
               }break;
            }
         }
      }
      dest_samples-=dest_sample_pos;
      return         src_sample_pos;
   }
}
static inline void Add(I16 &sample, Flt value)
{
   Int v=Round(value);
   sample=Mid(sample+v, SHORT_MIN, SHORT_MAX);
}
Int SoundResampler::add()
{
   if(speed==1) // no resample needed
   {
      Int samples=Min(src_samples, dest_samples), samples_left=samples;
      dest_samples-=samples;
      switch(dest_channels)
      {
         case 1: switch(src_channels) // DEST MONO
         {
            case 1: // SRC MONO
         #if 1
            REP(samples_left>>2)
            {
               Add(dest_mono[0], src_mono[0]*volume[0]);
               Add(dest_mono[1], src_mono[1]*volume[0]);
               Add(dest_mono[2], src_mono[2]*volume[0]);
               Add(dest_mono[3], src_mono[3]*volume[0]);
               src_mono+=4; dest_mono+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Add(*dest_mono++, *src_mono++ * volume[0]);
            }
            break;

            case 2: // SRC STEREO
         #if 1
            REP(samples_left>>2)
            {
               Add(dest_mono[0], src_stereo[0].l*volume[0] + src_stereo[0].r*volume[1]);
               Add(dest_mono[1], src_stereo[1].l*volume[0] + src_stereo[1].r*volume[1]);
               Add(dest_mono[2], src_stereo[2].l*volume[0] + src_stereo[2].r*volume[1]);
               Add(dest_mono[3], src_stereo[3].l*volume[0] + src_stereo[3].r*volume[1]);
               src_stereo+=4; dest_mono+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Add(*dest_mono++, src_stereo->l*volume[0] + src_stereo->r*volume[1]);
               src_stereo++;
            }
            break;
         }break;

         case 2: switch(src_channels) // DEST STEREO
         {
            case 1: // SRC MONO
         #if 1
            REP(samples_left>>2)
            {
               I16 sample=src_mono[0]; Add(dest_stereo[0].l, sample*volume[0]); Add(dest_stereo[0].r, sample*volume[1]);
                   sample=src_mono[1]; Add(dest_stereo[1].l, sample*volume[0]); Add(dest_stereo[1].r, sample*volume[1]);
                   sample=src_mono[2]; Add(dest_stereo[2].l, sample*volume[0]); Add(dest_stereo[2].r, sample*volume[1]);
                   sample=src_mono[3]; Add(dest_stereo[3].l, sample*volume[0]); Add(dest_stereo[3].r, sample*volume[1]);
               src_mono+=4; dest_stereo+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               I16 sample=*src_mono++;
               Add(dest_stereo->l, sample*volume[0]);
               Add(dest_stereo->r, sample*volume[1]);
               dest_stereo++;
            }
            break;

            case 2: // SRC STEREO
         #if 1
            REP(samples_left>>2)
            {
               Add(dest_stereo[0].l, src_stereo[0].l*volume[0]); Add(dest_stereo[0].r, src_stereo[0].r*volume[1]);
               Add(dest_stereo[1].l, src_stereo[1].l*volume[0]); Add(dest_stereo[1].r, src_stereo[1].r*volume[1]);
               Add(dest_stereo[2].l, src_stereo[2].l*volume[0]); Add(dest_stereo[2].r, src_stereo[2].r*volume[1]);
               Add(dest_stereo[3].l, src_stereo[3].l*volume[0]); Add(dest_stereo[3].r, src_stereo[3].r*volume[1]);
               src_stereo+=4; dest_stereo+=4;
            }
            samples_left&=3;
         #endif
            REP(samples_left)
            {
               Add(dest_stereo->l, src_stereo->l*volume[0]);
               Add(dest_stereo->r, src_stereo->r*volume[1]);
               src_stereo++; dest_stereo++;
            }
            break;
         }break;
      }
      return samples;
   }else // resample
   {
      Int dest_sample_pos=0,
           src_sample_pos;
      for(; ; dest_sample_pos++)
      {
         Flt src_sample_posf=dest_sample_pos*speed ; if( SUPPORT_SAMPLE_OFFSET)src_sample_posf+=src_sample_offset; // add leftover offset from previous operations
             src_sample_pos =Trunc(src_sample_posf); if(!SUPPORT_SAMPLE_OFFSET && (src_sample_pos>=src_samples || dest_sample_pos>=dest_samples))break; // if reached the end of any buffer then stop !! this has to be done after calculating 'src_sample_pos' which is used later !!
         Flt frac=src_sample_posf-src_sample_pos; // calculate sample position fraction
         if(SUPPORT_SAMPLE_OFFSET && (src_sample_pos>=src_samples || dest_sample_pos>=dest_samples)){src_sample_offset=frac; break;} // if reached the end of any buffer then stop and remember current 'src_sample_offset' for future operations !! this has to be done after calculating 'src_sample_pos' which is used later !!

         if(speed<1)
         { // cubic
            Int src_sample_posP=Max(src_sample_pos-1, 0);
            Int src_sample_pos1=Min(src_sample_pos+1, src_samples-1);
            Int src_sample_pos2=Min(src_sample_pos+2, src_samples-1);
            Vec4 w; Lerp4Weights(w, frac);
            switch(dest_channels)
            {
               case 1: switch(src_channels) // DEST MONO
               {
                  case 1: // SRC MONO
                  {
                     Add(*dest_mono++, (src_mono[src_sample_posP]*w.x + src_mono[src_sample_pos]*w.y + src_mono[src_sample_pos1]*w.z + src_mono[src_sample_pos2]*w.w)*volume[0]);
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &sP=src_stereo[src_sample_posP],
                            &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1],
                            &s2=src_stereo[src_sample_pos2];
                     Add(*dest_mono++, (sP.l*w.x + s0.l*w.y + s1.l*w.z + s2.l*w.w)*volume[0]   // Lerp(s0.l, s1.l, frac)
                                      +(sP.r*w.x + s0.r*w.y + s1.r*w.z + s2.r*w.w)*volume[1]); // Lerp(s0.r, s1.r, frac)
                  }break;
               }break;

               case 2: switch(src_channels) // DEST STEREO
               {
                  case 1: // SRC MONO
                  {
                     Flt sample=src_mono[src_sample_posP]*w.x + src_mono[src_sample_pos]*w.y + src_mono[src_sample_pos1]*w.z + src_mono[src_sample_pos2]*w.w;
                     Add(dest_stereo->l, sample*volume[0]);
                     Add(dest_stereo->r, sample*volume[1]);
                     dest_stereo++;
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &sP=src_stereo[src_sample_posP],
                            &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1],
                            &s2=src_stereo[src_sample_pos2];
                     Add(dest_stereo->l, (sP.l*w.x + s0.l*w.y + s1.l*w.z + s2.l*w.w)*volume[0]); // Lerp(s0.l, s1.l, frac)
                     Add(dest_stereo->r, (sP.r*w.x + s0.r*w.y + s1.r*w.z + s2.r*w.w)*volume[1]); // Lerp(s0.r, s1.r, frac)
                     dest_stereo++;
                  }break;
               }break;
            }
         }else // speed>1
         { // linear
            Int src_sample_pos1=Min(src_sample_pos+1, src_samples-1);
            Flt frac1=1-frac;
            switch(dest_channels)
            {
               case 1: switch(src_channels) // DEST MONO
               {
                  case 1: // SRC MONO
                  {
                     Add(*dest_mono++, (src_mono[src_sample_pos]*frac1 + src_mono[src_sample_pos1]*frac)*volume[0]); // Lerp(src_mono[src_sample_pos], src_mono[src_sample_pos1], frac)
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1];
                     Add(*dest_mono++, (s0.l*frac1 + s1.l*frac)*volume[0]   // Lerp(s0.l, s1.l, frac)
                                      +(s0.r*frac1 + s1.r*frac)*volume[1]); // Lerp(s0.r, s1.r, frac)
                  }break;
               }break;

               case 2: switch(src_channels) // DEST STEREO
               {
                  case 1: // SRC MONO
                  {
                     Flt sample=src_mono[src_sample_pos]*frac1 + src_mono[src_sample_pos1]*frac; // Lerp(src_mono[src_sample_pos], src_mono[src_sample_pos1], frac)
                     Add(dest_stereo->l, sample*volume[0]);
                     Add(dest_stereo->r, sample*volume[1]);
                     dest_stereo++;
                  }break;

                  case 2: // SRC STEREO
                  {
                   C Stereo &s0=src_stereo[src_sample_pos ],
                            &s1=src_stereo[src_sample_pos1];
                     Add(dest_stereo->l, (s0.l*frac1 + s1.l*frac)*volume[0]); // Lerp(s0.l, s1.l, frac)
                     Add(dest_stereo->r, (s0.r*frac1 + s1.r*frac)*volume[1]); // Lerp(s0.r, s1.r, frac)
                     dest_stereo++;
                  }break;
               }break;
            }
         }
      }
      dest_samples-=dest_sample_pos;
      return         src_sample_pos;
   }
}
/******************************************************************************/
Bool SoundResample(Int src_samples, Int src_channels, I16 *src_data, MemPtr<I16> dest_data, Flt speed, C Flt *volume)
{
   if(speed>EPS
   && (src_channels==1 || src_channels==2)
   && dest_data.continuous())
   {
      Int dest_samples=Round(src_samples/speed);
      if( dest_samples>=0 && Abs(RoundL(dest_samples*speed)-src_samples)<=65536) // check for overflow
      {
         Int dest_channels=src_channels;
         Flt vol[2];
         if(volume)
         {
            vol[0]=volume[0];
            vol[1]=volume[Min(1, dest_channels-1)];
         }else
         {
            vol[0]=vol[1]=1;
         }
         dest_data.setNum(dest_samples*dest_channels);
         SoundResampler resampler(speed, vol, dest_channels, dest_samples, dest_data.data(), src_channels);
         resampler.setSrc(src_samples, src_data);
         resampler.set();
         Int unwritten=resampler.dest_channels*resampler.dest_samples;
         ZeroFastN(dest_data.data()+(dest_data.elms()-unwritten), unwritten); // zero unwritten
         return true;
      }
   }
   dest_data.clear(); return false;
}
/******************************************************************************/
} // namespace EE
/******************************************************************************/
