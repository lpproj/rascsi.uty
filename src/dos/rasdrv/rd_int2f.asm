
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
%define DECLARE_RD_INT2F
	%include 'rd_int2f.inc'
	%include 'rd_funcs.inc'
	%include 'rd_scsi.inc'
	%include 'rasdrv.inc'

	extern org_2f

	extern opt_d_ptr
	extern drive_number


	TSR_CODE

New2F:
	pushf
	cmp	ax, 1100h
	jb	.chain_1
	cmp	ax, 112eh
	ja	.chain_1
	RD_PREPARE_REGFRAME
	MOVSEG	ds, cs
	mov	bx, ax
	sub	bx, 1100h
	add	bx, bx
	add	bx, bx
	cmp	word [bx + func2f11_table], 0
	je	.chain_2
	call	word [bx + func2f11_table + 2]
	jne	.chain_2
	cld
	mov	dx, ss
	mov	bp, sp
	cli
	mov	[org_sp], sp
	mov	[org_ss], ss
	sub	byte [in_private_stack], 1
	jc	.injob_1
	; avoid reent (return as error)
	or	byte [bp + r_flags], 01	; CF=1
	mov	word [bp + r_ax], 5		; (access denied)
	jmp	short .injob_3
.injob_1:
	; prepare private stack
	mov	sp, cs
	mov	ss, sp
	mov	sp, private_stack_bottom
.injob_2:
	; push regframe pointer
	push	dx
	push	bp
	mov	bp, sp
	call	word [bx + func2f11_table]
	cli
	add	sp, 4
.injob_3:
	add	byte [cs: in_private_stack], 1
	jnc	.injob_4
	; restore private stack
	mov	ss, [cs: org_ss]
	mov	sp, [cs: org_sp]
.injob_4:
	sti
	RD_DISPOSE_REGFRAME
	popf
	iret
.chain_2:
	RD_DISPOSE_REGFRAME
.chain_1:
	popf
.chain_0:
	jmp	far [cs: org_2f]


;
; invoke dos internal function
; ax bit14..0  func no
;    bit15=1   set DS to DOS DS
;
Invoke_Int2F12xx:
	push	ds
	cli
	mov	[cs: .tmp_bak_sp], sp
	mov	[cs: .tmp_bak_ss], ss
	; switch to DOS stack
	mov	sp, [cs: org_sp]
	mov	ss, [cs: org_ss]
.l2:
	sti
	test	ah, 80h
	jz	.l3
	mov	ds, [cs: org_ss]	; set DS = DOS DS
	and	ah, 7fh
.l3:
	int	2fh
	cli
	mov	ss, [cs: .tmp_bak_ss]
	mov	sp, [cs: .tmp_bak_sp]
	sti
	pop	ds
	ret
.tmp_bak_sp	dw	0
.tmp_bak_ss	dw	0


CheckAlways:
	test	al, 0
	ret

CheckFN1:
	push	ax
	push	si
	push	ds
%if 1
	mov	ax, [rd_drive]
	lds	si, [sda_fn1]
	cmp	ax, [si]
%else
	mov	ah, [rd_drive]
	lds	si, [sda_fn1]
	lodsb
	or	al, 20h			; A-Z -> a-z
	cmp	al, ah
%endif
	pop	ds
	pop	si
	pop	ax
	ret


CheckCDSesdi:
	push	ax
	mov	ax, [es: di + 43h]
	and	ah, 80h
	cmp	ah, 80h			; remote (or ifs) ?
	jne	.exit
	mov	al, [es: di]
	cmp	al, [rd_drive]
.exit:
	pop	ax
	ret


CheckCDS:
	push	di
	push	es
	les	di, [sda_pcds]
	les	di, [es: di]
	call	CheckCDSesdi
	pop	es
	pop	di
	ret

CheckSFTesdi:
	push	ax
	mov	ax, [es: di + 5]
	and	ah, 80h
	cmp	ah, 80h			; remote?
	jne	.exit
	and	al, 1fh
	add	al, 'a'
	cmp	al, [rd_drive]
.exit:
	pop	ax
	ret

CheckDTA:
	push	ax
	push	di
	push	es
	les	di, [sda_pdta]
	mov	al, [rd_drive]
	les	di, [es: di]
	sub	al, 'a'
	or	al, 0c0h
	cmp	al, [es: di]
	pop	es
	pop	di
	pop	ax
	ret

RD_Fallback_error:
	call	RD_FS_GetDOSError
RD_Fallback_error_AX:
	les	bx, [bp]
	mov	[es:bx + r_ax], ax
	or	byte [es: bx + r_flags], 1	; CF=1
	ret


RD_CheckInst:
	les	bx, [bp]
	push	word [es: bx + rd_extra]	; push stack word (for detecting MSCDEX)
	mov	ax, 1100h
	pushf
	call	far [org_2f]
	pop	word [es: bx + rd_extra]
	mov	byte [es: bx + r_al], 0ffh
