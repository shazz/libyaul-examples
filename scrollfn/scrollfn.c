// Screen Scroll tests
// 
// Shazz on libyaul, 2015

// play with line scroll function 
// scroll values can be read each 1, 2, 4 or 8th line (size of "blocks" to move)
// horizontal direction can be defined for each line with a integer part (11b) and a frac part (8b)
// vertical direction can be defined for each line with a integer part (3b) and a frac part (8b)

// lstau0 (0x00A0) = (ls->ls_lsta >> 17) & 0x0007 is 3bits upper address of scroll lines values
// lstal0 (0x00A2) = (ls->ls_lsta >> 1) & 0xFFFF is 15 bits lower address of scroll lines values (18bits total)

// enable : SCRN_LS_N0SCX | SCRN_LS_N0SCY | N0LSS0 | N0LSS1 | N0LZMX        

#define N0LSS0 (1 << 12)
#define N0LSS1 (1 << 13) //enable 8 lines block - 00, 01 10 11 for 1,2, 4 or 8 lines based on non-interlaced settings

#include <yaul.h>
#include <stdlib.h>

#include "logo.h"
#include "tables.h"

struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

#define CRAM_PAL_MODE_OFFSET(bank, nb)     (0x25F00000 + ((bank) << 9) +  ((nb) << 4))
#define NBG0_VRAM_BANK_OFFSET 0
#define NBG0_VRAM_PAL_NB 0
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_PAL_MODE_OFFSET(NBG0_VRAM_BANK_OFFSET, NBG0_VRAM_PAL_NB);

/* Line Scroll table address */
static uint32_t *line_scroll_tb = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x1F000);
struct scrn_ls_format linescrollfmt;

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
    struct vram_ctl *vram_ctl;
    uint16_t bmpma;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

    nbg0_format.sbf_scroll_screen = SCRN_NBG0;                      /* Normal/rotational background */
    nbg0_format.sbf_cc_count = SCRN_CCC_PALETTE_16;                 /* color mode */
    nbg0_format.sbf_bitmap_size.width = 512;                        /* Bitmap sizes: 512x256, 512x512, 1024x256, 1024x512 */
    nbg0_format.sbf_bitmap_size.height = 256;
    nbg0_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);   /* Bitmap pattern lead address */
    nbg0_format.sbf_color_palette = (uint32_t)_nbg0_color_palette;

    vdp2_scrn_bitmap_format_set(&nbg0_format);
    
    // then set CRAOFA and BMPNA (again) for NBG0
    MEMORY_WRITE(16, VDP2(CRAOFA), (NBG0_VRAM_BANK_OFFSET & 0x7));
    bmpma = MEMORY_READ(16, VDP2(BMPNA));
    bmpma |= ((NBG0_VRAM_PAL_NB & 0x7 ) << 0);
    MEMORY_WRITE(16, VDP2(BMPNA), bmpma);
    
    // set  priority for NB0 to be backward
    vdp2_priority_spn_set(SCRN_NBG0, 7);
     
    // Set Color mode to mode 0 (1KWord Color RAM mirrored)
    MEMORY_WRITE(16, VDP2(RAMCTL), 0x300);

    // 16 colors bitmap and Vertical cell scroll requires 1 access each
    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_VCSTDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_VCSTDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);
	
	// copy/init line scroll table in VRAM
	uint16_t y;
	for (y = 0; y < 256; y++) 
	{
        // set 32bits values, int part in high word
        // ex1 : reverse image line_scroll_tb[y] = (200-y) << 16;
        // ex2 : odd lines line_scroll_tb[y] = (2*y) << 16;
        // ex3 : line_scroll_tb[y]  = 0;
        line_scroll_tb[y]  = 0;
	}
    
    linescrollfmt.ls_scrn = SCRN_NBG0;
    linescrollfmt.ls_lsta = (uint32_t)line_scroll_tb;
    linescrollfmt.ls_int = 0;	                //for interlace, not used in libyaul code
    linescrollfmt.ls_fun = SCRN_LS_N0SCY ;	    //enable vert line scroll only
    vdp2_scrn_ls_set(&linescrollfmt);
    
    /* Copy the bitmap and BGR555 palette data */
    memcpy((void *)VRAM_ADDR_4MBIT(0, 0x00000), bitmap_data, sizeof(bitmap_data));   
    memcpy(_nbg0_color_palette, bitmap_palette, sizeof(bitmap_palette));    

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);
    vdp2_tvmd_display_set();
}

int main(void)
{
	hardware_init();
    initScrollScreens();

	static uint16_t back_screen_color[] = { COLOR_RGB_DATA | 0x0842 };
	vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

    uint16_t line = 199;
    uint16_t currentLinePos = 0;
    const uint16_t speed = 10;
    
	/* Main loop */
	for(;;)
	{
        /* Wait for next vblank */
		vdp2_tvmd_vblank_out_wait();
        
        uint16_t y;
        // erase top screen
        for (y = 0; y < line-1; y++) 
        {
                line_scroll_tb[y]  = 255 << 16;   //erase, nothing after y = 199
        }  
        
        // show one line moving down
        if(currentLinePos+speed < line)
        {
            currentLinePos+=speed;
            line_scroll_tb[currentLinePos] = line << 16;    
        }
        else    
        {
            line_scroll_tb[line] = line << 16; 
            currentLinePos = 0;
            if(line > 1 )line--;        
        }
            
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
