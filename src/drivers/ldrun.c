/****************************************************************************

Irem "M62" system

There's two crystals on Kid Kiki. 24.00 MHz and 3.579545 MHz for sound

TODO:
- in ldrun2, tile/sprite priority in the intermission scenes is not implemented
  (sprites should disappear behind the door). Same goes for the title screen in
  ldrun3. However when the sprite falls in the hole in ldrun3, it is masked by
  another sprite. So is there really a priority control?
- should sprites in ldrun be clipped at the top of the screen? (just below the
  score). I was doing that before, but I have removed the code because it broke
  ldrun3 title screen, where the title scrolls in from above.

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void irem_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void spelunk2_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int ldrun_vh_start( void );
int kidniki_vh_start( void );
int spelunk2_vh_start( void );
void irem_flipscreen_w(int offset,int data);
void ldrun3_vscroll_w(int offset,int data);
void ldrun4_hscroll_w(int offset,int data);
void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void ldrun4_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void lotlot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void kidniki_vh_screenrefresh(struct osd_bitmap *bitmap,int fullrefresh);
void spelunk2_vh_screenrefresh(struct osd_bitmap *bitmap,int fullrefresh);

extern struct MemoryReadAddress irem_sound_readmem[];
extern struct MemoryWriteAddress irem_sound_writemem[];
extern struct AY8910interface irem_ay8910_interface;
extern struct MSM5205interface irem_msm5205_interface;
void irem_sound_cmd_w(int offset, int value);

void irem_background_hscroll_w( int offset, int data );
void irem_background_vscroll_w( int offset, int data );
void kidniki_text_vscroll_w( int offset, int data );
void kidniki_background_bank_w( int offset, int data );

void spelunk2_gfxport_w( int offset, int data );

extern unsigned char *irem_textram;
extern int irem_textram_size;



/* Lode Runner 2 seems to have a simple protection on the bank switching */
/* circuitry. It writes data to ports 0x80 and 0x81, then reads port 0x80 */
/* a variable number of times (discarding the result) and finally retrieves */
/* data from the bankswitched ROM area. */
/* Since the data written to 0x80 is always the level number, I just use */
/* that to select the ROM. The only exception I make is a special case used in */
/* service mode to test the ROMs. */
static int ldrun2_bankswap;

int ldrun2_bankswitch_r(int offset)
{
	if (ldrun2_bankswap)
	{
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


		ldrun2_bankswap--;

		/* swap to bank #1 on second read */
		if (ldrun2_bankswap == 0)
			cpu_setbank(1,&RAM[0x12000]);
	}
	return 0;
}

void ldrun2_bankswitch_w(int offset,int data)
{
	int bankaddress;
	static int bankcontrol[2];
	int banks[30] =
	{
		0,0,0,0,0,1,0,1,0,0,
		0,1,1,1,1,1,0,0,0,0,
		1,0,1,1,1,1,1,1,1,1
	};
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankcontrol[offset] = data;
	if (offset == 0)
	{
		if (data < 1 || data > 30)
		{
if (errorlog) fprintf(errorlog,"unknown bank select %02x\n",data);
			return;
		}
		bankaddress = 0x10000 + (banks[data-1] * 0x2000);
		cpu_setbank(1,&RAM[bankaddress]);
	}
	else
	{
		if (bankcontrol[0] == 0x01 && data == 0x0d)
		/* special case for service mode */
			ldrun2_bankswap = 2;
		else ldrun2_bankswap = 0;
	}
}


/* Lode Runner 3 has, it seems, a poor man's protection consisting of a PAL */
/* (I think; it's included in the ROM set) which is read at certain times, */
/* and the game crashes if ti doesn't match the expected values. */
int ldrun3_prot_5_r(int offset)
{
	return 5;
}

int ldrun3_prot_7_r(int offset)
{
	return 7;
}


void ldrun4_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ((data & 0x01) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
}

static void kidniki_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x0f) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

void spelunk2_bankswitch_w( int offset, int data )
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	cpu_setbank(1,&RAM[0x20000 + 0x1000 * ((data & 0xc0)>>6)]);
	cpu_setbank(2,&RAM[0x10000 + 0x0400 *  (data & 0x3c)]);
}



static struct MemoryReadAddress ldrun_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ldrun2_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun2_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ldrun3_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xc800, ldrun3_prot_5_r },
	{ 0xcc00, 0xcc00, ldrun3_prot_7_r },
	{ 0xcfff, 0xcfff, ldrun3_prot_7_r },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun3_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ldrun4_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun4_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xc800, ldrun4_bankswitch_w },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress lotlot_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xafff, MRA_RAM },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress lotlot_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xafff, MWA_RAM, &irem_textram, &irem_textram_size },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress kidniki_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xafff, MRA_RAM },
	{ 0xd000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress kidniki_writemem[] = {
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xafff, videoram_w, &videoram, &videoram_size },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, MWA_RAM, &irem_textram, &irem_textram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress spelunk2_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_BANK1 },
	{ 0x9000, 0x9fff, MRA_BANK2 },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress spelunk2_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xbfff, videoram_w, &videoram, &videoram_size },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xcfff, MWA_RAM, &irem_textram, &irem_textram_size },
	{ 0xd000, 0xd002, spelunk2_gfxport_w },
	{ 0xd003, 0xd003, spelunk2_bankswitch_w },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort ldrun_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ -1 }	/* end of table */
};

