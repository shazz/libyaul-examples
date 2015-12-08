// Gouraud shaded cube demo for the Saturn
// Mic, 2008
// Ported by Shazz on libyaul, 2015

#include <yaul.h>

#include <stdlib.h>

struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

const uint16_t colors[6] = {    COLOR_RGB_DATA | COLOR_RGB555(31,0,0), 
                                COLOR_RGB_DATA | COLOR_RGB555(0,31,0),
                                COLOR_RGB_DATA | COLOR_RGB555(0,0,31),
                                COLOR_RGB_DATA | COLOR_RGB555(0,31,31),
                                COLOR_RGB_DATA | COLOR_RGB555(31,31,0),
                                COLOR_RGB_DATA | COLOR_RGB555(31,31,31)};                       

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

int main(void) 
{
	int k,m;
    uint16_t *p;

    hardware_init();

    static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(0, 0, 0) };
    vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);
    
    p = (uint16_t *)GOURAUD(0, 0);
    p[0] = COLOR_RGB_DATA | COLOR_RGB555(31,0,0);
    p[1] = COLOR_RGB_DATA | COLOR_RGB555(0,31,0);
    p[2] = COLOR_RGB_DATA | COLOR_RGB555(0,0,31);
    p[3] = COLOR_RGB_DATA | COLOR_RGB555(31,31,31);

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

    /* Main loop */
    for(;;)
    {
        vdp2_tvmd_vblank_out_wait();
      
        vdp1_cmdt_list_begin(0); 
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            int i;           
            for (i = 0; i < 6; i++)
            {
                memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));
                
                m = 20 + (i%3) * 100;
                k = 22 + (i/3) * 100;
                
                polygon_pointer.cp_color = colors[i];
                //p[CMDPMOD=0x02] = 0x00C0 | 0x2C to get RGB mode, gouraud shading enabled
                polygon_pointer.cp_mode.raw = 0x00C0 | 0x2C;
                
                polygon_pointer.cp_grad = GOURAUD(0, 0);
                polygon_pointer.cp_vertex.a.x = m;
                polygon_pointer.cp_vertex.a.y = k;
                polygon_pointer.cp_vertex.b.x = m + 79;
                polygon_pointer.cp_vertex.b.y = k;
                polygon_pointer.cp_vertex.c.x = m + 79;
                polygon_pointer.cp_vertex.c.y = k + 79;
                polygon_pointer.cp_vertex.d.x = m;
                polygon_pointer.cp_vertex.d.y = k + 79;
                
                vdp1_cmdt_polygon_draw(&polygon_pointer);
            }
            vdp1_cmdt_end();
        } 
        vdp1_cmdt_list_end(0);
        vdp2_tvmd_vblank_out_wait();
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