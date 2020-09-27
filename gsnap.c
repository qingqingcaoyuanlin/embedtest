/*
 * File:    gsnap.c
 * Author:  Li XianJing <xianjimli@hotmail.com>
 * Brief:   snap the linux mobile device screen.
 *
 * Copyright (c) 2009  Li XianJing <xianjimli@hotmail.com>
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * History:
 * ================================================================
 * 2009-08-20 Li XianJing <xianjimli@hotmail.com> created
 * 2011-02-28 Li XianJing <xianjimli@hotmail.com> suppport RGB888 framebuffer.
 * 2011-04-09 Li XianJing <xianjimli@hotmail.com> merge figofuture's png output.
 * 	ref: http://blog.chinaunix.net/space.php?uid=15059847&do=blog&cuid=2040565
 *
 */

#include <png.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

#define Screen_Dev "/dev/fb0"


struct _FBInfo;
typedef struct _FBInfo FBInfo;
typedef int (*UnpackPixel)(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b);

struct _FBInfo
{
	int fd;
	UnpackPixel unpack;
	unsigned char *bits;
	struct fb_fix_screeninfo fi;
	struct fb_var_screeninfo vi;
};

#define fb_width(fb)  ((fb)->vi.xres)
#define fb_height(fb) ((fb)->vi.yres)
#define fb_bpp(fb)    ((fb)->vi.bits_per_pixel>>3)
#define fb_size(fb)   ((fb)->vi.xres * (fb)->vi.yres * fb_bpp(fb))

static int fb_unpack_rgb565(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	unsigned short color = *(unsigned short*)pixel;

	*r = ((color >> 11) & 0xff) << 3;
	*g = ((color >> 5) & 0xff)  << 2;
	*b = (color & 0xff )<< 3;

	return 0;
}

static int fb_unpack_rgb24(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = pixel[fb->vi.red.offset>>3];
	*g = pixel[fb->vi.green.offset>>3];
	*b = pixel[fb->vi.blue.offset>>3];

	return 0;
}

static int fb_unpack_argb32(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = pixel[fb->vi.red.offset>>3];
	*g = pixel[fb->vi.green.offset>>3];
	*b = pixel[fb->vi.blue.offset>>3];

	return 0;
}

static int fb_unpack_none(FBInfo* fb, unsigned char* pixel, 
	unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = *g = *b = 0;

	return 0;
}

static void set_pixel_unpacker(FBInfo* fb)
{
	if(fb_bpp(fb) == 2)
	{
		fb->unpack = fb_unpack_rgb565;
	}
	else if(fb_bpp(fb) == 3)
	{
		fb->unpack = fb_unpack_rgb24;
	}
	else if(fb_bpp(fb) == 4)
	{
		fb->unpack = fb_unpack_argb32;
	}
	else
	{
		fb->unpack = fb_unpack_none;
		printf("%s: not supported format.\n", __func__);
	}

	return;
}

static int fb_open(FBInfo* fb, const char* fbfilename)
{
	fb->fd = open(fbfilename, O_RDWR);

	if (fb->fd < 0)
	{
		fprintf(stderr, "can't open %s\n", fbfilename);

		return -1;
	}

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fi) < 0)
		goto fail;

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vi) < 0)
		goto fail;

	fb->bits = mmap(0, fb_size(fb), PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);

	if (fb->bits == MAP_FAILED)
		goto fail;

	printf("---------------framebuffer---------------\n");
	printf("%s: \n  width : %8d\n  height: %8d\n  bpp   : %8d\n  r(%2d, %2d)\n  g(%2d, %2d)\n  b(%2d, %2d)\n",
		fbfilename, fb_width(fb), fb_height(fb), fb_bpp(fb), 
		fb->vi.red.offset, fb->vi.red.length,
		fb->vi.green.offset, fb->vi.green.length,
		fb->vi.blue.offset, fb->vi.blue.length);
	printf("-----------------------------------------\n");

	set_pixel_unpacker(fb);

	return 0;

fail:
	printf("%s is not a framebuffer.\n", fbfilename);
	close(fb->fd);

	return -1;
}

static void fb_close(FBInfo* fb)
{
	munmap(fb->bits, fb_size(fb));
	close(fb->fd);

	return;
}

static int snap2buffer(int quality, FBInfo* fb, unsigned char* buffer)
{
	int row_stride = 0; 
	unsigned char* outbuffer = buffer;
	JSAMPROW row_pointer[1] = {0};
	struct jpeg_error_mgr jerr;
	struct jpeg_compress_struct cinfo;

	memset(&jerr, 0x00, sizeof(jerr));
	memset(&cinfo, 0x00, sizeof(cinfo));

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);



    jpeg_mem_dest(&cinfo,&outbuffer, strlen(outbuffer));//---------outsize
	cinfo.image_width = fb_width(fb);
	cinfo.image_height = fb_height(fb);
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	row_stride = fb_width(fb) * 2;
	JSAMPLE* image_buffer = malloc(3 * fb_width(fb));

	while (cinfo.next_scanline < cinfo.image_height) 
	{
		int i = 0;
		int offset = 0;
		unsigned char* line = fb->bits + cinfo.next_scanline * fb_width(fb) * fb_bpp(fb);

		for(i = 0; i < fb_width(fb); i++, offset += 3, line += fb_bpp(fb))
		{
			fb->unpack(fb, line, image_buffer+offset, image_buffer + offset + 1, image_buffer + offset + 2);
		}

		row_pointer[0] = image_buffer;
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);

	return 0;
}


int snap(unsigned char* buffer)
{
    FBInfo fb;
	
    memset(&fb, 0x00, sizeof(fb));


    if (fb_open(&fb, Screen_Dev) != 0)
	{
		return -1;					
	}
	else
	{
		snap2buffer(100, &fb, buffer);	
	}
	fb_close(&fb);
    return 0;
    

}


