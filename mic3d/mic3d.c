// mic 3d demo for the Saturn
// Mic, 2006
// Ported by Shazz on libyaul, 2015

#include <yaul.h>
#include <stdlib.h>

#define INT2FIX(a) (((int)(a))<<10)
#define FIX2INT(a) (((int)(a))>>10)
struct smpc_peripheral_digital g_digital;

/* Frame counter */
volatile uint32_t g_frame_counter = 0;

static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

int 	    theta = 0;

int sintb[256] = {0,25,50,75,100,125,150,175,199,224,248,273,297,321,344,368,391,414,437,460,482,504,526,547,568,589,609,629,649,668,687,706,
724,741,758,775,791,807,822,837,851,865,878,890,903,914,925,936,946,955,964,972,979,986,993,999,1004,1008,1012,1016,1019,1021,1022,1023,
1024,1023,1022,1021,1019,1016,1012,1008,1004,999,993,986,979,972,964,955,946,936,925,914,903,890,878,865,851,837,822,807,791,775,758,741,
724,706,687,668,649,629,609,589,568,547,526,504,482,460,437,414,391,368,344,321,297,273,248,224,199,175,150,125,100,75,50,25,
-1,-26,-51,-76,-101,-126,-151,-176,-200,-225,-249,-274,-298,-322,-345,-369,-392,-415,-438,-461,-483,-505,-527,-548,-569,-590,-610,-630,-650,-669,-688,-707,
-725,-742,-759,-776,-792,-808,-823,-838,-852,-866,-879,-891,-904,-915,-926,-937,-947,-956,-965,-973,-980,-987,-994,-1000,-1005,-1009,-1013,-1017,-1020,-1022,-1023,-1024,
-1024,-1024,-1023,-1022,-1020,-1017,-1013,-1009,-1005,-1000,-994,-987,-980,-973,-965,-956,-947,-937,-926,-915,-904,-891,-879,-866,-852,-838,-823,-808,-792,-776,-759,-742,
-725,-707,-688,-669,-650,-630,-610,-590,-569,-548,-527,-505,-483,-461,-438,-415,-392,-369,-345,-322,-298,-274,-249,-225,-200,-176,-151,-126,-101,-76,-51,-26};

int pointM[28][3] = {
	{-5,-3,-2},
	{-3,-3,-2},
	{3,-3,-2},
	{5,-1,-2},
	{5,3,-2},
	{3,3,-2},
	{3,-1,-2},
	{1,-1,-2},
	{1,3,-2},
	{-1,3,-2},
	{-1,-1,-2},
	{-3,-1,-2},
	{-3,3,-2},
	{-5,3,-2},
//
	{-5,-3,2},
	{-3,-3,2},
	{3,-3,2},
	{5,-1,2},
	{5,3,2},
	{3,3,2},
	{3,-1,2},
	{1,-1,2},
	{1,3,2},
	{-1,3,2},
	{-1,-1,2},
	{-3,-1,2},
	{-3,3,2},
	{-5,3,2}
};

int pointI[10][3] = {
	{-1,-3,-2},
	{1,-1,-2},
	{1,3,-2},
	{-1,3,-2},
	{-1,-1,-2},
//
	{-1,-3,2},
	{1,-1,2},
	{1,3,2},
	{-1,3,2},
	{-1,-1,2}
};

int pointC[22][3] = {
	{-3,-3,-2},
	{1,-3,-2},
	{3,-1,-2},
	{1,-1,-2},
	{-1,-1,-2},
	{-1,1,-2},
	{3,1,-2},
	{3,3,-2},
	{-1,3,-2},
	{-3,3,-2},
	{-3,-1,-2},
//
	{-3,-3,2},
	{1,-3,2},
	{3,-1,2},
	{1,-1,2},
	{-1,-1,2},
	{-1,1,2},
	{3,1,2},
	{3,3,2},
	{-1,3,2},
	{-3,3,2},
	{-3,-1,2}
};



