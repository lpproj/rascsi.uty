
%ifndef PRIVATE_STACK_LENGTH_ASPI
%define PRIVATE_STACK_LENGTH_ASPI	320
%endif
PRIVATE_STACK_LENGTH	equ	PRIVATE_STACK_LENGTH_ASPI

%if 0
;-----------------------------------------------------------------------------
scsi_id		db 6
scsi_lun	db 0

scsi_status	db 0
cmd_result	db 0
cmd_result2	db 0

aspi_entry	dd 0

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

SCSI_NO_TRANSFER_AS	equ 18h
SCSI_FROM_DEVICE_AS	equ 08h
SCSI_TO_DEVICE_AS	equ 10h


aspi_entry	dd 0

aspi_cmdpkt:
ac_cmd		db 0		; +00
ac_aspistatus	db 0		; +01
ac_hostid	db 0		; +02
ac_flags	db 0		; +03
		dd 0		; +04
ac_id		db 0		; +08
ac_lun		db 0		; +09
ac_datalen	dd 0		; +10
ac_senselen	db 0		; +14
ac_databuf	dd 0		; +15
ac_nextpkt	dd 0		; +19
ac_cdblen	db 0		; +23
ac_hoststatus	db 0		; +24
ac_devstatus	db 0		; +25
ac_postfunc	dd 0		; +26
	times 34 db 0		; +30
ac_cdb:				; +64
	times (12 + REQUEST_SENSE_MAX) db 0
aspi_cmdpkt_bottom:

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


scsi_cmd_aspi:
	push bx
	push cx
	push dx
	push si
	push di
	push es
	mov	bx, di
	mov	dx, es
	MOVSEG	es, ds
	mov	di, aspi_cmdpkt
	mov	cx, (aspi_cmdpkt_bottom - aspi_cmdpkt) / 2
	xor	ax, ax
	rep	stosw
	mov	[ac_databuf], bx
	mov	[ac_databuf + 2], dx
	mov	byte [ac_cmd], 2
	mov	al, [scsi_host_id]
	mov	[ac_hostid], al
	mov	dx, [si + 2]		; dl=direction dh=cdblen
	mov	al, SCSI_TO_DEVICE_AS
	cmp	dl, SCSI_TO_DEVICE
	je	.set_dir
	mov	al, SCSI_FROM_DEVICE_AS
	cmp	dl, SCSI_FROM_DEVICE
	je	.set_dir
	mov	al, SCSI_NO_TRANSFER
.set_dir:
	mov	[ac_flags], al
	mov	[ac_cdblen], dh
	mov	ax, [scsi_id]		; lun:id
	mov	[ac_id], ax
	mov	ax, [si]		; data length
	mov	[ac_datalen], ax
	mov	byte [ac_senselen], REQUEST_SENSE_MAX
	mov	di, ac_cdb
	add	si, 4
	mov	cl, dh			; ac_cdblen
	rep	movsb
	; invoke aspi
	push	ds
	mov	ax, aspi_cmdpkt
	push	ax
	call	far [aspi_entry]
	add	sp, 4
	mov	al, [ac_devstatus]
	mov	[scsi_status], al
.poll_lp:
	mov	ah, [ac_aspistatus]
	cmp	ah, 1
	je	.scsi_noerr
	cmp	ah, 2
	jae	.scsi_err
	sti
	;hlt
	jmp	short .poll_lp
.scsi_err:
	cmp	ah, 80h
	jae	.exit_err
	xor	bh, bh
	mov	bl, [ac_cdblen]
	lea	si, [bx + aspi_cmdpkt]
	mov	di, request_sense_data
	mov	cx, REQUEST_SENSE_MAX
	rep	movsb
.exit_err:
	stc
	jmp	short .exit
.scsi_noerr:
	xor	ax, ax		; AX=0, CF=0
.exit:
	mov	[cmd_result], ah
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret



	SCSI_CODE_END

	INIT_CODE

InitASPI:
	push	ax
	push	bx
	push	cx
	push	dx
	mov	dx, aspi_devname
	mov	ax, 3d00h	; open
	int	21h
	jc	.err
	mov	bx, ax
	mov	dx, aspi_entry
	mov	cx, 4
	mov	ax, 4402h	; ioctl read
	int	21h
	pushf
	push	ax
	mov	ah, 3eh
	int	21h		; close
	pop	ax
	popf
	jc	.err
	cmp	ax, 4
	jne	.err
.noerr:
	clc
	jmp	short .exit
.err:
	stc
.exit:
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

	INIT_CODE_END

	INIT_DATA
aspi_devname:
	db	'SCSIMGR$', 0
msg_scsi_platform:
	db	'ASPI', 0
	INIT_DATA_END
