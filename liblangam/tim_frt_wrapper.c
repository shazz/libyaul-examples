/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz / TRSi
 */

#include <yaul.h>
#include <scu/bus/cpu/cpu/map.h>
#include <scu/bus/cpu/smpc/smpc/map.h>

#include "tim_frt_wrapper.h"

#define TCR     				0x0E16
#define FRC_H    				0x0E12
#define FRC_L    				0x0E13

#define	SMPC_MASK_DOTSEL		0x40
#define	SBL_SMPC_MASK_DOTSEL	0x4000

typedef float float32_t;

/*
 * Init CPU Free Running Timer
 */
void tim_frt_init(uint8_t mode)
{
	// set Time Control Register to 
	uint8_t reg_tcr = (MEMORY_READ(8, CPU(TCR)) & ~TIM_M_CKS) | mode;
	MEMORY_WRITE(8, CPU(TCR), (reg_tcr & ~TIM_M_CKS) | mode);
}

/*
 * Set CPU Free Running Timer initial value
 */
void tim_frt_set(uint16_t value)
{
	MEMORY_WRITE(8, CPU(FRC_H), (uint8_t) (value) >> 8);
	MEMORY_WRITE(8, CPU(FRC_L), (uint8_t) (value & 0xFF));
}

/*
 * Get CPU Free Running Timer current value
 */
uint16_t tim_frt_get(void)
{
	uint8_t reg_frt_h = (MEMORY_READ(8, CPU(FRC_H)));
	uint8_t reg_frt_l = (MEMORY_READ(8, CPU(FRC_H)));
	
	return (reg_frt_h << 8) | reg_frt_l;
}

/*
 * Convert frt value to microseconds
 */
fix16_t tim_frt_ticks_to_us(uint16_t value)
{
	/*
	 * The PLL oscillation frequency is in the 320 mode (NTSC: 26.8741 MHz, PAL: 26.6875 MHz) and the VDP1, VDP2 and SH-2 are run at this frequency.
	 * Changed to run in 352 mode (NTSC: 28.6364 MHz, PAL: 28.4375 MHz) by the CKCHG352 command.
	 */ 
	
	uint16_t tvstat_PAL_reg = MEMORY_READ(16, VDP2(TVSTAT)) & 0x1;		            // keep only PAL bit (0)
	uint8_t oreg10_dotsel = MEMORY_READ(8, OREG(10)) & SMPC_MASK_DOTSEL; 			// check if the Saturn is in 320 or 352 mode, normal smpc init sent the intback command retrieve it...	
	uint8_t reg_tcr_clksel = MEMORY_READ(8, CPU(TCR)) & TIM_M_CKS;					// extract the Clock select bits for the FRT
	
	fix16_t period;
	
	if(tvstat_PAL_reg == 0x1) 
		period = ( (oreg10_dotsel == 0) ? fix16_from_double(0.037470726) : fix16_from_double(0.035164835) ); /*PAL 26,28*/
	else 
		period = ( (oreg10_dotsel == 0) ? fix16_from_double(0.037210548) : fix16_from_double(0.03492059) );  /*NTSC 26,28*/
	
	// clk/8 = 8<<0, clk/32 = 8<<2, clk/128 = 8<<4
	// time in us = value * period * clk ratio
     
	return period * value * (8 << (reg_tcr_clksel << 1));
}

/*
 * #define TIM_FRT_CNT_TO_MCR(count)\
 *   (\
 *   (((*(uint16_t *)0x25f80004 & 0x1) == 0x1) ?   // PAL ‚©?
 *    ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) ://PAL 26,28
 *    ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) //NT 26,28
 *    * (count) * (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))
 */
	 