int faceM[23][4] = {
	{0,1,12,13},
	{1,2,6,11},
	{6,3,4,5},
	{10,7,8,9},
	{2,3,6,6},
	{14,15,26,27},
	{15,16,20,25},
	{20,17,18,19},
	{24,21,22,23},
	{16,17,20,20},
	{0,14,16,2},
	{14,0,13,27},
	{13,27,26,12},
	{11,25,26,12},
	{25,24,10,11},
	{24,10,9,23},
	{9,23,22,8},
	{7,21,22,8},
	{21,20,6,7},
	{20,6,5,19},
	{5,19,18,4},
	{3,17,18,4},
	{2,16,17,3}
};


int faceI[8][4] = {
	{0,1,4,4},
	{1,2,3,4},
	{0,5,6,1},
	{1,6,7,2},
	{3,8,7,2},
	{5,0,3,8},
	{5,6,9,9},
	{6,7,8,9}
};

int faceC[16][4] = {
	{0,1,3,10},
	{1,2,3,3},
	{10,4,8,9},
	{5,6,7,8},
	{11,12,14,21},
	{12,13,14,14},
	{21,15,19,20},
	{16,17,18,19},
	{0,11,12,1},
	{1,12,13,2},
	{4,15,13,2},
	{4,15,16,5},
	{5,16,17,6},
	{6,17,18,7},
	{9,20,18,7},
	{11,0,9,20}
};


typedef struct
{
	int x,y,z;
} point3d;

typedef struct
{
	int p0,p1,p2,p3;
} quadrangle;


point3d camera;
point3d *modelM=(point3d*)&pointM,rotatedM[28],projectedM[28];
quadrangle *letterM=(quadrangle*)&faceM;
int faceOrderM[23];

point3d *modelI=(point3d*)&pointI,rotatedI[10],projectedI[22];
quadrangle *letterI=(quadrangle*)&faceI;
int faceOrderI[8];

point3d *modelC=(point3d*)&pointC,rotatedC[22],projectedC[22];
quadrangle *letterC=(quadrangle*)&faceC;
int faceOrderC[16];

quadrangle faces[47];
point3d allPoints[60];
int faceOrder[47];

