/***************************************************************************

	Haunted Castle
	Haunted Castle (Alt)

	Notes:
		Video hardware is a real bitch ;)
		Graphics banking is only tested up to the end of level 2
		Konami SCC sound is not implemented.

	Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/konami/konami.h"
#include "cpu/z80/z80.h"

void hcastle_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
int hcastle_vh_start (void);
void hcastle_vh_stop (void);

extern unsigned char *hcastle_pf1_control,*hcastle_pf2_control;
extern unsigned char *hcastle_pf1_videoram,*hcastle_pf2_videoram;

void hcastle_pf1_video_w(int offset, int data);
void hcastle_pf2_video_w(int offset, int data);
void hcastle_gfxbank_w(int offset, int data);

static void hcastle_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x1f) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static void hcastle_sound_w(int offset, int data)
{
	if (offset==0) soundlatch_w(0,data);
	else cpu_cause_interrupt( 1, Z80_IRQ_INT );
}

static void hcastle_coin_w(int offset, int data)
{
	coin_counter_w(0,data & 0x40);
	coin_counter_w(1,data & 0x80);
}

static int speedup_r( int offs )
{
	unsigned char *RAM = Machine->memory_region[0];

	int data = ( RAM[0x18dc] << 8 ) | RAM[0x18dd];

	if ( data < Machine->memory_region_length[0] )
	{
		data = ( RAM[data] << 8 ) | RAM[data + 1];

		if ( data == 0xffff )
			cpu_spinuntil_int();
	}

	return RAM[0x18dc];
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x003f, MRA_RAM },
	{ 0x0200, 0x023f, MRA_RAM },
	{ 0x0410, 0x0410, input_port_0_r },
	{ 0x0411, 0x0411, input_port_1_r },
	{ 0x0412, 0x0412, input_port_2_r },
	{ 0x0413, 0x0413, input_port_5_r }, /* Dip 3 */
	{ 0x0414, 0x0414, input_port_4_r }, /* Dip 2 */
	{ 0x0415, 0x0415, input_port_3_r }, /* Dip 1 */
	{ 0x0600, 0x06ff, paletteram_r },
	{ 0x18dc, 0x18dc, speedup_r },
	{ 0x0700, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x003f, MWA_RAM, &hcastle_pf2_control },
	{ 0x0200, 0x023f, MWA_RAM, &hcastle_pf1_control },
	{ 0x0400, 0x0400, hcastle_bankswitch_w },
	{ 0x0404, 0x040b, hcastle_sound_w },
	{ 0x040c, 0x040c, watchdog_reset_w },
	{ 0x0410, 0x0410, hcastle_coin_w },
	{ 0x0418, 0x0418, hcastle_gfxbank_w },
	{ 0x0600, 0x06ff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
	{ 0x0700, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, hcastle_pf2_video_w, &hcastle_pf2_videoram },
	{ 0x3000, 0x3fff, MWA_RAM, &spriteram_2 },
	{ 0x4000, 0x4fff, hcastle_pf1_video_w, &hcastle_pf1_videoram },
	{ 0x5000, 0x5fff, MWA_RAM, &spriteram },
 	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/*****************************************************************************/

static void sound_bank_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[3];
	int bank_A=0x20000 * (data&0x3);
	int bank_B=0x20000 * ((data>>2)&0x3);

	K007232_bankswitch_0_w(RAM+bank_A,RAM+bank_B);
}

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM3812_status_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x98ff, MWA_NOP }, /* SCC */
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, sound_bank_w }, /* 7232 bankswitch */
	{ -1 }	/* end of table */
};

/*****************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, "Difficulty 2" )
	PORT_DIPSETTING(    0x00, "Very Easy" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x60, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
//	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), OSD_KEY_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*****************************************************************************/

static struct GfxLayout char_8x8 =
{
	8,8,
	16384 * 4,
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8,12,0,4, 24,28, 16,20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sdprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &char_8x8,    0, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/

static void irqhandler(int linestate)
{
//	cpu_set_irq_line(1,0,linestate);static void irqhandler(void)
}

static struct K007232_interface k007232_interface =
{
	1,		/* Number of chips */
	{ 3 },	/* Memory region */
	{ 40 }	/* Volume */
};

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,
	{ 35 },
	{ irqhandler },
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_KONAMI,
			4000000,	/* Derived from 24 MHz clock */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	hcastle_vh_start,
	hcastle_vh_stop,
	hcastle_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_K007232,
			&k007232_interface,
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

/***************************************************************************/

ROM_START( hcastle_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "768.k03",   0x08000, 0x08000, 0x40ce4f38 )
	ROM_LOAD( "768.g06",   0x10000, 0x20000, 0xcdade920 )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d91.j5",   0x000000, 0x80000, 0x2960680e )
	ROM_LOAD( "d92.j6",   0x080000, 0x80000, 0x65a2f227 )
	ROM_LOAD( "d94.g19",  0x100000, 0x80000, 0x9633db8b )
	ROM_LOAD( "d95.g21",  0x180000, 0x80000, 0xe3be3fdd )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",   0x00000, 0x08000, 0xb9fff184 )

	ROM_REGION(0x80000)	/* 512k for the samples */
	ROM_LOAD( "d93.e17",  0x00000, 0x80000, 0x01f9889c )
ROM_END

ROM_START( hcastlea_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "m03.k12",   0x08000, 0x08000, 0xd85e743d )
	ROM_LOAD( "b06.k8",    0x10000, 0x20000, 0xabd07866 )

	ROM_REGION_DISPOSE(0x200000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d91.j5",   0x000000, 0x80000, 0x2960680e )
	ROM_LOAD( "d92.j6",   0x080000, 0x80000, 0x65a2f227 )
	ROM_LOAD( "d94.g19",  0x100000, 0x80000, 0x9633db8b )
	ROM_LOAD( "d95.g21",  0x180000, 0x80000, 0xe3be3fdd )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",   0x00000, 0x08000, 0xb9fff184 )

	ROM_REGION(0x80000)	/* 512k for the samples */
	ROM_LOAD( "d93.e17",  0x00000, 0x80000, 0x01f9889c )
ROM_END

/***************************************************************************/

struct GameDriver hcastle_driver =
{
	__FILE__,
	0,
	"hcastle",
	"Haunted Castle (set 1)",
	"1988",
	"Konami",
	"Bryan McPhail",
	0,
	&machine_driver,
	0,

	hcastle_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver hcastlea_driver =
{
	__FILE__,
	&hcastle_driver,
	"hcastlea",
	"Haunted Castle (set 2)",
	"1988",
	"Konami",
	"Bryan McPhail",
	0,
	&machine_driver,
	0,

	hcastlea_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};