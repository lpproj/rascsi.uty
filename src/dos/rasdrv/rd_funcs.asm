
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
	%include 'rd_scsi.inc'
%define DECLARE_RD_FUNCS
	%include 'rd_funcs.inc'
	%include 'rd_fsdbg.inc'
	%include 'rasdrv.inc'

	extern org_2f
	extern drive_number


	TSR_CODE

; helpers


toupr_nameext:
	test	ah, 1
	jnz	ToUpr
	ret
toupr_path:
	test	ah, 2
	jnz	ToUpr
	ret
ToUpr:
	cmp	al, 'a'
	jb	.ret
	cmp	al, 'z'
	ja	.ret
	sub	al, 'a' - 'A'
.ret:
	ret



; invalid characters for FAT on the DOS
;  00~1Fh all
;  20~2Fh space " * + , /	22h, 2Ah, 2Bh, 2Fh
;  30~3Fh ; : < = > ?		3A~3Fh
;  40h~   \ | 			5Ch, 7Ch
; return:
; ZF=0 valid
; ZF=1 invalid
CheckFilenameCharP:
	cmp	al, '.'
	je	CheckFilenameChar.match
CheckFilenameChar:
	cmp	al, 21h
	jb	.match
	push	cx
	mov	cx, .table_bottom - .table
	push	di
	push	es
	mov	di, .table
	MOVSEG	es, cs
	repne	scasb
	pop	es
	pop	di
	pop	cx
	je	.match
	clc
	ret
.match:
	stc
	ret
.table:
	db	'"', '*', '+', ',', '/'
	db	';', ':', '<', '=', '>'
;	db	'?'			; used for wildcard in FCB
	db	'\', '|'		; note: available on DBCS trailing byte
;	db	7fh
;	sb	0ffh
.table_bottom:


; ds:si = filename str
;
; 1~8chars without period
; 1~8chars + period + 0~3chars
; .  (current dir)
; .. (parent dir)
UpperNameDOSish:
	; check '.' and '..'
	cmp	word [si], '.'
	je	.pre_exit
	cmp	word [si], '..'
	jne	.l2
	cmp	byte [si + 2], 0
	je	.pre_exit
	stc
.pre_exit:
	ret
.l2:
	push	ax
	push	si
.lp:
	lodsb
	cmp	al, 0
	je	.brk
	call	ToUpr
	mov	[si - 1], al
	jmp	short .lp
.brk:
	pop	si
	pop	ax
	ret



; sub
; ds:si src filename
; es:di dest
; cx    max count (name=8, ext=3)
; return
; ds:si  next of src
; al     byte [si]
; es:di  (modified)
; ah     bit6=1: '?' exist
; CF=1   found invalid char in src
copy_fcbish_nameext:
.lp:
	mov	al, [si]
	test	al, al		; CF=0
	jz	.exit
	jcxz	.exit
	call	CheckFilenameCharP
	jc	.l2
	call	IsDBCSLead
	jc	.dbcs
.cont:
	call	ToUpr
	stosb
	inc	si
	dec	cx
	jmp	short .lp
.dbcs:
	stosb
	inc	si
	dec	cx
	jz	.err		; err when broken dbcs
	mov	al, [si]
	test	al, al		; assert buffer overrun
	jnz	.cont
	jmp	short .err
.l2:
	cmp	al, '?'
	jne	.err
	or	ah, 40h
	jmp	short .cont
.err:
	stc
.exit:
	ret


; ds:si filname w/o dirsep
; es:di FCB-style filename field (all SBCS chars upcase)
; ah    bit0=1  accept '.' and '..'
; result:
;   cf=0 success
;      1 failure (invalid filename for DOS)

FilenameToFCB:
	push	ax
	push	cx
	push	si
	push	di
	; init dest field
	mov	cx, 8 + 3
	mov	al, ' '
	push	di
	rep	stosb
	pop	di
	; check '.' and '..'
	test	ah, 1
	jz	.copyname
	mov	al, '.'
	cmp	al, byte [si]	; '.' at first?
	jne	.copyname
	mov	cx, word [si + 1]
	test	cl, cl		; "." ?
	jz	.copy_p
	cmp	cx, '.'		; ".." ?
	jne	.err
.copy_pp:
	stosb
.copy_p:
	stosb
	jmp	short .noerr
.copyname:
	push	di
	mov	cx, 8
	call	copy_fcbish_nameext
	pop	di
	test	al, al
	jz	.noerr
	cmp	al, '.'		; not care h68k longname for now
	jne	.err
	inc	si
;.copyext
	add	di, 8
	mov	cx, 3
	call	copy_fcbish_nameext
	test	al, al
	jz	.noerr
.err:
	stc
	jmp	short .exit
.noerr:
	clc
.exit:
	pop	di
	pop	si
	pop	cx
	pop	ax
	ret


;
; store searched filename to sda dirent
; ds:bx rp_nfiles
;
NFilesToDirent:
	push	ax
	push	si
	push	di
	push	es
	lea	si, [bx + rp_nfiles.files + h68files.full]
	les	di, [sda_dirent]
	mov	ah, 1
	call	FilenameToFCB
	jc	.exit
	lea	si, [bx + rp_nfiles.files + h68files.attr]
	lodsw
	mov	[es: di + 0bh], al
	lodsw
	xchg	ah, al
	mov	[es: di + 16h], ax	; time
	lodsw
	xchg	ah, al
	mov	[es: di + 18h], ax	; date
	lodsw				; size_be high
	xchg	ah, al
	mov	[es: di + 1eh], ax	; write size high
	lodsw				; size_be low
	xchg	ah, al
	mov	[es: di + 1ch], ax	; write size low
	mov	word [es: di + 1ah], -1	; start cluster
	clc
.exit:
	pop	es
	pop	di
	pop	si
	pop	ax
	ret


