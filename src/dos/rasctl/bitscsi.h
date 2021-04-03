/*

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

*/

#ifndef bitscsi_h_included
#define bitscsi_h_included

#ifndef BS_REQUEST_SENSE_MAX
# define BS_REQUEST_SENSE_MAX 18
#endif
#ifndef BS_TRANSFER_DATA_MAX
# define BS_TRANSFER_DATA_MAX 16384
#endif


#ifdef DEBUG
# define PLINE { fprintf(stderr, "line %u\n", __LINE__); }
#else
# define PLINE
#endif

#if defined WIN32 || defined _WIN32

# define BITSCSI_WIN32   1

# include <windows.h>
# include <winioctl.h>

# if defined _MSC_VER || defined USE_WINDDK
#  include <ntddscsi.h>
# else
#  include <ddk/ntddscsi.h>
# endif

#elif 1 /* defined __DOS__ || defined MSDOS || defined DOS */
# define BITSCSI_DOS    1
# if (INT_MAX) <= 0x7fff
#  define BITSCSI_DOS16 1
# else
#  define BITSCSI_DOS32 1
# endif

#endif

#include <limits.h>

#if defined __STDC_VERSION__ || ((__STDC_VERSION__)>=199901L)
# include <stdint.h>
typedef uint8_t   BSu8;
typedef uint16_t  BSu16;
typedef uint32_t  BSu32;
#else
typedef unsigned char BSu8;
typedef unsigned short BSu16;
# if UINT_MAX == 0xffffffffUL
typedef unsigned int BSu32;
# else
typedef unsigned long BSu32;
# endif
#endif

#if defined __clang__
# define bs_packed_pre
# define bs_packed_post __attribute__((__packed__))
#elif defined __GNUC__
# define bs_packed_pre  __attribute__((__packed__))
# define bs_packed_post
#else
# define bs_packed_pre /* */
# define bs_packed_post /* */
#endif

#if !(defined __TURBOC__ || defined LSI_C || defined __GNUC__ || defined __clang__)
# pragma pack(1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BS_NO_TRANSFER = 0,
    BS_FROM_DEVICE,
    BS_TO_DEVICE
};

struct bs_packed_pre BS_CMDPKT {
    unsigned cdb_length;
    BSu8 scsi_status;
    BSu8 cdb[16];
    unsigned sense_length;
    BSu8 sense_data[BS_REQUEST_SENSE_MAX];
} bs_packed_post;
typedef struct BS_CMDPKT  BS_CMDPKT;


/* helpers (poke & peek) */
void bs_w16be(void *m, BSu16 v);
void bs_w24be(void *m, BSu32 v);
void bs_w32be(void *m, BSu32 v);
BSu16 bs_r16be(const void * const m);
BSu32 bs_r24be(const void * const m);
BSu32 bs_r32be(const void * const m);

/* helper */
struct bs_packed_pre BS_INQUIRY_STD {
    BSu8 dev_type;
    BSu8 dev_type_modifier;
    BSu8 version;
    BSu8 response_data_format;
    BSu8 additional_length;
    BSu8 reserved_05;
    BSu8 reserved_06;
    BSu8 flags_07;
    BSu8 vendor[8];
    BSu8 product_id[16];
    BSu8 product_revision[4];
} bs_packed_post;
typedef struct BS_INQUIRY_STD  BS_INQUIRY_STD;
#define SIZE_OF_BS_INQUIRY_STD  36

/* */

#if defined BITSCSI_DOS16

enum {
    BS_MACHINE_UNCHECK = -1,
    BS_MACHINE_UNKNOWN = 0,
    BS_MACHINE_IBMPC,
    BS_MACHINE_NEC98,
    BS_MACHINE_FMR
};
int bs_get_machine_dos(void);

int bs_init_aspidos(void);
int bs_cmd_aspidos(int adapter, int id, int lun, BS_CMDPKT *cmd, int transfer_direction, void *data, unsigned datalen);
int bs_cmd_55bios(int adapter, int id, int lun, BS_CMDPKT *cmd, int transfer_direction, void *data, unsigned datalen);

int bs_get_machine_dos(void);
int bs_cmd(int adapter, int id, int lun, BS_CMDPKT *cmd, int transfer_direction, void *data, unsigned datalen);

int bs_clear_cmdpkt(BS_CMDPKT *pkt);
int bs_inquiry(int adapter, int id, int lun, void *inq_data, unsigned transfer_length);



#endif /*  BITSCSI_DOS16 */

#ifdef __cplusplus
}
#endif

#if !(defined __TURBOC__ || defined LSI_C || defined __GNUC__ || defined __clang__)
# pragma pack()
#endif

#endif /* bitscsi_included */

