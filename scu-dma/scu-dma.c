/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaul.h>

static struct 
{
        bool st_begin;
            #define ST_STATUS_WAIT          0
            #define ST_STATUS_END           1
            #define ST_STATUS_ILLEGAL       2
        int st_status;
        struct 
        {
                int level_sf;
                int level_mode;
        } 
        st_level[3];
} 
state = {
        .st_begin = false,
        .st_status = ST_STATUS_WAIT,
        .st_level = {
                {       // level 0 default values
                        .level_sf = DMA_MODE_START_FACTOR_ENABLE,
                        .level_mode = DMA_MODE_DIRECT
                }, 
                {       // level 1 default values
                        .level_sf = DMA_MODE_START_FACTOR_ENABLE,
                        .level_mode = DMA_MODE_DIRECT
                }, 
                {       // level 2 default values
                        .level_sf = DMA_MODE_START_FACTOR_ENABLE,
                        .level_mode = DMA_MODE_DIRECT
                }
        }
};

static void scu_dma_illegal(void);
static void scu_dma_level(int);
static void scu_dma_level_0_end(void);
static void scu_dma_level_1_end(void);
static void scu_dma_level_2_end(void);

struct cons cons;
static char * consbuf;

struct smpc_peripheral_digital g_digital;
volatile uint32_t g_frame_counter = 0;
volatile uint32_t g_dma_counter = 0;
static uint32_t tick = 0;
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);
static void hblank_out_handler(irq_mux_handle_t *irq_mux __unused);

static bool g_counting = false;

int main(void)
{
        uint16_t mask;
        int level = DMA_LEVEL_0;
        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;
        irq_mux_t *hblank_out;
        
        unsigned int joyR = 0, oldjoyR = 0, joyL = 0,  oldjoyL = 0, joyA = 0, oldjoyA = 0, joyB = 0, oldjoyB = 0, joyC = 0, oldjoyC = 0;
        unsigned int joyY = 0, oldjoyY = 0, joyZ = 0, oldjoyZ = 0, joyStart = 0, oldjoyStart = 0, joyUp = 0, oldjoyUp = 0, joyDown = 0, oldjoyDown = 0;    

        static uint16_t back_screen_color[] = { COLOR_RGB_DATA | COLOR_RGB555(0, 7, 7) };

        vdp2_init();
        vdp1_init();

        smpc_init();
        smpc_peripheral_init();
        
        scu_dma_cpu_init();

        cons_init(&cons, CONS_DRIVER_VDP2);  
        consbuf = (char *)malloc(1024); 

        mask = IC_MASK_LEVEL_0_DMA_END | IC_MASK_LEVEL_1_DMA_END | IC_MASK_LEVEL_2_DMA_END | IC_MASK_DMA_ILLEGAL;

        cpu_intc_disable();
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
        hblank_out = vdp2_tvmd_hblank_in_irq_get();
        irq_mux_handle_add(hblank_out, hblank_out_handler, NULL);        
 
        scu_ic_mask_chg(IC_MASK_ALL, mask);
        scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_0_DMA_END, scu_dma_level_0_end);
        scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_1_DMA_END, scu_dma_level_1_end);
        scu_ic_interrupt_set(IC_INTERRUPT_LEVEL_2_DMA_END, scu_dma_level_2_end);
        scu_ic_interrupt_set(IC_INTERRUPT_DMA_ILLEGAL, scu_dma_illegal);
        scu_ic_mask_chg(IC_MASK_ALL & ~mask, IC_MASK_NONE);
        cpu_intc_enable();

        vdp2_tvmd_display_set(); 
        
        vdp2_scrn_back_screen_set(/* single_color = */ true, VRAM_ADDR_4MBIT(3, 0x1FFFE), back_screen_color, 1);    

        while (true) 
        {
            
                (void)sprintf(  consbuf, "[01;2H*** SCU DMA from H-RAM to VDP1 ***[02;2HRelease Start to run[03;2HTo configure press:[04;2HA/B/C : SCU DMA Level 0/1/2[05;2HL/R : Mode Direct/Indirect[06;2HDown/Up/Y/Z : Manual/VBI/VBO/HBL[08;2HLevel %d State %d Mode %d Trig %d[11;2HLast exec time: %08lu HBL", 
                level, state.st_status, state.st_level[level].level_mode, state.st_level[level].level_sf, g_dma_counter);   
                cons_buffer(&cons, consbuf);               
            
                vdp2_tvmd_vblank_in_wait();
            
                cons_flush(&cons);
                
                vdp2_tvmd_vblank_out_wait();
                            
                if (!state.st_begin) 
                {
                        if (g_digital.connected == 1)
                        {
                            joyL = g_digital.pressed.button.l;
                            joyR = g_digital.pressed.button.r;
                            joyStart = g_digital.released.button.start;
                            joyA = g_digital.pressed.button.a;
                            joyB = g_digital.pressed.button.b;
                            joyC = g_digital.pressed.button.c;
                            joyUp = g_digital.pressed.button.up;
                            joyDown = g_digital.pressed.button.down;
                            joyY = g_digital.pressed.button.y;                            
                            joyZ = g_digital.pressed.button.z;
                            
                            if(joyA & !oldjoyA)
                                    level = DMA_LEVEL_0;
                            else if (joyB & !oldjoyB)
                                    level = DMA_LEVEL_1;
                            else if (joyC & !oldjoyC)
                                    level = DMA_LEVEL_2;      
                            else if (joyL & !oldjoyL)
                                    state.st_level[level].level_mode = DMA_MODE_DIRECT;
                            else if (joyR & !oldjoyR)
                                    state.st_level[level].level_mode = DMA_MODE_INDIRECT;
                            else if (joyUp & !oldjoyUp)
                                    state.st_level[level].level_sf = DMA_MODE_START_FACTOR_VBLANK_IN;
                            else if (joyY & !oldjoyY)
                                    state.st_level[level].level_sf = DMA_MODE_START_FACTOR_VBLANK_OUT;
                            else if (joyZ & !oldjoyZ)
                                    state.st_level[level].level_sf = DMA_MODE_START_FACTOR_HBLANK_IN;
                            else if (joyDown & !oldjoyDown)
                                    state.st_level[level].level_sf = DMA_MODE_START_FACTOR_ENABLE;       

                            if (joyStart & !oldjoyStart) 
                            {
                                    state.st_begin = true;
                                    cons_buffer(&cons, "[9;2HStarted     "); 
                                    scu_dma_level(level);
                            }
                            else 
                            {
                                    cons_buffer(&cons, "[9;2HNot Started "); 
                            }
                            
                            oldjoyR = joyR;
                            oldjoyL = joyL;
                            oldjoyA = joyA;
                            oldjoyB = joyB;
                            oldjoyC = joyC;
                            oldjoyUp = joyUp;
                            oldjoyDown = joyDown;
                            oldjoyY = joyY;
                            oldjoyZ = joyZ; 
                            oldjoyStart = joyStart;
                            
                        }
                }

                switch (state.st_status) 
                {
                    case ST_STATUS_WAIT:
                            cons_buffer(&cons, "[10;2HStatus wait        "); 
                            break;
                    case ST_STATUS_ILLEGAL:
                            state.st_begin = false;
                            cons_buffer(&cons, "[10;2HStatus illegal     "); 
                            break;
                    case ST_STATUS_END:
                            state.st_begin = false;
                            cons_buffer(&cons, "[10;2HStatus end        "); 
                            state.st_status = ST_STATUS_WAIT;
                            break;
                }
        }
        return 0;
}

