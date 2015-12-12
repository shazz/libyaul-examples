/*
    Sega Saturn VDP1 example program
    by Charles MacDonald
    WWW: http://cgfm2.emuviews.com

    Based on the bmp8bpp.zip and startup.zip packages written by
    Bart Trzynadlowski. (www.dynarec.com/~bart)

    Uses the SH-COFF toolchain from KPIT Infosystems. (www.kpit.com)
    Load and run at address $06004000.

    Thanks to Stefano for help getting the VDP1 graphics working.

    Controls:

    Start - return to cheat cartridge menu
    A - Bit 2 of TVM field in VDP1 TVMD register
    B - Bit 1 of TVM field in VDP1 TVMD register
    C - Bit 0 of TVM field in VDP1 TVMD register
    X - Stop frame erase
    Y - DIE bit of VDP1 FBCR register
    Z - DIL bit of VDP1 FBCR register
    L - Enable double density interlace
    R - Enable 640 pixel mode
    
    Ported by Shazz on libyaul, 2015
*/

#include <yaul.h>
#include <stdlib.h>

/* Texture map size */
#define TX_W    128
#define TX_H    128
/* Point direction flags */
#define DIR_V       1
#define DIR_H       2
#define MESH(n) (mesh_flag=(n)?1:0)

/* Coordinate structure */
typedef struct { int16_t x,y; } point_t;

/* Describe a single point position and movement */
typedef struct {
    point_t c[4];
    uint8_t dir[4];
} object_t;

struct smpc_peripheral_digital g_digital;

/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

uint16_t mesh_flag = 0;   /* Global mesh enable flag */

/* Move four points of an object */
void update_pos(object_t *p)
{
    int i;
    for(i = 0; i < 4; i++)
    {
        if(p->dir[i] & DIR_V)
            p->c[i].y--;
        else
            p->c[i].y++;

        if(p->dir[i] & DIR_H)
            p->c[i].x--;
        else
            p->c[i].x++;

        if((p->c[i].y < 0) || (p->c[i].y > 224))
            p->dir[i] ^= DIR_V;

        if((p->c[i].x < 0) || (p->c[i].x > 320))
            p->dir[i] ^= DIR_H;
    }
}

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

