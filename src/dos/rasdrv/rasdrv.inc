
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

RD_CMD_BASE		equ	40h
RD_CMD_INITDEVICE	equ	(RD_CMD_BASE + 00h)
RD_CMD_CHECKDIR		equ	(RD_CMD_BASE + 01h)
RD_CMD_MAKEDIR		equ	(RD_CMD_BASE + 02h)
RD_CMD_REMOVEDIR	equ	(RD_CMD_BASE + 03h)
RD_CMD_RENAME		equ	(RD_CMD_BASE + 04h)
RD_CMD_DELETE		equ	(RD_CMD_BASE + 05h)
RD_CMD_ATTRIBUTE	equ	(RD_CMD_BASE + 06h)
RD_CMD_FILES		equ	(RD_CMD_BASE + 07h)
RD_CMD_NFILES		equ	(RD_CMD_BASE + 08h)
RD_CMD_CREATE		equ	(RD_CMD_BASE + 09h)
RD_CMD_OPEN		equ	(RD_CMD_BASE + 0ah)
RD_CMD_CLOSE		equ	(RD_CMD_BASE + 0bh)
RD_CMD_READ		equ	(RD_CMD_BASE + 0ch)
RD_CMD_WRITE		equ	(RD_CMD_BASE + 0dh)
RD_CMD_SEEK		equ	(RD_CMD_BASE + 0eh)
RD_CMD_TIMESTAMP	equ	(RD_CMD_BASE + 0fh)
RD_CMD_GETCAPACITY	equ	(RD_CMD_BASE + 10h)
RD_CMD_CTRLDRIVE	equ	(RD_CMD_BASE + 11h)
RD_CMD_GETDPB		equ	(RD_CMD_BASE + 12h)

RD_ERR_INVALIDFUNC	equ	(-1)
RD_ERR_FILENOTFND	equ	(-2)
RD_ERR_DIRNOTFND	equ	(-3)
RD_ERR_OVEROPENED	equ	(-4)
RD_ERR_CANTACCESS	equ	(-5)
RD_ERR_NOTOPENED	equ	(-6)
RD_ERR_INVALIDMEM	equ	(-7)
RD_ERR_OUTOFMEM		equ	(-8)
RD_ERR_INVALIDPTR	equ	(-9)
RD_ERR_INVALIDENV	equ	(-10)
RD_ERR_ILLEGALFMT	equ	(-11)
RD_ERR_ILLEGALMOD	equ	(-12)
RD_ERR_ILLEGALPATH	equ	(-13)
RD_ERR_INVALIDPRM	equ	(-14)
RD_ERR_INVALIDDRV	equ	(-15)
RD_ERR_DELCURDIR	equ	(-16)
RD_ERR_NOTIOCTL		equ	(-17)
RD_ERR_LASTFILE		equ	(-18)
RD_ERR_CANTWRITE	equ	(-19)
RD_ERR_DIRALREADY	equ	(-20)
RD_ERR_CANTDELETE	equ	(-21)
RD_ERR_CANTRENAME	equ	(-22)
RD_ERR_DISKFULL		equ	(-23)
RD_ERR_DIRFULL		equ	(-24)
RD_ERR_CANTSEEK		equ	(-25)
RD_ERR_SUPERVISOR	equ	(-26)
RD_ERR_THREADNAME	equ	(-27)
RD_ERR_BUFWRITE		equ	(-28)
RD_ERR_BACKGROUND	equ	(-29)
RD_ERR_OUTOFLOCK	equ	(-32)
RD_ERR_LOCKED		equ	(-33)

RD_ERR_FILEEXIST	equ	(-80)

RD_FATAL_INVALIDUNIT	equ	(-90)
RD_FATAL_INVALIDCOMMAND	equ	(-91)
RD_FATAL_WRITEPROTECT	equ	(-92)
RD_FATAL_MEDIAOFFLINE	equ	(-93)


; NOTE: most word/dword value is stored with big-endian

STRUC  h68namests
.wildcard	resb 1
.drive		resb 1
.path		resb 65
.name		resb 8
.ext		resb 3
.add		resb 10
ENDSTRUC


STRUC  h68files
.fatr		resb 1
.pad1		resb 3
.sector_be	resb 4
.offset_be	resb 2
.attr		resb 1
.pad2		resb 1
.time_be	resb 2
.date_be	resb 2
.size_be	resb 4
.full		resb 23
.pad3		resb 1
ENDSTRUC


STRUC  h68fcb
.fileptr_be	resb 4
.mode_be	resb 2
.attr		resb 1
.pad		resb 1
.time_be	resb 2
.date_be	resb 2
.size_be	resb 4
ENDSTRUC


STRUC  h68capacity
.freearea_be	resb 2
.clusters_be	resb 2
.sectors_be	resb 2
.bytes_be	resb 2
ENDSTRUC


STRUC  h68ctrldrive
.status		resb 1
.pad		resb 3
ENDSTRUC


STRUC  h68dpb
.sector_size_be	resb 2
.cluster_size	resb 1
.shift		resb 1
.fat_sector_be	resb 2
.fat_max	resb 1
.fat_size	resb 1
.file_max_be	resb 2
.data_sector_be	resb 2
.cluster_max_be	resb 2
.root_sector_be	resb 2
.media		resb 1
.pad		resb 1
ENDSTRUC


STRUC  h68dirent
.name		resb 8
.ext		resb 3
.attr		resb 1
.add		resb 10
.time_be	resb 2
.date_be	resb 2
.cluster_be	resb 2
.size_be	resb 4
ENDSTRUC


STRUC  h68ioctl
.buffer		resb 8
.param_be	resb 4
.media_be	resb 2
ENDSTRUC


STRUC  h68arg
.buffer		resb 256
ENDSTRUC


;------------------------------

STRUC  rp_files
.unit_be	resb 4
.key_be		resb 4
.name		resb h68namests_size
.files		resb h68files_size
ENDSTRUC

STRUC  rp_nfiles
.unit_be	resb 4
.key_be		resb 4
.files		resb h68files_size
ENDSTRUC


STRUC  rp_dir
.unit_be	resb 4
.name		resb h68namests_size
ENDSTRUC


STRUC  rp_rename
.unit_be	resb 4
.name		resb h68namests_size
.name_new	resb h68namests_size
ENDSTRUC


STRUC  rp_attribute
.unit_be	resb 4
.name		resb h68namests_size
.attr_be	resb 4
ENDSTRUC


STRUC  rp_open
.unit_be	resb 4
.key_be		resb 4
.name		resb h68namests_size
.fcb		resb h68fcb_size
ENDSTRUC


STRUC  rp_creat
.unit_be	resb 4
.key_be		resb 4
.name		resb h68namests_size
.fcb		resb h68fcb_size
.attr_be	resb 4
.force_be	resb 4
ENDSTRUC


STRUC  rp_close
.unit_be	resb 4
.key_be		resb 4
.fcb		resb h68fcb_size
ENDSTRUC


STRUC  rp_rw
.key_be		resb 4
.fcb		resb h68fcb_size
.len_be		resb 4
ENDSTRUC


STRUC  rp_seek
.key_be		resb 4
.fcb		resb h68fcb_size
.whence_be	resb 4
.offset_be	resb 4
ENDSTRUC


STRUC  rp_ftime
.unit_be	resb 4
.key_be		resb 4
.fcb		resb h68fcb_size
.date_be	resb 2
.time_be	resb 2
ENDSTRUC


