
/*
 * spr_fmat.src -- SPR FMAT       
 *
 * _SPR_MULF 
 * _SPR_DIVF 
 * _SPR_ITOF
 * _SPR_FTOI 
 * __divls
 *
 * #include "sega_spr.h"
 *
 */
 
/*
 * SH2 fixed point multiplication routine  '94.01.24
 *
 * C Langage Interface:
 * Fixed32 SPR_MULF( Fixed32 a, Fixed32 b )
 *
 * Fixed point multiplication:
 * H16.L16 x H16.L16 -> e16 H16.L16 o16
 *                          ~~~~~~~
 *    R4    x   R5    ->       R0
 *
 * REGs USED: R0, R3..R5
 *
 */

/*
 * SH2 fixed point division routine   '94.01.24
 *
 * C Langage Interface:
 * Fixed32 SPR_DIVF( Fixed32 a, Fixed32 b )
 *
 * Fixed point div:
 * e16 H16.L16 o16/ H16.L16 -> H16.L16
 *     ~~~~~~~
 *        R4      /   R5    ->   R0
 *
 * REGs USED: R0, R4, R5
 *
 */
  

.global _SPR_MULF
.type _SPR_MULF, @function
.global _SPR_DIVF
.type _SPR_DIVF, @function
.global	_SPR_ITOF
.type _SPR_ITOF, @function
.global	_SPR_FTOI
.type _SPR_FTOI, @function
.global	__divls
.type __divls, @function

.SECTION SEGA_P
.ALIGN=4

_SPR_MULF:
 	STS	MACH,R1
 	STS	MACL,R2
	DMULS.L R4,R5
 	STS	MACH,R3
 	STS	MACL,R0
 	LDS	R1,MACH
 	LDS	R2,MACL
	RTS				/* return */
 	XTRCT	R3,R0			/* xtract for Fixed32 */
_SPR_DIVF:
	MOV.L	A_DIV,R0
	SWAP.W	R4,R2			/*  make R2:R4 64bit pair */
	MOV.L	R5,@R0
	EXTS.W	R2,R2			/*   0  R4H   R4L  0 */
	MOV.L	R2,@(0x10,R0)
	SHLL16	R4
	MOV.L	R4,@(0x14,R0)
	RTS
 	MOV.L	@(0x14,R0),R0
_SPR_ITOF:
	MOV  	R4,R0
	RTS
	SHLL16	R0
_SPR_FTOI:
	MOV  	R4,R0
	SHLR16	R0
	RTS
	EXTS.W	R0,R0
	.ALIGN	4
__divls:
	MOV.L	R3,@-R15
	MOV	#0xF0,R3
	STC.L	SR,@-R15
	EXTU.B	R3,R3
	LDC	R3,SR
	MOV.L	A_DIV,R3
	MOV.L	R0,@R3			/*  SH2 DIV Unit */
	NOP
	MOV.L	R1,@(0x4,R3)
	NOP
 	MOV.L	@(0x14,R3),R0		/*  get DIV result */
	NOP
	LDC.L	@R15+,SR
	MOV.L	@R15+,R3
	RTS
	MOV	R0,R1
	.ALIGN	4
A_DIV:
.long	0xFFFFFF00


.END

/* end of file  */

