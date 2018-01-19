/*
aspidos.c: an ASPI (DOS version) wrapper of my own


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


#ifndef MY_ASPI_DOS16_H
#define MY_ASPI_DOS16_H


#ifndef SENSE_LEN
# define SENSE_LEN 14
#endif

#ifndef ASPI_SENSE_LEN_MAX
# if SENSE_LEN > 16
#  define ASPI_SENSE_LEN_MAX  SENSE_LEN
# else
#  define ASPI_SENSE_LEN_MAX  18
# endif
#endif

#ifndef SC_HA_INQUIRY
# define SC_HA_INQUIRY 0
# define SC_GET_DEV_TYPE 1
# define SC_EXEC_SCSI_CMD 2
# define SC_ABORT_SRB 3
# define SC_RESET_DEV 4
# define SC_SET_HA_PARMS 5
# define SC_GET_DISK_INFO 6
#endif /* SC_xxx */


#ifndef SRB_DIR_SCSI
# define SRB_POSTING  0x01
# define SRB_ENABLE_LINKING 0x02
# define SRB_ENABLE_RESIDUAL_COUNT 0x04
# define SRB_DIR_SCSI 0    /* bit4,3 */
# define SRB_DIR_AND  0x18
# define SRB_DIR_MASK 0xe7
# define SRB_DIR_IN   0x08
# define SRB_DIR_OUT  0x10
#endif /* SRB_xxx */

#define RESIDUAL_COUNT_SUPPORTED 0x02



#ifndef SS_COMP
# define SS_PENDING 0x00
# define SS_COMP 0x01
# define SS_ABORTED 0x02
# define SS_ABORT_FAIL 0x03
# define SS_ERR 0x04
# define SS_INVALID_CMD 0x80
# define SS_INVALID_HA 0x81
# define SS_NO_DEVICE 0x82
# define SS_INVALID_SRB 0xe0
# define SS_OLD_MANAGER 0xe1
# define SS_ILLEGAL_MODE 0xe2
# define SS_NO_ASPI 0xe3
# define SS_FAILED_INIT 0xe4
# define SS_ASPI_IS_BUSY 0xe5
# define SS_BUFFER_TOO_BIG 0xe6
# define SS_BUFFER_TO_BIG SS_BUFFER_TOO_BIG
#endif /* SS_xxx */


#ifndef HASTAT_OK
# define HASTAT_OK 0
# define HASTAT_TIMEOUT 9
# define HASTAT_COMMAND_TIMEOUT 0xb
# define HASTAT_MESSAGE_REJECT 0xd
# define HASTAT_BUS_RESET 0xe
# define HASTAT_PARITY_ERROR 0xf
# define HASTAT_REQUEST_SENSE_FAILED 0x10
# define HASTAT_SEL_TO 0x11
# define HASTAT_DO_DU 0x12
# define HASTAT_BUS_FREE 0x13
# define HASTAT_PHASE_ERR 0x14
#endif /* HASTAT_xxx */


#if !(defined(__TURBOC__) || defined(LSI_C))
#pragma pack(1)
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define SRB_COMMAND_BASE_ENTRIES_0 \
	unsigned char SRB_Cmd; \
	volatile unsigned char SRB_Status; \
	unsigned char SRB_HaId; \
	unsigned char SRB_Flags;

#define SRB_COMMAND_BASE_ENTRIES \
	SRB_COMMAND_BASE_ENTRIES_0 \
	unsigned long SRB_Hdr_Rsvd;


struct SRB_Header_d16 {
	SRB_COMMAND_BASE_ENTRIES
};

struct SRB_HAInquiry_d16 {
	SRB_COMMAND_BASE_ENTRIES_0
	unsigned short SRB_55AASignature;
	unsigned short SRB_ExtBufferSize;
	unsigned char HA_Count;
	unsigned char HA_SCSI_ID;
	unsigned char HA_ManagerId[16];
	unsigned char HA_Identifier[16];
	unsigned char HA_Unique[16];
	unsigned char HA_ExtBuffer[8]; /* HA_ExtBuffer[2] */
};

struct SRB_GDEVBlock_d16 {
	SRB_COMMAND_BASE_ENTRIES
	unsigned char SRB_Target;
	unsigned char SRB_Lun;
	unsigned char SRB_DeviceType;
	unsigned char SRB_Rsvd1;	/* padding for alignment (not required for DOS) */
};


#define SRB_EXEC_SCSI_CMD_BASE_ENTRIES \
	SRB_COMMAND_BASE_ENTRIES \
	unsigned char SRB_Target; \
	unsigned char SRB_Lun; \
	unsigned long SRB_BufLen; \
	unsigned char SRB_SenseLen; \
	void far *SRB_BufPointer; \
	void far *SRB_LinkPointer; \
	unsigned char SRB_CDBLen; \
	unsigned char SRB_HaStat; \
	unsigned char SRB_TargStat; \
	void (far *SRB_PostProc)(void); \
	unsigned char SRB_Rsvd2[34];


struct SRB_ExecSCSICmd_d16 {
	SRB_EXEC_SCSI_CMD_BASE_ENTRIES
	unsigned char CDBByte[32];
};

struct SRB_ExecSCSICmd6_d16 {
	SRB_EXEC_SCSI_CMD_BASE_ENTRIES
	unsigned char CDBByte[6];
	unsigned char SenseArea6[ASPI_SENSE_LEN_MAX];
};
struct SRB_ExecSCSICmd10_d16 {
	SRB_EXEC_SCSI_CMD_BASE_ENTRIES
	unsigned char CDBByte[10];
	unsigned char SenseArea10[ASPI_SENSE_LEN_MAX];
};


typedef struct SRB_Header_d16  SRB_Header;
typedef struct SRB_HAInquiry_d16  SRB_HAInquiry;
typedef struct SRB_GDEVBlock_d16  SRB_GDEVBlock;
typedef struct SRB_ExecSCSICmd_d16  SRB_ExecSCSICmd;
typedef struct SRB_ExecSCSICmd6_d16  SRB_ExecSCSICmd6;
typedef struct SRB_ExecSCSICmd10_d16  SRB_ExecSCSICmd10;



int aspiInit_d16(void);
int aspiExit_d16(void);
int SendASPICommand_d16(void far *);
unsigned GetASPISupportInfo_d16(void);

int aspiSendAndPoll_d16(void *);
unsigned char *aspiLastRequestData_d16(void);


#ifndef BUILD_MYASPI_d16
# define aspiInit  aspiInit_d16
# define aspiExit  aspiExit_d16
# define SendASPICommand  SendASPICommand_d16
# define GetASPISupportInfo  GetASPISupportInfo_d16
# define aspiSendAndPoll  aspiSendAndPoll_d16
# define aspiLastRequestData  aspiLastRequestData_d16
#endif


/* helper */

int aspiInquiry_d16(int, int, int, void *, unsigned);
int aspiModeSense_d16(int, int, int, unsigned char, void *, unsigned);

#ifndef BUILD_MYASPI_d16
# define aspiInquiry aspiInquiry_d16
# define aspiModeSense aspiModeSense_d16
#endif

void *  alloc_nobound64k(size_t);


#ifdef __cplusplus
}
#endif

#if !(defined(__TURBOC__) || defined(LSI_C))
#pragma pack()
#endif

#endif /* MY_ASPI_DOS16_H */
