#include <yaul.h>
#include <stdlib.h>
#include <inttypes.h>
#include <langam/dma_wrapper.h>
#include <langam/sequencer.h>

#include "scenes.h"


struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;
static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);
void init_vdp2_VRAM(void);

irq_mux_t *vblank_in;
irq_mux_t *vblank_out;

static void hardware_init(void)
{
	/* VDP2 */
	vdp2_init();
    init_vdp2_VRAM();
    
    /* set 320x240 res, back color mode */
    vdp2_tvmd_display_clear();
    uint16_t tvmd = MEMORY_READ(16, VDP2(TVMD));
    tvmd |= ((1 << 8) | (1 << 4));              // set BDCLMD,  VRES0 to 1
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);    
    
    // Set Color mode to mode 0 (2KWord Color RAM)
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);        

	/* VDP1 */
	vdp1_init();

	/* SMPC */
	smpc_init();
	smpc_peripheral_init();
	
	scu_dma_init();

	/* Disable interrupts */
	cpu_intc_disable();

	vblank_in = vdp2_tvmd_vblank_in_irq_get();
	irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

	vblank_out = vdp2_tvmd_vblank_out_irq_get();
	irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);

	/* Enable interrupts */
	cpu_intc_enable();
    
    /* Set screen to black */
    static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);
    //static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	//vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);    
}


int main(void)
{
		hardware_init();

        sequencer_initialize();

        sequencer_register("intro", 80, intro_init, intro_update, intro_draw, intro_exit);
        sequencer_register("reflection", 90000, reflection_init, reflection_update, reflection_draw, reflection_exit);

        sequencer_start();

        while(sequencer_isStarted()) 
        {
                vdp2_tvmd_vblank_in_wait();         /* VBL Begin, end of display */

                sequencer_update(g_frame_counter);  /* update stuff, better during display ? */
                sequencer_draw();                   /* draw stuff, better during VBL ? */
                
                vdp2_tvmd_vblank_out_wait();        /* VBL End, beginning of display */
                
                if (g_digital.connected == 1)
                {
                     if(g_digital.pressed.button.start) break;
                }          
        }
        
        sequencer_exit();

        return 0;
}

static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
    if(sequencer_isStarted())
    {
        g_frame_counter = (tick > 0) ? (g_frame_counter + 1) : 0;
        smpc_peripheral_digital_port(1, &g_digital);
    }
}

/*
 * vblank_in_handler
 *
 * VBL beginning of display interrupt handler
 *
 */
static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
 	tick = (tick & 0xFFFFFFFF) + 1;
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
    