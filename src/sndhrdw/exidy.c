#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6821pia.h"

static void *timer;
#define BASE_TIME 1/.894886
#define BASE_FREQ (1789773 * 2)
#define SH6840_FREQ 894886

int exidy_sample_channels[6];
unsigned int exidy_sh8253_count[3];     /* 8253 Counter */
int exidy_sh8253_clstate[3];                    /* which byte to load */
int riot_divider;
int riot_state;

#define RIOT_IDLE 0
#define RIOT_COUNTUP 1
#define RIOT_COUNTDOWN 2

int mtrap_voice;

static signed char exidy_waveform1[16] =
{
	/* square-wave */
	0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F,
	0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F
};

int exidy_shdata_latch = 0xFF;
int exidy_mhdata_latch = 0xFF;

/* 6532 variables */
static int irq_flag = 0;   /* 6532 interrupt flag register */
static int irq_enable = 0;
static int PA7_irq = 0;  /* IRQ-on-write flag (sound CPU) */

/* 6840 variables */
static int sh6840_CR1,sh6840_CR2,sh6840_CR3;
static int sh6840_MSB;
static unsigned int sh6840_timer[3];
static int exidy_sfxvol[3];
static int exidy_sfxctrl;

/***************************************************************************
	PIA Interface
***************************************************************************/

static void exidy_irq (void);

static pia6821_interface pia_intf =
{
	2,                                              /* 2 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
    { pia_1_porta_r, pia_2_porta_r },               /* input port A */
    { pia_1_ca1_r,   pia_2_ca1_r },                 /* input bit CA1 */
    { pia_1_ca2_r,   pia_2_ca2_r },                 /* input bit CA2 */
    { pia_1_portb_r, pia_2_portb_r },               /* input port B */
    { pia_1_cb1_r,   pia_2_cb1_r },                 /* input bit CB1 */
    { pia_1_cb2_r,   pia_2_cb2_r },                 /* input bit CB2 */
	{ pia_2_portb_w, pia_1_portb_w },               /* output port A */
	{ pia_2_porta_w, pia_1_porta_w },               /* output port B */
	{ pia_2_cb1_w,   pia_1_cb1_w },                 /* output CA2 */
	{ pia_2_ca1_w,   pia_1_ca1_w },                 /* output CB2 */
	{ 0, exidy_irq },                               /* IRQ A */
	{ 0, 0 }                                        /* IRQ B */
};

int exidy_sh_start(void)
{
	/* Init 8253 */
	exidy_sh8253_clstate[0]=0;
	exidy_sh8253_clstate[1]=0;
	exidy_sh8253_clstate[2]=0;
	exidy_sh8253_count[0]=0;
	exidy_sh8253_count[1]=0;
	exidy_sh8253_count[2]=0;

	exidy_sample_channels[0] = get_play_channels(1);
	exidy_sample_channels[1] = get_play_channels(2);
	exidy_sample_channels[2] = get_play_channels(3);
	osd_play_sample(exidy_sample_channels[0],(signed char*)exidy_waveform1,16,1000,0x00,1);
	osd_play_sample(exidy_sample_channels[1],(signed char*)exidy_waveform1,16,1000,0x00,1);
	osd_play_sample(exidy_sample_channels[2],(signed char*)exidy_waveform1,16,1000,0x00,1);

	/* Init PIA */
	pia_startup (&pia_intf);

	/* Init 6532 */
    timer=0;
    riot_divider = 1;
    riot_state = RIOT_IDLE;

	/* Init 6840 */
	sh6840_CR1 = sh6840_CR2 = sh6840_CR3 = 0;
	sh6840_MSB = 0;
	sh6840_timer[0] = sh6840_timer[1] = sh6840_timer[2] = 0;
	exidy_sfxvol[0] = exidy_sfxvol[1] = exidy_sfxvol[2] = 0;
    exidy_sample_channels[3] = get_play_channels(4);
	exidy_sample_channels[4] = get_play_channels(5);
	exidy_sample_channels[5] = get_play_channels(6);
	osd_play_sample(exidy_sample_channels[3],(signed char*)exidy_waveform1,16,1000,0x00,1);
	osd_play_sample(exidy_sample_channels[4],(signed char*)exidy_waveform1,16,1000,0x00,1);
    osd_play_sample(exidy_sample_channels[5],(signed char*)exidy_waveform1,16,1000,0x00,1);

	return 0;
}

