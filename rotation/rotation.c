/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz <shazz@trsi.de>
 */

#include <stdlib.h>

#include <yaul.h>
#include <langam.h>

#include "background.h"

extern uint8_t     oneshot;

#define RGB888_TO_RGB555(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5) | ((r) >> 3))

#define	SCL_X_AXIS		1
#define	SCL_Y_AXIS		2

static char * consbuf;

/* CRAM */
#define RBG0_VRAM_PAL_OFFSET 0
//static uint32_t *_rbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(RBG0_VRAM_PAL_OFFSET, 0, 0);

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

static void hardware_init(void)
{
	/* VDP2 */
	vdp2_init();

	/* VDP1 */
	vdp1_init();

	/* SMPC */
	smpc_init();
	smpc_peripheral_init();
    
    cons_init(CONS_DRIVER_VDP2);  
    consbuf = (char *)malloc(1024); 

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

static void init_screen_RBG0(void)
{
	// init rotation parameters, will be copied at vbl, mode 0
	//initRotateTable(SCL_VDP2_VRAM_B1+0x10000,2, RBG0, RBG1);
    uint32_t adr = vdp2_rbg_initRotateTable(VRAM_ADDR_4MBIT(3, 0x0)+0x10000, 1, RBG0, NON);	
    
    (void)sprintf(consbuf, "[01;2HAddress: %08lx", adr);
    cons_buffer(consbuf);     
	
	/*
	// set RBG0 parameters
	SCL_InitVramConfigTb(&tp);
	tp.vramModeA  = ON;		
	tp.vramModeB  = ON;		
	tp.vramA0     = SCL_RBG0_CHAR;	
	tp.vramA1     = SCL_RBG0_K;	
	SCL_SetVramConfig(&tp);
    
	SCL_InitConfigTb(&Rbg0Scfg);
	Rbg0Scfg.dispenbl = ON;
	Rbg0Scfg.coltype  = SCL_COL_TYPE_256;
	Rbg0Scfg.datatype = SCL_BITMAP;
	for(i=0;i<MAP_NUM;i++) Rbg0Scfg.plate_addr[i] = SCL_VDP2_VRAM_A0;
	SCL_SetConfig(SCL_RBG0, &Rbg0Scfg);
	*/

    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set RGB0 in bitmap mode, 16 col, 512x256 */
    #ifdef DOIT
    struct scrn_bitmap_format rbg0_format;
    //struct vram_ctl *vram_ctl;

    rbg0_format.sbf_scroll_screen 		= SCRN_RBG0; 						/* Normal/rotational background */
    rbg0_format.sbf_cc_count 			= SCRN_CCC_PALETTE_16; 				/* color mode */
    rbg0_format.sbf_bitmap_size.width 	= 512; 								/* Bitmap sizes: 512x256, 512x512, 1024x256, 1024x512 */
    rbg0_format.sbf_bitmap_size.height 	= 256;
    rbg0_format.sbf_bitmap_pattern 		= VRAM_ADDR_4MBIT(0, 0x0); 		/* Bitmap pattern lead address */
    rbg0_format.sbf_color_palette 		= (uint32_t)_rbg0_color_palette;
    rbg0_format.sbf_rp_mode				= 0; 								/*  Rotation parameter mode:   Mode 0: Rotation Parameter A, Mode 1: Rotation Parameter B, Mode 2: Swap Coefficient Data Read, Mode 3: Swap via Rotation Parameter Window */
    #endif
    
	// check BGON is set for RGB0 -> R0-TP-ON + R0-ON
    //vdp2_scrn_bitmap_format_set(&rbg0_format);

	// check RAM CTL is set for registers RDBSA00-*B11 -> 1 0011 0000 0111 set RGB0 bitmap on A0 and end table on A1 (!!!!)
    //MEMORY_WRITE(16, VDP2(RAMCTL), 0x1307);

    /*vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0; // needed ?
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);*/

	// copy bitmap in VRAM
    //memcpy((void *)VRAM_ADDR_4MBIT(0, 0x0), background_data, sizeof(background_data));

    /* Copy the BGR555 palette data */
    //memcpy(_rbg0_color_palette, background_palette, sizeof(background_palette));


 	// check BGON is set for RGB0 -> R0-TP-ON + R0-ON
	//MEMORY_WRITE(16, VDP2(BGON), 0x1010); //set R0ON(4) and T0TPON(12) (0001 0000 0001 0000)
	//vdp2_priority_spn_set(SCRN_RBG0, 7);
	//vdp2_scrn_display_set(SCRN_RBG0, /* transparent = */ false);
    vdp2_tvmd_display_set();

	// Set start position
	vdp2_rbg_moveTo(RBG_TB_A, FIXED(0), FIXED(0), FIXED(0));
	vdp2_rbg_scale(RBG_TB_A, FIXED(1.0), FIXED(1.0));
}


static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
  	g_frame_counter = (tick > 0) ? (g_frame_counter + 1) : 0;
  	smpc_peripheral_digital_port(1, &g_digital);

	// to be called only at VBL interrupt
  	vdp2_rbg_copyReg();
}

static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
 	tick = (tick & 0xFFFFFFFF) + 1;
}


int main(void)
{
	//fix32_t r;

	hardware_init();
	init_screen_RBG0();
    oneshot = 0;
    
	//vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE), RGB888_TO_RGB555(0, 0, 0));

	//r = FIXED(2);
	while (true)
	{
		vdp2_tvmd_vblank_in_wait();
        
        cons_flush();
        
		vdp2_tvmd_vblank_out_wait();

		
        //vdp2_rbg_rotate(RBG_TB_A, 0,0,r);

		/*if(PadData1 || PadData1EW)
		{
			if(PadData1 & PAD_U)
				SCL_Move( 0, r, 0);
			else if(PadData1 & PAD_D)
				SCL_Move( 0,-r, 0);
			if(PadData1 & PAD_R)
				SCL_Move(-r, 0, 0);
			else if(PadData1 & PAD_L)
				SCL_Move( r, 0, 0);
			if(PadData1 & PAD_RB)
				SCL_Move( 0, 0,-r);
			else if(PadData1 & PAD_LB)
				SCL_Move( 0, 0, r);
			if((PadData1 & PAD_Z))
				SCL_Rotate(0,r,0);
			if((PadData1 & PAD_Y))
				SCL_Rotate(0,0,r);
			if((PadData1 & PAD_X))
				SCL_Rotate(r,0,0);
			if((PadData1 & PAD_S))
			{
				SCL_MoveTo(0,0,0);
				SCL_RotateTo(0,0,0,SCL_X_AXIS);
			}
		}
        */
                        // exit
        if(g_digital.pressed.button.start) abort();
	}

	return 0;
}



