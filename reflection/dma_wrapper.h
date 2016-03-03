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
 * Init SCU DMA, interrupt handlers and DMA level
 */
void dma_init(enum dma_level lvl);

/*
 * Start a SCU DMA Transfer, direct mode, asynchronously
 */
void *dma_async_memcpy(void *dest, const void *src, size_t n);

/*
 * Start a SCU DMA Transfer, direct mode, synchronously (wait for end/illegal interrupt)
 */
void *dma_sync_memcpy(void *dest, const void *src, size_t n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !DMA_WRAPPER_H */