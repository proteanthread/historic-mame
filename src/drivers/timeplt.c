/***************************************************************************

Time Pilot memory map (preliminary)

Main processor memory map.
0000-5fff ROM
a000-a3ff Color RAM
a400-a7ff Video RAM
a800-afff RAM
b000-b7ff sprite RAM (only areas 0xb010 and 0xb410 are used).

memory mapped ports:

read:
c000      ???
c200      DSW2
	Dipswitch 2 adddbtll
        a = attract mode
        ddd = difficulty 0=easy, 7=hardest.
        b = bonus setting (easy/hard)
        t = table / upright
        ll = lives: 11=3, 10=4, 01=5, 00=255.
c300      IN0
c320      IN1
c340      IN2
c360      DSW1
        llllrrrr
        l == left coin mech, r = right coinmech.

write:
c000      command for the audio CPU
c200      watchdog reset
c300      interrupt enable
c301-c303 ?
c304      trigger interrupt on audio CPU
c305-c307 ?

interrupts:
standard NMI at 0x66


SOUND BOARD:
same as Pooyan

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int timeplt_sh_interrupt(void);
extern int timeplt_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xa000, 0xbfff, MRA_RAM },
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },	/* ?????????????????? */
	{ 0xc300, 0xc300, input_port_0_r },	/* IN0 */
	{ 0xc320, 0xc320, input_port_1_r },	/* IN1 */
	{ 0xc340, 0xc340, input_port_2_r },	/* IN2 */
	{ 0xc360, 0xc360, input_port_3_r },	/* DSW1 */
	{ 0xc200, 0xc200, input_port_4_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xa800, 0xafff, MWA_RAM },
	{ 0xa000, 0xa3ff, colorram_w, &colorram },
	{ 0xa400, 0xa7ff, videoram_w, &videoram },
	{ 0xb010, 0xb03f, MWA_RAM, &spriteram },
	{ 0xb410, 0xb43f, MWA_RAM, &spriteram_2 },
	{ 0xc300, 0xc300, interrupt_enable_w },
	{ 0xc200, 0xc200, MWA_NOP },
	{ 0xc000, 0xc000, sound_command_w },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, 0, OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x73,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 1, 2, "MOVE UP" },
        { 1, 0, "MOVE LEFT"  },
        { 1, 1, "MOVE RIGHT" },
        { 1, 3, "MOVE DOWN" },
        { 1, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "255", "5", "4", "3" }, 1 },
	{ 4, 0x08, "BONUS", { "20000 60000", "10000 50000" }, 1 },
	{ 4, 0x70, "DIFFICULTY", { "HARDEST", "HARD", "DIFFICULT", "MEDIUM", "NORMAL", "EASIER", "EASY" , "EASIEST" }, 1 },
	{ 4, 0x80, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout dwspritelayout =
{
	32,16,	/* 2x16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 39*8, 39*8, 38*8, 38*8, 37*8, 37*8, 36*8, 36*8, 35*8, 35*8, 34*8, 34*8, 33*8, 33*8, 32*8, 32*8,
			7*8, 7*8, 6*8, 6*8, 5*8, 5*8, 4*8, 4*8, 3*8, 3*8, 2*8, 2*8, 1*8, 1*8, 0*8, 0*8 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 32 },
	{ 1, 0x2000, &spritelayout,   32*4, 64 },
	{ 1, 0x2000, &dwspritelayout, 32*4, 64 },
	{ -1 } /* end of array */
};



/* these are NOT the original color PROMs */
static unsigned char color_prom[] =
{
	/* B4 - palette */
	0x00,0x00,0x05,0x06,0x07,0xFC,0x05,0xBD,0xB5,0xFD,0x05,0xB0,0xA5,0xE0,0x00,0xF7,
	0x00,0x00,0xF8,0x07,0x07,0xFD,0xF8,0xFA,0x05,0xDE,0x50,0x51,0x32,0xFD,0x30,0xF7,
	/* B5 - palette */
	0x00,0x3E,0x3E,0x80,0xFE,0x00,0xAC,0xEE,0xAC,0xC0,0x14,0x00,0x28,0x38,0x16,0xBC,
	0x00,0x3E,0x00,0xC0,0xFE,0xC0,0x3E,0x80,0x3E,0xF6,0x00,0x80,0x80,0x00,0x0C,0xBC,

	/* E12 (or F15?) - char lookup table */
	/* The PROM is 256x4, but only the first 128 bytes are used */
	0x0F,0x0D,0x0F,0x0C,0x0A,0x04,0x01,0x0F,0x0B,0x04,0x01,0x0F,0x0C,0x04,0x01,0x0F,
	0x0E,0x04,0x01,0x0F,0x00,0x04,0x01,0x0F,0x0A,0x04,0x05,0x01,0x0B,0x04,0x05,0x01,
	0x0C,0x04,0x05,0x01,0x0E,0x04,0x05,0x01,0x00,0x04,0x05,0x00,0x0A,0x06,0x08,0x02,
	0x0B,0x06,0x08,0x02,0x0C,0x06,0x08,0x02,0x0E,0x06,0x08,0x02,0x00,0x06,0x08,0x02,
	0x00,0x01,0x04,0x0F,0x00,0x04,0x02,0x06,0x00,0x01,0x08,0x04,0x00,0x0D,0x01,0x05,
	0x00,0x02,0x03,0x01,0x00,0x0C,0x0F,0x03,0x00,0x05,0x02,0x08,0x0A,0x01,0x04,0x03,
	0x00,0x06,0x0F,0x02,0x00,0x0F,0x03,0x05,0x00,0x03,0x01,0x0F,0x0A,0x02,0x0D,0x05,
	0x00,0x01,0x0F,0x08,0x0A,0x02,0x0D,0x05,0x0A,0x0B,0x09,0x0F,0x09,0x09,0x09,0x09,

	/* E9 - sprite lookup table */
	0x0F,0x0D,0x0F,0x05,0x00,0x0E,0x06,0x0A,0x00,0x04,0x09,0x01,0x00,0x04,0x09,0x01,
	0x00,0x04,0x09,0x01,0x00,0x0C,0x05,0x01,0x00,0x0E,0x05,0x01,0x00,0x0D,0x05,0x01,
	0x00,0x0B,0x05,0x01,0x00,0x01,0x0F,0x04,0x00,0x01,0x0F,0x04,0x00,0x01,0x0F,0x04,
	0x00,0x08,0x07,0x0C,0x00,0x01,0x0F,0x04,0x00,0x01,0x0F,0x04,0x00,0x0A,0x05,0x01,
	0x00,0x05,0x09,0x01,0x00,0x0B,0x0D,0x05,0x00,0x06,0x05,0x01,0x00,0x0A,0x03,0x01,
	0x00,0x0C,0x03,0x01,0x00,0x0E,0x03,0x01,0x00,0x0D,0x03,0x01,0x00,0x0B,0x03,0x01,
	0x00,0x0E,0x0C,0x0F,0x00,0x05,0x03,0x01,0x00,0x0E,0x06,0x09,0x00,0x04,0x09,0x05,
	0x00,0x09,0x0E,0x06,0x00,0x04,0x0E,0x05,0x00,0x09,0x0E,0x05,0x00,0x0B,0x05,0x01,
	0x00,0x0C,0x07,0x01,0x00,0x05,0x0F,0x09,0x00,0x05,0x04,0x09,0x00,0x0A,0x03,0x01,
	0x00,0x0C,0x02,0x03,0x00,0x0C,0x06,0x03,0x00,0x0C,0x06,0x09,0x00,0x0C,0x06,0x01,
	0x00,0x0E,0x06,0x0C,0x00,0x0C,0x02,0x0F,0x00,0x0C,0x02,0x09,0x00,0x0C,0x02,0x01,
	0x00,0x01,0x08,0x0F,0x00,0x0E,0x06,0x0F,0x00,0x09,0x0A,0x0F,0x00,0x05,0x06,0x0F,
	0x00,0x0B,0x09,0x05,0x00,0x0A,0x06,0x0C,0x00,0x0A,0x06,0x09,0x00,0x0A,0x02,0x09,
	0x00,0x06,0x01,0x0F,0x00,0x04,0x01,0x0F,0x00,0x0A,0x03,0x01,0x00,0x0A,0x02,0x0C,
	0x00,0x05,0x09,0x01,0x00,0x0A,0x02,0x01,0x00,0x0E,0x02,0x09,0x00,0x0E,0x02,0x0C,
	0x00,0x01,0x04,0x0F,0x00,0x01,0x04,0x0F,0x00,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz ? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			timeplt_sh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32,32*16,
	timeplt_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	timeplt_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	timeplt_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( timeplt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tm1", 0x0000, 0x2000 )
	ROM_LOAD( "tm2", 0x2000, 0x2000 )
	ROM_LOAD( "tm3", 0x4000, 0x2000 )

	ROM_REGION(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tm6", 0x0000, 0x2000 )
	ROM_LOAD( "tm4", 0x2000, 0x2000 )
	ROM_LOAD( "tm5", 0x4000, 0x2000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tm7", 0x0000, 0x1000 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xab09],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0xab29],"\x00\x43\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0xab08],1,8*5,f);
			RAM[0xa98b] = RAM[0xab09];
			RAM[0xa98c] = RAM[0xab0a];
			RAM[0xa98d] = RAM[0xab0b];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0xab08],1,8*5,f);
		fclose(f);
	}
}



