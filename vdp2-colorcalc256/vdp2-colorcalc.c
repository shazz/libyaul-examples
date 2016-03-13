/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaul.h>
#include <langam.h>

#include "blurry_256.h"
#include "blurry_head_256.h"

#define PLANE0_PNT  blurry_256_pattern_name_table
#define PLANE0_CD   blurry_256_cell_data
#define PLANE0_CP   blurry_256_cell_palette

#define PLANE2_PNT  blurry_head_256_pattern_name_table
#define PLANE2_CD   blurry_head_256_cell_data
#define PLANE2_CP   blurry_head_256_cell_palette


struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

uint8_t g_cc_NBG0, g_cc_NBG2;
uint8_t g_ecc_NBG0;

#define NBG0_VRAM_PAL_NB 2
#define NBG2_VRAM_PAL_NB 1

static uint16_t *_nbg0_planes[4] = {
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0)
}; // a plane needs 2048 u16 (512x256) or 0x1000 bytes (4096)

static uint16_t *_nbg2_planes[4] = {
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0)
}; // a plane needs 2048 u16 (512x256) or 0x1000 bytes (4096)

/* VRAM A0 after plane  */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x1000);
static uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(0, 0x1000));

/* VRAM B0 after plane */
static uint32_t *_nbg2_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x1000);
static uint16_t _nbg2_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(2, 0x1000));

/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(NBG0_VRAM_PAL_NB, 0, 0);
static uint32_t *_nbg2_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(NBG2_VRAM_PAL_NB, 0, 0);
static uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_0_OFFSET(NBG0_VRAM_PAL_NB, 0, 0));
static uint16_t _nbg2_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_0_OFFSET(NBG2_VRAM_PAL_NB, 0, 0));

static unsigned int joyLeft = 0, joyRight = 0, joyUp = 0, joyDown = 0;    
static unsigned int joyA= 0, joyB = 0, joyL = 0, joyR = 0;    
static unsigned int joyX= 0, joyY = 0; //, joyZ = 0;  
    
/*
 *  void init_scrollscreen_nbg0(void)
 */
void init_scrollscreen_nbg0(void)
{
    struct scrn_cell_format nbg0_format;
    
	nbg0_format.scf_scroll_screen = SCRN_NBG0;
	nbg0_format.scf_cc_count = SCRN_CCC_PALETTE_256;
	nbg0_format.scf_character_size= 1 * 1;
	nbg0_format.scf_pnd_size = 1; /* 1 word */
	nbg0_format.scf_auxiliary_mode = 1;
	nbg0_format.scf_cp_table = (uint32_t)_nbg0_cell_data;
	nbg0_format.scf_color_palette = (uint32_t)_nbg0_color_palette;
	nbg0_format.scf_plane_size = 1 * 1;
	nbg0_format.scf_map.plane_a = (uint32_t)_nbg0_planes[0];
	nbg0_format.scf_map.plane_b = (uint32_t)_nbg0_planes[1];
	nbg0_format.scf_map.plane_c = (uint32_t)_nbg0_planes[2];
	nbg0_format.scf_map.plane_d = (uint32_t)_nbg0_planes[3];

	vdp2_scrn_cell_format_set(&nbg0_format);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg0_page0 = _nbg0_planes[0];

	for (i = 0; i < 2048; i++) {
			uint16_t cell_data_number = _nbg0_cell_data_number + PLANE0_PNT[i];
			nbg0_page0[i] = cell_data_number | _nbg0_palette_number;
	}

	vdp2_priority_spn_set(SCRN_NBG0, 7);
    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);    
}

/*
 *  void init_scrollscreen_nbg2(void)
 */
void init_scrollscreen_nbg2(void)
{
	/* NBG2 stuff is located in B0 bank */

    /* set NBG2 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg2_format;

	nbg2_format.scf_scroll_screen = SCRN_NBG2;
	nbg2_format.scf_cc_count = SCRN_CCC_PALETTE_256;
	nbg2_format.scf_character_size= 1 * 1;
	nbg2_format.scf_pnd_size = 1; /* 1 word */
	nbg2_format.scf_auxiliary_mode = 1;
	nbg2_format.scf_cp_table = (uint32_t)_nbg2_cell_data;
	nbg2_format.scf_color_palette = (uint32_t)_nbg2_color_palette;
	nbg2_format.scf_plane_size = 1 * 1;
	nbg2_format.scf_map.plane_a = (uint32_t)_nbg2_planes[0];
	nbg2_format.scf_map.plane_b = (uint32_t)_nbg2_planes[1];
	nbg2_format.scf_map.plane_c = (uint32_t)_nbg2_planes[2];
	nbg2_format.scf_map.plane_d = (uint32_t)_nbg2_planes[3];

	vdp2_scrn_cell_format_set(&nbg2_format);    

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg2_page0 = _nbg2_planes[0];

	for (i = 0; i < 2048; i++) {
			uint16_t cell_data_number = _nbg2_cell_data_number + PLANE2_PNT[i];
			nbg2_page0[i] = cell_data_number | _nbg2_palette_number;
	}

	vdp2_priority_spn_set(SCRN_NBG2, 6);
    vdp2_scrn_display_set(SCRN_NBG2, /* transparent = */ false);
}


/*
 *  void set_VRAM_access(void)
 *  Set VRAM access priorities
 */
