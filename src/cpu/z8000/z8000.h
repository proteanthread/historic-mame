#ifndef Z8K_H

#include "osd_cpu.h"

typedef union {
	UINT8	B[16]; /* RL0,RH0,RL1,RH1...RL7,RH7 */
	UINT16	W[16]; /* R0,R1,R2...R15 */
	UINT32	L[8];  /* RR0,RR2,RR4..RR14 */
	UINT64	Q[4];  /* RQ0,RQ4,..RQ12 */
}	z8000_reg_file;

typedef struct {
	UINT16	op[4];		/* opcodes/data of current instruction */
    UINT16  pc;         /* program counter */
    UINT16  psap;       /* program status pointer */
    UINT16  fcw;        /* flags and control word */
    UINT16  refresh;    /* refresh timer/counter */
    UINT16  nsp;        /* system stack pointer */
    UINT16  irq_req;    /* CPU is halted, interrupt or trap request */
    UINT16  irq_srv;    /* serviced interrupt request */
    UINT16  irq_vec;    /* interrupt vector */
	int nmi_state;		/* NMI line state */
	int irq_state[2];	/* IRQ line states (NVI, VI) */
	int (*irq_callback)(int irqline);
	z8000_reg_file regs;/* registers */
}	z8000_Regs;

/* Interrupt Types that can be generated by outside sources */
#define Z8000_TRAP		0x4000	/* internal trap */
#define Z8000_NMI		0x2000	/* non maskable interrupt */
#define Z8000_SEGTRAP	0x1000	/* segment trap (Z8001) */
#define Z8000_NVI		0x0800	/* non vectored interrupt */
#define Z8000_VI		0x0400	/* vectored interrupt (LSB is vector)  */
#define Z8000_SYSCALL	0x0200	/* system call (lsb is vector) */
#define Z8000_HALT		0x0100	/* halted flag	*/
#define Z8000_INT_NONE  0x0000

/* PUBLIC FUNCTIONS */
extern void z8000_setregs(z8000_Regs *Regs);
extern void z8000_getregs(z8000_Regs *Regs);
extern unsigned z8000_getpc(void);
extern unsigned z8000_getreg(int regnum);
extern void z8000_setreg(int regnum, unsigned val);
extern void z8000_reset(void *param);
extern void z8000_exit(void);
extern int	z8000_execute(int cycles);
extern void z8000_set_nmi_line(int state);
extern void z8000_set_irq_line(int irqline, int state);
extern void z8000_set_irq_callback(int (*callback)(int irqline));
extern const char *z8000_info(void *context, int regnum);

extern void z8000_State_Save(int cpunum, void *f);
extern void z8000_State_Load(int cpunum, void *f);

/* PUBLIC GLOBALS */
extern int z8000_ICount;

#ifdef MAME_DEBUG
extern int mame_debug;
extern int DasmZ8000(char *buff, int pc);
#endif

#endif /* Z8K_H */
