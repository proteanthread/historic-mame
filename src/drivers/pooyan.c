/***************************************************************************

Pooyan memory map (preliminary)

Thanks must go to Mike Cuddy for providing information on this one.

Sound processor memory map.
0x3000-0x33ff RAM.
AY-8910 #1 : reg 0x5000
	     wr  0x4000
             rd  0x4000

AY-8910 #2 : reg 0x7000
	     wr  0x6000
             rd  0x6000

Main processor memory map.
0000-7fff ROM
8000-83ff color RAM
8400-87ff video RAM
8800-8fff RAM
9000-97ff sprite RAM (only areas 0x9010 and 0x9410 are used).

memory mapped ports:

read:
0xA000	Dipswitch 2 adddbtll
        a = attract mode
        ddd = difficulty 0=easy, 7=hardest.
        b = bonus setting (easy/hard)
        t = table / upright
        ll = lives: 11=3, 10=4, 01=5, 00=255.

0xA0E0  llllrrrr
        l == left coin mech, r = right coinmech.

0xA080	IN0 Port
0xA0A0  IN1 Port
0xA0C0	IN2 Port

write:
0xA100	command for the audio CPU.
0xA180	NMI enable. (0xA180 == 1 = deliver NMI to CPU).

0xA181	interrupt trigger on audio CPU.

0xA183	????

0xA184	????

0xA187	Watchdog reset.

interrupts:
standard NMI at 0x66

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern void pooyan_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void pooyan_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int pooyan_sh_interrupt(void);
extern int pooyan_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x8fff, MRA_RAM },	/* color and video RAM */
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa080, 0xa080, input_port_0_r },	/* IN0 */
	{ 0xa0a0, 0xa0a0, input_port_1_r },	/* IN1 */
	{ 0xa0c0, 0xa0c0, input_port_2_r },	/* IN2 */
	{ 0xa0e0, 0xa0e0, input_port_3_r },	/* DSW1 */
	{ 0xa000, 0xa000, input_port_4_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8800, 0x8fff, MWA_RAM },
	{ 0x8000, 0x83ff, colorram_w, &colorram },
	{ 0x8400, 0x87ff, videoram_w, &videoram },
	{ 0x9010, 0x903f, MWA_RAM, &spriteram },
	{ 0x9410, 0x943f, MWA_RAM, &spriteram_2 },
	{ 0xa180, 0xa180, interrupt_enable_w },
	{ 0xa187, 0xa187, MWA_NOP },
	{ 0xa100, 0xa100, sound_command_w },
        { 0xa028, 0xa028, MWA_RAM },
        { 0xa000, 0xa000, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
        { 0x8000, 0x8fff, MRA_RAM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
        { 0x8000, 0x8fff, MWA_RAM },
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
        { 1, 4, "FIRE"      },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x03, "LIVES", { "255", "5", "4", "3" }, 1 },
	{ 4, 0x08, "BONUS", { "30000", "50000" } },
	{ 4, 0x70, "DIFFICULTY", { "HARDEST", "HARD", "DIFFICULT", "MEDIUM", "NORMAL", "EASIER", "EASY" , "EASIEST" }, 1 },
	{ 4, 0x80, "DEMO SOUNDS", { "ON", "OFF" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0x1000*8+4, 0x1000*8+0, 4, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	4,	/* 4 bits per pixel */
	{ 0x1000*8+4, 0x1000*8+0, 4, 0 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x2000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



/* these are NOT the original color PROMs */
static unsigned char color_prom[] =
{
	/* char palette */
	0x0F,0xE1,0x1C,0x03,0xFF,0xF7,0xD0,0x88,0xB0,0xD4,0x1C,0x10,0xFD,0x41,0x92,0xFB,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x10,0xFC,0x49,0x92,0xFF,
	0x0F,0xE1,0x02,0x03,0x08,0xA0,0xD0,0x88,0xB0,0xD4,0x1C,0x10,0xFC,0x69,0x92,0xFB,
	0x0F,0x01,0x02,0x03,0xFC,0x10,0x3C,0x88,0xB0,0xD4,0x1C,0xB0,0xFC,0x49,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x1C,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0xFD,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x81,0x02,0x1C,0xE8,0xFC,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0xE0,0x90,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0xFC,0x02,0x03,0xE1,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0x08,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0x01,0x02,0x03,0xE0,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFF,
	0x0F,0xDC,0x06,0x1C,0xFF,0xE0,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0xFD,0x92,0xFF,
	0x0F,0xDC,0x02,0x03,0xFF,0x10,0x1C,0x88,0xB0,0xD4,0x1C,0x90,0xFC,0x49,0x92,0xFB,

	/* Sprite palette */
	0x69,0xCD,0xB0,0x03,0xC1,0xF7,0xD4,0xE9,0x6E,0xD0,0x5D,0x30,0xFF,0x00,0xE3,0xFB,
	0x48,0xCC,0x02,0x03,0xC0,0x10,0xFC,0x09,0xD0,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0x01,0x22,0xFF,0x08,0x10,0x0F,0xFF,0x12,0x17,0xDF,0xFF,0xFC,0x09,0xFF,0x0F,
	0x00,0x01,0x21,0x0F,0x08,0x10,0x0F,0xFF,0x12,0x1F,0x0F,0xFF,0xFC,0x29,0x6F,0xFF,
	0x48,0xD6,0x02,0x03,0xC0,0x10,0xFC,0x09,0xB0,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0x01,0x02,0x03,0xD7,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0xD8,0x92,0xFF,
	0x48,0xDB,0xA0,0x03,0xE0,0xB7,0xDA,0x09,0x14,0x1C,0xDE,0x90,0x98,0x49,0x92,0xFF,
	0x48,0xDB,0xB6,0x03,0xE0,0x10,0xA1,0x80,0x12,0x1C,0xDE,0x92,0xB5,0x49,0x92,0xFF,
	0x48,0x01,0x02,0x0F,0x08,0x10,0xFF,0xFF,0x12,0x1F,0xDE,0xFF,0xFC,0x49,0x0F,0xFF,
	0x48,0xF8,0x02,0x03,0xE0,0x10,0xFC,0x09,0xF4,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0xEF,0x02,0x01,0x08,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0xFC,0x02,0x03,0x08,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0xE0,0x02,0x03,0x08,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0x48,0x92,0xFF,
	0x48,0x01,0x02,0x03,0x08,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0x48,0x92,0xFF,
	0x48,0xE3,0x02,0x03,0x08,0x10,0x1C,0x09,0x12,0x1F,0xDE,0x90,0xFC,0x49,0x92,0xFF,
	0x48,0x1B,0x02,0x03,0xA8,0xF7,0x1C,0x09,0x12,0x1F,0x00,0x90,0xFC,0x49,0x92,0xFF
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
			3072000,	/* 3.0782 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			pooyan_sh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,32*16,
	pooyan_vh_convert_color_prom,

	0,
	generic_vh_start,
	generic_vh_stop,
	pooyan_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	pooyan_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pooyan_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic22_a4.cpu",  0x0000, 0x2000 )
	ROM_LOAD( "ic23_a5.cpu",  0x2000, 0x2000 )
	ROM_LOAD( "ic24_a6.cpu",  0x4000, 0x2000 )
	ROM_LOAD( "ic25_a7.cpu",  0x6000, 0x2000 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic13_g10.cpu", 0x0000, 0x1000 )
	ROM_LOAD( "ic14_g9.cpu",  0x1000, 0x1000 )
	ROM_LOAD( "ic16_a8.cpu",  0x2000, 0x1000 )
	ROM_LOAD( "ic15_a9.cpu",  0x3000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sd01_a7.snd",  0x0000, 0x1000 )
	ROM_LOAD( "sd02_a8.snd",  0x1000, 0x1000 )
ROM_END



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8a00],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x8a1b],"\x00\x00\x01",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x89c0],1,3*10,f);
			fread(&RAM[0x8a00],1,3*10,f);
			fread(&RAM[0x8e00],1,3*10,f);
			RAM[0x88a8] = RAM[0x8a00];
			RAM[0x88a9] = RAM[0x8a01];
			RAM[0x88aa] = RAM[0x8a02];
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
		fwrite(&RAM[0x89c0],1,3*10,f);
		fwrite(&RAM[0x8a00],1,3*10,f);
		fwrite(&RAM[0x8e00],1,3*10,f);
		fclose(f);
	}
}



struct GameDriver pooyan_driver =
{
	"Pooyan",
	"pooyan",
	"MIKE CUDDY\nALLARD VAN DER BAS\nNICOLA SALMORIA",
	&machine_driver,

	pooyan_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,	/* letters */
		0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a },
	0x00, 0x03,
	8*13, 8*16, 0x07,

	hiload, hisave
};
