
%if 0

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

%endif


	%include 'segdef.inc'
	%include 'rd_misc.inc'
	%include 'rd_msg.inc'
	%include 'rd_int2f.inc'
	%include 'rd_funcs.inc'
	%include 'rd_scsi.inc'
	%include 'rasdrv.inc'
%define DECLARE_RD_FSDBG
	%include 'rd_fsdbg.inc'


%ifdef DEBUG

	TSR_CODE

dbgPutFN1:
	pushf
	push	ax
	push	dx
	push	ds
	push	cs
	pop	ds
	mov	dx, .msg
	call	PutS0
	lds	dx, [sda_fn1]
	call	PutS0n
	pop	ds
	pop	dx
	pop	ax
	popf
	ret
.msg:
	db	'FN1=', 0

dbgPutFN2:
	pushf
	push	ax
	push	dx
	push	ds
	push	cs
	pop	ds
	mov	dx, .msg
	call	PutS0
	lds	dx, [sda_fn2]
	call	PutS0
	pop	ds
	pop	dx
	pop	ax
	popf
	ret
.msg:
	db	'FN2=', 0


dbgDumpMem_sub:
	pushf
	push	ax
	push	si
	push	ds
	MOVSEG	ds, cs
	call	PutS0
	mov	ax, es
	call	PutH16
	mov	al, ':'
	call	PutC
	mov	ax, di
	call	PutH16
	call	PutNL
	mov	dx, cx
	xor	cx, cx
	mov	si, di
	MOVSEG	ds, es
.lp:
	cmp	cx, dx
	jae	.brk
	test	cl, 15
	jnz	.lp_l1
	mov	ax, cx
	call	PutH16
.lp_l1:
	mov	al, ' '
	call	PutC
	lodsb
	call	PutH8
	inc	cx
	test	cl, 15
	jnz	.lp
	call	PutNL
	jmp	short .lp
.brk:
	test	cl, 15
	jz	.exit
	call	PutNL
.exit:
	pop	ds
	pop	si
	pop	ax
	popf
	ret



dbgPutCDS:
	pushf
	push	ax
	push	bx
	push	dx
	push	ds
	push	cs
	pop	ds
	mov	dx, .msg
	call	PutS0
	lds	bx, [sda_pcds]
	lds	dx, [bx]
	call	PutS0n
	pop	ds
	pop	dx
	pop	bx
	pop	ax
	popf
	ret
.msg:
	db	'CDS=', 0

dbgh68fcb_dsbx:
	push	cx
	push	dx
	push	di
	push	es
	mov	di, bx
	MOVSEG	es, ds
	mov	cx, h68fcb_size
	mov	dx, .msg_fcb
	call	dbgDumpMem_sub
	pop	es
	pop	di
	pop	dx
	pop	cx
	ret
.msg_fcb:
	db	'h68fcb=', 0

dbgPutSFT_sda:
	push	di
	push	es
	les	di, [sda_psft]
	jmp	short dbgPutSFT_esdi.l1
dbgPutSFT_esdi:
	push	di
	push	es
.l1:
	push	cx
	push	dx
	mov	dx, .msg_sft
	mov	cx, 35h
	pushf
	cmp	byte [cs: is_dos3], 0
	jne	.l2
	mov	cl, 3bh
.l2:
	popf
	call	dbgDumpMem_sub
	pop	dx
	pop	cx
	pop	es
	pop	di
	ret
.msg_sft:
	db	'SFT=', 0

dbgPutOpenMode:
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	mov	dx, .msg_stkword
	call	PutS0
	les	bx, [bp]
	mov	ax, [bx + rd_extra]
	call	PutH16
	mov	dx, .msg_mode
	call	PutS0
	les	bx, [sda_sattr]
	mov	al, [es: bx + 1]
	call	PutH8
	cmp	byte [is_dos3], 0
	jne	.putsft
	les	bx, [sda]
	mov	dx, .msg_xmode
	call	PutS0
	; DOS 4+: extended open mode
	mov	ax, [bx + 2ddh]
	call	PutH16
	mov	dx, .msg_xattr
	call	PutS0
	mov	ax, [bx + 2dfh]
	call	PutH16
	mov	dx, .msg_xact
	call	PutS0
	mov	ax, [bx + 2e1h]
	call	PutH16
.putsft:
	pop	es
	mov	dx, .msg_sft_2
	call	PutS0
	mov	ax, [es: di + 2]
	call	PutH16
	mov	al, ' '
	call	PutC
	mov	al, [es: di + 4]
	call	PutH8
	mov	al, ' '
	call	PutC
	mov	ax, [es: di + 5]
	call	PutH16
	call	PutNL
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
.msg_stkword:
	db	'STKWORD=', 0
.msg_mode:
	db	' MODE=', 0	; file open mode: [sda_sattr] + 1
.msg_xmode:
	db	' XMODE=', 0
.msg_xattr:
	db	' XATTR=', 0
.msg_xact:
	db	' XACT=', 0
.msg_sft_2:
	db	' SFTmode=', 0



puts0_cs:
	push	ds
	MOVSEG	ds, cs
	call	PutS0
	pop	ds
	ret

putsn:
	jcxz	.exit
	xchg	dx, si
.lp:
	lodsb
	call	PutC
	loop	.lp
	xchg	dx, si
.exit:
	ret


dbgPutNamests:
	pushf
	push	ax
	push	cx
	push	dx
	push	si
	mov	dx, .msg_path
	call	puts0_cs
	lea	dx, [si + h68namests.path]
	call	PutS0
	mov	dx, .msg_unq_nl
	call	puts0_cs
	mov	dx, .msg_name
	call	PutS0
	mov	cx, 8
	lea	dx, [si + h68namests.name]
	call	putsn
	mov	dx, .msg_unq_nl
	call	puts0_cs
	mov	dx, .msg_ext
	call	PutS0
	mov	cx, 3
	lea	dx, [si + h68namests.ext]
	call	putsn
	mov	dx, .msg_unq_nl
	call	puts0_cs
	pop	si
	pop	dx
	pop	cx
	pop	ax
	popf
	ret
.msg_path:
	db	'PATH="', 0
.msg_name:
	db	'NAME="', 0
.msg_ext:
	db	'EXT= "', 0
.msg_unq_nl:
	db	'"', 13, 10, 0


	TSR_CODE_END


%endif
