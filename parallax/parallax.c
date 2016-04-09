#include <yaul.h>
#include <langam.h>

#include <stdlib.h>

#include "back1.h"
#include "back2.h"
#include "back3.h"
#include "back4.h"
#include "sprite.h"
#include "boss.h"
#include "plasma.h"

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

// define 4 scroll screens of 2 planes pages
// each plane needs 4096 u16 (512x512) or 0x2000 bytes (8192)
static uint16_t *_nbg0_planes[4] = { /* VRAM A0 */
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x2000),
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0),    //not used
        (uint16_t *)VRAM_ADDR_4MBIT(0, 0x0)     //not used
}; 

static uint16_t *_nbg1_planes[4] = { /* VRAM A1 */
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x2000),
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0),    //not used
        (uint16_t *)VRAM_ADDR_4MBIT(1, 0x0)     //not used
}; 

static uint16_t *_nbg2_planes[4] = { /* VRAM B0 */
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x2000),
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0),    //not used
        (uint16_t *)VRAM_ADDR_4MBIT(2, 0x0)     //not used
}; 

static uint16_t *_nbg3_planes[4] = { /* VRAM B1 */
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0),
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x2000),
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0),     //not used
        (uint16_t *)VRAM_ADDR_4MBIT(3, 0x0)     //not used
};

/* VRAM A0 after planes, CRAM */
static uint32_t *_nbg0_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x4000);
static uint32_t *_nbg0_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(0, 0, 0);

/* VRAM A1 after planes, CRAM */
static uint32_t *_nbg1_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(1, 0x4000);
static uint32_t *_nbg1_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(1, 0, 0);

/* VRAM B0 after planes, CRAM */
static uint32_t *_nbg2_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(2, 0x4000);
static uint32_t *_nbg2_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(2, 0, 0);

/* VRAM B1 after planes, CRAM */
static uint32_t *_nbg3_cell_data = (uint32_t *)VRAM_ADDR_4MBIT(3, 0x4000);
static uint32_t *_nbg3_color_palette = (uint32_t *)CRAM_MODE_1_OFFSET(3, 0, 0);

// sprites 
static uint32_t _spaceship_char_pat_data = (uint32_t)CHAR(0x220);  
static uint32_t _boss_char_pat_data = (uint32_t)CHAR(0x1020);
static uint32_t _plasma_char_pat_data = (uint32_t)CHAR(0xD2E0);

struct vdp1_cmdt_sprite normal_sprite_spaceship_pointer; 
struct vdp1_cmdt_sprite normal_sprite_boss_pointer; 
struct vdp1_cmdt_sprite normal_sprite_plasma_pointer; 
struct vdp1_cmdt_system_clip_coord system_clip;
struct vdp1_cmdt_user_clip_coord user_clip;
struct vdp1_cmdt_local_coord local;

// joypad
static unsigned int joyLeft = 0, joyRight = 0, joyUp = 0, joyDown = 0, joyA = 0;    
    
static int16_t g_scroll_sprite_x = 0;   
static int16_t g_scroll_sprite_y = 10;   
static bool g_button_a_is_pushed = false;
static int16_t g_button_a_is_pushed_x = -40;   

static bool g_show_plasma = false;
static bool g_show_boss = true;

/*
 * void init_scrollscreen_nbg(int screen, uint16_t *planes[], uint32_t *cell_data_ptr, uint32_t *palette_ptr, uint16_t page0_data[], uint16_t page1_data[], uint8_t priority, bool transparent) 
 * 
 * 
 */
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
  
    uint16_t palette_number = VDP2_PN_CONFIG_0_PALETTE_NUMBER((uint32_t)palette_ptr);
    uint16_t cell_data_number = VDP2_PN_CONFIG_1_CHARACTER_NUMBER((uint32_t)cell_data_ptr);

	for (i = 0; i < 4096; i++) 
    {
        uint16_t cell_data_number0 = cell_data_number + page0_data[i];
        uint16_t cell_data_number1 = cell_data_number + page1_data[i];
        nbg_page0[i] = cell_data_number0 | palette_number;
        nbg_page1[i] = cell_data_number1 | palette_number;
	}

    vdp2_scrn_display_set(screen, transparent);
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

