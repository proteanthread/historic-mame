/****************************************************************************

Lode Runner

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


void ldrun_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int ldrun_vh_start(void);
void ldrun_vh_stop(void);
void ldrun2p_scroll_w(int offset,int data);
void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void ldrun2p_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void mpatrol_io_w(int offset, int value);
int mpatrol_io_r(int offset);
void mpatrol_adpcm_reset_w(int offset,int value);
void mpatrol_sound_cmd_w(int offset, int value);

void mpatrol_adpcm_int(int data);


void ldrun2p_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ((data & 0x01) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, mpatrol_sound_cmd_w },
//	{ 0x01, 0x01, kungfum_flipscreen_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ldrun2p_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun2p_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xc800, ldrun2p_bankswitch_w },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun2p_writeport[] =
{
	{ 0x00, 0x00, mpatrol_sound_cmd_w },
//	{ 0x01, 0x01, kungfum_flipscreen_w },	/* coin counter? */
	{ 0x82, 0x83, ldrun2p_scroll_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0801, 0x0802, MSM5205_data_w },
	{ 0x9000, 0x9000, MWA_NOP },    /* IACK */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
/* Start 1 & 2 also restarts and freezes the game with stop mode on
   and are used in test mode to enter and esc the various tests */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
		"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

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
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */

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

INPUT_PORTS_START( ldrun2p_input_ports )
	PORT_START	/* IN0 */
/* Start 1 & 2 also restarts and freezes the game with stop mode on
   and are used in test mode to enter and esc the various tests */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
		"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

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
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */

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



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*32*8, 256*32*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout ldrun2p_charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 2*2048*8*8, 2048*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout ldrun2p_spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout, 32*8, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ldrun2p_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &ldrun2p_charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x0c000, &ldrun2p_spritelayout, 32*8, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	910000,	/* .91 MHZ ?? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0, mpatrol_adpcm_reset_w }
};

static struct MSM5205interface msm5205_interface =
{
	2,			/* 2 chips */
	4000,       /* 4000Hz playback */
	mpatrol_adpcm_int,/* interrupt function */
	{ 255, 255 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	512, 512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	ldrun_vh_start,
	ldrun_vh_stop,
	ldrun_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};

static struct MachineDriver ldrun2p_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			ldrun2p_readmem,ldrun2p_writemem,readport,ldrun2p_writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	ldrun2p_gfxdecodeinfo,
	512, 512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	ldrun_vh_start,
	ldrun_vh_stop,
	ldrun2p_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ldrun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lr-a-4e", 0x0000, 0x2000, 0x164ba7a5 , 0x5d7e2a4d )
	ROM_LOAD( "lr-a-4d", 0x2000, 0x2000, 0x6dfb93ed , 0x96f20473 )
	ROM_LOAD( "lr-a-4b", 0x4000, 0x2000, 0x85c12e55 , 0xb041c4a9 )
	ROM_LOAD( "lr-a-4a", 0x6000, 0x2000, 0xdf829188 , 0x645e42aa )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d", 0x0000, 0x2000, 0x48d59ac3 , 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j", 0x2000, 0x2000, 0x30f168c5 , 0x43175e08 )
	ROM_LOAD( "lr-e-2f", 0x4000, 0x2000, 0x45820080 , 0xe0317124 )
	ROM_LOAD( "lr-b-4k", 0x6000, 0x2000, 0xd0fb93b3 , 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n", 0x8000, 0x2000, 0xd2282fc6 , 0x55154154 )
	ROM_LOAD( "lr-b-4c", 0xa000, 0x2000, 0x0655cce1 , 0x924e34d0 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr-e-3m", 0x0000, 0x0100, 0x9a120c08 , 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m", 0x0100, 0x0100, 0x44d20000 , 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l", 0x0200, 0x0100, 0x94b6080e , 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n", 0x0300, 0x0100, 0x50780000 , 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n", 0x0400, 0x0100, 0x6af0070e , 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l", 0x0500, 0x0100, 0x73100000 , 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p", 0x0600, 0x0020, 0x08080000 , 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f", 0xc000, 0x2000, 0xf5158e6d , 0x7a96accd )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "lr-a-3h", 0xe000, 0x2000, 0xc3f198a5 , 0x3f7f3939 )
ROM_END

