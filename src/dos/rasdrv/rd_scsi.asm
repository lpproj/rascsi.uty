
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
%define DECLARE_RD_SCSI
	%include 'rd_scsi.inc'
	%include 'rd_misc.inc'
	%include 'rd_msg.inc'
	%include 'rasdrv.inc'
	%include 'rd_funcs.inc'

%define SCSI_CODE  TSR_CODE
%define SCSI_CODE_END  TSR_CODE_END
%define SCSI_DATA  TSR_DATA
%define SCSI_DATA_END  TSR_DATA_END




	TSR_DATA
scsi_host_id	db 0
scsi_id		db 6
scsi_lun	db 0

scsi_status	db 0
cmd_result	db 0
cmd_result2	db 0

; for 55bios
request_sense_data:
	times REQUEST_SENSE_MAX db 0

	TSR_DATA_END


%ifdef SCSI_55BIOS
	%define scsi_cmd_55bios  scsi_cmd
	%include 'scsi_55.asm'
%endif
%ifdef SCSI_ASPI
	%define scsi_cmd_aspi  scsi_cmd
	%include 'scsi_as.asm'
%endif


	TSR_DATA

rd_scsi_fs_pkt:
	dw	0
	db	0		; from / to
	db	10
	db	2ah		; cdb[0] 2ah=WRITE(10), 28h=READ(10)
	db	0		;
	db	2		; cdb[2] RASETHER=1, RASDRV=2
rd_scsi_fs_pkt_func:
	db	0		; cdb[3] func
	db	0
	db	0
	db	0		; cdb[6] (transfer length upper)
rd_scsi_fs_pkt_len1:
	db	0		; cdb[7] transfer length middle
rd_scsi_fs_pkt_len0:
	db	0		; cdb[8] transfer length lower
rd_scsi_fs_pkt_phase:
	db	0		; cdb[9] phase SENDCMD=0, SENDOPTION=1

rd_fs_result_be32:
rd_fs_result_msb:
	db	0, 0, 0
rd_fs_result_lsb:
	db	0


	TSR_DATA_END

	TSR_CODE

RD_FS_GetDOSError:
	mov	ah, 7fh
	cmp	ah, [rd_fs_result_msb]
	mov	al, [rd_fs_result_lsb]
	mov	ah, [rd_fs_result_lsb - 1]
	jc	.conv
	ret		; ret if CF=0 (not a error)
.conv:
	cmp	ax, RD_FATAL_INVALIDUNIT
	jne	.c2
	mov	ax, 20			; unknown unit
	jmp	short .seterr
.c2:
	cmp	ax, RD_FATAL_WRITEPROTECT
	jne	.c3
	mov	ax, 19			; disk write protected
	jmp	short .seterr
.c3:
	cmp	ax, RD_FATAL_MEDIAOFFLINE
	jne	.c4
	mov	ax, 21			; drive not ready
	jmp	short .seterr
.c4:
	cmp	ax, RD_ERR_LOCKED
	jne	.c5
	mov	ax, 33			; lock violation
	jmp	short .seterr
.c5:
	cmp	ax, RD_ERR_OUTOFLOCK
	jne	.c6
	mov	ax, 36			; sharing buffer overflow
	jmp	short .seterr
.c6:
	cmp	ax, RD_ERR_CANTSEEK
	jne	.c7
	mov	ax, 25			; seek error
	jmp	short .seterr
.c7:
	cmp	ax, RD_ERR_NOTIOCTL	; -17 (FFEF)
	je	.denied
.fallback:
	neg	ax
	cmp	ax, 18		; -1..-18 -> 1..18
	jbe	.seterr
	cmp	ax, 80		; file exists
	je	.seterr
	cmp	ax, 33
	jbe	.denied
	mov	ax, 22		; unknown command
	jmp	short .seterr
.denied:
	mov	ax, 5
.seterr:
	stc
	ret


;
; al=func
; ds:si = rd_fs_packet
; cx = rd_fs_packet length
; es:di = receive buffer (if dx > 0)
; dx = receive buffer length
; result:
; CF=0 noerr
;   ax = low word of FS result status
; CF=1 error
;   ax = error (SCSI status)
;
RD_FS_SendCmd:
	push	dx
	xor	dx, dx
	jmp	short RD_FS_CalCmd.n0
RD_FS_CalCmd:
	push	dx
.n0:
	push	cx
	push	si
	push	di
	push	es
	MOVSEG	es, ds
	mov	di, si
	mov	si, rd_scsi_fs_pkt
	; send cmd packet
	mov	[si], cx
	mov	byte [si + 2], SCSI_TO_DEVICE
	mov	byte [si + 4], 2ah	; WRITE(10)
	mov	[si + 4 + 3], al	; func
	mov	[si + 4 + 7], ch
	mov	[si + 4 + 8], cl
	xor	ax, ax
	mov	[rd_fs_result_be32], ax
	mov	[rd_fs_result_be32 + 2], ax
	mov	byte [si + 4 + 9], al	; phase: 0=SENDCMD
	call	scsi_cmd
	jc	.exit		; err
	; get result
	mov	byte [si + 2], SCSI_FROM_DEVICE
	mov	byte [si + 4], 28h	; READ(10)
	mov	ax, 4
	mov	[si], ax
	mov	[si + 4 + 7], ah
	mov	[si + 4 + 8], al
	mov	byte [si + 4 + 9], 0	; phase: 0=GETRESULT
	mov	di, rd_fs_result_be32
	call	scsi_cmd
	jc	.exit		; err
	mov	al, [rd_fs_result_lsb]
	mov	ah, [rd_fs_result_lsb - 1]
	cmp	dx, 0
	je	.exit
	cmp	ax, 0
	jne	.exit
	mov	byte [si + 2], SCSI_FROM_DEVICE
	mov	byte [si + 4], 28h	; READ(10)
	mov	[si], dx
	mov	[si + 4 + 7], dh
	mov	[si + 4 + 8], dl
	mov	byte [si + 4 + 9], 1	; phase: 1=GETDATA
	pop	es
	pop	di
	call	scsi_cmd
	jmp	short .exit_esdi
