/***************************************************************************

	Prehistoric Isle video routines

	Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *pf1_bitmap,*pf2_bitmap;
static unsigned char *pf_video;
static unsigned char *pf_dirty;

static int vid_control[32];

static int offsetx;

/******************************************************************************/

void prehisle_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int offs,mx,my,color,tile,i;
	int colmask[0x80],code,pal_base;
	int scrollx,scrolly,quarter;

	unsigned char *tilemap = Machine->memory_region[4];

static int one_time=0;

//static int base=0;


	/* Build the dynamic palette */
	palette_init_used_colors();

	/* Text layer */
	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0;offs <0x800;offs += 2)
	{
		code = READ_WORD(&videoram[offs]);
		color=code>>12;
        //if (code==0xff20) continue;
		colmask[color] |= Machine->gfx[0]->pen_usage[code&0xfff];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Tiles - bottom layer */
	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (offs=0; offs<256; offs++)
		palette_used_colors[pal_base + offs] = PALETTE_COLOR_USED;

	/* Tiles - top layer */
	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0x0000;offs <0x4000;offs += 2 )
	{
		code = READ_WORD(&pf_video[offs]);
		color= code>>12;
		colmask[color] |= Machine->gfx[2]->pen_usage[code&0x7ff];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}

///* kludge */
palette_used_colors[pal_base + 16 * color +15] = PALETTE_COLOR_TRANSPARENT;
palette_change_color(pal_base + 16 * color +15 ,0,0,0);

	}

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0;offs <0x400;offs += 8 )
	{
		code = READ_WORD(&spriteram[offs+4])&0x1fff;
		color= READ_WORD(&spriteram[offs+6])>>12;
		if (code>0x13ff) code=0x13ff;
		colmask[color] |= Machine->gfx[3]->pen_usage[code];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc()) {
		memset(pf_dirty,1,0x4000);
		one_time=0;
	}


if (!one_time)
	/* Back layer, taken from tilemap rom */
	offsetx=0;
	for (quarter=0; quarter<8; quarter++)
	{
		my = -1;
		mx = 0;
		for (offs = 0x000+(quarter*0x800); offs < 0x800+(quarter*0x800);offs += 2)
		{
			my++;
			if (my == 32)
			{
				my = 0;
				mx++;
			}

			tile = tilemap[offs+1] + (tilemap[offs]<<8);
			color = tile>>12;
			drawgfx(pf1_bitmap,Machine->gfx[1],
	                    (tile & 0x7ff) | 0x800,
	                    color,
	                    tile&0x800,0,
	                    (16*mx)+offsetx,16*my,
	                    0,TRANSPARENCY_NONE,0);
	     }
		offsetx+=512;
	}

	scrollx=-READ_WORD(&vid_control[6]);
	scrolly=-READ_WORD(&vid_control[4]);
	copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

one_time=1;


	/* Front layer */
	offsetx=0;
	for (quarter=0; quarter<8; quarter++)
	{
		my = -1;
		mx = 0;
		for (offs = 0x000+(quarter*0x800); offs < 0x800+(quarter*0x800);offs += 2)
		{
			my++;
			if (my == 32)
			{
				my = 0;
				mx++;
			}
			if (!pf_dirty[offs]) continue;
			pf_dirty[offs]=0;
			tile = READ_WORD(&pf_video[offs]);
			color = tile>>12;
			drawgfx(pf2_bitmap,Machine->gfx[2],
	                    tile & 0x7ff,
	                    color,
	                    0,tile&0x800,
	                    (16*mx)+offsetx,16*my,
	                    0,TRANSPARENCY_NONE,0);
	     }
		offsetx+=512;
	}

	scrollx=-READ_WORD(&vid_control[2]);
	scrolly=-READ_WORD(&vid_control[0]);
	copyscrollbitmap(bitmap,pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Sprites */
	for (offs = 0;offs <0x800 ;offs += 8) {
		int x,y,sprite,sprite2,colour,fx,fy,extra;

	    y=READ_WORD (&spriteram[offs+0]);

//		if (y>256) continue; /* Speedup */
	    x=READ_WORD (&spriteram[offs+2]);
//		if (x&0x200) x-=0x200;


	    sprite=READ_WORD (&spriteram[offs+4]);
	    colour=READ_WORD (&spriteram[offs+6])>>12;

		fy=sprite&0x8000;
		fx=sprite&0x4000;

	    sprite=sprite&0x1fff;

		if (sprite>0x13ff) sprite=0x13ff;

		drawgfx(bitmap,Machine->gfx[3],
				sprite,
				colour,fx,fy,x,y,
				0,TRANSPARENCY_PEN,15);
	}

	/* Text layer */
	mx = -1;
	my = 0;
	for (offs = 0x000; offs < 0x800;offs += 2)
	{
		mx++;
		if (mx == 32)
		{
			mx = 0;
			my++;
		}
		tile = READ_WORD(&videoram[offs]);
		color = tile>>12;
		if ((tile&0xff)!=0x20)
			drawgfx(bitmap,Machine->gfx[0],
					tile & 0xfff,
					color,
					0,0,
					8*mx,8*my,
					0,TRANSPARENCY_PEN,15);
    }
}

/******************************************************************************/

int prehisle_vh_start (void)
{
	pf1_bitmap=osd_create_bitmap(4096,512);
	pf2_bitmap=osd_create_bitmap(4096,512);
	pf_video=malloc(0x4000);
	pf_dirty=malloc(0x4000);
	memset(pf_dirty,1,0x4000);
	return 0;
}

void prehisle_vh_stop (void)
{
	osd_free_bitmap(pf1_bitmap);
	osd_free_bitmap(pf2_bitmap);
	free(pf_video);
	free(pf_dirty);
}

void prehisle_video_w(int offset,int data)
{
	int oldword = READ_WORD(&pf_video[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&pf_video[offset],newword);
		pf_dirty[offset] = 1;
	}
}

int prehisle_video_r(int offset)
{
	return READ_WORD(&pf_video[offset]);
}

void prehisle_control_w(int offset,int data)
{
	switch (offset) {
		case 0:
			WRITE_WORD(&vid_control[0],data);
			break;
		case 0x10:
			WRITE_WORD(&vid_control[2],data);
			break;

		case 0x20:
			WRITE_WORD(&vid_control[4],data);
			break;

		case 0x30:
			WRITE_WORD(&vid_control[6],data);
			break;

		case 0x50:
			WRITE_WORD(&vid_control[8],data);
			break;

		case 0x52:
			WRITE_WORD(&vid_control[10],data);
			break;
		case 0x60:
			WRITE_WORD(&vid_control[12],data);
			break;
	}
}
