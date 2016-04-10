/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdlib.h>
#include <fixmath.h>

#include "misery512.h"
#include "saturn16.h"
#include "curves.h"

#define N0LSS0 (1 << 4)
#define N0LSS1 (1 << 5) //enable 8 lines block - 00, 01 10 11 for 1,2, 4 or 8 lines based on non-interlaced settings
#define N0LZMX (1 << 3) // also enable linescroll in sync with zoom, scale horizontally per line units

#define N1LSS0 (1 << 12)
#define N1LSS1 (1 << 13) /
#define N1LZMX (1 << 11)

struct smpc_peripheral_digital g_digital;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

/* Line Scroll table address */
static uint32_t *line_scroll_tb = (uint32_t *)VRAM_ADDR_4MBIT(1, 0x1F000);
struct scrn_ls_format linescrollfmt_nbg0;
struct scrn_ls_format linescrollfmt_nbg1;

static void config_0(void);
static void config_1(void);

void init_all(void)
{
    irq_mux_t *vblank_in;
    irq_mux_t *vblank_out;

    vdp2_init();
    // enable zoom out to 1/4 as we are in 16 colors mode for NBG0 (2 first bits, bits 8-9 for NBG1), that disables NBG2 display
    MEMORY_WRITE(16, VDP2(ZMCTL), 0x03 << 8);       
    
    vdp1_init();
    smpc_init();
    smpc_peripheral_init();

    cpu_intc_disable();
    vblank_in = vdp2_tvmd_vblank_in_irq_get();
    irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
    vblank_out = vdp2_tvmd_vblank_out_irq_get();
    irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
    cpu_intc_enable();

    // copy line scroll table in VRAM
	uint16_t y;
	for (y = 0; y < 512; y++) 
	{
			line_scroll_tb[y] = (lut[y]) << 15;
			line_scroll_tb[y + 512] = (lut[y]) << 15;
	}

    config_1();
    config_0();
    
    vdp2_tvmd_display_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_256);
    vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0));  
}

int main(void)
{
    uint16_t curve_index ;    
    int8_t zoom_dir;
    fix16_t zooming_factor, zooming_factor_incr;    
    fix16_t logical_x_pos, logical_y_pos;
    fix16_t physical_x_pos, physical_y_pos;
    uint16_t ofs_nbg0 = 0;
    uint16_t ofs_nbg1 = 0;

    init_all();
    
    curve_index = 0;    
    zoom_dir = 1;  
    
    zooming_factor = fix16_from_int(0);
    zooming_factor_incr = fix16_from_float(0.01);  
    logical_x_pos = fix16_from_int(0);
    logical_y_pos = fix16_from_int(0);
    physical_x_pos = fix16_from_int(0);
    physical_y_pos = fix16_from_int(0);

    while (!g_digital.pressed.button.start) 
    {   
        vdp2_tvmd_vblank_in_wait();  /* VBL Begin, end of display */

        curve_index++;
        if(zoom_dir > 0)
        {
            zooming_factor = fix16_add(zooming_factor, zooming_factor_incr);
            if(fix16_to_float(zooming_factor) >= 7.0)
            {
                zooming_factor = fix16_from_float(7.0);
                zoom_dir = -1;
            }
        }
        else
        {
            zooming_factor = fix16_sub(zooming_factor, zooming_factor_incr);
            if(fix16_to_float(zooming_factor) <= 0.0) 
            {
                zooming_factor = fix16_from_int(0);
                zoom_dir = 1;
            }
        }    

        logical_x_pos = fix16_from_float(test_coord_x[curve_index % 1024]);
        physical_x_pos = fix16_mul(logical_x_pos, zooming_factor);
        
        logical_y_pos = fix16_from_float(test_coord_y[curve_index % 1024]);
        physical_y_pos = fix16_mul(logical_y_pos, zooming_factor);        
        
        // update registers in one shot as they are contigous 16in.16dn
        MEMORY_WRITE(32, VDP2(SCXIN1), ((uint32_t) physical_x_pos) & 0x7FFFF00);
        MEMORY_WRITE(32, VDP2(SCYIN1), ((uint32_t) physical_y_pos) & 0x7FFFF00);
        MEMORY_WRITE(32, VDP2(ZMXIN1), ((uint32_t) zooming_factor) & 0x7FF00);
        MEMORY_WRITE(32, VDP2(ZMYIN1), ((uint32_t) zooming_factor) & 0x7FF00);      
        
        // for background
        linescrollfmt_nbg0.ls_scrn = SCRN_NBG0;
        linescrollfmt_nbg0.ls_fun = SCRN_LS_N0SCX | SCRN_LS_N1SCX;  // | N0LSS0; // | N0LSS0 | N0LSS1; // | N0LZMX        
        linescrollfmt_nbg0.ls_lsta = (uint32_t)line_scroll_tb | (ofs_nbg0 & (2048 - 1));
        vdp2_scrn_ls_set(&linescrollfmt_nbg0);
        ofs_nbg0 += 20;
        
        linescrollfmt_nbg1.ls_scrn = SCRN_NBG1;
        linescrollfmt_nbg1.ls_fun = SCRN_LS_N1SCX | SCRN_LS_N0SCX;  // | N0LSS0; // | N0LSS0 | N0LSS1; // | N0LZMX         
        linescrollfmt_nbg1.ls_lsta = (uint32_t)line_scroll_tb | (ofs_nbg1 & (2048 - 1));
        vdp2_scrn_ls_set(&linescrollfmt_nbg1);        
        ofs_nbg1 += 16;   
        
        vdp2_tvmd_vblank_out_wait();  /* VBL End, beginning of display */
    }
    
    abort();
    
    return 0;
}