void set_VRAM_access(void)
{
    struct vram_ctl *vram_ctl;
    
    // 16 colors bitmap requires 1 access each
    vram_ctl = vdp2_vram_control_get();
	
    // Rules:
    // only 2 PNDR per timing and split accros x0 and x1 banks
    // CHPNDR cannot be acceded during certain cycles after PNDR
    
	// Bank A0, 256 colors requires 2 access each
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_PNDR_NBG0; 	// NBG0 character pattern or bitmap data read
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0; 
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0; 
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

	// Bank A1
    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	// NBG1 character pattern or bitmap data read
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;     // NBG1 pattern name data read
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
	
	// Bank B0
    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	// NBG2 character pattern data read
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_NO_ACCESS; 	// NBG2 pattern name data read
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_PNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_CHPNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_CHPNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;	
    
	// Bank B1
    vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;	    
	
    vdp2_vram_control_set(vram_ctl);   
}

/*
 * Public Functions 
 * 
 */
void init(void)
{           
    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();    
        
    // Set Color mode to mode 0 (1KWord Color RAM), 2 banks
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);     
    
	// Enable color cal on NBG0 and NB2 only
    MEMORY_WRITE(16, VDP2(CCCTL), 0);    
    MEMORY_WRITE(16, VDP2(CCCTL), 0x5 /* (1 << 2) | (1 << 0)*/);    

    MEMORY_WRITE(16, VDP2(CCRNA), 0x0);
    MEMORY_WRITE(16, VDP2(CCRNB), 0x1F);
    
    /* DMA Indirect list, aligned on 64 bytes due to more than 24bytes size (6*4*3=72) */
    uint32_t dma_tbl[] __attribute__((aligned(64))) = { 
            (uint32_t)sizeof(PLANE0_CD), (uint32_t)_nbg0_cell_data, (uint32_t)PLANE0_CD, 
            (uint32_t)sizeof(PLANE0_CP), (uint32_t)_nbg0_color_palette, (uint32_t)PLANE0_CP, 
            (uint32_t)sizeof(PLANE2_CD), (uint32_t)_nbg2_cell_data, (uint32_t)PLANE2_CD, 
            (uint32_t)sizeof(PLANE2_CP), (uint32_t)_nbg2_color_palette, (uint32_t)PLANE2_CP                      
    };    
    scu_dma_listcpy(dma_tbl, 4*3);
    while(scu_dma_get_status(SCU_DMA_ALL_CH) == SCU_DMA_STATUS_WAIT);
    
    /* set all other stuff */ 
    init_scrollscreen_nbg0();
	init_scrollscreen_nbg2();
	set_VRAM_access();
    
    g_cc_NBG0 = 0x0;
    g_cc_NBG2 = 0x1F;
    
}

void read_digital_pad(void)
{
	if (g_digital.connected == 1)
	{
		joyUp = g_digital.pressed.button.up;
		joyDown = g_digital.pressed.button.down;
		joyRight = g_digital.pressed.button.right;                            
		joyLeft = g_digital.pressed.button.left;
        joyA = g_digital.released.button.a;
        joyB = g_digital.released.button.b;
        joyL = g_digital.released.button.l;
        joyR = g_digital.released.button.r;    
        joyX = g_digital.released.button.x;  
        joyY = g_digital.released.button.y;     

		if (joyDown)
		{
				if(g_cc_NBG2 < 0x1F) g_cc_NBG2++;
		}
		else if (joyUp)
		{
				if(g_cc_NBG2 > 0x0) g_cc_NBG2--;
		}
		else if (joyRight)
		{
				if(g_cc_NBG0 > 0x0) g_cc_NBG0--;
		}
		else if (joyLeft)
		{
				if(g_cc_NBG0 < 0x1F) g_cc_NBG0++;    				
		}
  		else if (joyA)
		{
            MEMORY_WRITE(16, VDP2(CCCTL), 0x0);    
            MEMORY_WRITE(16, VDP2(CCCTL), (1 << 15) | (1 << 14) | (1 << 12) | 0x5);    
		} 
   		else if (joyB)
		{
            MEMORY_WRITE(16, VDP2(CCCTL), 0x0);    
            MEMORY_WRITE(16, VDP2(CCCTL), 0x5);    
		}   
   		else if (joyL)
		{
            MEMORY_WRITE(16, VDP2(CCCTL), 0x0);    
            MEMORY_WRITE(16, VDP2(CCCTL), (1 << 10) | 0x5);    
            
            g_ecc_NBG0 = (g_ecc_NBG0 + 1) & 0x3;
            MEMORY_WRITE(16, VDP2(SFCCMD), g_ecc_NBG0);  
		}    
        else if (joyR)
		{
            MEMORY_WRITE(16, VDP2(CCCTL), 0x0);    
            MEMORY_WRITE(16, VDP2(CCCTL), 0x5);    
		}
        else if (joyX)
		{
            uint16_t reg = MEMORY_READ(16, VDP2(CCCTL));    
            MEMORY_WRITE(16, VDP2(CCCTL), reg | (1 << 9));    
		}  
         else if (joyY)
		{
            uint16_t reg = MEMORY_READ(16, VDP2(CCCTL));    
            MEMORY_WRITE(16, VDP2(CCCTL), reg & (0xFDFF));    
		}   
		
		// exit
		if(g_digital.pressed.button.start) abort();		

		
		MEMORY_WRITE(16, VDP2(CCRNA), g_cc_NBG0 & 0x1F);
		MEMORY_WRITE(16, VDP2(CCRNB), g_cc_NBG2 & 0x1F);    
	}  
        
}

int main(void)
{
        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;
        
        static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };

        vdp2_init();
		
        vdp1_init();

        smpc_init();
		
        smpc_peripheral_init();
        
        scu_dma_cpu_init();

        cpu_intc_disable();
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
        cpu_intc_enable();

		init();

        vdp2_tvmd_display_set(); 
        
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);    

        while (true) 
        {
                vdp2_tvmd_vblank_in_wait();
            
				read_digital_pad();
                
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
    tick = (tick & 0xFFFFFFFF) + 1;
}