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

#define VRAM_4_BANKS 	0
#define VRAM_2_BANKS 	1
#define VRAM_2_1_BANKS 	2
#define VRAM_1_2_BANKS 	3

struct vdp2_timings

typedef struct {
        uint8_t bank; 
		uint8_t timing; 
        uint8_t type;     
}  vdp2_timings_t;

void init_VRAM_access(int bank_mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !DMA_WRAPPER_H */