ROM_START( ldruna_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "roma4c", 0x0000, 0x2000, 0x0246614e , 0x279421e1 )
	ROM_LOAD( "lr-a-4d", 0x2000, 0x2000, 0x6dfb93ed , 0x96f20473 )
	ROM_LOAD( "roma4b", 0x4000, 0x2000, 0x6c27aa39 , 0x3c464bad )
	ROM_LOAD( "roma4a", 0x6000, 0x2000, 0x81c14dd9 , 0x899df8e0 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d", 0x0000, 0x2000, 0x48d59ac3 , 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j", 0x2000, 0x2000, 0x30f168c5 , 0x43175e08 )
	ROM_LOAD( "lr-e-2f", 0x4000, 0x2000, 0x45820080 , 0xe0317124 )
	ROM_LOAD( "lr-b-4k", 0x6000, 0x2000, 0xd0fb93b3 , 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n", 0x8000, 0x2000, 0xd2282fc6 , 0x55154154 )
	ROM_LOAD( "lr-b-4c", 0xa000, 0x2000, 0x0655cce1 , 0x924e34d0 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr-e-3m", 0x0000, 0x0100, 0x9a120c08 , 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m", 0x0100, 0x0100, 0x44d20000 , 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l", 0x0200, 0x0100, 0x94b6080e , 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n", 0x0300, 0x0100, 0x50780000 , 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n", 0x0400, 0x0100, 0x6af0070e , 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l", 0x0500, 0x0100, 0x73100000 , 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p", 0x0600, 0x0020, 0x08080000 , 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f", 0xc000, 0x2000, 0xf5158e6d , 0x7a96accd )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "lr-a-3h", 0xe000, 0x2000, 0xc3f198a5 , 0x3f7f3939 )
ROM_END

ROM_START( ldrun2p_rom )
	ROM_REGION(0x18000)	/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "lr4-a-4e", 0x00000, 0x4000, 0xefafcabb , 0x5383e9bf )
	ROM_LOAD( "lr4-a-4d.c", 0x04000, 0x4000, 0x148fe185 , 0x298afa36 )
	ROM_LOAD( "lr4-v-4k", 0x10000, 0x8000, 0xeae67c8c , 0x8b248abd )	/* banked at 8000-bfff */

	ROM_REGION_DISPOSE(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr4-v-2b", 0x00000, 0x4000, 0x65de6004 , 0x4118e60a )	/* characters */
	ROM_LOAD( "lr4-v-2d", 0x04000, 0x4000, 0xd5963614 , 0x542bb5b5 )
	ROM_LOAD( "lr4-v-2c", 0x08000, 0x4000, 0x5a62b042 , 0xc765266c )
	ROM_LOAD( "lr4-b-4k", 0x0c000, 0x4000, 0x7ed9aacd , 0xe7fe620c )	/* sprites */
	ROM_LOAD( "lr4-b-4f", 0x10000, 0x4000, 0x7c6a3092 , 0x6f0403db )
	ROM_LOAD( "lr4-b-3n", 0x14000, 0x4000, 0x171f1283 , 0xad1fba1b )
	ROM_LOAD( "lr4-b-4n", 0x18000, 0x4000, 0x377e20fe , 0x0e568fab )
	ROM_LOAD( "lr4-b-4c", 0x1c000, 0x4000, 0x6599496f , 0x82c53669 )
	ROM_LOAD( "lr4-b-4e", 0x20000, 0x4000, 0xc15c7fd4 , 0x767a1352 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr4-v-1m", 0x0000, 0x0100, 0xc8720b04 , 0xfe51bf1d ) /* character palette red component */
	ROM_LOAD( "lr4-b-1m", 0x0100, 0x0100, 0xe7500f04 , 0x5d8d17d0 ) /* sprite palette red component */
	ROM_LOAD( "lr4-v-1n", 0x0200, 0x0100, 0x93db0303 , 0xda0658e5 ) /* character palette green component */
	ROM_LOAD( "lr4-b-1n", 0x0300, 0x0100, 0x3b06010c , 0xda1129d2 ) /* sprite palette green component */
	ROM_LOAD( "lr4-v-1p", 0x0400, 0x0100, 0xc5c70607 , 0x0df23ebe ) /* character palette blue component */
	ROM_LOAD( "lr4-b-1l", 0x0500, 0x0100, 0xb8c00304 , 0x0d89b692 ) /* sprite palette blue component */
	ROM_LOAD( "lr4-b-5p", 0x0600, 0x0020, 0x08080000 , 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr4-a-3d", 0x8000, 0x4000, 0xe95a0aec , 0x86c6d445 )
	ROM_LOAD( "lr4-a-3f", 0xc000, 0x4000, 0xf17f9051 , 0x097c6c0a )
ROM_END



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
	&machine_driver,

	ldrun_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
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
	&machine_driver,

	ldruna_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver ldrun2p_driver =
{
	__FILE__,
	0,
	"ldrun2p",
	"Lode Runner (2 Players)",
	"1986",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&ldrun2p_machine_driver,

	ldrun2p_rom,
	0, 0,
	0,
	0,

	ldrun2p_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};