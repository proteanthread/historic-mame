/***************************************************************************

Blasteroids Memory Map
----------------------

BLASTEROIDS 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-03FFFF  R    D0-D15
Slapstic Program ROM               038000-03FFFF  R    D0-D15

Watchdog reset                     FF8000         W    xx
IRQ Acknowledge                    FF8200         W    xx
VBLANK Acknowledge                 FF8400         W    xx
Unlock EEPROM                      FF8600         W    xx

Priority RAM (1=MO, 0=PF)          FF8800-FF89FE  W    D0

Audio Send Port                    FF8A01         W    D0-D7
Sound Processor Reset              FF8C00         W    xx
Halt CPU until HBLANK              FF8E00         W    xx

Audio Receive Port                 FF9401         R    D0-D7
Whirly-Gig (Player 1)              FF9801         R    D0-D7
Whirly-Gig (Player 2)              FF9805         R    D0-D7

Self-Test                          FF9C01         R    D7
Audio Busy Flag                                   R    D6
Vertical Blank                                    R    D5
Horizontal Blank                                  R    D4
Player 1 Button 4                                 R    D3
Player 1 Transform                                R    D2
Player 1 Thrust                                   R    D1
Player 1 Fire                                     R    D0

Player 2 Button 4                  FF9C03         R    D3
Player 2 Transform                                R    D2
Player 2 Thrust                                   R    D1
Player 2 Fire                                     R    D0

Color RAM Motion Object            FFA000-FFA1FE  R/W  D0-D14
Color RAM Playfield                FFA200-FFA2FE  R/W  D0-D14

EEPROM                             FFB001-FFB3FF  R/W  D0-D7

Playfield RAM (40x30)              FFC000-FFCFFF  R/W  D0-D15
Row Programmable Interrupt         FFC050-FFCED0  R/W  D15

Motion Object V Position           FFD000-FFDFF8  R/W  D7-D15
Motion Object V Size                              R/W  D0-D3
Motion Object H Flip               FFD002-FFDFFA  R/W  D15
Motion Object V Flip                              R/W  D14
Motion Object Stamp                               R/W  D0-D13
Motion Object Link                 FFD004-FFDFFC  R/W  D3-D11
Motion Object H Position           FFD006-FFDFFE  R/W  D6-D15
Motion Object Palette                             R/W  D0-D3

RAM                                FFE000-FFFFFF  R/W

****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/ataraud2.h"
#include "vidhrdw/generic.h"


int blstroid_irq_state;


int blstroid_playfieldram_r(int offset);

void blstroid_irq_ack_w(int offset, int data);
void blstroid_priorityram_w(int offset, int data);
void blstroid_playfieldram_w(int offset, int data);

int blstroid_vh_start(void);
void blstroid_vh_stop(void);
void blstroid_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void blstroid_update_display_list(int scanline);



/*************************************
 *
 *		Initialization
 *
 *************************************/

static void blstroid_update_interrupts(int vblank, int sound)
{
	int newstate = 0;

	if (blstroid_irq_state)
		newstate = 1;
	if (vblank)
		newstate = 2;
	if (sound)
		newstate = 4;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


void blstroid_init_machine(void)
{
	atarigen_init_machine(blstroid_update_interrupts, 0);
	ataraud2_reset(1, 4, 2, 0x80);

	blstroid_irq_state = 0;
}



/*************************************
 *
 *		Interrupt handlers
 *
 *************************************/

int blstroid_interrupt(void)
{
	/* set a timer to reset the video parameters just before the end of VBLANK */
	timer_set(TIME_IN_USEC(Machine->drv->vblank_duration - 10), 0, blstroid_update_display_list);
	return atarigen_vblank_gen();
}



/*************************************
 *
 *		I/O read dispatch
 *
 *************************************/

int blstroid_io_r(int offset)
{
	if (offset == 0)
	{
		static int hblank = 0x10;
		int temp = input_port_2_r(offset);

		if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
		temp ^= hblank ^= 0x10;

		return temp | 0xff00;
	}
	else
		return input_port_3_r(offset) | 0xff00;
}



/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress blstroid_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0xff9400, 0xff9403, atarigen_sound_r },
	{ 0xff9800, 0xff9803, input_port_0_r },
	{ 0xff9804, 0xff9807, input_port_1_r },
	{ 0xff9c00, 0xff9c03, blstroid_io_r },
	{ 0xffa000, 0xffa3ff, paletteram_word_r },
	{ 0xffb000, 0xffb3ff, atarigen_eeprom_r },
	{ 0xffc000, 0xffcfff, blstroid_playfieldram_r },
	{ 0xffd000, 0xffdfff, MRA_BANK3 },
	{ 0xffe000, 0xffffff, MRA_BANK2 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress blstroid_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xff8000, 0xff8003, watchdog_reset_w },
	{ 0xff8200, 0xff8203, blstroid_irq_ack_w },
	{ 0xff8400, 0xff8403, atarigen_vblank_ack_w },
	{ 0xff8600, 0xff8603, atarigen_eeprom_enable_w },
	{ 0xff8800, 0xff89ff, blstroid_priorityram_w },
	{ 0xff8a00, 0xff8a03, atarigen_sound_w },
	{ 0xff8c00, 0xff8c03, atarigen_sound_reset_w },
	{ 0xff8e00, 0xff8e03, MWA_NOP },		/* halt until HBLANK */
	{ 0xffa000, 0xffa3ff, paletteram_xRRRRRGGGGGBBBBB_word_w, &paletteram },
	{ 0xffb000, 0xffb3ff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffc000, 0xffcfff, blstroid_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xffd000, 0xffdfff, MWA_BANK3, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xffe000, 0xffffff, MWA_BANK2 },
	{ -1 }  /* end of table */
};


