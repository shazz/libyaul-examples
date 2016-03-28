/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * shazz / TRSi
 */

#ifndef COLLIDE_H
#define COLLIDE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * 
 * Adapted from SDL_Collide.h by Amir 'Genjix' Taaki released under GNU GPL by genjix@gmail.com
 * 
 */

/**
 * @brief Pixel-perfect collision between two surfaces.
 *
 * @param SA the first sprite to check
 * @param SB the second sprite to check
 * @param skip how many pixels the looping test should skip.
 *             1 is a truly perfect pixel test, but a value
 *             of 4 should be fine.
 *             this parameter is here to speed up the test at
 *             the expense of accuracy.
 *
 * @return non-zero on collision, 0 otherwise
 */
int collide_pixel(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b, int skip);

/**
 * @brief Bounding-box collision between two surfaces.
 *
 * @param sprite_a the first sprite to check
 * @param sprite_b the second sprite to check
 *
 * @return non-zero on collision, 0 otherwise
 */
int collide_bounding_box(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b);

/**
 * @brief a circle intersection detection algorithm that will use the 
 * position of the centre of the sprite as the centre of
 * the circle and approximate the radius using the width and height
 * of the sprite (for example a rect of 4x6 would have r = 2.5).
 * @param sprite_a the first sprite to check
 * @param sprite_b the second sprite to check
 * @param offset
 * @return non-zero on collision, 0 otherwise
 */
int collide_bounding_circle(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b, int offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COLLIDE_H */