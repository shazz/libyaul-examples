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
#define NBG1_VRAM_PAL_NB 1


static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(NBG0_VRAM_PAL_NB, 0, 0);
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_0_OFFSET(NBG1_VRAM_PAL_NB, 0, 0);

static uint16_t g_timer1;
static uint16_t g_timer2;

void setVRAM_access(void);
void initScrollScreenNBG0(void);
void initScrollScreenNBG1(void);

void logo_init(void)
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
    /* assign all objects to color offset A except Back and NBG1 to offset B */
    MEMORY_WRITE(16, VDP2(CLOFSL), 0x0022); /* 010 0010 */

	/* Set R,G,B values for color offset A to -255 (force black) */
	MEMORY_WRITE(16, VDP2(COAR), -255);
	MEMORY_WRITE(16, VDP2(COAB), -255);
	MEMORY_WRITE(16, VDP2(COAG), -255);
    
	/* Set R,G,B values for color offset B to -255 (force black) */
	MEMORY_WRITE(16, VDP2(COBR), -255);
	MEMORY_WRITE(16, VDP2(COBB), -255);
	MEMORY_WRITE(16, VDP2(COBG), -255);    
    
    /* set all other stuff */ 
    initScrollScreenNBG0();
    initScrollScreenNBG1();
    setVRAM_access();
    
    // init globals
    g_timer1 = 0;
    g_timer2 = 255;
    
    /* go ! */
    vdp2_tvmd_display_set();      
}

/*
 *  void initScrollScreenNBBG1(void)
 *  Setup line scroll
 */
void initScrollScreenNBG1(void)
{
    /* set NBG1 in bitmap mode, 16 col, 512x256 */
    struct scrn_bitmap_format nbg1_format;
    //uint16_t bmpma;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();
    
    /* Copy the NBG1 bitmap, BGR555 palette data */
    memcpy((void *)VRAM_ADDR_4MBIT(1, 0x00000), stars_data, sizeof(stars_data));   
    memcpy(_nbg1_color_palette, stars_palette, sizeof(stars_palette));            

    nbg1_format.sbf_scroll_screen = SCRN_NBG1;                      /* Normal background */
    nbg1_format.sbf_cc_count = SCRN_CCC_PALETTE_16;                 /* color mode to PAL16 */
    nbg1_format.sbf_bitmap_size.width = 512;                        /* Bitmap sizes: 512x256 */
    nbg1_format.sbf_bitmap_size.height = 256;
    nbg1_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(1, 0x00000);   /* Bitmap pattern lead address */
    nbg1_format.sbf_color_palette = (uint32_t)_nbg1_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg1_format);
    
    // then set BMPNA (again) for NBG1
    MEMORY_WRITE(16, VDP2(CRAOFA), (NBG1_VRAM_PAL_NB & 0x7) << 4); 
    //bmpma = MEMORY_READ(16, VDP2(BMPNA));
    //bmpma &= 0x3037; 
    //bmpma |= ((NBG1_VRAM_PAL_NB & 0x7 ) << 8);   
    //MEMORY_WRITE(16, VDP2(BMPNA), bmpma);
    
    // set priority for NB1 to be backward NBG0
    vdp2_priority_spn_set(SCRN_NBG1, 6);        
    vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);
}

void initScrollScreenNBG0(void)
{
    struct scrn_bitmap_format nbg0_format;
    uint16_t bmpma;    
    
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
    
    // set max priority for NBG0 and transparent color
    vdp2_priority_spn_set(SCRN_NBG0, 7);   
    
    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);         
}

void setVRAM_access(void)
{
    struct vram_ctl *vram_ctl;
    
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
}

void logo_update(uint32_t timer __unused)
{
    // fade in
    if(g_timer1 <= 500)
    {
        // fade in offset A to +255
        MEMORY_WRITE(16, VDP2(COAR), -255 + g_timer1);
        MEMORY_WRITE(16, VDP2(COAB), -255 + g_timer1);
        MEMORY_WRITE(16, VDP2(COAG), -255 + g_timer1);    
    
        // fade in offset B to 0
        if(g_timer1 <= 245)
        {
            MEMORY_WRITE(16, VDP2(COBR), -255 + g_timer1);
            MEMORY_WRITE(16, VDP2(COBB), -255 + g_timer1);
            MEMORY_WRITE(16, VDP2(COBG), -255 + g_timer1); 
        }  
    
        g_timer1 += 10;
    }
    else if(g_timer2 >= 10)
    {
        MEMORY_WRITE(16, VDP2(COAR), g_timer2);
        MEMORY_WRITE(16, VDP2(COAB), g_timer2);
        MEMORY_WRITE(16, VDP2(COAG), g_timer2);  
        g_timer2 -= 10;
    }
}

void logo_draw(void)
{
     
}

void logo_exit(void)
{
    g_timer1 = 0;
    g_timer2 = 255;
}