/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( blstroid_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0xff, 0, IPT_DIAL | IPF_PLAYER1, 100, 0xff, 0, 0 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0xff, 0, IPT_DIAL | IPF_PLAYER2, 100, 0xff, 0, 0 )

	PORT_START		/* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	ATARI_AUDIO_2_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout blstroid_pflayout =
{
	16,8,	/* 16*8 chars (doubled horizontally) */
	8192,	/* 8192 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxLayout blstroid_molayout =
{
	16,8,	/* 16*8 chars */
	16384,	/* 16384 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0x80000*8+0, 0x80000*8+4, 0, 4, 0x80000*8+8, 0x80000*8+12, 8, 12,
			0x80000*8+16, 0x80000*8+20, 16, 20, 0x80000*8+24, 0x80000*8+28, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo blstroid_gfxdecodeinfo[] =
{
	{ 2, 0x00000, &blstroid_pflayout,  256, 16 },
	{ 2, 0x40000, &blstroid_molayout,    0, 16 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver blstroid_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			blstroid_readmem,blstroid_writemem,0,0,
			blstroid_interrupt,1
		},
		{
			ATARI_AUDIO_2_CPU(1)
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	blstroid_init_machine,

	/* video hardware */
	80*8, 30*8, { 0*8, 80*8-1, 0*8, 30*8-1 },
	blstroid_gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_SUPPORTS_DIRTY | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	blstroid_vh_start,
	blstroid_vh_stop,
	blstroid_vh_screenrefresh,

	/* sound hardware */
	ATARI_AUDIO_2_INTERFACES
};



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( blstroid_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "057-4123",  0x00000, 0x10000, 0xd14badc4 )
	ROM_LOAD_ODD ( "057-4121",  0x00000, 0x10000, 0xae3e93e8 )
	ROM_LOAD_EVEN( "057-4124",  0x20000, 0x10000, 0xfd2365df )
	ROM_LOAD_ODD ( "057-4122",  0x20000, 0x10000, 0xc364706e )

	ROM_REGION(0x14000)	/* 64k for 6502 code */
	ROM_LOAD( "blstroid.snd", 0x10000, 0x4000, 0xbaa8b5fe )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION_DISPOSE(0x140000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "blstroid.1l",  0x000000, 0x10000, 0x3c2daa5b ) /* playfield */
	ROM_LOAD( "blstroid.1m",  0x010000, 0x10000, 0xf84f0b97 ) /* playfield */
	ROM_LOAD( "blstroid.3l",  0x020000, 0x10000, 0xae5274f0 ) /* playfield */
	ROM_LOAD( "blstroid.3m",  0x030000, 0x10000, 0x4bb72060 ) /* playfield */
	ROM_LOAD( "blstroid.5m",  0x040000, 0x10000, 0x50e0823f ) /* mo */
	ROM_LOAD( "blstroid.6m",  0x050000, 0x10000, 0x729de7a9 ) /* mo */
	ROM_LOAD( "blstroid.8m",  0x060000, 0x10000, 0x090e42ab ) /* mo */
	ROM_LOAD( "blstroid.10m", 0x070000, 0x10000, 0x1ff79e67 ) /* mo */
	ROM_LOAD( "blstroid.11m", 0x080000, 0x10000, 0x4be1d504 ) /* mo */
	ROM_LOAD( "blstroid.13m", 0x090000, 0x10000, 0xe4409310 ) /* mo */
	ROM_LOAD( "blstroid.14m", 0x0a0000, 0x10000, 0x7aaca15e ) /* mo */
	ROM_LOAD( "blstroid.16m", 0x0b0000, 0x10000, 0x33690379 ) /* mo */
	ROM_LOAD( "blstroid.5n",  0x0c0000, 0x10000, 0x2720ee71 ) /* mo */
	ROM_LOAD( "blstroid.6n",  0x0d0000, 0x10000, 0x2faecd15 ) /* mo */
	ROM_LOAD( "blstroid.8n",  0x0e0000, 0x10000, 0xf10e59ed ) /* mo */
	ROM_LOAD( "blstroid.10n", 0x0f0000, 0x10000, 0x4d5fc284 ) /* mo */
	ROM_LOAD( "blstroid.11n", 0x100000, 0x10000, 0xa70fc6e6 ) /* mo */
	ROM_LOAD( "blstroid.13n", 0x110000, 0x10000, 0xf423b4f8 ) /* mo */
	ROM_LOAD( "blstroid.14n", 0x120000, 0x10000, 0x56fa3d16 ) /* mo */
	ROM_LOAD( "blstroid.16n", 0x130000, 0x10000, 0xf257f738 ) /* mo */
