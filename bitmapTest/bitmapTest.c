// Flat shaded cube demo for the Saturn
// Mic, 2006
// Ported by Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>

#include "bitmapTest.h"

struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

int sintb[256] = {0,25,50,75,100,125,150,175,199,224,248,273,297,321,344,368,391,414,437,460,482,504,526,547,568,589,609,629,649,668,687,706,
724,741,758,775,791,807,822,837,851,865,878,890,903,914,925,936,946,955,964,972,979,986,993,999,1004,1008,1012,1016,1019,1021,1022,1023,
1024,1023,1022,1021,1019,1016,1012,1008,1004,999,993,986,979,972,964,955,946,936,925,914,903,890,878,865,851,837,822,807,791,775,758,741,
724,706,687,668,649,629,609,589,568,547,526,504,482,460,437,414,391,368,344,321,297,273,248,224,199,175,150,125,100,75,50,25,
-1,-26,-51,-76,-101,-126,-151,-176,-200,-225,-249,-274,-298,-322,-345,-369,-392,-415,-438,-461,-483,-505,-527,-548,-569,-590,-610,-630,-650,-669,-688,-707,
-725,-742,-759,-776,-792,-808,-823,-838,-852,-866,-879,-891,-904,-915,-926,-937,-947,-956,-965,-973,-980,-987,-994,-1000,-1005,-1009,-1013,-1017,-1020,-1022,-1023,-1024,
-1024,-1024,-1023,-1022,-1020,-1017,-1013,-1009,-1005,-1000,-994,-987,-980,-973,-965,-956,-947,-937,-926,-915,-904,-891,-879,-866,-852,-838,-823,-808,-792,-776,-759,-742,
-725,-707,-688,-669,-650,-630,-610,-590,-569,-548,-527,-505,-483,-461,-438,-415,-392,-369,-345,-322,-298,-274,-249,-225,-200,-176,-151,-126,-101,-76,-51,-26};

/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);

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

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

    nbg0_format.sbf_scroll_screen = SCRN_NBG0; /* Normal/rotational background */
    nbg0_format.sbf_cc_count = SCRN_CCC_PALETTE_16; /* color mode */
    nbg0_format.sbf_bitmap_size.width = 512; /* Bitmap sizes: 512x256, 512x512, 1024x256, 1024x512 */
    nbg0_format.sbf_bitmap_size.height = 256;
    nbg0_format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000); /* Bitmap pattern lead address */

    vdp2_scrn_bitmap_format_set(&nbg0_format);
    vdp2_priority_spn_set(SCRN_NBG0, 7);

    MEMORY_WRITE(16, VDP2(RAMCTL), 0x1300);

    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
    vdp2_vram_control_set(vram_ctl);


    /* Copy the bitmap data */
    memcpy((void *)VRAM_ADDR_4MBIT(0, 0x00000), bitmap_data, sizeof(bitmap_data));   

    /* Copy the palette data */
    memcpy(_nbg0_color_palette, bitmap_palette, sizeof(bitmap_palette));    

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);
    vdp2_tvmd_display_set();

}

int main(void)
{
	hardware_init();
    initScrollScreens();

	static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };
	vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

	/* Main loop */
	for(;;)
	{
	  	vdp2_tvmd_vblank_out_wait();

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