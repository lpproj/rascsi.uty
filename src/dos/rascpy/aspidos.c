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

#include <ctype.h>
#include <dos.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILD_MYASPI_d16
#include "aspi_d16.h"

#define mybzero(p,n) memset(p,0,n)
#define strbzero(p) memset(&(p), 0, sizeof(p))
#define mymin(a,b)  ((a)<(b)?(a):(b))
#define mymax(a,b)  ((a)>(b)?(a):(b))


static char aspi_devdos[] = "SCSIMGR$";
#ifdef LSI_C
static void (far *aspi_dos_entry)(int, int, int, int, void far *);
#else
static void (cdecl far *aspi_dos_entry)(void far *);
#endif
static void (*aspi_dos_yield)(void);

static unsigned char aspi_last_request_data[ASPI_SENSE_LEN_MAX];


/* helper (memory) */

static size_t  is_fp_bound64k(const void far *p, size_t len)
{
	unsigned long la, la2;
	
	la = (16UL * (unsigned short)FP_SEG(p)) + (unsigned short)FP_OFF(p);
	la2 = (la + len) & 0xffff0000UL;
	return ((unsigned)(la >> 16) == (unsigned)(la2 >> 16))
			? 0
			: (size_t)(la2 - la);
}

void *
alloc_nobound64k(size_t length)
{
	int b_success = 0;
	void *p;
	unsigned long la;
	
	if (length == 0) return NULL;
	p = malloc(length);
	if (p) {
		size_t len2 = is_fp_bound64k(p, length);
		if (len2 == 0) {
			b_success = 1;
		}
		else {
			/* block "bounded" mem-blk
			   (hope the pointer is not moved when shrinking the block) */
			void *p2 = realloc(p, len2);
			if (p == p2) {
				p = malloc(length);
				if (p && is_fp_bound64k(p, length) == 0) {
					free(p2);
					b_success = 1;
				}
			}
		}
	}
	if (b_success) {
		if (p && length > 0) memset(p, 0, length);
	}
	else {
		fprintf(stderr, "fatal: bound64k allocation failed.\n");
		exit(1);
	}
	
#if 0
	printf("malloc %Fp", (void far *)p);
	printf(" LA=%08lX\n", (unsigned long)FP_SEG(p) * 16U + FP_OFF(p));
#endif
	return p;
}


/* aspi functions */


int aspiInit_d16(void)
{
	union REGS r;
	struct SREGS sr;
	int hdl = -1;
	unsigned rc = SS_NO_ASPI;
	
	if (aspi_dos_entry) return 0;
	
	r.x.ax = 0x3d00; /* open file */
	sr.ds = FP_SEG(aspi_devdos);
	r.x.dx = FP_OFF(aspi_devdos);
	intdos(&r, &r);
	if (r.x.cflag) return rc;
	hdl = r.x.ax;

	r.x.ax = 0x4400; /* get device info. */
	r.x.bx = hdl;
	intdos(&r, &r);
	if (!r.x.cflag) {
		if ((r.x.dx & 0x4080) == 0x4080) { /* char dev with IOCTL ? */
			r.x.ax = 0x4402; /* IOCTL read */
			r.x.bx = hdl;
			r.x.cx = 4;
			sr.ds = FP_SEG(&aspi_dos_entry);
			r.x.dx = FP_OFF(&aspi_dos_entry);
			intdosx(&r, &r, &sr);
			if (!r.x.cflag) {
				rc = (r.x.ax == 4) ? 0 : SS_FAILED_INIT;
			}
		}
	}
	r.h.ah = 0x3e; /* close handle */
	r.x.bx = hdl;
	intdos(&r, &r);
	
	return rc;
}


int
aspiExit_d16(void)
{
	aspi_dos_entry = 0L;
	return 0;
}


int SendASPICommand_d16(void far *p)
{
	if (!aspi_dos_entry) return -1;

#ifdef LSI_C
	(*aspi_dos_entry)(0, 0, 0, 0, p);
#else
	(*aspi_dos_entry)(p);
#endif
	return ((struct SRB_Header_d16 far *)p)->SRB_Status;
}


unsigned GetASPISupportInfo_d16(void)
{
	int rc;
	
	rc = aspiInit_d16();
	if (rc != 0) 
		return (unsigned)rc << 8;
	
	rc = (SS_COMP << 8) | 1; /* dummy */
	return rc;
}