static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
    smpc_peripheral_digital_port(1, &g_digital);
}

static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{

}

static void config_0(void)
{
    struct scrn_cell_format format;
   
    uint16_t *color_palette;
    color_palette = (uint16_t *)CRAM_MODE_1_OFFSET(1, 0, 0);
    
    uint16_t *planes[4];
    planes[0] = (uint16_t *)VRAM_ADDR_4MBIT(3, 0x00000);
    planes[1] = (uint16_t *)VRAM_ADDR_4MBIT(3, 0x00000);
    planes[2] = (uint16_t *)VRAM_ADDR_4MBIT(3, 0x00000);
    planes[3] = (uint16_t *)VRAM_ADDR_4MBIT(3, 0x00000);
    
    uint32_t *cpd;
    cpd = (uint32_t *)VRAM_ADDR_4MBIT(3, 0x2000);

    format.scf_scroll_screen = SCRN_NBG0;
    format.scf_cc_count = SCRN_CCC_PALETTE_16;
    format.scf_character_size = 1 * 1;
    format.scf_pnd_size = 1; /* 1-word */
    format.scf_auxiliary_mode = 1;
    format.scf_plane_size = 1 * 1;
    format.scf_cp_table = (uint32_t)cpd;
    format.scf_color_palette = (uint32_t)color_palette;
    format.scf_map.plane_a = (uint32_t)planes[0];
    format.scf_map.plane_b = (uint32_t)planes[1];
    format.scf_map.plane_c = (uint32_t)planes[2];
    format.scf_map.plane_d = (uint32_t)planes[3];

    struct vram_ctl *vram_ctl;
    vram_ctl = vdp2_vram_control_get();

    vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;
	
	linescrollfmt_nbg0.ls_scrn = SCRN_NBG0;
	linescrollfmt_nbg0.ls_lsta = (uint32_t)line_scroll_tb;
	linescrollfmt_nbg0.ls_fun = SCRN_LS_N0SCX;  // | N0LSS0; // | N0LSS0 | N0LSS1; // | N0LZMX
	
	vdp2_scrn_ls_set(&linescrollfmt_nbg0);

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear(); {
        
        /* Copy the cell data */
        memcpy(cpd, saturn16_character_pattern, sizeof(saturn16_character_pattern));        
        
        /* Copy the palette data */
        memcpy(color_palette, saturn16_cell_palette, sizeof(saturn16_cell_palette));
        
        uint32_t i;
        uint16_t *nbg0_page0 = planes[0];

        for (i = 0; i < 2048; i++) 
        {
            nbg0_page0[i] = (VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)cpd) + saturn16_pattern_name_table_page_0[i]) | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette);
        }

        vdp2_vram_control_set(vram_ctl);

        vdp2_scrn_cell_format_set(&format);
        vdp2_priority_spn_set(SCRN_NBG0, 7);
        vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);
    }
}

