
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
	%include 'rd_int2f.inc'
	%include 'rd_misc.inc'
	%include 'rd_scsi.inc'
	%include 'rd_msg.inc'

	global opt_d_ptr

	extern device_header
	extern device_attributes
	extern device_command_offset
	extern device_packet
	extern DeviceCommands
	global DeviceInitCommands
	extern FindDevice
	extern UnlinkDevice
	extern HookInts
	extern ReleaseInts

	extern rd_drive
	extern rd_basepath

	extern int_table
	extern int_table_2f
	extern New2F
	extern org_2f
	extern tsr_id
	extern tsr_psp

	extern tsr_bottom:wrt AGROUP



	INIT_DATA
opt_a		db	0
opt_i		db	6	; 0ffh
opt_r		db	0
opt_h		db	0
opt_err		db	0
opt_d_ptr	dw	0
	INIT_DATA_END

	INIT_CODE

GetOpt:
	push	ax
	push	bx
	push	si
	mov	bx, cmd_arg
.lp1:
	mov	si, [bx]
	add	bx, 2
	test	si, si
	jz	.exit
	lodsb
	cmp	al, '/'
	je	.opt
	cmp	al, '-'
	je	.opt
	jmp	.err
.opt:
	lodsb
	call	CharUpr
	cmp	al, '?'
	je	.opt_h
	cmp	al, 'H'
	jne	.opt_2
.opt_h:
	mov	byte [opt_h], 1
	jmp	short .lp1
.opt_2:
	cmp	al, 'R'
	jne	.opt_3
	mov	byte [opt_r], 1
	jmp	short .lp1
.opt_3:
	cmp	al, 'D'
	jne	.opt_4
	call	fetch_param
	jc	.err
	mov	[opt_d_ptr], si
	jmp	short .lp1
.opt_4:
	cmp	al, 'I'
	jne	.opt_5
	call	fetch_param
	jc	.err
	lodsb
	sub	al, '0'
	jb	.err
	cmp	al, 7
	jae	.err
	mov	[opt_i], al
	jmp	short .lp1
.opt_5:
	cmp	al, 'A'
	jne	.opt_6
	call	fetch_param
	jc	.err
	lodsb
	sub	al, '0'
	jb	.err
	cmp	al, 7
	jae	.err
	mov	[opt_a], al
	jmp	short .lp1
.opt_6:
.err:
	mov	byte [opt_err], 1
.exit:
	mov	al, 0ffh
	add	al, byte [opt_err]
	pop	si
	pop	bx
	pop	ax
	ret


fetch_param:
	lodsb
	cmp	al, 0
	je	.nextparam
	cmp	al, ':'
	je	.fetch
	cmp	al, '='
	je	.fetch
	dec	si
.fetch:
	clc
	ret
.nextparam:
	mov	si, [bx]
	or	si, si
	jz	.err
	add	bx, 2
	jmp	short .fetch
.err:
	stc
	ret


	INIT_CODE_END



	INIT_DATA

do_tsr		db	0

	INIT_DATA_END

	INIT_CODE


InitCommon:
	push	cs
	pop	ds
	mov	word [device_command_offset], DeviceCommands
	mov	word [int_table_2f + 6], New2F
	mov	ah, 52h
	int	21h
	mov	[lol], bx
	mov	[lol + 2], es
	mov	[int_table_2f + 8], cs
	mov	byte [int_table_2f + 1], 1

	mov	ax, 3000h
	int	21h
	mov	bx, ax
	mov	ax, 3306h
	int	21h
	xchg	bh, bl
	cmp	bx, 030ah
	jb	.unsupdos
	cmp	bh, 10
	jb	.supdos
.unsupdos:
	mov	dx, err_not_supported_dos
	jmp	.err
.supdos:
	cmp	bh, 4
	jae	.supdos_end
	mov	byte [is_dos3], 1
