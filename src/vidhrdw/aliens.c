#include "driver.h"
#include "vidhrdw/konamiic.h"


#define TILEROM_MEM_REGION 1
#define SPRITEROM_MEM_REGION 2

static int layer_colorbase[3],sprite_colorbase;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color,unsigned char *flags)
{
	*code |= ((*color & 0x3f) << 8) | (bank << 14);
	*color = ((*color & 0xc0) >> 6);
	*color += layer_colorbase[layer];
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority)
{
	/* Weird priority scheme. Why use three bits when two would suffice? */
	switch (*color & 0x70)
	{
		case 0x10: *priority = 0; break;
		case 0x00: *priority = 1; break;
		case 0x40: *priority = 2; break;
		case 0x20: *priority = 3; break;
	}
	*code |= (*color & 0x80) << 6;
	*color = sprite_colorbase + (*color & 0x0f);
}



/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int aliens_vh_start( void )
{
	paletteram = malloc(0x400);
	if (!paletteram) return 1;

	layer_colorbase[0] = 0;
	layer_colorbase[1] = 4;
	layer_colorbase[2] = 8;
	sprite_colorbase = 16;
	if (K052109_vh_start(TILEROM_MEM_REGION,NORMAL_PLANE_ORDER,tile_callback))
	{
		free(paletteram);
		return 1;
	}
	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,sprite_callback))
	{
		free(paletteram);
		K052109_vh_stop();
		return 1;
	}

	return 0;
}

void aliens_vh_stop( void )
{
	free(paletteram);
	K052109_vh_stop();
	K051960_vh_stop();
}


void aliens_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	palette_init_used_colors();
	K051960_mark_sprites_colors();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;	/* ?? */
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);	/* ?? */
	K051960_draw_sprites(bitmap,3,3);
	K052109_tilemap_draw(bitmap,1,0);
	K051960_draw_sprites(bitmap,2,2);
	K052109_tilemap_draw(bitmap,2,0);
	K051960_draw_sprites(bitmap,1,1);
	K052109_tilemap_draw(bitmap,0,0);
	K051960_draw_sprites(bitmap,0,0);
}