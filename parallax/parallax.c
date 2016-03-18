#include <yaul.h>
#include <langam.h>

#include <stdlib.h>

#include "back1.h"
#include "back2.h"
#include "back3.h"
#include "back4.h"
#include "tables.h"

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

#define NGB0_PNT_PLANE0     back4_16_pattern_name_table_page0
#define NGB0_PNT_PLANE1     back4_16_pattern_name_table_page1
#define NGB0_CD             back4_16_cell_data
#define NGB0_CP             back4_16_cell_palette

#define NGB1_PNT_PLANE0     back3_16_pattern_name_table_page0
#define NGB1_PNT_PLANE1     back3_16_pattern_name_table_page1
#define NGB1_CD             back3_16_cell_data
#define NGB1_CP             back3_16_cell_palette

#define NGB2_PNT_PLANE0     back2_16_pattern_name_table_page0
#define NGB2_PNT_PLANE1     back2_16_pattern_name_table_page1
#define NGB2_CD             back2_16_cell_data
#define NGB2_CP             back2_16_cell_palette

#define NGB3_PNT_PLANE0     back1_16_pattern_name_table_page0
#define NGB3_PNT_PLANE1     back1_16_pattern_name_table_page1
#define NGB3_CD             back1_16_cell_data
#define NGB3_CP             back1_16_cell_palette


struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

static uint16_t *_nbg0_planes[4] = {
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x2000),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)


static uint16_t *_nbg1_planes[4] = {
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x2000),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)

static uint16_t *_nbg2_planes[4] = {
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x2000),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)

static uint16_t *_nbg3_planes[4] = {
        /* VRAM B1 */
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0),
        /* VRAM B1 */
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x2000),
        /* VRAM B1 */
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0),
        /* VRAM B1 */
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)


/* VRAM A0 after planes */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x4000);
static uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(0, 0x4000));

/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);
static uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));

/* VRAM A1 after planes */
static uint32_t *_nbg1_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(1, 0x4000);
static uint16_t _nbg1_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(1, 0x4000));

/* CRAM */
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(1, 0, 0);
static uint16_t _nbg1_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(1, 0, 0));

/* VRAM B0 after planes */
static uint32_t *_nbg2_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x4000);
static uint16_t _nbg2_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(2, 0x4000));

/* CRAM */
static uint32_t *_nbg2_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(2, 0, 0);
static uint16_t _nbg2_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(2, 0, 0));

/* VRAM B1 after planes */
static uint32_t *_nbg3_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(3, 0x4000);
static uint16_t _nbg3_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(3, 0x4000));

/* CRAM */
static uint32_t *_nbg3_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(3, 0, 0);
static uint16_t _nbg3_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(3, 0, 0));


void init_scrollscreen_nbg0(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG0 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg0_format;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg0_format.scf_scroll_screen = SCRN_NBG0;
	nbg0_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg0_format.scf_character_size= 1 * 1;
	nbg0_format.scf_pnd_size = 1; /* 1 word */
	nbg0_format.scf_auxiliary_mode = 1;
	nbg0_format.scf_cp_table = (uint32_t)_nbg0_cell_data;
	nbg0_format.scf_color_palette = (uint32_t)_nbg0_color_palette;
	nbg0_format.scf_plane_size = 2 * 1;
	nbg0_format.scf_map.plane_a = (uint32_t)_nbg0_planes[0];
	nbg0_format.scf_map.plane_b = (uint32_t)_nbg0_planes[1];
	nbg0_format.scf_map.plane_c = (uint32_t)_nbg0_planes[2];
	nbg0_format.scf_map.plane_d = (uint32_t)_nbg0_planes[3];

	vdp2_scrn_cell_format_set(&nbg0_format);
    vdp2_priority_spn_set(SCRN_NBG0, 7);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg0_page0 = _nbg0_planes[0];
    uint16_t *nbg0_page1 = _nbg0_planes[1];

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = _nbg0_cell_data_number + NGB0_PNT_PLANE0[i];
        uint16_t cell_data_number1 = _nbg0_cell_data_number + NGB0_PNT_PLANE1[i];
        nbg0_page0[i] = cell_data_number0 | _nbg0_palette_number;
        nbg0_page1[i] = cell_data_number1 | _nbg0_palette_number;
	}

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);
}


