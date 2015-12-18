! apLib decruncher for SuperH-2 (Sega 32X / Saturn)
!
! Based on the M68000 version by MML 2010
! Size optimized (164 bytes) by Franck "hitchhikr" Charlet. 
!
! SH2 version (210 bytes) by Mic 2010

	.section .text
	.align 1
	.global		_start	

_start:
	mov.l	bin_start,r4
	mov.l	bin_dest,r5
	
	bsr	aplib_decrunch
_unpack_done:
	mov.l	bin_dest,r6
	jmp	@r6			/* r5 is saved... */
	or	r0,r0
	
! In: r4 = compressed data, r5 = destination
!
aplib_decrunch:
	mov.l	r8,@-r15
	sts.l	pr,@-r15

    	mov	#1,r6		! Initialize bits counter
copy_byte:
	mov.b	@r4+,r1
	mov.b	r1,@r5
	add	#1,r5
next_sequence_init:
	mov	#0,r7		! Initialize LWM
next_sequence:
	bsr   	get_bit		! if bit sequence is %0..., then copy next byte
	nop
	bf	copy_byte
	bsr	get_bit
	nop
	bf	code_pair	! if bit sequence is %10..., then is a code pair
	bsr	get_bit
	mov	#0,r0
	bf	short_match	! if bit sequence is %110..., then is a short match
	
       ! The sequence is %111..., the next 4 bits are the offset (0-15)
	bsr	get_bit
        mov	r5,r1
	bsr	get_bit
	addc	r0,r0
	bsr	get_bit
	addc	r0,r0
	bsr	get_bit
	addc	r0,r0
	addc	r0,r0
	
	tst	r0,r0
	bt	write_byte	! if offset == 0, then write 0x00
	
        ! If offset != 0, then write the byte on destination - offset
        sub	r0,r1
        mov.b	@r1,r0
write_byte:
        mov.b	r0,@r5
        bra	next_sequence_init
        add	#1,r5

! Code pair %10...
code_pair:     
	bsr	decode_gamma
	nop
	mov	r2,r0
	dt	r0
	dt	r0
	bf	normal_code_pair
	tst	r7,r7
	bf	normal_code_pair
	bsr	decode_gamma
	mov	r8,r0		! offset = old_offset
	bra	copy_code_pair
	nop
normal_code_pair:
	add	r7,r0
	dt	r0
	mov.b	@r4+,r1
	shll8	r0
	extu.b	r1,r1
	bsr	decode_gamma
	or	r1,r0
	mov	#0x7D,r1
	shll8	r1
	cmp/hs	r1,r0
	bf	compare_1280
	add	#1,r2
compare_1280:
	mov	#5,r1
	shll8	r1
	cmp/hs	r1,r0
	bf	compare_128
	bra	continue_short_match
	add	#1,r2
compare_128:
	mov	#0x7F,r1
	cmp/hi	r1,r0
	bt	continue_short_match
	bra	continue_short_match
	add	#2,r2

! get_bit: Get bits from the crunched data (r3) and insert the most significant bit in the carry flag.
get_bit: 
	dt	r6
	bf	still_bits_left
	mov	#8,r6
	mov.b	@r4+,r3
	shll16	r3
	shll8	r3
still_bits_left:
	rts
	shll	r3		
	
! decode_gamma: Decode values from the crunched data using gamma code
decode_gamma:
	sts	pr,r1		! Save return address
	mov   	#1,r2
get_more_gamma:
	bsr   	get_bit
	nop
	bsr	get_bit
	addc	r2,r2
	bt	get_more_gamma
	jmp	@r1		! Return
	nop
	
! Short match %110...
short_match:  
	mov	#1,r2
	mov.b	@r4+,r0		! Get offset (offset is 7 bits + 1 bit to mark if copy 2 or 3 bytes) 
	extu.b	r0,r0
	shlr	r0
	addc	r2,r2		! r2 becomes either 2 or 3
	tst	r0,r0
	bt	end_decrunch
continue_short_match:
	mov	r0,r8		! old_offset = offset
copy_code_pair:   
	mov	r5,r1
	sub	r0,r1
loop_do_copy:
	mov.b	@r1+,r0
	mov.b	r0,@r5
	dt	r2
	bf/s	loop_do_copy
	add	#1,r5
	
        bra   	next_sequence   ! Process next sequence
	mov	#1,r7		! LWM = 1
	
end_decrunch:
	lds.l	@r15+,pr
	mov.l	@r15+,r8	
	rts

	.align 2
bin_start:
	.long _bin_start
bin_dest:
	.long 0x6004000
_bin_start:
! Put your compressed binary here	
.incbin "cube.ap"
_bin_end:
