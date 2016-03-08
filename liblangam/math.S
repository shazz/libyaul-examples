
/*
 * spr_sinc.src
 *
 *
 * _fsin
 * _fcos 
 * 
 */
	.global	_fsin
	.type _fsin, @function
	.global	_fcos
	.type _fcos, @function

/* 
 * Cosine(a) Subroutine
 * Cosine(a) = Sine(90-a)
 * In :a		(R4)
 * Out:Cosine(a)   (R0)
 *
 *
 *
 */

	.SECTION	SEGA_P
	.ALIGN=4
	
_fcos:
	MOV.L	NINTY,R2	/*90-a ->R4 */
	NEG	R4,R4
	ADD	R2,R4

/*
 * Sine (a) Subroutine 
 *
 * In:a = angle (0 - 89)(R4)
 * Out:Sine(a)          (R0)
 *
 */

_fsin:
	MOVA	SINTBL,R0					/*sine table start address ->R1	*/
	STS	MACL,R3

	SWAP.W	R4,R1
	EXTU.W	R1,R1
	SHLL2	R1								/*R0<-Entry of The Table*/
	MOV.W	@(R0,R1),R2				/*sin(a) GRAD->R2*/

	ADD	#2,R0
	MULU	R4,R2
	MOV.W	@(R0,R1),R0				/*sin(a) value->R0*/
	STS	MACL,R2
	EXTU.W	R0,R0
	LDS	R3,MACL
	SWAP.W	R2,R2
	EXTU.W	R2,R2
	RTS
	ADD	R2,R0

/*	Constant Data definiton */

	.ALIGN	4
NINTY:
.long	0x005A0000	/*90*/

/*Sine Table Data Area(AREA SIZE = 91 x 2 x 2 = 364BYTES */

SINTBL:
/*	         delta, value */
.word	0x0477,0x0000		/*00deg */
.word	0x0478,0x0477		/*01deg */
.word	0x0476,0x08EF		/*02deg */
.word	0x0476,0x0D65		/*03deg */
.word	0x0474,0x11DB		/*04deg */
.word	0x0473,0x164F		/*05deg */
.word	0x0470,0x1AC2		/*06deg */
.word	0x046E,0x1F32		/*07deg */
.word	0x046C,0x23A0		/*08deg */
.word	0x0468,0x280C		/*09deg */
.word	0x0464,0x2C74		/*10deg */
.word	0x0461,0x30D8		/*11deg */
.word	0x045D,0x3539		/*12deg */
.word	0x0458,0x3996		/*13deg */
.word	0x0453,0x3DEE		/*14deg */
.word	0x044F,0x4241		/*15deg */
.word	0x0448,0x4690		/*16deg */
.word	0x0443,0x4AD8		/*17deg */
.word	0x043D,0x4F1B		/*18deg */
.word	0x0436,0x5358		/*19deg */
.word	0x0430,0x578E		/*20deg */
.word	0x0428,0x5BBE		/*21deg */
.word	0x0420,0x5FE6		/*22deg */
.word	0x0419,0x6406		/*23deg */
.word	0x0411,0x681F		/*24deg */
.word	0x0409,0x6C30		/*25deg */
.word	0x03FF,0x7039		/*26deg */
.word	0x03F7,0x7438		/*27deg */
.word	0x03ED,0x782F		/*28deg */
.word	0x03E3,0x7C1C		/*29deg */
.word	0x03DA,0x7FFF		/*30deg */
.word	0x03CF,0x83D9		/*31deg */
.word	0x03C5,0x87A8		/*32deg */
.word	0x03BA,0x8B6D		/*33deg */
.word	0x03AE,0x8F27		/*34deg */
.word	0x03A4,0x92D5		/*35deg */
.word	0x0397,0x9679		/*36deg */
.word	0x038B,0x9A10		/*37deg */
.word	0x0380,0x9D9B		/*38deg */
.word	0x0372,0xA11B		/*39deg */
.word	0x0366,0xA48D		/*40deg */
.word	0x0359,0xA7F3		/*41deg */
.word	0x034B,0xAB4C		/*42deg */
.word	0x033E,0xAE97		/*43deg */
.word	0x032F,0xB1D5		/*44deg */
.word	0x0322,0xB504		/*45deg */
.word	0x0313,0xB826		/*46deg */
.word	0x0305,0xBB39		/*47deg */
.word	0x02F6,0xBE3E		/*48deg */
.word	0x02E7,0xC134		/*49deg */
.word	0x02D8,0xC41B		/*50deg */
.word	0x02C8,0xC6F3		/*51deg */
.word	0x02B8,0xC9BB		/*52deg */
.word	0x02A8,0xCC73		/*53deg */
.word	0x0298,0xCF1B		/*54deg */
.word	0x0288,0xD1B3		/*55deg */
.word	0x0278,0xD43B		/*56deg */
.word	0x0266,0xD6B3		/*57deg */
.word	0x0256,0xD919		/*58deg */
.word	0x0244,0xDB6F		/*59deg */
.word	0x0234,0xDDB3		/*60deg */
.word	0x0221,0xDFE7		/*61deg */
.word	0x0211,0xE208		/*62deg */
.word	0x01FE,0xE419		/*63deg */
.word	0x01EC,0xE617		/*64deg */
.word	0x01DB,0xE803		/*65deg */
.word	0x01C8,0xE9DE		/*66deg */
.word	0x01B5,0xEBA6		/*67deg */
.word	0x01A4,0xED5B		/*68deg */
.word	0x0190,0xEEFF		/*69deg */
.word	0x017E,0xF08F		/*70deg */
.word	0x016B,0xF20D		/*71deg */
.word	0x0158,0xF378		/*72deg */
.word	0x0145,0xF4D0		/*73deg */
.word	0x0131,0xF615		/*74deg */
.word	0x011F,0xF746		/*75deg */
.word	0x010B,0xF865		/*76deg */
.word	0x00F7,0xF970		/*77deg */
.word	0x00E4,0xFA67		/*78deg */
.word	0x00D1,0xFB4B		/*79deg */
.word	0x00BD,0xFC1C		/*80deg */
.word	0x00A9,0xFCD9		/*81deg */
.word	0x0095,0xFD82		/*82deg */
.word	0x0081,0xFE17		/*83deg */
.word	0x006E,0xFE98		/*84deg */
.word	0x005A,0xFF06		/*85deg */
.word	0x0046,0xFF60		/*86deg */
.word	0x0032,0xFFA6		/*87deg */
.word	0x001E,0xFFD8		/*88deg */
.word	0x000A,0xFFF6		/*89deg */
.word	0x0000,0xFFFF		/*90deg */

.ALIGN      4
	
fcos:     
.long     _fcos

fsin:     
.long     _fsin

.END

/*   end of file */

