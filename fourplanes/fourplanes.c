/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdlib.h>

#include "misery512.h"
#include "saturn16.h"

static struct smpc_peripheral_digital digital_pad;
static uint32_t tick = 0;

static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

static void hardware_init(void);

static void config_0(void);
static void config_1(void);

static uint16_t x = 0;
static uint16_t y = 0;

static uint16_t g_zooming_factor_dn = 0;
static uint16_t g_zooming_factor_in = 1;

void vdp2_scrn_zm_x_set(uint8_t scrn, uint16_t in, uint16_t dn);
void vdp2_scrn_zm_y_set(uint8_t scrn, uint16_t in, uint16_t dn);

int
main(void)
{
    hardware_init();

    config_1();
    config_0();

    while (true) 
    {
        vdp2_tvmd_vblank_out_wait();

        if (digital_pad.connected == 1) 
        {
            if (digital_pad.pressed.button.left) 
            {
                x -= 4;
            } 
            else if (digital_pad.pressed.button.right) 
            {
                x += 4;
            }

            if (digital_pad.pressed.button.up) 
            {
                y -= 4;
            } 
            else if (digital_pad.pressed.button.down) 
            {
                y += 4;
            }
            
            if (digital_pad.pressed.button.l)
            {
                if(g_zooming_factor_dn < 0xFF-4) g_zooming_factor_dn += 4; 
                else
                {
                    if(g_zooming_factor_in < 0x7)
                    {
                        g_zooming_factor_in++;    
                        g_zooming_factor_dn = 0;
                    }
                }
            }
            else if (digital_pad.pressed.button.r)
            {
                if(g_zooming_factor_dn > 0x0+4) g_zooming_factor_dn -= 4; 
                else
                {
                    if(g_zooming_factor_in > 0x0)
                    {
                        g_zooming_factor_in--;
                        g_zooming_factor_dn = 0xFF;
                    }                       
                }            
            }            
            
            if(digital_pad.pressed.button.start) abort();		
        }

        vdp2_tvmd_vblank_in_wait();
        vdp2_scrn_scv_x_set(SCRN_NBG1, x, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG1, y, 0);
        vdp2_scrn_zm_x_set(SCRN_NBG1, g_zooming_factor_in, g_zooming_factor_dn);
        vdp2_scrn_zm_y_set(SCRN_NBG1, g_zooming_factor_in, g_zooming_factor_dn);        
    }
}

static void
hardware_init(void)
{
    /* VDP1 */
    vdp1_init();

    /* VDP2 */
    vdp2_init();
    vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE), 0x9C00);
    
	// enable zoom out to 1/4 as we are in 16 colors mode for NBG0 (2 first bits, bits 8-9 for NBG1), that disables NBG2 display
	MEMORY_WRITE(16, VDP2(ZMCTL), 0x03 << 8);        

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

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
    smpc_peripheral_digital_port(1, &digital_pad);
}

static void
vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
    if ((vdp2_tvmd_vcount_get()) == 0) {
        tick = (tick & 0xFFFFFFFF) + 1;
    }
}

static void
config_0(void)
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
    } vdp2_tvmd_display_set();
}

