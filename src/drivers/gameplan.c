#include "driver.h"
#include "vidhrdw/generic.h"

int gameplan_vh_start(void);
void gameplan_vh_stop(void);
int gameplan_video_r(int offset);
void gameplan_video_w(int offset, int data);
int gameplan_sound_r(int offset);
void gameplan_sound_w(int offset, int data);
int gameplan_via5_r(int offset);
void gameplan_via5_w(int offset, int data);
void gameplan_vh_screenrefresh(struct osd_bitmap *bitmap);
void gameplan_select_port(int offset, int data);
int gameplan_read_port(int offset);

static int gameplan_current_port;

void gameplan_select_port(int offset, int data)
{
#ifdef VERY_VERBOSE
	if (errorlog)
	{
		fprintf(errorlog, "VIA 2: PC %04x: %x -> reg%X\n",
				cpu_getpc(), data, offset);
	}
#endif /* VERY_VERBOSE */

	switch (offset)
	{
		case 0x00:
			switch(data)
			{
				case 0x01: gameplan_current_port = 0; break;
				case 0x02: gameplan_current_port = 1; break;
				case 0x04: gameplan_current_port = 2; break;
				case 0x08: gameplan_current_port = 3; break;
				case 0x80: gameplan_current_port = 4; break;
				case 0x40: gameplan_current_port = 5; break;

				default:
#ifdef VERBOSE
					if (errorlog)
						fprintf(errorlog, "  VIA 2: strange port request byte: %02x\n", data);
#endif
					return;
			}

#ifdef VERBOSE
			if (errorlog) fprintf(errorlog, "  VIA 2: selected port %d\n", gameplan_current_port);
#endif
			break;

		case 0x02:
#ifdef VERBOSE
			if (errorlog) fprintf(errorlog, "  VIA 2: wrote %02x to Data Direction Register B\n", data);
#endif
			break;

		case 0x03:
#ifdef VERBOSE
			if (errorlog) fprintf(errorlog, "  VIA 2: wrote %02x to Data Direction Register A\n", data);
#endif
			break;

		case 0x0c:
			if (errorlog)
			{
				if (data == 0xec || data == 0xcc)
				{
#ifdef VERBOSE
					fprintf(errorlog,
							"  VIA 2: initialised Peripheral Control Register to 0x%02x for VIA 2\n",
							data);
#endif
				}
				else
					fprintf(errorlog,
							"  VIA 2: unusual Peripheral Control Register value 0x%02x for VIA 2\n",
							data);
			}
			break;

		default:
			if (errorlog)
				fprintf(errorlog, "  VIA 2: unexpected register written to in VIA 2: %02x -> %02x\n",
						data, offset);
			break;
	}
}

int gameplan_read_port(int offset)
{
	return readinputport(gameplan_current_port);
}

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x03ff, MRA_RAM },
    { 0x032d, 0x03d8, MRA_RAM }, /* note: 300-32c and 3d9-3ff is
								  * written but never read?
								  * (write by code at e1df and e1e9,
								  * 32d is read by e258)*/
    { 0x2000, 0x200f, gameplan_video_r },
    { 0x2801, 0x2801, gameplan_read_port },
	{ 0x3000, 0x300f, gameplan_sound_r },
    { 0x9000, 0xffff, MRA_ROM },

    { -1 }						/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x03ff, MWA_RAM },
    { 0x2000, 0x200f, gameplan_video_w },		/* VIA 1 */
    { 0x2800, 0x280f, gameplan_select_port },	/* VIA 2 */
    { 0x3000, 0x300f, gameplan_sound_w },       /* VIA 3 */
    { 0x9000, 0xffff, MWA_ROM },

	{ -1 }						/* end of table */
};

static struct MemoryReadAddress readmem_snd[] =
{
	{ 0x0000, 0x0026, MRA_RAM },
	{ 0x01f6, 0x01ff, MRA_RAM },
	{ 0x0800, 0x080f, gameplan_via5_r },

#if 0
    { 0xa001, 0xa001, gameplan_ay_3_8910_1_r }, /* AY-3-8910 */
#else
    { 0xa001, 0xa001, soundlatch_r }, /* AY-3-8910 */
#endif

    { 0xf800, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_snd[] =
{
	{ 0x0000, 0x0026, MWA_RAM },
	{ 0x01f6, 0x01ff, MWA_RAM },
	{ 0x0800, 0x080f, gameplan_via5_w },

#if 0
    { 0xa000, 0xa000, gameplan_ay_3_8910_0_w }, /* AY-3-8910 */
    { 0xa002, 0xa002, gameplan_ay_3_8910_2_w }, /* AY-3-8910 */
#else
    { 0xa000, 0xa000, AY8910_control_port_0_w }, /* AY-3-8910 */
    { 0xa002, 0xa002, AY8910_write_port_0_w }, /* AY-3-8910 */
#endif

	{ 0xf800, 0xffff, MWA_ROM },

	{ -1 }  /* end of table */
};

int gameplan_interrupt(void)
{
	return 1;
}

INPUT_PORTS_START( kaos_input_ports )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
			  "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
			  "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
			  "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )	 /* coin sw. no.3 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )	 /* coin sw. no.2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )	 /* coin sw. no.1 */

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
			  "P1 Jump", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
			  "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
			  "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
			  "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
			  "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
			  "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
			  "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
			  "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
			  "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
			  "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */

	PORT_DIPNAME(0x0f, 0x0f, "Coinage", IP_KEY_NONE ) /* -> 039F */
	PORT_DIPSETTING(   0x0f, " 1 Coin / 1 Credit" )
	PORT_DIPSETTING(   0x0e, " 1 Coin / 1 Credit" )
	PORT_DIPSETTING(   0x0d, " 1 Coin / 2 Credits" )
	PORT_DIPSETTING(   0x0c, " 1 Coin / 3 Credits" )
	PORT_DIPSETTING(   0x0b, " 1 Coin / 4 Credits" )
	PORT_DIPSETTING(   0x0a, " 1 Coin / 5 Credits" )
	PORT_DIPSETTING(   0x09, " 1 Coin / 6 Credits" )
	PORT_DIPSETTING(   0x08, " 1 Coin / 7 Credits" )
	PORT_DIPSETTING(   0x07, " 1 Coin / 8 Credits" )
	PORT_DIPSETTING(   0x06, " 1 Coin / 9 Credits" )
	PORT_DIPSETTING(   0x05, " 1 Coin / 10 Credits" )
	PORT_DIPSETTING(   0x04, " 1 Coin / 11 Credits" )
	PORT_DIPSETTING(   0x03, " 1 Coin / 12 Credits" )
	PORT_DIPSETTING(   0x02, " 1 Coin / 13 Credits" )
	PORT_DIPSETTING(   0x01, " 1 Coin / 14 Credits" )
	PORT_DIPSETTING(   0x00, "2 Coins / 1 Credit" )

	PORT_DIPNAME(0x10, 0x10, "A4", IP_KEY_NONE ) /* -> 039A */
	PORT_DIPSETTING(   0x10, "10" )
	PORT_DIPSETTING(   0x00, "00" )

	PORT_DIPNAME(0x60, 0x20, "A5", IP_KEY_NONE ) /* -> 039C */
	PORT_DIPSETTING(   0x60, "10" )
	PORT_DIPSETTING(   0x40, "20" )
	PORT_DIPSETTING(   0x20, "30" )
	PORT_DIPSETTING(   0x00, "40" )

	PORT_DIPNAME(0x80, 0x80, "A7", IP_KEY_NONE ) /* -> 039D */
	PORT_DIPSETTING(   0x80, "Insert Coin To Play" )
	PORT_DIPSETTING(   0x00, "Free Play" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */

	PORT_DIPNAME(0x01, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "3" )
	PORT_DIPSETTING(   0x00, "4" )

	PORT_DIPNAME(0x02, 0x00, "$0397 (?)", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "2" )
	PORT_DIPSETTING(   0x02, "1" )

	PORT_DIPNAME(0x0c, 0x00, "$03a4 (?)", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "3" )
	PORT_DIPSETTING(   0x04, "2" )
	PORT_DIPSETTING(   0x08, "1" )
	PORT_DIPSETTING(   0x0c, "0" )

	PORT_DIPNAME(0x10, 0x00, "$03be (?)", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "c" )
	PORT_DIPSETTING(   0x10, "8" )

	PORT_DIPNAME(0x20, 0x00, "$03a5 (?)", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "1" )
	PORT_DIPSETTING(   0x20, "0" )

	PORT_DIPNAME(0x40, 0x00, "$0398 (?)", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "40" )
	PORT_DIPSETTING(   0x40, "0" )

	PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(   0x80, "Upright" )
	PORT_DIPSETTING(   0x00, "Cocktail" )

INPUT_PORTS_END


INPUT_PORTS_START( killcom_input_ports )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
			  "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
			  "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
			  "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )	 /* coin sw. no.3 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )	 /* coin sw. no.2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )	 /* coin sw. no.1 */

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1,
			  "P1 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
			  "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
			  "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
			  "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
			  "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1,
			  "P1 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
			  "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1,
			  "P1 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2,
			  "P2 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
			  "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
			  "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
			  "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
			  "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2,
			  "P2 Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
			  "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2,
			  "P2 Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */

	PORT_DIPNAME(0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(   0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(   0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(   0x00, "Free Play" )

	PORT_DIPNAME(0x04, 0x04, "Dip A 3", IP_KEY_NONE )
	PORT_DIPSETTING(   0x04, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x08, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "4" )
	PORT_DIPSETTING(   0x08, "5" )

	PORT_DIPNAME(0x10, 0x10, "Dip A 5", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x20, 0x20, "Dip A 6", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0xc0, 0xc0, "Reaction", IP_KEY_NONE )
	PORT_DIPSETTING(   0xc0, "Slowest" )
	PORT_DIPSETTING(   0x80, "Slow" )
	PORT_DIPSETTING(   0x40, "Fast" )
	PORT_DIPSETTING(   0x00, "Fastest" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */

	PORT_DIPNAME(0x01, 0x01, "Dip B 1", IP_KEY_NONE )
	PORT_DIPSETTING(   0x01, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x02, 0x02, "Dip B 2", IP_KEY_NONE )
	PORT_DIPSETTING(   0x02, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x04, 0x04, "Dip B 3", IP_KEY_NONE )
	PORT_DIPSETTING(   0x04, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x08, 0x08, "Dip B 4", IP_KEY_NONE )
	PORT_DIPSETTING(   0x08, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x10, 0x10, "Dip B 5", IP_KEY_NONE )
	PORT_DIPSETTING(   0x10, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x20, 0x20, "Dip B 6", IP_KEY_NONE )
	PORT_DIPSETTING(   0x20, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(   0x40, "Off" )
	PORT_DIPSETTING(   0x00, "On" )

	PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(   0x80, "Upright" )
	PORT_DIPSETTING(   0x00, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( megatack_input_ports )
        PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
                          "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
                          "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
                          "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )      /* coin sw. no.3 */
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )      /* coin sw. no.2 */
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )      /* coin sw. no.1 */

        PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

        PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
                          "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
                          "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
                          "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
                          "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
                          "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

        PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
                          "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
                          "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
                          "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
                          "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
                          "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

        PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */
        PORT_DIPNAME(0x03, 0x03, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(   0x01, "2 Coins/1 Credit" )
        PORT_DIPSETTING(   0x02, "2 Coins/1 Credits" )
        PORT_DIPSETTING(   0x03, "1 Coin/1 Credit" )
        PORT_DIPSETTING(   0x00, "Free Play" )
        PORT_DIPNAME(0x04, 0x04, "Dip A 3", IP_KEY_NONE )
        PORT_DIPSETTING(   0x04, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x08, 0x08, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(   0x08, "3" )
        PORT_DIPSETTING(   0x00, "4" )
        PORT_DIPNAME(0x10, 0x10, "Dip A 5", IP_KEY_NONE )
        PORT_DIPSETTING(   0x10, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x20, 0x20, "Dip A 6", IP_KEY_NONE )
        PORT_DIPSETTING(   0x20, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x40, 0x40, "Dip A 7", IP_KEY_NONE )
        PORT_DIPSETTING(   0x40, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x80, 0x80, "Dip A 8", IP_KEY_NONE )
        PORT_DIPSETTING(   0x80, "Off" )
        PORT_DIPSETTING(   0x00, "On" )

        PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */
        PORT_DIPNAME(0x07, 0x07, "Bonus Life", IP_KEY_NONE )
        PORT_DIPSETTING(   0x07, "20000" )
        PORT_DIPSETTING(   0x06, "30000" )
        PORT_DIPSETTING(   0x05, "40000" )
        PORT_DIPSETTING(   0x04, "50000" )
        PORT_DIPSETTING(   0x03, "60000" )
        PORT_DIPSETTING(   0x02, "70000" )
        PORT_DIPSETTING(   0x01, "80000" )
        PORT_DIPSETTING(   0x00, "90000" )
        PORT_DIPNAME(0x08, 0x08, "Dip B 4", IP_KEY_NONE )
        PORT_DIPSETTING(   0x08, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x10, 0x10, "Monitor View", IP_KEY_NONE )
        PORT_DIPSETTING(   0x10, "Direct" )
        PORT_DIPSETTING(   0x00, "Mirror" )
        PORT_DIPNAME(0x20, 0x20, "Monitor Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(   0x20, "Horizontal" )
        PORT_DIPSETTING(   0x00, "Vertical" )
        PORT_DIPNAME(0x40, 0x40, "Flip Screen", IP_KEY_NONE )
        PORT_DIPSETTING(   0x40, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(   0x80, "Upright" )
        PORT_DIPSETTING(   0x00, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( challeng_input_ports )
        PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_TILT,
                          "Slam", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, 0,
                          "Do Tests", OSD_KEY_F1, IP_JOY_NONE, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE,
                          "Select Test", OSD_KEY_F2, IP_JOY_NONE, 0)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )      /* coin sw. no.3 */
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )      /* coin sw. no.2 */
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )      /* coin sw. no.1 */

        PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )  /* 2 player start */
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  /* 1 player start */

        PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1,
                          "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1,
                          "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1,
                          "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1,
                          "P1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1,
                          "P1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

        PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2,
                          "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2,
                          "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2,
                          "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2,
                          "P2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2,
                          "P2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

        PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */
        PORT_DIPNAME(0x03, 0x03, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(   0x01, "2 Coins/1 Credit" )
        PORT_DIPSETTING(   0x02, "2 Coins/3 Credits" )
        PORT_DIPSETTING(   0x03, "1 Coin/1 Credit" )
        PORT_DIPSETTING(   0x00, "Free Play" )
        PORT_DIPNAME(0x04, 0x04, "Dip A 3", IP_KEY_NONE )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPSETTING(   0x04, "Off" )
        PORT_DIPNAME(0x08, 0x00, "Dip A 4", IP_KEY_NONE )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPSETTING(   0x08, "Off" )
        PORT_DIPNAME(0x10, 0x10, "Dip A 5", IP_KEY_NONE )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPSETTING(   0x10, "Off" )
        PORT_DIPNAME(0x20, 0x00, "Dip A 6", IP_KEY_NONE )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPSETTING(   0x20, "Off" )
        PORT_DIPNAME(0xc0, 0xc0, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(   0xc0, "3" )
        PORT_DIPSETTING(   0x80, "4" )
        PORT_DIPSETTING(   0x40, "5" )
        PORT_DIPSETTING(   0x00, "6" )

        PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */
        PORT_DIPNAME(0x07, 0x07, "Bonus Life", IP_KEY_NONE )
        PORT_DIPSETTING(   0x01, "20000" )
        PORT_DIPSETTING(   0x00, "30000" )
        PORT_DIPSETTING(   0x07, "40000" )
        PORT_DIPSETTING(   0x06, "50000" )
        PORT_DIPSETTING(   0x05, "60000" )
        PORT_DIPSETTING(   0x04, "70000" )
        PORT_DIPSETTING(   0x03, "80000" )
        PORT_DIPSETTING(   0x02, "90000" )
        PORT_DIPNAME(0x08, 0x08, "Dip B 4", IP_KEY_NONE )
        PORT_DIPSETTING(   0x08, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x10, 0x10, "Monitor View", IP_KEY_NONE )
        PORT_DIPSETTING(   0x10, "Direct" )
        PORT_DIPSETTING(   0x00, "Mirror" )
        PORT_DIPNAME(0x20, 0x20, "Monitor Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(   0x20, "Horizontal" )
        PORT_DIPSETTING(   0x00, "Vertical" )
        PORT_DIPNAME(0x40, 0x40, "Flip Screen", IP_KEY_NONE )
        PORT_DIPSETTING(   0x40, "Off" )
        PORT_DIPSETTING(   0x00, "On" )
        PORT_DIPNAME(0x80, 0x80, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(   0x80, "Upright" )
        PORT_DIPSETTING(   0x00, "Cocktail" )
INPUT_PORTS_END



static unsigned char palette[] =
{
	0xff,0xff,0xff, /* 0 WHITE   */
	0x20,0xff,0xff, /* 1 CYAN    */
	0xff,0x20,0xff, /* 2 MAGENTA */
	0x20,0x20,0xFF, /* 3 BLUE    */
	0xff,0xff,0x20, /* 4 YELLOW  */
	0x20,0xff,0x20, /* 5 GREEN   */
	0xff,0x20,0x20, /* 6 RED     */
	0x00,0x00,0x00, /* 7 BLACK   */
};


static unsigned short colortable[] =
{
	0, 0,	0, 1,	0, 2,	0, 3,	0, 4,	0, 5,	0, 6,	0, 7,
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chips */
	1500000,	/* 1.5 MHz ? */
	{ 0x20 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver =
{
    /* basic machine hardware */
    {							/* MachineCPU */
		{
			CPU_M6502,
			3579000 / 4,		/* 3.579 / 4 MHz */
			0,					/* memory_region */
			readmem, writemem, 0, 0,
			gameplan_interrupt,1 /* 1 interrupt per frame */
		},
		{
			CPU_M6502,
			3579000 / 4,		/* 3.579 / 4 MHz */
			1,					/* memory_region */
			readmem_snd,writemem_snd,0,0,
			gameplan_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,							/* CPU slices per frame */
    0,							/* init_machine */

    /* video hardware */
    32*8, 32*8,					/* screen_width, height */
    { 0, 32*8-1, 0, 32*8-1 },		/* visible_area */
    0,
    sizeof(palette)/3, sizeof(colortable)/sizeof(short),
	0,

	VIDEO_TYPE_RASTER, 0,
	gameplan_vh_start,
	gameplan_vh_stop,
	gameplan_vh_screenrefresh,

	0, 0, 0, 0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
the manuals for Megattack and the schematics for Kaos are up
on spies now. I took a quick look at the rom mapping for kaos
and it looks like the roms are split this way:

9000 G2 bot 2k
9800 J2 bot 2k
A000 J1 bot 2k
A800 G1 bot 2k
B000 F1 bot 2k
B800 E1 bot 2k

D000 G2 top 2k
D800 J2 top 2k
E000 J1 top 2k
E800 G1 top 2k
F000 F1 top 2k
F800 E1 top 2k

there are three 6522 VIAs, at 2000, 2800, and 3000
*/

ROM_START( kaos_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "kaosab.g2", 0x9000, 0x0800, 0x1bf0f6b2 )
    ROM_CONTINUE(		   0xd000, 0x0800			  )
    ROM_LOAD( "kaosab.j2", 0x9800, 0x0800, 0xa40d1c5d )
    ROM_CONTINUE(		   0xd800, 0x0800			  )
    ROM_LOAD( "kaosab.j1", 0xa000, 0x0800, 0x5768e2da )
    ROM_CONTINUE(		   0xe000, 0x0800			  )
    ROM_LOAD( "kaosab.g1", 0xa800, 0x0800, 0x8ab39f1d )
    ROM_CONTINUE(		   0xe800, 0x0800			  )
    ROM_LOAD( "kaosab.f1", 0xb000, 0x0800, 0x26d3e973 )
    ROM_CONTINUE(		   0xf000, 0x0800			  )
    ROM_LOAD( "kaosab.e1", 0xb800, 0x0800, 0x42b95983 )
    ROM_CONTINUE(		   0xf800, 0x0800			  )

    ROM_REGION(0x10000)
	ROM_LOAD( "kaossnd.e1", 0xf800, 0x800, 0xd406d7a6 )
ROM_END


ROM_START( killcom_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "killcom.e2", 0xc000, 0x800, 0x5cf2ceb6 )
    ROM_LOAD( "killcom.f2", 0xc800, 0x800, 0x28b53265 )
    ROM_LOAD( "killcom.g2", 0xd000, 0x800, 0xc85a6fe6 )
    ROM_LOAD( "killcom.j2", 0xd800, 0x800, 0xdcfaa92a )
    ROM_LOAD( "killcom.j1", 0xe000, 0x800, 0x8772c5e8 )
    ROM_LOAD( "killcom.g1", 0xe800, 0x800, 0xf2738623 )
    ROM_LOAD( "killcom.f1", 0xf000, 0x800, 0xca4212d4 )
    ROM_LOAD( "killcom.e1", 0xf800, 0x800, 0x6ed6e10a )

    ROM_REGION(0x10000)
	ROM_LOAD( "killsnd.e1", 0xf800, 0x800, 0x5ac95bbb )
ROM_END

ROM_START( megatack_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "megattac.e2", 0xc000, 0x800, 0xe1dfd187 )
    ROM_LOAD( "megattac.f2", 0xc800, 0x800, 0x64875115 )
    ROM_LOAD( "megattac.g2", 0xd000, 0x800, 0xcf270d9d )
    ROM_LOAD( "megattac.j2", 0xd800, 0x800, 0xf8d4bdea )
    ROM_LOAD( "megattac.j1", 0xe000, 0x800, 0x2ab96ecb )
    ROM_LOAD( "megattac.g1", 0xe800, 0x800, 0x57427484 )
    ROM_LOAD( "megattac.f1", 0xf000, 0x800, 0xd0fdd29f )
    ROM_LOAD( "megattac.e1", 0xf800, 0x800, 0x5a5799db )

    ROM_REGION(0x10000)
	ROM_LOAD( "megatsnd.e1", 0xf800, 0x800, 0xf47bab9f )
ROM_END

ROM_START( challeng_rom )
    ROM_REGION(0x10000)
    ROM_LOAD( "chall.6", 0xa000, 0x1000, 0xa9ed5d9d )
    ROM_LOAD( "chall.5", 0xb000, 0x1000, 0xf54a2ac8 )
    ROM_LOAD( "chall.4", 0xc000, 0x1000, 0x9dd93663 )
    ROM_LOAD( "chall.3", 0xd000, 0x1000, 0x53801d16 )
    ROM_LOAD( "chall.2", 0xe000, 0x1000, 0x1702df08 )
    ROM_LOAD( "chall.1", 0xf000, 0x1000, 0x0cfbdc29 )

    ROM_REGION(0x10000)
    /* the ROM is missing! */
ROM_END



static int kaos_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x03c8], "\x84\x00\x00", 3) == 0 &&
		memcmp(&RAM[0x03dd], "\x43\x00\x00", 3) == 0 &&
		memcmp(&RAM[0x03e0], "\x50\x50\x43", 3) == 0 &&
		memcmp(&RAM[0x03f5], "\x54\x4f\x44", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f, &RAM[0x03c8], 0x30);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void kaos_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f, &RAM[0x03c8], 0x30);
		osd_fclose(f);
	}
}


/* I give up!  This doesn't work - the highscore is at 0x88 or 0x91 or something... */
#if 0
static int killcom_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0088], "\x47\x00", 2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f, &RAM[0x0088], 0x2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void killcom_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f, &RAM[0x0091], 0x2);
		osd_fclose(f);
	}
}
#endif /* 0 */


struct GameDriver kaos_driver =
{
	"Kaos",
	"kaos",
	"Chris Moore (MAME driver)\n"
	"Al Kossow (hardware info)\n"
	"Santeri Saarimaa (not a sausage)",
	&machine_driver,

	kaos_rom,
	0, 0, 0, 0,					/* sound_prom */

	kaos_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	kaos_hiload, kaos_hisave
};


struct GameDriver killcom_driver =
{
	"Killer Comet",
	"killcom",
	"Chris Moore (MAME driver)\n"
	"Al Kossow (hardware info)\n"
	"Santeri Saarimaa (hardware info)",
	&machine_driver,

	killcom_rom,
	0, 0, 0, 0,					/* sound_prom */

	killcom_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	/* killcom_hiload, killcom_hisave */ 0, 0
};


struct GameDriver megatack_driver =
{
	"MegaTack",
	"megatack",
	"Chris Moore (MAME driver)\n"
	"Al Kossow (hardware info)\n"
	"Santeri Saarimaa (hardware info)",
	&machine_driver,

	megatack_rom,
	0, 0, 0, 0,					/* sound_prom */

	megatack_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver challeng_driver =
{
        "Challenger",
        "challeng",
        "Chris Moore (MAME driver)\nSanteri Saarimaa (hardware info)",
        &machine_driver,

        challeng_rom,
        0, 0, 0, 0,					/* sound_prom */

        challeng_input_ports,

        0, palette, colortable,
        ORIENTATION_DEFAULT,

        0, 0
};
