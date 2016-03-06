/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz / TRSi
 */

#include <yaul.h>
#include "dma_wrapper.h"

//0000 0000 0000 0000 0000 0000 0000 0011 - DDWT(1), DDMV(0) = 0   
//0000 0000 0000 0001 0000 0000 0011 0000 - D0WT(5), D0MV(4), D0BK(16) = 0
//0000 0000 0000 1010 0000 0001 0000 0000 - D1WT(19), D1MV(8), D1BK(17) = 0
//0000 0000 0000 0000 0011 0000 0000 0000 - D2WT(13), D2MV(12) = 0    
#define MASK_ALL_CH                 0x00003
#define MASK_CH0                    0x10030
#define MASK_CH1                    0xA0100
#define MASK_CH2                    0x03000

static void scu_dma_illegal(void);
static void scu_dma_level_0_end(void);
static void scu_dma_level_1_end(void);
static void scu_dma_level_2_end(void);

static int g_dma_transfer_status;
static int g_current_channel = DMA_LEVEL_0;
static bool g_dma_int_status[4];
static uint32_t g_dma_int_status_mask[4];

	
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
    scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_1_DMA_END, scu_dma_level_1_end);
    scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_2_DMA_END, scu_dma_level_2_end);
	scu_ic_interrupt_set(IC_INTERRUPT_DMA_ILLEGAL, scu_dma_illegal);
	scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);
	
    uint32_t reg_dsta = MEMORY_READ(32, SCU(DSTA));
    
    g_dma_int_status[0] = reg_dsta;
    
    g_dma_int_status[0] = ((MEMORY_READ(32, SCU(DSTA)) & MASK_CH0) == 0);
    g_dma_int_status[1] = ((MEMORY_READ(32, SCU(DSTA)) & MASK_CH1) == 0);
    g_dma_int_status[2] = ((MEMORY_READ(32, SCU(DSTA)) & MASK_CH2) == 0);
    g_dma_int_status[3] = ((MEMORY_READ(32, SCU(DSTA)) & MASK_ALL_CH) == 0);
    
    g_dma_int_status_mask[0] = MASK_CH0;    
    g_dma_int_status_mask[1] = MASK_CH1;   
    g_dma_int_status_mask[2] = MASK_CH2;   
    g_dma_int_status_mask[3] = MASK_ALL_CH;  
    
	cpu_intc_enable();	
	
    g_dma_transfer_status = SCU_DMA_STATUS_IDLE;
}

/*
 * Start a SCU DMA Transfer, direct mode, asynchronously
 */
void *scu_dma_async_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;
    
    cpu_intc_disable(); 
    
    // check Channel 0 is not busy
    uint32_t status = MEMORY_READ(32, SCU(DSTA)) & MASK_CH0;
    if(status != 0) return NULL;  

	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.update = 0; 
	cfg.add = 3;    		// sattech, need to be 3=001 if update mode bit is set for write address
    
	scu_dma_cpu_level_set(DMA_LEVEL_0, DMA_MODE_DIRECT, &cfg);
    
    g_dma_transfer_status = SCU_DMA_STATUS_WAIT;
	scu_dma_cpu_level_start(DMA_LEVEL_0); 	
    
    cpu_intc_enable();	
	
	return dest;
}

/*
 * Start a SCU DMA Transfer, direct mode, synchronously (wait for inactive DMA in status register)
 */
void *scu_dma_sync_memcpy(void *dest, const void *src, size_t n)
{
	struct dma_level_cfg cfg;	
    
    cpu_intc_disable(); 
	
    // check Channel 0 is not busy
    uint32_t status = MEMORY_READ(32, SCU(DSTA)) & MASK_CH0;
    if(status != 0) return NULL;  
	
	cfg.mode.direct.src = src; 
	cfg.mode.direct.dst = dest;  
	cfg.mode.direct.len = n;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.update = 0; 
	cfg.add = 3;    		// sattech, need to be 3=001 if update mode bit is set for write address

	scu_dma_cpu_level_set(DMA_LEVEL_0, DMA_MODE_DIRECT, &cfg);
    
    g_dma_transfer_status = SCU_DMA_STATUS_WAIT;
	scu_dma_cpu_level_start(DMA_LEVEL_0); 
	
    cpu_intc_enable();		
	
	while(scu_dma_get_status(SCU_DMA_CH0) != 0);
	
	return dest;
}

/*
 * Start a SCU DMA Transfer, indirect mode, asynchronously
 */
void scu_dma_listcpy(uint32_t * table)
{
	struct dma_level_cfg cfg;
    
    cpu_intc_disable(); 
    
    // find available channel
    g_dma_int_status[g_current_channel] = (MEMORY_READ(32, SCU(DSTA)) & g_dma_int_status_mask[g_current_channel])==0;
    while(!g_dma_int_status[g_current_channel])
    {
        g_current_channel= (g_current_channel + 1) % 3;
        g_dma_int_status[g_current_channel] = (MEMORY_READ(32, SCU(DSTA)) & g_dma_int_status_mask[g_current_channel])==0;
    }

    cfg.mode.indirect.nelems = sizeof(table)/sizeof(uint32_t);
    cfg.mode.indirect.tbl = (void *) table;

	cfg.starting_factor = DMA_MODE_START_FACTOR_ENABLE;
	cfg.update = 0; 
	cfg.add = 3;    		// sattech, need to be 3=001 if update mode bit is set for write address

	scu_dma_cpu_level_set(g_current_channel, DMA_MODE_INDIRECT, &cfg);
    
    g_dma_transfer_status = SCU_DMA_STATUS_WAIT;
	scu_dma_cpu_level_start(g_current_channel); 	
    
    cpu_intc_enable();  
}

/*
 * Get SCU DMA status
 */ 
uint8_t scu_dma_get_status(uint8_t channel)
{
    uint8_t status;
    
    cpu_intc_disable(); 
    
    if((MEMORY_READ(32, SCU(DSTA)) & g_dma_int_status_mask[channel]) == 0)	 
		status = SCU_DMA_STATUS_IDLE;
	else 
		status = SCU_DMA_STATUS_WAIT;
        
    cpu_intc_enable();  
    
    return status;
}

/*
 * Handler to DMA ILLEGAL interrupt (Direct mode)
 */ 
static void scu_dma_illegal(void)
{
    g_dma_transfer_status = SCU_DMA_STATUS_ILLEGAL;
}

/*
 * Handler to Level 0 DMA END interrupt 
 */ 
static void scu_dma_level_0_end(void)
{
    g_dma_transfer_status = SCU_DMA_STATUS_END;
}

/*
 * Handler to Level 1 DMA END interrupt 
 */ 
static void scu_dma_level_1_end(void)
{
    g_dma_transfer_status = SCU_DMA_STATUS_END;
}


/*
 * Handler to Level 2 DMA END interrupt 
 */ 
static void scu_dma_level_2_end(void)
{
    g_dma_transfer_status = SCU_DMA_STATUS_END;
}

