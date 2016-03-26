#include <yaul.h>
#include <stdlib.h>

#define PN_2x1_2WORDS_256
//#define PN_2x1_1WORD_AUX_256
//#define PN_2x1_1WORD_NOAUX_256
//#define PN_2x1_1WORD_AUX_16
//#define PN_2x1_1WORD_NOAUX_16
//#define PN_1x1_1WORD_AUX_256
//#define PN_1x1_1WORD_NOAUX_256

#ifdef PN_2x1_2WORDS_256
#include "test3.h"
#define CP_ADDRESS 0x6000
#define CP_BANK	   0
// doesn't work for bank 0, messy after some pixels on the right
#endif

#ifdef PN_2x1_1WORD_AUX_256
#include "test.h"
#define CP_ADDRESS 0x0000
#define CP_BANK    2 // not working
#endif

#ifdef PN_2x1_1WORD_NOAUX_256
#include "test.h"
#define CP_ADDRESS 0x8000
#define CP_BANK    2 // not working
#endif

#ifdef PN_2x1_1WORD_AUX_16
#include "test2.h"
#define CP_ADDRESS 0x4000
#define CP_BANK    0 // working
#endif

#ifdef PN_2x1_1WORD_NOAUX_16
#include "test2.h"
#define CP_ADDRESS 0x1000
#define CP_BANK    1 // working 1/3 then messy
#endif

#if defined(PN_1x1_1WORD_AUX_256)
#include "test4.h"
#define CP_ADDRESS 0x1000
#define CP_BANK    0
#endif

#if defined(PN_1x1_1WORD_NOAUX_256)
#include "test4.h"
#define CP_ADDRESS 0x0000
#define CP_BANK    1 // working 1/3 then messy, needs VRAM_CTL_CYCP_CHPNDR_NBG0
#endif

/*
 * VDP2 VRAM Organization
 * Bank A0
 * - NBG0  2 planes data, from 0x0 to 0x04000 (2x8192 bytes, 1 word per cell, 512x512px)
 * - NBG0 cell pattern data, from 0x04000 to 0x1BC00 (56704 bytes, 3040 8x8 cells in 8bits color)
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



struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

static uint16_t *_nbg0_planes[4] = {
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0)
}; // a plane needs 4096 u16 (512x512) or 0x2000 bytes (4096)

/* VRAM B0 after plane */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS);
/* CRAM */
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);

