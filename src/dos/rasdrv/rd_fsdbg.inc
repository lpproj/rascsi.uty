
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


%ifdef DECLARE_RD_FSDBG
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif



%ifdef DEBUG
	PUBDECL dbgPutFN1
	PUBDECL dbgPutFN2
	PUBDECL dbgPutCDS
	PUBDECL dbgPutSFT_esdi
	PUBDECL dbgPutSFT_sda
	PUBDECL dbgh68fcb_dsbx
	PUBDECL dbgPutNamests
	PUBDECL dbgPutOpenMode
  %macro Debug_PutFN1 0
	call	dbgPutFN1
  %endmacro
  %macro Debug_PutFN2 0
	call	dbgPutFN2
  %endmacro
  %macro Debug_PutCDS 0
	call	dbgPutCDS
  %endmacro
  %macro Debug_PutSFT_esdi 0
	call	dbgPutSFT_esdi
  %endmacro
  %macro Debug_PutSFT_sda 0
	call	dbgPutSFT_sda
  %endmacro
  %macro Debug_h68fcb_dsbx 0
	call	dbgh68fcb_dsbx
  %endmacro
  %macro Debug_PutNamests 0
	call	dbgPutNamests
  %endmacro
  %macro Debug_PutOpenMode 0
	call	dbgPutOpenMode
  %endmacro
%else
  %define Debug_PutFN1
  %define Debug_PutFN2
  %define Debug_PutCDS
  %define Debug_PutSFT_esdi
  %define Debug_PutSFT_sda
  %define Debug_PutNamests
  %define Debug_PutOpenMode
%endif



