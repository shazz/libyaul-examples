/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaul.h>
#include <langam/tim_frt_wrapper.h>

static char * consbuf;

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;
volatile uint32_t g_dma_counter = 0;
static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

void start_test(void)
{
	uint32_t i;
	for(i=0; i<500000; i++);
}

int main(void)
{
        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;
		
		uint16_t counter = 0; 		/* store fetched counter value here */
		float32_t micro_sec = 0; 	/* store microseconds here */
		
        unsigned int joyLeft = 0, oldjoyLeft = 0, joyRight = 0, oldjoyRight = 0, joyUp = 0, oldjoyUp = 0, joyDown = 0, oldjoyDown = 0;    

        static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 7, 7) };

        vdp2_init();
        vdp1_init();

        smpc_init();
        smpc_peripheral_init();
        
        cons_init(CONS_DRIVER_VDP2);  
        consbuf = (char *)malloc(1024); 
		
        cpu_intc_disable();
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);    
 
		/* Set FRT period to 8 */
        cpu_intc_enable();		
		
		tim_frt_init(TIM_CKS_8);

        vdp2_tvmd_display_set(); 
        
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);    

        while (true) 
        {
            
                (void)sprintf(  consbuf, "[01;2H**** CPU FRT Tests ****[04;2HPress Start to quit[05;2HTo configure press:[07;2HUp to reset counter[08;2HDown to start test[11;2HResults Value:%08u => %f us", 
					counter, micro_sec);
                cons_buffer(consbuf);               
            
                vdp2_tvmd_vblank_in_wait();
            
                cons_flush();
                
                vdp2_tvmd_vblank_out_wait();
        		
				if (g_digital.connected == 1)
				{
					joyUp = g_digital.pressed.button.up;
					joyDown = g_digital.pressed.button.down;
					joyRight = g_digital.pressed.button.right;                            
					joyLeft = g_digital.pressed.button.left;
					
			   
					if (joyUp & !oldjoyUp)
					{
						
						counter = 0; 								/* reset counter */
					}
					else if (joyDown & !oldjoyDown)
					{
						tim_frt_set(0); 							/* set counter value to 0 */
						start_test(); 								/* execute process to be timed */
						counter = tim_frt_get(); 					/* get counter value */
						micro_sec = tim_frt_ticks_to_us(counter); 	/* convert counter value to microseconds */
					}
					else if (joyLeft & !oldjoyLeft)
					{
						
						
					}	  
					else if (joyRight & !oldjoyRight)
					{
						
					}		   


					oldjoyUp = joyUp;
					oldjoyDown = joyDown;
					oldjoyLeft = joyLeft;
					oldjoyRight = joyRight; 
				}                  
                
                
                // exit
                if(g_digital.pressed.button.start) abort();
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
