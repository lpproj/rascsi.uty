
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

%ifdef DECLARE_MESSAGE
  %define PUBDECL	global
%else
  %define PUBDECL	extern
%endif

	PUBDECL msg_title
	PUBDECL msg_build_date
	PUBDECL msg_extra_info
	PUBDECL msg_help
	PUBDECL msg_success
	PUBDECL msg_success_scsiid1
	PUBDECL msg_success_scsiid2
	PUBDECL msg_success_drive1
	PUBDECL msg_success_drive2
	PUBDECL msg_success_rdbase1
	PUBDECL msg_success_rdbase2
	PUBDECL msg_release

	PUBDECL err_not_supported_dos
	PUBDECL err_no_device
	PUBDECL err_cant_release_device
	PUBDECL err_already_installed
	PUBDECL err_cant_install_as_device
	PUBDECL err_already_installed
	PUBDECL err_cant_install_as_device
	PUBDECL err_invalid_drive_letter
	PUBDECL err_invalid_root_path
	PUBDECL err_root_path_too_long
	PUBDECL err_drive_already_use
	PUBDECL err_invalid_scsi_id
	PUBDECL err_rasbridge_not_exist
	PUBDECL err_rasdrv_init_failure
	PUBDECL err_invalid_basepath


