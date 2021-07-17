/******************************************************************************
   The MIT License (MIT)

   Copyright (c) <2014> <Michal Drobot>
   Copyright Epic Games, Inc. All Rights Reserved.
   Copyright (c) Grzegorz Slazinski. All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
/******************************************************************************/
Flt AcosFast(Flt cos) // 'cos'=-1..1, returns [0, PI]
{
   Flt x  =Abs(cos);
   Flt res=(-0.156583*x+PI_2)*Sqrt(1-x);
   return (cos>=0) ? res : PI-res;
}
/******************************************************************************/
Flt AsinFast(Flt sin) // 'sin'=-1..1, returns [-PI/2, PI/2]
{
   Flt x  =Abs(sin);
   Flt res=(-0.156583*x+PI_2)*Sqrt(1-x);
   return (sin>=0) ? PI_2-res : res-PI_2;
}
/******************************************************************************/