void rotate(point3d *in,point3d *out,int angle,int n)
{
	int i;
	int temp;
	int can,san;

	san = sintb[angle&0xff];
	can = sintb[(angle+0x40)&0xff];

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


void transform(point3d *in,point3d *out,int xt,int yt,int zt,int n)
{
	int i;

	xt = INT2FIX(xt); yt = INT2FIX(yt); zt = INT2FIX(zt);
	for (i=0; i<n; i++)
	{
		out[i].x = in[i].x + xt;
		out[i].y = in[i].y + yt;
		out[i].z = in[i].z + zt;
	}
}


void project(point3d *in,point3d *out,int n)
{
	int i;

	for (i=0; i<n; i++)
	{
		out[i].x = camera.x + FIX2INT(((in[i].x * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].y = camera.y + FIX2INT(((in[i].y * camera.z) / (camera.z - FIX2INT(in[i].z))));
		out[i].z = in[i].z;
	}
}


int avgZ[64];	// Allow max 64 faces to be sorted
void sort_quads(quadrangle *f,point3d *p,int *order,int n)
{
	int i,j,temp;

	// Initialize arrays
	for (i=0; i<n; i++)
	{
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
			if (avgZ[j] > avgZ[i])
			{
				temp = avgZ[i]; avgZ[i] = avgZ[j]; avgZ[j] = temp;
				temp = order[i]; order[i] = order[j]; order[j] = temp;
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

int main()
{
	int i,j,k;
	uint16_t * pGour;

    hardware_init();
    
    // needed ?
    MEMORY_WRITE(16, VDP1(PTMR), 0x0002);
    MEMORY_WRITE(16, VDP1(EWDR), COLOR_RGB_DATA | COLOR_RGB555(7,7,20));
    
    static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(7, 7, 20) };
    vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);    

	for (i=0; i<28; i++)
	{
		modelM[i].x = INT2FIX(modelM[i].x * 6);
		modelM[i].y = INT2FIX(modelM[i].y * 6);
		modelM[i].z = INT2FIX(modelM[i].z * 6);
	}
	for (i=0; i<10; i++)
	{
		modelI[i].x = INT2FIX(modelI[i].x * 6);
		modelI[i].y = INT2FIX(modelI[i].y * 6);
		modelI[i].z = INT2FIX(modelI[i].z * 6);
	}
	for (i=0; i<22; i++)
	{
		modelC[i].x = INT2FIX(modelC[i].x * 6);
		modelC[i].y = INT2FIX(modelC[i].y * 6);
		modelC[i].z = INT2FIX(modelC[i].z * 6);
	}

	for (i=0; i<23; i++)
		faces[i] = letterM[i];
	for (i=0; i<8; i++)
	{
		faces[i+23].p0 = letterI[i].p0 + 28;
		faces[i+23].p1 = letterI[i].p1 + 28;
		faces[i+23].p2 = letterI[i].p2 + 28;
		faces[i+23].p3 = letterI[i].p3 + 28;
	}
	for (i=0; i<16; i++)
	{
		faces[i+31].p0 = letterC[i].p0 + 38;
		faces[i+31].p1 = letterC[i].p1 + 38;
		faces[i+31].p2 = letterC[i].p2 + 38;
		faces[i+31].p3 = letterC[i].p3 + 38;
	}

	camera.x = 160; camera.y = 112; camera.z = -200;

   	/* Make Gouraud shading table */
    pGour = (uint16_t *) 0x25C60000; //GOURAUD(0, 0);
    pGour[0] = COLOR_RGB_DATA | COLOR_RGB555(31,0,0);
    pGour[1] = COLOR_RGB_DATA | COLOR_RGB555(0,31,0);
    pGour[2] = COLOR_RGB_DATA | COLOR_RGB555(0,0,31);
    pGour[3] = COLOR_RGB_DATA | COLOR_RGB555(31,31,31);

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
        int i;

        rotate(modelM, rotatedM, theta, 28);
		rotate(modelI, rotatedI, theta, 10);
		rotate(modelC, rotatedC, theta++, 22);

		transform(rotatedM,rotatedM,-50,0,0,28);
		transform(rotatedC,rotatedC,35,0,0,22);

		project(rotatedM, projectedM, 28);
		project(rotatedI, projectedI, 10);
		project(rotatedC, projectedC, 22);

		for (i=0; i<28; i++)
			allPoints[i] = projectedM[i];
		for (i=0; i<10; i++)
			allPoints[i+28] = projectedI[i];
		for (i=0; i<22; i++)
			allPoints[i+38] = projectedC[i];

		sort_quads(faces, allPoints, faceOrder, 47);

		/* Wait for next vblank */
        vdp2_tvmd_vblank_out_wait(); 

        /* Make new list */
        vdp1_cmdt_list_begin(0);  
        {
            vdp1_cmdt_system_clip_coord_set(&system_clip);
            vdp1_cmdt_user_clip_coord_set(&user_clip);
            vdp1_cmdt_local_coord_set(&local);

            for (i=0; i<47; i++)
            {
                memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));
                
                j = faceOrder[i];
                // Check which direction this quad is facing
                k = ((-FIX2INT(avgZ[i])/4)+30);
                polygon_pointer.cp_color = COLOR_RGB_DATA | COLOR_RGB555(27*k/90+9,28*k/90+9,30*k/90+10);
                
                polygon_pointer.cp_mode.mesh = 1;
                polygon_pointer.cp_mode.end_code = 1;
                polygon_pointer.cp_mode.transparent_pixel = 1;
                polygon_pointer.cp_mode.color_mode = 5; //RGB
                polygon_pointer.cp_mode.cc_mode = 4; // Gouraud only                    
                //polygon_pointer.cp_mode.raw = 0x00C0 | 0x2C;

                polygon_pointer.cp_grad = 0;
                polygon_pointer.cp_vertex.a.x = allPoints[faces[j].p0].x;
                polygon_pointer.cp_vertex.a.y = allPoints[faces[j].p0].y;
                polygon_pointer.cp_vertex.b.x = allPoints[faces[j].p3].x;
                polygon_pointer.cp_vertex.b.y = allPoints[faces[j].p3].y;
                polygon_pointer.cp_vertex.c.x = allPoints[faces[j].p2].x;
                polygon_pointer.cp_vertex.c.y = allPoints[faces[j].p2].y;
                polygon_pointer.cp_vertex.d.x = allPoints[faces[j].p1].x;
                polygon_pointer.cp_vertex.d.y = allPoints[faces[j].p1].y;

                vdp1_cmdt_polygon_draw(&polygon_pointer);                    
            }
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
