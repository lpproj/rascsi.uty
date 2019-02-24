
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
%define DECLARE_RD_MISC
	%include 'rd_misc.inc'



%ifdef DEBUG
  %define PRINT_CODE	TSR_CODE
  %define PRINT_CODE_END	TSR_CODE_END
  %define PRINT_DATA	TSR_DATA
  %define PRINT_DATA_END	TSR_DATA_END
%else
  %define PRINT_CODE	INIT_CODE
  %define PRINT_CODE_END	INIT_CODE_END
  %define PRINT_DATA	INIT_DATA
  %define PRINT_DATA_END	INIT_DATA_END
%endif


;--------------------------------------

	TSR_CODE

%if 0
; es:di=str -> cx=length (di preserved)
strLen:
	push	ax
	push	di
	mov	al, 0
	mov	cx, 0ffffh
	repne	scasb
	lea	cx, [di - 1]
	pop	di
	pop	ax
	sub	cx, di
	ret
%endif

; copy null-terminated string ds:si -> es:di
strCpy:
	push	ax
	push	si
	push	di
.lp:
	lodsb
	stosb
	cmp	al, 0
	jne	.lp
	pop	di
	pop	si
	pop	ax
	ret

; result CF=1 DBCS lead
;           0 SBCS
IsDBCSLead:
	; Japanese SJIS (CP932) 81-9F, E0-FC
	cmp	al, 81h
	jae	.chk_dbcs
.sbcs:
	clc
	ret
.chk_dbcs:
	cmp	al, 9fh
	jbe	.dbcs
	cmp	al, 0e0h
	jb	.sbcs
	cmp	al, 0fch
	ja	.sbcs
.dbcs:
	stc
	ret




bzero_dssi:
	push	ax
	push	cx
	push	di
	push	es
	mov	di, si
	MOVSEG	es, ds
	mov	al, 0
	rep	stosb
	pop	es
	pop	di
	pop	cx
	pop	ax
	ret


	TSR_CODE_END

;--------------------------------------

CMDLINE_MAX	equ	127
CMDARGS_MAX	equ	10

	PRINT_DATA

putc_func	dw	PutC_dos

	PRINT_DATA_END

	PRINT_CODE

put_h4:
	add	al, '0'
	cmp	al, '9'
	jbe	PutC
	add	al, 'A'-'9'-1
PutC:
	jmp	[cs: putc_func]

PutC_dos:
	push	ax
	push	dx
	mov	dl, al
	mov	ah, 2
	int	21h
	pop	dx
	pop	ax
	ret

PutC_int29:
	push	bx
	int	29h
	pop	bx
	ret

PutH16:
	xchg	ah, al
	call	PutH8
	xchg	ah, al
PutH8:
	push	ax
	push	ax
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	call	put_h4
	pop	ax
	and	al, 0fh
	call	put_h4
	pop	ax
	ret


putd16_zero:
	mov	al, '0'
	call	PutC
	pop	ax
	ret

PutD16:
	push	ax
	test	ax, ax
	jz	putd16_zero
	push	bx
	push	cx
	push	dx
	mov	bx, 10
	xor	cx, cx
.lp:
	inc	cx
	xor	dx, dx
	div	bx
	push	dx
	test	ax, ax
	jnz	.lp
	mov	dx, ax
.lp2:
	pop	ax
	or	dx, ax
	jz	.lp2_2
	call	put_h4
.lp2_2:
	loop	.lp2
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

PutS0:
	push	ax
	push	si
	mov	si, dx
.lp:
	lodsb
	cmp	al, 0
	je	.brk
	call	PutC
	jmp	short .lp
.brk:
	pop	si
	pop	ax
	ret

PutS0n:
	call	PutS0
PutNL:
	push	ax
	mov	al, 13
	call	PutC
	mov	al, 10
	call	PutC
	pop	ax
	ret



%ifdef DEBUG

