/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x800

unsigned char *jrpacman_scroll,*jrpacman_bgpriority;
unsigned char *jrpacman_charbank,*jrpacman_spritebank;
unsigned char *jrpacman_palettebank,*jrpacman_colortablebank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Jr. Pac Man has two 256x4 palette PROMs (the three msb of the address are
  grounded, so the effective colors are only 32) and one 256x4 color lookup
  table PROM.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
  bit 0 -- 470 ohm resistor  -- GREEN

  bit 3 -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void jrpacman_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i+32] >> 0) & 0x01;
		bit2 = (color_prom[i+32] >> 1) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i+32] >> 2) & 0x01;
		bit2 = (color_prom[i+32] >> 3) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 64*4;i++)
		colortable[i] = color_prom[i + 64];
	for (i = 64*4;i < 64*4+64*4;i++)
	{
		if (color_prom[i - 64*4 + 64]) colortable[i] = color_prom[i - 64*4 + 64] + 0x10;
		else colortable[i] = 0;
	}
}



int jrpacman_vh_start(void)
{
	int len;


	/* Jr. Pac Man has a virtual screen twice as large as the visible screen */
	len = 2 * (Machine->drv->screen_width/8) * (Machine->drv->screen_height/8);

	if ((dirtybuffer = malloc(len)) == 0)
		return 1;
	memset(dirtybuffer,0,len);

	if ((tmpbitmap = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void jrpacman_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



void jrpacman_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;

		if (offset < 32)	/* line color - mark whole line as dirty */
		{
			int i;


			for (i = 2*32;i < 56*32;i += 32)
				dirtybuffer[i + offset] = 1;
		}
	}
}



void jrpacman_palettebank_w(int offset,int data)
{
	if (*jrpacman_palettebank != data)
	{
		int i;


		*jrpacman_palettebank = data;
		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;
	}
}



void jrpacman_colortablebank_w(int offset,int data)
{
	if (*jrpacman_colortablebank != data)
	{
		int i;


		*jrpacman_colortablebank = data;
		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;
	}
}



void jrpacman_charbank_w(int offset,int data)
{
	if (*jrpacman_charbank != data)
	{
		int i;


		*jrpacman_charbank = data;
		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void jrpacman_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;


			dirtybuffer[offs] = 0;

			/* Jr. Pac Man's screen layout is quite awkward */
			mx = offs / 32;
			my = offs % 32;

			if (mx >= 2 && mx < 60)
			{
				if (mx < 56)
				{
					sx = 57 - mx;
					sy = my + 2;

					drawgfx(tmpbitmap,Machine->gfx[0],
							videoram[offs] + 256 * *jrpacman_charbank,
						/* color is set line by line */
							(videoram[my] & 0x1f) + 0x20 * (*jrpacman_colortablebank & 1)
									+ 0x40 * (*jrpacman_palettebank & 1),
							0,0,8*sx,8*sy,
							0,TRANSPARENCY_NONE,0);
				}
				else
				{
					if (mx >= 58)
					{
						sx = 29 - my;
						sy = mx - 58;

						drawgfx(tmpbitmap,Machine->gfx[0],
								videoram[offs],
								(videoram[offs + 4*32] & 0x1f) + 0x20 * (*jrpacman_colortablebank & 1)
										+ 0x40 * (*jrpacman_palettebank & 1),
								0,0,8*sx,8*sy,
								0,TRANSPARENCY_NONE,0);
					}
					else
					{
						sx = 29 - my;
						sy = mx - 22;

						drawgfx(tmpbitmap,Machine->gfx[0],
								videoram[offs] + 0x100 * (*jrpacman_charbank & 1),
								(videoram[offs + 4*32] & 0x1f) + 0x20 * (*jrpacman_colortablebank & 1)
										+ 0x40 * (*jrpacman_palettebank & 1),
								0,0,8*sx,8*sy,
								0,TRANSPARENCY_NONE,0);
					}
				}
			}
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[36];


		for (i = 0;i < 2;i++)
			scroll[i] = 0;
		for (i = 2;i < 34;i++)
			scroll[i] = *jrpacman_scroll - 224;
		for (i = 34;i < 36;i++)
			scroll[i] = 0;

		copyscrollbitmap(bitmap,tmpbitmap,36,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	/* sprites #0 and #7 are not used */
	for (i = 6;i > 2;i--)
	{
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[2*i] >> 2) + 0x40 * (*jrpacman_spritebank & 1),
				(spriteram[2*i + 1] & 0x1f) + 0x20 * (*jrpacman_colortablebank & 1)
						+ 0x40 * (*jrpacman_palettebank & 1),
				spriteram[2*i] & 2,spriteram[2*i] & 1,
				239 - spriteram_2[2*i],272 - spriteram_2[2*i + 1],
				&Machine->drv->visible_area,
				(*jrpacman_bgpriority & 1) ? TRANSPARENCY_THROUGH : TRANSPARENCY_COLOR,Machine->background_pen);
	}
	/* the first two sprites must be offset one pixel to the left */
	for (i = 2;i > 0;i--)
	{
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[2*i] >> 2) + 0x40 * (*jrpacman_spritebank & 1),
				(spriteram[2*i + 1] & 0x1f) + 0x20 * (*jrpacman_colortablebank & 1)
						+ 0x40 * (*jrpacman_palettebank & 1),
				spriteram[2*i] & 2,spriteram[2*i] & 1,
				238 - spriteram_2[2*i],272 - spriteram_2[2*i + 1],
				&Machine->drv->visible_area,
				(*jrpacman_bgpriority & 1) ? TRANSPARENCY_THROUGH : TRANSPARENCY_COLOR,Machine->background_pen);
	}
}
