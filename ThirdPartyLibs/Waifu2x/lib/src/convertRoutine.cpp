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

#include <limits.h>
#include <cstring>
#include "convertRoutine.hpp"
#include "common.hpp"
#include "Buffer.hpp"
#include "w2xconv.h"

namespace w2xc
{

	// converting process inside program
	static bool convertWithModelsBasic
	(
		W2XConv *conv,
		ComputeEnv *env,
		W2Mat &inputPlane, W2Mat &outputPlane,
		Buffer *input_buf, Buffer *output_buf,
		std::vector<Model> &models,
		enum image_format fmt
	);

	static bool convertWithModelsBlockSplit
	(
		W2XConv *conv,
		ComputeEnv *env,
		W2Mat &inputPlane,
		W2Mat &outputPlane, std::vector<Model> &models,
		int blockSize,
		enum image_format fmt,
      bool clamp
	);

	bool convertWithModels
	(
		W2XConv *conv,
		ComputeEnv *env,
		W2Mat &inputPlane, W2Mat &outputPlane,
		std::vector<Model> &models,
		int blockSize,
		enum image_format fmt,
      bool clamp
   )
	{
		return convertWithModelsBlockSplit(conv, env, inputPlane, outputPlane, models, blockSize, fmt, clamp);
	}

	static bool convertWithModelsBasic
	(
		W2XConv *conv,
		ComputeEnv *env,
		W2Mat &inputPlane, W2Mat &outputPlane,
		Buffer *packed_input_buf,
		Buffer *packed_output_buf,
		std::vector<Model> &models,
		enum image_format fmt
	)
	{
		// padding is require before calling this function

		std::vector<W2Mat> inputPlanes;
		inputPlanes.emplace_back(W2Mat(inputPlane,0,0,0,0));

		W2Size filterSize(inputPlane.view_width, inputPlane.view_height);
		int filterWidth = filterSize.width;
		int filterHeight = filterSize.height;

		float *packed_input = (float*)packed_input_buf->get_write_ptr_host(env);

		switch (fmt) {
			case IMAGE_BGR:
			{
				pack_mat_bgr(packed_input, inputPlane, filterWidth, filterHeight);
				break;
			}
			case IMAGE_RGB:
			{
				pack_mat_rgb(packed_input, inputPlane, filterWidth, filterHeight);
				break;
			}
			case IMAGE_RGB_F32:
			{
				pack_mat_rgb_f32(packed_input, inputPlane, filterWidth, filterHeight);
				break;
			}
			case IMAGE_Y:
			{
				pack_mat(packed_input, inputPlanes, filterWidth, filterHeight, 1);
				break;
			}
		}

		for (int index = 0; index < (int)models.size(); index++)
		{
			int nOutputPlanes = models[index].getNOutputPlanes();
			int nInputPlanes = models[index].getNInputPlanes();

			if (!models[index].filter(conv, env, packed_input_buf, packed_output_buf, filterSize))
			{
				std::exit(-1);
			}
			
			std::swap(packed_input_buf, packed_output_buf);
		}

		if (IS_3CHANNEL(fmt))
		{
			packed_input = (float*)packed_input_buf->get_read_ptr_host(env, sizeof(float)*filterWidth*filterHeight*3);
		}
		else
		{
			packed_input = (float*)packed_input_buf->get_read_ptr_host(env, sizeof(float)*filterWidth*filterHeight);
		}

		switch (fmt) {
			case IMAGE_BGR:
			{
				outputPlane = W2Mat(filterSize.width*3, filterSize.height, 1);
				unpack_mat_bgr(outputPlane, packed_input, filterWidth, filterHeight);
				break;
			}
			case IMAGE_RGB:
			{
				outputPlane = W2Mat(filterSize.width*3, filterSize.height, 1);
				unpack_mat_rgb(outputPlane, packed_input, filterWidth, filterHeight);
				break;
			}
			case IMAGE_RGB_F32:
			{
				outputPlane = W2Mat(filterSize.width*3, filterSize.height, 4);
				unpack_mat_rgb_f32(outputPlane, packed_input, filterWidth, filterHeight);
				break;
			}
			case IMAGE_Y:
			{
				outputPlane = W2Mat(filterSize.width*1, filterSize.height, 4);
				unpack_mat1(outputPlane, packed_input, filterWidth, filterHeight);
				break;
			}
		}

		return true;
	}

   static constexpr int Mid(int x, int min, int max) {return (x>=max) ? max : (x<=min) ? min : x;}
   static constexpr int Mod(int x, int y           ) {int z=x%y; return (z>=0) ? z : z+y;}