static struct IOReadPort ldrun2_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ 0x80, 0x80, ldrun2_bankswitch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun2_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ 0x80, 0x81, ldrun2_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun3_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ 0x80, 0x80, ldrun3_vscroll_w },
	/* 0x81 used too, don't know what for */
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun4_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ 0x82, 0x83, ldrun4_hscroll_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort kidniki_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ 0x80, 0x81, irem_background_hscroll_w },
	{ 0x82, 0x83, kidniki_text_vscroll_w },
	{ 0x84, 0x84, kidniki_background_bank_w },
	{ 0x85, 0x85, kidniki_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort spelunk2_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, irem_flipscreen_w },	/* + coin counters */
	{ -1 }	/* end of table */
};



#define IN0_PORT \
/* Start 1 & 2 also restarts and freezes the game with stop mode on \
   and are used in test mode to enter and esc the various tests */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 ) \
	/* service coin must be active for 19 frames to be consistently recognized */ \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

#define IN1_PORT \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */ \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

#define IN2_PORT \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

#define	COINAGE_DSW \
	/* TODO: support the different settings which happen in Coin Mode 2 */ \
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */ \
	PORT_DIPSETTING(    0x90, "7 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" ) \
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" ) \
	PORT_DIPSETTING(    0x20, "1 Coin/7 Credits" ) \
	PORT_DIPSETTING(    0x10, "1 Coin/8 Credits" ) \
	PORT_DIPSETTING(    0x00, "Free Play" ) \
	/* setting 0x80 give 1 Coin/1 Credit */


