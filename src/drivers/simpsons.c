/***************************************************************************

The Simpsons (c) 1991 Konami Co. Ltd

Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "cpu/z80/z80.h"
#include "vidhrdw/konamiic.h"

/* from vidhrdw */
int simpsons_vh_start( void );
void simpsons_vh_stop( void );
void simpsons_priority_w(int offset,int data);
void simpsons_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* from machine */
int simpsons_eeprom_r( int offset );
void simpsons_eeprom_w( int offset, int data );
void simpsons_coin_counter_w( int offset, int data );
int simpsons_sound_interrupt_r( int offset );
int simpsons_sound_r(int offset);
int simpsons_speedup1_r( int offs );
int simpsons_speedup2_r( int offs );
void simpsons_init_machine( void );
int simpsons_eeprom_load(void);
void simpsons_eeprom_save(void);
extern int simpsons_firq_enabled;

/***************************************************************************

  Memory Maps

***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0fff, MRA_BANK3 },
	{ 0x1f80, 0x1f80, input_port_4_r },
	{ 0x1f81, 0x1f81, simpsons_eeprom_r },
	{ 0x1f90, 0x1f90, input_port_0_r },
	{ 0x1f91, 0x1f91, input_port_1_r },
	{ 0x1f92, 0x1f92, input_port_2_r },
	{ 0x1f93, 0x1f93, input_port_3_r },
	{ 0x1fc4, 0x1fc4, simpsons_sound_interrupt_r },
	{ 0x1fc6, 0x1fc7, simpsons_sound_r },	/* K053260 */
	{ 0x1fca, 0x1fca, watchdog_reset_r },
	{ 0x2000, 0x3fff, MRA_BANK4 },
	{ 0x0000, 0x3fff, K052109_r },
	{ 0x4856, 0x4856, simpsons_speedup2_r },
	{ 0x4942, 0x4942, simpsons_speedup1_r },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_BANK3 },
	{ 0x1fb0, 0x1fbf, K053251_w },
	{ 0x1fc0, 0x1fc0, simpsons_coin_counter_w },
	{ 0x1fc2, 0x1fc2, simpsons_eeprom_w },
	{ 0x1fc6, 0x1fc7, K053260_WriteReg },
	{ 0x2000, 0x3fff, MWA_BANK4 },
	{ 0x0000, 0x3fff, K052109_w },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static void z80_bankswitch_w( int offset, int data )
{
	unsigned char *RAM = Machine->memory_region[3];

	offset = 0x10000 + ( ( ( data & 7 ) - 2 ) * 0x4000 );

	cpu_setbank( 2, &RAM[ offset ] );
}

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static void z80_arm_nmi(int offset,int data)
{
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}

static struct MemoryReadAddress z80_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_ReadReg },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress z80_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, z80_arm_nmi },
	{ 0xfc00, 0xfc2f, K053260_WriteReg },
	{ 0xfe00, 0xfe00, z80_bankswitch_w },
	{ -1 }	/* end of table */
};

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START /* IN0 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 - Player 3 - Used on the 4p version */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN3 - Player 4 - Used on the 4p version */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( simpsn2p_input_ports )
	PORT_START /* IN0 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 - Player 3 - Used on the 4p version */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN3 - Player 4 - Used on the 4p version */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************

	Graphics Decoding

***************************************************************************/

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	32768,	/* 32768 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
	  2*4, 3*4, 0*4, 1*4,
	  0x200000*8+2*4, 0x200000*8+3*4, 0x200000*8+0*4, 0x200000*8+1*4,
	  0x100000*8+2*4, 0x100000*8+3*4, 0x100000*8+0*4, 0x100000*8+1*4,
	  0x300000*8+2*4, 0x300000*8+3*4, 0x300000*8+0*4, 0x300000*8+1*4,
	},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 2, 0x000000, &spritelayout,	0, 128 },
	{ -1 } /* end of array */
};


/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K053260_interface k053260_interface =
{
	3579545,
	4, /* memory region */
	{ MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) },
	0
};

