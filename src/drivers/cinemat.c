/***************************************************************************

Cinematronics vector game handlers

Special thanks to Neil Bradley, Zonn Moore, and Jeff Mitchell of the
Retrocade Alliance

Current drivers:

extern struct GameDriver spacewar_driver;
extern struct GameDriver barrier_driver;
extern struct GameDriver starcas_driver;
extern struct GameDriver tgunner_driver;
extern struct GameDriver ripoff_driver;
extern struct GameDriver armora_driver;
extern struct GameDriver wotw_driver;
extern struct GameDriver warrior_driver;
extern struct GameDriver starhawk_driver;
extern struct GameDriver solarq_driver;

Don't work due to bug in the input handling

extern struct GameDriver tgunner_driver;
extern struct GameDriver boxingb_driver;
extern struct GameDriver speedfrk_driver;
extern struct GameDriver sundance_driver;

Doesn't work due to possible CPU bug

extern struct GameDriver demon_driver;

to do:

* Fix Tailgunner/Boxing Bugs controls
* Fix Star Hawk controls
* Fix Speed Freak controls
* Find out why Demon is broken
* Fix problem with colors in Solar Quest - overlay instead?
* Possible overlay for Tailgunner - blue

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "cpu/ccpu/ccpu.h"


void cinemat_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int cinemat_vh_start (void);
void cinemat_vh_stop (void);
void cinemat_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

static int cinemat_readport (int offset);
static void cinemat_writeport (int offset, int data);

static int speedfrk_readports (int offset);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0, CCPU_PORT_MAX, cinemat_readport },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0, CCPU_PORT_MAX, cinemat_writeport },
	{ -1 }  /* end of table */
};

static struct IOReadPort speedfrk_readport[] =
{
	{ 0, CCPU_PORT_MAX, speedfrk_readports },
	{ -1 }  /* end of table */
};




static int cinemat_outputs;

static int cinemat_readport (int offset)
{
	switch (offset)
	{
		case CCPU_PORT_IOSWITCHES:
			return readinputport (0);

		case CCPU_PORT_IOINPUTS:
			return (readinputport (1) << 8) + readinputport (2);

		case CCPU_PORT_IOOUTPUTS:
			return cinemat_outputs;

		case CCPU_PORT_IN_JOYSTICKX:
			return readinputport (3) << 2;

		case CCPU_PORT_IN_JOYSTICKY:
			return readinputport (4) << 2;
	}

	return 0;
}


static void cinemat_writeport (int offset, int data)
{
	switch (offset)
	{
		case CCPU_PORT_IOOUTPUTS:
			cinemat_outputs = data;
			if (errorlog) fprintf (errorlog, "ccpu output: %04x:%04x\n", offset, data);
			break;
	}
}


static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0,      &fakelayout,     0, 256 },
	{ -1 } /* end of array */
};



static struct Samplesinterface cinemat_samples_interface =
{
	7	/* 7 channels */
};


#define CINEMA_MACHINE(driver, minx, miny, maxx, maxy) \
static struct MachineDriver driver##_machine_driver = \
{ \
	/* basic machine hardware */ \
	{ \
		{ \
			CPU_CCPU, \
			1000000, \
			0, \
			readmem,writemem,readport,writeport, \
			ignore_interrupt,1 \
		} \
	}, \
	38, 0,	/* frames per second, vblank duration (vector game, so no vblank) */ \
	1, \
	driver##_init_machine, \
\
	/* video hardware */ \
	400, 300, { minx, maxx, miny, maxy }, \
	gfxdecodeinfo, \
	256, 256, \
 	cinemat_init_colors, \
\
	VIDEO_TYPE_VECTOR, \
	0, \
	cinemat_vh_start, \
	cinemat_vh_stop, \
	cinemat_vh_screenrefresh, \
\
	/* sound hardware */ \
	0,0,0,0, \
	{ \
		{ \
			SOUND_SAMPLES, \
			&cinemat_samples_interface \
		} \
	} \
};

#define CINEMA_MACHINEX(driver, minx, miny, maxx, maxy) \
static struct MachineDriver driver##_machine_driver = \
{ \
	/* basic machine hardware */ \
	{ \
		{ \
			CPU_CCPU, \
			1000000, \
			0, \
			readmem,writemem,driver##_readport,writeport, \
			ignore_interrupt,1 \
		} \
	}, \
	38, 0,	/* frames per second, vblank duration (vector game, so no vblank) */ \
	1, \
	driver##_init_machine, \
\
	/* video hardware */ \
	400, 300, { minx, maxx, miny, maxy }, \
	gfxdecodeinfo, \
	256, 256, \
 	cinemat_init_colors, \
\
	VIDEO_TYPE_VECTOR, \
	0, \
	cinemat_vh_start, \
	cinemat_vh_stop, \
	cinemat_vh_screenrefresh, \
\
	/* sound hardware */ \
	0,0,0,0, \
	{ \
		{ \
			SOUND_SAMPLES, \
			&cinemat_samples_interface \
		} \
	} \
};