;
; es:di  full qualified pathname
;          x:\foo\bar.baz	drv=x, wildcard=0, path=\foo\, nameext=BAR.BAZ
;          x:\foo\bar.baz\	drv=x, wildcard=FF, path=\foo\, nameext=BAR.BAZ
;          x:\foo\bar.b?z	drv=x, wildcard=1, path=\foo\, nameext=BAR.B?Z
; ds:si  namests_t
; ah     flag
;        bit0=1     upcase name+ext
;        bit1=1   not supported: upcase path
;        bit2=1     allow wildcard '?' in name+ext
;        bit3=1     allow '.' and '..' as name+ext
;        bit7=1     do not initialize namests_t (path, name, ext)
;
PNtoNamests:
	push	ax
	push	bx
	push	cx
	push	dx
	push	bp
	push	si
	push	di
	push	ds
	push	es
	; ds:si=path es:di=namests
	; dx = top of src path (backup)
	; bx = top of namests (backup)
	; bp = (last dirsep) + 1 in src path (null when not found)
	xchg	si, di
	XCHGSEG	ds, es
	mov	bx, di
;	mov	dx, si
	xor	bp, bp
	; init namests
	test	ah, 80h
	jnz	.chkdrive
	mov	al, 0
	stosb			; wildcard = 0
	inc	di		; do not init drive
	mov	cx, h68namests_size - 2
	rep stosb
	lea	di, [bx + h68namests.name]
	mov	cx, 8 + 3	; name + ext
	mov	al, ' '
	rep stosb
.chkdrive:
	mov	al, [si]
	test	al, al		; is pathname ""?
	jz	.noerr
	cmp	byte [si + 1], ':'
	jne	.scanpath
	add	si, byte 2
	or	al, 20h
	;sub	al, 'a'
	sub	al, [cs: rd_drive]
	jc	.scanpath
	mov	[es: bx + h68namests.drive], al
.scanpath:
	mov	dx, si
.scanpath_lp:
	lodsb
	test	al, al
	jz	.scanpath_break
	cmp	al, '\'
	je	.scanpath_dirsep
	cmp	al, '/'
	jne	.scanpath_l2
.scanpath_dirsep:
	cmp	byte [si], 0	; ignore dirsep as the last char
	jne	.scanpath_dirsep2
	mov	byte [es: bx + h68namests.wildcard], 0ffh	; mark as 'path only'
	jmp	short .scanpath_l2
.scanpath_dirsep2:
	mov	bp, si
.scanpath_l2:
	call	IsDBCSLead
	jnc	.scanpath_lp
	lodsb		; dbcs trailing byte
	test	al, al	;  -> assert buffer overrun
	jnz	.scanpath_lp
.scanpath_break:
	dec	si
.copypath:
	lea	di, [bx + h68namests.path]
	mov	al, 0
	test	bp, bp
	jnz	.copypath_l2
	stosb
	mov	bp, dx
	jmp	short .copyname
.copypath_l2:
	mov	cx, bp
	sub	cx, dx
	cmp	cx, 64
	ja	.err
	mov	si, dx
	rep	movsb
	stosb
.copyname:
	mov	si, bp
	lea	di, [bx + h68namests.name]
	mov	cx, 8
	call	copy_namests_nameext
	jc	.copyext_pre
	; ext not exist? (todo: allow addendum filename (add[10]))
%if 0
	mov	dx, si		; now dx = top of add
	test	al, al
	jz	.noerr
	inc	si
	call	.skip_to_peried_or_end
	test	al, al
	jz	.copyadd
%else
	test	al, al
	jz	.noerr
%endif
.copyext_pre:
	cmp	al, '.'
	jne	.copyext_after
	inc	si
.copyext:
	lea	di, [bx + h68namests.ext]
	mov	cl, 3
	call	copy_namests_nameext
	jc	.copyext_after
	test	al, al
	jz	.noerr
.copyext_after:
	call	.chk_isdironly
	jmp	short .exit
.err:
	stc
	jmp	short .exit
.noerr:
	clc
.exit:
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
.chk_isdironly:
	cmp	byte [es: bx + h68namests.wildcard], 0ffh
	jne	.chk_isdironly_err
	cmp	al, '\'
	je	.chk_isdironly_exit
	cmp	al, '/'
	je	.chk_isdironly_exit
.chk_isdironly_err:
	stc
.chk_isdironly_exit:
	ret


copy_namests_nameext:
	and	ah, 0bfh
	call	copy_fcbish_nameext
	pushf
	test	ah, 40h
	jz	.exit
	or	byte [es: bx + h68namests.wildcard], 1
.exit:
	popf
	ret


;--------------------------------------------------------------

; copy openmode,attribute,date,time,length and current file pointer 
; ds:bx h68fcb
; es:di sft

RD_h68fcb_to_sft_with_init:
	push	ax
	mov	ax, 0
	mov	[es: di + 0bh], ax
	mov	[es: di + 19h], ax
	mov	[es: di + 1bh], ax
	mov	[es: di + 1dh], ax
	mov	[es: di + 1fh], al
	jmp	short RD_h68fcb_to_sft.l1

RD_h68fcb_to_sft_ftime:
	push	ax
	jmp	short RD_h68fcb_to_sft.ftime

RD_h68fcb_to_sft:
	push	ax
.l1:
	mov	ax, [bx + h68fcb.mode_be]	; file open mode
	mov	[es: di + 2], ah	; higher byte not modified: for FCB-open (bit15)
	mov	al, [bx + h68fcb.attr]		; file attribute
	and	al, 3fh
	mov	[es: di + 4], al
	mov	ax, [bx + h68fcb.size_be + 2]
	xchg	ah, al
	mov	[es: di + 11h], ax
	mov	ax, [bx + h68fcb.size_be]
	xchg	ah, al
	mov	[es: di + 13h], ax
	mov	ax, [bx + h68fcb.fileptr_be + 2]
	xchg	ah, al
	mov	[es: di + 15h], ax
	mov	ax, [bx + h68fcb.fileptr_be]
	xchg	ah, al
	mov	[es: di + 17h], ax
.ftime:
	mov	ax, [bx + h68fcb.time_be]
	xchg	ah, al
	mov	[es: di + 0dh], ax
	mov	ax, [bx + h68fcb.date_be]
	xchg	ah, al
	mov	[es: di + 0fh], ax
	pop	ax
	ret

