/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include "sequencer.h"
#include "scenes.h"
#include "tables.h"
#include "bitmaps.h"

#define NBG0_VRAM_PAL_NB 0

static uint32_t *backscreen_tb = (uint32_t *)VRAM_ADDR_4MBIT(3, 0x0);
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(0, 0, 0);
static uint16_t g_timer1;
static uint16_t g_timer2;
static uint32_t g_timer3;

void logo_init(void)
{
    struct vram_ctl *vram_ctl;
    uint16_t tvmd;
    struct scrn_bitmap_format nbg0_format;
    uint16_t bmpma;    
    
    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();    
        
    /* set 320x240 res */
    vdp2_tvmd_display_clear();
    tvmd = MEMORY_READ(16, VDP2(TVMD));
    tvmd |= ((1 << 8) | (1 << 4));  // set BDCLMD,  VRES0 to 1
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);    
    
    // Set Color mode to mode 0 (2KWord Color RAM)
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);        
    
	/* Enable color offset function on scroll screen NBG0 and assign all screens to color offset A */
    MEMORY_WRITE(16, VDP2(CLOFEN), 0x0001); /* 00 0001 */
    MEMORY_WRITE(16, VDP2(CLOFSL), 0x0000);

	/* Set R,G,B values for color offset A to -255 (force black) */
	MEMORY_WRITE(16, VDP2(COAR), -255);
	MEMORY_WRITE(16, VDP2(COAB), -255);
	MEMORY_WRITE(16, VDP2(COAG), -255);

    /* Copy the NBG0 bitmap, BGR555 palette data */
    memcpy((void *)VRAM_ADDR_4MBIT(0, 0x00000), bitmap_data, sizeof(bitmap_data));   
    memcpy(_nbg0_color_palette, bitmap_palette, sizeof(bitmap_palette));            

    nbg0_format.sbf_scroll_screen = SCRN_NBG0;                      /* Normal background */
    nbg0_format.sbf_cc_count = SCRN_CCC_PALETTE_16;                 /* color mode to PAL16 */
    nbg0_format.sbf_bitmap_size.width = 512;                        /* Bitmap sizes: 512x256 */
    nbg0_format.sbf_bitmap_size.height = 256;
    nbg0_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);   /* Bitmap pattern lead address */
    nbg0_format.sbf_color_palette = (uint32_t)_nbg0_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg0_format);
    
    // then set BMPNA (again) for NBG0
    bmpma = MEMORY_READ(16, VDP2(BMPNA));
    bmpma |= ((NBG0_VRAM_PAL_NB & 0x7 ) << 0);
    MEMORY_WRITE(16, VDP2(BMPNA), bmpma);        
    
    // set max priority for NBG0 and transaprent color
    vdp2_priority_spn_set(SCRN_NBG0, 7);   
    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);    
    
    /* Copy rasters bars color data to VRAM */
    memcpy(backscreen_tb, backscreen2, sizeof(backscreen2));   

    /* set BKTAL and BKTAU (Back screen table address register */
    uint32_t adr = ((uint32_t) backscreen_tb  ) + (240*2);
    MEMORY_WRITE(16, VDP2(BKTAL), ( ( (uint32_t) adr >> 1) & 0x0FFFF) );
    MEMORY_WRITE(16, VDP2(BKTAU), (0x8000 | ( ((uint32_t) adr >> 17) & 0x7)));        
    
    // 16 colors bitmap requires 1 access each
    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1; 
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS; //VRAM_CTL_CYCP_VCSTDR_NBG0
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS; //VRAM_CTL_CYCP_VCSTDR_NBG0
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);   
    
    vdp2_tvmd_display_set();    
    
    // init globals
    g_timer1 = 0;
    g_timer2 = 255;
    g_timer3 = 0;
}

void logo_update(uint32_t timer)
{
    timer++; // I don't understand this error: unused parameter 'timer' [-Werror=unused-parameter]
    // fade in
    if(g_timer1 <= 500)
    {
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
    
    if(g_timer3 <= 240*2) 
        g_timer3 += 4;    
}

void logo_draw(void)
{
    // set rasters
    uint32_t adr = ((uint32_t) backscreen_tb) + (240*2) - g_timer3;
    
    /* set BKTAL and BKTAU (Back screen table address register */
    MEMORY_WRITE(16, VDP2(BKTAL), ( ( (uint32_t) adr >> 1) & 0x0FFFF) );
    MEMORY_WRITE(16, VDP2(BKTAU), (0x8000 | ( ((uint32_t) adr >> 17) & 0x7)));       
}

void logo_exit(void)
{
    g_timer1 = 0;
    g_timer2 = 255;
}

