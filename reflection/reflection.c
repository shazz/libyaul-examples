// Amiga style reflection
// 
// Shazz on libyaul, 2015

#define N0LSS0 (1 << 12)
#define N0LSS1 (1 << 13) //enable 8 lines block - 00, 01 10 11 for 1,2, 4 or 8 lines based on non-interlaced settings

#include <yaul.h>
#include <stdlib.h>

#include "logo.h"
#include "stars.h"
#include "tables.h"

/*
 * VDP2 VRAM Organization
 * Bank A0
 * - NBG0 bitmap data, from 0x0 to 0x0FFFF (4*16384 bytes)
 * 
 * Bank A1
 * - NBG1 bitmap data, from 0x0 to 0x0FFFF (4*16384 bytes)
 *  
 * Bank B0
 * - Linescroll table, from 0x0 to 0x0F00 (4*4*240 bytes)
 * 
 * Bank B1
 * - Backscreen table, from 0x0 to 0x01E0 (2*240 bytes)
 * 
 * CRAM
 * - NBG0 palette set at palette 0 (0x0)
 * - NBG1 palette set at palette 16 (0x0)
 * 
 * Registers:
 * - TVMD: enable display, border color mode to back screen, 240 lines vertical resolution
 * - RAMCTL : set accesses to bitmap NBG0
 * - BMPNA : set palette 0 for NBG0
 * - BKTAL/BKTAU : set backscreen table address
 * - SCRCTL : set vertical line scroll for NBG0
 * - LSTA0U/STA0L : set line scroll table address
*/ 

#define	TIMER_T1_COUNTDOWN		    426    //9bits value

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;
static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);
static void timer0_handler(irq_mux_handle_t *);

static void scu_timer0(void);
void init_vdp2_VRAM(void);

irq_mux_t scu_timer0_irq_mux;
irq_mux_t * vblank_in;
irq_mux_t * vblank_out;

static uint32_t	g_timer0_handler_counter = 0;
static uint16_t	g_timer0_handler_compare = 0;
//static uint32_t	g_hblank_handler_counter = 0;

#define CRAM_PAL_MODE_OFFSET(bank, nb)     (0x25F00000 + ((bank) << 9) +  ((nb) << 5))
#define NBG0_VRAM_BANK_OFFSET 0
#define NBG0_VRAM_PAL_NB 0
#define NBG1_VRAM_BANK_OFFSET 1
#define NBG1_VRAM_PAL_NB 0
//static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_PAL_MODE_OFFSET(NBG0_VRAM_BANK_OFFSET, NBG0_VRAM_PAL_NB);
//static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_PAL_MODE_OFFSET(NBG1_VRAM_BANK_OFFSET, NBG1_VRAM_PAL_NB);

static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(0, 0, 0);
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(1, 0, 0);

/* Line Scroll table address */
static uint32_t *line_scroll_tb = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x0);
static uint32_t *backscreen_tb = (uint32_t *)VRAM_ADDR_4MBIT(3, 0x0);
struct scrn_ls_format linescrollfmt;

/*
 * static void hardware_init(void)
 * init hw
 */ 
static void hardware_init(void)
{
	/* VDP2 */
	vdp2_init();
    init_vdp2_VRAM();
    
    /* set 320x240 res */
    vdp2_tvmd_display_clear();
    uint16_t tvmd = MEMORY_READ(16, VDP2(TVMD));
    tvmd |= ((1 << 8) | (1 << 4));  // set BDCLMD,  VRES0 to 1
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);    
    
    // Set Color mode to mode 0 (2KWord Color RAM)
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);        
    
	/* Enable color offset function on scroll screen NBG0 and assign all screens to color offset A */
    MEMORY_WRITE(16, VDP2(CLOFEN), 0x0001); /* 00 0001 */
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

	/* Disable interrupts */
	cpu_intc_disable();

	irq_mux_t *vblank_in;
	irq_mux_t *vblank_out;

	vblank_in = vdp2_tvmd_vblank_in_irq_get();
	irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

	vblank_out = vdp2_tvmd_vblank_out_irq_get();
	irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
    
	// create mux handlers for timers
	irq_mux_init(&scu_timer0_irq_mux);

	// set interrupts
    uint32_t mask;
    mask = IC_MASK_TIMER_0;
    scu_ic_mask_chg(IC_MASK_ALL, mask);
	scu_ic_interrupt_set(IC_INTERRUPT_TIMER_0, &scu_timer0);
	scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);

	// set timer0 handler
	irq_mux_handle_add(&scu_timer0_irq_mux, timer0_handler, NULL);
    
    // Set timer0 compare and enable
	g_timer0_handler_compare = 180;
	scu_timer_0_set(g_timer0_handler_compare);
	scu_timer_1_mode_set(false);    // to be fixed in libyaul, else Timer0 is never enabled

	/* Enable interrupts */
	cpu_intc_enable();
}
     