void exidy_sh_stop(void)
{
	osd_stop_sample(exidy_sample_channels[0]);
	osd_stop_sample(exidy_sample_channels[1]);
	osd_stop_sample(exidy_sample_channels[2]);
}

static void riot_interrupt(int parm)
{
    if (riot_state == RIOT_COUNTUP) {
        irq_flag |= 0x80; /* set timer interrupt flag */
        if (irq_enable) cpu_cause_interrupt (1, M6502_INT_IRQ);
        riot_state = RIOT_COUNTDOWN;
        timer = timer_set (TIME_IN_USEC((1*BASE_TIME)*0xFF), 0, riot_interrupt);
    }
    else {
        timer=0;
        riot_state = RIOT_IDLE;
    }
}

/*
 *  PIA callback to generate the interrupt to the main CPU
 */

static void exidy_irq (void)
{
    cpu_cause_interrupt (1, M6502_INT_IRQ);
}


void exidy_shriot_w(int offset,int data)
{
   long i=0;

//   if (errorlog) fprintf(errorlog,"RIOT: %x=%x\n",offset,data);
   offset &= 0x7F;
   switch (offset)
   {
	case 0:
		mtrap_voice = data;
		return;
   	case 7: /* 0x87 - Enable Interrupt on PA7 Transitions */
		PA7_irq = data;
		return;
	case 0x14:
	case 0x1c:
        irq_enable=offset & 0x08;
	    riot_divider = 1;
        if (timer) timer_remove(timer);
        timer = timer_set (TIME_IN_USEC((1*BASE_TIME)*data), 0, riot_interrupt);
        riot_state = RIOT_COUNTUP;
        return;
	case 0x15:
	case 0x1d:
        irq_enable=offset & 0x08;
	    riot_divider = 8;
        if (timer) timer_remove(timer);
        timer = timer_set (TIME_IN_USEC((8*BASE_TIME)*data), 0, riot_interrupt);
        riot_state = RIOT_COUNTUP;
        return;
    case 0x16:
    case 0x1e:
        irq_enable=offset & 0x08;
	    riot_divider = 64;
        if (timer) timer_remove(timer);
        timer = timer_set (TIME_IN_USEC((64*BASE_TIME)*data), 0, riot_interrupt);
        riot_state = RIOT_COUNTUP;
		return;
	case 0x17:
    case 0x1f:
        irq_enable=offset & 0x08;
	    riot_divider = 1024;
        if (timer) timer_remove(timer);
        timer = timer_set (TIME_IN_USEC((1024*BASE_TIME)*data), 0, riot_interrupt);
        riot_state = RIOT_COUNTUP;
		return;
	default:
	    if (errorlog) fprintf(errorlog,"Undeclared RIOT write: %x=%x\n",offset,data);
	    return;
	}
	return; /* will never execute this */
}


int exidy_shriot_r(int offset)
{
	static int temp;

//  if (errorlog) fprintf(errorlog,"RIOT(r): %x\n",offset);
	offset &= 0x07;
	switch (offset)
	{
    case 0x02:
        return 0xFF;
	case 0x05: /* 0x85 - Read Interrupt Flag Register */
	case 0x07:
		temp = irq_flag;
		irq_flag = 0;   /* Clear int flags */
		return temp;
	case 0x04:
	case 0x06:
		irq_flag = 0;
		if (riot_state == RIOT_COUNTUP) {
	        return timer_timeelapsed(timer)/(TIME_IN_USEC((riot_divider*BASE_TIME)));
		}
		else {
			return timer_timeleft(timer)/(TIME_IN_USEC((riot_divider*BASE_TIME)));
		}
	default:
	    if (errorlog) fprintf(errorlog,"Undeclared RIOT read: %x  PC:%x\n",offset,cpu_get_pc());
  	    return 0xff;
	}
	return 0;
}

