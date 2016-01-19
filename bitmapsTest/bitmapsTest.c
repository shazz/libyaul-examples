// Bitmaps Test
// 
// Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>

//#include "bitmapTest.h"
#include "background.h"
#include "logo.h"
#include "tables.h"

#define SCROLLFN
//#define SCROLL_HORIZ   
//#define SCROLL_VERT
//#define ZOOM
//#define JOYSTICK    

void vdp2_scrn_zm_x_set(uint8_t scrn, uint16_t in, uint8_t dn);
void vdp2_scrn_zm_y_set(uint8_t scrn, uint16_t in, uint8_t dn);

struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);


/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 32);

#ifdef SCROLLFN
/* Line Scroll table address */
static uint32_t *line_scroll_tb = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x1F000);
struct scrn_ls_format linescrollfmt;
#endif

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

void initScrollScreens(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG0 in bitmap mode, 16 col, 512x256 */
    struct scrn_bitmap_format nbg0_format;
    struct scrn_bitmap_format nbg1_format;
    struct vram_ctl *vram_ctl;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

    nbg0_format.sbf_scroll_screen = SCRN_NBG0; /* Normal/rotational background */
    nbg0_format.sbf_cc_count = SCRN_CCC_PALETTE_16; /* color mode */
    nbg0_format.sbf_bitmap_size.width = 512; /* Bitmap sizes: 512x256, 512x512, 1024x256, 1024x512 */
    nbg0_format.sbf_bitmap_size.height = 256;
    nbg0_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000); /* Bitmap pattern lead address */
    nbg0_format.sbf_color_palette = (uint32_t)_nbg0_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg0_format);
    vdp2_priority_spn_set(SCRN_NBG0, 6);

    nbg1_format.sbf_scroll_screen = SCRN_NBG1; /* Normal/rotational background */
    nbg1_format.sbf_cc_count = SCRN_CCC_PALETTE_16; /* color mode */
    nbg1_format.sbf_bitmap_size.width = 512; /* Bitmap sizes: 512x256, 512x512, 1024x256, 1024x512 */
    nbg1_format.sbf_bitmap_size.height = 256;
    nbg1_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(1, 0x00000); /* Bitmap pattern lead address */
    nbg1_format.sbf_color_palette = (uint32_t)_nbg1_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg1_format);
    vdp2_priority_spn_set(SCRN_NBG1, 7);    

    MEMORY_WRITE(16, VDP2(RAMCTL), 0x1300);

    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);
	
    #ifdef ZOOM
	// enable zoom out to 1/4 as we are in 16 colors mode for NBG0 (2 first bits, bits 8-9 for NBG1), that disables NBG2 display
	//MEMORY_WRITE(16, VDP2(ZMCTL), 0x03);
    #endif
	
    #ifdef SCROLLFN
	// copy line scroll table in VRAM
	uint16_t y;
	for (y = 0; y < 512; y++) 
	{
			line_scroll_tb[y] = (lut[y]) << 16;
			line_scroll_tb[y + 512] = (lut[y]) << 16;
	}
	
	linescrollfmt.ls_scrn = SCRN_NBG1;
	linescrollfmt.ls_lsta = (uint32_t)line_scroll_tb;
	linescrollfmt.ls_int = 0;	//for interlace, not used in libyaul code
	linescrollfmt.ls_fun = SCRN_LS_N1SCX;	//enable Horiz line scroll, | SCRN_LS_N0SCY would also enables Vert line scroll	
	
	#define N0LSS0 (1 << 4)
	#define N0LSS1 (1 << 5) //enable 8 lines block - 00, 01 10 11 for 1,2, 4 or 8 lines based on non-interlaced settinss
	#define N0LZMX (1 << 3) // also enable linescroll in sync with zoom, scale horizontally per line units
	linescrollfmt.ls_fun = SCRN_LS_N1SCX;  // | N0LSS0; // | N0LSS0 | N0LSS1; // | N0LZMX
	
	vdp2_scrn_ls_set(&linescrollfmt);
    #endif
	
    /* Copy the bitmap data */
    memcpy((void *)VRAM_ADDR_4MBIT(0, 0x00000), background_data, sizeof(background_data));   
    memcpy((void *)VRAM_ADDR_4MBIT(1, 0x00000), logo_data, sizeof(logo_data));   


    /* Copy the BGR555 palette data */
    memcpy(_nbg0_color_palette, background_palette, sizeof(background_palette));    
    memcpy(_nbg1_color_palette, logo_palette, sizeof(logo_palette));  

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);
    vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);
    vdp2_tvmd_display_set();

}

