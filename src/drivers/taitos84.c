/***************************************************************************

This driver supports the following games:

Gyrodine - (c) 1984 Taito Corporation.
Son of Phoenix - (c) 1985 Associated Overseas MFR, Inc.
SRD Mission - (c) 1986 Taito Corporation.


Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

/* from vidhrdw */
extern unsigned char *taitos84_videoram;
extern int taitos84_videoram_size;
extern unsigned char *taitos84_back_scrollY_lo;
extern unsigned char *taitos84_back_scrollX;
void taitos84_gfxctrl_w(int offset,int data);
void taitos84_flipscreen_w(int offset,int data);
void taitos84_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void taitos84_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);


static unsigned char *shared_ram;

static int shared_ram_r( int offs )
{
	return shared_ram[ offs ];
}

static void shared_ram_w( int offs, int data )
{
	shared_ram[ offs ] = data;
}

static int special_spriteram_r( int offs )
{
	/* RAM is 4 bits wide, must set the high bits to 1 for the RAM test to pass */
	return spriteram_2[ offs] | 0xf0;
}



/* time to abuse the preprocessor for the memory/port maps */

#define Main_MemMap( name, shared, watchdog )										\
	static struct MemoryReadAddress name##_readmem[] = {							\
		{ 0x0000, 0x7fff, MRA_ROM },												\
		{ 0x8000, 0x87ff, videoram_r }, /* background tiles */						\
		{ 0x8800, 0x8fff, colorram_r }, /* background color */						\
		{ 0x9000, 0x97ff, MRA_RAM }, /* foreground tiles */							\
		{ 0x9800, 0x9fff, special_spriteram_r },	/* 4 bits wide */				\
		{ 0xa000, 0xa7ff, MRA_RAM }, /* sprites */									\
		{ shared, shared+0x7ff, shared_ram_r }, /* shared RAM */					\
		{ 0xf800, 0xffff, shared_ram_r }, /* shared mirror always here */			\
		{ -1 }	/* end of table */													\
	};																				\
																					\
	static struct MemoryWriteAddress name##_writemem[] = {							\
		{ 0x0000, 0x7fff, MWA_ROM },												\
		{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },					\
		{ 0x8800, 0x8fff, colorram_w, &colorram },									\
		{ 0x9000, 0x97ff, MWA_RAM, &taitos84_videoram, &taitos84_videoram_size },	\
		{ 0x9800, 0x9fff, MWA_RAM, &spriteram_2 },	/* 4 bits wide */				\
		{ 0xa000, 0xa7ff, MWA_RAM, &spriteram, &spriteram_size },					\
		{ 0xa800, 0xa800, MWA_RAM, &taitos84_back_scrollY_lo }, /* back scroll Y */	\
		{ 0xb000, 0xb000, taitos84_gfxctrl_w }, /* back scroll MSB + other stuff */	\
		{ 0xb800, 0xb800, MWA_RAM, &taitos84_back_scrollX }, /* back scroll X */	 \
		{ shared, shared+0x7ff, shared_ram_w, &shared_ram }, /* shared RAM */		\
		{ 0xf800, 0xffff, shared_ram_w }, /* shared mirror always here */			\
		{ watchdog, watchdog, watchdog_reset_w },								\
		{ -1 }	/* end of table */													\
	};

#define Sub_MemMap( name, rom_end, shared, extra_ram, in0, in1, in2 )	\
	static struct MemoryReadAddress name##_sub_readmem[] = {			\
		{ 0x0000, rom_end, MRA_ROM },									\
		{ shared, shared+0x7ff, shared_ram_r }, /* shared RAM */		\
		{ extra_ram, extra_ram+0x7ff, MRA_RAM }, /* extra RAM */		\
		{ in0, in0, input_port_2_r }, /* input port */					\
		{ in1, in1, input_port_3_r }, /* input port */					\
		{ in2, in2, input_port_4_r }, /* input port */					\
		{ -1 }	/* end of table */										\
	};																	\
																		\
	static struct MemoryWriteAddress name##_sub_writemem[] = {			\
		{ 0x0000, rom_end, MWA_ROM },									\
		{ shared, shared+0x7ff, shared_ram_w }, /* shared RAM */		\
		{ extra_ram, extra_ram+0x7ff, MWA_RAM }, /* extra RAM */		\
		{ -1 }	/* end of table */										\
	};