int main()
{
    unsigned int joyA = 0, joyB = 0, joyC = 0, joyX = 0, joyY = 0, joyZ = 0, joyL = 0, joyR = 0, joyStart = 0;
    unsigned int oldjoyA = 0, oldjoyB = 0, oldjoyC = 0, oldjoyX = 0, oldjoyY = 0, oldjoyZ = 0, oldjoyL = 0, oldjoyR = 0, oldjoyStart = 0;
    uint16_t fbcr = 0x0001;
    int x, y, frame_count = 0;
    object_t foo[4];
    
	uint16_t * pGour, * p;

    hardware_init();
    
    // needed ?
    MEMORY_WRITE(16, VDP1(PTMR), 0x0002);
    MEMORY_WRITE(16, VDP1(EWDR), COLOR_RGB_DATA | COLOR_RGB555(7,7,7));
    
    static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(7, 7, 7) };
    vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);    

    /* Make texture map */
    p = (uint16_t *)CHAR(0);
    for (y=0; y<TX_H; y++)
    {
        for (x=0; x<TX_W; x++)
        {
            *(p++) = COLOR_RGB_DATA | COLOR_RGB555((x*x-y*y)>>4,x^y,(x*x+y*y)>>6); //COLOR_RGB_DATA | ((y^x)&31);
        }
    }
      
   	/* Make Gouraud shading table */
    pGour = (uint16_t *) GOURAUD(0, 0);
    pGour[0] = COLOR_RGB_DATA | COLOR_RGB555(31,0,0);
    pGour[1] = COLOR_RGB_DATA | COLOR_RGB555(0,31,0);
    pGour[2] = COLOR_RGB_DATA | COLOR_RGB555(0,0,31);
    pGour[3] = COLOR_RGB_DATA | COLOR_RGB555(31,31,31);

    /* Initialize object positions */
    for(y = 0; y < 4; y++)
    for(x = 0; x < 4; x++)
    {
        foo[y].c[x].x = rand() % 320;
        foo[y].c[x].y = rand() % 224;
        foo[y].dir[x] = rand() & 3;
    }
    
    vdp1_cmdt_list_init();
    vdp1_cmdt_list_clear_all();
    
    struct vdp1_cmdt_system_clip_coord system_clip;
    system_clip.scc_coord.x = 320 - 1;
    system_clip.scc_coord.y = 224 - 1;

    struct vdp1_cmdt_user_clip_coord user_clip;
    user_clip.ucc_coords[0].x = 0;
    user_clip.ucc_coords[0].y = 0;
    user_clip.ucc_coords[1].x = 320 - 1;
    user_clip.ucc_coords[1].y = 224 - 1;

    struct vdp1_cmdt_local_coord local;
    local.lc_coord.x = 0;
    local.lc_coord.y = 0;  
    
    struct vdp1_cmdt_polygon polygon_pointer;
    struct vdp1_cmdt_polyline polyline_pointer, polylineframe_pointer;
    struct vdp1_cmdt_sprite distorted_sprite_pointer;

    /* Main loop */
    for(;;)
    {
        int i;
        uint16_t temp;
        
		/* Wait for next vblank */
        vdp2_tvmd_vblank_out_wait(); 

        /* Trigger plot and bump frame counter */
        MEMORY_WRITE(16, VDP1(FBCR), fbcr);
        frame_count++;
        
        /* Poll joypad */
        if (g_digital.connected == 1) 
        {
            joyA = g_digital.pressed.button.a;
            joyB = g_digital.pressed.button.b;
            joyC = g_digital.pressed.button.c;
            joyStart = g_digital.pressed.button.start;
        }     
        
        if(joyStart & !oldjoyStart) break;
        
        /* Update TVMR */
        temp = 0x0000;
        if(joyA & !oldjoyA) temp |= 0x0004;
        else if(joyB & !oldjoyB) temp |= 0x0002;
        else if(joyC & !oldjoyC) temp |= 0x0001;
        MEMORY_WRITE(16, VDP1(TVMR), temp);
        
        /* Update TVMD */
        temp = 0x8000;
        if(joyL & !oldjoyL) temp |= 0x00C0;
        if(joyR & !oldjoyR) temp |= 0x0002;
        MEMORY_WRITE(16, VDP1(TVMD), temp);

        /* Update FBCR */
        fbcr = 0x0001;
        if(joyY & !oldjoyY) fbcr |= 0x0008;
        if(joyZ & !oldjoyZ) fbcr |= 0x0004;

        /* Update EWRR */
        MEMORY_WRITE(16, VDP1(EWRR), (joyX & !oldjoyX) ? 0x0101 : 0x50DF);        
        
        oldjoyA = joyA;
        oldjoyB = joyB; 
        oldjoyC =  joyC;     
        oldjoyStart = joyStart;
        
        /* Move objects */
        for(i=0;i<4;i++)
            update_pos(&foo[i]);
        
        /* Make new list */
        vdp1_cmdt_list_begin(0);  
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            MESH(0);
            memset(&polylineframe_pointer, 0x00, sizeof(struct vdp1_cmdt_polyline));
            polylineframe_pointer.cl_color = COLOR_RGB_DATA | COLOR_RGB555(31, 0 , 0);
            polylineframe_pointer.cl_mode.raw = 0x00C0 | mesh_flag;
            polylineframe_pointer.cl_vertex.a.x = 319;
            polylineframe_pointer.cl_vertex.a.y = 0;
            polylineframe_pointer.cl_vertex.b.x = 319;
            polylineframe_pointer.cl_vertex.b.y = 223;
            polylineframe_pointer.cl_vertex.c.x = 0;
            polylineframe_pointer.cl_vertex.c.y = 223;
            polylineframe_pointer.cl_vertex.d.x = 0;
            polylineframe_pointer.cl_vertex.d.y = 0;
            
            vdp1_cmdt_polyline_draw(&polylineframe_pointer);  
            
            MESH(1);
            memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));
            
            polygon_pointer.cp_color = COLOR_RGB_DATA | COLOR_RGB555(15, 15 , 15);
            polygon_pointer.cp_mode.mesh = mesh_flag;
            polygon_pointer.cp_mode.end_code = 1;
            polygon_pointer.cp_mode.transparent_pixel = 1;
            //polygon_pointer.cp_mode.color_mode = 5; //RGB
            polygon_pointer.cp_mode.cc_mode = 4; // Gouraud only                    
            //polygon_pointer.cp_mode.raw = 0x00C0 | 0x2C;

            polygon_pointer.cp_grad = GOURAUD(0, 0);
            polygon_pointer.cp_vertex.a.x = foo[0].c[0].x;
            polygon_pointer.cp_vertex.a.y = foo[0].c[0].y;
            polygon_pointer.cp_vertex.b.x = foo[0].c[1].x;
            polygon_pointer.cp_vertex.b.y = foo[0].c[1].y;
            polygon_pointer.cp_vertex.c.x = foo[0].c[2].x;
            polygon_pointer.cp_vertex.c.y = foo[0].c[2].y;
            polygon_pointer.cp_vertex.d.x = foo[0].c[3].x;
            polygon_pointer.cp_vertex.d.y = foo[0].c[3].y;

            vdp1_cmdt_polygon_draw(&polygon_pointer);    

            MESH(0);
            memset(&polyline_pointer, 0x00, sizeof(struct vdp1_cmdt_polyline));
            polyline_pointer.cl_color = COLOR_RGB_DATA | COLOR_RGB555(31, 0 , 31);
            polyline_pointer.cl_mode.raw = 0x00C0 | mesh_flag;
            polyline_pointer.cl_vertex.a.x = foo[1].c[0].x;
            polyline_pointer.cl_vertex.a.y = foo[1].c[0].y;
            polyline_pointer.cl_vertex.b.x = foo[1].c[1].x;
            polyline_pointer.cl_vertex.b.y = foo[1].c[1].y;
            polyline_pointer.cl_vertex.c.x = foo[1].c[2].x;
            polyline_pointer.cl_vertex.c.y = foo[1].c[2].y;
            polyline_pointer.cl_vertex  .d.x = foo[1].c[3].x;
            polyline_pointer.cl_vertex.d.y = foo[1].c[3].y;
            
            vdp1_cmdt_polyline_draw(&polyline_pointer);  

            memset(&distorted_sprite_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));
            distorted_sprite_pointer.cs_type = CMDT_TYPE_DISTORTED_SPRITE;

            distorted_sprite_pointer.cs_mode.cc_mode = 0;
            distorted_sprite_pointer.cs_mode.color_mode = 5; 	// mode 5 RGB
            distorted_sprite_pointer.cs_mode.mesh = mesh_flag;
            distorted_sprite_pointer.cs_mode.end_code = 1;
            //distorted_sprite_pointer.cs_mode.user_clipping = 1;
            //eq to normal_sprite_pointer.cs_mode.raw = 0x00A8 | mesh_flag;

            distorted_sprite_pointer.cs_width = TX_W;
            distorted_sprite_pointer.cs_height = TX_H;
            distorted_sprite_pointer.cs_vertex.a.x = foo[2].c[0].x;
            distorted_sprite_pointer.cs_vertex.a.y = foo[2].c[0].y;
            distorted_sprite_pointer.cs_vertex.b.x = foo[2].c[1].x;
            distorted_sprite_pointer.cs_vertex.b.y = foo[2].c[1].y;
            distorted_sprite_pointer.cs_vertex.c.x = foo[2].c[2].x;
            distorted_sprite_pointer.cs_vertex.c.y = foo[2].c[2].y;
            distorted_sprite_pointer.cs_vertex.d.x = foo[2].c[3].x;
            distorted_sprite_pointer.cs_vertex.d.y = foo[2].c[3].y;
            distorted_sprite_pointer.cs_char = CHAR(0);
            //distorted_sprite_pointer.cs_grad = 0;

            vdp1_cmdt_sprite_draw(&distorted_sprite_pointer);                
            vdp1_cmdt_end();
        }
        vdp1_cmdt_list_end(0);  

        //vdp2_tvmd_vblank_in_wait();
        vdp1_cmdt_list_commit();
        
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
