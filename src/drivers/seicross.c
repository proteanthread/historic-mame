/***************************************************************************

Seicross memory map (preliminary by Mirko)

0000-6fff ROM
7800-7fff RAM
9000-93ff videoram
9c00-9fff colorram

Read:
7814-7815 Protection check
A000      Joystick + Players start button
A800      player #2 controls + coin + ?
B000      test switches
B800      watchdog reset?

Write:
8820-887f Sprite ram
9800-981f Scroll control
9880-989f ? (always 0?)

I/O ports:
0         8910 control
1         8910 write
4         8910 read

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int seicross_protection_r(int offset);

extern unsigned char *seicross_row_scroll;
extern void seicross_colorram_w(int offset,int data);
extern void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void seicross_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int seicross_sh_start(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x7814, 0x7815, seicross_protection_r },	/* ??? */
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x981f, MRA_RAM },
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* test */
	{ 0xb800, 0xb800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram },
	{ 0x9800, 0x981f, MWA_RAM, &seicross_row_scroll },
	{ 0x8820, 0x887f, MWA_RAM, &spriteram },
	{ 0x9c00, 0x9fff, seicross_colorram_w, &colorram },
	{ 0x9880, 0x989f, MWA_NOP },	/* ? */
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x04, 0x04, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				0, 0, OSD_KEY_1, OSD_KEY_2 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				0, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,	/* standup cabinet */
		{ 0, 0, 0, 0, OSD_KEY_3, 0, OSD_KEY_CONTROL, 0 },
		{ 0, 0, 0, 0, 0, 0, OSD_JOY_FIRE, 0 }
	},
	{	/* test */
		0x00,
		{ OSD_KEY_F1, OSD_KEY_F2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* ? */
		0x60,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 0, 0, "MOVE UP" },
        { 0, 2, "MOVE LEFT" },
        { 0, 3, "MOVE RIGHT" },
        { 0, 1, "MOVE DOWN" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x40, "DEMO MODE", { "ON", "OFF" }, 1 },
	{ 3, 0x20, "DEBUG MODE", { "ON", "OFF" }, 1 },
	{ 3, 0x80, "SW7B", { "0", "1" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes are packed in one byte */
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes are packed in one byte */
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 17*8+0, 17*8+1, 17*8+2, 17*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 16 },
	{ 1, 0x1000, &charlayout,   0, 16 },
        { 1, 0x1000, &charlayout,   0, 16 },
	{ 1, 0x2000, &spritelayout, 0, 16 },
	{ 1, 0x3000, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* char palette */
	0x00,0x79,0x04,0x87,0x00,0xb7,0xff,0x5f,0x00,0xc0,0xe8,0xf4,0x00,0x3f,0x04,0x38,
	0x00,0x0d,0x7a,0xb7,0x00,0x07,0x26,0x02,0x00,0x27,0x16,0x30,0x00,0xb7,0xf4,0x0c,
	0x00,0x4f,0xf6,0x24,0x00,0xb6,0xff,0x5f,0x00,0x33,0x00,0xb7,0x00,0x66,0x00,0x3a,
	0x00,0xc0,0x3f,0xb7,0x00,0x20,0xf4,0x16,0x00,0xff,0x7f,0x87,0x00,0xb6,0xf4,0xc0,
	/* bigsprite palette */
	0x00,0xff,0x18,0xc0,0x00,0xff,0xc6,0x8f,0x00,0x0f,0xff,0x1e,0x00,0xff,0xc0,0x67,
	0x00,0x47,0x7f,0x80,0x00,0x88,0x47,0x7f,0x00,0x7f,0x88,0x47,0x00,0x40,0x08,0xff
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	96,4*24,
	cclimber_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	seicross_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	seicross_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( seicross_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "smc1", 0x0000, 0x1000 )
	ROM_LOAD( "smc2", 0x1000, 0x1000 )
	ROM_LOAD( "smc3", 0x2000, 0x1000 )
	ROM_LOAD( "smc4", 0x3000, 0x1000 )
	ROM_LOAD( "smc5", 0x4000, 0x1000 )
	ROM_LOAD( "smc6", 0x5000, 0x1000 )	/* ?? */
	ROM_LOAD( "smc7", 0x6000, 0x1000 )	/* ?? */

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "smcc", 0x0000, 0x1000 )
	ROM_LOAD( "smcd", 0x1000, 0x1000 )
	ROM_LOAD( "smca", 0x2000, 0x1000 )
	ROM_LOAD( "smcb", 0x3000, 0x1000 )

	ROM_REGION(0x2000)	/* samples */
	ROM_LOAD( "smc8", 0x0000, 0x0800 )	/* ????? */
	ROM_LOAD( "smc9", 0x0800, 0x0800 )	/* ????? */
ROM_END



struct GameDriver seicross_driver =
{
	"seicross",
        "seicross",
        "MIRKO BUFFONI\nNICOLA SALMORIA",
	&machine_driver,

	seicross_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x0a, 0x0b,
	8*13, 8*16, 0x0a,

	0, 0
};