/* actual definitions */
Main_MemMap( gyrodine, 0xf000, 0xe000 )
Main_MemMap( sonofphx, 0xf000, 0x0000 )
Main_MemMap( srdmissn, 0xe000, 0x0000 )
Sub_MemMap( gyrodine, 0x1fff, 0x4000, 0x0000, 0x8080, 0x8040, 0x8000 )
Sub_MemMap( sonofphx, 0x7fff, 0xa000, 0x0000, 0xc080, 0xc040, 0xc000 )
Sub_MemMap( srdmissn, 0x7fff, 0x8000, 0x8800, 0xf400, 0xf401, 0xf402 )

static void sub_cpu_control_w( int offs, int data ) {

	if ( data & 1 ) {
		cpu_halt( 1, 1 );
		cpu_reset( 1 );
	} else
		cpu_halt( 1, 0 );

}

#define Main_PortMap( name, base )								\
	static struct IOReadPort name##_readport[] = {				\
		{ -1 }	/* end of table */								\
	};															\
																\
	static struct IOWritePort name##_writeport[] = {			\
		{ base+0, base+0, interrupt_enable_w },					\
		{ base+1, base+1, taitos84_flipscreen_w },				\
		{ base+2, base+2, sub_cpu_control_w },					\
		{ -1 }	/* end of table */								\
	};

#define Sub_PortMap( name, ay0_base, ay1_base )					\
	static struct IOReadPort name##_sub_readport[] = {			\
		{ ay0_base+2, ay0_base+2, AY8910_read_port_0_r },		\
		{ -1 }	/* end of table */								\
	};															\
																\
	static struct IOWritePort name##_sub_writeport[] = {		\
		{ ay0_base+0, ay0_base+0, AY8910_control_port_0_w },	\
		{ ay0_base+1, ay0_base+1, AY8910_write_port_0_w },		\
		{ ay1_base+0, ay1_base+0, AY8910_control_port_1_w },	\
		{ ay1_base+1, ay1_base+1, AY8910_write_port_1_w },		\
		{ -1 }	/* end of table */								\
	};

