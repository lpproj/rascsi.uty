
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


%ifdef DECLARE_RD_SCSI
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif

	PUBDECL msg_scsi_platform
	PUBDECL InitSCSI
	PUBDECL scsi_cmd

	PUBDECL RD_FS_SendCmd
	PUBDECL RD_FS_CalCmd
	PUBDECL RD_FS_readopt_continuous
	PUBDECL RD_FS_sendopt_prephase

	PUBDECL RD_FS_GetDOSError

	PUBDECL rd_fs_result_be32
	PUBDECL rd_fs_result_lsb

	PUBDECL rd_fs_init_extra_param
	PUBDECL rd_fs_init_extra_param_bottom


%ifndef REQUEST_SENSE_MAX
  %define REQUEST_SENSE_MAX  18
%endif

SCSI_NO_TRANSFER	equ 0
SCSI_FROM_DEVICE	equ 1
SCSI_TO_DEVICE		equ 2

