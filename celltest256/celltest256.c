#include <yaul.h>
#include <stdlib.h>

#include "test256.h"

/*
 * VDP2 VRAM Organization
 * Bank A0
 * - NBG0 plane data, from 0x0 to 0x01000 (4096 bytes, 1 word per cell, 512x256px)
 * - NBG0 cell pattern data, from 0x00000 to 0x0DD80 (56704 bytes, 886 8x8 cells in 8bits color)
 * => total : 56704 bytes instead of 131072 bytes in bitmap mode which fits in 128KB bank
 * 
 * Bank A1
 * - Nothing
 *  
 * Bank B0
 * - Nothing
 * 
 * Bank B1
 * - Nothing
 * 
 * CRAM
 * - NBG0 palette 256 set at palette 0 (0x0)
 * 
 * Registers:
 * - TVMD: enable display, border color mode to back screen, 224 lines vertical resolution
*/ 

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

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

/* VRAM B0 after plane */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x1000);
static uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(0, 0x1000));

/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);
static uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));

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

void init_scrollscreen_nbg0(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG0 in cell mode, 256 col, 1x1 */
    struct scrn_cell_format nbg0_format;
    struct vram_ctl *vram_ctl;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

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
    vdp2_priority_spn_set(SCRN_NBG0, 7);

    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;
      
    vdp2_vram_control_set(vram_ctl);

    /* Copy the palette data */
    memcpy(_nbg0_color_palette, test256_cell_palette, sizeof(test256_cell_palette));

    /* Copy the cell data */
    memcpy(_nbg0_cell_data, test256_cell_data, sizeof(test256_cell_data));

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg0_page0 = _nbg0_planes[0];

	for (i = 0; i < 2048; i++) 
    {
        uint16_t cell_data_number = _nbg0_cell_data_number + test256_pattern_name_table[i];
        nbg0_page0[i] = cell_data_number | _nbg0_palette_number;
	}

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);
}

int main(void)
{
	hardware_init();
    init_scrollscreen_nbg0();
    
    vdp2_tvmd_display_set(); 

	static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(5, 5, 5) };
	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);  

	/* Main loop */
	while (true) 
	{
	  	vdp2_tvmd_vblank_in_wait();

        if (g_digital.connected == 1)
        {
            static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(2, 2, 2) };
            vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);              
            // exit
            if(g_digital.pressed.button.start) abort();		
        }
        
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