/* actual definitions */
Main_PortMap( gyrodine, 0x00 )
Main_PortMap( sonofphx, 0x00 )
Main_PortMap( srdmissn, 0x08 )
Sub_PortMap( gyrodine, 0x00, 0xc0 )
Sub_PortMap( sonofphx, 0x00, 0x40 )
Sub_PortMap( srdmissn, 0x80, 0x84 )

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( gyrodine_input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( input_ports )	/* sonofphx and lstmissn */
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Slow Motion" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x10,     0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Sound Test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************

	Graphics Decoding

***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,           /* 8*8 characters */
	256,           /* 256 characters */
	2,             /* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8           /* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,		/* 3 bits per pixel */
	{ 0*1024*32*8, 1*1024*32*8, 2*1024*32*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout = {
	8,8,	/* 16*16 tiles */
	1024,	/* 1024 tiles */
	3,		/* 3 bits per pixel */
	{ 0*1024*8*8, 1*1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,		0, 64 },
	{ 1, 0x01000, &spritelayout,	0, 32 },
	{ 1, 0x19000, &tilelayout,		0, 32 },
	{ -1 } /* end of array */
};



/***************************************************************************

	Sound Interface

***************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? */
	{ 40, 40 },
	AY8910_DEFAULT_GAIN,
	{ input_port_0_r, 0 },
	{ input_port_1_r, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

/***************************************************************************

	Machine Driver

***************************************************************************/

#define Machine_Driver( name ) \
	static struct MachineDriver name##_machine_driver = { \
		/* basic machine hardware */																\
		{																							\
			{																						\
				CPU_Z80,																			\
				18432000 / 4,	/* 18.432 MHz crystal */											\
				0,																					\
				name##_readmem,name##_writemem,name##_readport,name##_writeport,					\
				nmi_interrupt,1																		\
			},																						\
			{																						\
				CPU_Z80,																			\
				18432000 / 4,	/* 18.432 MHz crystal */											\
				3,																					\
				name##_sub_readmem,name##_sub_writemem,name##_sub_readport,name##_sub_writeport,	\
				interrupt,4																			\
			}																						\
		},																							\
		60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */			\
		100,	/* cpu interleaving */																\
		0,																							\
																									\
		/* video hardware */																		\
		64*8, 32*8, { 0*8, 36*8-1, 2*8, 30*8-1 },													\
		gfxdecodeinfo,																		\
		256, 256,																					\
		taitos84_vh_convert_color_prom,																\
																									\
		VIDEO_TYPE_RASTER,																			\
		0,																							\
		generic_vh_start,																			\
		generic_vh_stop,																			\
		taitos84_vh_screenrefresh,																	\
																									\
		/* sound hardware */																		\
		0,0,0,0,																					\
		{																							\
			{																						\
				SOUND_AY8910,																		\
				&ay8910_interface																	\
			}																						\
		}																							\
	};

/* actual definitions */
Machine_Driver( gyrodine )
Machine_Driver( sonofphx )
Machine_Driver( srdmissn )

/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( gyrodine_rom )
	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "a21.02", 0x0000, 0x2000, 0xc5ec4a50 )
	ROM_LOAD( "a21.03", 0x2000, 0x2000, 0x4e9323bd )
	ROM_LOAD( "a21.04", 0x4000, 0x2000, 0x57e659d4 )
	ROM_LOAD( "a21.05", 0x6000, 0x2000, 0x1e7293f3 )

	ROM_REGION( 0x1f000 ) /* temporary space for graphics */
	ROM_LOAD( "a21.15", 0x00000, 0x1000, 0xadba18d0 ) /* chars */
	ROM_LOAD( "a21.14", 0x01000, 0x2000, 0x9c5c4d5b ) /* sprites - plane 0 */
	/* 0x03000-0x04fff empty */
	ROM_LOAD( "a21.13", 0x05000, 0x2000, 0xd36b5aad ) /* sprites - plane 0 */
	/* 0x07000-0x08fff empty */
	ROM_LOAD( "a21.12", 0x09000, 0x2000, 0xf387aea2 ) /* sprites - plane 1 */
	/* 0x0b000-0x0cfff empty */
	ROM_LOAD( "a21.11", 0x0d000, 0x2000, 0x87967d7d ) /* sprites - plane 1 */
	/* 0x0f000-0x10fff empty */
	ROM_LOAD( "a21.10", 0x11000, 0x2000, 0x59640ab4 ) /* sprites - plane 2 */
	/* 0x13000-0x14fff empty */
	ROM_LOAD( "a21.09", 0x15000, 0x2000, 0x22ad88d8 ) /* sprites - plane 2 */
	/* 0x17000-0x18fff empty */
	ROM_LOAD( "a21.08", 0x19000, 0x2000, 0xa57df1c9 ) /* tiles - plane 0 */
	ROM_LOAD( "a21.07", 0x1b000, 0x2000, 0x63623ba3 ) /* tiles - plane 1 */
	ROM_LOAD( "a21.06", 0x1d000, 0x2000, 0x4cc969a9 ) /* tiles - plane 2 */

	ROM_REGION( 0x340 ) /* PROMs */
	ROM_LOAD( "a21.16", 0x0000, 0x0100, 0xcc25fb56 ) /* red */
	ROM_LOAD( "a21.17", 0x0100, 0x0100, 0xca054448 ) /* green */
	ROM_LOAD( "a21.18", 0x0200, 0x0100, 0x23c0c449 ) /* blue */
	ROM_LOAD( "a21.20", 0x0300, 0x0020, 0xefc4985e ) /* char lookup table */
	ROM_LOAD( "a21.19", 0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */

	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "a21.01", 0x0000, 0x2000, 0xb2ce0aa2 )
ROM_END

ROM_START( srdmissn_rom )
	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "5.t2",   0x0000, 0x4000, 0xa682b48c )
	ROM_LOAD( "7.t3",   0x4000, 0x4000, 0x1719c58c )

	ROM_REGION( 0x1f000 ) /* temporary space for graphics */
	ROM_LOAD( "15.4a",  0x00000, 0x1000, 0x4961f7fd ) /* chars */
	ROM_LOAD( "14.6a",  0x01000, 0x4000, 0x3d4c0447 ) /* sprites - plane 0 */
	ROM_LOAD( "13.7a",  0x05000, 0x4000, 0x22414a67 ) /* sprites - plane 0 */
	ROM_LOAD( "12.8a",  0x09000, 0x4000, 0x61e34283 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",  0x0d000, 0x4000, 0xbbbaffef ) /* sprites - plane 1 */
	ROM_LOAD( "10.10a", 0x11000, 0x4000, 0xde564f97 ) /* sprites - plane 2 */
	ROM_LOAD( "9.11a",  0x15000, 0x4000, 0x890dc815 ) /* sprites - plane 2 */
	ROM_LOAD( "17.9h",  0x19000, 0x2000, 0x41211458 ) /* tiles - plane 1 */
	ROM_LOAD( "18.10h", 0x1b000, 0x2000, 0x740eccd4 ) /* tiles - plane 0 */
	ROM_LOAD( "16.11h", 0x1d000, 0x2000, 0xc1f4a5db ) /* tiles - plane 2 */

	ROM_REGION( 0x340 ) /* PROMs */
	ROM_LOAD( "mr.1j",  0x0000, 0x0100, 0x110a436e ) /* red */
	ROM_LOAD( "mg.1h",  0x0100, 0x0100, 0x0fbfd9f0 ) /* green */
	ROM_LOAD( "mb.1f",  0x0200, 0x0100, 0xa342890c ) /* blue */
	ROM_LOAD( "m2.5j",  0x0300, 0x0020, 0x190a55ad ) /* char lookup table */
	ROM_LOAD( "m1.2c",  0x0320, 0x0020, 0x83a39201 ) /* timing? not used */

	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "1.t7",   0x0000, 0x4000, 0xdc48595e )
	ROM_LOAD( "3.t8",   0x4000, 0x4000, 0x216be1e8 )