int main(void)
{
    #ifdef SCROLL_HORIZ
    uint16_t counterX = 0;
    #endif
    
    #ifdef SCROLL_VERT
	uint16_t counterY = 0;
	int8_t incrY = +1;
    #endif
    
    #ifdef JOYSTICK
    unsigned int joyR = 0;
    unsigned int oldjoyR = 0;
    unsigned int joyL = 0;
    unsigned int oldjoyL = 0;  

    unsigned int joyA = 0;
    unsigned int oldjoyA = 0;
    unsigned int joyB = 0;
    unsigned int oldjoyB = 0;  
    unsigned int joyStart = 0;
    #endif
    
    #ifdef ZOOM
 	uint16_t fracZoom = 0;
	uint16_t intZoom = 1;   
    #endif

	
	hardware_init();
    initScrollScreens();

	static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

	/* Main loop */
	for(;;)
	{
        /* Wait for next vblank */
		vdp2_tvmd_vblank_out_wait();
        
        //vdp2_scrn_zm_x_set(SCRN_NBG0, 3, 0);  
        //vdp2_scrn_zm_y_set(SCRN_NBG0, 3, 0); 
        
        #ifdef JOYSTICK
        if (g_digital.connected == 1)
        {
            joyL = g_digital.pressed.button.l;
            joyR = g_digital.pressed.button.r;
            joyA = g_digital.pressed.button.a;
            joyB = g_digital.pressed.button.b;
            joyStart = g_digital.pressed.button.start;
            
            oldjoyR = joyR;
            oldjoyL = joyL;                
            oldjoyA = joyA;  
            oldjoyB = joyB;  
        }   
        #endif
		
        #ifdef SCROLL_HORIZ
        // play with horiz normal screen scroll functions, params are integer+frac part
        vdp2_scrn_scv_x_set(SCRN_NBG0, lut[counterX], 0);
        counterX++;
        if(counterX >= 512) counterX = 0;            
        #endif
		
        #ifdef SCROLL_VERT
        // play with vert normal screen scroll functions, params are integer+frac part
        vdp2_scrn_scv_y_set(SCRN_NBG0, lut[counterY], 0);
        counterY += incrY;
        if(counterY >= 239) incrY=-1;            
        else if(counterY <= 0) incrY=1;   
        #endif
        
        
        #ifdef ZOOM
        // play with zoom
        if(joyL & !oldjoyL)
        {
            if(fracZoom > 0)
            {
                fracZoom -= 1;
            }
            else
            {
                if(intZoom > 0)
                {
                        intZoom -= 1;
                        fracZoom = 255;
                }    
            }
        }              
        else if(joyR & !oldjoyR)
        {                       
            if(fracZoom < 255)
            {
                fracZoom += 1;
            }
            else
            {
               if(intZoom < 4) 
                {
                    intZoom += 1;
                    fracZoom=0;
                }                 
            }
        }
        else if(joyA & !oldjoyA)
        {
            if(intZoom > 0)
            {
                intZoom += 1;
            }                     
        }
        else if(joyB & !oldjoyB)
        {
            if(intZoom < 5)
            {
                intZoom -= 1;
            }    

        }    
        vdp2_scrn_zm_x_set(SCRN_NBG0, intZoom, fracZoom);  
        vdp2_scrn_zm_y_set(SCRN_NBG0, intZoom, fracZoom);              
        #endif
            
        #ifdef SCROLLFN
        // play with line scroll function 
        // scroll values can be read each 1, 2, 4 or 8th line (size of "blocks" to move)
        // horizontal direction can be defined for each line with a integer part (11b) and a frac part (8b)
        // vertical direction can be defined for each line with a integer part (3b) and a frac part (8b)
        
        // lstau0 (0x00A0) = (ls->ls_lsta >> 17) & 0x0007 is 3bits upper address of scroll lines values
        // lstal0 (0x00A2) = (ls->ls_lsta >> 1) & 0xFFFF is 15 bits lower address of scroll lines values (18bits total)
        static uint16_t ofs = 0;
        linescrollfmt.ls_scrn = SCRN_NBG1;
        linescrollfmt.ls_lsta = (uint32_t)line_scroll_tb | (ofs & (2048 - 1));
        linescrollfmt.ls_int = 0;	//for interlace, not used in libyaul code
        linescrollfmt.ls_fun = SCRN_LS_N1SCX;	//enable Horiz line scroll, | SCRN_LS_N0SCY would also enables Vert lien scroll

        vdp2_scrn_ls_set(&linescrollfmt);
        ofs += 16;	
        #endif
		
        #ifdef JOYSTICK
        if(joyStart)
        {
            abort();
            oldjoyB = oldjoyA;
            oldjoyA = oldjoyL = oldjoyR = joyA = joyB = joyL = joyR = 0;
            oldjoyA = oldjoyB;
        }
        #endif
        
        vdp2_tvmd_vblank_in_wait();

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

void vdp2_scrn_zm_x_set(uint8_t scrn, uint16_t in, uint8_t dn)
{
#define ZMXIN0          0x0078
#define ZMXDN0          0x007A
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

void vdp2_scrn_zm_y_set(uint8_t scrn, uint16_t in, uint8_t dn)
{
#define ZMYIN0          0x007C
#define ZMYDN0          0x007E
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