RD_sft_to_h68fcb:
	push	ax
	mov	ax, [es: di + 17h]
	xchg	ah, al
	mov	[bx + h68fcb.fileptr_be], ax
	mov	ax, [es: di + 15h]
	xchg	ah, al
	mov	[bx + h68fcb.fileptr_be + 2], ax
	mov	ax, [es: di + 02h]
	and	ax, 7fh
	xchg	ah, al
	mov	[bx + h68fcb.mode_be], ax
	mov	al, [es: di + 4]
	and	al, 3fh
	mov	[bx + h68fcb.attr], ax
	mov	ax, [es: di + 0dh]
	xchg	ah, al
	mov	[bx + h68fcb.time_be], ax
	mov	ax, [es: di + 0fh]
	xchg	ah, al
	mov	[bx + h68fcb.date_be], ax
	mov	ax, [es: di + 13h]
	xchg	ah, al
	mov	[bx + h68fcb.size_be], ax
	mov	ax, [es: di + 11h]
	xchg	ah, al
	mov	[bx + h68fcb.size_be + 2], ax
	pop	ax
	ret


;--------------------------------------------------------------

;
; subfunc for FindFirst, GetAttr
; ds:si buffer for packet (rd_fs_packet)
; es:di pathname
; al key (1~SEARCHKEY_MAX and SEARCHKEY_MAX+1 (reserved for getattr)
; dl search attribute
; dh flags
;    bit7=1  do not setup findfirst data blk
; result
; ds:bx  ptr to nfiles
; ds:di  result ptr to nfiles.files
; CF=0
;   ax = attribute of the file (fs error when ax > 00ffh)
; CF=1 error
;   ax = dos error status
RD_findfirst:
	push	cx
	push	dx
	mov	cx, rp_files_size
	call	bzero_dssi
	push	ax
	mov	[si + rp_files.key_be + 3], al
	mov	ah, rp_nfiles_size
	mul	ah
	add	ax, rd_search_table - rp_nfiles_size
	mov	bx, ax
	pop	ax
	mov	[bx + rp_nfiles.key_be + 3], al
	mov	[si + rp_files.files], dl		; search attribute
	push	si
	add	si, rp_files.name
	mov	ah, 0
	call	PNtoNamests
	pop	si
%if 0
	cmp	dl, 8		; volume label only?
	jne	.l2
	mov	word [si + rp_files.name + h68namests.path], '/'
.l2:
%endif
	test	dh, 80h
	jnz	.cmd
	; prepare findfirst data block
	les	di, [sda_searchblock]
	;RD_GetSDADTA	es, di
	push	si
	mov	al, [drive_number]
	or	al, 0c0h
	stosb				; +00    drvletter
	mov	cx, 11
	add	si, h68namests.name
	rep	movsb			; +01~0B template (FCB style)
	mov	al, dl
	stosb				; +0C    search attr
	xor	ax, ax
	stosw				; +0D~0E entry count
	add	di, 13h - 0fh
	mov	ax, bx
	stosw				; +13    ptr to internal search tbl (rasdrv private)
	pop	si
.cmd:
	push	es
	mov	cx, rp_files_size
	lea	di, [bx + rp_nfiles.files]
	mov	dx, h68files_size
	MOVSEG	es, ds
	mov	al, RD_CMD_FILES
	call	RD_FS_CalCmd
	pop	es
	jc	.deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.fserr
	mov	al, [di + h68files.attr]	; es=ds
	and	ax, 3fh				; ah=0 (and CF=0)
	jmp	short .exit
.deverr:
	mov	ax, 21			; device not ready
	stc
	jmp	short .exit
.fserr:
	call	RD_FS_GetDOSError
.exit:
	pop	dx
	pop	cx
	ret


; ds:si buffer for packet (rd_fs_packet)
; es:di dirname
; result
; 
; CF=0
;   ax = file attirbute
;   ds:bx = rp_nfiles
; CF=1 error
;   ax = dos err status
RD_stat:
	push	cx
	push	dx
	push	di
	mov	al, SEARCHKEY_MAX + 1
	mov	dx, 803fh
	call	RD_findfirst
	;jc	.err
;.exit:
	pop	di
	pop	dx
	pop	cx
	ret

; ds:si buffer for packet (rd_fs_packet)
; es:di dirname
; result
; 
; CF=0
;   ax = file attirbute
; CF=1 error
;   ax = dos err status
RD_stat_attronly:
	push	cx
	mov	cx, rp_attribute_size
	call	bzero_dssi
	push	si
	add	si, rp_attribute.name
	mov	ah, 0
	call	PNtoNamests
	pop	si
	; special case for "/" (root dir)
	cmp	byte [si + rp_attribute.name + h68namests.path], 0
	jne	.do_cmd
	cmp	byte [si + rp_attribute.name + h68namests.name], ' '
	jne	.do_cmd
	mov	ax, 30h
	jmp	short .exit
.do_cmd:
	mov	byte [si + rp_attribute.attr_be + 3], 0ffh	; get attribute
	mov	al, RD_CMD_ATTRIBUTE
	call	RD_FS_SendCmd
	jc	.deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.fserr
	and	ax, 03fh		; ah=0 (and CF=0)
.exit:
	pop	cx
	ret
.deverr:
	mov	ax, 21			; device not ready
	stc
	jmp	short .exit
.fserr:
	call	RD_FS_GetDOSError
	jmp	short .exit


; ds:si buffer for packet (rd_fs_packet)
; es:di dirname
; result
; CF=0
;   ax = rasdrv result (0=dir exist !0=not dir)
; CF=1 error
;   ax = doserr
RD_checkdir:
%if 1
	call	RD_stat_attronly
%else
	push	bx
	push	di
	call	RD_stat
	pop	di
	pop	bx
%endif
	jc	.exit
	test	ah, ah
	jnz	.exit
	and	al, 18h
	xor	al, 10h
.exit:
	ret


; search free key and reserve
; result
; ax  key index (1..FILEKEY_MAX)
;     0  all keys not available
RD_allockey:
	push	si
	mov	si, rd_filekey_in_use_1
.lp:
	lodsb
	test	al, al
	jz	.found
	cmp	si, rd_filekey_in_use_bottom
	jb	.lp
	mov	si, rd_filekey_in_use_1
	jmp	short .brk
.found:
	mov	byte [si - 1], 1
	mov	ax, si
.brk:
	sub	ax, rd_filekey_in_use_1
	pop	si
	ret


; free reserved key
; ax  key index (1..FILEKEY_MAX)
; result
;     CF=0  success
;        1  err
RD_freekey:
	xchg	ax, bx
	cmp	bx, FILEKEY_MAX
	ja	.err
	test	bx, bx		; CF=0
	jz	.err
	mov	byte [bx + rd_filekey_in_use], 0
.exit:
	xchg	ax, bx
	ret
.err:
	stc
	jmp	short .exit

RD_freekey_dssi:
	push	ax
	mov	ax, [si + 2]
	xchg	ah, al
	call	RD_freekey
	pop	ax
	ret

RD_freekey_dssi4:
	push	ax
	mov	ax, [si + 4 + 2]
	xchg	ah, al
	call	RD_freekey
	pop	ax
	ret



;--------------------------------------------------------------
; ds:si buffer for packet (rd_fs_packet)
; es:di sft
;       sft + 0dh mtime
;       sft + 0fh mdate
;
; result:
; CF=0 noerr
; ax 0
; CF=1 err
; ax doserr

RD_SyncFileTime_sft:
	push	bx
	push	cx
	push	dx
	mov	cx, rp_ftime_size
	call	bzero_dssi
	mov	bx, [es: di + SFT_FILEKEY_OFFSET]
	xchg	bh, bl
	mov	[si + rp_ftime.key_be + 2], bx
	lea	bx, [si + rp_ftime.fcb]
	call	RD_sft_to_h68fcb
%if 1
	mov	ax, [bx + h68fcb.time_be]
	mov	[si + rp_ftime.time_be], ax
	mov	ax, [bx + h68fcb.date_be]
	mov	[si + rp_ftime.date_be], ax
%else
	mov	ax, [es: di + 0dh]
	xchg	ah, al
	mov	ax, [bx + h68fcb.time_be]
	mov	ax, [es: di + 0fh]
	xchg	ah, al
	mov	ax, [bx + h68fcb.date_be]
%endif

	push	di
	push	es
	mov	di, bx
	MOVSEG	es, ds
	mov	dx, h68fcb_size
	mov	al, RD_CMD_TIMESTAMP
	call	RD_FS_CalCmd
	pop	es
	pop	di
	jc	.deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.fserr
	clc
.exit:
	pop	dx
	pop	cx
	pop	bx
	ret
.deverr:
	mov	ax, 21			; device not ready
	stc
	jmp	short .exit
.fserr:
	call	RD_FS_GetDOSError
	jmp	short .exit


RD_Commit_sft:
	mov	ax, [es: di + 05h]	; sft +5 device info word
	test	byte [es: di + 2], 7	; check file open mode
	jz	.for_rdonly
;.for_wr:
	test	ah, 40h			; sync mtime if WRONLY/RDWR and bit14=1
	jnz	.sync_actual_mtime
	test	al, 40h			; update mtime if the file is wasted (bit6=0)
	jnz	.sync_actual_mtime
	push	dx
	mov	ax, 120dh | 8000h	; get date(AX) and time(DX)
	call	Invoke_Int2F12xx
	mov	[es: di + 0dh], dx	; sft +Dh mtime
	mov	[es: di + 0fh], ax	; sft +Fh mdate
	pop	dx
	jmp	short .sync_actual_mtime
.for_rdonly:
	test	ah, 40h			; sync mtime if RDONLY and bit14=1
	jz	.exit			; otherwise not touch mtime
.sync_actual_mtime:
	call	RD_SyncFileTime_sft
	;jc	.exit
	;or	word [es: di + 05h], 4000h
.exit:
	ret



;--------------------------------------------------------------

; ifs functions
; bx, dx: broken
; ds = cs
; ss, sp = private stack (not pointed within DOS DS)
; [ss:bp] (top of stack) dword : pointer to rd_regframe
; flags: cld sti
; other regs: preserved original value at int2F entry
;
; it may be modified all registers except of ss:sp
; (Note: RD_Fallback_error needs to keep bp)
;

RD_Chdir:		; 2F1105
	mov	si, rd_fs_packet
	les	di, [sda_fn1]
	call	RD_checkdir
	jc	.err
	cmp	ax, 0			; dir exist?
	jne	.err_nopath
	; update CDS
	push	ds
	RD_GetSDACDS	es, di
	mov	ax, [es: di + 4fh]
	lds	si, [sda_fn1]
	add	si, ax
	add	di, ax
	call	strCpy
	pop	ds
	jmp	rd_success_noreg
.err:
	jmp	RD_Fallback_error
.err_nopath:
	mov	ax, 3			; path not found
	jmp	RD_Fallback_error_AX


RD_FindFirst:		; 2F111B
	; check exact dir
	mov	si, rd_fs_packet
	les	di, [sda_fn1]
	; make up search handle(key)
	mov	al, [rd_searchkey_number]
	inc	al
	cmp	al, SEARCHKEY_MAX
	jbe	.searchkey_upd
	mov	al, 1
.searchkey_upd:
	mov	[rd_searchkey_number], al
	les	di, [sda_sattr]
	mov	dl, [es: di]
	cmp	dl, 8			; special case: volume file only?
	je	.ff_l2
	or	dl, 21h			; always match: read-only and archive
.ff_l2:
	xor	dh, dh
	les	di, [sda_fn1]
	call	RD_findfirst
	jnc	.to_dirent
	jmp	RD_Fallback_error_AX
.to_dirent:
	lea	si, [di + h68files.full]
	call	UpperNameDOSish
	call	NFilesToDirent
	jc	RD_FindNext
	jmp	rd_success_noreg

RD_Find_deverr:
	mov	ax, 21		; drive not ready (or 5=access denied?)
	jmp	RD_Fallback_error_AX


RD_FindNext:		; 2F111C
	les	di, [sda_searchblock]
	;RD_GetSDADTA	es, di
	mov	si, [es: di + 13h]	; get pointer of internal tbl
	cmp	si, rd_search_table
	jb	.invalid_dta
	cmp	si, (rd_search_table_bottom - rp_nfiles_size)
	ja	.invalid_dta
	mov	al, [si + rp_nfiles.key_be + 3]
	cmp	al, 0
	je	.invalid_dta
	cmp	al, SEARCHKEY_MAX
	jbe	.l2
.invalid_dta:
	mov	ax, 13		; data invalid ?
	jmp	RD_Fallback_error_AX
.l2:
	lea	di, [si + rp_nfiles.files]
	MOVSEG	es, ds
	mov	cx, rp_nfiles_size
	mov	dx, h68files_size
	mov	al, RD_CMD_NFILES
	call	RD_FS_CalCmd
	jc	RD_Find_deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err
	push	si
	lea	si, [di + h68files.full]
	call	UpperNameDOSish
	pop	si
	mov	bx, si
	call	NFilesToDirent
	jc	.l2
	les	di, [sda_searchblock]
	;RD_GetSDADTA	es, di
	inc	word [es: di + 0dh]
	jmp	rd_success_noreg
.err:
	jmp	RD_Fallback_error


RD_Diskinfo:		; 2F110C
	mov	si, rd_fs_packet
	mov	cx, 4 + h68dpb_size
	call	bzero_dssi
	lea	di, [si + 4]
	MOVSEG	es, ds
	mov	cx, 4
	mov	dx, h68dpb_size
	mov	al, RD_CMD_GETDPB
	call	RD_FS_CalCmd
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err
	les	bx, [bp]		; es:bx = register frame
	;mov	ah, [di + h68dpb.media]
	mov	ah, 0f8h		; fixed disk (note: GetDPB will return F3h)
	mov	al, [di + h68dpb.cluster_size]
	inc	al
	mov	[es: bx + r_ax], ax	; AH=mediaID, AL=sectors per cluster
	mov	ax, [di + h68dpb.sector_size_be]
	xchg	ah, al
	mov	[es: bx + r_cx], ax	; CX=bytes per sector
	mov	ax, [di + h68dpb.cluster_max_be]
	xchg	ah, al
	cmp	ax, 0fff6h
	jbe	.l2
	mov	ax, 0fff6h
.l2:
	mov	[es: bx + r_bx], ax	; BX=total clusters
	mov	[es: bx + r_dx], ax	; DX=free clusters
	jmp	rd_success_frame_esbx
.err_cmd:
	mov	ax, 21		; drive not ready
	jmp	RD_Fallback_error_AX
.err:
	jmp	RD_Fallback_error


RD_GetAttr:		; 2F110F
	mov	si, rd_fs_packet
	les	di, [sda_fn1]
	call	RD_stat
	jnc	.l2
	jmp	RD_Fallback_error_AX
.l2:
	lea	di, [bx + rp_nfiles.files]
	les	bx, [bp]
	mov	[es: bx + r_ax], ax	; AX=attr
	mov	ax, [di + h68files.time_be]
	xchg	ah, al
	mov	[es: bx + r_cx], ax	; CX=time
	mov	ax, [di + h68files.date_be]
	xchg	ah, al
	mov	[es: bx + r_dx], ax	; DX=date
	mov	ax, [di + h68files.size_be]
	xchg	ah, al
	mov	[es: bx + r_bx], ax	; BX=filesize (upper)
	mov	ax, [di + h68files.size_be + 2]
	xchg	ah, al
	mov	[es: bx + r_di], ax	; BX=filesize (lower)
	jmp	rd_success_frame_esbx



RD_Creat:		; 2F1117
	push	ds
	lds	si, [bp]
	mov	dx, [si + rd_extra]
	pop	ds
	mov	dh, 12h		; creat always
	and	dl, 7fh
	mov	cx, [es: di + 2]
	mov	cl, 2
	call	RD_Open2_sub
	jc	.err_ax
;.noerr:
	jmp	rd_success_noreg
.err_ax:
	jmp	RD_Fallback_error_AX


RD_Open:		; 2F1116
	push	ds
	lds	si, [bp]
	mov	cx, [si + rd_extra]
	pop	ds
	mov	dx, 0100h
	call	RD_Open2_sub
	jc	.err_ax
;.noerr:
	jmp	rd_success_noreg
.err_ax:
	cmp	ax, 1			; invalid func (open2 failure)?
	jne	.err_ax_exit
	mov	ax, 2			; file not found
.err_ax_exit:
	jmp	RD_Fallback_error_AX


; -------------------------------------
;
; (ds:si)  rd_fs_packet
; DH  action code
; DL  file creation attributes
; CX  file open mode
; sda_fn1  pathname
; es:di    sft
;
; result:
; CF=0 success
;      sft filled
;      CX  status (1:opened 2:created 3:replaced)
; CF=1 error
;      AX doserror


RD_Open2_sub:
	mov	si, rd_fs_packet
	; check file existance (and fill h68namests)
	push	dx
	push	di
	push	es
	les	di, [sda_fn1]
	mov	dx, 803fh
	mov	al, SEARCHKEY_MAX + 1
	call	RD_findfirst
	pop	es
	pop	di
	pop	dx
	jnc	.chk_exist
;chk_not_exist:
	cmp	ax, 21		; device not ready? (guess SCSI err)
	je	.err_stc
	cmp	ax, 80		; file exists (just to be safe)
	je	.chk_exist
	mov	ah, dh
	and	ah, 0f0h
	cmp	ah, 10h
	mov	dh, 2		; (file created)
	je	.creat
	jmp	short .err_invalid
.chk_exist:
	mov	ah, dh
	and	ah, 0fh
	cmp	ah, 2
	mov	dh, 3		; (file replaced)
	je	.creat
	cmp	ah, 1
	je	.open
.err_invalid:
	mov	ax, 1
	stc
	ret
.err_fileexists:
	mov	ax, 80
	stc
	ret
.err_filenotfound:
	mov	ax, 2
	stc
	ret
.err_toomanyopen:
	mov	ax, 4
.err_stc:
	stc
	ret
;
.creat:
	call	RD_allockey
	jz	.err_toomanyopen
	mov	[si + rp_creat.key_be + 3], al
	push	bp
	xor	ah, ah
	mov	al, dh
	mov	bp, ax		; bp = 2 or 3
	push	cx
	push	si
	add	si, rp_creat.fcb
	mov	cx, h68fcb_size + 4 + 4
	call	bzero_dssi
	pop	si
	pop	cx
	mov	[si + rp_creat.attr_be + 3], dl
	mov	byte [si + rp_creat.force_be + 3], 1			; newfile
	mov	byte [si + rp_creat.fcb + h68fcb.mode_be + 1], 2	; rw mode
	; set current time to ftime on file creation
	mov	ax, 120dh | 8000h	; get current date and time
	call	Invoke_Int2F12xx
%if 0
	xchg	ah, al
	xchg	dh, dl
	mov	[si + rp_creat.fcb + h68fcb.time_be], dx
	mov	[si + rp_creat.fcb + h68fcb.date_be], ax
%else
	mov	[es: di + 0dh], dx
	mov	[es: di + 0fh], ax
	
%endif
	push	di
	push	es
	lea	di, [si + rp_creat.fcb]
	MOVSEG	es, ds
	mov	bx, di
	mov	cx, rp_creat_size
	mov	dx, h68fcb_size
	mov	al, RD_CMD_CREATE
	call	RD_FS_CalCmd
	pop	es
	pop	di
	jc	.deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.fserr
	; filesize = 0, fileptr = 0
	xor	ax, ax
	mov	[es: di + 11h], ax		; sft + 11h: filesize
	mov	[es: di + 13h], ax
	mov	[es: di + 15h], ax		; sft + 15h: fileptr
	mov	[es: di + 17h], ax
	;mov	dx, 8040h
	jmp	.setup_sft
; open existing file
.open:
	call	RD_allockey
	jz	.err_toomanyopen
	mov	[si + rp_open.key_be + 3], al
	push	bp
	mov	bp, 1
	mov	[si + rp_open.fcb + h68fcb.mode_be], ch
	mov	[si + rp_open.fcb + h68fcb.mode_be + 1], cl
	; fetch file information (attr, mtime, size) from findfirst cache
	xor	ax, ax
	mov	[si + rp_open.fcb + h68fcb.fileptr_be], ax
	mov	[si + rp_open.fcb + h68fcb.fileptr_be + 2], ax
	mov	al, [bx + rp_nfiles.files + h68files.attr]
	mov	[si + rp_open.fcb + h68fcb.attr], ax
	mov	ax, [bx + rp_nfiles.files + h68files.time_be]
	mov	[si + rp_open.fcb + h68fcb.time_be], ax
	mov	ax, [bx + rp_nfiles.files + h68files.date_be]
	mov	[si + rp_open.fcb + h68fcb.date_be], ax
	mov	ax, [bx + rp_nfiles.files + h68files.size_be]
	mov	[si + rp_open.fcb + h68fcb.size_be], ax
	mov	ax, [bx + rp_nfiles.files + h68files.size_be + 2]
	mov	[si + rp_open.fcb + h68fcb.size_be + 2], ax
	push	cx
	push	dx
	push	di
	push	es
	MOVSEG	es, ds
	lea	di, [si + rp_open.fcb]
	mov	bx, di
	mov	cx, rp_open_size
	mov	dx, h68fcb_size
	mov	al, RD_CMD_OPEN
	call	RD_FS_CalCmd
	pop	es
	pop	di
	pop	dx
	pop	cx
	jc	.deverr
	cmp	byte [rd_fs_result_be32], 80h
	jae	.fserr
	;and	cx, 0f0ffh
	;mov	[bx + h68fcb.mode_be], ch
	mov	[bx + h68fcb.mode_be + 2], cl
	;mov	dx, 8040h
;
.setup_sft:
	mov	dx, 8040h
	call	RD_h68fcb_to_sft_with_init
	mov	ax, [si + 6]		; rp_open(creat).key_be + 2
	xchg	ah, al
	mov	[es: di + SFT_FILEKEY_OFFSET], ax	; store file key
	; set up sft filename field
	push	si
	push	di
	mov	cx, 8 + 3
	add	si, rp_open.name + h68namests.name
	add	di, 20h
	rep	movsb
	pop	di
	pop	si
	; sft + 5 (word) device info
	; bit15=1  remote
	; bit14=x  sync actual mtime on close with: 0=current system time  1=mtime on the sft
	; bit13=0  (1=named pipe)
	; bit12=0  (1=no inherit)
	; bit11=0  (1=network sppoler)
	; bit7=0   (1=device)
	; bit6=x   0=dirty (the file has been written) 1=clean (not been written)
	; bit5~0   drive number
	or	dl, [drive_number]
	mov	[es: di + 5], dx	; device info
	mov	ax, [es: di + 2]	; file open mode
	test	ah, 80h			; FCB open?
	jnz	.open_fcb
	and	ax, 000fh
	mov	[es: di + 2], ax
	jmp	short .update_sft_2
.open_fcb:
	or	al, 0f0h
	mov	[es: di + 2], ax
	push	di
	push	es
	mov	ax, 120ch | 8000h	; set sft owner
	call	Invoke_Int2F12xx
	pop	es
	pop	di
.update_sft_2:
;
.noerr:
	clc
.exit:
	mov	cx, bp
	pop	bp
	ret
.deverr:
	mov	ax, 21		; device not ready
	jmp	short .err
.fserr:
	call	RD_FS_GetDOSError
.err:
	call	RD_freekey_dssi4
	stc
	jmp	short .exit


RD_Open2:		; 2F112E
	push	ds
	lds	bx, [sda]
	mov	dh, [bx + 2ddh]	; extended file open action code
	mov	dl, [bx + 2dfh]	; extended file open attibutes
	mov	cx, [bx + 2e1h]	; extended file open mode
	pop	ds
	call	RD_Open2_sub
	jc	.err_ax
	les	bx, [bp]
	mov	[es: bx + r_cx], cx
	jmp	rd_success_frame_esbx
.err_ax:
	jmp	RD_Fallback_error_AX


RD_Close:		; 2F1106
	mov	si, rd_fs_packet
	mov	ax, [es: di]
	test	ax, ax			; check file open count
	jz	.err_invalid_handle
.correct_handle:
	dec	ax
	mov	[es: di], ax
	jnz	.noerr
	call	RD_Commit_sft
.close:
	mov	cx, rp_close_size
	call	bzero_dssi
	lea	bx, [si + rp_close.fcb]
	call	RD_sft_to_h68fcb
	mov	ax, [es: di + SFT_FILEKEY_OFFSET]
	xchg	ah, al
	mov	[si + rp_close.key_be + 2], ax
	push	di
	push	es
	mov	di, bx
	mov	cx, rp_close_size
	mov	dx, h68fcb_size
	mov	al, RD_CMD_CLOSE
	call	RD_FS_CalCmd
	pop	es
	pop	di
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err
	call	RD_freekey_dssi4
	jc	.err_invalid_handle
.noerr:
	jmp	rd_success_noreg
.err_invalid_handle:
	mov	ax, 6
	jmp	short .err_ax
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err:
	jmp	RD_Fallback_error


; (ds:si  rd_fs_packet)
; es:di  sft (for obtaining filekey)
; al     whence
; cx:dx  offset
;
; result:
; ds:bx  rp_seek.fcb
; CF=0   success
;        cx:dx  offset from begining of the file
;        sft    update (offset + 15h: current offset)
; CF=1   error
;        ax     doserror
;
RD_lseek_sft:
	mov	si, rd_fs_packet
	push	cx
	mov	cx, rp_seek_size
	call	bzero_dssi
	pop	cx
	mov	[si + rp_seek.whence_be + 3], al
	mov	ax, [es: di + SFT_FILEKEY_OFFSET]	; store file key
	xchg	ah, al
	mov	[si + rp_rw.key_be + 2], ax
	lea	bx, [si + rp_rw.fcb]
	call	RD_sft_to_h68fcb
	xchg	ch, cl
	xchg	dh, dl
	mov	[si + rp_seek.offset_be], cx
	mov	[si + rp_seek.offset_be + 2], dx
	mov	al, RD_CMD_SEEK
	push	di
	push	es
	mov	cx, rp_seek_size
	lea	di, [si + rp_seek.fcb]
	MOVSEG	es, ds
	mov	dx, h68fcb_size
	call	RD_FS_CalCmd
	pop	es
	pop	di
	mov	ax, 21		; device err (drive not ready)
	jc	.exit
	; lseek ok?
	; (rd_fs_result = ftell(key), or RD_ERR_NOTOPENED/INVALIDPRM/CANTSEEK as err)
	mov	cx, [si + rp_seek.fcb + h68fcb.fileptr_be]
	mov	dx, [si + rp_seek.fcb + h68fcb.fileptr_be + 2]
	cmp	cx, [rd_fs_result_be32]
	jne	.fserr
	cmp	dx, [rd_fs_result_be32 + 2]
	jne	.fserr
	xchg	ch, cl
	xchg	dh, dl
	mov	[es: di + 15h], dx
	mov	[es: di + 17h], cx
;.noerr:
	xor	ax, ax			; CF=0, AX=0
.exit:
	ret
.fserr:
	call	RD_FS_GetDOSError
	ret


RD_readwrite_prepare:
	;mov	si, rd_fs_packet
	; - sync actual pointer with the sft's one
	; - setup ds:bx = rp_rw.fcb (rp_seek.fcb)
	push	cx
	push	dx
	mov	dx, [es: di + 15h]	; cx:dx = current offset 
	mov	cx, [es: di + 17h]
	mov	al, 0			; SEEK_SET
	call	RD_lseek_sft
	pop	dx
	pop	cx
	ret


RD_Write:		; 2F1109
	call	RD_readwrite_prepare
	jc	.err_ax
	mov	word [si + rp_rw.len_be], 0
	mov	[si + rp_rw.len_be + 3], cl
	mov	[si + rp_rw.len_be + 2], ch
	test	byte [es: di + 2], 7
	jz	.err_deny		; access denied if fmode==RDONLY
	jcxz	.sendwrite
	push	di
	push	es
	RD_GetSDADTA	es, di
	mov	al, RD_CMD_WRITE
	call	RD_FS_sendopt_prephase
	pop	es
	pop	di
	jc	.err_cmd
.sendwrite:
	mov	al, RD_CMD_WRITE
	push	cx
	mov	cx, rp_rw_size
	call	RD_FS_SendCmd
	pop	cx
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err
	cmp	cx, ax
	mov	cx, 0
	jb	.upd_sft		; avoid overrun (to be safe)
	mov	cx, ax
.upd_sft:
	and	word [es: di + 5], 0bfcfh	; device info bit14=0 bit6=0 (touch mtime)
	add	word [es: di + 15h], cx	; move file pointer
	adc	word [es: di + 17h], 0
	; if (cx == 0 || fileptr > filelength) filelength = fileptr
	push	cx
	test	cx, cx
	mov	dx, word [es: di + 15h]
	mov	cx, word [es: di + 17h]
	jz	.upd_sft_filelength
	cmp	cx, word [es: di + 13h]
	jb	.upd_sft_filelength_exit
	cmp	dx, word [es: di + 11h]
	jbe	.upd_sft_filelength_exit
.upd_sft_filelength:
	mov	word [es: di + 11h], dx
	mov	word [es: di + 13h], cx
.upd_sft_filelength_exit:
	pop	cx
	les	bx, [bp]
	mov	[es: bx + r_cx], cx
	jmp	rd_success_frame_esbx
.err_deny:
	mov	ax, 5
	jmp	short .err_ax
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err:
	jmp	RD_Fallback_error


RD_Read:		; 2F1108
	call	RD_readwrite_prepare
	jc	.err_ax
	mov	word [si + rp_rw.len_be], 0
	mov	[si + rp_rw.len_be + 3], cl
	mov	[si + rp_rw.len_be + 2], ch
	mov	al, RD_CMD_READ
	push	cx
	push	di
	push	es
	mov	di, bx
	MOVSEG	es, ds
	mov	cx, rp_rw_size
	mov	dx, h68fcb_size
	call	RD_FS_CalCmd
	pop	es
	pop	di
	pop	cx
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err
	cmp	cx, ax		; avoid buffer overrun
	mov	cx, 0
	jb	.upd_sft
	mov	cx, ax
	jcxz	.upd_sft
	push	di
	push	es
	RD_GetSDADTA	es, di
	call	RD_FS_readopt_continuous
	pop	es
	pop	di
	jc	.err_cmd
.upd_sft:
%if 0
	call	RD_h68fcb_to_sft
%else
	add	word [es: di + 15h], cx	; move file pointer
	adc	word [es: di + 17h], 0
%endif
	les	bx, [bp]
	mov	[es: bx + r_cx], cx
	jmp	rd_success_frame_esbx
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err:
	jmp	RD_Fallback_error


RD_Commit:		; 2F1107
	mov	si, rd_fs_packet
	call	RD_Commit_sft
	jmp	rd_success_noreg


RD_SetAttr:		; 2F110E
	mov	si, rd_fs_packet
	mov	cx, rp_attribute_size
	call	bzero_dssi
	les	bx, [bp]
	mov	al, [bx + rd_extra]
	and	al, 3fh
	mov	[si + rp_attribute.attr_be + 3], al
	les	di, [sda_fn1]
	push	si
	add	si, rp_attribute.name
	mov	ah, 0
	call	PNtoNamests
	pop	si
	jc	.err_invalid_pn
	mov	al, RD_CMD_ATTRIBUTE
	call	RD_FS_SendCmd
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err_fs
	jmp	rd_success_noreg
.err_invalid_pn:
	mov	ax, 2		; 2=file not found (11=format invalid)
	jmp	short .err_ax
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err_fs:
	jmp	RD_Fallback_error


RD_SeekFromEnd:		; 2F1121
	mov	al, 2		; SEEK_END
	xor	dx, dx		; offset = 0
	xor	cx, cx
	call	RD_lseek_sft
	jc	.err_ax
%if 0
	mov	dx, [es: di + 15h]
	mov	cx, [es: di + 17h]
%endif
	les	bx, [bp]
	mov	[es: bx + r_ax], dx
	mov	[es: bx + r_dx], cx
	jmp	rd_success_frame_esbx
.err_ax:
	jmp	RD_Fallback_error_AX


RD_Delete:		; 2F1113
	mov	dl, RD_CMD_DELETE
	jmp	short RD_mkrmdir


RD_Rmdir:		; 2F1101
	mov	dl, RD_CMD_REMOVEDIR
	jmp	short RD_mkrmdir


RD_Mkdir:		; 2F1103
	mov	dl, RD_CMD_MAKEDIR
RD_mkrmdir:
	mov	si, rd_fs_packet
	mov	cx, rp_dir_size
	call	bzero_dssi
	les	di, [sda_fn1]
	push	si
	add	si, rp_dir.name
	mov	ah, 0
	call	PNtoNamests
	pop	si
	jc	.err_invalid_pn
	mov	al, dl
	call	RD_FS_SendCmd
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err_fs
	jmp	rd_success_noreg
.err_invalid_pn:
	mov	ax, 2		; 2=file not found (11=format invalid)
	jmp	short .err_ax
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err_fs:
	jmp	RD_Fallback_error


RD_Rename:		; 2F1111
	mov	si, rd_fs_packet
	mov	cx, rp_rename_size
	call	bzero_dssi
	push	si
	add	si, rp_rename.name
	les	di, [sda_fn1]
	mov	ah, 0
	call	PNtoNamests
	jc	.nm_end
	add	si, rp_rename.name_new - rp_rename.name
	les	di, [sda_fn2]
	mov	ah, 0
	call	PNtoNamests
.nm_end:
	pop	si
	jc	.err_invalid_pn
	mov	al, RD_CMD_RENAME
	call	RD_FS_SendCmd
	jc	.err_cmd
	cmp	byte [rd_fs_result_be32], 80h
	jae	.err_fs
	jmp	rd_success_noreg
.err_invalid_pn:
	mov	ax, 2		; 2=file not found (11=format invalid)
	jmp	short .err_ax
.err_cmd:
	mov	ax, 21		; drive not ready
.err_ax:
	jmp	RD_Fallback_error_AX
.err_fs:
	jmp	RD_Fallback_error


RD_Flock:		; 2F110A
;RD_Funlock3:		; 2F110B
;RD_CreatWithoutCDS:	; 2F1118
;RD_FindFirstWithoutCDS:	; 2F1119
;RD_CloseAll:		; 2F111D
;RD_FlushAll:		; 2F1120
;RD_HookSFT:		; 2F112C
;RD_Attr2:		; 2F112D

rd_fallback_notready:
	mov	ax, 21	; (drive not ready)
rd_fallback_errret:
	jmp	RD_Fallback_error

rd_success_noreg:
	les	bx, [bp]
rd_success_frame_esbx:
	and	byte [es: bx + r_flags], 0feh	; CF=0
	ret


	TSR_CODE_END


	TSR_DATA

	global rd_buffer
	global rd_fs_packet
	global rd_fs_packet_bottom

	global rd_search_table

rd_filekey_in_use:
	db	0ffh			; index 0
rd_filekey_in_use_1:
	times (FILEKEY_MAX) db 0	; index 1..FILEKEY_MAX
rd_filekey_in_use_bottom:

rd_searchkey_number:
	db	SEARCHKEY_MAX
rd_search_table:	; index 1..SEARCHKEY_MAX
	times (SEARCHKEY_MAX * rp_nfiles_size) db 0
rd_getattr_table:	; index (SEARCHKEY_MAX+1)
	times rp_nfiles_size db 0
rd_search_table_bottom:

rd_buffer:
rd_fs_packet:
	times rp_rename_size db 0	; max size (for rename)
rd_fs_packet_bottom:

	TSR_DATA_END



