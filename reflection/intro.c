/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include <langam/dma_wrapper.h>
#include <langam/sequencer.h>

#include "scenes.h"
#include "tables.h"
#include "mjjprod.h"
#include "stars.h"
#include "ship.h"


/*
 * VDP2 VRAM Organization
 * Bank A0
 * - NBG0 plane data, from 0x0 to 0x01000 (4096 bytes, 1 word per cell, 512x256px)
 * - NBG0 cell pattern data, from 0x01000 to 0x07000 (8416 bytes, 263 8x8 cells in 4bits color)* 
 * 
 * Bank A1
 * - NBG1 plane data, from 0x0 to 0x02000 (8192 bytes, 1 word per cell, 512x512px)
 * - NBG1 cell pattern data, from 0x02000 to 0x08000 (6240 bytes, 195 8x8 cells in 4bits color)
 *  
 * Bank B0
 * - NBG2 plane data, from 0x0 to 0x01000 (4096 bytes, 1 word per cell, 512x256px)
 * - NBG2 cell pattern data, from 0x01000 to 0x07000 (24576 bytes, 768 8x8 cells in 4bits color)
 * 
 * Bank B1
 * - nothing
 * 
 * CRAM
 * - NBG0 palette set at palette 2
 * - NBG1 palette set at palette 0
 * - NBG2 palette set at palette 1
 * 
 * Registers:
 * - TVMD: enable display, border color mode to back screen, 240 lines vertical resolution
*/ 

#define NBG0_VRAM_PAL_NB 2
#define NBG1_VRAM_PAL_NB 0
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

static uint16_t *_nbg1_planes[4] = {
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)

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

/* VRAM A1 after plane */
static uint32_t *_nbg1_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(1, 0x2000);
static uint16_t _nbg1_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(1, 0x2000));

/* VRAM B0 after plane */
static uint32_t *_nbg2_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x1000);
static uint16_t _nbg2_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(2, 0x1000));

/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(NBG0_VRAM_PAL_NB, 0, 0);
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(NBG1_VRAM_PAL_NB, 0, 0);
static uint32_t *_nbg2_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(NBG2_VRAM_PAL_NB, 0, 0);
static uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(NBG0_VRAM_PAL_NB, 0, 0));
static uint16_t _nbg1_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(NBG1_VRAM_PAL_NB, 0, 0));
static uint16_t _nbg2_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(NBG2_VRAM_PAL_NB, 0, 0));

static uint16_t g_timer1;
static uint16_t g_timer2;
static int16_t  g_timer3;


/*
 *  void _intro_init_scrollscreen_nbg0(void)
 */
void _intro_init_scrollscreen_nbg0(void)
{
    struct scrn_cell_format nbg0_format;
    
	nbg0_format.scf_scroll_screen = SCRN_NBG0;
	nbg0_format.scf_cc_count = SCRN_CCC_PALETTE_16;
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

    /* Copy the palette data */
    //scu_dma_sync_memcpy(_nbg0_color_palette, mjjprod_cell_palette, sizeof(mjjprod_cell_palette));

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg0_page0 = _nbg0_planes[0];

	for (i = 0; i < 2048; i++) {
			uint16_t cell_data_number = _nbg0_cell_data_number + mjjprod_pattern_name_table[i];
			nbg0_page0[i] = cell_data_number | _nbg0_palette_number;
	}

	vdp2_priority_spn_set(SCRN_NBG0, 7);
    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);    
}

/*
 *  void _intro_init_scrollscreen_nbg1(void)
 */