rd_success_noreg:
	les	bx, [bp]
rd_success_frame_esbx:
	and	byte [es: bx + r_flags], 0feh	; CF=0
	ret


	TSR_CODE_END


	TSR_DATA


func2f11_table:
	dw	RD_CheckInst, CheckAlways	; 2f1100
	dw	RD_Rmdir, CheckFN1		; 2f1101
	dw	0, 0				; 2f1002
	dw	RD_Mkdir, CheckFN1		; 2f1103
	dw	0, 0				; 2f1104
	dw	RD_Chdir, CheckFN1		; 2f1105
	dw	RD_Close, CheckSFTesdi		; 2f1106
	dw	RD_Commit, CheckSFTesdi	; 2f1107
	dw	RD_Read, CheckSFTesdi	; 2f1108
	dw	RD_Write, CheckSFTesdi	; 2f1109
	dw	0, 0 ;RD_Flock, CheckSFTesdi	; 2f110a
	dw	0, 0 ;RD_Funlock3, CheckSFTesdi	; 2f110b
	dw	RD_Diskinfo, CheckCDSesdi ;CheckCDS	; 2f110c
	dw	0, 0; func2f110d, CheckFN1	; 2f110d
	dw	RD_SetAttr, CheckFN1		; 2f110e
	dw	RD_GetAttr, CheckFN1		; 2f110f
	dw	0, 0				; 2f1110
	dw	RD_Rename, CheckFN1		; 2f1111
	dw	0, 0				; 2f1112
	dw	RD_Delete, CheckFN1		; 2f1113
	dw	0, 0				; 2f1114
	dw	0, 0				; 2f1115
	dw	RD_Open, CheckFN1		; 2f1116
	dw	RD_Creat, CheckFN1		; 2f1117
	dw	0, 0 ;RD_CreatWithoutCDS, CheckFN1		; 2f1118
	dw	0, 0 ;RD_FindFirstWithoutCDS, CheckFN1		; 2f1119
	dw	0, 0				; 2f111a
	dw	RD_FindFirst, CheckFN1		; 2f111b
	dw	RD_FindNext, CheckDTA		; 2f111c
	dw	0, 0 ; RD_CloseAll, CheckNone	; 2f111d
	dw	0, 0				; 2f111e
	dw	0, 0				; 2f111f
	dw	0, 0 ; RD_FlushAll, CheckNone	; 2f1120
	dw	RD_SeekFromEnd, CheckSFTesdi	; 2f1121
	dw	0, 0				; 2f1122
	dw	0, 0				; 2f1123
	dw	0, 0				; 2f1124
	dw	0, 0				; 2f1125
	dw	0, 0				; 2f1126
	dw	0, 0				; 2f1127
	dw	0, 0				; 2f1128
	dw	0, 0				; 2f1129
	dw	0, 0				; 2f112a
	dw	0, 0				; 2f112b
	dw	0, 0 ; RD_UpdateSFT, CheckSFTesdi	; 2f112c
	dw	0, 0 ; RD_attr2, CheckSFTesdi		; 2f112d
	dw	RD_Open2, CheckFN1		; 2f112e
func2f11_table_bottom:




lol	dd	0
sda	dd	0

sda_table_top:
sda_pdta:
	dw	000ch
	dw	0
sda_fn1:
	dw	009eh
	dw	0
sda_fn2:
	dw	011eh
	dw	0
sda_searchblock:
	dw	019eh
	dw	0
sda_dirent:
	dw	01b3h
	dw	0
sda_fcb1:
	dw	022bh
	dw	0
sda_fcb2:
	dw	0237h
	dw	0
sda_sattr:
	dw	024dh
	dw	0
sda_psft:
	dw	027eh
	dw	0
sda_pcds:
	dw	0282h
	dw	0
sda_table_bottom:

in_private_stack:
	db	0

is_dos3:
	db	0

MAX_BASEPATH	equ	63	; 67 - 1 (last null-char) - 2 (leading '?:') - 1 (to be safe)

cds_template:
rd_drive:
	db	'a:'		; use lowercase 'a'..'z'
rd_basepath:
	db	'\'
	times	(67 - 3) db 0
	dw	0c080h
	dw	0, 0
	dw	0ffffh, 0ffffh
	dw	0
rd_basepath_offset:
	dw	0
cds_template_bottom:

	TSR_DATA_END


	TSR_DATA

	align 2
org_sp	dw	0
org_ss	dw	0
private_stack:
	times 96 dw 0
private_stack_bottom:

	TSR_DATA_END


;----------------------------------------------------------

	INIT_CODE

; es:di=str -> es:di=pointer to last char (or to '\0' if strlen=0), al=[es:di]
strLastPtr:
	cmp	byte [es: di], 0
	je	.brk
.lp:
	mov	ax, [es: di]
	cmp	ah, 0
	je	.brk
	inc	di
	jmp	short .lp
.brk:
	ret


