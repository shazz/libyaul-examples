#include "vdp2_vram_wrapper.h"

/*
 *  void init_VRAM_access(void)
 *  Set VRAM access default priorities
 */
void init_VRAM_access(int bank_mode)
{
    struct vram_ctl *vram_ctl;
    
    vram_ctl = vdp2_vram_control_get();

	if(bank_mode == VRAM_4_BANKS || bank_mode == VRAM_2_BANKS || bank_mode == VRAM_2_1_BANKS || bank_mode == VRAM_1_2_BANKS)
	{
		// Bank A0
		vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	
		vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS; 
		vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS; 
		vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;
	}
	
	if(bank_mode == VRAM_4_BANKS || bank_mode == VRAM_2_BANKS || bank_mode == VRAM_2_1_BANKS || bank_mode == VRAM_1_2_BANKS)
	{
		// Bank A1
		vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	
		vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;     
		vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
	}
		
	if(bank_mode == VRAM_4_BANKS || bank_mode == VRAM_2_1_BANKS || bank_mode == VRAM_1_2_BANKS)
	{
		// Bank B0
		vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS; 	
		vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_NO_ACCESS; 	
		vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;	
		
	}
	
	if(bank_mode == VRAM_4_BANKS)
	{
		// Bank B1
		vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
		vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;	 
	
	}   
	
    vdp2_vram_control_set(vram_ctl);   
}

/*
 *  void set_VRAM_access(struct vdp2_timings_t[] timings)
 *  Set VRAM access specific priorities
 */
void set_VRAM_access(struct vdp2_timings_t[] timings)
{
    struct vram_ctl *vram_ctl;
    uint8_t index;
	
    vram_ctl = vdp2_vram_control_get();

	for(index=0; index < (sizeof(timings)/sizeof(vdp2_timings)); index++)
	{
			struct vdp2_timings_t s_timing = timings[index];
			
			switch(s_timing.timing)
			{
				case VRAM_TIMING_T1:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t1 = s_timing.type;
					break;
				case VRAM_TIMING_T2:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t2 = s_timing.type;
					break;
				case VRAM_TIMING_T3:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t3 = s_timing.type;
					break;
				case VRAM_TIMING_T4:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t4 = s_timing.type;
					break;
				case VRAM_TIMING_T5:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t5 = s_timing.type;
					break;
				case VRAM_TIMING_T6:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t6 = s_timing.type;
					break;
				case VRAM_TIMING_T7:
					vram_ctl->vram_cycp.pt[vdp2_timings.bank].t7 = s_timing.type;
					break;
				default:
					break;
			}
	}
	
    vdp2_vram_control_set(vram_ctl);   
}