/*
 *  void initBackScroll(void)
 *  Setup rasters
 */
void initBackScroll(void)
{
    // wait vblank
    vdp2_tvmd_display_clear();
        
    /* Copy rasters bars color data to VRAM */
    memcpy(backscreen_tb, backscreen, sizeof(backscreen));   
    
    /* set BKTAL and BKTAU (Back screen table address register */
    MEMORY_WRITE(16, VDP2(BKTAL), ( ( (uint32_t) backscreen_tb >> 1) & 0x0FFFF) );
    MEMORY_WRITE(16, VDP2(BKTAU), (0x8000 | ( ((uint32_t) backscreen_tb >> 17) & 0x7)));    
    
    //vdp2_scrn_back_screen_set(/* single_color = */ false, VRAM_ADDR_4MBIT(2, 0), (uint16_t *)backscreen, 240*2);   
}

/*
 *  void initScrollScreenNBBG0(void)
 *  Setup line scroll
 */
void initScrollScreenNBG0(void)
{
    /* set NBG0 in bitmap mode, 16 col, 512x256 */
    struct scrn_bitmap_format nbg0_format;
    uint16_t bmpma;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();
    
    /* Copy the NBG0 bitmap, BGR555 palette data */
    memcpy((void *)VRAM_ADDR_4MBIT(0, 0x00000), bitmap_data, sizeof(bitmap_data));   
    memcpy(_nbg0_color_palette, bitmap_palette, sizeof(bitmap_palette));            

    nbg0_format.sbf_scroll_screen = SCRN_NBG0;                      /* Normal background */
    nbg0_format.sbf_cc_count = SCRN_CCC_PALETTE_16;                 /* color mode to PAL16 */
    nbg0_format.sbf_bitmap_size.width = 512;                        /* Bitmap sizes: 512x256 */
    nbg0_format.sbf_bitmap_size.height = 256;
    nbg0_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);   /* Bitmap pattern lead address */
    nbg0_format.sbf_color_palette = (uint32_t)_nbg0_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg0_format);
    
    // then set BMPNA (again) for NBG0
    bmpma = MEMORY_READ(16, VDP2(BMPNA));
    bmpma |= ((NBG0_VRAM_PAL_NB & 0x7 ) << 0);
    MEMORY_WRITE(16, VDP2(BMPNA), bmpma);
	
	// init vertical line scroll table in VRAM
	uint16_t y;
    // draw lines in normal order
	for (y = 0; y < 180; y++) 
	{
        line_scroll_tb[2*y+0]  = 0;   // set horizontal offset
        line_scroll_tb[2*y+1]  = y << 16;   // set vertical line value
	}
    // blank lines
    for (y = 180; y < 184; y++) 
	{
        line_scroll_tb[2*y+0]  = 0;         // set horizontal offset
        line_scroll_tb[2*y+1]  = 0;         // set vertical line value
	}     
    // reverse bitmap squeezed, 1 lines upon 2
    for (y = 0; y < 240-184; y++) 
	{
        line_scroll_tb[2*184+(2*y+0)]  = lut[y] << 15;          // set horizontal offset
        line_scroll_tb[2*184+(2*y+1)]  = (184-(2*y)) << 16;     // set vertical line value
	}  
    
    linescrollfmt.ls_scrn = SCRN_NBG0;
    linescrollfmt.ls_lsta = (uint32_t)line_scroll_tb;
    linescrollfmt.ls_int = 0;	              
    linescrollfmt.ls_fun =  SCRN_LS_N0SCX | SCRN_LS_N0SCY;	   // enable : SCRN_LS_N0SCX | SCRN_LS_N0SCY | N0LSS0 | N0LSS1 | N0LZMX       
    vdp2_scrn_ls_set(&linescrollfmt);
        
    // set priority for NB1 to be backward NBG0
    vdp2_priority_spn_set(SCRN_NBG0, 7);   
        
    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);
}

/*
 *  void initScrollScreenNBBG1(void)
 *  Setup line scroll
 */