int aspiSendAndPoll_d16(void *p)
{
	int rc = SendASPICommand_d16(p);
	
	while(rc == 0) {
		if (aspi_dos_yield) (*aspi_dos_yield)();
		rc = ((struct SRB_Header_d16 far *)p)->SRB_Status;
	}
	if (rc != SS_COMP && rc != -1) {
		SRB_ExecSCSICmd *psc = p;
		unsigned clen = psc->SRB_CDBLen;
		if (psc->SRB_Cmd == SC_EXEC_SCSI_CMD && 
		    (clen == 6 || clen == 10 || clen == 12) &&
		    (psc->CDBByte[clen] & 0x70) == 0x70 && (psc->CDBByte[clen + 2] & 0x0f) != 0
		   )
		{
			unsigned cpylen = psc->SRB_SenseLen;
			if (cpylen > ASPI_SENSE_LEN_MAX) cpylen = ASPI_SENSE_LEN_MAX;
			memcpy(aspi_last_request_data, psc->CDBByte + clen, cpylen);
		}
	}

	return rc;
}

unsigned char *aspiLastRequestData_d16(void)
{
	return aspi_last_request_data;
}


/* --------------------------------------------------------- */


#define SendASPICommand  SendASPICommand_d16
#define GetASPISupportInfo  GetASPISupportInfo_d16
#define aspiSendAndPoll  aspiSendAndPoll_d16

static void setCDB16(unsigned char *b, unsigned v)
{
	b[0] = (unsigned char)(v >> 8);
	b[1] = (unsigned char)(v);
}
static void setCDB24(unsigned char *b, unsigned long v)
{
	b[0] = (unsigned char)(v >> 16);
	b[1] = (unsigned char)(v >> 8);
	b[2] = (unsigned char)(v);
}
static void setCDB32(unsigned char *b, unsigned long v)
{
	b[0] = (unsigned char)(v >> 24);
	b[1] = (unsigned char)(v >> 16);
	b[2] = (unsigned char)(v >> 8);
	b[3] = (unsigned char)(v);
}

static void setupSRB_Exec(SRB_ExecSCSICmd *sc, int host, int id, int lun, int flags, void *buffer, unsigned buffer_len)
{
	strbzero(*sc);
	sc->SRB_Cmd = SC_EXEC_SCSI_CMD;
	sc->SRB_HaId = host;
	sc->SRB_Flags = flags;
	sc->SRB_Target = id;
	sc->SRB_Lun = lun;
	sc->SRB_BufLen = buffer_len;
	sc->SRB_SenseLen = SENSE_LEN;
	sc->SRB_BufPointer = buffer;
	if (lun >= 0 && lun <= 7) sc->CDBByte[1] = (lun & 7) << 5;
}

static void setupSRB_Exec6(SRB_ExecSCSICmd *sc, int host, int id, int lun, int flags, void *buffer, unsigned buffer_len)
{
	setupSRB_Exec(sc, host, id, lun, flags, buffer, buffer_len);
	sc->SRB_CDBLen = 6;
	sc->CDBByte[4] = (unsigned char)mymin(buffer_len, 255U);
}

static void setupSRB_Exec10(SRB_ExecSCSICmd *sc, int host, int id, int lun, int flags, void *buffer, unsigned buffer_len)
{
	setupSRB_Exec(sc, host, id, lun, flags, buffer, buffer_len);
	sc->SRB_CDBLen = 10;
	setCDB16(sc->CDBByte + 7, buffer_len);
}



int aspiInquiry_d16(int host, int id, int lun, void *buffer, unsigned buffer_len)
{
	SRB_ExecSCSICmd sc;
	int rc;
	
	setupSRB_Exec6(&sc, host, id, lun, SRB_DIR_IN, buffer, buffer_len);
	sc.CDBByte[0] = 0x12;
	
	rc = aspiSendAndPoll(&sc);
	
	return rc;
}


int aspiModeSense_d16(int host, int id, int lun, unsigned char page, void *buffer, unsigned buffer_len)
{
	SRB_ExecSCSICmd sc;
	int rc;

	if (buffer_len <= 255) {
		setupSRB_Exec6(&sc, host, id, lun, SRB_DIR_IN, buffer, buffer_len);
		sc.CDBByte[0] = 0x1a;
	} else {
		setupSRB_Exec10(&sc, host, id, lun, SRB_DIR_IN, buffer, buffer_len);
		sc.CDBByte[0] = 0x5a;
	}
	sc.CDBByte[2] = page;

	rc = aspiSendAndPoll(&sc);
	
	return rc;
}



/* --------------------------------------------------------- */

#ifdef TEST

#define aspiModeSense aspiModeSense_d16
#define aspiInquiry aspiInquiry_d16


void dumpmem(const void *mem, unsigned len)
{
	const unsigned char *p = mem;
	unsigned n, cnt;
	unsigned char c, s[32];
	const unsigned CNT_LINE = 16;
	
	if (len <= 0) return;
	
	for(n=0, cnt=0; n < len; ++n) {
		if (cnt == 0) {
			printf("%04X", n);
		}
		c = p[n];
		printf(" %02X", c);
		if (c < ' ' || c > 0x7e) c = '.';
		s[cnt++] = c;
		if (cnt >= CNT_LINE) {
			s[cnt] = '\0';
			printf("  %s\n", s);
			cnt = 0;
		}
	}
	
}