	static bool convertWithModelsBlockSplit
	(
		W2XConv *conv,
		ComputeEnv *env,
		W2Mat &inputPlane_2,
		W2Mat &outputPlane_2,
		std::vector<Model> &models,
		int blockSize,
		enum image_format fmt,
      bool clamp
	)
	{
		// padding is not required before calling this function
		// initialize local variables
		int nModel = (int) models.size();

		//insert padding to inputPlane
		int tempWidth = (int) inputPlane_2.view_width + nModel*2;
		int tempHeight = (int) inputPlane_2.view_height + nModel*2;
		int inputWidth = inputPlane_2.view_width;
		int inputHeight = inputPlane_2.view_height;

		W2Mat tempMat_2(tempWidth, tempHeight, inputPlane_2.type);
		int elem_size = CV_ELEM_SIZE(inputPlane_2.type);

      for(int y=0; y<tempHeight; y++)
      {
         int    ys=y-nModel;
         char *dst=   tempMat_2.ptr<char>(y),
              *src=inputPlane_2.ptr<char>(clamp ? Mid(ys, 0, inputHeight-1) : Mod(ys, inputHeight));
         for(int x=0; x<tempWidth ; x++)
         {
            int      xs=x-nModel;
            char *src_x=src+elem_size*(clamp ? Mid(xs, 0, inputWidth-1) : Mod(xs, inputWidth));
            memcpy(dst, src_x, elem_size);
            dst+=elem_size;
         }
      }

		if (blockSize == 0)
		{
			blockSize = env->pref_block_size;
		}

		Buffer *input_buf, *output_buf;
		int ok_count = 0;

		while (true)
		{
			long long max_size = 0;

			int width = (std::min)(tempMat_2.view_width, blockSize);
			int height = (std::min)(tempMat_2.view_height, blockSize);

			for (int index = 0; index < (int)models.size(); index++)
			{
				long long bufsize =
					(long long)sizeof(float) *
					(long long)width *
					(long long)height *
					(long long)models[index].getNOutputPlanes();

				max_size = (std::max)(max_size, (long long)bufsize);
			}

			if ((sizeof(void*)==4) && max_size >= INT_MAX)
			{
				/* pass */
			}
			else
			{
				input_buf = new Buffer(env, max_size);
				output_buf = new Buffer(env, max_size);

				if (input_buf->prealloc(conv, env) && output_buf->prealloc(conv, env))
				{
					break;
				}

				delete input_buf;
				delete output_buf;
			}

			blockSize /= 2;
			
			if (blockSize == 0)
			{
				abort();
			}
		}

		int blockWidth = (std::min)(blockSize, tempMat_2.view_width);
		int blockHeight = (std::min)(blockSize, tempMat_2.view_height);
		int clipWidth = blockWidth - 2*nModel;
		int clipHeight = blockHeight - 2*nModel;

		// calcurate split rows/cols
		unsigned int splitColumns = (inputWidth + (clipWidth-1)) / clipWidth;
		unsigned int splitRows = (inputHeight + (clipHeight-1)) / clipHeight;

		/*switch (fmt) ESENTHEL CHANGED don't allocate a temporary, but use what we've requested (assumes type matches)
		{
			case IMAGE_BGR:
			case IMAGE_RGB:
			{
				outputPlane_2 = W2Mat(inputWidth, inputHeight, CV_8UC3);
				break;
			}
			case IMAGE_RGB_F32:
			{
				outputPlane_2 = W2Mat(inputWidth, inputHeight, CV_32FC3);
				break;
			}
			case IMAGE_Y:
			{
				outputPlane_2 = W2Mat(inputWidth, inputHeight, CV_32FC1);
				break;
			}
			default:
			{
				abort();
			}
		}*/

		for (unsigned int r = 0; r < splitRows; r++)
		{
			int clipStartY = r * clipHeight;
			int clipEndY = 0;

			if (r == splitRows - 1)
			{
				clipEndY = tempMat_2.view_height;
			}
			else
			{
				clipEndY = r * clipHeight + blockHeight;
			}

			for (unsigned int c = 0; c < splitColumns; c++)
			{
				// start to convert
				W2Mat processBlockOutput;

				int clipStartX = c * clipWidth;
				int clipEndX = 0;

				if (c == splitColumns - 1)
				{
					clipEndX = tempMat_2.view_width;
				}
				else 
				{
					clipEndX = c * (blockWidth - 2 * nModel) + blockWidth;
				}

				int curBlockWidth = clipEndX - clipStartX;
				int curBlockHeight = clipEndY - clipStartY;
				
				W2Mat processBlock(tempMat_2, clipStartX, clipStartY, curBlockWidth, curBlockHeight);

				int elemSize = 0;

				switch (fmt)
				{
					case IMAGE_BGR:
					case IMAGE_RGB:
					{
						elemSize = 3;
						break;
					}
					case IMAGE_RGB_F32:
					{
						elemSize = 12;
						break;
					}
					case IMAGE_Y:
					{
						elemSize = 4;
						break;
					}
				}

				if (!convertWithModelsBasic
					(
						conv,
						env,
						processBlock,
						processBlockOutput,
						input_buf,
						output_buf,
						models,
						fmt
					)
				)return false;

				int srcStartY = nModel;
				int srcStartX = nModel;

				int dstStartY = r * (blockHeight - 2*nModel);
				int dstStartX = c * (blockWidth - 2*nModel);
				int copyWidth = curBlockWidth - (nModel * 2);
				int copyHeight = curBlockHeight - (nModel * 2);

				for (int yi=0; yi<copyHeight; yi++)
				{
					char *src = processBlockOutput.ptr<char>(yi + srcStartY);
					char *dst = outputPlane_2.ptr<char>(yi + dstStartY);

					src += srcStartX * elemSize;
					dst += dstStartX * elemSize;

					memcpy(dst, src, copyWidth * elemSize);
				}
			} // end process 1 column

		} // end process all blocks

		delete input_buf;
		delete output_buf;

		return true;
	}
}

