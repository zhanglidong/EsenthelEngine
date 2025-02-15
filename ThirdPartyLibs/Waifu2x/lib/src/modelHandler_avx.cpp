/*
* The MIT License (MIT)
* Copyright (c) 2015 amigo(white luckers), tanakamura, DeadSix27, YukihoAA and contributors
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#if (defined _M_IX86 || defined __i386__) || (defined _M_X64 || defined __x86_64__)

#include <thread>
#include <immintrin.h>
#include <atomic>
#include "filters.hpp"
#include "Env.hpp"

#if !defined _WIN32 && ((defined _M_IX86 || defined __i386__) || (defined _M_X64 || defined __x86_64__))
   #define FUNC_ATTR __attribute__((target("avx")))
#else
   #define FUNC_ATTR
#endif

typedef __m256 v256_t;
FUNC_ATTR static inline __m256 madd256(__m256 v0, __m256 v1, __m256 v2)
{
	return _mm256_add_ps(_mm256_mul_ps(v0, v1), v2);
}

#define load_broadcast _mm256_broadcast_ss
#define load256 _mm256_load_ps
#define store256 _mm256_store_ps
#define add256 _mm256_add_ps
#define max256 _mm256_max_ps
#define min256 _mm256_min_ps
#define zero _mm256_setzero_ps
#define set1 _mm256_set1_ps
#define mul256 _mm256_mul_ps

FUNC_ATTR static inline float hadd8(__m256 v)
{
	v = _mm256_hadd_ps(v, v);
	v = _mm256_hadd_ps(v, v);

	float v0 = _mm_cvtss_f32(_mm256_extractf128_ps(v,0));
	float v1 = _mm_cvtss_f32(_mm256_extractf128_ps(v,1));

	return v0 + v1;
}

#include "modelHandler_avx_func.hpp"


#undef UNROLL
typedef __m256 vreg_t;
#define VEC_NELEM 8
#ifdef __x86_64
#define UNROLL 5
#else
#define UNROLL 2
#endif
#define store_vreg(ptr,val) _mm256_store_ps((float*)(ptr), val)
#define load_vreg(ptr) _mm256_load_ps((float*)(ptr))
#define load_vreg_broadcast(ptr) _mm256_broadcast_ss((float*)(ptr))
FUNC_ATTR static inline __m256 madd_vreg(__m256 a, __m256 b, __m256 c)
{
    return _mm256_add_ps(_mm256_mul_ps(a,b), c);
}
#define add_vreg _mm256_add_ps
#define zero_vreg _mm256_setzero_ps
#define min_vreg _mm256_min_ps
#define max_vreg _mm256_max_ps
#define set1_vreg _mm256_set1_ps

#define SIMD_OPLANE

#include "modelHandler_simd.hpp"

namespace w2xc
{
	void filter_AVX_impl
	(
		ComputeEnv *env,
		const float *packed_input,
		float *packed_output,
		int nInputPlanes,
		int nOutputPlanes,
		const float *fbiases,
		const float *weight,
		int ip_width,
		int ip_height
	)
	{
		if (simd_available(nInputPlanes, nOutputPlanes))
		{
			filter_simd_impl0
			(
				env,
				packed_input,
				packed_output,
				nInputPlanes,
				nOutputPlanes,
				fbiases,
				weight,
				ip_width,
				ip_height
			);
		}
		else
		{
			filter_AVX_impl0
			(
				env,
				packed_input,
				packed_output,
				nInputPlanes,
				nOutputPlanes,
				fbiases,
				weight,
				ip_width,
				ip_height
			);
		}
	}
}

#endif