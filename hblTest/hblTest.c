// Test of HBL and SCU Timers interruption handlers
// 
// Shazz on libyaul, 2016

#include <yaul.h>
#include <stdlib.h>
#include <scu/ic.h>

#define	DISPLAY_PIXEL_WIDTH			320
#define	DISPLAY_PIXEL_HEIGHT		240
#define	TIMER_T1_COUNTDOWN		    426    //9bits value

// Timer 1 data value for screen line size equivalent, from SEGA SCU final revision doc
// 320px = 426 ticks (+106)
// 352px = 454 ticks (+102)
// 424px = 211 ticks (???)
// 426px = 212 ticks (???)
struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

static void scu_timer0(void);
static void scu_timer1(void);

static uint32_t	g_timer0_handler_counter = 0;
static uint16_t	g_timer0_handler_compare = 0;
static uint32_t	g_timer1_handler_counter = 0;
static uint32_t	g_hblank_handler_counter = 0;
static uint32_t	g_frames_displayed_counter = 0;

void hblank_out_handler(irq_mux_handle_t *irq_mux __unused);
void timer0_handler(irq_mux_handle_t *irq_mux __unused);
void timer1_handler(irq_mux_handle_t *irq_mux __unused);

irq_mux_t scu_timer0_irq_mux;
irq_mux_t scu_timer1_irq_mux;
irq_mux_t * vblank_in;
irq_mux_t * vblank_out;
irq_mux_t * hblank_out;

static struct cons cons;

static void hardware_init(void)
{
	/* VDP2 */
	vdp2_init();

	/* Enable color offset function on scroll screen NBG0 and assign all screens to color offset A */
    MEMORY_WRITE(16, VDP2(CLOFEN), 0x007f); /* 11 1111 */
    MEMORY_WRITE(16, VDP2(CLOFSL), 0x0000);

	/* Set R,G,B values for color offset A to none */
	MEMORY_WRITE(16, VDP2(COAR), 0x0000);
	MEMORY_WRITE(16, VDP2(COAB), 0x0000);
	MEMORY_WRITE(16, VDP2(COAG), 0x0000);    

	/* VDP1 */
	vdp1_init();

	/* SMPC */
	smpc_init();
	smpc_peripheral_init();

	/* CONS */
	cons_init(&cons, CONS_DRIVER_VDP2);

	static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);
    
	cons_write(&cons, "\n[1;44m   *** HBL and TIMERS TEST ***     [m\n\n");

	/* Disable interrupts */
	cpu_intc_disable();

	vblank_in = vdp2_tvmd_vblank_in_irq_get();
	irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

	vblank_out = vdp2_tvmd_vblank_out_irq_get();
	irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);

	// create mux handlers for timers
	irq_mux_init(&scu_timer0_irq_mux);
	irq_mux_init(&scu_timer1_irq_mux);

	// set interrupts
    uint32_t mask;
    mask = IC_MASK_HBLANK_IN | IC_MASK_TIMER_0 | IC_MASK_TIMER_1;
    scu_ic_mask_chg(IC_MASK_ALL, mask);
	scu_ic_interrupt_set(IC_INTERRUPT_TIMER_0, &scu_timer0);
	scu_ic_interrupt_set(IC_INTERRUPT_TIMER_1, &scu_timer1);
	scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);

	// set handlers
	hblank_out = vdp2_tvmd_hblank_in_irq_get();
	irq_mux_handle_add(hblank_out, hblank_out_handler, NULL);
	irq_mux_handle_add(&scu_timer0_irq_mux, timer0_handler, NULL);
	irq_mux_handle_add(&scu_timer1_irq_mux, timer1_handler, NULL);
    
    // Set timer0 compare and timer1 countdown value+enable
	g_timer0_handler_compare = DISPLAY_PIXEL_HEIGHT/2;
	scu_timer_0_set(g_timer0_handler_compare);
	scu_timer_1_set(TIMER_T1_COUNTDOWN);
	scu_timer_1_mode_set(true);    

	/* Enable interrupts */
	cpu_intc_enable();
}