void initScrollScreenNBG1(void)
{
    /* set NBG1 in bitmap mode, 16 col, 512x256 */
    struct scrn_bitmap_format nbg1_format;
    //uint16_t bmpma;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();
    
    /* Copy the NBG1 bitmap, BGR555 palette data */
    memcpy((void *)VRAM_ADDR_4MBIT(1, 0x00000), stars_data, sizeof(stars_data));   
    memcpy(_nbg1_color_palette, stars_palette, sizeof(stars_palette));            

    nbg1_format.sbf_scroll_screen = SCRN_NBG1;                      /* Normal background */
    nbg1_format.sbf_cc_count = SCRN_CCC_PALETTE_16;                 /* color mode to PAL16 */
    nbg1_format.sbf_bitmap_size.width = 512;                        /* Bitmap sizes: 512x256 */
    nbg1_format.sbf_bitmap_size.height = 256;
    nbg1_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(1, 0x00000);   /* Bitmap pattern lead address */
    nbg1_format.sbf_color_palette = (uint32_t)_nbg1_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg1_format);
    
    // then set BMPNA (again) for NBG1
    MEMORY_WRITE(16, VDP2(CRAOFA), (NBG1_VRAM_BANK_OFFSET & 0x7) << 4); 
    //bmpma = MEMORY_READ(16, VDP2(BMPNA));
    //bmpma &= 0x3037; 
    //bmpma |= ((NBG1_VRAM_PAL_NB & 0x7 ) << 8);   
    //MEMORY_WRITE(16, VDP2(BMPNA), bmpma);
    
    // set priority for NB1 to be backward NBG0
    vdp2_priority_spn_set(SCRN_NBG1, 6);        
    vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);
    
}

void startDisplay(void)
{
    struct vram_ctl *vram_ctl;
        
    // 16 colors bitmap requires 1 access each
    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1; 
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS; //VRAM_CTL_CYCP_VCSTDR_NBG0
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS; //VRAM_CTL_CYCP_VCSTDR_NBG0
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);   
    
    vdp2_tvmd_display_set();
}

int main(void)
{
    uint16_t ofs = 0;
    uint16_t counterX = 0;
	uint16_t counterY = 0;
	int8_t incrY = +2;    
    
	hardware_init();
    initBackScroll();
    initScrollScreenNBG0();
    initScrollScreenNBG1();
    startDisplay();
        
	/* Main loop */
	for(;;)
	{
        /* Wait for next vblank */
        vdp2_tvmd_vblank_in_wait();
        
        // apply sinus on reflection
        uint16_t y;
        for (y = 0; y < 240-184; y++) 
        {
            line_scroll_tb[2*184+(2*y+0)]  = lut[(ofs + (10*y)) % 512] << 13;          // set horizontal offset
            line_scroll_tb[2*184+(2*y+1)]  = (184-(2*y)) << 16;                  // set vertical line value
        }
        ofs += 4;
        
        /*linescrollfmt.ls_scrn = SCRN_NBG0;
        linescrollfmt.ls_lsta = (uint32_t)line_scroll_tb;
        linescrollfmt.ls_int = 0;	              
        linescrollfmt.ls_fun =  SCRN_LS_N0SCX | SCRN_LS_N0SCY;	   // enable : SCRN_LS_N0SCX | SCRN_LS_N0SCY | N0LSS0 | N0LSS1 | N0LZMX       
        vdp2_scrn_ls_set(&linescrollfmt);        
        */
        
        vdp2_scrn_scv_y_set(SCRN_NBG0, lut[counterY] >> 3, 0);
        counterY += incrY;
        if(counterY >= 239) incrY=-2;            
        else if(counterY <= 0) incrY=2;           
        //vdp2_scrn_scv_x_set(SCRN_NBG1, counterX++, 0);
        if(counterX > 320) counterX = 0;           
        
        vdp2_tvmd_vblank_out_wait();
	}
    

	return 0;
}

static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
  	g_frame_counter = (tick > 0) ? (g_frame_counter + 1) : 0;
  	smpc_peripheral_digital_port(1, &g_digital);
}

static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
    // reset color offset function at EACH frame
    MEMORY_WRITE(16, VDP2(COAR), 0);
    MEMORY_WRITE(16, VDP2(COAB), 0);
    MEMORY_WRITE(16, VDP2(COAG), 0);  

 	tick = (tick & 0xFFFFFFFF) + 1;
}

static void scu_timer0(void)
{
        irq_mux_handle(&scu_timer0_irq_mux);
}

static void timer0_handler(irq_mux_handle_t *irq_mux __unused)
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

void init_vdp2_VRAM(void)
{
	uint32_t	loop;
	uint32_t	maxloop;
        
	loop = 0;
	maxloop = 512*1024;

	while (loop < maxloop)
	{
		*((uint32_t *) (VRAM_ADDR_4MBIT(0, 0) + loop)) = 0;

		loop += 4;
	}
}
