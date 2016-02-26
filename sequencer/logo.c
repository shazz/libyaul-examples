/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include "sequencer.h"
#include "scenes.h"

void logo_init(void)
{
#ifdef DEBUG       
    cons_write(&cons, "[07;2HUIniting Logo"); 
#endif    
}

void logo_update(void)
{
#ifdef DEBUG       
    cons_write(&cons, "[08;2HUpdating Logo");  
#endif    
}

void logo_draw(void)
{
#ifdef DEBUG       
    cons_write(&cons, "[09;2HDrawing Logo"); 
#endif
}

void logo_exit(void)
{
#ifdef DEBUG       
    cons_write(&cons, "[10;2HDesinit Logo");  
#endif
    vdp2_scrn_display_clear();
}