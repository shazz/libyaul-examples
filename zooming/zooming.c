#include <yaul.h>
#include <langam.h>

#include <stdlib.h>

#include "zooming3.h"

/*
 * VDP2 VRAM Organization
 * Bank A0
 * - NBG0 2 planes data, from 0x0 to 0x04000 (2x 8192 bytes, 1 word per cell, 512x512px)
 * - NBG0 cell pattern data, from 0x04000 (61 312 bytes, 1916 8x8 cells in 4bits color)
 * 
 * Bank A1
 * - NBG1 2 planes data, from 0x0 to 0x04000 (2x 8192 bytes, 1 word per cell, 512x512px)
 * - NBG1 cell pattern data, from 0x04000  (20 800 bytes, 650 8x8 cells in 4bits color)
 *  
 * Bank B0
 * - NBG2 2 planes data, from 0x0 to 0x04000 (2x 8192 bytes, 1 word per cell, 512x512px)
 * - NBG2 cell pattern data, from 0x04000 (17 440 bytes, 545 8x8 cells in 4bits color)
 * 
 * Bank B1
 * - NBG3 2 planes data, from 0x0 to 0x04000 (2x 8192 bytes, 1 word per cell, 512x512px)
 * - NBG3 cell pattern data, from 0x04000 (19 104 bytes, 597 8x8 cells in 4bits color)
 * 
 * CRAM
 * - NBG0 palette 16 set at palette 0 
 * - NBG1 palette 16 set at palette bank256 1
 * - NBG2 palette 16 set at palette bank256 2 
 * - NBG3 palette 16 set at palette bank256 3
 * 
 * Registers:
 * - TVMD: enable display, border color mode to back screen, 240 lines vertical resolution
*/ 

#define NGB0_PNT_PLANE0     zooming3_pattern_name_table_page0
#define NGB0_PNT_PLANE1     zooming3_pattern_name_table_page1
//#define NGB0_PNT_PLANE2     zooming2_pattern_name_table_page2
//#define NGB0_PNT_PLANE3     zooming2_pattern_name_table_page3
#define NGB0_CD             zooming3_cell_data
#define NGB0_CP             zooming3_cell_palette

void vdp2_scrn_zm_x_set(uint8_t scrn, uint16_t in, uint16_t dn);
void vdp2_scrn_zm_y_set(uint8_t scrn, uint16_t in, uint16_t dn);

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

// define 4 scroll screens of 2 planes pages
// each plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)
static uint16_t *_nbg0_planes[4] = { /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x2000),
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x4000),    //not used
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x6000)     //not used
}; 


/* VRAM A0 after planes, CRAM */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x4000);
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);


uint16_t g_zooming_factor_dn = 0;
uint16_t g_zooming_factor_in = 1;

// joypad
static unsigned int joyLeft = 0, joyRight = 0, joyUp = 0, joyDown = 0, joyA = 0;    

    
/*
 * void init_scrollscreen_nbg(int screen, uint16_t *planes[], uint32_t *cell_data_ptr, uint32_t *palette_ptr, uint16_t page0_data[], uint16_t page1_data[], uint8_t priority, bool transparent) 
 * 
 * 
 */
//void init_scrollscreen_nbg(int screen, uint16_t *planes[], uint32_t *cell_data_ptr, uint32_t *palette_ptr, uint16_t page0_data[], uint16_t page1_data[], uint16_t page2_data[], uint16_t page3_data[], uint8_t priority, bool transparent)
void init_scrollscreen_nbg(int screen, uint16_t *planes[], uint32_t *cell_data_ptr, uint32_t *palette_ptr, uint16_t page0_data[], uint16_t page1_data[], uint8_t priority, bool transparent)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG2 in cell mode, 16 col, 1x1 */
    struct scrn_cell_format nbg_format;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg_format.scf_scroll_screen = screen;
	nbg_format.scf_cc_count = SCRN_CCC_PALETTE_16;
	nbg_format.scf_character_size= 1 * 1;
	nbg_format.scf_pnd_size = 1; /* 1 word */
	nbg_format.scf_auxiliary_mode = 1;
	nbg_format.scf_cp_table = (uint32_t)cell_data_ptr;
	nbg_format.scf_color_palette = (uint32_t)palette_ptr;
	nbg_format.scf_plane_size = 2 * 1;
	nbg_format.scf_map.plane_a = (uint32_t)planes[0];
	nbg_format.scf_map.plane_b = (uint32_t)planes[1];
	nbg_format.scf_map.plane_c = (uint32_t)planes[2];
	nbg_format.scf_map.plane_d = (uint32_t)planes[3];

	vdp2_scrn_cell_format_set(&nbg_format);
    vdp2_priority_spn_set(screen, priority);

    /* Build the pattern data */   
    uint32_t i;
    uint16_t *nbg_page0 = planes[0];
    uint16_t *nbg_page1 = planes[1];
    //uint16_t *nbg_page2 = planes[2];
    //uint16_t *nbg_page3 = planes[3];
  
    uint16_t palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER((uint32_t)palette_ptr);
    uint16_t cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)cell_data_ptr);

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = cell_data_number + page0_data[i];
        uint16_t cell_data_number1 = cell_data_number + page1_data[i];
        //uint16_t cell_data_number2 = cell_data_number + page2_data[i];
        //uint16_t cell_data_number3 = cell_data_number + page3_data[i];
        nbg_page0[i] = cell_data_number0 | palette_number;
        nbg_page1[i] = cell_data_number1 | palette_number;
        //nbg_page2[i] = cell_data_number2 | palette_number;
        //nbg_page3[i] = cell_data_number3 | palette_number;
	}

    vdp2_scrn_display_set(screen, transparent);
}