void _intro_init_scrollscreen_nbg1(void)
{
	/* NBG1 stuff is located in A1 bank */

    /* set NBG1 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg1_format;

	nbg1_format.scf_scroll_screen = SCRN_NBG1;
	nbg1_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg1_format.scf_character_size= 1 * 1;
	nbg1_format.scf_pnd_size = 1; /* 1 word */
	nbg1_format.scf_auxiliary_mode = 1;
	nbg1_format.scf_cp_table = (uint32_t)_nbg1_cell_data;
	nbg1_format.scf_color_palette = (uint32_t)_nbg1_color_palette;
	nbg1_format.scf_plane_size = 1 * 1;
	nbg1_format.scf_map.plane_a = (uint32_t)_nbg1_planes[0];
	nbg1_format.scf_map.plane_b = (uint32_t)_nbg1_planes[1];
	nbg1_format.scf_map.plane_c = (uint32_t)_nbg1_planes[2];
	nbg1_format.scf_map.plane_d = (uint32_t)_nbg1_planes[3];

	vdp2_scrn_cell_format_set(&nbg1_format);

    /* Copy the palette data */
    //scu_dma_sync_memcpy(_nbg1_color_palette, ship_cell_palette, sizeof(ship_cell_palette));
    
    /* Copy the cell data */
    //scu_dma_sync_memcpy(_nbg1_cell_data, ship_cell_data, sizeof(ship_cell_data));

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg1_page0 = _nbg1_planes[0];

	for (i = 0; i < 4096; i++) {
			uint16_t cell_data_number = _nbg1_cell_data_number + ship_pattern_name_table[i];
			nbg1_page0[i] = cell_data_number | _nbg1_palette_number;
	}

	vdp2_priority_spn_set(SCRN_NBG1, 6);
    vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);
}

/*
 *  void _intro_init_scrollscreen_nbg2(void)
 */
void _intro_init_scrollscreen_nbg2(void)
{
	/* NBG2 stuff is located in B0 bank */

    /* set NBG2 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg2_format;

	nbg2_format.scf_scroll_screen = SCRN_NBG2;
	nbg2_format.scf_cc_count = SCRN_CCC_PALETTE_16;
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

    /* Copy the palette data */
    //scu_dma_sync_memcpy(_nbg2_color_palette, stars_cell_palette, sizeof(stars_cell_palette));  

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg2_page0 = _nbg2_planes[0];

	for (i = 0; i < 2048; i++) {
			uint16_t cell_data_number = _nbg2_cell_data_number + stars_pattern_name_table[i];
			nbg2_page0[i] = cell_data_number | _nbg2_palette_number;
	}

	vdp2_priority_spn_set(SCRN_NBG2, 5);
    vdp2_scrn_display_set(SCRN_NBG2, /* transparent = */ true);
}


/*
 *  void _intro_set_VRAM_access(void)
 *  Set VRAM access priorities
 */