int main(void)
{
	char *buf;

	hardware_init();

	cons_write(&cons, "Time to count....\n");
	buf = (char *)malloc(1024);

	/* Main loop */
	for(;;)
	{
		// end of display
	  	vdp2_tvmd_vblank_in_wait();
	  	g_frames_displayed_counter++;

		(void)sprintf(buf, "[06;2HVBL : %08lu", g_frame_counter);  /* VBL count */
		cons_write(&cons, buf);
		(void)sprintf(buf, "[07;2HFDL: %08lu", g_frames_displayed_counter); /* Frames displayed count <= VBL */
		cons_write(&cons, buf);
		(void)sprintf(buf, "[08;2HHBL: %08lu", g_hblank_handler_counter); /* HBL count */
		cons_write(&cons, buf);
		(void)sprintf(buf, "[09;2HT0 : %08lu CMP : %04u", g_timer0_handler_counter, g_timer0_handler_compare); /* Timer 0 counter and compare */
		cons_write(&cons, buf);
		(void)sprintf(buf, "[10;2HT1 : %08lu CNT : %03d", g_timer1_handler_counter, TIMER_T1_COUNTDOWN); /* Timer 1 counter and count down value */
		cons_write(&cons, buf);

	  	cons_flush(&cons);

	  	// end of display
		vdp2_tvmd_vblank_out_wait();
	}

	return 0;
}
 
static void scu_timer0(void)
{
        irq_mux_handle(&scu_timer0_irq_mux);
}

static void scu_timer1(void)
{
        irq_mux_handle(&scu_timer1_irq_mux);
}


/*
 * vblank_in_handler
 *
 * VBL end of display interrupt handler
 *
 */
static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
  	g_frame_counter = (tick > 0) ? (g_frame_counter + 1) : 0;
  	smpc_peripheral_digital_port(1, &g_digital);
}

/*
 * vblank_in_handler
 *
 * VBL beginning of display interrupt handler
 *
 */
static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
    MEMORY_WRITE(16, VDP2(COAR), 0);
    MEMORY_WRITE(16, VDP2(COAB), 0);
    MEMORY_WRITE(16, VDP2(COAG), 0);   
  
 	tick = (tick & 0xFFFFFFFF) + 1;
}

/*
 * timer0_handler
 *
 * Timer0 interrupt handler
 * incr global g_timer0_handler_counter
 * count in fact... VBL as compare value is set to 
 * screen height
 *
 */
void timer0_handler(irq_mux_handle_t *irq_mux __unused)
{
	/* Paranoia save of MACH/MACL */
	__asm__("sts.l	macl,@-r15\n");
    __asm__("sts.l	mach,@-r15\n");

   	/* Update the timer 0 counter */
	g_timer0_handler_counter += 1;

    MEMORY_WRITE(16, VDP2(COAR), -80);
    MEMORY_WRITE(16, VDP2(COAB), -80);
    MEMORY_WRITE(16, VDP2(COAG), -80); 

	/* reset compare reg */
    scu_timer_0_set(g_timer0_handler_compare);
   
	/* Restore the Paranoia save of MACH/MACL */
	__asm__("lds.l	@r15+,mach\n");
    __asm__("lds.l	@r15+,macl\n");    
}


/*
 * timer1_handler
 *
 * Timer1 interrupt handler
 * incr global g_timer1_handler_counter
 * reset count down data to TIMER_T1_COUNTDOWN, set to be equal to one line
 *
 */
void timer1_handler(irq_mux_handle_t *irq_mux __unused)
{
	/* Paranoia save of MACH/MACL */
	__asm__("sts.l	macl,@-r15\n");
    __asm__("sts.l	mach,@-r15\n");

    /* Update the timer 1 counter */
	g_timer1_handler_counter += 1;
    scu_timer_1_set(TIMER_T1_COUNTDOWN);
    
	/* Restore the Paranoia save of MACH/MACL */
	__asm__("lds.l	@r15+,mach\n");
    __asm__("lds.l	@r15+,macl\n");  
}

/*
 * Hblankhandler
 *
 * HBL out interrupt handler
 * incr global g_timer1_handler_counter
 * counts HBLs
 *
 */
void hblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
	/* Paranoia save of MACH/MACL */
	__asm__("sts.l	macl,@-r15\n");
    __asm__("sts.l	mach,@-r15\n");

   /* Update the HBL out counter */
	g_hblank_handler_counter += 1;

	/* Restore the Paranoia save of MACH/MACL */
	__asm__("lds.l	@r15+,mach\n");
    __asm__("lds.l	@r15+,macl\n");  
}

