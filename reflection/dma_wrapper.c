/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz / TRSi
 */

#include <yaul.h>
#include "dma_wrapper.h"

static void scu_dma_illegal(void);
static void scu_dma_level_0_end(void);
static void scu_dma_level_1_end(void);
static void scu_dma_level_2_end(void);

#define DMA_STATUS_WAIT          0
#define DMA_STATUS_END           1
#define DMA_STATUS_ILLEGAL       2
static int g_dma_transfer_status;
static enum dma_level dma_level;
	
/*
 * Init SCU DMA, interrupt handlers and DMA level
 */	
void dma_init(enum dma_level lvl)
{	
	uint16_t mask;
	
	scu_dma_cpu_init();

	mask = IC_MASK_LEVEL_0_DMA_END | IC_MASK_LEVEL_1_DMA_END | IC_MASK_LEVEL_2_DMA_END | IC_MASK_DMA_ILLEGAL;

	cpu_intc_disable();    

	scu_ic_mask_chg(IC_MASK_ALL, mask);
	scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_0_DMA_END, scu_dma_level_0_end);
	scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_1_DMA_END, scu_dma_level_1_end);
	scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_2_DMA_END, scu_dma_level_2_end);
	scu_ic_interrupt_set(IC_INTERRUPT_DMA_ILLEGAL, scu_dma_illegal);
	scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);
	
	cpu_intc_enable();	
	
	dma_level = lvl;
}

/*
 * Start a SCU DMA Transfer, direct mode, asynchronously
 */
void *dma_async_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;

	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.add = 3;    		// sattech, need to be 001

	scu_dma_cpu_level_set(dma_level, DMA_MODE_DIRECT, &cfg);
	scu_dma_cpu_level_start(DMA_MODE_DIRECT); 	
	
	return dest;
}

/*
 * Start a SCU DMA Transfer, direct mode, synchronously (wait for end/illegal interrupt)
 */
void *dma_sync_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;

	g_dma_transfer_status = DMA_STATUS_WAIT;
	
	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.add = 3;    		// sattech, need to be 001

	scu_dma_cpu_level_set(dma_level, DMA_MODE_DIRECT, &cfg);
	scu_dma_cpu_level_start(DMA_MODE_DIRECT); 
	
	while(g_dma_transfer_status == DMA_STATUS_WAIT)
	{
		// maybe a timeout would be nice to avoid hangling the CPU...
	}
	g_dma_transfer_status = DMA_STATUS_WAIT;
	
	return dest;
}

/*
 * Handler to Level 1 DMA ILLEGAL interrupt
 */ 
static void scu_dma_illegal(void)
{
    g_dma_transfer_status = DMA_STATUS_ILLEGAL;
}

/*
 * Handler to Level 0 DMA END interrupt 
 */ 
static void scu_dma_level_0_end(void)
{
    g_dma_transfer_status = DMA_STATUS_END;
}

/*
 * Handler to Level 1 DMA END interrupt
 */ 
static void scu_dma_level_1_end(void)
{
    g_dma_transfer_status = DMA_STATUS_END;
}

/*
 * Handler to Level 2 DMA END interrupt
 */ 
static void scu_dma_level_2_end(void)
{
    g_dma_transfer_status = DMA_STATUS_END;
}