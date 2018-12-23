/*-
 * Copyright (c) 2010-2012, China Telecommunication Corporations
 * All rights reserved.
 *
 * Authors:
 * 		Qiong Sun, Chan Zhao, Bo Li, Daobiao Gong 
 * 		
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the China Telecommunication Corporations nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdlib.h>

static char bitmaparray[64] =
{
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
};

int bitmap_empty(char*bitmap,int size)
{
    int i =0;

    for (; i < size; i ++)
	{
		if (bitmap[i] != 0)
			return 0;
	}
	return 1;
}

int bitmap_full(char* bitmap,int size)
{
    int i=0;
    for(;i<size;i++)
	{
		if ((unsigned char)bitmap[i]  != 255)
			return 0;
	}
	return 1;

}

/* return value = 0,1,-1
 * -1 indicates failture
 */
int bitmap_get(char* bitmap,int bit)
{
	int n = bit/8 ;
        if((n<0) || (n>7))
           return -1;
	return bitmap[n] & bitmaparray[bit];
}

int bitmap_set(char* bitmap,int bit,int value)
{
	int n = bit / 8 ;
    if((n < 0) || (n > 7))
        return -1;
    if (value == 1)
    {
	   bitmap[n] = bitmap[n] | bitmaparray[bit];
	}
    else if (value ==0)
    {
       bitmap[n] = bitmap[n] & (~bitmaparray[bit]);
    }
	return 0;
}


int portrange_test(char *bitmap,int startbit,int endbit)
{
	int i = startbit;

	for(; i <= endbit; i ++)
	{
		if(bitmap_get(bitmap, i) !=0)
			return 0;
	}
	return 1;
} 

void portrange_set(char *bitmap, int startbit, int endbit, int value)
{
	int i = startbit;
	for(; i<= endbit; i ++)
	{
		bitmap_set(bitmap, i, value);
	}
} 