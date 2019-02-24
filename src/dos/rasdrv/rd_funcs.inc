
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


%ifdef DECLARE_RD_FUNCS
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif


	PUBDECL RD_Rmdir		; 2F1101
	PUBDECL RD_Mkdir		; 2F1103
	PUBDECL RD_Chdir		; 2F1105
	PUBDECL RD_Close		; 2F1106
	PUBDECL RD_Commit		; 2F1107
	PUBDECL RD_Read			; 2F1108
	PUBDECL RD_Write		; 2F1109
	PUBDECL RD_Flock		; 2F110A
;	PUBDECL RD_Funlock3		; 2F110B
	PUBDECL RD_Diskinfo		; 2F110C
	PUBDECL RD_SetAttr		; 2F110E
	PUBDECL RD_GetAttr		; 2F110F
	PUBDECL RD_Rename		; 2F1111
	PUBDECL RD_Delete		; 2F1113
	PUBDECL RD_Open			; 2F1116
	PUBDECL RD_Creat		; 2F1117
;	PUBDECL RD_OpenWithoutCDS	; 2F1118
;	PUBDECL RD_FindFirstWithoutCDS	; 2F1119

	PUBDECL RD_FindFirst		; 2F111B
	PUBDECL RD_FindNext		; 2F111C
;	PUBDECL RD_CloseAll		; 2F111D
;	PUBDECL RD_FlushAll		; 2F1120
	PUBDECL RD_SeekFromEnd		; 2F1121
;	PUBDECL RD_UpdateSFT		; 2F112C
;	PUBDECL RD_Attr2		; 2F112D
	PUBDECL RD_Open2		; 2F112E

	PUBDECL RD_checkdir

	PUBDECL rd_buffer

%define FILEKEY_MAX	64
%define SEARCHKEY_MAX	3
%define SFT_FILEKEY_OFFSET	1dh

