// Saturn flags demo
// Mic, 2006
// Ported to libyaul by Shazz, 2015

#include <yaul.h>

#include <stdlib.h>

#include "flags.h"
#include "adore_pat.h"
#include "sintb.h"
#include "sin1k.h"

struct smpc_peripheral_digital g_digital;

/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

/* Convert between integer and fixed point */
#define FIX2INT(a) (((int)(a))>>10)

typedef struct {
	int x,y,z;
} point3d;

char		country[18][12] = {
	"Bahamas",	"Belgium",	"Chile",	"Denmark",	"Finland",	"France",
	"Germany",	"Greece",	"Iceland",	"India",	"Ireland",	"Italy",
	"Norway",	"Spain",	"Sweden",	"Switzerland",	"Trinidad",	"USA"
};

unsigned char *maps[18] = {
	&bahamas[0],	&belgium[0],	&chile[0],	&denmark[0],	&finland[0],	&france[0],	&germany[0],	&greece[0],
	&iceland[0],	&india[0],	&ireland[0],	&italy[0],	&norway[0],	&spain[0],	&sweden[0],	&switzerland[0],
	&trinidad[0],	&usa[0]
};

int			    flagnum    = 0;
uint16_t		colorLut[256];

point3d 		vtx[264],flag[264];
point3d			camera;

unsigned short 	ang[2];

unsigned char 	base_col[8] = {7,39,71,103,135,167,199,230};
unsigned char 	basecol;

unsigned char	*map, map_ij;

int fontoffset = 0;