void init_scrollscreen_nbg0(void)
{
    /* We want to be in VBLANK */
    vdp2_tvmd_display_clear();

    /* set NBG0 in cell mode, 256 col, 1x1, 2px1p */
    struct scrn_cell_format nbg0_format;
    struct vram_ctl *vram_ctl;
    uint32_t i;

    /* We want to be in VBLANK-IN (retrace) */
    vdp2_tvmd_display_clear();

	nbg0_format.scf_scroll_screen = SCRN_NBG0;
    nbg0_format.scf_character_size= 1 * 1;

#if defined(PN_2x1_1WORD_AUX_16) || defined(PN_2x1_1WORD_NOAUX_16)
	nbg0_format.scf_cc_count = SCRN_CCC_PALETTE_16;
#else
	nbg0_format.scf_cc_count = SCRN_CCC_PALETTE_256;
#endif

#ifdef PN_2x1_2WORDS_256
	nbg0_format.scf_pnd_size = 2; /* 2 word */
#else
	nbg0_format.scf_pnd_size = 1; /* 1 word */
#endif

#if defined(PN_2x1_1WORD_AUX_256) || defined(PN_2x1_1WORD_AUX_16) || defined(PN_1x1_1WORD_AUX_256)
	nbg0_format.scf_auxiliary_mode = 1;
#else
	nbg0_format.scf_auxiliary_mode = 0;
#endif

	nbg0_format.scf_cp_table = (uint32_t)_nbg0_cell_data;
	nbg0_format.scf_color_palette = (uint32_t)_nbg0_color_palette;
#if defined(PN_1x1_1WORD_AUX_256) || defined(PN_1x1_1WORD_NOAUX_256)
	nbg0_format.scf_plane_size = 1 * 1;
#else
	nbg0_format.scf_plane_size = 2 * 1;
#endif

	nbg0_format.scf_map.plane_a = (uint32_t)_nbg0_planes[0];
	nbg0_format.scf_map.plane_b = (uint32_t)_nbg0_planes[1];
	nbg0_format.scf_map.plane_c = (uint32_t)_nbg0_planes[2];
	nbg0_format.scf_map.plane_d = (uint32_t)_nbg0_planes[3];

	vdp2_scrn_cell_format_set(&nbg0_format);
    vdp2_priority_spn_set(SCRN_NBG0, 7);

    vram_ctl = vdp2_vram_control_get();
    vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_PNDR_NBG0;
    vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

    
    vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;

/*
    vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_CHPNDR_NBG0;
   
    vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
    vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
    vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;    
      */ 


    vdp2_vram_control_set(vram_ctl);

    /* Copy the palette data */
    memcpy(_nbg0_color_palette, test_cell_palette, sizeof(test_cell_palette));

    /* Copy the cell data */
    memcpy(_nbg0_cell_data, test_cell_data, sizeof(test_cell_data));

    //uint16_t character_number = nbg0_format.scf_cp_table >> 5;
    //uint16_t sc_number = (character_number & 0x7C00) >> 10;   /* Supplementary char number bits */

    // set 1 word pn, aux mode on and add Supplementary character number bit (5 bits, 2 last ignorted)
    //uint16_t pncn0 = 0xC000 | sc_number;
    //MEMORY_WRITE(16, VDP2(PNCN0), pncn0);

#ifdef PN_2x1_1WORD_AUX_256
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_2x1_1WORD_NOAUX_256
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_0_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_2x1_2WORDS_256
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_4_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_2_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_2x1_1WORD_AUX_16
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_2x1_1WORD_NOAUX_16
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_0_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_1x1_1WORD_AUX_256
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif
#ifdef PN_1x1_1WORD_NOAUX_256
    uint16_t _nbg0_cell_data_number = VDP2_PN_CONFIG_0_CHARACTER_NUMBER((uint32_t)VRAM_ADDR_4MBIT(CP_BANK, CP_ADDRESS));
    uint16_t _nbg0_palette_number = VDP2_PN_CONFIG_1_PALETTE_NUMBER(CRAM_MODE_1_OFFSET(0, 0, 0));
#endif

    /* Build the pattern data */
#if defined(PN_1x1_1WORD_AUX_256) || defined(PN_1x1_1WORD_NOAUX_256)
    uint16_t *nbg0_page0 = _nbg0_planes[0];
	for (i = 0; i < 2048; i++)
    {
        uint16_t cell_data_number = _nbg0_cell_data_number + test_pattern_name_table_page0[i];
        nbg0_page0[i] = cell_data_number | _nbg0_palette_number;
	}
#elif defined(PN_2x1_2WORDS_256)
    uint32_t *nbg0_page0 = (uint32_t*)_nbg0_planes[0];
    uint32_t *nbg0_page1 = (uint32_t*) &_nbg0_planes[0][64*64];
	for (i = 0; i < 4096; i++)
    {
        uint32_t cell_data_number0 = _nbg0_cell_data_number + test_pattern_name_table_page0[i];
        uint32_t cell_data_number1 = _nbg0_cell_data_number + test_pattern_name_table_page1[i];
        nbg0_page0[i] = cell_data_number0 | _nbg0_palette_number;
        nbg0_page1[i] = cell_data_number1 | _nbg0_palette_number;
	}
#else
    uint16_t *nbg0_page0 = _nbg0_planes[0];
    uint16_t *nbg0_page1 = (uint16_t*) &_nbg0_planes[0][64*64];
	for (i = 0; i < 4096; i++)
    {
        uint16_t cell_data_number0 = _nbg0_cell_data_number + test_pattern_name_table_page0[i];
        uint16_t cell_data_number1 = _nbg0_cell_data_number + test_pattern_name_table_page1[i];
        nbg0_page0[i] = cell_data_number0 | _nbg0_palette_number;
        nbg0_page1[i] = cell_data_number1 | _nbg0_palette_number;
	}
#endif

    vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);
}

int main(void)
{
    irq_mux_t *vblank_in;
    irq_mux_t *vblank_out;
    uint32_t g_scroll_back4_x = 0;
	static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };

    vdp2_init();
    vdp1_init();
    smpc_init();
    smpc_peripheral_init();

    cpu_intc_disable();
    vblank_in = vdp2_tvmd_vblank_in_irq_get();
    irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
    vblank_out = vdp2_tvmd_vblank_out_irq_get();
    irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
    cpu_intc_enable();

    init_scrollscreen_nbg0();

    vdp2_tvmd_display_set();

	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);

	/* Main loop */
	while (true)
	{
	  	vdp2_tvmd_vblank_in_wait();

        //g_scroll_back4_x++;
        vdp2_scrn_scv_x_set(SCRN_NBG0, g_scroll_back4_x, 0);
#if !defined(PN_1x1_1WORD_AUX_256) && !defined(PN_1x1_1WORD_NOAUX_256)
        vdp2_scrn_scv_y_set(SCRN_NBG0, 128, 0);
#endif
        if(g_digital.pressed.button.start) abort();
        if(g_digital.pressed.button.right) g_scroll_back4_x++;
        if(g_digital.pressed.button.left) g_scroll_back4_x--;

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