void init_vdp2_scrollescreens(void)
{
    /* DMA Indirect list, aligned on 64 bytes due to more than 24bytes size (6*4*3=72) */
    uint32_t dma_tbl[] __attribute__((aligned(128))) = { 
            (uint32_t)sizeof(NGB0_CD), (uint32_t)_nbg0_cell_data, (uint32_t)NGB0_CD, 
            (uint32_t)sizeof(NGB0_CP), (uint32_t)_nbg0_color_palette, (uint32_t)NGB0_CP, 
            (uint32_t)sizeof(NGB1_CD), (uint32_t)_nbg1_cell_data, (uint32_t)NGB1_CD, 
            (uint32_t)sizeof(NGB1_CP), (uint32_t)_nbg1_color_palette, (uint32_t)NGB1_CP, 
            (uint32_t)sizeof(NGB2_CD), (uint32_t)_nbg2_cell_data, (uint32_t)NGB2_CD, 
            (uint32_t)sizeof(NGB2_CP), (uint32_t)_nbg2_color_palette, (uint32_t)NGB2_CP, 
            (uint32_t)sizeof(NGB3_CD), (uint32_t)_nbg3_cell_data, (uint32_t)NGB3_CD, 
            (uint32_t)sizeof(NGB3_CP), (uint32_t)_nbg3_color_palette, (uint32_t)NGB3_CP,
            (uint32_t)sizeof(spaceship_char_pat), _spaceship_char_pat_data, (uint32_t)spaceship_char_pat,
            (uint32_t)sizeof(plasma_char_pat), _plasma_char_pat_data, (uint32_t)plasma_char_pat,            
            (uint32_t)sizeof(boss_ecd_char_pat), _boss_char_pat_data, (uint32_t)boss_ecd_char_pat
    };    
    scu_dma_listcpy(dma_tbl, 11*3);
    while(scu_dma_get_status(SCU_DMA_ALL_CH) == SCU_DMA_STATUS_WAIT);
    
    set_VRAM_access_priorities();
    
    init_scrollscreen_nbg(SCRN_NBG0, _nbg0_planes, _nbg0_cell_data, _nbg0_color_palette, NGB0_PNT_PLANE0, NGB0_PNT_PLANE1, 7, true);
    init_scrollscreen_nbg(SCRN_NBG1, _nbg1_planes, _nbg1_cell_data, _nbg1_color_palette, NGB1_PNT_PLANE0, NGB1_PNT_PLANE1, 3, true);
    init_scrollscreen_nbg(SCRN_NBG2, _nbg2_planes, _nbg2_cell_data, _nbg2_color_palette, NGB2_PNT_PLANE0, NGB2_PNT_PLANE1, 2, true);
    init_scrollscreen_nbg(SCRN_NBG3, _nbg3_planes, _nbg3_cell_data, _nbg3_color_palette, NGB3_PNT_PLANE0, NGB3_PNT_PLANE1, 1, false);
     
    vdp2_tvmd_display_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_240); 
}

void read_digital_pad(void)
{
	if (g_digital.connected == 1)
	{
		joyUp = g_digital.pressed.button.up;
		joyDown = g_digital.pressed.button.down;
		joyRight = g_digital.pressed.button.right;                            
		joyLeft = g_digital.pressed.button.left;
        joyA = g_digital.pressed.button.a;
        
		if (joyUp)
		{
            if(g_scroll_sprite_y>0) g_scroll_sprite_y -= 2;             
		}
		else if (joyDown)
		{
            if(g_scroll_sprite_y<224) g_scroll_sprite_y += 2;
		}
        if (joyRight)
		{
            if(g_scroll_sprite_x<320) g_scroll_sprite_x += 2;
		}
		else if (joyLeft)
		{
            if(g_scroll_sprite_x>0) g_scroll_sprite_x -= 2;  			
		}
		
        if (joyA)
		{
            g_button_a_is_pushed = true;
            g_show_plasma = true;
            g_button_a_is_pushed_x = g_scroll_sprite_x + 48;
		}        
		
		// exit
		if(g_digital.pressed.button.start) abort();		
	}  
}

void check_collisions()
{
    //if(collide_bounding_circle(&normal_sprite_boss_pointer, &normal_sprite_plasma_pointer, 10) != 0) return false;
    //if(collide_bounding_box(&normal_sprite_boss_pointer, &normal_sprite_plasma_pointer) != 0) return false;
    
    if(g_show_plasma && g_show_boss)
    {
        if(collide_pixel(&normal_sprite_boss_pointer, &normal_sprite_plasma_pointer, 4) != 0)
        { 
            g_show_boss = false;
            g_button_a_is_pushed = false;
        }
    }
}

/*
 * void update_plasma(void)
 * set plasma position and decide to show or not
 */ 
