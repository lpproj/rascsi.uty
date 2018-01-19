/*
rasdrv.h: a sample of using RASDRV on DOS


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

#ifndef MY_RASDRV_H
#define MY_RASDRV_H

#include "mypack1.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RASCSI_VENDOR_SIGNATURE   "RaSCSI"
#define RASCSI_VENDOR_ID          "RaSCSI  "
#define RASCSI_BRIDGE_SIGNATURE   "RASCSI BRIDGE"
#define RASCSI_BRIDGE_PRODUCT_ID  "RASCSI BRIDGE   "


typedef signed char  RSC_I8;
typedef unsigned char  RSC_U8;
typedef signed short  RSC_I16;
typedef unsigned short  RSC_U16;
typedef signed long  RSC_I32;
typedef unsigned long  RSC_U32;

typedef RSC_I32  RASDRV_RESULT;

struct RSC_BE16 {
	RSC_U8	b[2];
} GCC_ATTRIBUTE_PACK1;
struct RSC_BE32 {
	RSC_U8	b[4];
} GCC_ATTRIBUTE_PACK1;

typedef struct RSC_BE16  RSC_BE16;
typedef struct RSC_BE32  RSC_BE32;


extern RSC_U16  peekBE16(const RSC_BE16 *);
extern RSC_U32  peekBE32(const RSC_BE32 *);
extern void  pokeBE16(RSC_BE16 *, RSC_U16 v);
extern void  pokeBE32(RSC_BE32 *, RSC_U32 v);

#define RASCSI_BRIDGE_TAP		1
#define RASCSI_BRIDGE_RASDRV	2
#define RASCSI_TAP_SETMAC		0
#define RASCSI_TAP_GETMAC		0
#define RASCSI_TAP_SEND			1
#define RASCSI_TAP_RECV1		1
#define RASCSI_TAP_RECV_GETLENGTH	0
#define RASCSI_TAP_RECV_GETDATA	1
#define RASCSI_TAP_RECV2		2
#define RASCSI_RASDRV_SENDCMD	0
#define RASCSI_RASDRV_GETRESULT	0
#define RASCSI_RASDRV_GETDATA	1
#define RASCSI_RASDRV_SENDOPTION	1
#define RASCSI_RASDRV_GETOPTION	2

#ifndef HUMAN68K_PATH_MAX
# define HUMAN68K_PATH_MAX	96
#endif

#define H68K_FS_CMDBASE(n)		(0x40 + (n))

#define FS_CMD_INITDEVICE		H68K_FS_CMDBASE(0)
#define FS_CMD_CHECKDIR			H68K_FS_CMDBASE(1)
#define FS_CMD_MAKEDIR			H68K_FS_CMDBASE(2)
#define FS_CMD_REMOVEDIR		H68K_FS_CMDBASE(3)
#define FS_CMD_RENAME			H68K_FS_CMDBASE(4)
#define FS_CMD_DELETE			H68K_FS_CMDBASE(5)
#define FS_CMD_ATTRIBUTE		H68K_FS_CMDBASE(6)
#define FS_CMD_FILES			H68K_FS_CMDBASE(7)
#define FS_CMD_NFILES			H68K_FS_CMDBASE(8)
#define FS_CMD_FINDFIRST		FS_CMD_FILES
#define FS_CMD_FINDNEXT			FS_CMD_NFILES
#define FS_CMD_CREATE			H68K_FS_CMDBASE(9)
#define FS_CMD_OPEN				H68K_FS_CMDBASE(0x0a)
#define FS_CMD_CLOSE			H68K_FS_CMDBASE(0x0b)
#define FS_CMD_READ				H68K_FS_CMDBASE(0x0c)
#define FS_CMD_WRITE			H68K_FS_CMDBASE(0x0d)
#define FS_CMD_SEEK				H68K_FS_CMDBASE(0x0e)
#define FS_CMD_TIMESTAMP		H68K_FS_CMDBASE(0x0f)
#define FS_CMD_GETCAPACITY		H68K_FS_CMDBASE(0x10)
#define FS_CMD_CTRLDRIVE		H68K_FS_CMDBASE(0x11)
#define FS_CMD_GETDPB			H68K_FS_CMDBASE(0x12)
#define FS_CMD_DISKREAD			H68K_FS_CMDBASE(0x13)
#define FS_CMD_DISKWRITE		H68K_FS_CMDBASE(0x14)
#define FS_CMD_IOCTL			H68K_FS_CMDBASE(0x15)
#define FS_CMD_FLUSH			H68K_FS_CMDBASE(0x16)
#define FS_CMD_CHECKMEDIA		H68K_FS_CMDBASE(0x17)
#define FS_CMD_LOCK				H68K_FS_CMDBASE(0x18)


#define RSC_FS_ERR(n)		(RSC_U32)(0xffffff00UL | (RSC_U32)(RSC_I32)(n))

#define FS_INVALIDFUNC			RSC_FS_ERR(0xff)
#define FS_FILENOTFND			RSC_FS_ERR(0xfe)
#define FS_DIRNOTFND			RSC_FS_ERR(0xfd)
#define FS_OVEROPENED			RSC_FS_ERR(0xfc)
#define FS_CANTACCESS			RSC_FS_ERR(0xfb)
#define FS_NOTOPENED			RSC_FS_ERR(0xfa)
#define FS_INVALIDMEM			RSC_FS_ERR(0xf9)
#define FS_OUTOFMEM				RSC_FS_ERR(0xf8)
#define FS_INVALIDPTR			RSC_FS_ERR(0xf7)
#define FS_INVALIDENV			RSC_FS_ERR(0xf6)
#define FS_ILLEGALFMT			RSC_FS_ERR(0xf5)
#define FS_ILLEGALMOD			RSC_FS_ERR(0xf4)
#define FS_INVALIDPATH			RSC_FS_ERR(0xf3)
#define FS_INVALIDPRM			RSC_FS_ERR(0xf2)
#define FS_INVALIDDRV			RSC_FS_ERR(0xf1)
#define FS_DELCURDIR			RSC_FS_ERR(0xf0)
#define FS_NOTIOCTRL			RSC_FS_ERR(0xef)
#define FS_LASTFILE				RSC_FS_ERR(0xee)
#define FS_CANTWRITE			RSC_FS_ERR(0xed)
#define FS_DIRALREADY			RSC_FS_ERR(0xec)
#define FS_CANTDELETE			RSC_FS_ERR(0xeb)
#define FS_CANTRENAME			RSC_FS_ERR(0xea)
#define FS_DISKFULL				RSC_FS_ERR(0xe9)
#define FS_DIRFULL				RSC_FS_ERR(0xe8)
#define FS_CANTSEEK				RSC_FS_ERR(0xe7)
#define FS_SUPERVISOR			RSC_FS_ERR(0xe6)
#define FS_THREADNAME			RSC_FS_ERR(0xe5)
#define FS_BUFWRITE				RSC_FS_ERR(0xe4)
#define FS_BACKGROUND			RSC_FS_ERR(0xe3)
#define FS_OUTOFLOCK			RSC_FS_ERR(0xe0)
#define FS_LOCKED				RSC_FS_ERR(0xdf)
#define FS_DRIVEOPENED			RSC_FS_ERR(0xde)
#define FS_LINKOVER				RSC_FS_ERR(0xdd)
#define FS_FILEEXIST			RSC_FS_ERR(0xb0)

#define FS_FATAL_MEDIAOFFLINE	RSC_FS_ERR(0xa3)
#define FS_FATAL_WRITEPROTECT	RSC_FS_ERR(0xa2)
#define FS_FATAL_INVALIDCOMMAND	RSC_FS_ERR(0xa1)
#define FS_FATAL_INVALIDUNIT	RSC_FS_ERR(0xa0)

#define RASDRV_ISERROR(r)  ((RSC_U8)((RSC_U32)(r)>>24))
#define RASDRV_ISERR(r)  RASDRV_ISERROR(r)

enum h68_attribute_t {
	AT_READONLY		= 0x01,
	AT_HIDDEN		= 0x02,
	AT_SYSTEM		= 0x04,
	AT_VOLUME		= 0x08,
	AT_DIRECTORY	= 0x10,
	AT_ARCHIVE		= 0x20,
	AT_ALL			= 0xff
};

enum h68_open_t {
	OP_READ			= 0,
	OP_WRITE		= 1,
	OP_FULL			= 2,
	OP_MASK			= 0x0f,
	OP_SHARE_NONE	= 0x10,
	OP_SHARE_READ	= 0x20,
	OP_SHARE_WRITE	= 0x30,
	OP_SHARE_FULL	= 0x40,
	OP_SHARE_MASK	= 0x70,
	OP_SPECIAL		= 0x100,
};

enum h68_seek_t {
	SK_BEGIN		= 0,
	SK_CURRENT		= 1,
	SK_END			= 2,
};

enum h68_media_t {
	MEDIA_2DD_10	= 0xe0,
	MEDIA_1D_9		= 0xe5,
	MEDIA_2D_9		= 0xe6,
	MEDIA_1D_8		= 0xe7,
	MEDIA_2D_8		= 0xe8,
	MEDIA_2HT		= 0xea,
	MEDIA_2HS		= 0xeb,
	MEDIA_2HDE		= 0xec,
	MEDIA_1DD_9		= 0xee,
	MEDIA_1DD_8		= 0xef,
	MEDIA_MSDOS_REMOVABLE = 0xf0,
	MEDIA_MANUAL	= 0xf1,
	MEDIA_REMOVABLE	= 0xf2,
	MEDIA_REMOTE	= 0xf3,
	MEDIA_DAT		= 0xf4,
	MEDIA_CDROM		= 0xf5,
	MEDIA_MO		= 0xf6,
	MEDIA_SCSI_HD	= 0xf7,
	MEDIA_SASI_HD	= 0xf8,
	MEDIA_RAMDISK	= 0xf9,
	MEDIA_2HQ		= 0xfa,
	MEDIA_2DD_8		= 0xfb,
	MEDIA_2DD_9		= 0xfc,
	MEDIA_2HC		= 0xfd,
	MEDIA_2HD		= 0xfe,
};



/*
---------------------------------------
internal structs for rasdrv.c
---------------------------------------
*/

