/*
bitscsi - a minimal SCSI wrapper

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

#include <ctype.h>
#include <dos.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILD_BITSCSI
#include "bitscsi.h"

#ifndef offsetof
# define offsetof(typ,id) ((unsigned)(&((typ *)0)->(id)) - (unsigned)&(typ *)0)
#endif

#if defined LSI_C
# define BS_ASPI_PREARGS(a) a,a,a,a,
# define BS_ASPI_DECLSPEC   far
#else
# define BS_ASPI_PREARGS(a)
# define BS_ASPI_DECLSPEC   cdecl far
#endif
typedef void ( BS_ASPI_DECLSPEC *ASPIPROC)( BS_ASPI_PREARGS(int) void far *);
#define call_aspi(addr,param)  (*(addr))( BS_ASPI_PREARGS(0) param)


#if !(defined __TURBOC__ || defined LSI_C)
# pragma pack(1)
#endif

#define SRB_COMMAND_BASE_ENTRIES_0 \
    BSu8 SRB_Cmd; \
    volatile BSu8 SRB_Status; \
    BSu8 SRB_HaId; \
    BSu8 SRB_Flags;

#define SRB_COMMAND_BASE_ENTRIES \
    SRB_COMMAND_BASE_ENTRIES_0 \
    BSu32 SRB_Hdr_Rsvd;

#define SRB_COMMAND_BASE_ENTRIES_1 \
    SRB_COMMAND_BASE_ENTRIES \
    BSu8 SRB_Target; \
    BSu8 SRB_Lun;

#define SRB_EXEC_SCSI_CMD_BASE_ENTRIES \
    SRB_COMMAND_BASE_ENTRIES_1 \
    BSu32 SRB_BufLen; \
    BSu8 SRB_SenseLen; \
    void far *SRB_BufPointer; \
    void far *SRB_LinkPointer; \
    BSu8 SRB_CDBLen; \
    BSu8 SRB_HaStat; \
    BSu8 SRB_TargStat; \
    void (far *SRB_PostProc)(void); \
    BSu8 SRB_Rsvd2[34];


struct bs_packed_pre SRB_GetDeviceType {
    SRB_COMMAND_BASE_ENTRIES_1
    BSu8 SRB_DevType;
} bs_packed_post;
typedef struct SRB_GetDeviceType  SRB_GetDeviceType;

struct bs_packed_pre SRB_HAInquiry {
    SRB_COMMAND_BASE_ENTRIES_0
    BSu16 SRB_55AASignature;
    BSu16 SRB_ExtBufferSize;
    BSu8 HA_Count;
    BSu8 HA_SCSI_ID;
    char HA_ManagerId[16];
    char HA_Identifier[16];
    char HA_Unique[16];
    BSu8 HA_ExtBuffer[2];
    BSu8 reserved_3C[4];
} bs_packed_post;
typedef struct SRB_HAInquiry  SRB_HAInquiry;

struct bs_packed_pre SRB_ExecSCSICmd {
    SRB_EXEC_SCSI_CMD_BASE_ENTRIES
    BSu8 CDB[16 + BS_REQUEST_SENSE_MAX];
} bs_packed_post;
typedef struct SRB_ExecSCSICmd  SRB_ExecSCSICmd;

union ASPI_RequestBlock {
    struct SRB_HAInquiry        hainq;
    struct SRB_GetDeviceType    dtype;
    struct SRB_ExecSCSICmd      cmd;
};

#if !(defined __TURBOC__ || defined LSI_C)
# pragma pack()
#endif


void bs_w16be(void *m, BSu16 v)
{
    unsigned char *s = m;
    *s = v >> 8;
    s[1] = v & 0xffU;
}
void bs_w24be(void *m, BSu32 v)
{
    unsigned char *s = m;
    *s = v >> 16;
    s[1] = ((unsigned)v >> 8) & 0xffU;
    s[2] = v & 0xffU;
}
void bs_w32be(void *m, BSu32 v)
{
    unsigned char *s = m;
    *s = v >> 24;
    s[1] = (v >> 16) & 0xffU;
    s[2] = ((unsigned)v >> 8) & 0xffU;
    s[3] = v & 0xffU;
}

BSu16 bs_r16be(const void * const m)
{
    const BSu8 *s = (const BSu8 *)m;
    return ((BSu16)(*s) << 8) | s[1];
}

BSu32 bs_r24be(const void * const m)
{
    const BSu8 *s = (const BSu8 *)m;
    return ((BSu32)(*s) << 16) | ((BSu16)(s[1]) << 8) | s[2];
}

BSu32 bs_r32be(const void * const m)
{
    const BSu8 *s = (const BSu8 *)m;
    return ((BSu32)(*s) << 24) | ((BSu32)(s[1]) << 16) | ((BSu16)(s[2]) << 8) | s[3];
}




/*
    ASPI stuff
*/