void update_plasma(void)
{
    if(g_button_a_is_pushed)
    {
        if(normal_sprite_plasma_pointer.cs_position.x  < 0)
        {
            normal_sprite_plasma_pointer.cs_position.x = g_button_a_is_pushed_x;
            normal_sprite_plasma_pointer.cs_position.y = g_scroll_sprite_y;
        }
        else normal_sprite_plasma_pointer.cs_position.x += 6;

        if(normal_sprite_plasma_pointer.cs_position.x >= 320)
        {
            g_button_a_is_pushed = false;
            normal_sprite_plasma_pointer.cs_position.x  = -40;
            g_show_plasma = false;
        }
        g_show_plasma = true;
    } 
    else
    {
        g_show_plasma = false;
        normal_sprite_plasma_pointer.cs_position.x  = -40;
    }
}       

void update_boss(void)
{  
    if(!g_show_boss)
    {
        normal_sprite_boss_pointer.cs_position.x = 340;
        normal_sprite_boss_pointer.cs_position.y = 80;       
        g_show_boss = true;
    }
    
    int16_t spaceship_mid_y = g_scroll_sprite_y + normal_sprite_spaceship_pointer.cs_height/2;
    int16_t boss_mid_y = normal_sprite_boss_pointer.cs_position.y + normal_sprite_boss_pointer.cs_height/2;
    
    if(spaceship_mid_y < boss_mid_y) normal_sprite_boss_pointer.cs_position.y--;
    if(spaceship_mid_y > boss_mid_y) normal_sprite_boss_pointer.cs_position.y++;
    if(normal_sprite_boss_pointer.cs_position.x - normal_sprite_spaceship_pointer.cs_width - g_scroll_sprite_x < 50)
        normal_sprite_boss_pointer.cs_position.x++;
    else if(normal_sprite_boss_pointer.cs_position.x - normal_sprite_spaceship_pointer.cs_width - g_scroll_sprite_x > 50) 
        normal_sprite_boss_pointer.cs_position.x--;
        
}

void init_vdp1_sprites(void)
{
    vdp1_cmdt_list_init();
    vdp1_cmdt_list_clear_all();
    
    // set coord systems
    system_clip.scc_coord.x = 320 - 1;
    system_clip.scc_coord.y = 224 - 1;
    
    user_clip.ucc_coords[0].x = 0;
    user_clip.ucc_coords[0].y = 0;
    user_clip.ucc_coords[1].x = 320 - 1;
    user_clip.ucc_coords[1].y = 224 - 1;

    local.lc_coord.x = 0;
    local.lc_coord.y = 0;
        
    g_scroll_sprite_x = 20;
    g_scroll_sprite_y = 100; 
    
    memset(&normal_sprite_spaceship_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));
    normal_sprite_spaceship_pointer.cs_type = CMDT_TYPE_NORMAL_SPRITE;
    normal_sprite_spaceship_pointer.cs_mode.cc_mode = 0x0; // no gouraud or any cc
    normal_sprite_spaceship_pointer.cs_mode.color_mode = 5; 	// mode 5 RGB    
    normal_sprite_spaceship_pointer.cs_mode.mesh = 0;
    normal_sprite_spaceship_pointer.cs_mode.user_clipping = 1;
    normal_sprite_spaceship_pointer.cs_mode.end_code = 0;
    normal_sprite_spaceship_pointer.cs_width = 48; // multiple of 8
    normal_sprite_spaceship_pointer.cs_height = 24; // multiple of 1
    normal_sprite_spaceship_pointer.cs_position.x = g_scroll_sprite_x;
    normal_sprite_spaceship_pointer.cs_position.y = g_scroll_sprite_y;
    normal_sprite_spaceship_pointer.cs_mode.transparent_pixel = 0;
    normal_sprite_spaceship_pointer.cs_char = _spaceship_char_pat_data;    
    
    memset(&normal_sprite_boss_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));
    normal_sprite_boss_pointer.cs_type = CMDT_TYPE_NORMAL_SPRITE;
    normal_sprite_boss_pointer.cs_mode.cc_mode = 0x0; // no gouraud or any cc
    normal_sprite_boss_pointer.cs_mode.color_mode = 5; 	// mode 5 RGB
    normal_sprite_boss_pointer.cs_mode.mesh = 0;
    normal_sprite_boss_pointer.cs_mode.user_clipping = 1;
    normal_sprite_boss_pointer.cs_mode.end_code = 1;
    normal_sprite_boss_pointer.cs_mode.transparent_pixel = 1;    
    normal_sprite_boss_pointer.cs_width = 154; // multiple of 8
    normal_sprite_boss_pointer.cs_height = 82; // multiple of 1
    normal_sprite_boss_pointer.cs_position.x = 320;
    normal_sprite_boss_pointer.cs_position.y = 80;
    normal_sprite_boss_pointer.cs_char = _boss_char_pat_data;        
  
    memset(&normal_sprite_plasma_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));
    normal_sprite_plasma_pointer.cs_type = CMDT_TYPE_NORMAL_SPRITE;
    normal_sprite_plasma_pointer.cs_mode.cc_mode = 0x0; // no gouraud or any cc
    normal_sprite_plasma_pointer.cs_mode.color_mode = 5; 	// mode 5 RGB
    normal_sprite_plasma_pointer.cs_mode.mesh = 0;
    normal_sprite_plasma_pointer.cs_mode.user_clipping = 1;
    normal_sprite_plasma_pointer.cs_mode.end_code = 1;
    normal_sprite_plasma_pointer.cs_mode.transparent_pixel = 1;    
    normal_sprite_plasma_pointer.cs_width = 80; // multiple of 8
    normal_sprite_plasma_pointer.cs_height = 16; // multiple of 1
    normal_sprite_plasma_pointer.cs_position.x = -80;
    normal_sprite_plasma_pointer.cs_position.y = 0;
    normal_sprite_plasma_pointer.cs_char = _plasma_char_pat_data;     

    // Set Sprite priorities on VDP2
    MEMORY_WRITE(16, VDP2(SPCTL), (1 << 5));
    MEMORY_WRITE(16, VDP2(PRISA), 0x6);    
}