static void scu_dma_level(int level __unused)
{
        struct dma_level_cfg cfg;

        cfg.mode.direct.src = (void *)0x06040000;   // High work RAM
        cfg.mode.direct.dst = (void *)0x05C00000;   //VDP1
        
        if(level == DMA_LEVEL_0)
            cfg.mode.direct.len = 0x100000-1;
        else 
            cfg.mode.direct.len = 0x1000-1;
  
        cfg.starting_factor = state.st_level[level].level_sf;
        cfg.add = 3;    // sattech, need to be 001

        scu_dma_cpu_level_set(level, state.st_level[level].level_mode, &cfg);
        
        g_dma_counter = 0;
        g_counting = true;
        scu_dma_cpu_level_start(level); // needed for all triggers ?
}

static void scu_dma_illegal(void)
{
    g_counting = false;
    state.st_status = ST_STATUS_ILLEGAL;
}

static void scu_dma_level_0_end(void)
{
    g_counting = false;
    state.st_status = ST_STATUS_END;
}

static void scu_dma_level_1_end(void)
{
    g_counting = false;
    state.st_status = ST_STATUS_END;
}

static void scu_dma_level_2_end(void)
{
    g_counting = false;
    state.st_status = ST_STATUS_END;
}

static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
    g_frame_counter = (tick > 0) ? (g_frame_counter + 1) : 0;
    smpc_peripheral_digital_port(1, &g_digital);
}

static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
    tick = (tick & 0xFFFFFFFF) + 1;
}

static void hblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
    if(g_counting) g_dma_counter++;
}