ROM_END

ROM_START( sonofphx_rom )
	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "5.f4",   0x0000, 0x2000, 0xe0d2c6cf )
	ROM_LOAD( "6.h4",   0x2000, 0x2000, 0x3a0d0336 )
	ROM_LOAD( "7.j4",   0x4000, 0x2000, 0x57a8e900 )

	ROM_REGION( 0x1f000 ) /* temporary space for graphics */
	ROM_LOAD( "14.4a",  0x00000, 0x1000, 0xb3859b8b ) /* chars */
	ROM_LOAD( "8.6a",   0x01000, 0x4000, 0x0e9f757e ) /* sprites - plane 0 */
	ROM_LOAD( "9.7a",   0x05000, 0x4000, 0xf7d2e650 ) /* sprites - plane 0 */
	ROM_LOAD( "10.8a",  0x09000, 0x4000, 0xe717baf4 ) /* sprites - plane 1 */
	ROM_LOAD( "11.9a",  0x0d000, 0x4000, 0x04b2250b ) /* sprites - plane 1 */
	ROM_LOAD( "12.10a", 0x11000, 0x4000, 0xd110e140 ) /* sprites - plane 2 */
	ROM_LOAD( "13.11a", 0x15000, 0x4000, 0x8fdc713c ) /* sprites - plane 2 */
	ROM_LOAD( "15.9h",  0x19000, 0x2000, 0xc9213469 ) /* tiles - plane 0 */
	ROM_LOAD( "16.10h", 0x1b000, 0x2000, 0x7de5d39e ) /* tiles - plane 1 */
	ROM_LOAD( "17.11h", 0x1d000, 0x2000, 0x0ba5f72c ) /* tiles - plane 2 */

	ROM_REGION( 0x340 ) /* PROMs */
	ROM_LOAD( "r.1f",   0x0200, 0x0100, 0xb7f48b41 ) /* red */
	ROM_LOAD( "g.1h",   0x0100, 0x0100, 0xacd7a69e ) /* green */
	ROM_LOAD( "b.1j",   0x0000, 0x0100, 0x3ea35431 ) /* blue */
	/* 0x0300-0x031f empty - looks like there isn't a lookup table PROM */
	ROM_LOAD( "2c",     0x0320, 0x0020, 0x83a39201 ) /* timing? (not used) */

	ROM_REGION( 0x10000 ) /* 64k for code */
	ROM_LOAD( "1.f2",   0x0000, 0x2000, 0xc485c621 )
	ROM_LOAD( "2.h2",   0x2000, 0x2000, 0xb3c6a886 )
	ROM_LOAD( "3.j2",   0x4000, 0x2000, 0x197e314c )
	ROM_LOAD( "4.k2",   0x6000, 0x2000, 0x4f3695a1 )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver gyrodine_driver = {
	__FILE__,
	0,
	"gyrodine",
	"Gyrodine",
	"1984",
	"Taito Corporation",
	"Ernesto Corvi",
	0,
	&gyrodine_machine_driver,
	0,

	gyrodine_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	gyrodine_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver sonofphx_driver = {
	__FILE__,
	0,
	"sonofphx",
	"Son of Phoenix",
	"1985",
	"Associated Overseas MFR, Inc",
	"Ernesto Corvi",
	0,
	&sonofphx_machine_driver,
	0,

	sonofphx_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};

struct GameDriver srdmissn_driver = {
	__FILE__,
	0,
	"srdmissn",
	"S.R.D. Mission",
	"1986",
	"Taito Corporation",
	"Ernesto Corvi",
	0,
	&srdmissn_machine_driver,
	0,

	srdmissn_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};