void _intro_set_VRAM_access(void)
{
    struct vram_ctl *vram_ctl;
    
    // 16 colors bitmap requires 1 access each
    vram_ctl = vdp2_vram_control_get();
	
    // Rules:
    // only 2 PNDR per timing and split accros x0 and x1 banks
    // CHPNDR cannot be acceded during certain cycles after PNDR
    
	// Bank A0, 16 colors bitmap requires 1 access each
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_PNDR_NBG0; 	// NBG0 character pattern or bitmap data read
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0; 
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS; 
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

	// Bank A1
    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_PNDR_NBG1; 	// NBG1 character pattern or bitmap data read
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;     // NBG1 pattern name data read
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
	
	// Bank B0
    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	// NBG2 character pattern data read
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_PNDR_NBG2; 	// NBG2 pattern name data read
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG2;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
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
void intro_init(void)
{       
    uint16_t tvmd;
	
    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();    
        
    /* set 320x240 res */
    vdp2_tvmd_display_clear();
    tvmd = MEMORY_READ(16, VDP2(TVMD));
    tvmd |= ((1 << 8) | (1 << 4));  // set BDCLMD,  VRES0 to 1
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);    
    
    // Set Color mode to mode 0 (2KWord Color RAM)
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);        
    
	/* Enable color offset function on all screens, back and sprites */
    MEMORY_WRITE(16, VDP2(CLOFEN), 0x007F); /* 111 1111 */
    /* assign all objects to color offset A except Back and NBG2 to offset B */
    MEMORY_WRITE(16, VDP2(CLOFSL), 0x0024); /* 010 0100 */

	/* Set R,G,B values for color offset A to -255 (force black) */
	MEMORY_WRITE(16, VDP2(COAR), -255);
	MEMORY_WRITE(16, VDP2(COAB), -255);
	MEMORY_WRITE(16, VDP2(COAG), -255);
    
	/* Set R,G,B values for color offset B to -255 (force black) */
	MEMORY_WRITE(16, VDP2(COBR), -255);
	MEMORY_WRITE(16, VDP2(COBB), -255);
	MEMORY_WRITE(16, VDP2(COBG), -255);    
    
    /* DMA Indirect list, aligned on 64 bytes due to more than 24bytes size (6*4*3=72) */
    uint32_t dma_tbl[] __attribute__((aligned(64))) = { 
            (uint32_t)sizeof(mjjprod_cell_data), (uint32_t)_nbg0_cell_data, (uint32_t)mjjprod_cell_data, 
            (uint32_t)sizeof(mjjprod_cell_palette), (uint32_t)_nbg0_color_palette, (uint32_t)mjjprod_cell_palette, 
            (uint32_t)sizeof(ship_cell_data), (uint32_t)_nbg1_cell_data, (uint32_t)ship_cell_data, 
            (uint32_t)sizeof(ship_cell_palette), (uint32_t)_nbg1_color_palette, (uint32_t)ship_cell_palette,             
            (uint32_t)sizeof(stars_cell_data), (uint32_t)_nbg2_cell_data, (uint32_t)stars_cell_data,
            (uint32_t)sizeof(stars_cell_palette), (uint32_t)_nbg2_color_palette, SCU_DMA_END_CODE | (uint32_t)stars_cell_palette             
    };    
    scu_dma_listcpy(dma_tbl);
    while(scu_dma_get_status(SCU_DMA_ALL_CH) == SCU_DMA_STATUS_WAIT);
    
    /* set all other stuff */ 
    _intro_init_scrollscreen_nbg0();
    _intro_init_scrollscreen_nbg1();
	_intro_init_scrollscreen_nbg2();
    _intro_set_VRAM_access();
    
    // init globals
    g_timer1 = 0;
    g_timer2 = 255;
    g_timer3 = -255;
    
    /* go ! */
    vdp2_tvmd_display_set();      
}

void intro_update(uint32_t timer __unused)
{
    // fade in
    if(g_timer1 <= 500)
    {
        // fade in offset A to +255
        MEMORY_WRITE(16, VDP2(COAR), -255 + g_timer1);
        MEMORY_WRITE(16, VDP2(COAB), -255 + g_timer1);
        MEMORY_WRITE(16, VDP2(COAG), -255 + g_timer1);    
        g_timer1 += 10;
    }
    else if(g_timer2 >= 10)
    {
        MEMORY_WRITE(16, VDP2(COAR), g_timer2);
        MEMORY_WRITE(16, VDP2(COAB), g_timer2);
        MEMORY_WRITE(16, VDP2(COAG), g_timer2);  
        g_timer2 -= 10;
    }
    
    // fade in offset B to 0
    if(g_timer3 <= -4)
    {
        MEMORY_WRITE(16, VDP2(COBR), g_timer3);
        MEMORY_WRITE(16, VDP2(COBB), g_timer3);
        MEMORY_WRITE(16, VDP2(COBG), g_timer3); 
        g_timer3 += 4;  
    }  
    else
    {
        MEMORY_WRITE(16, VDP2(COBR), 0);
        MEMORY_WRITE(16, VDP2(COBB), 0);
        MEMORY_WRITE(16, VDP2(COBG), 0); 
    }
    
      
}

void intro_draw(void)
{
     
}

void intro_exit(void)
{
    g_timer1 = 0;
    g_timer2 = 255;
    g_timer3 = 0;
}