.supdos_end:

	call	ParseCmd
	call	GetOpt

	mov	dx, msg_title
	call	PutS0
	mov	al, ' '
	call	PutC
	mov	al, '('
	call	PutC
	mov	dx, msg_scsi_platform
	call	PutS0
	mov	al, ')'
	call	PutC
	mov	al, ' '
	call	PutC
	mov	dx, msg_build_date
	call	PutS0n
	cmp	byte [msg_extra_info], 0
	je	.pext_e
	mov	dx, msg_extra_info
	call	PutS0n
.pext_e:

%if 1
	mov	dx, err_cant_install_as_device
	cmp	word [tsr_psp], 0
	je	.err
%endif

	cmp	byte [opt_h], 0
	je	.opt_2
	mov	dx, msg_help
	call	PutS0n
	jmp	short .noerr
.opt_2:
	cmp	byte [opt_r], 0
	jne	.DoRelease

.opt_3:

;.DoInstall:
	mov	dx, err_already_installed
	call	FindDevice
	jnc	.err
	mov	byte [do_tsr], 1

	mov	al, [opt_i]
	mov	ah, 0
	mov	bl, [opt_a]
	call	InitSCSI
	jc	.err

	call	InitFS
	jc	.err

	mov	si, int_table
	call	HookInts

	mov	dx, msg_success
	call	PutS0n
	mov	dx, msg_success_scsiid1
	call	PutS0
	mov	al, [opt_i]
	and	ax, 7
	call	PutD16
	mov	dx, msg_success_scsiid2
	call	PutS0n
	mov	dx, msg_success_drive1
	call	PutS0
	mov	al, [rd_drive]
	and	al, 0dfh		; toupper
	call	PutC
	mov	al, ':'
	call	PutC
	mov	dx, msg_success_drive2
	call	PutS0n
	mov	dx, msg_success_rdbase1
	call	PutS0
	mov	dx, rd_basepath
	test	dx, dx
	jnz	.p_rdbase
	mov	al, '/'
	call	PutC
.p_rdbase:
	call	PutS0
.p_rdbase_exit:
	mov	dx, msg_success_rdbase2
	call	PutS0n

.noerr:
	clc
	ret
.err:
	call	PutS0n
	stc
	ret

.DoRelease:
	mov	dx, err_no_device
	call	FindDevice
	jc	.err
	mov	dx, err_cant_release_device
	mov	cx, [es: bx + tsr_psp]
	jcxz	.err
	call	ReleaseFS
	;jc	.err
	mov	dx, err_cant_release_device
	call	ReleaseInts
	jc	.err
	call	UnlinkDevice
	mov	es, cx
	mov	ah, 49h
	int	21h
	mov	dx, msg_release
	call	PutS0n
	jmp	short .noerr




DeviceInitCommands:		;
	pushf
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es
	cld

	push	cs
	pop	ds
	les	bx, [device_packet]
	cmp	byte [es: bx + 2], 0
	jne	.errexit
	les	bx, [es: bx + 18]	; parameter string after "device="
	; skip argv[0]
.lpprm:
	mov	al, [es: bx]
	inc	bx
	cmp	al, '/'			; in the case of 'device=foo.sys/bar'
	je	.lpprm_brk
	cmp	al, 20h
	ja	.lpprm
.lpprm_brk:
	dec	bx
	mov	cx, 127			; CMDLINE_MAX
	call	CopyCmdLine
	call	InitCommon
	jc	.errexit
	cmp	byte [do_tsr], 0
	je	.noerrexit

.success_dev:
	lds	bx, [device_packet]
	mov	ax, tsr_bottom + 15
	and	ax, 0fff0h
	mov	word [bx + 3], 0100h
	mov	byte [bx + 13], 1
	mov	word [bx + 14], ax
	mov	word [bx + 16], cs
	jmp	short .exit

.noerrexit:
.errexit:
	cmp	byte [is_dos3], 0
	je	.errexit_l2
	mov	word [device_attributes], 0	; turn into blkdev (workaround for old dos)
