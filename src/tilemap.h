/* tilemap.h */

#ifndef TILEMAP_H
#define TILEMAP_H


#define ALL_TILEMAPS	0
/* ALL_TILEMAPS may be used with:
	tilemap_update, tilemap_render, tilemap_set_flip, tilemap_mark_all_pixels_dirty
*/

#define TILEMAP_OPAQUE			0x00
#define TILEMAP_TRANSPARENT		0x01
#define TILEMAP_SPLIT			0x02
/*
	TILEMAP_SPLIT should be used if the pixels from a single tile
	can appear in more than one plane.
*/

#define TILEMAP_IGNORE_TRANSPARENCY		0x10
#define TILEMAP_BACK					0x20
#define TILEMAP_FRONT					0x40
/*
	when rendering a split layer, pass TILEMAP_FRONT or TILEMAP_BACK or'd with the
	tile_priority value to specify the part to draw.
*/

extern struct tile_info {
	unsigned char *pen_data; /* pointer to gfx data */
	unsigned short *pal_data; /* pointer to palette */
	unsigned int pen_usage;	/* used pens mask */
	/*
		you must set tile_info.pen_data, tile_info.pal_data and tile_info.pen_usage
		in the callback.  You can use the SET_TILE_INFO() macro below to do this.
		tile_info.flags and tile_info.priority will be automatically preset to 0,
		games that don't need them don't need to explicitly set them to 0
	*/
	unsigned char flags; /* see below */
	unsigned char priority;
} tile_info;

#define SET_TILE_INFO(GFX,CODE,COLOR) { \
	const struct GfxElement *gfx = Machine->gfx[(GFX)]; \
	int _code = (CODE) % gfx->total_elements; \
	tile_info.pen_data = gfx->gfxdata->line[_code*gfx->height]; \
	tile_info.pal_data = &gfx->colortable[gfx->color_granularity * (COLOR)]; \
	tile_info.pen_usage = gfx->pen_usage[_code]; \
}

/* tile flags, set by get_tile_info callback */
#define TILE_FLIPX					0x01
#define TILE_FLIPY					0x02
#define TILE_SPLIT(T)				((T)<<2)
/* TILE_SPLIT is for use with TILEMAP_SPLIT layers.  It selects transparency type. */
#define TILE_IGNORE_TRANSPARENCY	0x10
/* TILE_IGNORE_TRANSPARENCY is used if you need an opaque tile in a transparent layer */

#define TILE_FLIPYX(YX)				(YX)
#define TILE_FLIPXY(XY)			((((XY)>>1)|((XY)<<1))&3)
/*
	TILE_FLIPYX is a shortcut that can be used by approx 80% of games,
	since yflip frequently occurs one bit higher than xflip within a
	tile attributes byte.
*/

struct tilemap {
	int type;
	int enable;
	int attributes;
	int transparent_pen;
	unsigned int transmask[4];

	int num_rows, num_cols, num_tiles;
	int tile_width, tile_height, width, height;

	void (*mark_visible)( int, int );
	void (*draw)( int, int );
	void (*draw_opaque)( int, int );

	unsigned char **pendata;
	unsigned short **paldata;
	unsigned int *pen_usage;

	char *priority, **priority_row;
	char *visible, **visible_row;
	char *dirty_vram;

	char *dirty_pixels;
	unsigned char *flags;

	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int col, int row );

	int scrolled;
	int scroll_rows, scroll_cols;
	int *rowscroll, *colscroll;

	struct rectangle clip;
	int clip_left,clip_right,clip_top,clip_bottom;

	/* cached color data */
	struct osd_bitmap *pixmap;
	int pixmap_line_offset;

	/* foreground mask */
	struct osd_bitmap *fg_mask;
	unsigned char *fg_mask_data;
	unsigned char **fg_mask_data_row;
	int fg_mask_line_offset;
	unsigned short *fg_span, **fg_span_row;

	/* background mask (for the back half of a split layer) */
	struct osd_bitmap *bg_mask;
	unsigned char *bg_mask_data;
	unsigned char **bg_mask_data_row;
	int bg_mask_line_offset;
	unsigned short *bg_span, **bg_span_row;

	struct tilemap *next; /* resource management */
};

/* don't call these from drivers - they are called from mame.c */
void tilemap_init( void );
void tilemap_close( void );

struct tilemap *tilemap_create(
	void (*tile_get_info)( int col, int row ),
	int type,
	int tile_width, int tile_height, /* in pixels */
	int num_cols, int num_rows /* in tiles */
);

void tilemap_dispose( struct tilemap *tilemap );
/* you needn't to call this in vh_close.  It's supplied for games that need to change
	tile size or cols/rows dynamically */

int tilemap_set_scroll_cols( struct tilemap *tilemap, int scroll_cols );
int tilemap_set_scroll_rows( struct tilemap *tilemap, int scroll_rows );
/* these return 1 if there is insufficient memory */

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int col, int row );
void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap );
void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap );

void tilemap_set_scrollx( struct tilemap *tilemap, int row, int value );
void tilemap_set_scrolly( struct tilemap *tilemap, int col, int value );

//void tilemap_set_screen_linescroll( struct tilemap *tilemap, int row, int value );
/* not yet supported */

#define TILEMAP_FLIPX 0x1
#define TILEMAP_FLIPY 0x2
void tilemap_set_flip( struct tilemap *tilemap, int attributes );
void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip );
void tilemap_set_enable( struct tilemap *tilemap, int enable );

void tilemap_update( struct tilemap *tilemap );
void tilemap_render( struct tilemap *tilemap );
void tilemap_draw( struct osd_bitmap *dest, struct tilemap *tilemap, int priority );


#endif