static void BS_ASPI_DECLSPEC aspi_stub_entry( BS_ASPI_PREARGS(int) void far *p);

static int mydoserr;
static ASPIPROC aspientry = aspi_stub_entry;
static union ASPI_RequestBlock aspirb;

static ASPIPROC getASPIEntry(void)
{
    static const char aspi_devdos[] = "SCSIMGR$";
    ASPIPROC addr = 0L;
    union REGS r;
    struct SREGS sr;

    mydoserr = 0;
    r.x.ax = 0x3d00;
    r.x.dx = FP_OFF(aspi_devdos);
    sr.ds = FP_SEG(aspi_devdos);
    intdosx(&r, &r, &sr);
    if (r.x.cflag) {
        mydoserr = r.x.ax;
    }
    else {
        unsigned h = r.x.ax;
        r.x.ax = 0x4402;
        r.x.bx = h;
        r.x.cx = sizeof(addr);  /* 4 */
        r.x.dx = FP_OFF(&addr);
        sr.ds = FP_SEG(&addr);
        intdosx(&r, &r, &sr);
        if (r.x.cflag) mydoserr = r.x.ax;
        r.h.ah = 0x3e;
        r.x.bx = h;
        intdosx(&r, &r, &sr);
    }

    return addr;
}

#if defined LSI_C
static void BS_ASPI_DECLSPEC aspi_stub_entry(int d0, int d1, int d2, int d3, void far *p)
#else
static void BS_ASPI_DECLSPEC aspi_stub_entry(void far *p)
#endif
{
    if (bs_init_aspidos() != 0) {
        fprintf(stderr, "FATAL: ASPI driver(s) not found\n");
        exit(-1);
    }
    call_aspi(aspientry, p);
}

static
int CallASPI(void *rb)
{
    BSu8 st;

    if (!aspientry) return -1;
    ((struct SRB_ExecSCSICmd *)rb)->SRB_Status = 0;
    call_aspi(aspientry, rb);
    while ((st = ((struct SRB_ExecSCSICmd *)rb)->SRB_Status) == 0) {
        /* ASPIYield(); */
    }
    return (int)(unsigned)st;
}

int bs_cmd_aspidos(int adapter, int id, int lun, BS_CMDPKT *pkt, int transfer_direction, void *data, unsigned transfer_length)
{
    int rc = -1;
    struct SRB_ExecSCSICmd *cmd = &(aspirb.cmd);
    BSu8 *psense;
    BSu8 flag;

    memset(cmd, 0, sizeof(aspirb.cmd) /* offsetof(struct SRB_ExecSCSICmd, CDB) */);
    cmd->SRB_Cmd = 2;
    cmd->SRB_HaId = adapter;
    switch(transfer_direction) {
        case BS_FROM_DEVICE: flag = 0x08; break;
        case BS_TO_DEVICE: flag = 0x10; break;
        case BS_NO_TRANSFER: flag = 0x18; transfer_length = 0; break;
        default:
            return -1;
    }
    cmd->SRB_Flags = flag;
    cmd->SRB_Target = id;
    cmd->SRB_Lun = lun;
    cmd->SRB_BufLen = transfer_length;
    cmd->SRB_SenseLen = BS_REQUEST_SENSE_MAX;
    if (transfer_length) cmd->SRB_BufPointer = data;
    cmd->SRB_CDBLen = pkt->cdb_length;
    memcpy(cmd->CDB, pkt->cdb, pkt->cdb_length);
    psense = (BSu8 *)&(cmd->CDB[0]) + pkt->cdb_length;
    if (cmd->SRB_SenseLen) memset(psense, 0, cmd->SRB_SenseLen);

    CallASPI(cmd);
    pkt->sense_data[0] = 0;
    if ((pkt->scsi_status = cmd->SRB_TargStat) != 0) {
        pkt->sense_length = cmd->SRB_SenseLen;
        if (pkt->sense_length) memcpy(pkt->sense_data, psense, pkt->sense_length);
    }

    if (cmd->SRB_Status == 1) {
        rc = cmd->SRB_BufLen;
    }
    else {
        rc = -1;
    }
    return rc;
}

int bs_init_aspidos(void)
{
    ASPIPROC a = getASPIEntry();
    SRB_HAInquiry hai;

    if (!a) return -1;
    memset(&hai, 0, sizeof(hai));
    hai.SRB_55AASignature = 0xaa55;
    call_aspi(a, &hai);
    if (hai.HA_Count == 0 || hai.HA_ManagerId[0] == '\0') return -1;
    aspientry = a;

    return 0;
}