#define	COINAGE2_DSW \
	/* TODO: support the different settings which happen in Coin Mode 2 */ \
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */ \
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x10, "8 Coins/3 Credits" ) \
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x20, "5 Coins/3 Credits" ) \
	PORT_DIPSETTING(    0x30, "3 Coins/2 Credits" ) \
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(    0x40, "2 Coins/3 Credits" ) \
	PORT_DIPSETTING(    0x90, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(    0x70, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(    0x60, "1 Coin/5 Credits" ) \
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" ) \
	PORT_DIPSETTING(    0x00, "Free Play" ) \


INPUT_PORTS_START( ldrun_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Slow" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	COINAGE_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( ldrun2_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x02, 0x02, "Game Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	COINAGE_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( ldrun3_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x02, 0x02, "Game Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	COINAGE_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( ldrun4_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x02, 0x02, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "1 Player Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	COINAGE_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "2 Players Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "6" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow 2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode (must set 2P game to No)", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( lotlot_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Very Slow" )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	COINAGE2_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( kidniki_input_ports )
	PORT_START
	IN0_PORT

	PORT_START
	IN1_PORT

	PORT_START
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x00, "80000" )
	COINAGE2_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Game Repeats", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( spelunk2_input_ports )
	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* IN1 */
	IN1_PORT

	PORT_START	/* IN2 */
	IN2_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Energy Decrese", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Slow" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	COINAGE2_DSW

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x08, "Yes" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



#define TILELAYOUT(NUM) static struct GfxLayout tilelayout_##NUM =  \
{                                                                   \
	8,8,	/* 8*8 characters */                                    \
	NUM,	/* NUM characters */                                    \
	3,	/* 3 bits per pixel */                                      \
	{ 2*NUM*8*8, NUM*8*8, 0 },                                      \
	{ 0, 1, 2, 3, 4, 5, 6, 7 },                                     \
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },                     \
	8*8	/* every char takes 8 consecutive bytes */                  \
}

TILELAYOUT(1024);
TILELAYOUT(2048);
TILELAYOUT(4096);


static struct GfxLayout lotlot_charlayout =
{
	12,10, /* character size */
	256, /* number of characters */
	3, /* bits per pixel */
	{ 0, 256*32*8, 2*256*32*8 },
	{ 0, 1, 2, 3, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout kidniki_charlayout =
{
	12,8, /* character size */
	1024, /* number of characters */
	3, /* bits per pixel */
	{ 0, 0x4000*8, 2*0x4000*8 },
	{ 0, 1, 2, 3, 64+0,64+1,64+2,64+3,64+4,64+5,64+6,64+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spelunk2_charlayout =
{
	12,8, /* character size */
	256, /* number of characters */
	3, /* bits per pixel */
	{ 0, 0x2000*8, 2*0x2000*8 },
	{
		0,1,2,3,
		0x800*8+0,0x800*8+1,0x800*8+2,0x800*8+3,
		0x800*8+4,0x800*8+5,0x800*8+6,0x800*8+7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

#define SPRITELAYOUT(NUM) static struct GfxLayout spritelayout_##NUM =         \
{                                                                              \
	16,16,	/* 16*16 sprites */                                                \
	NUM,	/* NUM sprites */                                                  \
	3,	/* 3 bits per pixel */                                                 \
	{ 2*NUM*32*8, NUM*32*8, 0 },                                               \
	{ 0, 1, 2, 3, 4, 5, 6, 7,                                                  \
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },  \
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,                                  \
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },                    \
	32*8	/* every sprite takes 32 consecutive bytes */                      \
}

SPRITELAYOUT(256);
SPRITELAYOUT(512);
SPRITELAYOUT(1024);
SPRITELAYOUT(2048);


static struct GfxDecodeInfo ldrun_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_1024,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout_256,   256, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ldrun2_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_1024,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout_512,   256, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ldrun3_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_2048,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x0c000, &spritelayout_512,   256, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ldrun4_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_2048,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x0c000, &spritelayout_1024,  256, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo lotlot_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &lotlot_charlayout,    0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout_256,   256, 32 },	/* use colors 256-511 */
	{ 1, 0x0c000, &lotlot_charlayout,  512, 32 },	/* use colors 512-767 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo kidniki_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_4096,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x18000, &spritelayout_2048,  256, 32 },	/* use colors 256-511 */
	{ 1, 0x4c000, &kidniki_charlayout,   0, 32 },	/* use colors   0-255 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo spelunk2_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout_4096,	     0, 64 },	/* use colors   0-511 */
	{ 1, 0x18000, &spritelayout_1024,  512, 32 },	/* use colors 512-767 */
	{ 1, 0x30000, &spelunk2_charlayout,  0, 64 },	/* use colors   0-511 */
	{ 1, 0x31000, &spelunk2_charlayout,  0, 64 },	/* use colors   0-511 */
	{ -1 } /* end of array */
};



#define MACHINE_DRIVER(GAMENAME,READPORT,COLORS,CONVERTCOLOR)                                \
                                                                                             \
static struct MachineDriver GAMENAME##_machine_driver =                                      \
{                                                                                            \
	/* basic machine hardware */                                                             \
	{                                                                                        \
		{                                                                                    \
			CPU_Z80,                                                                         \
			4000000,	/* 4 Mhz (?) */                                                      \
			0,                                                                               \
			GAMENAME##_readmem,GAMENAME##_writemem,READPORT##_readport,GAMENAME##_writeport, \
			interrupt,1                                                                      \
		},                                                                                   \
		{                                                                                    \
			CPU_M6803 | CPU_AUDIO_CPU,                                                       \
			1000000,	/* 1.0 Mhz ? */                                                      \
			3,                                                                               \
			irem_sound_readmem,irem_sound_writemem,0,0,                                      \
			ignore_interrupt,0	/* interrupts are generated by the ADPCM hardware */         \
		}                                                                                    \
	},                                                                                       \
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */        \
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	0,                                                                                       \
                                                                                             \
	/* video hardware */                                                                     \
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },                                            \
	GAMENAME##_gfxdecodeinfo,                                                                \
	COLORS, COLORS,                                                                          \
	CONVERTCOLOR##_vh_convert_color_prom,                                                    \
                                                                                             \
	VIDEO_TYPE_RASTER,                                                                       \
	0,                                                                                       \
	GAMENAME##_vh_start,                                                                     \
	generic_vh_stop,                                                                         \
	GAMENAME##_vh_screenrefresh,                                                             \
                                                                                             \
	/* sound hardware */                                                                     \
	0,0,0,0,                                                                                 \
	{                                                                                        \
		{                                                                                    \
			SOUND_AY8910,                                                                    \
			&irem_ay8910_interface                                                           \
		},                                                                                   \
		{                                                                                    \
			SOUND_MSM5205,                                                                   \
			&irem_msm5205_interface                                                          \
		}                                                                                    \
	}                                                                                        \
}


#define lotlot_writeport ldrun_writeport
#define	ldrun2_vh_start ldrun_vh_start
#define	ldrun3_vh_start ldrun_vh_start
#define	ldrun4_vh_start ldrun_vh_start
#define	lotlot_vh_start ldrun_vh_start
#define	ldrun2_vh_screenrefresh ldrun_vh_screenrefresh
#define	ldrun3_vh_screenrefresh ldrun_vh_screenrefresh

MACHINE_DRIVER(ldrun,   ldrun, 512,irem);
MACHINE_DRIVER(ldrun2,  ldrun2,512,irem);
MACHINE_DRIVER(ldrun3,  ldrun, 512,irem);
MACHINE_DRIVER(ldrun4,  ldrun, 512,irem);
MACHINE_DRIVER(lotlot,  ldrun, 768,irem);
MACHINE_DRIVER(kidniki, ldrun, 512,irem);
MACHINE_DRIVER(spelunk2,ldrun, 768,spelunk2);



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ldrun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lr-a-4e",      0x0000, 0x2000, 0x5d7e2a4d )
	ROM_LOAD( "lr-a-4d",      0x2000, 0x2000, 0x96f20473 )
	ROM_LOAD( "lr-a-4b",      0x4000, 0x2000, 0xb041c4a9 )
	ROM_LOAD( "lr-a-4a",      0x6000, 0x2000, 0x645e42aa )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d",      0x0000, 0x2000, 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j",      0x2000, 0x2000, 0x43175e08 )
	ROM_LOAD( "lr-e-2f",      0x4000, 0x2000, 0xe0317124 )
	ROM_LOAD( "lr-b-4k",      0x6000, 0x2000, 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n",      0x8000, 0x2000, 0x55154154 )
	ROM_LOAD( "lr-b-4c",      0xa000, 0x2000, 0x924e34d0 )

	ROM_REGION(0x0720)	/* color proms */
	ROM_LOAD( "lr-e-3m",      0x0000, 0x0100, 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m",      0x0100, 0x0100, 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l",      0x0200, 0x0100, 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n",      0x0300, 0x0100, 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n",      0x0400, 0x0100, 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l",      0x0500, 0x0100, 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p",      0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lr-b-6f",      0x0620, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f",      0xc000, 0x2000, 0x7a96accd )
	ROM_LOAD( "lr-a-3h",      0xe000, 0x2000, 0x3f7f3939 )
ROM_END

ROM_START( ldruna_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "roma4c",       0x0000, 0x2000, 0x279421e1 )
	ROM_LOAD( "lr-a-4d",      0x2000, 0x2000, 0x96f20473 )
	ROM_LOAD( "roma4b",       0x4000, 0x2000, 0x3c464bad )
	ROM_LOAD( "roma4a",       0x6000, 0x2000, 0x899df8e0 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d",      0x0000, 0x2000, 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j",      0x2000, 0x2000, 0x43175e08 )
	ROM_LOAD( "lr-e-2f",      0x4000, 0x2000, 0xe0317124 )
	ROM_LOAD( "lr-b-4k",      0x6000, 0x2000, 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n",      0x8000, 0x2000, 0x55154154 )
	ROM_LOAD( "lr-b-4c",      0xa000, 0x2000, 0x924e34d0 )

	ROM_REGION(0x0720)	/* color proms */
	ROM_LOAD( "lr-e-3m",      0x0000, 0x0100, 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m",      0x0100, 0x0100, 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l",      0x0200, 0x0100, 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n",      0x0300, 0x0100, 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n",      0x0400, 0x0100, 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l",      0x0500, 0x0100, 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p",      0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lr-b-6f",      0x0620, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f",      0xc000, 0x2000, 0x7a96accd )
	ROM_LOAD( "lr-a-3h",      0xe000, 0x2000, 0x3f7f3939 )
ROM_END

ROM_START( ldrun2_rom )
	ROM_REGION(0x14000)	/* 64k for code + 16k for banks */
	ROM_LOAD( "lr2-a-4e.a",   0x00000, 0x2000, 0x22313327 )
	ROM_LOAD( "lr2-a-4d",     0x02000, 0x2000, 0xef645179 )
	ROM_LOAD( "lr2-a-4a.a",   0x04000, 0x2000, 0xb11ddf59 )
	ROM_LOAD( "lr2-a-4a",     0x06000, 0x2000, 0x470cc8a1 )
	ROM_LOAD( "lr2-h-1c.a",   0x10000, 0x2000, 0x7ebcadbc )	/* banked at 8000-9fff */
	ROM_LOAD( "lr2-h-1d.a",   0x12000, 0x2000, 0x64cbb7f9 )	/* banked at 8000-9fff */

	ROM_REGION_DISPOSE(0x12000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr2-h-1e",     0x00000, 0x2000, 0x9d63a8ff )	/* characters */
	ROM_LOAD( "lr2-h-1j",     0x02000, 0x2000, 0x40332bbd )
	ROM_LOAD( "lr2-h-1h",     0x04000, 0x2000, 0x9404727d )
	ROM_LOAD( "lr2-b-4k",     0x06000, 0x2000, 0x79909871 )	/* sprites */
	ROM_LOAD( "lr2-b-4e",     0x08000, 0x2000, 0x75172d1f )
	ROM_LOAD( "lr2-b-3n",     0x0a000, 0x2000, 0x3cc5893f )
	ROM_LOAD( "lr2-b-4f",     0x0c000, 0x2000, 0x06ba1ef4 )
	ROM_LOAD( "lr2-b-4c",     0x0e000, 0x2000, 0xfbe6d24c )
	ROM_LOAD( "lr2-b-4n",     0x10000, 0x2000, 0x49c12f42 )

	ROM_REGION(0x0720)	/* color proms */
	ROM_LOAD( "lr2-h-3m",     0x0000, 0x0100, 0x2c5d834b )	/* character palette red component */
	ROM_LOAD( "lr2-b-1m",     0x0100, 0x0100, 0x4ec9bb3d )	/* sprite palette red component */
	ROM_LOAD( "lr2-h-3l",     0x0200, 0x0100, 0x3ae69aca )	/* character palette green component */
	ROM_LOAD( "lr2-b-1n",     0x0300, 0x0100, 0x1daf1fa4 )	/* sprite palette green component */
	ROM_LOAD( "lr2-h-3n",     0x0400, 0x0100, 0x2b28aec5 )	/* character palette blue component */
	ROM_LOAD( "lr2-b-1l",     0x0500, 0x0100, 0xc8fb708a )	/* sprite palette blue component */
	ROM_LOAD( "lr2-b-5p",     0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lr2-b-6f",     0x0620, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr2-a-3e",     0xa000, 0x2000, 0x853f3898 )
	ROM_LOAD( "lr2-a-3f",     0xc000, 0x2000, 0x7a96accd )
	ROM_LOAD( "lr2-a-3h",     0xe000, 0x2000, 0x2a0e83ca )
ROM_END

ROM_START( ldrun3_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lr3-a-4e",     0x0000, 0x4000, 0x5b334e8e )
	ROM_LOAD( "lr3-a-4d.a",   0x4000, 0x4000, 0xa84bc931 )
	ROM_LOAD( "lr3-a-4b.a",   0x8000, 0x4000, 0xbe09031d )

	ROM_REGION_DISPOSE(0x18000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr3-n-2a",     0x00000, 0x4000, 0xf9b74dee )	/* characters */
	ROM_LOAD( "lr3-n-2c",     0x04000, 0x4000, 0xfef707ba )
	ROM_LOAD( "lr3-n-2b",     0x08000, 0x4000, 0xaf3d27b9 )
	ROM_LOAD( "lr3-b-4k",     0x0c000, 0x4000, 0x63f070c7 )	/* sprites */
	ROM_LOAD( "lr3-b-3n",     0x10000, 0x4000, 0xeab7ad91 )
	ROM_LOAD( "lr3-b-4c",     0x14000, 0x4000, 0x1a460a46 )

	ROM_REGION(0x0820)	/* color proms */
	ROM_LOAD( "lr3-n-2l",     0x0000, 0x0100, 0xe880b86b ) /* character palette red component */
	ROM_LOAD( "lr3-b-1m",     0x0100, 0x0100, 0xf02d7167 ) /* sprite palette red component */
	ROM_LOAD( "lr3-n-2k",     0x0200, 0x0100, 0x047ee051 ) /* character palette green component */
	ROM_LOAD( "lr3-b-1n",     0x0300, 0x0100, 0x9e37f181 ) /* sprite palette green component */
	ROM_LOAD( "lr3-n-2m",     0x0400, 0x0100, 0x69ad8678 ) /* character palette blue component */
	ROM_LOAD( "lr3-b-1l",     0x0500, 0x0100, 0x5b11c41d ) /* sprite palette blue component */
	ROM_LOAD( "lr3-b-5p",     0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lr3-n-4f",     0x0620, 0x0100, 0xdf674be9 )	/* unknown */
	ROM_LOAD( "lr3-b-6f",     0x0720, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr3-a-3d",     0x8000, 0x4000, 0x28be68cd )
	ROM_LOAD( "lr3-a-3f",     0xc000, 0x4000, 0xcb7186b7 )
ROM_END

ROM_START( ldrun4_rom )
	ROM_REGION(0x18000)	/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "lr4-a-4e",     0x00000, 0x4000, 0x5383e9bf )
	ROM_LOAD( "lr4-a-4d.c",   0x04000, 0x4000, 0x298afa36 )
	ROM_LOAD( "lr4-v-4k",     0x10000, 0x8000, 0x8b248abd )	/* banked at 8000-bfff */

	ROM_REGION_DISPOSE(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr4-v-2b",     0x00000, 0x4000, 0x4118e60a )	/* characters */
	ROM_LOAD( "lr4-v-2d",     0x04000, 0x4000, 0x542bb5b5 )
	ROM_LOAD( "lr4-v-2c",     0x08000, 0x4000, 0xc765266c )
	ROM_LOAD( "lr4-b-4k",     0x0c000, 0x4000, 0xe7fe620c )	/* sprites */
	ROM_LOAD( "lr4-b-4f",     0x10000, 0x4000, 0x6f0403db )
	ROM_LOAD( "lr4-b-3n",     0x14000, 0x4000, 0xad1fba1b )
	ROM_LOAD( "lr4-b-4n",     0x18000, 0x4000, 0x0e568fab )
	ROM_LOAD( "lr4-b-4c",     0x1c000, 0x4000, 0x82c53669 )
	ROM_LOAD( "lr4-b-4e",     0x20000, 0x4000, 0x767a1352 )

	ROM_REGION(0x0820)	/* color proms */
	ROM_LOAD( "lr4-v-1m",     0x0000, 0x0100, 0xfe51bf1d ) /* character palette red component */
	ROM_LOAD( "lr4-b-1m",     0x0100, 0x0100, 0x5d8d17d0 ) /* sprite palette red component */
	ROM_LOAD( "lr4-v-1n",     0x0200, 0x0100, 0xda0658e5 ) /* character palette green component */
	ROM_LOAD( "lr4-b-1n",     0x0300, 0x0100, 0xda1129d2 ) /* sprite palette green component */
	ROM_LOAD( "lr4-v-1p",     0x0400, 0x0100, 0x0df23ebe ) /* character palette blue component */
	ROM_LOAD( "lr4-b-1l",     0x0500, 0x0100, 0x0d89b692 ) /* sprite palette blue component */
	ROM_LOAD( "lr4-b-5p",     0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lr4-v-4h",     0x0620, 0x0100, 0xdf674be9 )	/* unknown */
	ROM_LOAD( "lr4-b-6f",     0x0720, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr4-a-3d",     0x8000, 0x4000, 0x86c6d445 )
	ROM_LOAD( "lr4-a-3f",     0xc000, 0x4000, 0x097c6c0a )
ROM_END

ROM_START( lotlot_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lot-a-4e",     0x0000, 0x4000, 0x2913d08f )
	ROM_LOAD( "lot-a-4d",     0x4000, 0x4000, 0x0443095f )

	ROM_REGION_DISPOSE(0x12000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lot-k-4a",     0x00000, 0x2000, 0x1b3695f4 )	/* tiles */
	ROM_LOAD( "lot-k-4c",     0x02000, 0x2000, 0xbd2b0730 )
	ROM_LOAD( "lot-k-4b",     0x04000, 0x2000, 0x930ddd55 )
	ROM_LOAD( "lot-b-4k",     0x06000, 0x2000, 0xfd27cb90 )	/* sprites */
	ROM_LOAD( "lot-b-3n",     0x08000, 0x2000, 0xbd486fff )
	ROM_LOAD( "lot-b-4c",     0x0a000, 0x2000, 0x3026ee6c )
	ROM_LOAD( "lot-k-4p",     0x0c000, 0x2000, 0x3b7d95ba )	/* chars */
	ROM_LOAD( "lot-k-4l",     0x0e000, 0x2000, 0xf98dca1f )
	ROM_LOAD( "lot-k-4n",     0x10000, 0x2000, 0xf0cd76a5 )

	ROM_REGION(0x0e20)	/* color proms */
	ROM_LOAD( "lot-k-2f",     0x0000, 0x0100, 0xb820a05e ) /* tile palette red component */
	ROM_LOAD( "lot-b-1m",     0x0100, 0x0100, 0xc146461d ) /* sprite palette red component */
	ROM_LOAD( "lot-k-2l",     0x0200, 0x0100, 0xac3e230d ) /* character palette red component */
	ROM_LOAD( "lot-k-2e",     0x0300, 0x0100, 0x9b1fa005 ) /* tile palette green component */
	ROM_LOAD( "lot-b-1n",     0x0400, 0x0100, 0x01e07db6 ) /* sprite palette green component */
	ROM_LOAD( "lot-k-2k",     0x0500, 0x0100, 0x1811ad2b ) /* character palette green component */
	ROM_LOAD( "lot-k-2d",     0x0600, 0x0100, 0x315ed9a8 ) /* tile palette blue component */
	ROM_LOAD( "lot-b-1l",     0x0700, 0x0100, 0x8b6fcde3 ) /* sprite palette blue component */
	ROM_LOAD( "lot-k-2j",     0x0800, 0x0100, 0xe791ef2a ) /* character palette blue component */
	ROM_LOAD( "lot-b-5p",     0x0900, 0x0020, 0x110b21fd )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "lot-k-7e",     0x0920, 0x0200, 0x6cef0fbd )	/* unknown */
	ROM_LOAD( "lot-k-7h",     0x0b20, 0x0200, 0x04442bee )	/* unknown */
	ROM_LOAD( "lot-b-6f",     0x0d20, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lot-a-3h",     0xe000, 0x2000, 0x0781cee7 )
ROM_END

ROM_START( kidniki_rom )
	ROM_REGION( 0x30000 )	/* main CPU */
	ROM_LOAD( "dr04.4e",      0x00000, 0x04000, 0x80431858 )
	ROM_LOAD( "dr03.4cd",     0x04000, 0x04000, 0xdba20934 )
	ROM_LOAD( "dr11.8k",      0x10000, 0x08000, 0x04d82d93 )	/* banked at 8000-9fff */
	ROM_LOAD( "dr12.8l",      0x18000, 0x08000, 0xc0b255fd )
	ROM_CONTINUE(             0x28000, 0x08000 )

	ROM_REGION_DISPOSE(0x58000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dr06.2b",      0x00000, 0x8000, 0x4d9a970f )	/* tiles */
	ROM_LOAD( "dr07.2dc",     0x08000, 0x8000, 0xab59a4c4 )
	ROM_LOAD( "dr05.2a",      0x10000, 0x8000, 0x2e6dad0c )
	ROM_LOAD( "dr21.4k",      0x18000, 0x4000, 0xa06cea9a )	/* sprites */
	ROM_LOAD( "dr19.4f",      0x1c000, 0x4000, 0xb34605ad )
	ROM_LOAD( "dr22.4l",      0x20000, 0x4000, 0x41303de8 )
	ROM_LOAD( "dr20.4jh",     0x24000, 0x4000, 0x5fbe6f61 )
	ROM_LOAD( "dr14.3p",      0x28000, 0x4000, 0x76cfbcbc )
	ROM_LOAD( "dr24.4p",      0x2c000, 0x4000, 0xd51c8db5 )
	ROM_LOAD( "dr23.4nm",     0x30000, 0x4000, 0x03469df8 )
	ROM_LOAD( "dr13.3nm",     0x34000, 0x4000, 0xd5c3dfe0 )
	ROM_LOAD( "dr16.4cb",     0x38000, 0x4000, 0xf1d1bb93 )
	ROM_LOAD( "dr18.4e",      0x3c000, 0x4000, 0xedb7f25b )
	ROM_LOAD( "dr17.4dc",     0x40000, 0x4000, 0x4fb87868 )
	ROM_LOAD( "dr15.4a",      0x48000, 0x4000, 0xe0b88de5 )
	ROM_LOAD( "dr08.4l",      0x4c000, 0x4000, 0x32d50643 )	/* chars */
	ROM_LOAD( "dr09.4m",      0x50000, 0x4000, 0x17df6f95 )
	ROM_LOAD( "dr10.4n",      0x54000, 0x4000, 0x820ce252 )

	ROM_REGION( 0x0920 ) /* color proms */
	ROM_LOAD( "dr25.3f",      0x0000, 0x0100, 0x8e91430b )	/* character palette red component */
	ROM_LOAD( "dr30.1m",      0x0100, 0x0100, 0x28c73263 )	/* sprite palette red component */
	ROM_LOAD( "dr26.3h",      0x0200, 0x0100, 0xb563b93f )	/* character palette green component */
	ROM_LOAD( "dr31.1n",      0x0300, 0x0100, 0x3529210e )	/* sprite palette green component */
	ROM_LOAD( "dr27.3j",      0x0400, 0x0100, 0x70d668ef )	/* character palette blue component */
	ROM_LOAD( "dr29.1l",      0x0500, 0x0100, 0x1173a754 )	/* sprite palette blue component */
	ROM_LOAD( "dr32.5p",      0x0600, 0x0020, 0x11cd1f2e )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "dr28.8f",      0x0620, 0x0200, 0x6cef0fbd )	/* unknown */
	ROM_LOAD( "dr33.6f",      0x0820, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION( 0x10000 )	/* sound CPU */
	ROM_LOAD( "dr00.3a",      0x4000, 0x04000, 0x458309f7 )
	ROM_LOAD( "dr01.3cd",     0x8000, 0x04000, 0xe66897bd )
	ROM_LOAD( "dr02.3f",      0xc000, 0x04000, 0xf9e31e26 ) /* 6803 code */
ROM_END

ROM_START( spelunk2_rom )
	ROM_REGION( 0x24000 )	/* main CPU */
	ROM_LOAD( "sp2-a.4e",     0x0000, 0x4000, 0x96c04bbb )
	ROM_LOAD( "sp2-a.4d",     0x4000, 0x4000, 0xcb38c2ff )
	ROM_LOAD( "sp2-r.7d",     0x10000, 0x8000, 0x558837ea )	/* banked at 9000-9fff */
	ROM_LOAD( "sp2-r.7c",     0x18000, 0x8000, 0x4b380162 )	/* banked at 9000-9fff */
	ROM_LOAD( "sp2-r.7b",     0x20000, 0x4000, 0x7709a1fe )	/* banked at 8000-8fff */

	ROM_REGION_DISPOSE(0x36000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sp2-r.1d",     0x00000, 0x8000, 0xc19fa4c9 )	/* tiles */
	ROM_LOAD( "sp2-r.3b",     0x08000, 0x8000, 0x366604af )
	ROM_LOAD( "sp2-r.1b",     0x10000, 0x8000, 0x3a0c4d47 )
	ROM_LOAD( "sp2-b.4k",     0x18000, 0x4000, 0x6cb67a17 )	/* sprites */
	ROM_LOAD( "sp2-b.4f",     0x1c000, 0x4000, 0xe4a1166f )
	ROM_LOAD( "sp2-b.3n",     0x20000, 0x4000, 0xf59e8b76 )
	ROM_LOAD( "sp2-b.4n",     0x24000, 0x4000, 0xfa65bac9 )
	ROM_LOAD( "sp2-b.4c",     0x28000, 0x4000, 0x1caf7013 )
	ROM_LOAD( "sp2-b.4e",     0x2c000, 0x4000, 0x780a463b )
	ROM_LOAD( "sp2-r.4l",     0x30000, 0x2000, 0xb12f939a )	/* chars */
	ROM_LOAD( "sp2-r.4m",     0x32000, 0x2000, 0x5591f22c )
	ROM_LOAD( "sp2-r.4p",     0x34000, 0x2000, 0x32967081 )

	ROM_REGION( 0x0a20 ) /* color proms */
	ROM_LOAD( "sp2-r.1k",     0x0000, 0x0200, 0x31c1bcdc )	/* chars red and green component */
	ROM_LOAD( "sp2-r.2k",     0x0200, 0x0100, 0x1cf5987e )	/* chars blue component */
	ROM_LOAD( "sp2-r.2j",     0x0300, 0x0100, 0x1acbe2a5 )	/* chars blue component */
	ROM_LOAD( "sp2-b.1m",     0x0400, 0x0100, 0x906104c7 )	/* sprites red component */
	ROM_LOAD( "sp2-b.1n",     0x0500, 0x0100, 0x5a564c06 )	/* sprites green component */
	ROM_LOAD( "sp2-b.1l",     0x0600, 0x0100, 0x8f4a2e3c )	/* sprites blue component */
	ROM_LOAD( "sp2-b.5p",     0x0700, 0x0020, 0xcd126f6a )	/* sprite height, one entry per 32 */
	                                                        /* sprites. Used at run time! */
	ROM_LOAD( "sp2-r.8j",     0x0720, 0x0200, 0x875cc442 )	/* unknown */
	ROM_LOAD( "sp2-b.6f",     0x0920, 0x0100, 0x34d88d3c )	/* unknown - common to the other games */

	ROM_REGION( 0x10000 )	/* sound CPU */
	ROM_LOAD( "sp2-a.3d",     0x8000, 0x04000, 0x839ec7e2 ) /* adpcm data */
	ROM_LOAD( "sp2-a.3f",     0xc000, 0x04000, 0xad3ce898 ) /* 6803 code */
ROM_END



static int ldrun_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE5E5],"\x01\x01\x00",3) == 0 &&
			memcmp(&RAM[0xE6AA],"\x20\x20\x04",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE5E5],200);
			osd_fclose(f);
			memcpy (&RAM[0xe59a],&RAM[0xe6a3],3);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void ldrun_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE5E5],200);
		osd_fclose(f);
	}
}

static int ldrun2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE6bd],"\x00\x48\x54",3) == 0 &&
		memcmp(&RAM[0xE782],"\x20\x20\x05",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
		osd_fread(f,&RAM[0xE6bd],200);
		osd_fclose(f);
		memcpy (&RAM[0xe672],&RAM[0xe77b],3);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void ldrun2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{


		osd_fwrite(f,&RAM[0xE6bd],200);
		osd_fclose(f);
	}
}




static int ldrun4_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE735],"\x00\x28\x76",3) == 0 &&
			memcmp(&RAM[0xE7FA],"\x20\x20\x06",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE735],200);
			osd_fclose(f);
			memcpy (&RAM[0xe6ea],&RAM[0xe7f3],3);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void ldrun4_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE735],200);
		osd_fclose(f);
	}
}


static int kidniki_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE062],"\x00\x10\x00",3) == 0 &&
			memcmp(&RAM[0xE0CE],"\x20\x20\x00",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE062],111);
			osd_fclose(f);
			memcpy (&RAM[0xE02b],&RAM[0xE062],3);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void kidniki_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE062],111);
		osd_fclose(f);
	}
}