static unsigned mystou(const char *s)
{
	long v = strtol(s, NULL, 0);
	
	return (v > UINT_MAX || v < 0) ? UINT_MAX : (unsigned)(unsigned long)v;
}


int main(int argc, char *argv[])
{
	int optH = 0;
	unsigned optM = 128;
	SRB_HAInquiry shi;
	int rc;
	int i, l;
	char s[32];
	char *data;
	unsigned data_len = 0x4000;
	
	while(argc > 1) {
		char *s;
		--argc;
		s = *++argv;
		if (*s == '-' || *s == '/') {
			++s;
			switch(*s++) {
				case '?': case 'H': case 'h': optH=1; break;
				case 'M': case 'm': optM = mystou(s); break;
			}
		}
	}
	
	if (data_len < optM) data_len = optM;
	
	rc = GetASPISupportInfo_d16();
	if (((unsigned)rc >> 8) != SS_COMP) {
		printf("ASPI driver/device not available. (rc=0x%X)\n", rc);
		return 1;
	}
	
	data = malloc(data_len);
	if (!data) {
		printf("memory allocation failure.\n");
		return 1;
	}
	
	mybzero(s, sizeof(s));
	strbzero(shi);
	rc = SendASPICommand(&shi);
	if (rc != 1) {
		printf("Host Adapter Inquiry status=0x%X\n", rc);
		return 1;
	}
	printf("Host Adapter No.        %u\n", shi.SRB_HaId);
	printf("Number of Host Adapters %u\n", shi.HA_Count);
	printf("SCSI ID of the Host     %u\n", shi.HA_SCSI_ID);
	memcpy(s, shi.HA_ManagerId, sizeof(shi.HA_ManagerId));
	printf("SCSI Manager ID         %s\n", s);
	memcpy(s, shi.HA_Identifier, sizeof(shi.HA_Identifier));
	printf("HOST Adapter ID         %s\n", s);
	memcpy(s, shi.HA_Unique, sizeof(shi.HA_Unique));
	printf("HOST Unique Parameter   %s\n", s);
	
	for(i=0; i<8; ++i) {
		SRB_GDEVBlock sd;
		if (i == shi.HA_SCSI_ID) continue;
		l = 0;
		strbzero(sd);
		sd.SRB_Cmd = 1;
		sd.SRB_HaId = shi.SRB_HaId;
		sd.SRB_Target = i;
		sd.SRB_Lun = l;
		printf(" ID=%u LUN=%u ... ", sd.SRB_Target, sd.SRB_Lun);
		rc = aspiSendAndPoll(&sd);
		printf("rc=0x%02X", rc);
		if (rc == 1) {
			char inq[36];
#if 1
			printf(" device type=0x%02X\n", sd.SRB_DeviceType);
			printf("  ");
			rc = aspiInquiry(sd.SRB_HaId, sd.SRB_Target, sd.SRB_Lun, inq, sizeof(inq));
#else
			SRB_ExecSCSICmd6 sc;
			printf(" device type=0x%02X\n", sd.SRB_DeviceType);
			printf("  ");
			strbzero(sc);
			sc.SRB_Cmd = 2;
			sc.SRB_HaId = sd.SRB_HaId;
			sc.SRB_Flags = SRB_DIR_IN;
			sc.SRB_Target = sd.SRB_Target;
			sc.SRB_Lun = sd.SRB_Lun;
			sc.SRB_SenseLen = SENSE_LEN;
			sc.SRB_BufPointer = inq;
			sc.SRB_BufLen = sizeof(inq);
			sc.SRB_CDBLen = 6;
			sc.CDBByte[0] = 0x12; /* INQUIRY */
			sc.CDBByte[1] = ((sd.SRB_Lun & 7) << 5);
			sc.CDBByte[4] = sizeof(inq);
			rc = aspiSendAndPoll(&sc);
#endif
			if (rc == 1) {
				int n;
				for(n=0; n<8; ++n) printf(" %02X", (unsigned char)(inq[n]));
				printf("\n   ");
				printf("\"");
				for(n=8; n<36; ++n) printf("%c", inq[n]);
				printf("\"");
				
				memset(data, 0xff, data_len);
				rc = aspiModeSense(sd.SRB_HaId, sd.SRB_Target, sd.SRB_Lun, 0x3f, data, optM);
				printf("\n mode sense rc=0x%X", rc);
				if (rc == 1) {
					printf("\n");
					dumpmem(data, optM);
				}
			} else {
				printf("INQUIRY failure (rc=0x%X)", rc);
			}
		}
		printf("\n");
	}
	
	
	return 0;
}


#endif
