/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz / TRSi
 */

#include <yaul.h>
#include "collide.h"


/*returns maximum or minimum of number*/
#define COLLIDE_MAX(a,b)	((a > b) ? a : b)
#define COLLIDE_MIN(a,b)	((a < b) ? a : b)

/*
 *	returns true if pixel at (u,v ) is a transparent pixel
 *  only works inb BGR format for the moment  
 */
bool is_transparent_pixel(struct vdp1_cmdt_sprite * sprite, int u , int v)
{
    // check outbounds
    if(v > sprite->cs_height) return false;
    if(u > sprite->cs_width) return false;
    
    if(sprite->cs_mode.transparent_pixel == 1)
    {
        if(sprite->cs_mode.color_mode = 5)
        {
            /*here p is the address to the pixel we want to retrieve*/
            uint16_t *pixel = (uint16_t *)sprite->cs_char + (((v * sprite->cs_width) + u) * 2);
            /*test whether pixels color == color of transparent pixels for that sprite*/
            return (pixel == 0x0);            
        }
        else
        {
            // TODO
            // Transparent value is always zero, only size changes
            return false;  
        }
    }
    else
        return false;
}

/**
 * tests whether 2 circles intersect
 *
 * circle1 : centre (x1,y1) with radius r1
 * circle2 : centre (x2,y2) with radius r2
 *
 * (allow distance between circles of offset)
 */
int intersect_circles(int x1, int y1, int r1, int x2, int y2, int r2, int offset)
{
	int xdiff = x2 - x1;	// x plane difference
	int ydiff = y2 - y1;	// y plane difference
	
	/* distance between the circles centres squared */
	int dcentre_sq = (ydiff*ydiff) + (xdiff*xdiff);
	
	/* calculate sum of radiuses squared */
	int r_sum_sq = r1 + r2;	// square on seperate line, so
	r_sum_sq *= r_sum_sq;	// dont recompute r1 + r2

	return (dcentre_sq - r_sum_sq <= (offset*offset));
}

/*
	pixel perfect collision test
*/
int collide_pixel(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b, int skip)
{
    int x, y;
    int xstart, xend, ystart, yend;
    
	/*a - bottom right co-ordinates*/
	int ax1 = sprite_a->cs_position.x + sprite_a->cs_width - 1;
	int ay1 = sprite_a->cs_position.y + sprite_a->cs_height - 1;
	
	/*b - bottom right co-ordinates*/
	int bx1 = sprite_b->cs_position.x + sprite_b->cs_width - 1;
	int by1 = sprite_b->cs_position.y + sprite_b->cs_height - 1;

	/*check if bounding boxes intersect*/
	if((bx1 < sprite_a->cs_position.x) || (ax1 < sprite_b->cs_position.x))
		return 0;
	if((by1 < sprite_a->cs_position.y) || (ay1 < sprite_b->cs_position.y))
		return 0;

    /*
    Now lets make the bouding box for which we check for a pixel collision
	To get the bounding box we do
	    Ax1,Ay1_____________
		|		|
		|		|
		|		|
		|    Bx1,By1_____________
		|	|	|	|
		|	|	|	|
		|_______|_______|	|
			|    Ax2,Ay2	|
			|		|
			|		|
			|____________Bx2,By2

	To find that overlap we find the biggest left hand cordinate AND the smallest right hand co-ordinate
	To find it for y we do the biggest top y value AND the smallest bottom y value
	Therefore the overlap here is Bx1,By1 --> Ax2,Ay2

	Remember	Ax2 = Ax1 + SA->w
			Bx2 = Bx1 + SB->w
			Ay2 = Ay1 + SA->h
			By2 = By1 + SB->h

	now we loop round every pixel in area of intersection
    if 2 pixels alpha values on 2 surfaces at the same place != 0 then we have a collision
    */
    
	xstart = COLLIDE_MAX(sprite_a->cs_position.x, sprite_b->cs_position.x);
	xend = COLLIDE_MIN(ax1, bx1);

	ystart = COLLIDE_MAX(sprite_a->cs_position.y, sprite_b->cs_position.y);
	yend = COLLIDE_MIN(ay1, by1);

	for(y = ystart ; y <= yend ;  y += skip)
	{
		for(x = xstart ; x <= xend ; x += skip)
		{
			/*compute offsets for surface before pass to TransparentPixel test*/            
			if( !is_transparent_pixel(sprite_a , x-sprite_a->cs_position.x , y-sprite_a->cs_position.y)
			    && !is_transparent_pixel(sprite_b , x-sprite_b->cs_position.x , y-sprite_b->cs_position.y) )
				return 1;
		}
	}
    
    return 0;
}

/*
	bounding box collision test
*/
int collide_bounding_box(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b)
{
	if(sprite_b->cs_position.x + sprite_b->cs_width < sprite_a->cs_position.x)	return 0;	/*Just checking if their*/
	if(sprite_b->cs_position.x > sprite_a->cs_position.x + sprite_a->cs_width)	return 0;	/*Bounding boxes even touch*/
	/*Write down on paper if*/
	if(sprite_b->cs_position.y + sprite_b->cs_height < sprite_a->cs_position.y)	return 0;	/*Don't make sense*/
	if(sprite_b->cs_position.y > sprite_a->cs_position.y + sprite_a->cs_height)	return 0;

    /*Bouding boxes intersect*/
	return 1;				
}

/*
	a circle intersection detection algorithm that will use
	the position of the centre of the surface as the centre of
	the circle and approximate the radius using the width and height
	of the surface (for example a rect of 4x6 would have r = 2.5).
*/
int collide_bounding_circle(struct vdp1_cmdt_sprite *sprite_a, struct vdp1_cmdt_sprite *sprite_b, int offset)
{
	/* if radius is not specified we approximate them using sprite's
	width and height average and divide by 2*/
    
	int r1 = (sprite_a->cs_width + sprite_a->cs_height) / 4;	// same as / 2) / 2;
	int r2 = (sprite_b->cs_width + sprite_b->cs_height) / 4;

	int x1 = sprite_a->cs_position.x + sprite_a->cs_width / 2;		// offset x and y
	int y1 = sprite_a->cs_position.y + sprite_a->cs_height / 2;		// co-ordinates into
                                        // centre of image
	int x2 = sprite_b->cs_position.x + sprite_b->cs_width / 2;
	int y2 =  sprite_b->cs_position.y + sprite_b->cs_height / 2;

	return intersect_circles(x1, y1, r1, x2, y2, r2, offset);
}


