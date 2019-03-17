
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


%ifdef PUBDECL
  %undef PUBDECL
%endif


%ifdef DECLARE_RD_INT2F
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif


	PUBDECL is_dos3
	PUBDECL lol
	PUBDECL sda
	PUBDECL sda_fn1
	PUBDECL sda_fn2
	PUBDECL sda_psft
	PUBDECL sda_dirent
	PUBDECL sda_fcb1
	PUBDECL sda_fcb2
	PUBDECL sda_sattr
	PUBDECL sda_pcds
	PUBDECL sda_pdta
	PUBDECL sda_searchblock
	PUBDECL rd_drive
	PUBDECL rd_basepath
	PUBDECL rd_basepath_offset

	PUBDECL New2F
	PUBDECL InitFS
	PUBDECL ReleaseFS

	PUBDECL rd_success_noreg
	PUBDECL rd_success_frame_esbx
	PUBDECL RD_Fallback_error
	PUBDECL RD_Fallback_error_AX
	PUBDECL Invoke_Int2F12xx

; register frame
STRUC rd_regframe
.R_es		resw	1
.R_ds		resw	1
.R_di		resw	1
.R_si		resw	1
.R_bp		resw	1
.R_sp		resw	1
.R_bx		resw	1
.R_dx		resw	1
.R_cx		resw	1
.R_ax		resw	1
.R_flags_0	resw	1
.R_ip_org	resw	1
.R_cs_org	resw	1
.R_flags	resw	1
.rd_extra	resw	1
;
ENDSTRUC

%macro RD_PREPARE_REGFRAME 0
	push	ax
	push	cx
	push	dx
	push	bx
	push	sp
	push	bp
	push	si
	push	di
	push	ds
	push	es
%endmacro

%macro RD_DISPOSE_REGFRAME 0
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	pop	ax	; just drop (sp)
	pop	bx
	pop	dx
	pop	cx
	pop	ax
%endmacro

%define r_ax	rd_regframe.R_ax
%define r_al	rd_regframe.R_ax
%define r_ah	(rd_regframe.R_ax + 1)
%define r_bx	rd_regframe.R_bx
%define r_bl	rd_regframe.R_bx
%define r_bh	(rd_regframe.R_bx + 1)
%define r_cx	rd_regframe.R_cx
%define r_cl	rd_regframe.R_cx
%define r_ch	(rd_regframe.R_cx + 1)
%define r_dx	rd_regframe.R_dx
%define r_dl	rd_regframe.R_dx
%define r_dh	(rd_regframe.R_dx + 1)
%define r_si	rd_regframe.R_si
%define r_di	rd_regframe.R_di
%define r_ds	rd_regframe.R_ds
%define r_es	rd_regframe.R_es
%define r_flags	rd_regframe.R_flags
%define rd_extra	rd_regframe.rd_extra


%macro RD_GetSDACDS 2
	l%1	%2, [sda_pcds]
	l%1	%2, [%1: %2]
%endmacro

%macro RD_GetSDADTA 2
	l%1	%2, [sda_pdta]
	l%1	%2, [%1: %2]
%endmacro


%macro RD_GetSDADirent 2
	l%1	%2, [sda_pdta]
	l%1	%2, [%1: %2]
%endmacro