void cinemat_interleave (int romSize, int numRoms)
{
	unsigned char *temp = malloc (2 * romSize);
	unsigned char *src1, *src2, *dest;
	int i, j;

	if (temp)
	{
		for (i = 0; i < numRoms; i += 2)
		{
			src1 = &Machine->memory_region[0][i * romSize + 0];
			src2 = &Machine->memory_region[0][i * romSize + romSize];
			dest = temp;

			for (j = 0; j < romSize; j++)
				*dest++ = *src1++, *dest++ = *src2++;

			memcpy (&Machine->memory_region[0][i * romSize + 0], temp, 2 * romSize);
		}

		free (temp);
	}
}


void cinemat4k_rom_decode (void)
{
	cinemat_interleave (0x800, 2);
}

void cinemat8k_rom_decode (void)
{
	cinemat_interleave (0x800, 4);
}

void cinemat16k_rom_decode (void)
{
	cinemat_interleave (0x1000, 4);
}

void cinemat32k_rom_decode (void)
{
	cinemat_interleave (0x1000, 8);
}



static unsigned char color_prom_bilevel[] = { CCPU_MONITOR_BILEV };
static unsigned char color_prom_bilevel_overlay[] = { CCPU_MONITOR_BILEV | 0x80 };
static unsigned char color_prom_bilevel_backdrop[] = { CCPU_MONITOR_BILEV | 0x40 };

static unsigned char color_prom_16color[] = { CCPU_MONITOR_16COL };

static unsigned char color_prom_64color[] = { CCPU_MONITOR_64COL };

static unsigned char color_prom_wotw[] = { CCPU_MONITOR_WOWCOL };


/* switch definitions are all mangled; for ease of use, I created these handy macros */

#define SW7 0x40
#define SW6 0x02
#define SW5 0x04
#define SW4 0x08
#define SW3 0x01
#define SW2 0x20
#define SW1 0x10

#define SW7OFF SW7
#define SW6OFF SW6
#define SW5OFF SW5
#define SW4OFF SW4
#define SW3OFF SW3
#define SW2OFF SW2
#define SW1OFF SW1

#define SW7ON  0
#define SW6ON  0
#define SW5ON  0
#define SW4ON  0
#define SW3ON  0
#define SW2ON  0
#define SW1ON  0


/***************************************************************************

  Spacewar

***************************************************************************/

INPUT_PORTS_START ( spacewar_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW2|SW1, SW2ON|SW1ON, "Game Time", IP_KEY_NONE )
	PORT_DIPSETTING( SW2OFF|SW1OFF, "1:30/coin" )
	PORT_DIPSETTING( SW2ON |SW1ON,  "2:00/coin" )
	PORT_DIPSETTING( SW2ON |SW1OFF, "3:00/coin" )
	PORT_DIPSETTING( SW2OFF|SW1ON,  "4:00/coin" )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  0, "Option 0", OSD_KEY_0_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  0, "Option 5", OSD_KEY_5_PAD, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1 )

	PORT_START /* inputs low */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  0, "Option 7", OSD_KEY_7_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  0, "Option 2", OSD_KEY_2_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x20, IP_ACTIVE_LOW,  0, "Option 6", OSD_KEY_6_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x10, IP_ACTIVE_LOW,  0, "Option 1", OSD_KEY_1_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  0, "Option 9", OSD_KEY_9_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  0, "Option 4", OSD_KEY_4_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW,  0, "Option 8", OSD_KEY_8_PAD, IP_JOY_NONE, 0 )
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  0, "Option 3", OSD_KEY_3_PAD, IP_JOY_NONE, 0 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( spacewar_rom )
	ROM_REGION(0x1000)	/* 4k for code */
	ROM_LOAD( "spacewar.1l", 0x0000, 0x0800, 0xedf0fd53 )
	ROM_LOAD( "spacewar.2r", 0x0800, 0x0800, 0x4f21328b )
ROM_END


static const char *spacewar_sample_names[] =
{
    0	/* end of array */
};


