/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *lastduel_vram,*lastduel_scroll2,*lastduel_scroll1;
static struct osd_bitmap *scroll1_bitmap,*scroll2_bitmap;
static unsigned char *scroll1_dirty,*scroll2_dirty;
static int scroll[16];

void lastduel_scroll_w( int offset, int data )
{
	scroll[offset]=data&0xffff;  /* Scroll data, other bits unknown */
}

int lastduel_scroll1_r(int offset)
{
	return READ_WORD(&lastduel_scroll1[offset]);
}

void lastduel_scroll1_w(int offset,int value)
{
	int oldword = READ_WORD(&lastduel_scroll1[offset]);
	int newword = COMBINE_WORD(oldword,value);

	if (oldword != newword)
	{
		WRITE_WORD(&lastduel_scroll1[offset],value);
		scroll1_dirty[offset/4]=1;
	}
}

int lastduel_scroll2_r(int offset)
{
	return READ_WORD(&lastduel_scroll2[offset]);
}

void lastduel_scroll2_w(int offset,int value)
{
	int oldword = READ_WORD(&lastduel_scroll2[offset]);
	int newword = COMBINE_WORD(oldword,value);

	if (oldword != newword)
	{
		WRITE_WORD(&lastduel_scroll2[offset],value);
		scroll2_dirty[offset/4]=1;
	}
}

int lastduel_vram_r(int offset)
{
	return READ_WORD(&lastduel_vram[offset]);
}

void lastduel_vram_w(int offset,int value)
{
 	COMBINE_WORD_MEM(&lastduel_vram[offset],value);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int lastduel_vh_start(void)
{
	scroll1_bitmap=osd_create_bitmap(1024,1024);
	scroll2_bitmap=osd_create_bitmap(1024,1024);
	scroll1_dirty=(unsigned char *)malloc(0x1000);
	scroll2_dirty=(unsigned char *)malloc(0x1000);

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void lastduel_vh_stop(void)
{
	osd_free_bitmap(scroll1_bitmap);
	osd_free_bitmap(scroll2_bitmap);
	free(scroll1_dirty);
	free(scroll2_dirty);
}

/***************************************************************************/

void lastduel_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs,color,code,tile;
	int mx,my;
	int colmask[16];
	unsigned int *pen_usage; /* Save some struct derefs */

	/* Build the dynamic palette */
	palette_init_used_colors();

	/* Text layer colours */
	pen_usage= Machine->gfx[1]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		tile=READ_WORD (&lastduel_vram[offs]);
		color = ((tile & 0xf000) >> 12);
		tile=tile&0xfff;

		colmask[color] |= pen_usage[tile];
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[192*4 + 4 * color +3] = PALETTE_COLOR_TRANSPARENT;
		for (i = 0;i < 3;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[192*4 + 4 * color + i] = PALETTE_COLOR_USED;
		}
	}

	pen_usage= Machine->gfx[3]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0x4000 - 4;offs >= 0;offs -= 4)
	{
		color=READ_WORD(&lastduel_scroll1[offs+2])&0xf;
		code=READ_WORD(&lastduel_scroll1[offs])&0x1fff;

		colmask[color] |= pen_usage[code];
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[16*16 + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[16*16 + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	pen_usage= Machine->gfx[2]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0x4000 - 4;offs >= 0;offs -= 4)
	{
		color=READ_WORD(&lastduel_scroll2[offs+2])&0xf;
		code=READ_WORD(&lastduel_scroll2[offs])&0xfff;

		colmask[color] |= pen_usage[code];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	pen_usage= Machine->gfx[0]->pen_usage;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for(offs=0x500-8;offs>-1;offs-=8)
	{
		int attributes = READ_WORD(&spriteram[offs+2]);
		code=READ_WORD(&spriteram[offs]);
		color = attributes&0xf;

		colmask[color] |= pen_usage[code];
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[32*16 + 16 * color +15] = PALETTE_COLOR_TRANSPARENT;
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[32*16 + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Check for complete remap and redirty if needed */
	if (palette_recalc())
	{
		memset(scroll2_dirty,1,0x1000);
		memset(scroll1_dirty,1,0x1000);
	}

	/* Draw foreground layer  */
	mx=-1; my=0;
	for (offs = 0 ;offs < 0x4000; offs += 4)
	{
		mx++;
		if (mx==64) {mx=0; my++;}

		if (!scroll1_dirty[offs/4]) continue;
		scroll1_dirty[offs/4]=0;

		color=READ_WORD(&lastduel_scroll1[offs+2]);
		code=READ_WORD(&lastduel_scroll1[offs]);

		drawgfx(scroll1_bitmap,Machine->gfx[3],code&0x1fff,color&0xf,color&0x20,color&0x40,mx*16,my*16,0,TRANSPARENCY_NONE,0);
	}

	/* Background layer  */
	mx=-1; my=0;
	for (offs = 0 ;offs < 0x4000; offs += 4)
	{
		mx++;
		if (mx==64) {mx=0; my++;}

		if (!scroll2_dirty[offs/4]) continue;
		scroll2_dirty[offs/4]=0;

		color=READ_WORD(&lastduel_scroll2[offs+2]);
		code=READ_WORD(&lastduel_scroll2[offs]);

		drawgfx(scroll2_bitmap,Machine->gfx[2],code&0xfff,color&0xf,color&0x20,color&0x40,mx*16,my*16,0,TRANSPARENCY_NONE,0);
	}

	{
		int scrollx,scrolly;

		scrollx=-scroll[6];
		scrolly=-scroll[4];
		copyscrollbitmap(bitmap,scroll2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		scrollx=-scroll[2];
		scrolly=-scroll[0];
		copyscrollbitmap(bitmap,scroll1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	/* Sprites */
	for(offs=0x500-8;offs>=0;offs-=8)
	{
		int attributes,sy,sx,flipx,flipy;
		code=READ_WORD(&spriteram[offs]);
		if (!code) continue;

		attributes = READ_WORD(&spriteram[offs+2]);
		sy = READ_WORD(&spriteram[offs+4]) & 0x1ff;
		sx = READ_WORD(&spriteram[offs+6]) & 0x1ff;

		flipx = attributes&0x20;
		flipy = attributes&0x40;
		color = attributes&0xf;

		if( sy>0x100 )
			sy -= 0x200;

		drawgfx(bitmap,Machine->gfx[0],
			code,
			color,
			flipx,flipy,
			sx,sy,
			&Machine->drv->visible_area,
			TRANSPARENCY_PEN,15);
	}

	/* Text tiles */
	mx=-1; my=0;
	for (offs = 0 ;offs < 0x1000; offs += 2)
	{
		mx++;
		if (mx==64) {mx=0; my++;}

		tile=READ_WORD (&lastduel_vram[offs]);

		color = ((tile & 0xf000) >> 12);
		tile=tile&0xfff;

		if (tile==0x20) continue; /* Transparent tile */

 		drawgfx(bitmap,Machine->gfx[1],tile,
			color, 0,0, 8*mx,8*my,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
	}
}
