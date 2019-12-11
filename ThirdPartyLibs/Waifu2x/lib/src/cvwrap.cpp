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

#include <stdlib.h>
#include <string.h>
#include "cvwrap.hpp"

namespace w2xc
{

W2Mat::~W2Mat()
{
	if (data_owner)
	{
		data_owner=false;
		free(data);
		data = nullptr;
	}
}


W2Mat::W2Mat()
	: data_owner(false),
	data(nullptr),
	data_byte_width(0),
	data_height(0),
	view_top(0),
	view_left(0),
	view_width(0),
	view_height(0)
{
}

W2Mat::W2Mat(int width, int height, int type)
	: data_owner(true),
	data_byte_width(width*CV_ELEM_SIZE(type)),
	data_height(height),
	view_top(0),
	view_left(0),
	view_width(width),
	view_height(height),
	type(type)
{
	this->data = (char*)calloc(height, data_byte_width);
}

/*W2Mat::W2Mat(int width, int height, int type, void *data, int data_step)
	: data_owner(true),
	data_byte_width(data_step),
	data_height(height),
	view_top(0),
	view_left(0),
	view_width(width),
	view_height(height),
	type(type)
{
	this->data = (char*)calloc(height, data_byte_width);
	memcpy(this->data, data, height * data_byte_width);
}*/
W2Mat::W2Mat(int width, int height, int type, void *data, int data_step) // ESENTHEL CHANGED
	: data_owner(false),
   data((char*)data),
	data_byte_width(data_step),
	data_height(height),
	view_top(0),
	view_left(0),
	view_width(width),
	view_height(height),
	type(type)
{
}


W2Mat & W2Mat::operator=(W2Mat &&rhs)
{
    this->data_owner = rhs.data_owner;
    this->data = rhs.data;
    this->data_byte_width = rhs.data_byte_width;
    this->data_height = rhs.data_height;
    this->view_top = rhs.view_top;
    this->view_left = rhs.view_left;
    this->view_width = rhs.view_width;
    this->view_height= rhs.view_height;
    this->type = rhs.type;

    rhs.data_owner = false;
    rhs.data = NULL;

    return *this;
}

W2Mat::W2Mat(const W2Mat & rhs, int view_left_offset, int view_top_offset, int view_width, int view_height)
{	
    this->data_owner = false;
    this->data = rhs.data;
    this->data_byte_width = rhs.data_byte_width;
    this->data_height = rhs.data_height;

    this->view_left = rhs.view_left + view_left_offset;
    this->view_top = rhs.view_top + view_top_offset;
    this->view_width = view_width;
    this->view_height = view_height;

    this->type = rhs.type;
}

void W2Mat::copyTo(W2Mat* target)
{
	W2Mat ret(this->view_width, this->view_height, this->type);

	int elem_size = CV_ELEM_SIZE(this->type);
	int w = this->view_width;
	int h = this->view_height;

	for (int yi = 0; yi < h; yi++)
	{
		char *out = ret.ptr<char>(yi);
		char *in = this->ptr<char>(yi);

		memcpy(out, in, w * elem_size);
	}
	*target = std::move(ret);
}

}