void spacewar_init_machine (void)
{
	ccpu_Config (0, CCPU_MEMSIZE_4K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (spacewar, 0, 0, 1024, 768)


struct GameDriver spacewar_driver =
{
	__FILE__,
	0,
	"spacewar",
	"Space Wars",
	"1978",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&spacewar_machine_driver,
	0,

	spacewar_rom,
	cinemat4k_rom_decode, 0,
	spacewar_sample_names,
	0,	/* sound_prom */

	spacewar_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};


/***************************************************************************

  Barrier

***************************************************************************/

INPUT_PORTS_START ( barrier_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW1, SW1ON, "Innings/Game", IP_KEY_NONE )
	PORT_DIPSETTING(    SW1ON,  "3" )
	PORT_DIPSETTING(    SW1OFF, "5" )
	PORT_DIPNAME( SW2, SW2OFF, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    SW2ON,  "Off" )
	PORT_DIPSETTING(    SW2OFF, "On" )
	PORT_BIT ( SW7|SW6|SW5|SW4|SW3, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BITX( 0x40, IP_ACTIVE_LOW,  0, "Skill C", OSD_KEY_C, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW,  0, "Skill B", OSD_KEY_B, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BITX( 0x01, IP_ACTIVE_LOW,  0, "Skill A", OSD_KEY_A, IP_JOY_NONE, 0 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( barrier_rom )
	ROM_REGION(0x1000)	/* 4k for code */
	ROM_LOAD( "barrier.t7", 0x0000, 0x0800, 0x7c3d68c8 )
	ROM_LOAD( "barrier.p7", 0x0800, 0x0800, 0xaec142b5 )
ROM_END


static const char *barrier_sample_names[] =
{
    0	/* end of array */
};


void barrier_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_4K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (barrier, 0, 0, 1024, 768)


struct GameDriver barrier_driver =
{
	__FILE__,
	0,
	"barrier",
	"Barrier",
	"1979",
	"Vectorbeam",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&barrier_machine_driver,
	0,

	barrier_rom,
	cinemat4k_rom_decode, 0,
	barrier_sample_names,
	0,	/* sound_prom */

	barrier_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_ROTATE_270 ^ ORIENTATION_FLIP_X,

	0,0
};



/***************************************************************************

  Star Hawk

***************************************************************************/

/* TODO: 4way or 8way stick? */
INPUT_PORTS_START ( starhawk_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW7, SW7OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW7OFF, "Off" )
	PORT_DIPSETTING( SW7ON,  "On" )
	PORT_BIT ( SW6, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( SW5, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( SW4, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( SW3, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    SW2OFF|SW1OFF, "2:00/4:00" )
	PORT_DIPSETTING(    SW2ON |SW1OFF, "1:30/3:00" )
	PORT_DIPSETTING(    SW2OFF|SW1ON,  "1:00/2:00" )
	PORT_DIPSETTING(    SW2ON |SW1ON,  "0:45/1:30" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( starhawk_rom )
	ROM_REGION(0x1000)	/* 4k for code */
	ROM_LOAD( "u7", 0x0000, 0x0800, 0x376e6c5c )
	ROM_LOAD( "r7", 0x0800, 0x0800, 0xbb71144f )
ROM_END


static const char *starhawk_sample_names[] =
{
    0	/* end of array */
};


void starhawk_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_4K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (starhawk, 0, 0, 1024, 768)


struct GameDriver starhawk_driver =
{
	__FILE__,
	0,
	"starhawk",
	"Star Hawk",
	"1981",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&starhawk_machine_driver,
	0,

	starhawk_rom,
	cinemat4k_rom_decode, 0,
	starhawk_sample_names,
	0,	/* sound_prom */

	starhawk_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Star Castles

***************************************************************************/

INPUT_PORTS_START ( starcas_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7ON, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Test Pattern", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7ON,               "Off" )
	PORT_DIPSETTING( SW7OFF,              "On" )
	PORT_DIPNAME( SW4|SW3, SW4OFF|SW3OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON |SW3OFF,       "2 coins/1 credit" )
	PORT_DIPSETTING( SW4ON |SW3ON,        "4 coins/3 credits" )
	PORT_DIPSETTING( SW4OFF|SW3OFF,       "1 coin/1 credit" )
	PORT_DIPSETTING( SW4OFF|SW3ON,        "2 coins/3 credits" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING( SW2OFF|SW1OFF,       "3" )
	PORT_DIPSETTING( SW2ON |SW1OFF,       "4" )
	PORT_DIPSETTING( SW2OFF|SW1ON,        "5" )
	PORT_DIPSETTING( SW2ON |SW1ON,        "6" )
	PORT_BIT ( SW6|SW5, SW6OFF|SW5OFF, IPT_UNUSED )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


ROM_START( starcas_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "starcast.t7", 0x0000, 0x0800, 0x65d0a225 )
	ROM_LOAD( "starcast.p7", 0x0800, 0x0800, 0xd8f58d9a )
	ROM_LOAD( "starcast.u7", 0x1000, 0x0800, 0xd4f35b82 )
	ROM_LOAD( "starcast.r7", 0x1800, 0x0800, 0x9fd3de54 )
ROM_END


static const char *starcas_sample_names[] =
{
    0	/* end of array */
};


void starcas_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_8K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (starcas, 0, 0, 1024, 768)


struct GameDriver starcas_driver =
{
	__FILE__,
	0,
	"starcas",
	"Star Castle",
	"1980",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&starcas_machine_driver,
	0,

	starcas_rom,
	cinemat8k_rom_decode, 0,
	starcas_sample_names,
	0,	/* sound_prom */

	starcas_input_ports,

	color_prom_bilevel_overlay, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};


/***************************************************************************

  Tailgunner

***************************************************************************/

INPUT_PORTS_START ( tgunner_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW6|SW2|SW1, SW6OFF|SW2OFF|SW1OFF, "Shield Points", IP_KEY_NONE )
	PORT_DIPSETTING( SW6ON |SW2ON |SW1ON,  "15" )
	PORT_DIPSETTING( SW6ON |SW2OFF|SW1ON,  "20" )
	PORT_DIPSETTING( SW6ON |SW2ON |SW1OFF, "30" )
	PORT_DIPSETTING( SW6ON |SW2OFF|SW1OFF, "40" )
	PORT_DIPSETTING( SW6OFF|SW2ON |SW1ON,  "50" )
	PORT_DIPSETTING( SW6OFF|SW2OFF|SW1ON,  "60" )
	PORT_DIPSETTING( SW6OFF|SW2ON |SW1OFF, "70" )
	PORT_DIPSETTING( SW6OFF|SW2OFF|SW1OFF, "80" )
	PORT_DIPNAME( SW3, SW3OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW3ON,                "2 coins/1 credit" )
	PORT_DIPSETTING( SW3OFF,               "1 coin/1 credit" )
	PORT_BIT ( SW7|SW5|SW4, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick X */
	PORT_ANALOG ( 0x6ff, 0x300, IPT_AD_STICK_X, 100, 0, 0, 0x620 )

	PORT_START /* joystick Y */
	PORT_ANALOG ( 0x6ff, 0x300, IPT_AD_STICK_Y, 100, 0, 0, 0x620 )
INPUT_PORTS_END


ROM_START( tgunner_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "tgunner.t70", 0x0000, 0x0800, 0x21ec9a04 )
	ROM_LOAD( "tgunner.p70", 0x0800, 0x0800, 0x8d7410b3 )
	ROM_LOAD( "tgunner.t71", 0x1000, 0x0800, 0x2c954ab6 )
	ROM_LOAD( "tgunner.p71", 0x1800, 0x0800, 0x8e2c8494 )
ROM_END


static const char *tgunner_sample_names[] =
{
    0	/* end of array */
};


void tgunner_init_machine (void)
{
	ccpu_Config (0, CCPU_MEMSIZE_8K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (tgunner, 0, 0, 1024, 768)


struct GameDriver tgunner_driver =
{
	__FILE__,
	0,
	"tailg",
	"Tailgunner",
	"1979",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&tgunner_machine_driver,
	0,

	tgunner_rom,
	cinemat8k_rom_decode, 0,
	tgunner_sample_names,
	0,	/* sound_prom */

	tgunner_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};


/***************************************************************************

  Ripoff

***************************************************************************/

INPUT_PORTS_START ( ripoff_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7OFF, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7OFF,           "Off" )
	PORT_DIPSETTING( SW7ON,            "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Scores", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF,           "Individual" )
	PORT_DIPSETTING( SW6ON,            "Combined" )
	PORT_DIPNAME( SW5, SW5ON, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING( SW5ON,            "Off" )
	PORT_DIPSETTING( SW5OFF,           "On" )
	PORT_DIPNAME( SW4|SW3, SW4ON|SW3ON, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    SW4ON |SW3OFF, "2 coins/1 credit" )
	PORT_DIPSETTING(    SW4OFF|SW3OFF, "4 coins/3 credits" )
	PORT_DIPSETTING(    SW4ON |SW3ON,  "1 coin/1 credit" )
	PORT_DIPSETTING(    SW4OFF|SW3ON,  "2 coins/3 credits" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1ON, "Fuel Pods", IP_KEY_NONE )
	PORT_DIPSETTING(    SW2ON |SW1OFF, "4" )
	PORT_DIPSETTING(    SW2OFF|SW1OFF, "8" )
	PORT_DIPSETTING(    SW2ON |SW1ON,  "12" )
	PORT_DIPSETTING(    SW2OFF|SW1ON,  "16" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( ripoff_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "ripoff.t7", 0x0000, 0x0800, 0x40c2c5b8 )
	ROM_LOAD( "ripoff.p7", 0x0800, 0x0800, 0xa9208afb )
	ROM_LOAD( "ripoff.u7", 0x1000, 0x0800, 0x29c13701 )
	ROM_LOAD( "ripoff.r7", 0x1800, 0x0800, 0x150bd4c8 )
ROM_END


static const char *ripoff_sample_names[] =
{
    0	/* end of array */
};


void ripoff_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_8K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (ripoff, 0, 0, 1024, 768)


struct GameDriver ripoff_driver =
{
	__FILE__,
	0,
	"ripoff",
	"Rip Off",
	"1979",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&ripoff_machine_driver,
	0,

	ripoff_rom,
	cinemat8k_rom_decode, 0,
	ripoff_sample_names,
	0,	/* sound_prom */

	ripoff_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Speed Freak

***************************************************************************/

int speedfrk_gear = 1;

int speedfrk_in2_r(int offset)
{
	int gear;
	int val;

	/* check the fake gear input port and determine the bit settings for the gear */
	gear = input_port_5_r(0);
	if (gear & 0x10)				speedfrk_gear=1;
	else if (gear & 0x20)			speedfrk_gear=2;
	else if (gear & 0x40)			speedfrk_gear=3;
	else if (gear & 0x80)			speedfrk_gear=4;

	if (speedfrk_gear==1)		val = 0xE0;
	else if (speedfrk_gear==2)	val = 0xD0;
	else if (speedfrk_gear==3)	val = 0xB0;
	else						val = 0x70;

	/* add the start key into the mix */
	if (input_port_2_r(0) & 0x80) val |= 0x80;
	else val &= ~0x80;

	/* and for the cherry on top, we add the scrambled analog steering */
	{
		int steer = input_port_2_r(0) & 0x0f;

		steer = ((steer & 0x08) >> 3) | ((steer & 0x07) << 1);
		val |= steer;
	}

	return val;
}

static int speedfrk_readports (int offset)
{
	switch (offset)
	{
		case CCPU_PORT_IOSWITCHES:
			return readinputport (0);

		case CCPU_PORT_IOINPUTS:
			return (readinputport (1) << 8) + speedfrk_in2_r (0);

		case CCPU_PORT_IOOUTPUTS:
			return cinemat_outputs;

		case CCPU_PORT_IN_JOYSTICKX:
			return readinputport (3) << 2;

		case CCPU_PORT_IN_JOYSTICKY:
			return readinputport (4) << 2;
	}

	return 0;
}

INPUT_PORTS_START ( speedfrk_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW7, SW7OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW7OFF, "Off" )
	PORT_DIPSETTING( SW7ON,  "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "Off" )
	PORT_DIPSETTING( SW6ON,  "On" )
	PORT_DIPNAME( SW5, SW5OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Off" )
	PORT_DIPSETTING( SW5ON,  "On" )
	PORT_DIPNAME( SW4, SW4OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW4OFF, "Off" )
	PORT_DIPSETTING( SW4ON,  "On" )
	PORT_DIPNAME( SW3, SW3OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW3OFF, "Off" )
	PORT_DIPSETTING( SW3ON,  "On" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1ON, "Extra Time", IP_KEY_NONE )
	PORT_DIPSETTING(    SW2ON |SW1ON,  "69" )
	PORT_DIPSETTING(    SW2ON |SW1OFF, "99" )
	PORT_DIPSETTING(    SW2OFF|SW1ON,  "129" )
	PORT_DIPSETTING(    SW2OFF|SW1OFF, "159" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 ) /* gas */

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x70, IP_ACTIVE_LOW,  IPT_UNUSED ) /* actually the gear shift, see fake below */
	PORT_ANALOG ( 0x0f, 0x00, IPT_DIAL, 25, 0, 0x03, 0x0a )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* in5 - fake for gear shift */
	PORT_BIT ( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2, "1st gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2, "2nd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "3rd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2, "4th gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
INPUT_PORTS_END


ROM_START( speedfrk_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "speedfrk.t7", 0x0000, 0x0800, 0x3552c03f )
	ROM_LOAD( "speedfrk.p7", 0x0800, 0x0800, 0x4b90cdec )
	ROM_LOAD( "speedfrk.u7", 0x1000, 0x0800, 0x616c7cf9 )
	ROM_LOAD( "speedfrk.r7", 0x1800, 0x0800, 0xfbe90d63 )
ROM_END


static const char *speedfrk_sample_names[] =
{
    0	/* end of array */
};


void speedfrk_init_machine (void)
{
	ccpu_Config (0, CCPU_MEMSIZE_8K, CCPU_MONITOR_BILEV);
}

/* we use custom input ports */
CINEMA_MACHINEX (speedfrk, 0, 0, 1024, 768)


struct GameDriver speedfrk_driver =
{
	__FILE__,
	0,
	"speedfrk",
	"Speed Freak",
	"19??",
	"Vectorbeam",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&speedfrk_machine_driver,
	0,

	speedfrk_rom,
	cinemat8k_rom_decode, 0,
	speedfrk_sample_names,
	0,	/* sound_prom */

	speedfrk_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Sundance

***************************************************************************/

INPUT_PORTS_START ( sundance_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BIT ( SW7|SW6|SW5, SW7OFF|SW6OFF|SW5OFF,  IPT_UNUSED )
	PORT_DIPNAME( SW4, SW4ON, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON,  "1 coin/2 players" )
	PORT_DIPSETTING( SW4OFF, "2 coins/2 players" )
	PORT_DIPNAME( SW3, SW3ON, "Language", IP_KEY_NONE )
	PORT_DIPSETTING( SW3OFF, "Japanese" )
	PORT_DIPSETTING( SW3ON,  "English" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1ON, "Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    SW2ON |SW1ON,  "0:45/coin" )
	PORT_DIPSETTING(    SW2OFF|SW1ON,  "1:00/coin" )
	PORT_DIPSETTING(    SW2ON |SW1OFF, "1:30/coin" )
	PORT_DIPSETTING(    SW2OFF|SW1OFF, "2:00/coin" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 1 motion */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 2 motion */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 1 motion */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 2 motion */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED ) /* 2 suns */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 1 motion */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 2 motion */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 1 motion */

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED ) /* 4 suns */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED ) /* Grid */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED ) /* 3 suns */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED ) /* player 2 motion */

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( sundance_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "sundance.t7", 0x0000, 0x0800, 0xd5b9cb19 )
	ROM_LOAD( "sundance.p7", 0x0800, 0x0800, 0x445c4f20 )
	ROM_LOAD( "sundance.u7", 0x1000, 0x0800, 0x67887d48 )
	ROM_LOAD( "sundance.r7", 0x1800, 0x0800, 0x10b77ebd )
ROM_END


static const char *sundance_sample_names[] =
{
    0	/* end of array */
};


void sundance_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_8K, CCPU_MONITOR_16COL);
}


CINEMA_MACHINE (sundance, 0, 0, 1024, 768)


struct GameDriver sundance_driver =
{
	__FILE__,
	0,
	"sundance",
	"Sundance",
	"1979",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&sundance_machine_driver,
	0,

	sundance_rom,
	cinemat8k_rom_decode, 0,
	sundance_sample_names,
	0,	/* sound_prom */

	sundance_input_ports,

	color_prom_16color, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};



/***************************************************************************

  Warrior

***************************************************************************/

INPUT_PORTS_START ( warrior_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW7, SW7OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW7OFF, "Off" )
	PORT_DIPSETTING( SW7ON,  "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "Off" )
	PORT_DIPSETTING( SW6ON,  "On" )
	PORT_DIPNAME( SW5, SW5OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Off" )
	PORT_DIPSETTING( SW5ON,  "On" )
	PORT_DIPNAME( SW4, SW4OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW4OFF, "Off" )
	PORT_DIPSETTING( SW4ON,  "On" )
	PORT_DIPNAME( SW3, SW3OFF, "Test Grid", IP_KEY_NONE )
	PORT_DIPSETTING( SW3ON,  "Off" )
	PORT_DIPSETTING( SW3OFF, "On" )
	PORT_DIPNAME( SW2, SW2OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW2OFF, "Off" )
	PORT_DIPSETTING( SW2ON,  "On" )
	PORT_DIPNAME( SW1, SW1ON, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW1OFF, "Off" )
	PORT_DIPSETTING( SW1ON,  "On" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( warrior_rom )
	ROM_REGION(0x2000)	/* 8k for code */
	ROM_LOAD( "warrior.t7", 0x0000, 0x0800, 0xac3646f9 )
	ROM_LOAD( "warrior.p7", 0x0800, 0x0800, 0x517d3021 )
	ROM_LOAD( "warrior.u7", 0x1000, 0x0800, 0x2e39340f )
	ROM_LOAD( "warrior.r7", 0x1800, 0x0800, 0x8e91b502 )
ROM_END


static const char *warrior_sample_names[] =
{
    0	/* end of array */
};


void warrior_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_8K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (warrior, 0, 0, 1024, 780)


struct GameDriver warrior_driver =
{
	__FILE__,
	0,
	"warrior",
	"Warrior",
	"1978",
	"Vectorbeam",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&warrior_machine_driver,
	0,

	warrior_rom,
	cinemat8k_rom_decode, 0,
	warrior_sample_names,
	0,	/* sound_prom */

	warrior_input_ports,

	color_prom_bilevel_backdrop, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Armor Attack

***************************************************************************/

INPUT_PORTS_START ( armora_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7ON, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7ON,  "Off" )
	PORT_DIPSETTING( SW7OFF, "On" )
	PORT_DIPNAME( SW5, SW5OFF, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Off" )
	PORT_DIPSETTING( SW5ON,  "On" )
	PORT_DIPNAME( SW4|SW3, SW4OFF|SW3OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON |SW3OFF, "2 coins/1 credit" )
	PORT_DIPSETTING( SW4ON |SW3ON,  "4 coins/3 credits" )
	PORT_DIPSETTING( SW4OFF|SW3OFF, "1 coin/1 credit" )
	PORT_DIPSETTING( SW4OFF|SW3ON,  "2 coins/3 credits" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Jeeps", IP_KEY_NONE )
	PORT_DIPSETTING( SW2ON |SW1ON,  "2" )
	PORT_DIPSETTING( SW2OFF|SW1ON,  "3" )
	PORT_DIPSETTING( SW2ON |SW1OFF, "4" )
	PORT_DIPSETTING( SW2OFF|SW1OFF, "5" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( armora_rom )
	ROM_REGION(0x4000)	/* 16k for code */
	ROM_LOAD( "AR414LE.T6", 0x0000, 0x1000, 0xd7e71f84 )
	ROM_LOAD( "AR414LO.P6", 0x1000, 0x1000, 0xdf1c2370 )
	ROM_LOAD( "AR414UE.U6", 0x2000, 0x1000, 0xb0276118 )
	ROM_LOAD( "AR414UO.R6", 0x3000, 0x1000, 0x229d779f )
ROM_END


static const char *armora_sample_names[] =
{
    0	/* end of array */
};


void armora_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_16K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (armora, 0, 0, 1024, 768)


struct GameDriver armora_driver =
{
	__FILE__,
	0,
	"armora",
	"Armor Attack",
	"1980",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&armora_machine_driver,
	0,

	armora_rom,
	cinemat16k_rom_decode, 0,
	armora_sample_names,
	0,	/* sound_prom */

	armora_input_ports,

	color_prom_bilevel_overlay, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Solar Quest

***************************************************************************/

INPUT_PORTS_START ( solarq_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7ON, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7ON,  "Off" )
	PORT_DIPSETTING( SW7OFF, "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Extra Ship", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "25 captures" )
	PORT_DIPSETTING( SW6ON,  "40 captures" )
	PORT_DIPNAME( SW5, SW5OFF, "Mode", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Normal" )
	PORT_DIPSETTING( SW5ON,  "Free Play" )
	PORT_DIPNAME( SW4|SW3, SW4OFF|SW3OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON |SW3OFF, "2 coins/1 credit" )
	PORT_DIPSETTING( SW4ON |SW3ON,  "4 coins/3 credits" )
	PORT_DIPSETTING( SW4OFF|SW3OFF, "1 coin/1 credit" )
	PORT_DIPSETTING( SW4OFF|SW3ON,  "2 coins/3 credits" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Ships", IP_KEY_NONE )
	PORT_DIPSETTING( SW2OFF|SW1OFF, "2" )
	PORT_DIPSETTING( SW2ON |SW1OFF, "3" )
	PORT_DIPSETTING( SW2OFF|SW1ON,  "4" )
	PORT_DIPSETTING( SW2ON |SW1ON,  "5" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START1 ) /* also hyperspace */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_START2 ) /* also nova */

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( solarq_rom )
	ROM_REGION(0x4000)	/* 16k for code */
	ROM_LOAD( "solar.6t", 0x0000, 0x1000, 0x1f3c5333 )
	ROM_LOAD( "solar.6p", 0x1000, 0x1000, 0xd6c16bcc )
	ROM_LOAD( "solar.6u", 0x2000, 0x1000, 0xa5970e5c )
	ROM_LOAD( "solar.6r", 0x3000, 0x1000, 0xb763fff2 )
ROM_END


static const char *solarq_sample_names[] =
{
    0	/* end of array */
};


void solarq_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_16K, CCPU_MONITOR_64COL);
}


CINEMA_MACHINE (solarq, 0, 0, 1024, 768)

struct GameDriver solarq_driver =
{
	__FILE__,
	0,
	"solarq",
	"Solar Quest",
	"1981",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&solarq_machine_driver,
	0,

	solarq_rom,
	cinemat16k_rom_decode, 0,
	solarq_sample_names,
	0,	/* sound_prom */

	solarq_input_ports,

	color_prom_64color, 0, 0,
	ORIENTATION_ROTATE_180,

	0,0
};



/***************************************************************************

  Demon

***************************************************************************/

INPUT_PORTS_START ( demon_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_DIPNAME( SW7, SW7OFF, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING( SW7ON,  "Free Play" )
	PORT_DIPSETTING( SW7OFF, "Normal" )
	PORT_DIPNAME( SW6, SW6OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "Off" )
	PORT_DIPSETTING( SW6ON,  "On" )
	PORT_DIPNAME( SW5, SW5OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Off" )
	PORT_DIPSETTING( SW5ON,  "On" )
	PORT_DIPNAME( SW4, SW4OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW4OFF, "Off" )
	PORT_DIPSETTING( SW4ON,  "On" )
	PORT_DIPNAME( SW3, SW3OFF, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING( SW3OFF, "Off" )
	PORT_DIPSETTING( SW3ON,  "On" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW2ON |SW1OFF, "2 coins/1 credit" )
	PORT_DIPSETTING( SW2ON |SW1ON,  "4 coins/3 credits" )
	PORT_DIPSETTING( SW2OFF|SW1OFF, "1 coin/1 credit" )
	PORT_DIPSETTING( SW2OFF|SW1ON,  "2 coins/3 credits" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN ) /* also mapped to Button 3, player 2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_TILT )

	PORT_START /* inputs low */
	PORT_DIPNAME( 0x80, 0x80, "Test Pattern", IP_KEY_NONE )
	PORT_DIPSETTING( 0x80, "Off" )
	PORT_DIPSETTING( 0x00, "On" )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( demon_rom )
	ROM_REGION(0x4000)	/* 16k for code */
	ROM_LOAD( "demon.7t", 0x0000, 0x1000, 0x866596c1 )
	ROM_LOAD( "demon.7p", 0x1000, 0x1000, 0x1109e2f1 )
	ROM_LOAD( "demon.7u", 0x2000, 0x1000, 0xd447a3c3 )
	ROM_LOAD( "demon.7r", 0x3000, 0x1000, 0x64b515f0 )
ROM_END


static const char *demon_sample_names[] =
{
    0	/* end of array */
};


void demon_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_16K, CCPU_MONITOR_BILEV);
}


CINEMA_MACHINE (demon, 0, 0, 1024, 800)


struct GameDriver demon_driver =
{
	__FILE__,
	0,
	"demon",
	"Demon",
	"1982",
	"Rock-Ola",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	GAME_NOT_WORKING,
	&demon_machine_driver,
	0,

	demon_rom,
	cinemat16k_rom_decode, 0,
	demon_sample_names,
	0,	/* sound_prom */

	demon_input_ports,

	color_prom_bilevel, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  War of the Worlds

***************************************************************************/

INPUT_PORTS_START ( wotw_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7OFF, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7OFF, "Off" )
	PORT_DIPSETTING( SW7ON,  "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "Off" )
	PORT_DIPSETTING( SW6ON,  "On" )
	PORT_DIPNAME( SW4, SW4OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON,  "2 coins/3 credits" )
	PORT_DIPSETTING( SW4OFF, "1 coin/1 credit" )
	PORT_DIPNAME( SW2, SW2OFF, "Ships", IP_KEY_NONE )
	PORT_DIPSETTING( SW2OFF, "3" )
	PORT_DIPSETTING( SW2ON,  "5" )
	PORT_BIT ( SW5|SW3|SW1, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_2WAY )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START /* joystick X */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* joystick Y */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END


ROM_START( wotw_rom )
	ROM_REGION(0x4000)	/* 16k for code */
	ROM_LOAD( "wow_le.t7", 0x0000, 0x1000, 0xb16440f9 )
	ROM_LOAD( "wow_lo.p7", 0x1000, 0x1000, 0xbfdf4a5a )
	ROM_LOAD( "wow_ue.u7", 0x2000, 0x1000, 0x9b5cea48 )
	ROM_LOAD( "wow_uo.r7", 0x3000, 0x1000, 0xc9d3c866 )
ROM_END


static const char *wotw_sample_names[] =
{
    0	/* end of array */
};


void wotw_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_16K, CCPU_MONITOR_WOWCOL);
}


CINEMA_MACHINE (wotw, 0, 0, 1024, 768)


struct GameDriver wotw_driver =
{
	__FILE__,
	0,
	"wotw",
	"War of the Worlds",
	"1981",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&wotw_machine_driver,
	0,

	wotw_rom,
	cinemat16k_rom_decode, 0,
	wotw_sample_names,
	0,	/* sound_prom */

	wotw_input_ports,

	color_prom_wotw, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};



/***************************************************************************

  Boxing Bugs

***************************************************************************/

INPUT_PORTS_START ( boxingb_input_ports )
	PORT_START /* switches */
	PORT_BITX( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )
	PORT_BITX( SW7, SW7OFF, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( SW7OFF, "Off" )
	PORT_DIPSETTING( SW7ON,  "On" )
	PORT_DIPNAME( SW6, SW6OFF, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING( SW6OFF, "Off" )
	PORT_DIPSETTING( SW6ON,  "On" )
	PORT_DIPNAME( SW5, SW5OFF, "Attract Sound", IP_KEY_NONE )
	PORT_DIPSETTING( SW5OFF, "Off" )
	PORT_DIPSETTING( SW5ON,  "On" )
	PORT_DIPNAME( SW4, SW4ON, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING( SW4ON,  "at 30,000" )
	PORT_DIPSETTING( SW4OFF, "at 50,000" )
	PORT_DIPNAME( SW3, SW3ON, "Cannons", IP_KEY_NONE )
	PORT_DIPSETTING( SW3OFF, "3" )
	PORT_DIPSETTING( SW3ON,  "5" )
	PORT_DIPNAME( SW2|SW1, SW2OFF|SW1OFF, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( SW2ON |SW1OFF, "2 coins/1 credit" )
	PORT_DIPSETTING( SW2ON |SW1ON,  "4 coins/3 credits" )
	PORT_DIPSETTING( SW2OFF|SW1OFF, "1 coin/1 credit" )
	PORT_DIPSETTING( SW2OFF|SW1ON,  "2 coins/3 credits" )

	PORT_START /* inputs high */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START /* inputs low */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* joystick X */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_X, 100, 0, 0, 255 )

	PORT_START /* joystick Y */
	PORT_ANALOG ( 0xff, 0x7f, IPT_AD_STICK_Y, 100, 0, 0, 255 )
INPUT_PORTS_END


ROM_START( boxingb_rom )
	ROM_REGION(0x8000)	/* 32k for code */
	ROM_LOAD( "u1a", 0x0000, 0x1000, 0xd3115b0f )
	ROM_LOAD( "u1b", 0x1000, 0x1000, 0x3a44268d )
	ROM_LOAD( "u2a", 0x2000, 0x1000, 0xc97a9cbb )
	ROM_LOAD( "u2b", 0x3000, 0x1000, 0x98d34ff5 )
	ROM_LOAD( "u3a", 0x4000, 0x1000, 0x5bb3269b )
	ROM_LOAD( "u3b", 0x5000, 0x1000, 0x85bf83ad )
	ROM_LOAD( "u4a", 0x6000, 0x1000, 0x25b51799 )
	ROM_LOAD( "u4b", 0x7000, 0x1000, 0x7f41de6a )
ROM_END


static const char *boxingb_sample_names[] =
{
    0	/* end of array */
};


void boxingb_init_machine (void)
{
	ccpu_Config (1, CCPU_MEMSIZE_32K, CCPU_MONITOR_WOWCOL);
}


CINEMA_MACHINE (boxingb, 0, 0, 1024, 768)


struct GameDriver boxingb_driver =
{
	__FILE__,
	0,
	"boxingb",
	"Boxing Bugs",
	"1981",
	"Cinematronics",
	"Aaron Giles (Mame Driver)\nZonn Moore (hardware info)\nJeff Mitchell (hardware info)\n"
	"Neil Bradley (hardware info)\n"VECTOR_TEAM,
	0,
	&boxingb_machine_driver,
	0,

	boxingb_rom,
	cinemat32k_rom_decode, 0,
	boxingb_sample_names,
	0,	/* sound_prom */

	boxingb_input_ports,

	color_prom_wotw, 0, 0,
	ORIENTATION_FLIP_Y,

	0,0
};





