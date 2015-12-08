// Mesh processing test for the Saturn
// Mic, 2008
// Ported by Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>


struct smpc_peripheral_digital g_digital;
/* Frame counter */
volatile uint32_t g_frame_counter = 0;
uint16_t mesh_flag = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

const uint16_t colors[6] = {
				COLOR_RGB_DATA | COLOR_RGB555(31,0,0),
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
    int k, m, i, j;
    uint16_t *p;

    hardware_init();
    static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(0, 0, 0) };

    vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

    p = (uint16_t *)GOURAUD(0, 0);
    p[0] = COLOR_RGB_DATA | COLOR_RGB555(31,0,0);
    p[1] = COLOR_RGB_DATA | COLOR_RGB555(0,31,0);
    p[2] = COLOR_RGB_DATA | COLOR_RGB555(0,0,31);
    p[3] = COLOR_RGB_DATA | COLOR_RGB555(31,31,31);

    p = (uint16_t *)0x25C40000; // ?
    for (i=0; i<64; i++)
    {
        for (j=0; j<64; j++)
        {
            *(p++) = COLOR_RGB_DATA | ((i^j)&31);
        }
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
    struct vdp1_cmdt_sprite normal_sprite_pointer;

  /* Main loop */
    for(;;)
    {
        vdp2_tvmd_vblank_out_wait();

        vdp1_cmdt_list_begin(0);  
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            mesh_flag = 0;
            for (i = 0; i < 8; i++) 
            {
                memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));
                memset(&normal_sprite_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));

                m = 17 + (i&3) * 70;
                k = 43 + (i>>2) * 74;

                mesh_flag = (i>>1)&1;

                if (i&4)
                {
                    normal_sprite_pointer.cs_type = CMDT_TYPE_NORMAL_SPRITE;

                    normal_sprite_pointer.cs_mode.cc_mode = ((((i&1)?0x60000:0)==0)?0:0x2C);
                    normal_sprite_pointer.cs_mode.color_mode = 5; 	// mode 5 RGB
                    normal_sprite_pointer.cs_mode.mesh = mesh_flag;
                    normal_sprite_pointer.cs_mode.user_clipping = 1;
                    normal_sprite_pointer.cs_mode.end_code = 1;
                
                    //eq to normal_sprite_pointer.cs_mode.raw = 0x400 | 0x0080 | mesh_flag << 8 | ((((i&1)?0x60000:0)==0)?0:0x2C) | ((5&7)<<3);

                    normal_sprite_pointer.cs_width = 64;
                    normal_sprite_pointer.cs_height = 64;
                    normal_sprite_pointer.cs_position.x = m;
                    normal_sprite_pointer.cs_position.y = k;

                    normal_sprite_pointer.cs_char = 0x40000;

                    normal_sprite_pointer.cs_grad = ((i&1)?GOURAUD(0, 0):0); //((i&1)?0x60000:0) >> 3;

                    vdp1_cmdt_sprite_draw(&normal_sprite_pointer);
                }
                else
                {
                    polygon_pointer.cp_color = colors[i];
                    polygon_pointer.cp_mode.raw = 0x00C0 | ((i&1)?(0x2C):0);

                    polygon_pointer.cp_grad = ((i&1)?GOURAUD(0, 0):0);
                    polygon_pointer.cp_vertex.a.x = m;
                    polygon_pointer.cp_vertex.a.y = k;
                    polygon_pointer.cp_vertex.b.x = m + 63;
                    polygon_pointer.cp_vertex.b.y = k;
                    polygon_pointer.cp_vertex.c.x = m + 63;
                    polygon_pointer.cp_vertex.c.y = k + 63;
                    polygon_pointer.cp_vertex.d.x = m;
                    polygon_pointer.cp_vertex.d.y = k + 63;

                    vdp1_cmdt_polygon_draw(&polygon_pointer);
                }
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
