/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *lwings_paletteram;
unsigned char *lwings_backgroundram;
unsigned char *lwings_backgroundattribram;
int lwings_paletteram_size;
int lwings_backgroundram_size;
unsigned char *lwings_scrolly;
unsigned char *lwings_scrollx;

unsigned char *trojan_scrolly;
unsigned char *trojan_scrollx;
unsigned char *trojan_bk_scrolly;
unsigned char *trojan_bk_scrollx;

static unsigned char *dirtybuffer2;
static unsigned char *dirtybuffer3;
static unsigned char *dirtybuffer4;
static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;
static unsigned char lwings_paletteram_dirty[0x40];

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int lwings_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(lwings_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,lwings_backgroundram_size);

        /* Palette RAM dirty buffer */
        if ((dirtybuffer3 = malloc(lwings_paletteram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer3,1,lwings_paletteram_size);

        if ((dirtybuffer4 = malloc(lwings_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer4,1,lwings_backgroundram_size);

	/* the background area is twice as tall as the screen */
        if ((tmpbitmap2 = osd_new_bitmap(2*Machine->drv->screen_width,
                2*Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;

}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void lwings_vh_stop(void)
{
        osd_free_bitmap(tmpbitmap2);
        free(dirtybuffer2);
        free(dirtybuffer3);
        free(dirtybuffer4);
	generic_vh_stop();
}

void lwings_background_w(int offset,int data)
{
        if (lwings_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

                lwings_backgroundram[offset] = data;
	}
}

void lwings_backgroundattrib_w(int offset,int data)
{
        if (lwings_backgroundattribram[offset] != data)
	{
                dirtybuffer4[offset] = 1;

                lwings_backgroundattribram[offset] = data;
	}
}


void lwings_paletteram_w(int offset,int data)
{
        if (lwings_paletteram[offset] != data)
	{
                dirtybuffer3[offset]=1;
                lwings_paletteram[offset] = data;
                /* Mark entire colour schemes dirty if touched */
                lwings_paletteram_dirty[(offset>>4)&0x3f]=1;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void lwings_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int j, i, offs;

	/* rebuild the colour lookup table */
	for (j=0; j<3; j++)
	{
		/*
		0000-007f:  background palette. (8x16 colours)
		0180-01ff:  sprite palette.     (8x16 colours)
		0200-0240:  character palette   (16x4 colours)
		*/
		/* CHARS TILES SPRITES */
		int start[3]={0x0200, 0x0000, 0x0180};
		int count[3]={  0x40,   0x80,   0x80};
		int base=start[j];
		int max=count[j];

		for (i=0; i<max; i++)
		{
			if (dirtybuffer3[base] || dirtybuffer3[base+0x0400])
			{
				int redgreen=lwings_paletteram[base];
				int blue=lwings_paletteram[base+0x0400]>>4;
				int red=redgreen >> 4;
				int green=redgreen & 0x0f;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				dirtybuffer3[base] = dirtybuffer3[base+0x0400] = 0;

				setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
			}
			base++;
		}
	}

        for (offs = lwings_backgroundram_size - 1;offs >= 0;offs--)
        {
                int sx,sy, colour;
                /*
                        Tiles
                        =====
                        0x80 Tile code MSB
                        0x40 Tile code MSB
                        0x20 Tile code MSB
                        0x10 X flip
                        0x08 Y flip
                        0x04 Colour
                        0x02 Colour
                        0x01 Colour
                */

                colour=(lwings_backgroundattribram[offs] & 0x07);
                if (dirtybuffer2[offs] != 0 || dirtybuffer4[offs] != 0 ||
                      lwings_paletteram_dirty[colour])
                {
                      int code;
                      dirtybuffer2[offs] = dirtybuffer4[offs] = 0;

                      sx = offs / 32;
                      sy = offs % 32;
                      code=lwings_backgroundram[offs];
                      code+=((((int)lwings_backgroundattribram[offs]) &0xe0) <<3);

                      drawgfx(tmpbitmap2,Machine->gfx[1],
                                   code,
                                   colour,
                                   (lwings_backgroundattribram[offs] & 0x08),
                                   (lwings_backgroundattribram[offs] & 0x10),
                                   16 * sx,16 * sy,
                                   0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrolly = -(lwings_scrollx[0] + 256 * lwings_scrollx[1]);
		scrollx = -(lwings_scrolly[0] + 256 * lwings_scrolly[1]);

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int code,sx,sy;

		/*
		Sprites
		=======
		0x80 Sprite code MSB
		0x40 Sprite code MSB
		0x20 Colour
		0x10 Colour
		0x08 Colour
		0x04 Y flip
		0x02 X flip
		0x01 X MSB
		*/
		sx = spriteram[offs + 3] - 0x100 * (spriteram[offs + 1] & 0x01);
		sy = spriteram[offs + 2];
                if (sx && sy)
                {
                        code = spriteram[offs];
                        code += (spriteram[offs + 1] & 0xc0) << 2;

                        drawgfx(bitmap,Machine->gfx[2],
				code,
				(spriteram[offs + 1] & 0x38) >> 3,
				spriteram[offs + 1] & 0x02,spriteram[offs + 1] & 0x04,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                }
	}

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy,flipx,flipy;


		sx = offs % 32;
		sy = offs / 32;
		flipx = colorram[offs] & 0x10;
		flipy = colorram[offs] & 0x20;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 4 * (colorram[offs] & 0xc0),
				colorram[offs] & 0x0f,
				flipx,flipy,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
	}

	memset(lwings_paletteram_dirty, 0, sizeof(lwings_paletteram_dirty));
}

/*
TROJAN
======

Differences:

Tile attribute (no y flip, possible priority)
Sprite attribute (more sprites)
Extra scroll layer

*/


int  trojan_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(lwings_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,lwings_backgroundram_size);

        /* Palette RAM dirty buffer */
        if ((dirtybuffer3 = malloc(lwings_paletteram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer3,1,lwings_paletteram_size);

        if ((dirtybuffer4 = malloc(lwings_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer4,1,lwings_backgroundram_size);

        if ((tmpbitmap3 = osd_new_bitmap(16*0x12,
                16*0x12,Machine->scrbitmap->depth)) == 0)
        {
		free(dirtybuffer4);
		free(dirtybuffer3);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;

        }
        return 0;
}

void trojan_vh_stop(void)
{
        osd_free_bitmap(tmpbitmap3);
		free(dirtybuffer4);
        free(dirtybuffer3);
		free(dirtybuffer2);
        generic_vh_stop();
}

void trojan_render_foreground( struct osd_bitmap *bitmap, int scrollx, int scrolly, int priority )
{
        int scrlx = -(scrollx&0x0f);
        int scrly = -(scrolly&0x0f);
        int sx, sy;
        int offsy = (scrolly >> 4)-1;
        int offsx = (scrollx >> 4)*32-32;

        int transp0,transp1;
        if( priority ){
		transp0 = 0xFFFF;	/* draw nothing (all pens transparent) */
		transp1 = 0xF00F;	/* high priority half of tile */
	}
        else {
		transp0 = 1;		/* TRANSPARENCY_PEN, color 0 */
		transp1 = 0x0FF0;	/* low priority half of tile */
	}

        for (sx=0; sx<0x12; sx++)
        {
                offsx&=0x03ff;
                for (sy=0; sy<0x12; sy++)
                {
                /*
                        Tiles
                        0x80 Tile code MSB
                        0x40 Tile code MSB
                        0x20 Tile code MSB
                        0x10 X flip
                        0x08 Priority ????
                        0x04 Colour
                        0x02 Colour
                        0x01 Colour
               */
                       int offset = offsx+( (sy+offsy)&0x1f );
                       int attribute = lwings_backgroundattribram[offset];
                       drawgfx(bitmap,Machine->gfx[1],
                                lwings_backgroundram[offset] + ((attribute &0xe0) <<3),
                                attribute & 0x07,
                                attribute & 0x10,
                                0,
                                16 * sx+scrlx-16,16 * sy+scrly-16, &Machine->drv->visible_area, TRANSPARENCY_PENS,(attribute & 0x08)?transp1:transp0 );
                }
                offsx+=0x20;
	}
}


void trojan_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int j, i, offs, sx, sy, scrollx, scrolly;
        int offsy, offsx;

	/* rebuild the colour lookup table */
        for (j=0; j<4; j++)
	{
		/*
		*/
		/* CHARS TILES SPRITES */
                int start[4]={0x0300,  0x0100,  0x0280, 0x0000};
                int count[4]={  0x40,    0x80,    0x80, 0x80};
		int base=start[j];
		int max=count[j];

		for (i=0; i<max; i++)
		{
			if (dirtybuffer3[base] || dirtybuffer3[base+0x0400])
			{
				int redgreen=lwings_paletteram[base];
				int blue=lwings_paletteram[base+0x0400]>>4;
				int red=redgreen >> 4;
				int green=redgreen & 0x0f;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				dirtybuffer3[base] = dirtybuffer3[base+0x0400] = 0;

				setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
			}
			base++;
		}
	}

        {
              static int oldoffsy=0xffff;
              static int oldoffsx=0xffff;

              int scrollx = (trojan_bk_scrollx[0]);
              int scrolly = (trojan_bk_scrolly[0]);

              offsy = 0x20 * scrolly;
              offsx = (scrollx >> 4);
              scrollx = -(scrollx & 0x0f);
              scrolly = 0; /* Y doesn't scroll ??? */
              if (oldoffsy != offsy || oldoffsx != offsx)
              {
                  unsigned char *p=Machine->memory_region[4];
                  oldoffsx=offsx;
                  oldoffsy=offsy;

                  for (sy=0; sy < 0x11; sy++)
                  {
                      offsy &= 0x7fff;
                      for (sx=0; sx<0x11; sx++)
                      {
                          int code, colour;
                          int offset=offsy + ((2*(offsx+sx)) & 0x3f);
                          code = *(p+offset);
                          colour = *(p+offset+1);
                          drawgfx(tmpbitmap3, Machine->gfx[3],
                                   code+((colour&0x80)<<1),
                                   colour & 0x07,
                                   colour&0x10,
                                   colour&0x20,
                                   16 * sx,16 * sy,
                                   0,TRANSPARENCY_NONE,0);
                      }
                      offsy += 0x800;
                  }
              }
              copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }

        scrollx = (trojan_scrollx[0] + 256 * trojan_scrollx[1]);
        scrolly = (trojan_scrolly[0] + 256 * trojan_scrolly[1]);

        trojan_render_foreground( bitmap, scrollx, scrolly, 0 );

	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
                int code,sx,sy, attrib;

		/*
		Sprites
		=======
		0x80 Sprite code MSB
		0x40 Sprite code MSB
                0x20 Sprite code MSB
                0x10 X flip
		0x08 Colour
                0x04 colour
                0x02 colour
		0x01 X MSB
		*/

                attrib = spriteram[offs + 1];
                sx = spriteram[offs + 3] - 0x100 * (attrib & 0x01);
		sy = spriteram[offs + 2];

                if (sx && sy)
                {
                        code = spriteram[offs];


                        if( attrib&0x40 ) code += 256;
                        if( attrib&0x80 ) code += 256*4;
                        if( attrib&0x20 ) code += 256*2;


                        drawgfx(bitmap,Machine->gfx[2],
				code,
                                (attrib & 0x0e) >> 1,
                                attrib & 0x10,1,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
                }
	}

        trojan_render_foreground( bitmap, scrollx, scrolly, 1 );


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy,flipx,flipy;
                int code, attrib;
                attrib = colorram[offs];
                code = videoram[offs] + 4 * (attrib & 0xc0);
		if (code != 0x20)     /* don't draw spaces */
		{
			sx = offs % 32;
			sy = offs / 32;
                	flipx = colorram[offs] & 0x10;
	                flipy = colorram[offs] & 0x20;

			drawgfx(bitmap,Machine->gfx[0],
				code,
				colorram[offs] & 0x0f,
				flipx,flipy,
				8*sx,8*sy,
				&Machine->drv->visible_area,
                                TRANSPARENCY_PEN,3);
		}
	}

        memset(lwings_paletteram_dirty, 0, sizeof(lwings_paletteram_dirty));
}