/*** TMS34010: Portable TMS34010 emulator ***********************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
	 originally based on code by Aaron Giles

	Public include file

*****************************************************************************/

#ifndef _TMS34010_H
#define _TMS34010_H

#include "osd_cpu.h"

/* TMS34010 State */
typedef struct
{
	UINT32 op;
	UINT32 pc;
	UINT32 st;        /* Only here so we can display it in the debug window */
	union		  				/* The register files are interleaved, so */
	{							/* that the SP occupies the same location in both */
			INT32 Bregs[241];	/* Only every 16th entry is actually used */
		struct
		{
			INT32 unused[225];
			INT32 Aregs[16];
		} a;
	} regs;
	UINT32 nflag;
	UINT32 cflag;
	UINT32 notzflag;  /* So we can just do an assignment to set it */
	UINT32 vflag;
	UINT32 pflag;
	UINT32 ieflag;
	UINT32 fe0flag;
	UINT32 fe1flag;
	UINT32 fw[2];
	UINT32 fw_inc[2];  /* Same as fw[], except when fw = 0, fw_inc = 32 */
	UINT32 IOregs[32];
	void (*F0_write) (UINT32 bitaddr, UINT32 data);
	void (*F1_write) (UINT32 bitaddr, UINT32 data);
	 INT32 (*F0_read) (UINT32 bitaddr);
	 INT32 (*F1_read) (UINT32 bitaddr);
    UINT32 (*pixel_write)(UINT32 address, UINT32 value);
    UINT32 (*pixel_read)(UINT32 address);
	UINT32 transparency;
	UINT32 window_checking;
	 INT32 (*raster_op)(INT32 newpix, INT32 oldpix);
	UINT32 lastpixaddr;
	UINT32 lastpixword;
	UINT32 lastpixwordchanged;
	UINT32 xytolshiftcount1;
	UINT32 xytolshiftcount2;
	UINT16* shiftreg;
	void (*to_shiftreg)  (UINT32 address, UINT16* shiftreg);
    void (*from_shiftreg)(UINT32 address, UINT16* shiftreg);
	UINT8* stackbase;
	UINT32 stackoffs;
#if NEW_INTERRUPT_SYSTEM
	int nmi_state;
	int irq_state[5];
	int (*irq_callback)(int irqline);
#endif
} TMS34010_Regs;

/* average instruction time in cycles */
#define TMS34010_AVGCYCLES 2

/* Interrupt Types that can be generated by outside sources */
#define TMS34010_INT_NONE	0x0000
#define TMS34010_INT1		0x0002	/* External Interrupt 1 */
#define TMS34010_INT2		0x0004	/* External Interrupt 2 */

/* PUBLIC FUNCTIONS */
extern unsigned TMS34010_GetPC(void);
extern void TMS34010_SetRegs(TMS34010_Regs *Regs);
extern void TMS34010_GetRegs(TMS34010_Regs *Regs);
extern void TMS34010_Reset(void);
extern int	TMS34010_Execute(int cycles);
#if NEW_INTERRUPT_SYSTEM
extern void TMS34010_set_nmi_line(int linestate);
extern void TMS34010_set_irq_line(int irqline, int linestate);
extern void TMS34010_set_irq_callback(int (*callback)(int irqline));
extern void TMS34010_internal_interrupt(int type);
#else
extern void TMS34010_Cause_Interrupt(int type);
extern void TMS34010_Clear_Pending_Interrupts(void);
#endif

extern void TMS34010_State_Save(int cpunum, void *f);
extern void TMS34010_State_Load(int cpunum, void *f);

void TMS34010_HSTADRL_w (int offset, int data);
void TMS34010_HSTADRH_w (int offset, int data);
void TMS34010_HSTDATA_w (int offset, int data);
int  TMS34010_HSTDATA_r (int offset);
void TMS34010_HSTCTLH_w (int offset, int data);

/* Sets functions to read/write shift register */
void TMS34010_set_shiftreg_functions(int cpu,
									 void (*to_shiftreg  )(UINT32, UINT16*),
									 void (*from_shiftreg)(UINT32, UINT16*));

/* Sets functions to read/write shift register */
void TMS34010_set_stack_base(int cpu, UINT8* stackbase, UINT32 stackoffs);

/* Writes to the 34010 io */
void TMS34010_io_register_w(int offset, int data);

/* Reads from the 34010 io */
int TMS34010_io_register_r(int offset);

/* Checks whether the display is inhibited */
int TMS34010_io_display_blanked(int cpu);

int TMS34010_get_DPYSTRT(int cpu);

/* PUBLIC GLOBALS */
extern int TMS34010_ICount;


/* Use this macro in the memory definitions to specify bit-based addresses */
#define TOBYTE(bitaddr) ((UINT32) (bitaddr)>>3)


#endif /* _TMS34010_H */