struct h68_namests_t {
	RSC_U8 wildcard;
	RSC_U8 drive;
	RSC_U8 path[65];
	RSC_U8 name[8];
	RSC_U8 ext[3];
	RSC_U8 add[10];
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_namests_t  h68_namests_t;


struct h68_files_t {
	RSC_U8 fatr;
	RSC_U8 pad1[3];
	RSC_BE32 sector;
	RSC_BE16 offset;
	RSC_U8 attr;
	RSC_U8 pad2;
	RSC_BE16 time;
	RSC_BE16 date;
	RSC_BE32 size;
	RSC_U8 full[23];
	RSC_U8 pad3;
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_files_t  h68_files_t;


struct h68_fcb_t {
	RSC_BE32 fileptr;
	RSC_BE16 mode;
	RSC_U8 attr;
	RSC_U8 pad;
	RSC_BE16 time;
	RSC_BE16 date;
	RSC_BE32 size;
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_fcb_t  h68_fcb_t;


struct h68_capacity_t {
	RSC_BE16 freearea;
	RSC_BE16 clusters;
	RSC_BE16 sectors;
	RSC_BE16 bytes;
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_capacity_t  h68_capacity_t;


struct h68_ctrldrive_t {
	RSC_U8 status;
	RSC_U8 pad[3];
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_ctrldrive_t  h68_ctrldrive_t;


struct h68_dpb_t {
	RSC_BE16 sector_size;
	RSC_U8 cluster_size;
	RSC_U8 shift;
	RSC_BE16 fat_sector;
	RSC_U8 fat_max;
	RSC_U8 fat_size;
	RSC_BE16 file_max;
	RSC_BE16 data_sector;
	RSC_BE16 cluster_max;
	RSC_BE16 root_sector;
	RSC_U8 media;
	RSC_U8 pad;
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_dpb_t  h68_dpb_t;


struct h68_dirent_t {
	RSC_U8 name[8];
	RSC_U8 ext[3];
	RSC_U8 attr;
	RSC_U8 add[10];
	RSC_BE16 time;
	RSC_BE16 date;
	RSC_BE16 cluster;
	RSC_BE32 size;
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_dirent_t  h68_dirent_t;


union h68_ioctrl_t {
	RSC_U8 buffer[8];
	RSC_BE32 param;
	RSC_BE16 media;
} GCC_ATTRIBUTE_PACK1;
typedef union h68_ioctrl_t  h68_ioctrl_t;


struct h68_argument_t {
	RSC_U8 buf[256];
} GCC_ATTRIBUTE_PACK1;
typedef struct h68_argument_t  h68_argument_t;



struct rasdrv_pkt_files {
	RSC_BE32		unit;
	RSC_BE32		nkey;
	h68_namests_t	namests;
	h68_files_t		files;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_files  rasdrv_pkt_files;

struct rasdrv_pkt_nfiles {
	RSC_BE32		unit;
	RSC_BE32		nkey;
	h68_files_t		files;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_nfiles  rasdrv_pkt_nfiles;

struct rasdrv_pkt_dir {
	RSC_BE32		unit;
	h68_namests_t	namests;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_dir  rasdrv_pkt_dir;

struct rasdrv_pkt_rename {
	RSC_BE32		unit;
	h68_namests_t	namests;
	h68_namests_t	namests_new;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_rename  rasdrv_pkt_rename;

struct rasdrv_pkt_attribute {
	RSC_BE32		unit;
	h68_namests_t	namests;
	RSC_BE32		attr;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_attribute  rasdrv_pkt_attribute;

#define rasdrv_pkt_entry_open \
	RSC_BE32		unit; \
	RSC_BE32		key; \
	h68_namests_t	namests; \
	h68_fcb_t		fcb;

struct rasdrv_pkt_create {
	rasdrv_pkt_entry_open
	RSC_BE32		attr;
	RSC_BE32		force;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_create  rasdrv_pkt_create;
typedef rasdrv_pkt_create  rasdrv_pkt_creat;

struct rasdrv_pkt_open {
	rasdrv_pkt_entry_open
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_open  rasdrv_pkt_open;

struct rasdrv_pkt_close {
	RSC_BE32		unit;
	RSC_BE32		key;
	h68_fcb_t		fcb;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_close  rasdrv_pkt_close;

struct rasdrv_pkt_rw {
	RSC_BE32		key;
	h68_fcb_t		fcb;
	RSC_BE32		length;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_rw  rasdrv_pkt_rw;

struct rasdrv_pkt_seek {
	RSC_BE32		key;
	h68_fcb_t		fcb;
	RSC_BE32		whence;
	RSC_BE32		offset;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_seek  rasdrv_pkt_seek;

struct rasdrv_pkt_timestamp {
	RSC_BE32		unit;
	RSC_BE32		key;
	h68_fcb_t		fcb;
	RSC_BE16		date;
	RSC_BE16		time;
} GCC_ATTRIBUTE_PACK1;
typedef struct rasdrv_pkt_timestamp  rasdrv_pkt_timestamp;


/*
---------------------------------------
*/

typedef SRB_ExecSCSICmd10	rasdrv_t; /* ASPI request block */

extern rasdrv_t *h_rasdrv;
extern void *rasdrv_pkt_buf;
extern void *rasdrv_tfr_buf;
extern size_t rasdrv_tfr_buflen;

#define RASDRV_HANDLE_MAX 20

enum {
	RASDRV_HANDLE_EMPTY = 0,
	RASDRV_HANDLE_FCB = 1,
	RASDRV_HANDLE_FILES = 2,
	RASDRV_HANDLE_RESERVED = 0xff
};

void  rasdrv_mem_init(void);
int find_rasdrv(int check_id);


void  rasdrv_handle_init(void);
int  rasdrv_handle_new(int type);
int  rasdrv_handle_delete(int hdl);
h68_fcb_t *  rasdrv_handle_fcb(int hdl);
h68_files_t *  rasdrv_handle_files(int hdl);

char *  rasdrv_filenameptr(const char *s_top);


/*
---------------------------------------
Fs functions
---------------------------------------
*/

RASDRV_RESULT  rasdrvFsInitDevice(rasdrv_t *rd, const void *p);
RASDRV_RESULT  rasdrvFsFiles(rasdrv_t *rd, RSC_U32 unit, RSC_U32 nkey, const h68_namests_t *n, h68_files_t *f);
RASDRV_RESULT  rasdrvFsNfiles(rasdrv_t *rd, RSC_U32 unit, RSC_U32 nkey, h68_files_t *f);
RASDRV_RESULT  rasdrvFsCheckDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns);
RASDRV_RESULT  rasdrvFsMakeDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns);
RASDRV_RESULT  rasdrvFsRemoveDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns);
RASDRV_RESULT  rasdrvFsCreate(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const h68_namests_t *ns, int do_force, unsigned attr);
RASDRV_RESULT  rasdrvFsOpen(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const h68_namests_t *ns);
RASDRV_RESULT  rasdrvFsDelete(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns);
RASDRV_RESULT  rasdrvFsRename(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns, const h68_namests_t *ns_new);
RASDRV_RESULT  rasdrvFsAttribute(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns, unsigned char attr_set);
#define rasdrvFsAttributeGet(rd,u,n)  rasdrvFsAttribute(rd,u,n,0xffU)
RASDRV_RESULT  rasdrvFsClose(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb);
RASDRV_RESULT  rasdrvFsRead(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, void *mem, size_t length);
RASDRV_RESULT  rasdrvFsWrite(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const void *mem, size_t length);
RASDRV_RESULT  rasdrvFsSeek(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, long offset, int whence);
RASDRV_RESULT  rasdrvFsTimeStamp(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, unsigned dosdate, unsigned dostime);


RASDRV_RESULT  rasdrvFsCheckDir2(rasdrv_t *rd, RSC_U32 unit, const char *path);
RASDRV_RESULT  rasdrvFsMakeDir2(rasdrv_t *rd, RSC_U32 unit, const char *path);
RASDRV_RESULT  rasdrvFsRemoveDir2(rasdrv_t *rd, RSC_U32 unit, const char *path);
RASDRV_RESULT  rasdrvFsCreate2(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const char *pathname, int do_force, unsigned attr);
RASDRV_RESULT  rasdrvFsOpen2(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const char *pathname);
RASDRV_RESULT  rasdrvFsRename2(rasdrv_t *rd, RSC_U32 unit, const char *name, const char *name_new);
RASDRV_RESULT  rasdrvFsDelete2(rasdrv_t *rd, RSC_U32 unit, const char *pathname);
RASDRV_RESULT  rasdrvFsAttribute2(rasdrv_t *rd, RSC_U32 unit, const char *pathname, unsigned char attr_set);
#define rasdrvFsAttributeGet2(rd,u,n)  rasdrvFsAttribute2(rd,u,n,0xffU)





#ifdef RSC_PACKED_PRAGMA
#pragma pack()
#endif

#ifdef __cplusplus
}
#endif

#endif