static void config_1(void) 
{       
    struct scrn_cell_format format;
    uint8_t i;
    uint16_t *color_palette[16];
    
    for(i=0; i<16; i++) color_palette[i] = (uint16_t *)CRAM_MODE_1_OFFSET(0, i, 0);
    
    uint16_t *planes[4];
    planes[0] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0*0x08000);
    planes[1] = (uint16_t *)VRAM_ADDR_4MBIT(0, 1*0x08000);
    planes[2] = (uint16_t *)VRAM_ADDR_4MBIT(0, 2*0x08000);
    planes[3] = (uint16_t *)VRAM_ADDR_4MBIT(0, 3*0x08000);
    
    uint32_t *cpd;
    cpd = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x00000);

    format.scf_scroll_screen = SCRN_NBG1;
    format.scf_cc_count = SCRN_CCC_PALETTE_16;
    format.scf_character_size = 1 * 1;
    format.scf_pnd_size = 1; /* 1-word */
    format.scf_auxiliary_mode = 1;
    format.scf_plane_size = 2 * 2;
    format.scf_cp_table = (uint32_t)cpd;
    format.scf_color_palette = (uint32_t)color_palette[0];
    format.scf_map.plane_a = (uint32_t)planes[0];
    format.scf_map.plane_b = (uint32_t)planes[1];
    format.scf_map.plane_c = (uint32_t)planes[2];
    format.scf_map.plane_d = (uint32_t)planes[3];

    struct vram_ctl *vram_ctl;
    vram_ctl = vdp2_vram_control_get();

    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;

	linescrollfmt_nbg1.ls_scrn = SCRN_NBG1;
	linescrollfmt_nbg1.ls_lsta = (uint32_t)line_scroll_tb;
	linescrollfmt_nbg1.ls_fun = SCRN_LS_N1SCX; // | N0LZMX ;  // | N0LSS0; // | N0LSS0 | N0LSS1; // | N0LZMX
    vdp2_scrn_ls_set(&linescrollfmt_nbg1);

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear(); {
        
        /* Copy the cell data */
        memcpy(cpd, misery512_character_pattern, sizeof(misery512_character_pattern));        
        
        /* Copy the palette data */
        memcpy(color_palette[0], misery512_cell_palette, sizeof(misery512_cell_palette));
    
        uint32_t page_width;
        page_width = SCRN_CALCULATE_PAGE_WIDTH(&format);
        uint32_t page_height;
        page_height = SCRN_CALCULATE_PAGE_HEIGHT(&format);
        uint32_t page_size;
        page_size = SCRN_CALCULATE_PAGE_SIZE(&format);

        uint16_t *a_pages[4];
        a_pages[0] = &planes[0][0];
        a_pages[1] = &planes[0][1 * (page_size / 2)];
        a_pages[2] = &planes[0][2 * (page_size / 2)];
        a_pages[3] = &planes[0][3 * (page_size / 2)];

        uint16_t *b_pages[4];
        b_pages[0] = &planes[1][0];
        b_pages[1] = &planes[1][1 * (page_size / 2)];
        b_pages[2] = &planes[1][2 * (page_size / 2)];
        b_pages[3] = &planes[1][3 * (page_size / 2)];

        uint16_t *c_pages[4];
        c_pages[0] = &planes[2][0];
        c_pages[1] = &planes[2][1 * (page_size / 2)];
        c_pages[2] = &planes[2][2 * (page_size / 2)];
        c_pages[3] = &planes[2][3 * (page_size / 2)];

        uint16_t *d_pages[4];
        d_pages[0] = &planes[3][0];
        d_pages[1] = &planes[3][1 * (page_size / 2)];
        d_pages[2] = &planes[3][2 * (page_size / 2)];
        d_pages[3] = &planes[3][3 * (page_size / 2)];

        
        uint32_t page_x;
        uint32_t page_y;
        for (page_y = 0; page_y < page_height; page_y++) 
        {
            for (page_x = 0; page_x < page_width; page_x++) 
            {
                uint16_t page_idx;
                page_idx = page_x + (page_width * page_y);

                uint16_t pnd;
                pnd = (VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)cpd) + misery512_pattern_name_table_page_0[page_idx]);

                a_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[0]);
                a_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[0]);
                a_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[1]);
                a_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[1]);

                b_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[1]);
                b_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[1]);
                b_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[2]);
                b_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[2]);

                c_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[2]);
                c_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[2]);
                c_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[3]);
                c_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[3]);
                
                d_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[3]);
                d_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[3]);
                d_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[4]);
                d_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette[4]);
            }
        }

        vdp2_vram_control_set(vram_ctl);

        vdp2_scrn_cell_format_set(&format);
        vdp2_priority_spn_set(SCRN_NBG1, 6);
        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ false);
    }
}