int main(void)
{
    uint32_t padButton = 0;
    uint32_t g_scroll_back4_x = 0, g_scroll_back3_x = 0, g_scroll_back2_x = 0, g_scroll_back1_x = 0;
    int16_t g_scroll_back1_y = 33, g_scroll_back2_y = 33, g_scroll_back3_y = 33, g_scroll_back4_y = 33;

    irq_mux_t *vblank_in;
    irq_mux_t *vblank_out;
    
    static uint16_t back_screen_color = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 0) };

    vdp2_init();
    /* set 320x240 res, back color mode */    
    MEMORY_WRITE(16, VDP2(TVMD), 0x8110);        
    
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
    
    init_vdp1_sprites();
   
	vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color);  
    
	/* Main loop */
	while (!padButton) 
	{
        read_digital_pad(); 
                
	  	vdp2_tvmd_vblank_in_wait();
    
        // update positions
        vdp2_scrn_scv_x_set(SCRN_NBG0, g_scroll_back4_x, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG1, g_scroll_back3_x, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG2, g_scroll_back2_x, 0);
        vdp2_scrn_scv_x_set(SCRN_NBG3, g_scroll_back1_x, 0);
        
        vdp2_scrn_scv_y_set(SCRN_NBG0, g_scroll_back4_y, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG1, g_scroll_back3_y, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG2, g_scroll_back2_y, 0);
        vdp2_scrn_scv_y_set(SCRN_NBG3, g_scroll_back1_y, 0);        
        
        g_scroll_back4_x = (g_scroll_back4_x + 6) % 1024;
        g_scroll_back3_x = (g_scroll_back3_x + 4) % 1024;
        g_scroll_back2_x = (g_scroll_back2_x + 2) % 1024;
        g_scroll_back1_x = (g_scroll_back1_x + 1) % 1024;
        
        // move parallax y against spaceship y
        g_scroll_back1_y = 33 + ( g_scroll_sprite_y - (240/2)) / 6;
        g_scroll_back2_y = 33 + ( g_scroll_sprite_y - (240/2)) / 8;
        g_scroll_back3_y = 33 + ( g_scroll_sprite_y - (240/2)) / 8;
        g_scroll_back4_y = 33 + ( g_scroll_sprite_y - (240/2)) / 9;
        
        check_collisions();
        update_plasma();
        update_boss();
        
        // show sprite
        vdp1_cmdt_list_begin(0); 
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            normal_sprite_spaceship_pointer.cs_position.x = g_scroll_sprite_x;
            normal_sprite_spaceship_pointer.cs_position.y = g_scroll_sprite_y;
                        
            vdp1_cmdt_sprite_draw(&normal_sprite_spaceship_pointer);
            if(g_show_plasma)     vdp1_cmdt_sprite_draw(&normal_sprite_plasma_pointer);            
            if(g_show_boss)       vdp1_cmdt_sprite_draw(&normal_sprite_boss_pointer);

            vdp1_cmdt_end();
        } 
        vdp1_cmdt_list_end(0);
        vdp2_tvmd_vblank_in_wait();
        vdp1_cmdt_list_commit();           
        
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
    
    if(tick % 10 == 0)
    {
        // change sprite
        if(normal_sprite_boss_pointer.cs_char == _boss_char_pat_data)
        {
            normal_sprite_boss_pointer.cs_char = _boss_char_pat_data + 24928;  
        }
        else
        {
           normal_sprite_boss_pointer.cs_char = _boss_char_pat_data;
        }   
    } 
}