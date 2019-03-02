
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
%define DECLARE_MESSAGE
	%include 'rd_msg.inc'

cr	equ	13
lf	equ	10

	INIT_DATA

err_not_supported_dos:
	db	'Not supported version of DOS', 0
err_unsupported_host:
	db	'Host device not supported', 0
err_no_device:
	db	'Driver is not installed', 0
err_cant_release_device:
	db	"Can't release because RASDRV is a device driver", 0
err_already_installed:
	db	'RASDRV already installed', 0
err_cant_install_as_device:
	db	"Can't install as device", 0
err_invalid_drive_letter:
	db	'Invalid drive letter', 0
err_invalid_root_path:
	db	'Invalid root path', 0
err_root_path_too_long:
	db	'Too long root path', 0
err_drive_already_use:
	db	'Specified drive is already used', 0
err_invalid_scsi_id:
	db	'Invalid SCSI ID', 0
err_rasbridge_not_exist:
	db	'RaSCSI Bridge device not exist', 0
err_rasdrv_init_failure:
	db	'RASDRV init failure', 0
err_invalid_basepath:
	db	'Invalid root path', 0

msg_title:
	db	'RASDRV for DOS', 0
msg_build_date:
	db	'built at ', __DATE__, ' ', __TIME__ , 0
msg_extra_info:
	db	'**alpha version** '
%ifdef DEBUG
	db	'[DEBUG] '
%endif
	db	0
msg_success:
	db	'Successfully installed', 0
msg_success_scsiid1:
	db	' SCSI ID:     ', 0
msg_success_scsiid2:
	db	0
msg_success_drive1:
	db	' Drive:       ', 0
msg_success_drive2:
	db	0
msg_success_rdbase1:
	db	' RaSCSI base: ', 0
msg_success_rdbase2:
	db	0

msg_release:
	db	'Successfully released', 0
msg_help:
	db	'Usage: RASDRV -In -Ddrive:[path]', cr, lf
	db	'  -I   SCSI ID of RaSCSI bridge device', cr, lf
	db	'  -D   mount drive (for DOS) and root path (for Raspbian)', cr, lf
	db	'  -R   remove RASDRV from memory', cr, lf
	db	'example:', cr, lf
	db	'  rasdrv -I6 -DX:/home/pi', cr, lf
	db	0



	INIT_DATA_END

