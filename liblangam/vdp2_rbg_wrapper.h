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


#define RBG0		0x00000001
#define RBG1		0x00000002
#define NON			0 
#define SPR 		0x00000100
#define RBG_TB_A	RBG0
#define RBG_TB_B	RBG1

typedef int32_t fix32_t;    	/* Fixed-point representation, gRadix point is between bit 15 & 16   */

fix32_t SPR_MULF(fix32_t a, fix32_t b);
fix32_t SPR_DIVF(fix32_t a, fix32_t b);
fix32_t fsin(fix32_t degree);
fix32_t fcos(fix32_t degree);

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

fix32_t	Fsin(fix32_t a);
fix32_t	Fcos(fix32_t a);

#define FIXED(x)	      	((fix32_t)((x) * 65536.0))
#define MUL_FIXED(a, b)       SPR_MULF(a, b)
//PROTOTYPE(fix32_t, SPR_MULF, (fix32_t a, fix32_t b));
#define DIV_FIXED(a, b)       SPR_DIVF(a, b)
//PROTOTYPE(fix32_t, SPR_DIVF, (fix32_t a, fix32_t b));


/****************************************************
 *      Rotate Scroll Function Register Cluster     *
 *      Address ranger 1800B0H - 1800BFH            *
 ****************************************************/
typedef struct rot_t{
	uint16_t	paramode;		/* Rotate Parameter Mode                    RPMD    1800B0H */
	uint16_t	paramcontrl;	/* Rotate Parameter Read Control            RPRCTL  1800B2H */
	uint16_t	k_contrl;		/* Keisu Table Control                      KTCTL   1800B4H */       
	uint16_t	k_offset;		/* Keisu Addres Offset                      KTAOF   1800B6H */
	uint16_t	mapover[2];		/* Rotate Scroll Map Over                   OVPNRA  1800B8H + OVPNRB 1800BAH */
	uint32_t	paramaddr;		/* Rotate Parameter Tabel Address           RPTAU   1800BCH + RPTAL  1800BEH*/
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

typedef struct rotreg_t {
	xyz_t		screenst;
	xy_t		screendlt;
	xy_t		delta;
	fix32_t		matrix_a;
	fix32_t		matrix_b;
	fix32_t		matrix_c;
	fix32_t		matrix_d;
	fix32_t		matrix_e;
	fix32_t		matrix_f;
	xyz16_t		viewp;
	uint16_t	dummy1;
	xyz16_t		rotatecenter;
	uint16_t	dummy2;
	xy_t		move;
	xy_t		zoom;
	fix32_t		k_tab;
	xy_t		k_delta;
/*	fix32_t		dummy3[8];	*/
	fix32_t		dummy3[2];
} rotreg_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !VPD2_RBG_WRAPPER_H */