void set_VRAM_access_priorities()
{
    struct vram_ctl * vram_ctl;
    
    vram_ctl = vdp2_vram_control_get();
    
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;    
    
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;     

    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;       
    
    vdp2_vram_control_set(vram_ctl);
}

void init_vdp2_scrollescreens(void)
{
    /* DMA Indirect list, aligned on 64 bytes due to more than 24bytes size (6*4*3=72) */
    uint32_t dma_tbl[] __attribute__((aligned(64))) = { 
            (uint32_t)sizeof(NGB0_CD), (uint32_t)_nbg0_cell_data, (uint32_t)NGB0_CD, 
            (uint32_t)sizeof(NGB0_CP), (uint32_t)_nbg0_color_palette, (uint32_t)NGB0_CP, 
    };    
    scu_dma_listcpy(dma_tbl, 2*3);
    while(scu_dma_get_status(SCU_DMA_ALL_CH) == SCU_DMA_STATUS_WAIT);
    
    set_VRAM_access_priorities();
    
    //init_scrollscreen_nbg(SCRN_NBG0, _nbg0_planes, _nbg0_cell_data, _nbg0_color_palette, NGB0_PNT_PLANE0, NGB0_PNT_PLANE1, NGB0_PNT_PLANE2, NGB0_PNT_PLANE3, 7, true);    
    init_scrollscreen_nbg(SCRN_NBG0, _nbg0_planes, _nbg0_cell_data, _nbg0_color_palette, NGB0_PNT_PLANE0, NGB0_PNT_PLANE1, 7, true);
     
    vdp2_tvmd_display_set(); 
}

void read_digital_pad(void)
{
	if (g_digital.connected == 1)
	{
		joyUp = g_digital.released.button.up;
		joyDown = g_digital.released.button.down;
		joyRight = g_digital.pressed.button.right;                            
		joyLeft = g_digital.pressed.button.left;
        joyA = g_digital.pressed.button.a;
        
		if (joyUp)
		{
            if(g_zooming_factor_in < 0x7) g_zooming_factor_in++; 
		}
		else if (joyDown)
		{
            if(g_zooming_factor_in > 0x0) g_zooming_factor_in--; 
		}
        if (joyRight)
		{
            if(g_zooming_factor_dn < 0xFF) g_zooming_factor_dn++; 
            else
            {
                g_zooming_factor_dn = 0;
                if(g_zooming_factor_in < 0x7) g_zooming_factor_in++;    
            }
		}
		else if (joyLeft)
		{
            if(g_zooming_factor_dn > 0x0) g_zooming_factor_dn--; 
            else
            {
                g_zooming_factor_dn = 0xFF;
                if(g_zooming_factor_in > 0x0) g_zooming_factor_in--;   
            }            
		}
		else if (joyA)
		{

		}        
		
		// exit
		if(g_digital.pressed.button.start) abort();		
	}  
}

int main(void)
{
    uint32_t padButton = 0;
    uint32_t g_scroll_back4_x = 0;
    int16_t  g_scroll_back4_y = 128; //384;

    irq_mux_t *vblank_in;
    irq_mux_t *vblank_out;
    
    static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };

    vdp2_init();
    /* set 320x240 res, back color mode */    
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);        
    
	// enable zoom out to 1/4 as we are in 16 colors mode for NBG0 (2 first bits, bits 8-9 for NBG1), that disables NBG2 display
	MEMORY_WRITE(16, VDP2(ZMCTL), 0x03);    
    
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
    
	init_vdp2_scrollescreens();
   
	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);  
    
	/* Main loop */
	while (!padButton) 
	{
	  	vdp2_tvmd_vblank_in_wait();
        
        g_scroll_back4_x++;
        vdp2_scrn_scv_x_set(SCRN_NBG0, g_scroll_back4_x, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG0, g_scroll_back4_y, 0);
        vdp2_scrn_zm_x_set(SCRN_NBG0, g_zooming_factor_in, g_zooming_factor_dn);
        vdp2_scrn_zm_y_set(SCRN_NBG0, g_zooming_factor_in, g_zooming_factor_dn);
        
        read_digital_pad();   
        
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