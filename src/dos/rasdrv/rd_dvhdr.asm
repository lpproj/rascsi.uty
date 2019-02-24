
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

; %define HAVE_DEVICE_API 1

%ifdef HAVE_DEVICE_API
DEVATTR		equ	0c000h
%else
DEVATTR		equ	8000h
%endif

; device header

	TSR_HEADER

	extern DeviceInitCommands
	global device_packet

	global device_header
	global device_attributes
	global DeviceCommands
	global device_command_offset
	global device_name

	global FindDevice
	global UnlinkDevice
	global HookInts
	global ReleaseInts

	global tsr_id
	global tsr_psp
	global int_table
	global drive_number

	global int_table_2f
	global org_2f

	global tsr_bottom


;--------------------------------------
device_header:
	dw	0ffffh, 0ffffh
device_attributes:
	dw	DEVATTR
	dw	DeviceStrategy
device_command_offset:
	dw	DeviceInitCommands
device_name:
	db	'$RASDRV$'

tsr_id	db	'RASDRV'
device_id_end:
	db	'00'
tsr_psp:
	dw	0
drive_number:
	db	0
	db	0

int_table:
int_table_2f:
	db	2fh, 0		; vector, do_hook/is_hooked
org_2f:
	dd	0		; org
;new2f_pointer
	dd	0		; new

	db	0


	TSR_HEADER_END

	TSR_CODE

DeviceStrategy:
	mov	word [cs: device_packet], bx
	mov	word [cs: device_packet + 2], es
	retf

%ifdef HAVE_DEVICE_API

	extern DeviceAPI

;DeviceAPI:		; DeviceAPI_dummy
;	; error: ax = -1, CF=1
;	mov	ax, 0ffffh
;	stc
;	retf

DeviceCommands:
	pushf
	push	bx
	push	ds
	lds	bx, [cs: device_packet]
	cmp	byte [bx + 2], 3	; IOCTL INPUT?
	je	.ioctlin
	mov	word [bx + 3], 8103h
.exit:
	pop	ds
	pop	bx
	popf
	retf
.ioctlin:
	cmp	word [bx + 18], 4
	je	.ioctlin_l2
	mov	word [bx + 3], 810bh
	mov	word [bx + 18], 0
	jmp	short .exit
.ioctlin_l2:
	mov	word [bx + 3], 0100h
	lds	bx, [bx + 14]
	mov	word [bx], DeviceAPI
	mov	word [bx + 2], cs
	jmp	short .exit
%else

DeviceCommands:
	push	bx
	push	ds
	lds	bx, [cs: device_packet]
	mov	word [bx + 3], 8103h
	pop	ds
	pop	bx
	retf
%endif

	TSR_CODE_END

	TSR_DATA

device_packet	dd	0

	TSR_DATA_END


	INIT_DATA
	INIT_DATA_END

	INIT_CODE

;
; result
; CF=0 device found
;   es:bx device header
; CF=1 not found
FindDevice:
	push	cx
	push	si
	push	di
	mov	ah, 52h
	int	21h
	add	bx, 22h		; actual NUL: don't care DOS 2.x and 3.0
.lp:
	mov	ax, es
	mov	cx, ds
	mov	si, device_name
	lea	di, [bx + si]
	; skip myself
	cmp	ax, cx
	jne	.chk
	cmp	si, di
	je	.next
.chk:
	mov	ax, 1
	mov	cx, device_id_end - device_name
	repe	cmpsb
	clc
	je	.found		; found (CF=0)
.next:
	add	ax, [es: bx]
	jc	.exit		; not found (offset of next dev=ffff, CF=1)
	les	bx, [es: bx]
	jmp	short .lp
.found:
	mov	ax, [es: bx + device_id_end]	; ah=as_device, al=internal version
.exit:
	pop	di
	pop	si
	pop	cx
	ret


UnlinkDevice:
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	mov	si, bx
	mov	di, es
	mov	ah, 52h
	int	21h
	add	bx, 22h		; NUL header (DOS 3.1+)
.lp:
	mov	cx, [es: bx]
	mov	dx, [es: bx + 2]
	cmp	cx, si
	jne	.l2
	cmp	dx, di
	jne	.l2
	; if the deivce is found in the next chain...
	push	ds
	lds	di, [es: bx]		; the next device hdr 
	lds	di, [si]		; next chain in the next
	mov	[es: bx], di
	mov	[es: bx + 2], ds
	pop	ds
	clc
	jmp	short .exit
.l2:
	cmp	cx, 0ffffh	; end of device chain?
	je	.err
	les	bx, [es: bx]
	jmp	short .lp
.err:
	stc
.exit:
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret


;
; hook interrupt vector(s)
; ds:si    int_table
;
HookInts:
	push	ax
	push	bx
	push	dx
	push	si
	push	es
.lp:
	mov	ax, [si]
	test	al, al
	jz	.brk
	test	ah, ah
	jz	.next
	mov	ah, 35h
	int	21h
	mov	[si + 2], bx
	mov	[si + 4], es
	mov	dx, [si + 6]	; do not hook if new==0000:0000
	mov	bx, [si + 8]
	test	dx, dx
	jnz	.lp_hk
	test	bx, bx
	jz	.next
.lp_hk:
	mov	ah, 25h
	mov	al, [si]
	push	ds
	mov	ds, bx
	int	21h
	pop	ds
.next:
	add	si, 10
	jmp	short .lp
.brk:
	pop	es
	pop	si
	pop	dx
	pop	bx
	pop	ax
	ret

; 
; unhook interrupt vector(s)
;
; es:bx tsr(device) header
;
; result:
; CF=0 success
; CF=1 failure (vector is hooked by another promgram(s))
;
ReleaseInts:
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	es
	lea	si, [bx + int_table]
.lp_chk:
	mov	ax, [es: si]
	test	al, al
	jz	.do_unhook
	test	ah, ah
	jz	.next_chk
	push	bx
	push	es
	mov	ah, 35h
	int	21h
	mov	dx, es
	mov	cx, bx
	pop	es
	pop	bx
	cmp	cx, [es: si + 6]
	jne	.err
	cmp	dx, [es: si + 8]
	jne	.err
.next_chk:
	add	si, 10
	jmp	short .lp_chk
.err:
	stc
.exit:
	pop	es
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
.do_unhook:
	lea	si, [bx + int_table]
.lp_uh:
	mov	ax, [es: si]
	test	al, al		; CF=0
	jz	.exit
	test	ah, ah
	jz	.next_uh
	push	ds
	lds	dx, [es: si + 2]
	mov	ah, 25h
	int	21h
	pop	ds
.next_uh:
	add	si, 10
	jmp	short .lp_uh


	INIT_CODE_END


	TSR_BOTTOM
tsr_bottom:
	TSR_BOTTOM_END