static int spelunk2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE066],"\x99\x11\x59",3) == 0 &&
			memcmp(&RAM[0xE0C7],"\x49\x4e\x3f",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE066],100);
			osd_fclose(f);
			memcpy(&RAM[0xE04f],&RAM[0xe066],2);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void spelunk2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE066],100);
		osd_fclose(f);
	}
}

static int lotlot_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xE96a],"\x00\x02\x51",3) == 0 &&
			memcmp(&RAM[0xEb8d],"\x49\x20\x20",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE96b],0x226);
			osd_fclose(f);
			memcpy(&RAM[0xe956],&RAM[0xE96b],3);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void lotlot_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE96b],0x226);
		osd_fclose(f);
	}
}



struct GameDriver ldrun_driver =
{
	__FILE__,
	0,
	"ldrun",
	"Lode Runner (set 1)",
	"1984",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)",
	0,
	&ldrun_machine_driver,
	0,

	ldrun_rom,
	0, 0,
	0,
	0,

	ldrun_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun_hiload, ldrun_hisave
};

struct GameDriver ldruna_driver =
{
	__FILE__,
	&ldrun_driver,
	"ldruna",
	"Lode Runner (set 2)",
	"1984",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)",
	0,
	&ldrun_machine_driver,
	0,

	ldruna_rom,
	0, 0,
	0,
	0,

	ldrun_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun_hiload, ldrun_hisave
};