/* Project the points to screen space */
void project(point3d *in, point3d *out, int n) {
	int i;

	for (i=0; i<n; i++) {
		out[i].x = camera.x + FIX2INT(((in[i].x * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].y = camera.y + FIX2INT(((in[i].y * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].z = in[i].z;
	}
}

void loadFont()
{
    uint8_t * pFont;
    uint16_t * pPal;
    int i;

    /* Set up a palette for the text */
	pPal = (uint16_t *)CLUT(0, 0); //(uint16_t*)0x25C54000;
	*(pPal++) = 0;
	for (i=0; i<4; i++)
    {
		*(pPal + i) =  COLOR_RGB_DATA | COLOR_RGB555(i*8+7,31,31);
		*(pPal + (7-i)) =  COLOR_RGB_DATA | COLOR_RGB555(i*8+7,i*4+19,31);
	}

    /* Load the font */
	pFont = (uint8_t *)CHAR(0); //(unsigned int*)0x25C50000;
	for (i=0; i<(adore_pat_dat_len-fontoffset); i++) //x300
		*(pFont++) = adore_pat_dat[i+fontoffset]; /* Font data, 96 characters, 4bpp */
}

/* Print a string at a given position on the screen using sprites */
void draw_text(char *txt, int xpos, int ypos)
{
	unsigned int i;
    int x;

	x = 0;
	for (i=0; i < strlen(txt); i++)
    {
		/* Draw normal characters */
        struct vdp1_cmdt_sprite normal_sprite_pointer;
        memset(&normal_sprite_pointer, 0x00, sizeof(struct vdp1_cmdt_sprite));
        normal_sprite_pointer.cs_type = CMDT_TYPE_NORMAL_SPRITE;
        normal_sprite_pointer.cs_mode.color_mode = 1; 	// mode 1 COLMODE_16_LUT
        normal_sprite_pointer.cs_mode.user_clipping = 1;
        normal_sprite_pointer.cs_mode.end_code = 1;

        //#define CLIP_DRAW_INSIDE 0x400
        //#define COLMODE_16_LUT	1
        //normal_sprite_pointer.cs_mode.raw = CLIP_DRAW_INSIDE | 0x0080 | ((COLMODE_16_LUT&7)<<3);

        normal_sprite_pointer.cs_clut = CLUT(0, 0);
        normal_sprite_pointer.cs_width = 8;
        normal_sprite_pointer.cs_height = 8;
        normal_sprite_pointer.cs_position.x = xpos + x;
        normal_sprite_pointer.cs_position.y = ypos;
        normal_sprite_pointer.cs_char =  CHAR(0) + ((txt[i]-' ')<<5);
        normal_sprite_pointer.cs_grad = 0;

        vdp1_cmdt_sprite_draw(&normal_sprite_pointer);

		x += 8;
	}
}



/* Switch to another flag */
void switch_flag(int i)
{
	if ((i>=0)&&(i<18))
    {
		map = maps[i];
		flagnum = i;
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
	int i,j,k;
	int xadd,yadd;
	int sizeX,sizeY;
	int iMul22;
    unsigned int joyR = 0;
    unsigned int oldjoyR = 0;
    unsigned int joyL = 0;
    unsigned int oldjoyL = 0;
	unsigned short xan,yan,zan,xan2;

    hardware_init();

    static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 0, 4) };
    vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

	/* Set up the lookup tables for the flag */
  	for (i=0; i<32; i++) {
   		colorLut[i]    = COLOR_RGB_DATA | (i<<10);
   		colorLut[i+32] = COLOR_RGB_DATA | i;
   		colorLut[i+64] = COLOR_RGB_DATA | (i<<10)|(i<<5)|i;
   		colorLut[i+96] = COLOR_RGB_DATA | ((i/3)<<10)|((i/3)<<5)|(i/3);
   		colorLut[i+128]= COLOR_RGB_DATA | (((i>>1)+(i>>2)+(i>>3))<<5);
   		colorLut[i+160]= COLOR_RGB_DATA | (i<<10)|(i<<5)|i;
   		colorLut[i+192]= COLOR_RGB_DATA | ((i&30)<<4)|i;
   		colorLut[i+224]= COLOR_RGB_DATA | (i<<5)|i;
  	}

    loadFont();

	camera.x = 48; camera.y = 32; camera.z = -256;

	flagnum = 0;
  	map = &bahamas[0];

  	ang[0] = ang[1] = 0;

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
         /* Wait for next vblank */
        vdp2_tvmd_vblank_out_wait();

   		sizeX = 0;
   		xan2 = ang[0];  yan = ang[1];

		/* Place some points along three sine curves. Factors are scaled by 2^10. */
    	for (i=0; i<22; i++) {
       		yadd = FIX2INT(4301 * sin1k[yan&0x3ff]);
       		xan  = xan2;

       		k     = 0;
       		sizeY = 0;
       		for (j=0; j<12; j++) {
          		zan  = xan+(xan>>1)+yan;
          		xadd = FIX2INT(6656 * sin1k[xan & 0x3ff]);

          		vtx[k + i].x = 47104 + sizeX + xadd;
          		vtx[k + i].y = 40960 + sizeY + yadd;
          		vtx[k + i].z = FIX2INT(9728 * sin1k[zan & 0x3ff]);

          		xan   += 45;
          		sizeY += 6707;
          		k     += 22;
       		}
       		yan   += 50;
       		sizeX += 7168;
    	}

		/* Get screen coordinates */
    	project(vtx, flag, 264);

        /* Poll joypad */
        if (g_digital.connected == 1)
        {
            joyL = g_digital.pressed.button.l;
            joyR = g_digital.pressed.button.r;

            if(joyL & !oldjoyL) switch_flag(flagnum-1);
            else if(joyR & !oldjoyR) switch_flag(flagnum+1);

            oldjoyR = joyR;
            oldjoyL = joyL;
        }

        /* Make new list */
        vdp1_cmdt_list_begin(0);
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            /* Draw some text on the screen */
            draw_text(country[flagnum],160-(strlen(country[flagnum])<<2),28);
            draw_text("L: Previous",32,192);
            draw_text("R: Next",236,192);


            /* Now render the flag, quad by quad */
            iMul22 = 0;
            for (i=0; i<11; i++)
            {
                for (j=0; j<21; j++)
                {
                    map_ij = map[iMul22+j];
                    if (map_ij)
                    {
                        memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));

                        /* Get base color and shade */
                        basecol = base_col[map_ij-1];
                        k = ((vtx[iMul22+j].z + 9730)/926)+5;
                        if (k>24) k=24;

                        polygon_pointer.cp_color = colorLut[basecol+k];
                        polygon_pointer.cp_mode.end_code = 1;
                        polygon_pointer.cp_mode.transparent_pixel = 1;
                        polygon_pointer.cp_mode.color_mode = 0; // to check
                        polygon_pointer.cp_mode.cc_mode = 0; // to check

                        polygon_pointer.cp_grad = 0;
                        polygon_pointer.cp_vertex.a.x = flag[iMul22+j].x;
                        polygon_pointer.cp_vertex.a.y = flag[iMul22+j].y;
                        polygon_pointer.cp_vertex.b.x = flag[iMul22+j+1].x;
                        polygon_pointer.cp_vertex.b.y = flag[iMul22+j+1].y;
                        polygon_pointer.cp_vertex.c.x = flag[iMul22+j+23].x;
                        polygon_pointer.cp_vertex.c.y = flag[iMul22+j+23].y;
                        polygon_pointer.cp_vertex.d.x = flag[iMul22+j+22].x;
                        polygon_pointer.cp_vertex.d.y = flag[iMul22+j+22].y;

                        vdp1_cmdt_polygon_draw(&polygon_pointer);
                    }
                }
                iMul22 += 22;
            }

            ang[0] += 1;
            ang[1] += 2;

            vdp1_cmdt_end();
        }
        vdp1_cmdt_list_end(0);

        vdp2_tvmd_vblank_in_wait();
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
