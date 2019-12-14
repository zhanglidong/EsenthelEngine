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

#include "modelHandler.hpp"
#include "cvwrap.hpp"
#include <fstream>
#include <thread>
#include <atomic>
#include "common.hpp"
#include "filters.hpp"
#include "params.h"
#include "Env.hpp"
#include "w2xconv.h"
#include "threadPool.hpp"

namespace w2xc
{
	bool Model::filter_CV
	(
		ComputeEnv *env,
		Buffer *packed_input_buf,
		Buffer *packed_output_buf,
		const W2Size &size
	)
	{
		size_t in_size = sizeof(float) * size.width * size.height * nInputPlanes;
		const float *packed_input = (float*)packed_input_buf->get_read_ptr_host(env, in_size);
		float *packed_output = (float*)packed_output_buf->get_write_ptr_host(env);

		auto thread_func = [&](IntPtr yi)
		{
			int w = size.width;
			int h = size.height;

			float *out_line = packed_output + w*nOutputPlanes * yi;

			int yi0 = yi-1;
			int yi1 = yi;
			int yi2 = yi+1;

			if (yi == 0)
			{
				yi0 = 0;
			}

			if (yi == h-1)
			{
				yi2 = yi1;
			}

			const float *in_line0 = packed_input + w * nInputPlanes * yi0;
			const float *in_line1 = packed_input + w * nInputPlanes * yi1;
			const float *in_line2 = packed_input + w * nInputPlanes * yi2;

			for (int xi=0; xi<w; xi++)
			{
				int x0 = xi-1;
				int x1 = xi;
				int x2 = xi+1;

				if (xi == 0)
				{
					x0 = 0;
				}

				if (xi == w-1)
				{
					x2 = x1;
				}

				const float *in00 = in_line0 + x0 * nInputPlanes;
				const float *in01 = in_line0 + x1 * nInputPlanes;
				const float *in02 = in_line0 + x2 * nInputPlanes;

				const float *in10 = in_line1 + x0 * nInputPlanes;
				const float *in11 = in_line1 + x1 * nInputPlanes;
				const float *in12 = in_line1 + x2 * nInputPlanes;

				const float *in20 = in_line2 + x0 * nInputPlanes;
				const float *in21 = in_line2 + x1 * nInputPlanes;
				const float *in22 = in_line2 + x2 * nInputPlanes;

				for (int oi=0; oi<nOutputPlanes; oi++)
				{
					float sum = 0;

					for (int ii=0; ii<nInputPlanes; ii++)
					{
						int wMatIndex = nInputPlanes * oi + ii;
						const float *w = weights[wMatIndex].ptr<float>(0);

						sum += in00[ii] * w[0];
						sum += in01[ii] * w[1];
						sum += in02[ii] * w[2];

						sum += in10[ii] * w[3];
						sum += in11[ii] * w[4];
						sum += in12[ii] * w[5];

						sum += in20[ii] * w[6];
						sum += in21[ii] * w[7];
						sum += in22[ii] * w[8];
					}

					float v = sum;
					v += (float) biases[oi];
					float mtz = (std::max)(v, 0.0f);
					float ltz = (std::min)(v, 0.0f);
					v = ltz*0.1f + mtz;

					out_line[xi*nOutputPlanes + oi] = v;
				}
			}
		};

      EE::MultiThreadedCall(size.height, thread_func);
		return true;
	}

