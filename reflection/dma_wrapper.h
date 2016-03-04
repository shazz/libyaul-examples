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
 * Get SCU DMA status
 */ 
int scu_dma_get_status(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !DMA_WRAPPER_H */