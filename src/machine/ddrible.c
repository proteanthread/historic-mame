/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/M6809/M6809.h"

unsigned char *ddrible_sharedram;
unsigned char *ddrible_snd_sharedram;
int int_enable_0, int_enable_1;

void ddrible_init_machine( void )
{
    /* Set optimization flags for M6809 */
    m6809_Flags = M6809_FAST_S | M6809_FAST_U;
	int_enable_0 = int_enable_1 = 0;
	cpu_reset( 1 );
}

void ddrible_bankswitch_w( int offset,int data )
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x0f) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

int ddrible_interrupt_0( void )
{
	if (int_enable_0)
		return M6809_INT_FIRQ;
	return ignore_interrupt();
}

int ddrible_interrupt_1( void )
{
	if (int_enable_1)
		return M6809_INT_FIRQ;
	return ignore_interrupt();
}

void int_0_w( int offset,int data )
{
	if (data & 0x02)
		int_enable_0 = 1;
	else
		int_enable_0 = 0;
}

void int_1_w( int offset,int data )
{
	if (data & 0x02)
		int_enable_1 = 1;
	else
		int_enable_1 = 0;
}

int ddrible_sharedram_r( int offset )
{
	return ddrible_sharedram[offset];
}

void ddrible_sharedram_w( int offset,int data )
{
	ddrible_sharedram[offset] = data;
}

int ddrible_snd_sharedram_r( int offset )
{
	return ddrible_snd_sharedram[offset];
}

void ddrible_snd_sharedram_w( int offset,int data )
{
	ddrible_snd_sharedram[offset] = data;
}