void init_scrollscreen_nbg1(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG1 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg1_format;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg1_format.scf_scroll_screen = SCRN_NBG1;
	nbg1_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg1_format.scf_character_size= 1 * 1;
	nbg1_format.scf_pnd_size = 1; /* 1 word */
	nbg1_format.scf_auxiliary_mode = 1;
	nbg1_format.scf_cp_table = (uint32_t)_nbg1_cell_data;
	nbg1_format.scf_color_palette = (uint32_t)_nbg1_color_palette;
	nbg1_format.scf_plane_size = 2 * 1;
	nbg1_format.scf_map.plane_a = (uint32_t)_nbg1_planes[0];
	nbg1_format.scf_map.plane_b = (uint32_t)_nbg1_planes[1];
	nbg1_format.scf_map.plane_c = (uint32_t)_nbg1_planes[2];
	nbg1_format.scf_map.plane_d = (uint32_t)_nbg1_planes[3];

	vdp2_scrn_cell_format_set(&nbg1_format);
    vdp2_priority_spn_set(SCRN_NBG1, 6);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg1_page0 = _nbg1_planes[0];
    uint16_t *nbg1_page1 = _nbg1_planes[1];

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = _nbg1_cell_data_number + NGB1_PNT_PLANE0[i];
        uint16_t cell_data_number1 = _nbg1_cell_data_number + NGB1_PNT_PLANE1[i];
        nbg1_page0[i] = cell_data_number0 | _nbg1_palette_number;
        nbg1_page1[i] = cell_data_number1 | _nbg1_palette_number;
	}

    vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);
}

void init_scrollscreen_nbg2(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG2 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg2_format;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg2_format.scf_scroll_screen = SCRN_NBG2;
	nbg2_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg2_format.scf_character_size= 1 * 1;
	nbg2_format.scf_pnd_size = 1; /* 1 word */
	nbg2_format.scf_auxiliary_mode = 1;
	nbg2_format.scf_cp_table = (uint32_t)_nbg2_cell_data;
	nbg2_format.scf_color_palette = (uint32_t)_nbg2_color_palette;
	nbg2_format.scf_plane_size = 2 * 1;
	nbg2_format.scf_map.plane_a = (uint32_t)_nbg2_planes[0];
	nbg2_format.scf_map.plane_b = (uint32_t)_nbg2_planes[1];
	nbg2_format.scf_map.plane_c = (uint32_t)_nbg2_planes[2];
	nbg2_format.scf_map.plane_d = (uint32_t)_nbg2_planes[3];

	vdp2_scrn_cell_format_set(&nbg2_format);
    vdp2_priority_spn_set(SCRN_NBG2, 5);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg2_page0 = _nbg2_planes[0];
    uint16_t *nbg2_page1 = _nbg2_planes[1];

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = _nbg2_cell_data_number + NGB2_PNT_PLANE0[i];
        uint16_t cell_data_number1 = _nbg2_cell_data_number + NGB2_PNT_PLANE1[i];
        nbg2_page0[i] = cell_data_number0 | _nbg2_palette_number;
        nbg2_page1[i] = cell_data_number1 | _nbg2_palette_number;
	}

    vdp2_scrn_display_set(SCRN_NBG2, /* transparent = */ true);
}

void init_scrollscreen_nbg3(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG2 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg3_format;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg3_format.scf_scroll_screen = SCRN_NBG3;
	nbg3_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg3_format.scf_character_size= 1 * 1;
	nbg3_format.scf_pnd_size = 1; /* 1 word */
	nbg3_format.scf_auxiliary_mode = 1;
	nbg3_format.scf_cp_table = (uint32_t)_nbg3_cell_data;
	nbg3_format.scf_color_palette = (uint32_t)_nbg3_color_palette;
	nbg3_format.scf_plane_size = 2 * 1;
	nbg3_format.scf_map.plane_a = (uint32_t)_nbg3_planes[0];
	nbg3_format.scf_map.plane_b = (uint32_t)_nbg3_planes[1];
	nbg3_format.scf_map.plane_c = (uint32_t)_nbg3_planes[2];
	nbg3_format.scf_map.plane_d = (uint32_t)_nbg3_planes[3];

	vdp2_scrn_cell_format_set(&nbg3_format);
    vdp2_priority_spn_set(SCRN_NBG3, 4);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg3_page0 = _nbg3_planes[0];
    uint16_t *nbg3_page1 = _nbg3_planes[1];

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = _nbg3_cell_data_number + NGB3_PNT_PLANE0[i];
        uint16_t cell_data_number1 = _nbg3_cell_data_number + NGB3_PNT_PLANE1[i];
        nbg3_page0[i] = cell_data_number0 | _nbg3_palette_number;
        nbg3_page1[i] = cell_data_number1 | _nbg3_palette_number;
	}

    vdp2_scrn_display_set(SCRN_NBG3, /* transparent = */ false);
}