void exidy_sh8253_w(int offset,int data)
{
	int i,c;
	long f;


//    if (errorlog) fprintf(errorlog,"8253: %x=%x\n",offset,data);

	i = offset & 0x03;
	if (i == 0x03) {
		c = (data & 0xc0) >> 6;
		if (exidy_sh8253_count[c])
			f = BASE_FREQ / exidy_sh8253_count[c];
		else
            f = 1;

		if ((data & 0x0E) == 0) {
//			exidy_sh8253_clstate[c] = 0;
//			f=0;
			osd_adjust_sample(exidy_sample_channels[c],f,0x00);
		}
		else {
			osd_adjust_sample(exidy_sample_channels[c],f,0xFF);
		}
	}

	if (i < 0x03)
	{
		if (!exidy_sh8253_clstate[i])
		{
			exidy_sh8253_clstate[i]=1;
			exidy_sh8253_count[i] &= 0xFF00;
			exidy_sh8253_count[i] |= (data & 0xFF);
		}
		else
		{
			exidy_sh8253_clstate[i]=0;
			exidy_sh8253_count[i] &= 0x00FF;
			exidy_sh8253_count[i] |= ((data & 0xFF) << 8);
            if (!exidy_sh8253_count[i])
                f = 1;
            else
                f = BASE_FREQ / exidy_sh8253_count[i];
			osd_adjust_sample(exidy_sample_channels[i],f,0xFF);
//          if (errorlog) fprintf(errorlog,"Timer %d = %u\n",i,exidy_sh8253_count[i]);
		}
	}


}

int exidy_sh8253_r(int offset)
{
//        if (errorlog) fprintf(errorlog,"8253(R): %x\n",offset);
	return 0;
}

int exidy_sh6840_r(int offset) {
    if (errorlog) fprintf(errorlog,"6840R %x\n",offset);
    return 0;
}

void exidy_sh6840_w(int offset,int data) {
    	offset &= 0x07;
	switch (offset) {
		case 0:
			if (sh6840_CR2 & 0x01) {
				sh6840_CR1 = data;
				if ((data & 0x38) == 0) osd_adjust_sample(exidy_sample_channels[3],0,0x00);
			}
			else {
				sh6840_CR3 = data;
				if ((data & 0x38) == 0) osd_adjust_sample(exidy_sample_channels[5],0,0x00);
			}
			if (errorlog) fprintf(errorlog,"6840 CR1:%x CR3:%x\n",sh6840_CR1,sh6840_CR3);
			break;

		case 1:
			sh6840_CR2 = data;
			if ((data & 0x38) == 0) osd_adjust_sample(exidy_sample_channels[4],0,0x00);
			if (errorlog) fprintf(errorlog,"6840 CR2:%x\n",sh6840_CR2);
			break;
		case 2:
		case 4:
		case 6:
			sh6840_MSB = data;
			break;
		case 3:
			sh6840_timer[0] = (sh6840_MSB << 8) | (data & 0xFF);
			if (sh6840_timer[0] != 0 && !(exidy_sfxctrl & 0x02))
				osd_adjust_sample(exidy_sample_channels[3],SH6840_FREQ/sh6840_timer[0],exidy_sfxvol[0]);
			else
				osd_adjust_sample(exidy_sample_channels[3],0,0x00);
			break;
		case 5:
			sh6840_timer[1] = (sh6840_MSB << 8) | (data & 0xFF);
			if (sh6840_timer[1] != 0)
				osd_adjust_sample(exidy_sample_channels[4],SH6840_FREQ/sh6840_timer[1],exidy_sfxvol[1]);
			else
				osd_adjust_sample(exidy_sample_channels[4],0,0x00);
			break;
		case 7:
			sh6840_timer[2] = (sh6840_MSB << 8) | (data & 0xFF);
			if (sh6840_timer[2] != 0)
				osd_adjust_sample(exidy_sample_channels[5],SH6840_FREQ/sh6840_timer[2],exidy_sfxvol[2]);
			else
				osd_adjust_sample(exidy_sample_channels[5],0,0x00);
			break;
	}
}

void exidy_sfxctrl_w(int offset,int data) {
	switch (offset & 0x03) {
	case 0:
		exidy_sfxctrl = data;
		if (!(data & 0x02)) osd_adjust_sample(exidy_sample_channels[3],0,0x00);
		break;
	case 1:
	case 2:
	case 3:
		exidy_sfxvol[offset - 1] = (data & 0x07) << 5;
		break;
	}
}

void mtrap_voiceio_w(int offset,int data) {

}

int mtrap_voiceio_r(int offset) {
	return 0;
}
