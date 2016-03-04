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

#define DMA_STATUS_IDLE             0
#define DMA_STATUS_WAIT             1
#define DMA_STATUS_END              2
#define DMA_STATUS_ILLEGAL          3
static int g_dma_transfer_status = DMA_STATUS_IDLE;
	
/*
 * Init SCU DMA, interrupt handlers
 */	
void scu_dma_init()
{	
	uint16_t mask;
	
	scu_dma_cpu_init();

	mask = IC_MASK_LEVEL_0_DMA_END | IC_MASK_LEVEL_1_DMA_END | IC_MASK_LEVEL_2_DMA_END | IC_MASK_DMA_ILLEGAL;

	cpu_intc_disable();    

	scu_ic_mask_chg(IC_MASK_ALL, mask);
	scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_0_DMA_END, scu_dma_level_0_end);
	scu_ic_interrupt_set(IC_INTERRUPT_DMA_ILLEGAL, scu_dma_illegal);
	scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);
	
	cpu_intc_enable();	
	
    g_dma_transfer_status = DMA_STATUS_IDLE;
}

/*
 * Start a SCU DMA Transfer, direct mode, asynchronously
 */
void *scu_dma_async_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;
    
	uint32_t mask = 0x10030; //0000 0000 0000 0001 0000 0000 0011 0000
	uint32_t regDSTA = MEMORY_READ(32, SCU(DSTA));
	
    if(regDSTA & mask != 0) return NULL;  

	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.update = 0; 
	cfg.add = 0;    		// sattech, need to be 3=001 if update mode bit is set for write address

	scu_dma_cpu_level_set(DMA_LEVEL_0, DMA_MODE_DIRECT, &cfg);
    
    g_dma_transfer_status = DMA_STATUS_WAIT;
	scu_dma_cpu_level_start(DMA_MODE_DIRECT); 	
	
	return dest;
}

/*
 * Start a SCU DMA Transfer, direct mode, synchronously (wait for inactive DMA in status register)
 */
void *scu_dma_sync_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;
	uint32_t mask = 0x10030; //0000 0000 0000 0001 0000 0000 0011 0000
	
	uint32_t regDSTA = MEMORY_READ(32, SCU(DSTA));
	
    if(regDSTA & mask != 0) return NULL;  
	
	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.update = 0; 
	cfg.add = 0;    		// sattech, need to be 3=001 if update mode bit is set for write address

	scu_dma_cpu_level_set(dma_level, DMA_MODE_DIRECT, &cfg);
    g_dma_transfer_status = DMA_STATUS_WAIT;
	scu_dma_cpu_level_start(DMA_MODE_DIRECT); 
		
	
	while(regDSTA & mask != 0)
	{
		regDSTA =  MEMORY_READ(32, SCU(DSTA));
	}
	
	return dest;
}

/*
 * Get SCU DMA status
 */ 
int scu_dma_get_status(void)
{
	uint32_t regDSTA;

	regDSTA = MEMORY_READ(32, SCU(DSTA));
    if(regDSTA & mask != 0)	 
		return DMA_STATUS_IDLE;
	else 
		return DMA_STATUS_WAIT;
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
