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

#include "Buffer.hpp"
#include "w2xconv.h"

namespace w2xc
{

Buffer::Buffer(ComputeEnv *env, size_t byte_size) : env(env), byte_size(byte_size), last_write(Processor::EMPTY, 0)
{
    int num_cl_dev = env->num_cl_dev;
    int num_cuda_dev = env->num_cuda_dev;

#ifdef CLLIB_H
    cl_ptr_list = new cl_mem[num_cl_dev];
    cl_valid_list = new bool[num_cl_dev];
#endif

#ifdef HAVE_CUDA
    cuda_ptr_list = new CUdeviceptr[num_cuda_dev];
    cuda_valid_list = new bool[num_cuda_dev];
#endif

    clear(env);
}

Buffer::~Buffer()
{
    release(env);

#ifdef CLLIB_H
    delete [] cl_ptr_list;
    delete [] cl_valid_list;
#endif
#ifdef HAVE_CUDA
    delete [] cuda_ptr_list;
    delete [] cuda_valid_list;
#endif
}

void Buffer::clear(ComputeEnv *env)
{
   int num_cl_dev = env->num_cl_dev;
   int num_cuda_dev = env->num_cuda_dev;

#ifdef CLLIB_H
   for (int i=0; i<num_cl_dev; i++)
   {
        cl_valid_list[i] = false;
        cl_ptr_list[i] = nullptr;
   }
#endif
#ifdef HAVE_CUDA
   for (int i=0; i<num_cuda_dev; i++)
	{
      cuda_valid_list[i] = false;
      cuda_ptr_list[i] = 0;
   }
#endif

    host_valid = false;
    host_ptr = nullptr;
}


void Buffer::release(ComputeEnv *env)
{
    int num_cl_dev = env->num_cl_dev;
    int num_cuda_dev = env->num_cuda_dev;

#ifdef CLLIB_H
   for (int i=0; i<num_cl_dev; i++)
	{
      if (cl_ptr_list[i])
      {
         clReleaseMemObject(cl_ptr_list[i]);
         cl_ptr_list[i] = nullptr;
      }
      cl_valid_list[i] = false;
    }
#endif
#ifdef HAVE_CUDA
   for(int i=0; i<num_cuda_dev; i++)
	{
      if(cuda_ptr_list[i])
      {
         cuMemFree(cuda_ptr_list[i]);
         cuda_ptr_list[i] = nullptr;
      }
      cuda_valid_list[i] = false;
   }
#endif

   if (host_ptr)
   {
      w2xc_aligned_free(host_ptr);
      host_ptr = nullptr;
   }
   host_valid = false;
}

void Buffer::invalidate(ComputeEnv *env)
{
    int num_cl_dev = env->num_cl_dev;
    int num_cuda_dev = env->num_cuda_dev;

#ifdef CLLIB_H
   for (int i=0; i<num_cl_dev; i++)cl_valid_list[i] = false;
#endif
#ifdef HAVE_CUDA
   for (int i=0; i<num_cuda_dev; i++)cuda_valid_list[i] = false;
#endif
   host_valid = false;
}
#ifdef CLLIB_H
cl_mem Buffer::get_read_ptr_cl(ComputeEnv *env,int devid, size_t read_byte_size)
{
    if (cl_valid_list[devid])
	{
        return cl_ptr_list[devid];
    }

    if (host_valid == false)
	{
        /* xx */
        abort();
        return nullptr;
    }

    OpenCLDev *dev = &env->cl_dev_list[devid];
	
    if (cl_ptr_list[devid] == nullptr)
	{
        cl_int err;
        cl_ptr_list[devid] = clCreateBuffer(dev->context, CL_MEM_READ_WRITE, byte_size, nullptr, &err);
    }
	
    clEnqueueWriteBuffer(dev->queue, cl_ptr_list[devid], CL_TRUE, 0, read_byte_size, host_ptr, 0, nullptr, nullptr);
    cl_valid_list[devid] = true;
	
    return cl_ptr_list[devid];
}
cl_mem Buffer::get_write_ptr_cl(ComputeEnv *env,int devid)
{
    invalidate(env);
    OpenCLDev *dev = &env->cl_dev_list[devid];
	
    if (cl_ptr_list[devid] == nullptr)
	{
        cl_int err;
        cl_ptr_list[devid] = clCreateBuffer(dev->context, CL_MEM_READ_WRITE, byte_size, nullptr, &err);
    }

    last_write.type = Processor::OpenCL;
    last_write.devid = devid;

    cl_valid_list[devid] = true;
    return cl_ptr_list[devid];
}
#endif

#ifdef HAVE_CUDA
CUdeviceptr Buffer::get_read_ptr_cuda(ComputeEnv *env,int devid, size_t read_byte_size)
{
    if (cuda_valid_list[devid])
	{
        return cuda_ptr_list[devid];
    }

    if (host_valid == false)
	{
        /* xx */
        abort();
        return 0;
    }

    CUDADev *dev = &env->cuda_dev_list[devid];
    cuCtxPushCurrent(dev->context);

    if (cuda_ptr_list[devid] == 0)
	{
        CUresult err;
        err = cuMemAlloc(&cuda_ptr_list[devid], byte_size);
        if (err != CUDA_SUCCESS)
		{
            abort();
        }
    }

    cuMemcpyHtoD(cuda_ptr_list[devid], host_ptr, read_byte_size);
    cuda_valid_list[devid] = true;
    CUcontext old;
    cuCtxPopCurrent(&old);

    return cuda_ptr_list[devid];
}

CUdeviceptr Buffer::get_write_ptr_cuda(ComputeEnv *env,int devid)
{
    invalidate(env);

    CUDADev *dev = &env->cuda_dev_list[devid];
    cuCtxPushCurrent(dev->context);

    if (cuda_ptr_list[devid] == 0)
	{
        CUresult err;
        err = cuMemAlloc(&cuda_ptr_list[devid], byte_size);
        if (err != CUDA_SUCCESS)
		{
            abort();
        }
    }

    last_write.type = Processor::CUDA;
    last_write.devid = devid;

    cuda_valid_list[devid] = true;
    CUcontext old;
    cuCtxPopCurrent(&old);

    return cuda_ptr_list[devid];
}
#endif

void * Buffer::get_read_ptr_host(ComputeEnv *env, size_t read_byte_size)
{
   if (host_valid)
      return host_ptr;

   if (host_ptr == nullptr)
      host_ptr = w2xc_aligned_malloc(byte_size, 64);

#ifdef CLLIB_H
   if (last_write.type == Processor::OpenCL)
	{
      OpenCLDev *dev = &env->cl_dev_list[last_write.devid];
      clEnqueueReadBuffer(dev->queue, cl_ptr_list[last_write.devid], CL_TRUE, 0, read_byte_size, host_ptr, 0, nullptr, nullptr);
   }else
#endif   
#ifdef HAVE_CUDA
   if (last_write.type == Processor::CUDA)
	{
      CUDADev *dev = &env->cuda_dev_list[last_write.devid];
      cuCtxPushCurrent(dev->context);
      cuMemcpyDtoH(host_ptr, cuda_ptr_list[last_write.devid], read_byte_size);

      CUcontext old;
      cuCtxPopCurrent(&old);
   }else
#endif
	{
      abort();
   }

   host_valid = true;
   return host_ptr;
}


bool Buffer::prealloc(W2XConv *conv, ComputeEnv *env)
{
    if (host_ptr == nullptr)
	{
        host_ptr = w2xc_aligned_malloc(byte_size, 64);
        if (host_ptr == nullptr)
		{
            return false;
        }
    }

    switch (conv->target_processor->type)
	{
		case W2XCONV_PROC_HOST:
		{
			break;
		}
#ifdef CLLIB_H
		case W2XCONV_PROC_OPENCL:
		{
			// xx
			//devid = conv->target_processor->dev_id;
			int devid = 0;
			if (cl_ptr_list[devid] == nullptr)
			{
				cl_int err;
				OpenCLDev *dev = &env->cl_dev_list[devid];
				cl_ptr_list[devid] = clCreateBuffer(dev->context, CL_MEM_READ_WRITE, byte_size, nullptr, &err);
				if (cl_ptr_list[devid] == nullptr)
				{
					return false;
				}

				/* touch memory to force allocation */
				char data = 0;
				err = clEnqueueWriteBuffer(dev->queue, cl_ptr_list[devid], CL_TRUE, 0, 1, &data, 0, nullptr, nullptr);
				if (err != CL_SUCCESS)
				{
					clReleaseMemObject(cl_ptr_list[devid]);
					cl_ptr_list[devid] = nullptr;
					return false;
				}

			}
		}break;
#endif
#ifdef HAVE_CUDA
		case W2XCONV_PROC_CUDA:
		{
			// xx
			// devid = conv->target_processor->dev_id;
			int devid = 0;

			if (cuda_ptr_list[devid] == 0)
			{
				CUresult err;
				CUDADev *dev = &env->cuda_dev_list[devid];
				cuCtxPushCurrent(dev->context);
				err = cuMemAlloc(&cuda_ptr_list[devid], byte_size);
				CUcontext old;
				cuCtxPopCurrent(&old);

				if (err != CUDA_SUCCESS)
				{
					return false;
				}
			}
		}break;
#endif
    }
	
    return true;
}

void * Buffer::get_write_ptr_host(ComputeEnv *env)
{
    invalidate(env);

    last_write.type = Processor::HOST;
    last_write.devid = 0;

    if (host_ptr == nullptr)
	{
        host_ptr = w2xc_aligned_malloc(byte_size, 64);
    }

    host_valid = true;

    return host_ptr;
}


}
