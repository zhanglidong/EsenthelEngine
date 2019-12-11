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

#ifndef MODEL_HANDLER_HPP_
#define MODEL_HANDLER_HPP_

#include <iostream>
#include <memory>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include "Buffer.hpp"
#include "filters.hpp"
#include "cvwrap.hpp"

namespace w2xc
{
	struct Model
	{
		// getter function
	   int  getNInputPlanes()const {return  nInputPlanes;}
	   int getNOutputPlanes()const {return nOutputPlanes;}

		// public operation function
		bool filter
		(
			W2XConv *conv,
			ComputeEnv *env,
			Buffer *packed_input,
			Buffer *packed_output,
			const W2Size &size
		);

      // ESENTHEL CHANGED
      struct Weight
      {
         float w[3][3];

                                 float& at (int y, int x) {return w[y][x];}
         template<typename TYPE> float* ptr(int y       ) {return w[y];}
      };
      Weight *weights;
      float  *biases;   int biases_size()const {return nOutputPlanes;}
		int nInputPlanes;
		int nOutputPlanes;
		int kernelSize;

	private:
		// thread worker function
		bool filterWorker
		(
			std::vector<W2Mat> &inputPlanes,
			std::vector<W2Mat> &weightMatrices,
			std::vector<W2Mat> &outputPlanes,
			unsigned int beginningIndex,
			unsigned int nWorks
		);

		bool filter_CV(
			ComputeEnv *env,
			Buffer *packed_input,
			Buffer *packed_output,
			const W2Size &size
		);

		bool filter_AVX_OpenCL(
			W2XConv *conv,
			ComputeEnv *env,
			Buffer *packed_input,
			Buffer *packed_output,
			const W2Size &size
		);
	};
}

#endif /* MODEL_HANDLER_HPP_ */