	bool Model::filter_AVX_OpenCL
	(
		W2XConv *conv,
		ComputeEnv *env,
		Buffer *packed_input_buf,
		Buffer *packed_output_buf,
		const W2Size &size
	)
	{
		int vec_width;
		int weight_step;
		const struct W2XConvProcessor *proc = conv->target_processor;

		bool gpu = (proc->type == W2XCONV_PROC_OPENCL) || (proc->type == W2XCONV_PROC_CUDA);

		if (gpu)
		{
			weight_step = GPU_VEC_WIDTH;
			vec_width = GPU_VEC_WIDTH;
		}
		else
		{
			weight_step = nOutputPlanes;
			vec_width = VEC_WIDTH;
		}

		float *weight_flat = (float*)w2xc_aligned_malloc(sizeof(float)*nInputPlanes*weight_step*3*3, 64);
		float *fbiases_flat = (float*)w2xc_aligned_malloc(sizeof(float) * biases_size(), 64);

		for (int i=0; i<(int)biases_size(); i++)
		{
			fbiases_flat[i] = (float) biases[i];
		}

		if (nOutputPlanes == 1)
		{
			if (gpu)
			{
				for (int ii=0; ii<nInputPlanes; ii++)
				{
					auto &wm = weights[ii];
					const float *src0 = wm.ptr<float>(0);
					const float *src1 = wm.ptr<float>(1);
					const float *src2 = wm.ptr<float>(2);

					float *dst = weight_flat + ii * 9;
					dst[0] = src0[0];
					dst[1] = src0[1];
					dst[2] = src0[2];

					dst[3] = src1[0];
					dst[4] = src1[1];
					dst[5] = src1[2];

					dst[6] = src2[0];
					dst[7] = src2[1];
					dst[8] = src2[2];

				}
			}
			else
			{
				for (int ii=0; ii<nInputPlanes; ii++)
				{
					auto &wm = weights[ii];
					const float *src0 = wm.ptr<float>(0);
					const float *src1 = wm.ptr<float>(1);
					const float *src2 = wm.ptr<float>(2);

					int ii_0 = ii % vec_width;
					int ii_1 = (ii / vec_width) * vec_width;

					float *dst = weight_flat + ii_1 * 9  + ii_0;
					dst[0 * vec_width] = src0[0];
					dst[1 * vec_width] = src0[1];
					dst[2 * vec_width] = src0[2];

					dst[3 * vec_width] = src1[0];
					dst[4 * vec_width] = src1[1];
					dst[5 * vec_width] = src1[2];

					dst[6 * vec_width] = src2[0];
					dst[7 * vec_width] = src2[1];
					dst[8 * vec_width] = src2[2];
				}
			}
		}
		else if (gpu && nInputPlanes == 1)
		{
			for (int oi=0; oi<nOutputPlanes; oi++)
			{
				auto &wm = weights[oi];
				const float *src0 = wm.ptr<float>(0);
				const float *src1 = wm.ptr<float>(1);
				const float *src2 = wm.ptr<float>(2);

				float *dst = weight_flat + oi * 9;
				dst[0] = src0[0];
				dst[1] = src0[1];
				dst[2] = src0[2];

				dst[3] = src1[0];
				dst[4] = src1[1];
				dst[5] = src1[2];

				dst[6] = src2[0];
				dst[7] = src2[1];
				dst[8] = src2[2];
			}
		}
		else if (nOutputPlanes == 3)
		{
			/* |       o0        |       o1        | o2 ... |
			 * |i0 i1 i2 ... i127|i0 i1 i2 ... i127| ...    |*/
			for (int oi=0; oi<nOutputPlanes; oi++)
			{
				for (int ii=0; ii<nInputPlanes; ii++)
				{
					int mi = oi*nInputPlanes+ii;
					auto &wm = weights[mi];
					const float *src0 = wm.ptr<float>(0);
					const float *src1 = wm.ptr<float>(1);
					const float *src2 = wm.ptr<float>(2);

					float *dst = weight_flat + (oi * nInputPlanes * 9) + ii;
					dst[0*nInputPlanes] = src0[0];
					dst[1*nInputPlanes] = src0[1];
					dst[2*nInputPlanes] = src0[2];

					dst[3*nInputPlanes] = src1[0];
					dst[4*nInputPlanes] = src1[1];
					dst[5*nInputPlanes] = src1[2];

					dst[6*nInputPlanes] = src2[0];
					dst[7*nInputPlanes] = src2[1];
					dst[8*nInputPlanes] = src2[2];
				}
			}
		}
		else if (gpu && (nInputPlanes == 3) && (nOutputPlanes == 32))
		{
			/* | i0             | i1        | i2 .. iN-1|
			 * |o0 o1 o2 o3..o31|o0 .... o32| ....      |
			 * |<-            ->|
			 * |    32          |
			 * |   x  9         |
			 */

			for (int oi=0; oi<nOutputPlanes; oi++)
			{
				for (int ii=0; ii<nInputPlanes; ii++)
				{
					int mi = oi*nInputPlanes+ii;
					auto &wm = weights[mi];
					const float *src0 = wm.ptr<float>(0);
					const float *src1 = wm.ptr<float>(1);
					const float *src2 = wm.ptr<float>(2);

					float *dst = weight_flat + (ii * nOutputPlanes * 9) + oi;
					dst[0*nOutputPlanes] = src0[0];
					dst[1*nOutputPlanes] = src0[1];
					dst[2*nOutputPlanes] = src0[2];

					dst[3*nOutputPlanes] = src1[0];
					dst[4*nOutputPlanes] = src1[1];
					dst[5*nOutputPlanes] = src1[2];

					dst[6*nOutputPlanes] = src2[0];
					dst[7*nOutputPlanes] = src2[1];
					dst[8*nOutputPlanes] = src2[2];
				}
			}
		}
		else
		{
			bool simd_oplane = false;
			bool simd_iplane = false;
			int simd_vec_width = 0;

			if (proc->type == W2XCONV_PROC_HOST)
			{
				switch (proc->sub_type)
				{
					case W2XCONV_PROC_HOST_SSE3:
					{
						simd_vec_width = 4;
						simd_oplane = true;
						break;
					}
					case W2XCONV_PROC_HOST_NEON:
					{
						simd_vec_width = 4;
						simd_oplane = true;
						break;
					}
					case W2XCONV_PROC_HOST_ALTIVEC:
					{
						simd_vec_width = 8;
						simd_oplane = true;
						break;
					}
					case W2XCONV_PROC_HOST_AVX:
					case W2XCONV_PROC_HOST_FMA:
					{
						simd_vec_width = 8;
						simd_oplane = true;
						break;
					}
				}
			}

			simd_oplane = simd_oplane && (nInputPlanes%(simd_vec_width*4) == 0) && (nOutputPlanes%(simd_vec_width*2) == 0);
			simd_iplane = simd_iplane && (nInputPlanes%(simd_vec_width*4) == 0) && (nOutputPlanes%(simd_vec_width*2) == 0);

			if (simd_oplane || simd_iplane)
			{
				/* 
				 * weight_chunk (16x32x3x4 = 6144[Byte])
				 * (where op_block_size=16, ip_block_size=32)
				 *
				 * 111                                            oplane x16
				 * 16 16 .. (x16)  ..16                           iplane x32
				 *            \               |               /   horiz  x3
				 *                                                oplane xnOutputPlane_block
				 *                                                iplane xnInputPlane_block
				 *                                                vert   x3
				 */
				int ip_block_size;
				int op_block_size;

				if (simd_oplane)
				{
					ip_block_size = (simd_vec_width*4);
					op_block_size = (simd_vec_width*2);
				}
				else {
					ip_block_size = (simd_vec_width*2);
					op_block_size = (simd_vec_width*4);
				}

				int nInputPlane_block = nInputPlanes/ip_block_size;
				int nOutputPlane_block = nOutputPlanes/op_block_size;

				float *dst = weight_flat;

				for (int dposy=0; dposy<3; dposy++)
				{
					for (int ii0=0; ii0<nInputPlane_block; ii0++)
					{
						for (int oi0=0; oi0<nOutputPlane_block; oi0++)
						{
							for (int dposx=0; dposx<3; dposx++)
							{
								if (simd_oplane)
								{
									for (int ii1=0; ii1<ip_block_size; ii1++)
									{
										for (int oi1=0; oi1<op_block_size; oi1++)
										{
											int ii = ii0*ip_block_size + ii1;
											int oi = oi0*op_block_size + oi1;
											int mi = oi*nInputPlanes + ii;

											auto &wm = weights[mi];
											float &src = wm.at(dposy, dposx);
											*dst = src;

											dst++;
										}
									}
								}
								else
								{
									for (int oi1=0; oi1<op_block_size; oi1++)
									{
										for (int ii1=0; ii1<ip_block_size; ii1++)
										{
											int ii = ii0*ip_block_size + ii1;
											int oi = oi0*op_block_size + oi1;
											int mi = oi*nInputPlanes + ii;

											auto &wm = weights[mi];
											float &src = wm.at(dposy, dposx);
											*dst = src;

											dst++;
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				/* | i0        | i1        | i2 .. iN-1|   i0      | i1        | ..
				 * |o0 o1 o2 o3|o0 o1 o2 o3| ....      |o4 o5 o6 o7|o4 o5 o6 o7| ..
				 * |<-       ->|
				 * | VEC_WIDTH |
				 * |   x  9    |
				 */
				for (int oi=0; oi<nOutputPlanes; oi++)
				{
					for (int ii=0; ii<nInputPlanes; ii++)
					{
						int mi = oi*nInputPlanes+ii;
						auto &wm = weights[mi];
					   const float *src0 = wm.ptr<float>(0);
					   const float *src1 = wm.ptr<float>(1);
					   const float *src2 = wm.ptr<float>(2);

						int oi_0 = oi % vec_width;
						int oi_1 = (oi / vec_width) * vec_width;

						float *dst = weight_flat + ((ii*weight_step + oi_1) * 9) + oi_0;
						dst[0*vec_width] = src0[0];
						dst[1*vec_width] = src0[1];
						dst[2*vec_width] = src0[2];

						dst[3*vec_width] = src1[0];
						dst[4*vec_width] = src1[1];
						dst[5*vec_width] = src1[2];

						dst[6*vec_width] = src2[0];
						dst[7*vec_width] = src2[1];
						dst[8*vec_width] = src2[2];
					}
				}
			}
		}

		size_t in_size = size.width * size.height * sizeof(float) * nInputPlanes;
		size_t out_size = size.width * size.height * sizeof(float) * nOutputPlanes;

		{
#ifdef CLLIB_H
			if (proc->type == W2XCONV_PROC_OPENCL)
			{
				filter_OpenCL_impl
				(
					env,
					packed_input_buf,
					packed_output_buf,
					nInputPlanes,
					nOutputPlanes,
					fbiases_flat,
					weight_flat,
					size.width,
					size.height
				);
			}else
#endif
#ifdef HAVE_CUDA
         if (proc->type == W2XCONV_PROC_CUDA)
			{
				filter_CUDA_impl
				(
					env,
					packed_input_buf,
					packed_output_buf,
					nInputPlanes,
					nOutputPlanes,
					fbiases_flat,
					weight_flat,
					size.width,
					size.height
				);
			}else
#endif
			{
				const float *packed_input = (float*)packed_input_buf->get_read_ptr_host(env, in_size);
				float *packed_output = (float*)packed_output_buf->get_write_ptr_host(env);

				switch (proc->sub_type)
				{
#if ((defined _M_IX86 || defined __i386__) || (defined _M_X64 || defined __x86_64__))
					case W2XCONV_PROC_HOST_FMA:
					{
						filter_FMA_impl
						(
							env,
							packed_input,
							packed_output,
							nInputPlanes,
							nOutputPlanes,
							fbiases_flat,
							weight_flat,
							size.width,
							size.height
						);
						break;
					}
					case W2XCONV_PROC_HOST_AVX:
					{
						filter_AVX_impl
						(
							env,
							packed_input,
							packed_output,
							nInputPlanes,
							nOutputPlanes,
							fbiases_flat,
							weight_flat,
							size.width,
							size.height
						);
						break;
					}
					case W2XCONV_PROC_HOST_SSE3:
					{
						filter_SSE_impl
						(
							env,
							packed_input,
							packed_output,
							nInputPlanes,
							nOutputPlanes,
							fbiases_flat,
							weight_flat,
							size.width,
							size.height
						);
						break;
					}
#endif
#if (defined _M_ARM || defined __arm__) || (defined _M_ARM64 || defined __aarch64__)
					case W2XCONV_PROC_HOST_NEON:
					{
						filter_NEON_impl
						(
							env,
							packed_input,
							packed_output,
							nInputPlanes,
							nOutputPlanes,
							fbiases_flat,
							weight_flat,
							size.width,
							size.height
						);
						break;
					}
#endif
#ifdef PPCOPT
					case W2XCONV_PROC_HOST_ALTIVEC:
					{
						filter_AltiVec_impl
						(
							env,
							packed_input,
							packed_output,
							nInputPlanes,
							nOutputPlanes,
							fbiases_flat,
							weight_flat,
							size.width,
							size.height
						);
						break;
					}
#endif
					default:
					{
						filter_CV(env, packed_input_buf, packed_output_buf, size);
						break;
					}
				}
			}
		}

		w2xc_aligned_free(fbiases_flat);
		w2xc_aligned_free(weight_flat);

		return true;
	}

	bool Model::filter (W2XConv *conv, ComputeEnv *env, Buffer *packed_input_buf, Buffer *packed_output_buf, W2Size const &size)
	{
		bool ret;

		bool avx_available = true;
		bool cl_available = true;
		bool cuda_available = true;

		if (nOutputPlanes > GPU_VEC_WIDTH)
		{
			cl_available = false;
			cuda_available = false;
		}

		if (nOutputPlanes == 32 && nInputPlanes == 1)
		{
			/* i1 o32 filter */
		}
		else if (nOutputPlanes == 1 && nInputPlanes == 128)
		{
			/* i128 o32 filter */
		}
		else if (nOutputPlanes == 32 && nInputPlanes == 3)
		{
			/* i3 o32 filter */
		}
		else if (nOutputPlanes == 3 && nInputPlanes == 128)
		{
			/* i128 o3 filter */
		}
		else
		{
			if (nInputPlanes & 1)
			{
				cl_available = false;
				cuda_available = false;
				avx_available = false;
			}

			if (nOutputPlanes & 31)
			{
				cl_available = false;
				cuda_available = false;
				avx_available = false;
			}

			if (nInputPlanes == 32 || nInputPlanes == 64 || nInputPlanes == 128)
			{
				/* ok */
			}
			else
			{
				cuda_available = false;
			}
		}

		const struct W2XConvProcessor *proc = conv->target_processor;

		if ((cl_available && proc->type == W2XCONV_PROC_OPENCL) ||
			(cuda_available && proc->type == W2XCONV_PROC_CUDA) ||
			(avx_available && proc->type == W2XCONV_PROC_HOST))
		{
			ret = filter_AVX_OpenCL(conv, env, packed_input_buf, packed_output_buf, size);
		}
		else
		{
			ret = filter_CV(env, packed_input_buf, packed_output_buf, size);
		}

		return ret;
	}
}