ROM_END

ROM_START( blstroi2_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "blstroid.6c",  0x00000, 0x10000, 0x5a092513 )
	ROM_LOAD_ODD ( "blstroid.6b",  0x00000, 0x10000, 0x486aac51 )
	ROM_LOAD_EVEN( "blstroid.4c",  0x20000, 0x10000, 0xd0fa38fe )
	ROM_LOAD_ODD ( "blstroid.4b",  0x20000, 0x10000, 0x744bf921 )

	ROM_REGION(0x14000)	/* 64k for 6502 code */
	ROM_LOAD( "blstroid.snd", 0x10000, 0x4000, 0xbaa8b5fe )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION_DISPOSE(0x140000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "blstroid.1l",  0x000000, 0x10000, 0x3c2daa5b ) /* playfield */
	ROM_LOAD( "blstroid.1m",  0x010000, 0x10000, 0xf84f0b97 ) /* playfield */
	ROM_LOAD( "blstroid.3l",  0x020000, 0x10000, 0xae5274f0 ) /* playfield */
	ROM_LOAD( "blstroid.3m",  0x030000, 0x10000, 0x4bb72060 ) /* playfield */
	ROM_LOAD( "blstroid.5m",  0x040000, 0x10000, 0x50e0823f ) /* mo */
	ROM_LOAD( "blstroid.6m",  0x050000, 0x10000, 0x729de7a9 ) /* mo */
	ROM_LOAD( "blstroid.8m",  0x060000, 0x10000, 0x090e42ab ) /* mo */
	ROM_LOAD( "blstroid.10m", 0x070000, 0x10000, 0x1ff79e67 ) /* mo */
	ROM_LOAD( "blstroid.11m", 0x080000, 0x10000, 0x4be1d504 ) /* mo */
	ROM_LOAD( "blstroid.13m", 0x090000, 0x10000, 0xe4409310 ) /* mo */
	ROM_LOAD( "blstroid.14m", 0x0a0000, 0x10000, 0x7aaca15e ) /* mo */
	ROM_LOAD( "blstroid.16m", 0x0b0000, 0x10000, 0x33690379 ) /* mo */
	ROM_LOAD( "blstroid.5n",  0x0c0000, 0x10000, 0x2720ee71 ) /* mo */
	ROM_LOAD( "blstroid.6n",  0x0d0000, 0x10000, 0x2faecd15 ) /* mo */
	ROM_LOAD( "blstroid.8n",  0x0e0000, 0x10000, 0xf10e59ed ) /* mo */
	ROM_LOAD( "blstroid.10n", 0x0f0000, 0x10000, 0x4d5fc284 ) /* mo */
	ROM_LOAD( "blstroid.11n", 0x100000, 0x10000, 0xa70fc6e6 ) /* mo */
	ROM_LOAD( "blstroid.13n", 0x110000, 0x10000, 0xf423b4f8 ) /* mo */
	ROM_LOAD( "blstroid.14n", 0x120000, 0x10000, 0x56fa3d16 ) /* mo */
	ROM_LOAD( "blstroid.16n", 0x130000, 0x10000, 0xf257f738 ) /* mo */
ROM_END


/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver blstroid_driver =
{
	__FILE__,
	0,
	"blstroid",
	"Blasteroids (version 4)",
	"1987",
	"Atari Games",
	"Aaron Giles (MAME driver)\nNeil Bradley (Hardware Info)",
	0,
	&blstroid_machine_driver,
	0,

	blstroid_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	blstroid_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver blstroi2_driver =
{
	__FILE__,
	&blstroid_driver,
	"blstroi2",
	"Blasteroids (version 2)",
	"1987",
	"Atari Games",
	"Aaron Giles (MAME driver)\nNeil Bradley (Hardware Info)",
	0,
	&blstroid_machine_driver,
	0,

	blstroi2_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	blstroid_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