static int result_55;

int bs_cmd_55bios(int adapter, int id, int lun, BS_CMDPKT *pkt, int transfer_direction, void *data, unsigned transfer_length)
{
    unsigned char packet55[20];
    unsigned char daua;
    union REGS r;
    struct SREGS sr;
    int do_negate_manualy = 0;

    (void)adapter;
    daua = (id & 0xff0f) | 0xc0;
    packet55[0] = lun;
    switch(transfer_direction) {
        case BS_FROM_DEVICE: packet55[1] = 0x44; break;
        case BS_TO_DEVICE: packet55[1] = 0x48; break;
        case BS_NO_TRANSFER: packet55[1] = 0; transfer_length = 0; break;
        default:
            return -1;
    }
    packet55[2] = pkt->cdb_length;
    packet55[3] = 0;
    memcpy(packet55 + 4, pkt->cdb, 16 /* 12 */);

    r.h.ah = 0x09;
    r.h.al = daua;
    r.x.cx = transfer_length;
    r.x.dx = FP_OFF(packet55);
    sr.ds = FP_SEG(packet55);
    r.x.bx = FP_OFF(data);
    sr.es = FP_SEG(data);
    int86x(0x1b, &r, &r, &sr);

    result_55 = (r.x.ax & 0xff00) | packet55[0];
    do_negate_manualy = ((r.h.ah & 0xf0) != 0 && r.h.ah != 0x22);
    if (r.h.ah == 0x1b || r.h.ah == 0x2b) {
        unsigned char msg_code = 0;
        r.h.ah = 0x1b;  /* TRANSFER STATUS */
        r.h.al = daua;
        r.x.cx = 1;
        r.x.bx = FP_OFF(packet55);
        sr.es = FP_SEG(packet55);
        int86x(0x1b, &r, &r, &sr);
        r.h.ah = 0x1f;  /* TRANSFER MESSAGE IN */
        r.h.al = daua;
        r.x.cx = 1;
        r.x.bx = FP_OFF(msg_code);
        sr.es = FP_SEG(msg_code);
        int86x(0x1b, &r, &r, &sr);
    }
    if (r.x.cflag) result_55 |= 0x00ff;
    if (do_negate_manualy) {
        r.h.ah = 0x03; /* NEGATE ACK */
        r.h.al = daua;
        int86x(0x1b, &r, &r, &sr);
    }

    return r.x.cflag ? -1 : 0;
}



static int machine_type = -1;

int bs_get_machine_dos(void)
{
    if (machine_type < 0) {
        BSu16 biosseg = *(BSu16 far *)MK_FP(0xffff, 3);
        union REGS r;

        if (biosseg == 0xfd80U) machine_type = BS_MACHINE_NEC98;    /* PC-98x1 normal */
#if 0
        if (biosseg == 0xf800U) machine_type = BS_MACHINE_NEC98;    /* PC-98 hi-res? */
#endif
        r.x.ax = 0x0f00;
        r.h.bh = 0xff;
        int86(0x10, &r, &r);
        if (r.h.bh != 0xff && r.h.ah != 0x0f) machine_type = BS_MACHINE_IBMPC;
        /* fallback */
#if 1 /* defined DEFAULT_55BIOS */
        machine_type = BS_MACHINE_NEC98;    /* (workaround for some PC98 emulators like DOSBox-X) */
#else
        machine_type = BS_MACHINE_UNKNOWN;
#endif
    }
    return machine_type;
}

int bs_clear_cmdpkt(BS_CMDPKT *pkt)
{
    int len = sizeof(BS_CMDPKT);
    memset(pkt, 0, len);
    return len;
}

int bs_cmd(int adapter, int id, int lun, BS_CMDPKT *pkt, int transfer_direction, void *data, unsigned transfer_length)
{
    if (((adapter & 0xfff0) == 0xc0 || (id & 0xfff0 == 0xc0)) && bs_get_machine_dos() == BS_MACHINE_NEC98) {
        return bs_cmd_55bios(adapter, (id & 0xff0f), lun, pkt, transfer_direction, data, transfer_length);
    }
    return bs_cmd_aspidos(adapter, id, lun, pkt, transfer_direction, data, transfer_length);
}

int bs_inquiry(int adapter, int id, int lun, void *inq_data, unsigned transfer_length)
{
    int rc;
    BS_CMDPKT pkt;

    bs_clear_cmdpkt(&pkt);
    if (transfer_length > 255) transfer_length = 255;
    pkt.cdb_length = 6;
    pkt.cdb[0] = 0x12;
    pkt.cdb[4] = transfer_length;
    rc = bs_cmd(adapter, id, lun, &pkt, BS_FROM_DEVICE, inq_data, transfer_length);
    return rc;
}

