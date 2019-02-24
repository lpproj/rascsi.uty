
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

%ifdef DECLARE_RD_MISC
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif


	PUBDECL cmd_argc
	PUBDECL cmd_arg
	PUBDECL cmdline
	PUBDECL ParseCmd
	PUBDECL CharUpr
	PUBDECL CopyCmdLine

	PUBDECL strCpy
	PUBDECL IsDBCSLead
	PUBDECL bzero_dssi

	PUBDECL putc_func
	PUBDECL PutC
	PUBDECL PutC_dos
	PUBDECL PutC_int29
	PUBDECL PutNL
	PUBDECL PutS0
	PUBDECL PutS0n
	PUBDECL PutH8
	PUBDECL PutH16
	PUBDECL PutD16

%ifdef DEBUG
	PUBDECL PutRegs
%endif


%macro MOVSEG 2
	push	%2
	pop	%1
%endmacro

%macro XCHGSEG 2
	push	%2
	push	%1
	pop	%2
	pop	%1
%endmacro


