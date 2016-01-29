// rasters
// 
// Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>

#include "tables.h"

struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);


/* back Scroll table address */
static uint32_t *back_scroll_tb = (uint32_t *)VRAM_ADDR_4MBIT(4, 0x0);

static void hardware_init(void)
{
	/* VDP2 */
	vdp2_init();

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

	/* Enable interrupts */
	cpu_intc_enable();
}
                            
void initBackScroll(void)
{
    // wait vblank
    vdp2_tvmd_display_clear();
    
    /* set 320x240 res */
    uint16_t tvmd = MEMORY_READ(16, VDP2(TVMD));
    tvmd |= ((1 << 8) | (1 << 4));  // set BDCLMD to 1 and set VRES0 to 1
    
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);
    
    MEMORY_WRITE(16, VDP2(EXTEN), 0);
    MEMORY_WRITE(16, VDP2(BGON), 0);
    MEMORY_WRITE(16, VDP2(LCTAU), 0);
    MEMORY_WRITE(16, VDP2(LCTAL), 0);
    MEMORY_WRITE(16, VDP2(BKTAL), 0);
    MEMORY_WRITE(16, VDP2(BKTAU), 0);
        
    /* Copy rasters bars color data to VRAM */
    memcpy(back_scroll_tb, backscroll, sizeof(backscroll));   
    
    /* set BKTAL and BKTAU (Back screen table address register */
    MEMORY_WRITE(16, VDP2(BKTAL), (uint32_t)back_scroll_tb & 0x0FFFF);
    MEMORY_WRITE(16, VDP2(BKTAU), (0x8000 | ( ((uint32_t) back_scroll_tb >> 16) & 0x7)));    
}

int main(void)
{
	hardware_init();
    initBackScroll();

    //int i = 0;
	/* Main loop */
	for(;;)
	{
        /* Wait for next vblank */
		vdp2_tvmd_vblank_out_wait();

   		//MEMORY_WRITE(16, VDP2(BKTAL), 127 + lut[i++ % 512] );
        
        vdp2_tvmd_vblank_in_wait();
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
 	tick = (tick & 0xFFFFFFFF) + 1;
}
