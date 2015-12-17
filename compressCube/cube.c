// Flat shaded cube demo for the Saturn
// Mic, 2006
// Ported by Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>

// Convert between integer and fixed point
#define INT2FIX(a) (((int)(a))<<10)
#define FIX2INT(a) (((int)(a))>>10)

typedef struct
{
	int x,y,z;
} point;

typedef struct
{
	int p0,p1,p2,p3;
	int color;
} quad;

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


// Points defining the cube
int points[8][3] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
 				 	{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};

// Faces defining the cube. The 5th element is a color mask.
int faces[6][5] = {{0,1,2,3, 0x0000ff},{4,5,6,7, 0x00ff00},{0,1,5,4, 0xff0000},
    		       {5,1,2,6, 0x00ffff},{3,2,6,7, 0xff00ff},{0,4,7,3, 0xffffff}};

point *	model = (point *)&points;
point rotated[8], projected[8], camera;
quad * cube = (quad *)&faces;
int faceOrder[6];
int avgZ[32];	// Allow max 32 faces to be sorted

// rotate points
void rotate(point *in, point *out, int angle, int n)
{
	int i;
	int temp;
	int can,san;

	// use precomputed sin table
	san = sintb[angle & 0xff];
	can = sintb[(angle+0x40) & 0xff];

	for (i=0; i<n; i++)
	{
		// About X
		out[i].y = ((in[i].y * can) - (in[i].z * san)); out[i].y=FIX2INT(out[i].y);
		out[i].z = ((in[i].y * san) + (in[i].z * can)); out[i].z=FIX2INT(out[i].z);

		// About Y
		out[i].x = ((in[i].x * can) - (out[i].z * san)); out[i].x=FIX2INT(out[i].x);
		out[i].z = ((in[i].x * san) + (out[i].z * can)); out[i].z=FIX2INT(out[i].z);

		// About Z
		temp = out[i].x;
		out[i].x = ((out[i].x * can) - (out[i].y * san)); out[i].x=FIX2INT(out[i].x);
		out[i].y = ((temp * san) + (out[i].y * can)); out[i].y=FIX2INT(out[i].y);
	}
}

// projection
void project(point *in,point *out,int n)
{
	int i;

	for (i=0; i<n; i++)
	{
		out[i].x = camera.x + FIX2INT(((in[i].x * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].y = camera.y + FIX2INT(((in[i].y * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].z = in[i].z;
	}
}

// sort n faces
void sort_quads(quad *f, point *p, int *order, int n)
{
	int i,j,temp;

	// Initialize arrays
	for (i=0; i<n; i++)
	{
		// for each face, compute the sum of each vertex z coord
		avgZ[i] = p[f[i].p0].z +
                  p[f[i].p1].z +
                  p[f[i].p2].z +
                  p[f[i].p3].z;
		order[i] = i;
	}

	// Bubble-sort the whole lot.. yeehaw!
	for (i=0; i<n-1; i++)
	{
		for (j=i+1; j<n; j++)
		{
			if (avgZ[j] < avgZ[i])
			{
				temp = avgZ[i];
				avgZ[i] = avgZ[j];
				avgZ[j] = temp;

				temp = order[i];
				order[i] = order[j];
				order[j] = temp;
			}
		}
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


int main(void)
{
	int theta = 0;
    int i;
    uint16_t *p;

	hardware_init();

	static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(7, 7, 20) };
	vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);

    p = (uint16_t *)GOURAUD(0, 0);
    p[0] = COLOR_RGB_DATA | COLOR_RGB555(31,0,0);
    p[1] = COLOR_RGB_DATA | COLOR_RGB555(0,31,0);
    p[2] = COLOR_RGB_DATA | COLOR_RGB555(0,0,31);
    p[3] = COLOR_RGB_DATA | COLOR_RGB555(31,31,31);
    
	for (i=0; i<8; i++)
	{
		model[i].x = INT2FIX(model[i].x * 35);
		model[i].y = INT2FIX(model[i].y * 35);
		model[i].z = INT2FIX(model[i].z * 35);
	}

	camera.x = 160; camera.y = 112; camera.z = -200;

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

			rotate(model, rotated, theta++, 8);
			project(rotated, projected, 8);
			sort_quads(cube, rotated, faceOrder, 6);

			for (i = 0; i < 6; i++)
			{
				memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));

				int face = faceOrder[5-i];	// take from the farest face to the nearest
				int k = (((-FIX2INT( avgZ[5-i]) / 4) + 35) * 31) /70;		// Calculate shade, 0..31
				int colmsk = cube[face].color;							// Get color mask for this face

				polygon_pointer.cp_color = COLOR_RGB_DATA | COLOR_RGB555(k&colmsk, k&(colmsk>>8), k&(colmsk>>16));
				polygon_pointer.cp_mode.mesh = 0;
				polygon_pointer.cp_mode.end_code = 1;
				polygon_pointer.cp_mode.transparent_pixel = 1;
				polygon_pointer.cp_mode.color_mode = 5; //RGB
				polygon_pointer.cp_mode.cc_mode = 4; // Gouraud only

				polygon_pointer.cp_grad = 0;
				polygon_pointer.cp_vertex.a.x = projected[cube[face].p0].x;
				polygon_pointer.cp_vertex.a.y = projected[cube[face].p0].y;
				polygon_pointer.cp_vertex.b.x = projected[cube[face].p3].x;
				polygon_pointer.cp_vertex.b.y = projected[cube[face].p3].y;
				polygon_pointer.cp_vertex.c.x = projected[cube[face].p2].x;
				polygon_pointer.cp_vertex.c.y = projected[cube[face].p2].y;
				polygon_pointer.cp_vertex.d.x = projected[cube[face].p1].x;
				polygon_pointer.cp_vertex.d.y = projected[cube[face].p1].y;

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