static void
config_1(void)
{
    struct scrn_cell_format format;
   
    uint16_t *color_palette0 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 0, 0);
    uint16_t *color_palette1 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 1, 0);
    uint16_t *color_palette2 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 2, 0);
    uint16_t *color_palette3 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 3, 0);
    uint16_t *color_palette4 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 4, 0);
    uint16_t *color_palette5 = (uint16_t *)CRAM_MODE_1_OFFSET(0, 5, 0);
    
    uint16_t *planes[4];
    planes[0] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x08000);
    planes[1] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x10000);
    planes[2] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x18000);
    planes[3] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x20000);
    
    uint32_t *cpd;
    cpd = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x00000);
    //uint16_t cpd_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(0, 0x0));

    format.scf_scroll_screen = SCRN_NBG1;
    format.scf_cc_count = SCRN_CCC_PALETTE_16;
    format.scf_character_size = 1 * 1;
    format.scf_pnd_size = 1; /* 1-word */
    format.scf_auxiliary_mode = 1;
    format.scf_plane_size = 2 * 2;
    format.scf_cp_table = (uint32_t)cpd;
    format.scf_color_palette = (uint32_t)color_palette0;
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
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear(); {
        
        /* Copy the cell data */
        memcpy(cpd, misery512_character_pattern, sizeof(misery512_character_pattern));        
        
        /* Copy the palette data */
        memcpy(color_palette0, misery512_cell_palette0, sizeof(misery512_cell_palette0));
        memcpy(color_palette1, misery512_cell_palette1, sizeof(misery512_cell_palette1));
        memcpy(color_palette2, misery512_cell_palette2, sizeof(misery512_cell_palette2));
        memcpy(color_palette3, misery512_cell_palette3, sizeof(misery512_cell_palette3));
        memcpy(color_palette4, misery512_cell_palette4, sizeof(misery512_cell_palette4));
        memcpy(color_palette5, misery512_cell_palette5, sizeof(misery512_cell_palette5));
             
        
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
        for (page_y = 0; page_y < page_height; page_y++) {
            for (page_x = 0; page_x < page_width; page_x++) {
                uint16_t page_idx;
                page_idx = page_x + (page_width * page_y);

                uint16_t pnd;
                pnd = (VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)cpd) + misery512_pattern_name_table_page_0[page_idx]);

                a_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette0);
                a_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette1);
                a_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette2);
                a_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette3);

                b_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette4);
                b_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette5);
                b_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette0);
                b_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette1);

                c_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette2);
                c_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette3);
                c_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette4);
                c_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette5);
                
                d_pages[0][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette0);
                d_pages[1][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette1);
                d_pages[2][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette2);
                d_pages[3][page_idx] = pnd | VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)color_palette3);
            }
        }

        vdp2_vram_control_set(vram_ctl);

        vdp2_scrn_cell_format_set(&format);
        vdp2_priority_spn_set(SCRN_NBG1, 6);
        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ false);
    } vdp2_tvmd_display_set();
}

/*
 * The coord increment should be a value smaller then 1 to zoon in 
 * and larger than 1 to zoom out
 * no zoom means equal to 1 
 * 
 * Only NBG0 and NBG1 can be zoomed
 * can be changed during horizontal retrace
 * max zoom out is set to 1/4 means value = 4
 * max zoom out is constrainted by bitmap color depth: 1/4 in 16 colors mode, 1/2 in 16/256 colors mode
 */
void vdp2_scrn_zm_x_set(uint8_t scrn, uint16_t in, uint16_t dn)
{
#define ZMXIN0          0x0078
#define ZMXDN0          0x007A
#define ZMXIN1          0x0088
#define ZMXDN1          0x008A
        /*  integer part rounded to 3 bits*/
        in &= 0x07; 

        switch (scrn) {
        case SCRN_NBG0:
				/*  frac part rounded to 8 bits, shifted left by 8 */
                dn &= 0xFF; // 
                dn <<= 8;

                /* Write to memory */
                MEMORY_WRITE(16, VDP2(ZMXIN0), in);
                MEMORY_WRITE(16, VDP2(ZMXN0D), dn);
                break;
        case SCRN_NBG1:
                dn &= 0xFF;
                dn <<= 8;

                /* Write to memory */
                MEMORY_WRITE(16, VDP2(ZMXIN1), in);
                MEMORY_WRITE(16, VDP2(ZMXDN1), dn);
                break;
        default:
                return;
        }
}

void vdp2_scrn_zm_y_set(uint8_t scrn, uint16_t in, uint16_t dn)
{
#define ZMYIN0          0x007C
#define ZMYDN0          0x007E
#define ZMYIN1          0x008C
#define ZMYDN1          0x008E
        /*  integer part rounded to 3 bits*/
        in &= 0x07; 

        switch (scrn) {
        case SCRN_NBG0:
				/*  frac part rounded to 8 bits, shifted left by 8 */
                dn &= 0xFF; // 
                dn <<= 8;

                /* Write to memory */
                MEMORY_WRITE(16, VDP2(ZMYIN0), in);
                MEMORY_WRITE(16, VDP2(ZMYDN0), dn);
                break;
        case SCRN_NBG1:
                dn &= 0xFF;
                dn <<= 8;

                /* Write to memory */
                MEMORY_WRITE(16, VDP2(ZMYIN1), in);
                MEMORY_WRITE(16, VDP2(ZMYDN1), dn);
                break;
        default:
                return;
        }
}