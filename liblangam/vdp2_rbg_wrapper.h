/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * shazz / TRSi
 */

#ifndef VPD2_RBG_WRAPPER_H
#define VPD2_RBG_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <fixmath.h>

#define RBG0		0x00000001
#define RBG1		0x00000002
#define NON			0 
#define SPR 		0x00000100
#define RBG_TB_A	RBG0
#define RBG_TB_B	RBG1

#define VRAM_BANK_NOT_USED 					0
#define RBG0_VRAM_BANK_COEF_TABLE 			1
#define RBG0_VRAM_BANK_PATTERN_NAME 		2
#define RBG0_VRAM_BANK_CHAR_PATTERN			3
#define RBG0_VRAM_BANK_BITMAP_PATTERN		3
#define RBG1_VRAM_BANK_COEF_TABLE 			5
#define RBG1_VRAM_BANK_PATTERN_NAME 		6
#define RBG1_VRAM_BANK_CHAR_PATTERN			7
#define RBG1_VRAM_BANK_BITMAP_PATTERN		7


#if 0

fix32_t SPR_MULF(fix32_t a, fix32_t b);
fix32_t SPR_DIVF(fix32_t a, fix32_t b);
fix32_t fsin(fix32_t degree);
fix32_t fcos(fix32_t degree);

typedef int32_t fix32_t;    	/* Fixed-point representation, gRadix point is between bit 15 & 16   */
#define SIN(a)              Fsin(a)
#define COS(a)              Fcos(a)
#define FIXED(x)	      	((fix32_t)((x) * 65536.0))
#define MUL_FIXED(a, b)       SPR_MULF(a, b)
//PROTOTYPE(fix32_t, SPR_MULF, (fix32_t a, fix32_t b));
#define DIV_FIXED(a, b)       SPR_DIVF(a, b)
//PROTOTYPE(fix32_t, SPR_DIVF, (fix32_t a, fix32_t b));

#else

typedef fix16_t fix32_t;
#define SIN(a)                  fix16_sin(fix16_deg_to_rad(a))
#define COS(a)                  fix16_cos(fix16_deg_to_rad(a))
#define FIXED(x)	      	    fix16_from_int(x)
#define MUL_FIXED(a, b)         fix16_mul(a, b)
#define DIV_FIXED(a, b)         fix16_div(a, b)

#endif

fix32_t	Fsin(fix32_t a);
fix32_t	Fcos(fix32_t a);

uint32_t vdp2_rbg_initRotateTable(uint32_t address, uint16_t mode, uint32_t rA, uint32_t rB);
void vdp2_rbg_move(uint32_t screen, fix32_t x, fix32_t y, fix32_t z);
void vdp2_rbg_moveTo(uint32_t screen, fix32_t x, fix32_t y, fix32_t z);
void vdp2_rbg_rotate(uint32_t screen, fix32_t xy, fix32_t z, fix32_t disp);
void vdp2_rbg_rotateZD(uint32_t screen, fix32_t r);
void vdp2_rbg_rotateX(uint32_t screen, fix32_t xAngle);
void vdp2_rbg_rotateY(uint32_t screen, fix32_t yAngle);
void vdp2_rbg_rotateZ(uint32_t screen, fix32_t zAngle);
void vdp2_rbg_scale(uint32_t screen, fix32_t Sx, fix32_t Sy);
void vdp2_rbg_memcpyw(void *dest, void *src, uint32_t tcnt);
void vdp2_rbg_copyReg();

/*
 * void vdp2_rbg_set_VRAM_banks(int vram_a0, int vram_a1, int vram_b0, int vram_b1, bool useCRAM)
 * Set RAMCTL bits to define where the rotation tables are located in VRAM and CRAM
 * 
 * Should be replaced by complete vdp2_vram_control_set in libyaul
 */
void vdp2_rbg_set_VRAM_banks(int vram_a0, int vram_a1, int vram_b0, int vram_b1, bool useCRAM);

/****************************************************
 *      Rotate Scroll Function Register Cluster     *
 *      Address ranger 1800B0H - 1800BFH            *
 ****************************************************/
typedef struct rot_t{
	uint16_t	paramode;		/* Rotate Parameter Mode                    RPMD    0x1800B0 */
	uint16_t	paramcontrl;	/* Rotate Parameter Read Control            RPRCTL  0x1800B2 */
	uint16_t	k_contrl;		/* Coef Table Control                       KTCTL   0x1800B4 */       
	uint16_t	k_offset;		/* Coef Address Offset                      KTAOF   0x1800B6 */
	uint16_t	mapover[2];		/* Rotate Scroll Map Over                   OVPNRA  0x1800B8 + OVPNRB 0x1800BA */
	uint32_t	paramaddr;		/* Rotate Parameter Table Address           RPTAU   0x1800BC + RPTAL  0x1800BE*/
} rot_t;

/**************************************************
 *      Scroll Parameters Definition              *
 **************************************************/
typedef struct  xy_t {
	 fix32_t         x;
	 fix32_t         y;
} xy_t;

typedef struct  xy16_t {
        uint16_t         x;
        uint16_t         y;
} xy16_t;

typedef struct  xyz16_t {
        int16_t         x;
        int16_t         y;
        int16_t         z;
} xyz16_t;

typedef struct  xyz_t {
	 fix32_t         x;
	 fix32_t         y;
	 fix32_t         z;
} xyz_t;

/**************************************************
 *      Rotation parameter table Definition       *
 **************************************************/
typedef struct rotreg_t {
	xyz_t		screenst;		/* Screen start coord Xst,Yst,Zst (int-frac) 12 */
	xy_t		screendlt;		/* Screen vertical coord delta dXst,dYst (int-frac) 8 */
	xy_t		delta;			/* Screen horizontal coord delta dX,dY (int-frac) 8 */
	fix32_t		matrix_a;		/* Rot matric param A (int-frac) 4 */
	fix32_t		matrix_b;		/* Rot matric param B (int-frac) 4 */
	fix32_t		matrix_c;		/* Rot matric param C (int-frac) 4 */
	fix32_t		matrix_d;		/* Rot matric param D (int-frac) 4 */
	fix32_t		matrix_e;		/* Rot matric param E (int-frac) 4 */
	fix32_t		matrix_f;		/* Rot matric param F (int-frac) 4 */
	xyz16_t		viewp;			/* Viewpoint coord Px, Py, Pz (int) 6 */
	uint16_t	ignored1;		/* ignored 2 */
	xyz16_t		rotatecenter;	/* Center point coord Cx, Cy, Cz (int) 6 */
	uint16_t	ignored2;		/* ignored 2 */
	xy_t		move;			/* Horiz shift Mx, My (int-frac) 8 */
	xy_t		zoom;			/* Scaling coef kx, ky (int-frac) 8 */
	fix32_t		k_tab;			/* Coef table start address KAst (int-frac) 4 */
	xy_t		k_delta;		/* Coef table vertical and horizontal address delta dKAst, dKAx (int-frac) 8 */
	Fixed32		ignored3[2];	/* padding for memory alignment I guess between TA and TB, I would have set 8 and not 2 get get aligned at 0x80 instead of 0x68, see comment in SBL struct. Or due to divide by 4 ? */
} rotreg_t; /* size 0x60 + 0x8 */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !VPD2_RBG_WRAPPER_H */