struct GameDriver ldrun2_driver =
{
	__FILE__,
	0,
	"ldrun2",
	"Lode Runner II - The Bungeling Strikes Back",
	/* Japanese version is called Bangeringu Teikoku No Gyakushuu */
	"1984",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&ldrun2_machine_driver,
	0,

	ldrun2_rom,
	0, 0,
	0,
	0,

	ldrun2_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun2_hiload, ldrun2_hisave
};

struct GameDriver ldrun3_driver =
{
	__FILE__,
	0,
	"ldrun3",
	"Lode Runner III - Majin No Fukkatsu",
	"1985",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&ldrun3_machine_driver,
	0,

	ldrun3_rom,
	0, 0,
	0,
	0,

	ldrun3_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun2_hiload, ldrun2_hisave
};

struct GameDriver ldrun4_driver =
{
	__FILE__,
	0,
	"ldrun4",
	"Lode Runner IV - Teikoku Karano Dasshutsu",
	"1986",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&ldrun4_machine_driver,
	0,

	ldrun4_rom,
	0, 0,
	0,
	0,

	ldrun4_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun4_hiload, ldrun4_hisave
};

struct GameDriver lotlot_driver =
{
	__FILE__,
	0,
	"lotlot",
	"Lot Lot",
	"1985",
	"Irem (licensed from Tokuma Shoten)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&lotlot_machine_driver,
	0,

	lotlot_rom,
	0, 0,
	0,
	0,

	lotlot_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	lotlot_hiload, lotlot_hisave
};

struct GameDriver kidniki_driver =
{
	__FILE__,
	0,
	"kidniki",
	"Kid Niki, Radical Ninja",
	"1986",
	"Irem (Data East USA license)",
	"Phil Stroffolino\nAaron Giles (sound)",
	0,
	&kidniki_machine_driver,
	0,

	kidniki_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kidniki_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	kidniki_hiload, kidniki_hisave
};

struct GameDriver spelunk2_driver =
{
	__FILE__,
	0,
	"spelunk2",
	"Spelunker II",
	"1986",
	"Irem (licensed from Broderbund)",
	"Phil Stroffolino\nAaron Giles (sound)",
	0,
	&spelunk2_machine_driver,
	0,

	spelunk2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	spelunk2_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	spelunk2_hiload, spelunk2_hisave
};