struct GameDriver timeplt_driver =
{
	"Time Pilot",
	"timeplt",
	"NICOLA SALMORIA\nALAN J MCCORMICK\nMIKE CUDDY",
	&machine_driver,

	timeplt_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,

	{ 0x13,0x49,0xa8,0xcd,0xf3,0xae,0x42,0xb0,0x17,0x5d,	/* numbers */
		0x74,0x8c,0x77,0x87,0x34,0x00,0x39,0xc4,0x67,0x21,0x7a,0xc5,0x38,	/* letters */
		0x3b,0x68,0x88,0x2f,0x5f,0x9f,0x6d,0x0d,0x0e,0x0f,0x1f,0x89,0xa9 },
	16, 18,
	8*13, 8*16, 20,

	hiload, hisave
};

struct GameDriver spaceplt_driver =
{
	"Space Pilot",
	"spaceplt",
	"NICOLA SALMORIA\nALAN J MCCORMICK\nMIKE CUDDY",
	&machine_driver,

	timeplt_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x13,0x49,0xa8,0xcd,0xf3,0xae,0x42,0xb0,0x17,0x5d,	/* numbers */
		0x74,0x8c,0x77,0x87,0x34,0x00,0x39,0xc4,0x67,0x21,0x7a,0xc5,0x38,	/* letters */
		0x3b,0x68,0x88,0x2f,0x5f,0x9f,0x6d,0x0d,0x0e,0x0f,0x1f,0x89,0xa9 },
	16, 18,
	8*13, 8*16, 20,

	hiload, hisave
};