.errexit_l2:
	lds	bx, [device_packet]
	mov	word [bx + 3], 810ch
	mov	byte [bx + 13], 0
	mov	word [bx + 14], 0
	mov	word [bx + 16], cs
.exit:
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	popf
	retf


..start:
StartCommand:
	cld
	push	cs
	pop	ds
	mov	ah, 51h
	int	21h
	mov	[tsr_psp], bx
	mov	es, bx
	; resize current MCB
	mov	dx, ss
	xchg	bx, dx
	sub	bx, dx
	mov	ax, sp
	test	al, 0fh			; check the case of FFF1~FFFF
	jz	.resizemcb_l2
	inc	bx
.resizemcb_l2:
	mov	cl, 4
	shr	ax, cl
	add	bx, ax
	mov	ah, 4ah
	int	21h

	mov	bx, 0080h
	xor	ch, ch
	mov	cl, [es: bx]
	inc	bx
	call	CopyCmdLine

	call	InitCommon
	jc	.errexit
	cmp	byte [do_tsr], 0
	jne	.success_tsr

.noerrexit:
	mov	ax, 4c00h
	int	21h
.errexit:
	mov	ax, 4c01h
	int	21h

.success_tsr:
%if 0
	les	bx, [lol]
	mov	cx, word [es: bx]
	mov	dx, word [es: bx + 2]
	mov	[device_header], cx
	mov	[device_header + 2], dx
	mov	word [es: bx], device_header
	mov	word [es: bx + 2], cs
%else
	call	FindDevice
	;mov	dx. err_already_installed
	;jc	.errexit
	; add myself to device chain
	mov	word [es: bx], device_header
	mov	word [es: bx + 2], cs
%endif
	mov	es, [tsr_psp]
	xor	ax, ax
	xchg	ax, word [es: 2ch]	; segment of environment
	test	ax, ax
	jz	.success_l2
	; free current enviroment
	push	es
	mov	es, ax
	mov	ah, 49h
	int	21h
	pop	es
.success_l2:
	mov	word [putc_func], PutC_int29
	; ... and stay resident (psp:0000~cs:tsr_bottom)
	mov	dx, cs
	mov	ax, es
	sub	dx, ax
	mov	ax, tsr_bottom + 15
	mov	cl, 4
	shr	ax, cl
	add	dx, ax
	mov	ax, 3100h
	int	21h
	; abnormal termination
	mov	ax, 4cffh
	int	21h

%ifdef DEBUG
putargs:
	mov	cx, [cmd_argc]
	mov	ax, cx
	call	PutD16
	call	PutNL
	mov	si, cmd_arg
	jcxz	.brk
.lp:
	lodsw
	mov	dx, ax
	call	PutS0
	call	PutNL
	loop	.lp
.brk:
	xor	ax, ax
	mov	al, [opt_i]
	call	PutD16
	call	PutNL
	mov	al, [opt_h]
	call	PutD16
	call	PutNL
	mov	al, [opt_r]
	call	PutD16
	call	PutNL
	mov	ax, [opt_d_ptr]
	call	PutH16
	mov	dx, ax
	test	dx, dx
	jz	.b2
	mov	al, ' '
	call	PutC
	call	PutS0
.b2:
	call	PutNL

	mov	ah, 52h
	int	21h
	xor	cx, cx
	mov	cl, [es: bx + 21h]	; lastdrive
	les	bx, [es: bx + 16h]	; cds array
	mov	ax, cx
	call	PutD16
	mov	al, ' '
	call	PutC
	mov	ax, es
	call	PutH16
	mov	al, ':'
	call	PutC
	mov	ax, bx
	call	PutH16
	call	PutNL
.cdslp:
	push	ds
	push	es
	pop	ds
	mov	dx, bx
	call	PutS0n
	pop	ds
	add	bx, 58h
	loop	.cdslp

	ret
%endif


	INIT_CODE_END


;
	INIT_STACK

	resw 128		; 256 bytes

	INIT_STACK_END