void set_VRAM_access_priorities()
{
    struct vram_ctl * vram_ctl;
    
    vram_ctl = vdp2_vram_control_get();
    
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;    
    
    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_PNDR_NBG1;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;    

    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_PNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;    

    vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_PNDR_NBG3;
    vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_CHPNDR_NBG3;
    vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;
      
    vdp2_vram_control_set(vram_ctl);
}

void init(void)
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

        /* DMA Indirect list, aligned on 64 bytes due to more than 24bytes size (6*4*3=72) */
    uint32_t dma_tbl[] __attribute__((aligned(128))) = { 
            (uint32_t)sizeof(NGB0_CD), (uint32_t)_nbg0_cell_data, (uint32_t)NGB0_CD, 
            (uint32_t)sizeof(NGB0_CP), (uint32_t)_nbg0_color_palette, (uint32_t)NGB0_CP, 
            (uint32_t)sizeof(NGB1_CD), (uint32_t)_nbg1_cell_data, (uint32_t)NGB1_CD, 
            (uint32_t)sizeof(NGB1_CP), (uint32_t)_nbg1_color_palette, (uint32_t)NGB1_CP, 
            (uint32_t)sizeof(NGB2_CD), (uint32_t)_nbg2_cell_data, (uint32_t)NGB2_CD, 
            (uint32_t)sizeof(NGB2_CP), (uint32_t)_nbg2_color_palette, (uint32_t)NGB2_CP, 
            (uint32_t)sizeof(NGB3_CD), (uint32_t)_nbg3_cell_data, (uint32_t)NGB3_CD, 
            (uint32_t)sizeof(NGB3_CP), (uint32_t)_nbg3_color_palette, (uint32_t)NGB3_CP                    
    };    
    scu_dma_listcpy(dma_tbl, 8*3);
    while(scu_dma_get_status(SCU_DMA_ALL_CH) == SCU_DMA_STATUS_WAIT);
    
    set_VRAM_access_priorities();
    
    init_scrollscreen_nbg0();
    init_scrollscreen_nbg1();
    init_scrollscreen_nbg2();
    init_scrollscreen_nbg3();
     
    vdp2_tvmd_display_set(); 
}

int main(void)
{
    uint32_t padButton = 0;
    uint32_t g_scroll_back4 = 0, g_scroll_back3 = 0, g_scroll_back2 = 0, g_scroll_back1 = 0;
    uint32_t g_scroll_backy = 0;
    uint16_t index = 0;
    
	init();
   
	static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);  

	/* Main loop */
	while (!padButton) 
	{
        
	  	vdp2_tvmd_vblank_in_wait();

        vdp2_scrn_scv_x_set(SCRN_NBG0, g_scroll_back4, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG1, g_scroll_back3, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG2, g_scroll_back2, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG3, g_scroll_back1, 0);
        
        g_scroll_backy = 34;
        //g_scroll_backy = 35 + (lut[index % 512])/8;
        vdp2_scrn_scv_y_set(SCRN_NBG0, g_scroll_backy, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG1, g_scroll_backy, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG2, g_scroll_backy, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG3, g_scroll_backy, 0);        
        
        g_scroll_back4 = (g_scroll_back4 + 6) % 1024;
        g_scroll_back3 = (g_scroll_back3 + 4) % 1024;
        g_scroll_back2 = (g_scroll_back2 + 2) % 1024;
        g_scroll_back1 = (g_scroll_back1 + 1) % 1024;
        index++;
        
        
        if (g_digital.connected == 1)
        {
            padButton = g_digital.pressed.button.start; 	
        }        
        
        vdp2_tvmd_vblank_out_wait();
    }
    
    abort();

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