; al=drive -> es:di=&(CDS[drive])
getCDSforDrive:
	push	ax
	les	di, [lol]
	les	di, [es: di + 16h]	; &(CDS[0])
	mov	ah, 58h
	cmp	byte [is_dos3], 0
	je	.l2
	mov	ah, 51h
.l2:
	mul	ah
	add	di, ax
	pop	ax
	ret


SetupCDS:
	push	ax
	push	bx
	push	cx
	push	si
	push	di
	push	es
	
	push	dx
	
	les	bx, [lol]
	mov	ax, [es: bx + 20h]		; al=installed drives, ah=number of CDS
	mov	dx, err_invalid_drive_letter
	mov	si, [opt_d_ptr]
	test	si, si
	jz	.chkdrive
	lodsb
	call	CharUpr
	sub	al, 'A'
	jb	.err
	cmp	al, 'Z' - 'A'
	ja	.err
.chkdrive:
	MOVSEG	es, ds
	cmp	al, ah
	jae	.err
	mov	[drive_number], al
	add	byte [rd_drive], al
	test	si, si
	jnz	.chkpath
	mov	si, basepath_default
	jmp	short .copypath
.chkpath:
	mov	dx, err_invalid_root_path
	lodsb
	cmp	al, ':'
	je	.copypath
	cmp	al, '='
	je	.copypath
	jmp	 .err
.copypath:
	mov	cx, MAX_BASEPATH
	mov	di, rd_basepath
.cp_lp:
	lodsb
	;db	3eh	; ds:
	stosb
	cmp	al, 0
	je	.cp_brk
	loop	.cp_lp
	mov	dx, err_root_path_too_long
	jmp	short .err
.cp_brk:
	mov	di, rd_basepath
	call	strLastPtr
	cmp	al, '/'
	je	.setcds
	cmp	al, '\'
	je	.setcds
.drop_lastsep:
	inc	di
	mov	word [di], '/'
.setcds:
; prepare CDS
	mov	cx, di
	mov	si, cds_template
	sub	cx, si
	mov	[rd_basepath_offset], cx
	inc	cx
	push	si
.pathconv_lp:
	cmp	byte [si], '/'
	jne	.pathconv_next
	mov	byte [si], '\'
.pathconv_next:
	inc	si
	loop	.pathconv_lp
.pathconv_brk:
;
	MOVSEG	es, ds
	mov	di, cds_template
	mov	si, rd_buffer
	call	RD_checkdir
	pop	si
	mov	dx, err_invalid_basepath
	jc	.err
	cmp	ah, 0
	jnz	.err

	mov	al, [drive_number]
	call	getCDSforDrive
	mov	dx, err_drive_already_use
	test	word [es: di + 43h], 0c000h
	jnz	.err
	; copy template to actual CDS
	mov	cx, cds_template_bottom - cds_template
	rep	movsb
	jmp	short .noerr

.err:
	pop	ax	; drop original dx
	stc
	jmp	short .exit

.noerr:
	clc
	pop	dx
.exit:
	pop	es
	pop	di
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret


InitFS:
	cmp	byte [is_dos3], 0
	je	.setup_sda
	mov	word [sda_fn1], 0092h
	mov	word [sda_fn2], 0112h
	mov	word [sda_searchblock], 0192h
	mov	word [sda_dirent], 01a7h
	mov	word [sda_fcb1], 0218h
	mov	word [sda_fcb2], 0224h
	mov	word [sda_sattr], 023ah
	mov	word [sda_psft], 0268h
	mov	word [sda_pcds], 026ch
.setup_sda:
	push	ds
	mov	ax, 5d06h
	int	21h
	MOVSEG	es, ds
	pop	ds
	mov	[sda], si
	mov	[sda + 2], es

	mov	cx, (sda_table_bottom - sda_table_top) / 4
	mov	bx, sda_table_top
.lp_wt:
	add	[bx], si
	mov	[bx + 2], es
	add	bx, 4
	loop	.lp_wt
%if 0
	MOVSEG	es, ds
	mov	di, rd_basepath
	call	RD_checkdir
	mov	dx, err_invalid_basepath
	jc	.err
%endif
	call	SetupCDS
	jc	.err

.noerr:
	clc
	ret
.err:
	stc
	ret


ReleaseFS:
	push	ax
	push	di
	push	es
	mov	al, [es: bx + drive_number]
	cmp	al, 'Z' - 'A'
	ja	.err
	call	getCDSforDrive
	test	word [es: di + 43h], 08000h	; network?
	jz	.err
	mov	word [es: di + 43h], 0000h
	add	al, 'A'
	mov	ah, ':'
	stosw
	mov	ax, '\'
	stosw
	clc
	jmp	short .exit
.err:
	stc
.exit:
	pop	es
	pop	di
	pop	ax
	ret




	INIT_CODE_END

	INIT_DATA
cds_for_the_drive:
	dd	0
basepath_default:
	db	'/home/pi', 0
	INIT_DATA_END
