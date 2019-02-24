
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

%ifndef SCSI_CODE
  %define SCSI_CODE
  %define SCSI_CODE_END
%endif
%ifndef SCSI_DATA
  %define SCSI_DATA
  %define SCSI_DATA_END
%endif

%ifndef REQUEST_SENSE_MAX
  %define REQUEST_SENSE_MAX  18
%endif


;
;
;
;

%if 0
;-----------------------------------------------------------------------------
scsi_id		db 6
scsi_lun	db 0

scsi_status	db 0
cmd_result	db 0
cmd_result2	db 0

SCSI_NO_TRANSFER	equ 0
SCSI_FROM_DEVICE	equ 1
SCSI_TO_DEVICE		equ 2

struc	; SCSI_CMD_PACKET
	scsi_data_length	resw	1	; transfer data length (by word)
	scsi_transfer_direction	resb	1	; transfer direction (by byte: 0, 1, 2)
	scsi_cdb_length		resb	1	; cdb length (by byte: 6, 10 or 12)
	scsi_cdb		resb	12	; cdb (12 bytes)
endstruc

REQUEST_SENSE_MAX	equ 18
request_sense_data:
	times REQUEST_SENSE_MAX db 0

;-----------------------------------------------------------------------------
%endif

	SCSI_DATA

SCSI_NO_TRANSFER_55	equ 00h
SCSI_FROM_DEVICE_55	equ 44h
SCSI_TO_DEVICE_55	equ 48h


scsi55_cmdpkt:
	times 16 db 0

scsi55_cmdpkt_reqsense:
	db 0
	db SCSI_FROM_DEVICE_55
	db 6			; 6bytes cdb
	db 0			; reserved
	db 03h			; CDB0 REQUEST SENSE
	db 0			;    1 (lun)
	db 0			;    2
	db 0			;    3
	db REQUEST_SENSE_MAX	;    4 (allocation length)
	db 0			;    5 (control)
	db 0, 0, 0, 0, 0, 0

	SCSI_DATA_END

	SCSI_CODE

;
; ds:si command parameters
; es:di data buffer pointer
; [ds: scsi_id]   unit id (0..6)
; [ds: scsi_lun]  unit lun
;
; return:
; [ds: scsi_status] SCSI status
;
; CF    set if error
; AH    result code of host (if error)
; ds: request_sense_data  request sense data (when the target returns 'check condition')
;

scsi_cmd_55bios:
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	MOVSEG	es, cs
	mov	dx, scsi55_cmdpkt
	mov	di, dx
	mov	cx, [si]
	push	cx
	mov	al, [scsi_lun]
	stosb
	mov	ah, byte [si + 2]
	mov	al, SCSI_FROM_DEVICE_55
	cmp	ah, SCSI_FROM_DEVICE
	je .l2
	mov	al, SCSI_TO_DEVICE_55
	cmp	ah, SCSI_TO_DEVICE
	je	.l2
	mov	al, SCSI_NO_TRANSFER_55
.l2:
	mov	ah, [si + 3]	; cdb length
	stosw
	inc	di
	mov	cl, ah
	mov	ch, 0
	add	si, 4
	rep	movsb
	pop	cx
	pop	es
	pop	di
	mov	bx, di
	mov	al, [scsi_id]
	or	al, 0c0h
	mov	ah, 09h
%ifdef DEBUG
	call	invoke_int1b
%else
	int	1bh
%endif
	mov	[cmd_result], ah
	jz	.err			; ZF=1: scsi bios in busy
	jc	.err
	mov	byte [cmd_result], 0
	mov	al, [scsi55_cmdpkt]	; get SCSI status
	mov	[scsi_status], al
	and	al, 3eh
	jz	.exit_noerr
	cmp	al, 2		; check codition ?
	stc
	jne	.exit
	cmp	byte [scsi55_cmdpkt + 4], 03h	; REQUEST SENSE?
	stc
	je	.exit
	mov	al, [scsi_lun]
	mov	[scsi55_cmdpkt_reqsense], al
	push	es
	mov	dx, scsi55_cmdpkt_reqsense
	mov	bx, request_sense_data
	MOVSEG	es, cs
	mov	cx, REQUEST_SENSE_MAX
	mov	al, [scsi_id]
	or	al, 0c0h
	mov	ah, 09h
%ifdef DEBUG
	call	invoke_int1b
%else
	int	1bh
%endif
	pop	es
.exit_noerr:
	clc
.exit:
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret
.err:
	lahf
	mov	[cmd_result2], ah
	stc
	jmp	short .exit


%ifdef DEBUG

invoke_int1b:
	push	bp
	mov	byte [cs: .msg_c], '-'
	mov	byte [cs: .msg_z], '-'
	mov	bp, ax
	int	1bh
	jc	.err
	jz	.err
.exit:
	pop	bp
	ret
.err:
	push	ax
	push	dx
	mov	dx, ax
	pushf
	jnc	.err2
	mov	byte [cs: .msg_c], 'C'
.err2:
	jnz	.err3
	mov	byte [cs: .msg_z], 'Z'
.err3:
	mov	al, [cs: .msg_c]
	call	PutC
	mov	al, [cs: .msg_z]
	call	PutC
	mov	al, ' '
	call	PutC
	mov	ax, dx
	call	PutH16
	mov	al, ' '
	call	PutC
	mov	al, [cs: cmd_result]
	call	PutH8
	popf
	pop	dx
	pop	ax
	jmp	short .exit

.msg_c:	db	'-'
.msg_z:	db	'-'

%endif

	SCSI_CODE_END


	INIT_DATA
msg_scsi_platform:
	db	'PC-98 SCSI BIOS', 0
	INIT_DATA_END