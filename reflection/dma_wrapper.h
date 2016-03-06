/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * shazz / TRSi
 */

#ifndef DMA_WRAPPER_H
#define DMA_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SCU_DMA_END_CODE (1 << 31)

#define SCU_DMA_STATUS_IDLE             0
#define SCU_DMA_STATUS_WAIT             1
#define SCU_DMA_STATUS_END              2
#define SCU_DMA_STATUS_ILLEGAL          3

#define SCU_DMA_CH0                    0
#define SCU_DMA_CH1                    1
#define SCU_DMA_CH2                    2
#define SCU_DMA_ALL_CH                 3

/*
 * Init SCU DMA, interrupt handlers
 */
void scu_dma_init();

/*
 * Start a SCU DMA Transfer, direct mode, asynchronously
 */
void *scu_dma_async_memcpy(void *dest, const void *src, size_t n);

/*
 * Start a SCU DMA Transfer, direct mode, synchronously (wait for end/illegal interrupt)
 */
void *scu_dma_sync_memcpy(void *dest, const void *src, size_t n);

/*
 * Start a SCU DMA Transfer, indirect mode, asynchronously
 */
void scu_dma_listcpy(uint32_t * table);

/*
 * Get SCU DMA status
 */ 
uint8_t scu_dma_get_status(uint8_t channel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !DMA_WRAPPER_H */