.exit:
	pop	es
	pop	di
.exit_esdi:
	pop	si
	pop	cx
	pop	dx
	ret


;
; 'continuous'
; rd_scsi_fs_pkt  scsi packet (after invoked RD_FS_SendCmd/RD_FS_CalCmd)
; es:di buffer
; cx    buffer length
; result:
; CF=0 noerr
;   ax = low word of FS result status
; CF=1 error
;   ax = error (SCSI status or FS result status)
RD_FS_readopt_continuous:
	push	si
	mov	si, rd_scsi_fs_pkt
	mov	[si], cx
	mov	byte [si + 2], SCSI_FROM_DEVICE
	mov	byte [si + 4], 28h	; READ(10)
	mov	[si + 4 + 7], ch
	mov	[si + 4 + 8], cl
	mov	byte [si + 4 + 9], 2	; phase: 2=GETOPTION
	call	scsi_cmd
	pop	si
rd_fs_continuous_exit:
	jc	.exit
	mov	al, [rd_fs_result_lsb]		; ax = low word of the result
	mov	ah, [rd_fs_result_lsb - 1]	;      (by little endian)
.exit:
	ret


RD_FS_sendopt_prephase:
	push	si
	mov	si, rd_scsi_fs_pkt
	mov	[si], cx
	mov	byte [si + 2], SCSI_TO_DEVICE
	mov	byte [si + 4], 2ah	; WRITE(10)
	mov	[si + 4 + 3], al	; func
	mov	[si + 4 + 7], ch
	mov	[si + 4 + 8], cl
	mov	byte [si + 4 + 9], 1	; phase: 1=SENDOPTION
	call	scsi_cmd
	pop	si
	jmp	short rd_fs_continuous_exit


	TSR_CODE_END


	INIT_DATA

	extern rd_buffer
%define inquiry_data  rd_buffer

INQUIRY_DATA_LENGTH equ 36

inq_data:
.devtype	db 0
.modifier	db 0
.version	db 0
.response_data	db 0
.addlength	db 0
.reserved05	db 0
.reserved06	db 0
.flags		db 0
.vendor:	times 8 db 0
.product:	times 16 db 0
.revision:	times 4 db 0
	db	0

rascsi_vendor		db 'RaSCSI  '
rascsi_product_bridge	db 'RASCSI BRIDGE   '

scsi_cmdpkt_inquiry:
	dw INQUIRY_DATA_LENGTH	; transfer data length
	db SCSI_FROM_DEVICE	; transfer direction
	db 6			; cdb length
	db 12h			; cdb[0] INQUIRY
	db 0			; cdb[1]
	db 0			; cdb[2]
	db 0			; cdb[3]
	db INQUIRY_DATA_LENGTH	; cdb[4]
	db 0			; cdb[5]
	db 0, 0, 0, 0, 0, 0

rd_fs_init_first_param:
%ifdef SCSI_ASPI
	db 'RASDRVAS', 0
%else
	db 'RASDRV55', 0
%endif
rd_fs_init_extra_param:
	times 64 db 0
rd_fs_init_extra_param_bottom:
	db 0
	db 0

	INIT_DATA_END


	INIT_CODE

CheckRaSCSI:
	push	ax
	push	cx
	push	dx
	push	si
	push	di
	push	es
	MOVSEG	es, ds
	mov	di, inq_data ;inquiry_data
	mov	[inq_data.product], di	; just waste product info. (to be safe)
	mov	si, scsi_cmdpkt_inquiry
	call	scsi_cmd
	jc	.err
	mov	si, rascsi_product_bridge
	mov	di, inq_data.product
	mov	cx, 16
	repe cmpsb
	jne	.err
	clc
	jmp	short .exit
.err:
	stc
.exit:
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	ax
	ret


InitSCSI:
	mov	dx, err_unsupported_host
%ifdef SCSI_ASPI
	call	InitASPI
	jc	.err
%endif
%ifdef SCSI_55BIOS
	call	Init55BIOS
	jc	.err
%endif
	cmp	al, 7
	mov	dx, err_invalid_scsi_id
	jae	.err
	mov	[scsi_id], ax		; lun:id
	mov	[scsi_host_id], bl
	mov	dx, err_rasbridge_not_exist
	call	CheckRaSCSI
	jc	.err
	;
	mov	si, rd_fs_init_first_param
	mov	cx, h68arg_size
	mov	al, RD_CMD_INITDEVICE
	call	RD_FS_SendCmd
	mov	dx, err_rasdrv_init_failure
	jc	.err
	test	byte [rd_fs_result_be32], 80h	; CF=0, ZF = (dst & 0x80) == 0
	jnz	.err
	cmp	ax, 1				; number of units (if not err)
	jae	.exit
.err:
	stc
.exit:
	ret


	INIT_CODE_END