static int simpsons_irq( void )
{
	if ( cpu_getiloops() == 0 )
	{
		if ( simpsons_firq_enabled )
			return KONAMI_INT_FIRQ;
	}
	else
	{
		if (K052109_is_IRQ_enabled())
			return KONAMI_INT_IRQ;
	}

	return ignore_interrupt();
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_KONAMI,
			6000000, /* ? */
			0,
			readmem,writemem,0,0,
			simpsons_irq,2,	/* IRQ triggered by the 052109, FIRQ by the sprite hardware */
        },
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,
			3,
			z80_readmem,z80_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
								/* NMIs are generated by the 053260 */
        }
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* inter-cpu interleaving factor */
	simpsons_init_machine,

	/* video hardware */
	64*8, 32*8, { 14*8, (64-14)*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	16*128, 16*128,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	simpsons_vh_start,
	simpsons_vh_stop,
	simpsons_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface
		}
	}
};


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( simpsons_rom )
	ROM_REGION( 0x90000 ) /* code + banked roms + banked ram */
	ROM_LOAD( "g02.16c",      0x10000, 0x20000, 0x580ce1d6 )
	ROM_LOAD( "g01.17c",      0x30000, 0x20000, 0x9f843def )
	ROM_LOAD( "j13.13c",      0x50000, 0x20000, 0xaade2abd )
    ROM_LOAD( "j12.15c",      0x70000, 0x18000, 0x479e12f2 )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x100000 ) /* graphics ( dont dispose as the program can read them ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, 0xba1ec910 ) /* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, 0xcf2bbcab ) /* tiles */

	ROM_REGION( 0x400000 ) /* graphics ( dont dispose as the program can read them ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, 0x7de500ad ) /* sprites? */
	ROM_LOAD( "simp_12n.rom", 0x100000, 0x100000, 0x577dbd53 ) /* sprites? */
	ROM_LOAD( "simp_8n.rom",  0x200000, 0x100000, 0xaa085093 ) /* sprites? */
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, 0x55fab05d ) /* sprites? */

	ROM_REGION( 0x28000 ) /* Z80 code + banks */
	ROM_LOAD( "e03.6g",       0x00000, 0x08000, 0x866b7a35 )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x140000 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, 0x1397a73b )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, 0x78778013 )
ROM_END

ROM_START( simpsn2p_rom )
	ROM_REGION( 0x90000 ) /* code + banked roms + banked ram */
	ROM_LOAD( "g02.16c",      0x10000, 0x20000, 0x580ce1d6 )
	ROM_LOAD( "simp_p01.rom", 0x30000, 0x20000, 0x07ceeaea )
	ROM_LOAD( "simp_013.rom", 0x50000, 0x20000, 0x8781105a )
    ROM_LOAD( "simp_012.rom", 0x70000, 0x18000, 0x244f9289 )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x100000 ) /* graphics ( dont dispose as the program can read them ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, 0xba1ec910 ) /* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, 0xcf2bbcab ) /* tiles */

	ROM_REGION( 0x400000 ) /* graphics ( dont dispose as the program can read them ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, 0x7de500ad ) /* sprites? */
	ROM_LOAD( "simp_12n.rom", 0x100000, 0x100000, 0x577dbd53 ) /* sprites? */
	ROM_LOAD( "simp_8n.rom",  0x200000, 0x100000, 0xaa085093 ) /* sprites? */
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, 0x55fab05d ) /* sprites? */

	ROM_REGION( 0x28000 ) /* Z80 code + banks */
	ROM_LOAD( "simp_g03.rom", 0x00000, 0x08000, 0x76c1850c )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x140000 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, 0x1397a73b )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, 0x78778013 )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static void gfx_untangle(void)
{
	konami_rom_deinterleave(1);
}

struct GameDriver simpsons_driver =
{
	__FILE__,
	0,
	"simpsons",
	"The Simpsons (4 Players)",
	"1991",
	"Konami",
	"Ernesto Corvi",
	0,
	&machine_driver,
	0,

	simpsons_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
    ORIENTATION_DEFAULT,
	simpsons_eeprom_load, simpsons_eeprom_save
};

struct GameDriver simpsn2p_driver =
{
	__FILE__,
	&simpsons_driver,
	"simpsn2p",
	"The Simpsons (2 Players)",
	"1991",
	"Konami",
	"Ernesto Corvi",
	0,
	&machine_driver,
	0,

	simpsn2p_rom,
	gfx_untangle, 0,
	0,
	0,	/* sound_prom */

	simpsn2p_input_ports,

	0, 0, 0,
    ORIENTATION_DEFAULT,
	simpsons_eeprom_load, simpsons_eeprom_save
};
