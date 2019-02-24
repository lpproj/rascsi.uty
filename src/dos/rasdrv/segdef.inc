
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


	BITS 16
	CPU 8086

[segment _TSR_HEADER align=16]
[segment _TSR_TEXT align=2]
[segment _TSR_DATA align=2]
[segment _TSR_BOTTOM align=16]
[segment _INIT_TEXT align=16]
[segment _INIT_DATA]
[segment _INIT_BSS align=16]
%ifidn __OUTPUT_FORMAT__, obj
[segment STACK align=16 class=STACK stack]
%endif

group AGROUP  _TSR_HEADER _TSR_TEXT _TSR_DATA _TSR_BOTTOM _INIT_TEXT _INIT_DATA _INIT_BSS


%define TSR_HEADER	[segment _TSR_HEADER]
%define TSR_CODE	[segment _TSR_TEXT]
%define TSR_DATA	[segment _TSR_DATA]
%define TSR_BOTTOM	[segment _TSR_BOTTOM]
%define INIT_CODE	[segment _INIT_TEXT]
%define INIT_DATA	[segment _INIT_TEXT]
%define INIT_BSS	[segment _INIT_BSS]
%define INIT_STACK	[segment STACK]


%define TSR_HEADER_END
%define TSR_CODE_END
%define TSR_DATA_END
%define TSR_BOTTOM_END
%define INIT_CODE_END
%define INIT_DATA_END
%define INIT_STACK_END