PutRegs:
	pushf		; +20
	push	ax	; +18
	push	cx	; +16
	push	dx	; +14
	push	bx	; +12
	mov	bx, sp
	push	sp	; +10
	push	bp	; +8
	push	si	; +6
	push	di	; +4
	push	ds	; +2
	push	es	; +0
	push	cs
	pop	ds
	mov	bp, sp
	mov	dx, .msg_ax
	call	PutS0
	mov	ax, [bp + 18]
	call	PutH16
	mov	dx, .msg_bx
	call	PutS0
	mov	ax, [bp + 12]
	call	PutH16
	mov	dx, .msg_cx
	call	PutS0
	mov	ax, cx
	call	PutH16
	mov	dx, .msg_dx
	call	PutS0
	mov	ax, [bp + 14]
	call	PutH16
	mov	dx, .msg_bp
	call	PutS0
	mov	ax, [bp + 8]
	call	PutH16
	mov	dx, .msg_sp
	call	PutS0
	mov	ax, bx
	add	ax, 12
	call	PutH16
	mov	dx, .msg_si
	call	PutS0
	mov	ax, si
	call	PutH16
	mov	dx, .msg_di
	call	PutS0
	mov	ax, di
	call	PutH16
	call	PutNL
	mov	dx, .msg_ds
	call	PutS0
	mov	ax, [bp + 2]
	call	PutH16
	mov	dx, .msg_es
	call	PutS0
	mov	ax, [bp]
	call	PutH16
	mov	dx, .msg_ss
	call	PutS0
	mov	ax, ss
	call	PutH16
	mov	dx, .msg_flags
	call	PutS0
	mov	ax, [bp + 20]
	call	PutH16
	call	PutNL
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	pop	ax	; drop
	pop	bx
	pop	dx
	pop	cx
	pop	ax
	popf
	ret
.msg_ax:
	db	' AX:', 0
.msg_bx:
	db	' BX:', 0
.msg_cx:
	db	' CX:', 0
.msg_dx:
	db	' DX:', 0
.msg_bp:
	db	' BP:', 0
.msg_sp:
	db	' SP:', 0
.msg_si:
	db	' SI:', 0
.msg_di:
	db	' DI:', 0
.msg_ds:
	db	' DS:', 0
.msg_es:
	db	' ES:', 0
.msg_ss:
	db	' SS:', 0
.msg_flags:
	db	' flags:', 0

%endif ; DEBUG

	PRINT_CODE_END



	INIT_CODE

CharUpr:
	cmp	al, 'a'
	jb	.exit
	cmp	al, 'z'
	ja	.exit
	sub	al, 'a' - 'A'
.exit:
	ret


ParseCmd:
	push	ax
	push	si
	push	di
	push	es
	push	ds
	pop	es
	mov	si, cmdline
	mov	di, cmd_arg
	xor	cx, cx

.lp:
	lodsb
.lp_1:
	cmp	si, cmdline_tail
	jae	.exit
	cmp	al, 13
	je	.exit
	cmp	al, 10
	je	.exit
	cmp	al, 20h
	jbe	.lp
	dec	si
	mov	ax, si
	stosw
	inc	cx

.lp2:
	lodsb
	cmp	si, cmdline_tail
	jae	.exit
	cmp	al, 20h
	ja	.lp2
	mov	byte [si-1], 0
	jmp	short .lp_1
.exit:
	mov	[cmd_argc], cx
	pop	es
	pop	di
	pop	si
	pop	ax
	ret


	INIT_DATA

cmd_argc	dw	0
cmd_arg:
	times (CMDARGS_MAX + 1) dw 0
cmdline:
	times (CMDLINE_MAX) db 0
cmdline_tail:
	db	0


	INIT_DATA_END


	INIT_CODE

; 
; es:bx = source command line (terminated with CR or LF)
; cx = max length (<=127)
;
; result:
; ds:cmdline copied command line (terminated with CR)
CopyCmdLine:
	push	cx
	push	si
	push	di
	push	ds
	push	es
	; xchg ds, es
	push	ds
	push	es
	pop	ds
	pop	es
	mov	si, bx
	mov	di, cmdline
	jcxz	.exit
.lp:
	lodsb
	cmp	al, 13
	je	.exit
	cmp	al, 10
	je	.exit
	; null to space (workaround for device driver on NEC98 MS-DOS)
	cmp	al, 0
	jne	.l2
	mov	al, ' '
.l2:
	stosb
	loop	.lp
.exit:
	mov	al, 13
	stosb
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	cx
	ret



	INIT_CODE_END
