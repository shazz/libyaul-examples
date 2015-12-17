! 152-byte LZSS bootup decoder for the SH2

	.section .text
	.align 1
	.global		_start
_start:
	mov.l	text_buf,r3
	mov.l	bin_size,r4
	mov.l	bin_start,r8
	mov.l	bin_dest,r6
	mov	#32,r1
	mov.w	N_minus_F,r0
	mov	r0,r7
_init_text_buf:			! Set all bytes in text_buf to 0x20
	add	#-1,r0
	mov.b	r1,@(r0,r3)
	cmp/pl	r0
	bt	_init_text_buf
	mov.w	buf_pos_mask,r9
	mov.l	text_buf,r11
	mov	#0,r3		! Input stream position
_lzss_loop:
	cmp/hs	r4,r3
	bt	_lzss_done
	mov	r8,r0
	mov.b	@(r0,r3),r5	! The first byte of each block contains the flags
	mov	#8,r10		! 8 flags in one byte
	add	#1,r3
_check_flags:
	shlr	r5		! Shift the next flag into T
	bf	_flag_clear
	cmp/hs	r4,r3		! Make sure we're not beyond the last byte of the input stream
	bt	_lzss_done	
	mov	r8,r0
	mov.b	@(r0,r3),r1		
	mov	r11,r0
	add	#1,r3
	bra	_output_byte
	mov	#1,r2		! Make sure the bf after _output_byte isn't taken
_flag_clear:
	cmp/hs	r4,r3
	bt	_lzss_done
	mov	r8,r0
	mov.b	@(r0,r3),r2
	extu.b	r2,r2
	add	#1,r3
	mov.b	@(r0,r3),r1
	add	#1,r3
	extu.b	r1,r12
	shlr2	r12
	shlr2	r12
	shll8	r12		! r12 = (r12 & 0xf0) << 4
	or	r2,r12
	mov	#15,r2
	and	r1,r2
	and	r9,r12
	mov	r11,r0
	bra	_next_string_pos
	add	#4,r2
_copy_string:
	mov.b	@(r0,r12),r1
	add	#1,r12
	and	r9,r12
_output_byte:
	mov.b	r1,@r6		! Write to the output stream
	mov.b	r1,@(r0,r7)	! Write to text_buf
	add	#1,r7
	add	#1,r6
	and	r9,r7
_next_string_pos:
	dt	r2
	bf	_copy_string
_next_flag:
	dt	r10
	bt	_lzss_loop
	bra	_check_flags
	or	r0,r0
_lzss_done:
	mov.l	bin_dest,r6
	jmp	@r6
	or	r0,r0
	
	.align 2
text_buf:
	.long	_text_buf
bin_start:
	.long _bin_start
bin_size:
	.long _bin_end-_bin_start
bin_dest:
	.long 0x6004000
N_minus_F:
	.short	4078
buf_pos_mask:
	.short	4095

_bin_start:
! Put your compressed binary here	
.incbin "cube.lzss"
_bin_end:
	.comm	_text_buf,4078,4
	