/***************************************************************************

  vidhrdw.c

  Generic functions used by the Cinematronics Vector games

***************************************************************************/

#include "driver.h"
#include "vector.h"
#include "cpu/ccpu/ccpu.h"

#define VEC_SHIFT 16


int sdwGameXSize;
int sdwGameYSize;
int sdwXOffset;
int sdwYOffset;

static int lastx, lasty;
static int inited = 0;
static struct artwork *backdrop;
static struct artwork *overlay;

static int vgColor;

void CinemaVectorData (int fromx, int fromy, int tox, int toy, int color)
{
	if (!inited)
	{
		vector_clear_list ();
		inited = 1;
	}
	if (fromx != lastx || fromx != lasty)
		vector_add_point (fromx << VEC_SHIFT, fromy << VEC_SHIFT, 0, 0);
	vector_add_point (tox << VEC_SHIFT, toy << VEC_SHIFT, vgColor, color * 12);
	lastx = tox;
	lasty = toy;
}

#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE

static void shade_fill (unsigned char *palette, int rgb, int start_index, int end_index, int start_inten, int end_inten)
{
	int i, inten, index_range, inten_range;

	index_range = end_index-start_index;
	inten_range = end_inten-start_inten;
	for (i = start_index; i <= end_index; i++)
	{
		inten = start_inten + (inten_range) * (i-start_index) / (index_range);
		palette[3*i  ] = (rgb & RED  )? inten : 0;
		palette[3*i+1] = (rgb & GREEN)? inten : 0;
		palette[3*i+2] = (rgb & BLUE )? inten : 0;
	}
}

void cinemat_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,j,k;
	char filename[1024];

	int trcl1[] = { 0,0,2,2,1,1 };
	int trcl2[] = { 1,2,0,1,0,2 };
	int trcl3[] = { 2,1,1,0,2,0 };

	struct artwork_element no_overlay[]=
	{
		{{0, 400-1, 0, 300-1}, 255, 255, 255, 0},
		{{-1,-1,-1,-1},0,0,0,0}
	};
	struct artwork_element starcas_overlay[]=
	{
		{{0, 400-1, 0, 300-1}, 0, 6, 25, 64},
		{{ 200, 49, 150, -1},33, 0, 16, 64},
		{{ 200, 38, 150, -1},32, 15, 0, 64},
		{{ 200, 29, 150, -1},30, 33, 0, 64},
		{{-1,-1,-1,-1},0,0,0,0}
	};
	struct artwork_element sundance_overlay[]=
	{
		{{0, 400-1, 0, 300-1}, 32, 32, 0, 32},
		{{-1,-1,-1,-1},0,0,0,0}
	};
	struct artwork_element tailg_overlay[]=
	{
		{{0, 400-1, 0, 300-1}, 0, 64, 64, 32},
		{{-1,-1,-1,-1},0,0,0,0}
	};
	struct artwork_element solarq_overlay[]=
	{
		{{0, 400-1, 0, 300-1}, 0, 6, 25, 64},
		{{ 0,  399, 279, 299},31, 0, 12, 64},
		{{ 200, 12, 150,  -1},31, 31, 0, 64},
		{{-1,-1,-1,-1},0,0,0,0}
	};

	struct artwork_element *simple_overlays[5];

	simple_overlays[0] = no_overlay;
	simple_overlays[1] = starcas_overlay;
	simple_overlays[2] = sundance_overlay;
	simple_overlays[3] = tailg_overlay;
	simple_overlays[4] = solarq_overlay;

	overlay = NULL;
	backdrop = NULL;

	/* initialize the first 8 colors with the basic colors */
	for (i = 0; i < 8; i++)
	{
		palette[3*i  ] = (i & RED  ) ? 0xff : 0;
		palette[3*i+1] = (i & GREEN) ? 0xff : 0;
		palette[3*i+2] = (i & BLUE ) ? 0xff : 0;
	}

	vgColor = WHITE;

	shade_fill (palette, WHITE, 8, 23, 0, 255);

	/* fill the rest of the 256 color entries depending on the game */
	switch (color_prom[0] & 0x0f)
	{
		case  CCPU_MONITOR_BILEV:
		case  CCPU_MONITOR_16COL:

			sprintf (filename, "%s.png", Machine->gamedrv->name );
			/* Attempt to load overlay if requested */
			if (color_prom[0] & 0x80)
			{
				/* Attempt to load artwork from file */
				overlay=artwork_load(filename, 24, Machine->drv->total_colors-24);

				if ((overlay==NULL) && (color_prom[0] & 0x20))
				{
					/* no overlay file found - use simple artwork */
					artwork_elements_scale(simple_overlays[color_prom[1]],
										   Machine->scrbitmap->width,
										   Machine->scrbitmap->height);
					overlay=artwork_create(simple_overlays[color_prom[1]], 24,
										   Machine->drv->total_colors-24);
				}

				if (overlay != NULL)
				{
					if (overlay_set_palette (overlay, palette, Machine->drv->total_colors-24))
						return;
				}
				else
					shade_fill (palette, WHITE, 24, 128+24, 0, 255);
			}
			else
			/* Attempt to load backdrop if requested */
			if (color_prom[0] & 0x40)
			{
				if ((backdrop=artwork_load(filename, 24, Machine->drv->total_colors-24))!=NULL)
					memcpy (palette+3*backdrop->start_pen, backdrop->orig_palette, 3*backdrop->num_pens_used);
				else
					shade_fill (palette, WHITE, 24, 128+24, 0, 255);
			}
			break;

		case  CCPU_MONITOR_64COL:
			shade_fill (palette, RED  ,     8, 128+4, 1, 254);
			shade_fill (palette, GREEN, 128+5, 254  , 1, 254);
			break;

		case  CCPU_MONITOR_WOWCOL:
			/* TODO: support real color */
			/* put in 40 shades for red, blue and magenta */
			shade_fill (palette, RED       ,   8,  47, 10, 250);
			shade_fill (palette, BLUE      ,  48,  87, 10, 250);
			shade_fill (palette, RED|BLUE  ,  88, 127, 10, 250);

			/* put in 20 shades for yellow and green */
			shade_fill (palette, GREEN     , 128, 147, 10, 250);
			shade_fill (palette, RED|GREEN , 148, 167, 10, 250);

			/* and 14 shades for cyan and white */
			shade_fill (palette, BLUE|GREEN, 168, 181, 10, 250);
			shade_fill (palette, WHITE     , 182, 194, 10, 250);

			/* Fill in unused gaps with more anti-aliasing colors. */
			/* There are 60 slots available.           .ac JAN2498 */
			i=195;
			for (j=0; j<6; j++)
			{
				for (k=7; k<=16; k++)
				{
					palette[3*i+trcl1[j]] = ((256*k)/16)-1;
					palette[3*i+trcl2[j]] = ((128*k)/16)-1;
					palette[3*i+trcl3[j]] = 0;
					i++;
				}
			}
			break;
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cinemat_vh_start (void)
{
	vector_set_shift (VEC_SHIFT);
	inited = 1;
	if (backdrop) backdrop_refresh (backdrop);
	if (overlay) overlay_remap (overlay);
	return vector_vh_start();
}


void cinemat_vh_stop (void)
{
	if (backdrop) artwork_free (backdrop);
	if (overlay) artwork_free (overlay);
	vector_vh_stop();
	inited = 0;
}


void cinemat_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	if (inited || full_refresh)
	{
		if (backdrop)
			vector_vh_update_backdrop(bitmap, backdrop, full_refresh);
		if (overlay)
			vector_vh_update_overlay(bitmap, overlay, full_refresh);
		else
			vector_vh_update(bitmap, full_refresh);
		inited = 0;
	}
}