/*
rasdrv.c: a sample of using RASDRV on DOS


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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aspi_d16.h"
#include "rasdrv.h"

/* typedef SRB_ExecSCSICmd10	rasdrv_t; */

typedef union {
	h68_files_t files;
	h68_fcb_t fcb;
} RASDRV_HANDLE_BODY;

typedef struct {
	int type;
	RASDRV_HANDLE_BODY *handle;
} RASDRV_HANDLE_TABLE;

static RASDRV_HANDLE_TABLE rashdl[RASDRV_HANDLE_MAX];


static unsigned char rasdrv_ha_id = 0;
static unsigned char rasdrv_dev_id = 0xff;
static unsigned char rasdrv_dev_lun = 0;


struct rasdrv_memblk {
	rasdrv_t  sc_rasdrv;
	char  rasdrv_pkt_buf[512];
#if 1
	char  rasdrv_tfr_buf[8192];
#else
	char  rasdrv_tfr_buf[2352];
#endif
};

rasdrv_t *h_rasdrv;
void *rasdrv_pkt_buf;
void *rasdrv_tfr_buf;
size_t rasdrv_tfr_buflen = 0;


void
rasdrv_mem_init(void)
{
	struct rasdrv_memblk *p;
	
	p = alloc_nobound64k(sizeof(struct rasdrv_memblk));
	h_rasdrv = &(p->sc_rasdrv);
	rasdrv_pkt_buf = &(p->rasdrv_pkt_buf);
	rasdrv_tfr_buf = &(p->rasdrv_tfr_buf);
	rasdrv_tfr_buflen = sizeof(p->rasdrv_tfr_buf);
}

void
setup_rasdrv_t(void *p)
{
	rasdrv_t *sc = p;
	memset(sc, 0, sizeof(*sc));
	sc->SRB_Cmd = SC_EXEC_SCSI_CMD;
	sc->SRB_HaId = rasdrv_ha_id;
	sc->SRB_Target = rasdrv_dev_id;
	sc->SRB_Lun = rasdrv_dev_lun;
	sc->SRB_CDBLen = 10;
	sc->CDBByte[1] = (sc->CDBByte[1] & 0x1f) || (rasdrv_dev_lun << 5);
	sc->SRB_SenseLen = SENSE_LEN;
}



int find_rasdrv(int check_id)
{
	int rc_id = -1;
	int id_bottom = 0, id_top = 6;
	int id;
	char inq[36];
	
	if (check_id >= id_bottom && check_id <= id_top) {
		id_bottom = id_top = check_id;
	}
	for(id = id_top; id >= id_bottom; --id) {
		memset(inq, 0, sizeof(inq));
		if (aspiInquiry(rasdrv_ha_id, id, rasdrv_dev_lun, inq, sizeof(inq)) == SS_COMP) {
			if (memcmp(inq + 16, RASCSI_BRIDGE_PRODUCT_ID, 16) == 0) {
				rc_id = rasdrv_dev_id = (unsigned char)id;
				setup_rasdrv_t(h_rasdrv);
				break;
			}
		}
	}
	return rc_id;
}


int
rasdrvSCSIMessage(rasdrv_t *rd, unsigned char cmd, unsigned char devtype, unsigned char funccode, unsigned char phase, void *buffer, unsigned buffer_len)
{
	if (cmd != 0x28 && cmd != 0x2a) return -1;
	
	rd->SRB_Flags = (cmd == 0x2a) ? SRB_DIR_OUT : SRB_DIR_IN;
	rd->SRB_BufPointer = buffer;
	rd->SRB_BufLen = buffer_len;
	
	memset(rd->CDBByte, 0, 10);
	rd->CDBByte[0] = cmd;
	rd->CDBByte[2] = devtype;
	rd->CDBByte[3] = funccode;
#if UINT_MAX > 0xffffU
	rd->CDBByte[6] = (unsigned char)(buffer_len >> 16);
#endif
	rd->CDBByte[7] = (unsigned char)(buffer_len >> 8);
	rd->CDBByte[8] = (unsigned char)buffer_len;
	rd->CDBByte[9] = phase;

	rd->SRB_Status = 0;
	return aspiSendAndPoll(rd);
}


RSC_U16
peekBE16(const RSC_BE16 *r)
{
	return 
			((unsigned)(r->b[0]) << 8) |
			((unsigned)(r->b[1]));
}

void
pokeBE16(RSC_BE16 *r, RSC_U16 v)
{
	r->b[1] = (unsigned char)(v);
	r->b[0] = (unsigned char)(v >> 8);
}

RSC_U32
peekBE32(const RSC_BE32 *r)
{
	return 
			((unsigned long)(r->b[0]) << 24) |
			((unsigned long)(r->b[1]) << 16) |
			((unsigned)(r->b[2]) << 8) |
			((unsigned)(r->b[3]));
}

void
pokeBE32(RSC_BE32 *r, RSC_U32 v)
{
	r->b[3] = (unsigned char)(v);
	r->b[2] = (unsigned char)(v >> 8);
	r->b[1] = (unsigned char)(v >> 16);
	r->b[0] = (unsigned char)(v >> 24);
}





RASDRV_RESULT
rasdrv_FsSendCmd(rasdrv_t *rd, unsigned char funccode, const void *buf, unsigned buf_len)
{
	RSC_BE32	rc_be;
	int rc_aspi;
	
	pokeBE32(&rc_be, (unsigned long)(-1L));
	rc_aspi = rasdrvSCSIMessage(rd, 0x2a, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_SENDCMD, (void *)buf, buf_len);
	if (rc_aspi == SS_COMP) {
		rasdrvSCSIMessage(rd, 0x28, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_GETRESULT, &rc_be, sizeof(rc_be));
	}
	return peekBE32(&rc_be);
}

RASDRV_RESULT
rasdrv_FsCalCmd(rasdrv_t *rd, unsigned char funccode, const void *sendbuf, unsigned sendlen, void *recvbuf, unsigned recvlen)
{
	RSC_BE32	rc_be;
	int rc_aspi;

	pokeBE32(&rc_be, (unsigned long)(-1L));
	rc_aspi = rasdrvSCSIMessage(rd, 0x2a, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_SENDCMD, (void *)sendbuf, sendlen);
	if (rc_aspi == SS_COMP) {
		rc_aspi = rasdrvSCSIMessage(rd, 0x28, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_GETRESULT, &rc_be, sizeof(rc_be));
		if (rc_aspi == SS_COMP && rc_be.b[0] == 0) {
			rasdrvSCSIMessage(rd, 0x28, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_GETDATA, recvbuf, recvlen);
			
		}
	}
	
	return peekBE32(&rc_be);
}

RASDRV_RESULT
rasdrv_FsReadOpt(rasdrv_t *rd, unsigned char funccode, void *buf, unsigned buf_len)
{
	RASDRV_RESULT rc = (RASDRV_RESULT)(-1L);
	
	if (rasdrvSCSIMessage(rd, 0x28, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_GETOPTION, buf, buf_len) == SS_COMP) {
		rc = 0;
	}
	
	return rc;
}

RASDRV_RESULT
rasdrv_FsWriteOpt(rasdrv_t *rd, unsigned char funccode, const void *buf, unsigned buf_len)
{
	RASDRV_RESULT rc = (RASDRV_RESULT)(-1L);
	
	if (rasdrvSCSIMessage(rd, 0x2a, RASCSI_BRIDGE_RASDRV, funccode, RASCSI_RASDRV_SENDOPTION, (void *)buf, buf_len) == SS_COMP) {
		rc = 0;
	}
	
	return rc;
}


RASDRV_RESULT
rasdrvFsInitDevice(rasdrv_t *rd, const void *p)
{
	h68_argument_t a;
	
	a.buf[0] = '\0';
	if (p) strcpy(a.buf, p);
	
	/* result: number of units, or error */
	return rasdrv_FsSendCmd(rd, FS_CMD_INITDEVICE, &a, sizeof(a));
}


RASDRV_RESULT
rasdrvFsFiles(rasdrv_t *rd, RSC_U32 unit, RSC_U32 nkey, const h68_namests_t *n, h68_files_t *f)
{
	rasdrv_pkt_files *pkt = rasdrv_pkt_buf;
	
	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->nkey), nkey);
	memcpy(&(pkt->namests), n, sizeof(pkt->namests));
	memcpy(&(pkt->files), f, sizeof(pkt->files));
	
	return rasdrv_FsCalCmd(rd, FS_CMD_FILES, pkt, sizeof(*pkt), f, sizeof(*f));
}

RASDRV_RESULT
rasdrvFsNfiles(rasdrv_t *rd, RSC_U32 unit, RSC_U32 nkey, h68_files_t *f)
{
	rasdrv_pkt_nfiles *pkt = rasdrv_pkt_buf;
	
	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->nkey), nkey);
	memcpy(&(pkt->files), f, sizeof(pkt->files));
	
	return rasdrv_FsCalCmd(rd, FS_CMD_NFILES, pkt, sizeof(*pkt), f, sizeof(*f));
}



static
RASDRV_RESULT
rasdrvFsCMR(rasdrv_t *rd, unsigned char fs_cmd, RSC_U32 unit, const h68_namests_t *ns)
{
	rasdrv_pkt_dir *pkt = rasdrv_pkt_buf;
	
	pokeBE32(&(pkt->unit), unit);
	memcpy(&(pkt->namests), ns, sizeof(*ns));
	
	/*
	result: 0				found dir or file
			FS_DIRNOTFOUND	dir (or file) not found
			
			FS_FATAL_MEDIAOFFLINE
			FS_FATAL_INVALIDUNIT
	*/
	return rasdrv_FsSendCmd(rd, fs_cmd, pkt, sizeof(*pkt));
}
RASDRV_RESULT
rasdrvFsCheckDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns)
{
	return rasdrvFsCMR(rd, FS_CMD_CHECKDIR, unit, ns);
}
RASDRV_RESULT
rasdrvFsMakeDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns)
{
	return rasdrvFsCMR(rd, FS_CMD_MAKEDIR, unit, ns);
}
RASDRV_RESULT
rasdrvFsRemoveDir(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns)
{
	return rasdrvFsCMR(rd, FS_CMD_REMOVEDIR, unit, ns);
}



static
RASDRV_RESULT
rasdrv_FsCreatOpen(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const h68_namests_t *ns, int do_creat, unsigned attr)
{
	rasdrv_pkt_create *pkt = rasdrv_pkt_buf;
	RASDRV_RESULT rc;
	
	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	memcpy(&(pkt->namests), ns, sizeof(pkt->namests));
	if (do_creat) {
		unsigned do_creat_force = (do_creat >= 2);
		pokeBE32(&(pkt->attr), attr);
		pokeBE32(&(pkt->force), do_creat_force);
		rc = rasdrv_FsCalCmd(rd, FS_CMD_CREATE, pkt, sizeof(rasdrv_pkt_create), fcb, sizeof(*fcb));
	} else {
		rc = rasdrv_FsCalCmd(rd, FS_CMD_OPEN, pkt, sizeof(rasdrv_pkt_open), fcb, sizeof(*fcb));
	}
	
	return rc;
}


RASDRV_RESULT
rasdrvFsCreate(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const h68_namests_t *ns, int do_force, unsigned attr)
{
	return rasdrv_FsCreatOpen(rd, unit, key, fcb, ns, do_force ? 2:1, attr);
}
RASDRV_RESULT
rasdrvFsOpen(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const h68_namests_t *ns)
{
	return rasdrv_FsCreatOpen(rd, unit, key, fcb, ns, 0, 0);
}


RASDRV_RESULT
rasdrvFsDelete(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns)
{
	rasdrv_pkt_dir *pkt = rasdrv_pkt_buf;
	
	pokeBE32(&(pkt->unit), unit);
	memcpy(&(pkt->namests), ns, sizeof(pkt->namests));
	return rasdrv_FsSendCmd(rd, FS_CMD_DELETE, pkt, sizeof(*pkt));
}


RASDRV_RESULT
rasdrvFsRename(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns, const h68_namests_t *ns_new)
{
	rasdrv_pkt_rename *pkt = rasdrv_pkt_buf;
	
	pokeBE32(&(pkt->unit), unit);
	memcpy(&(pkt->namests), ns, sizeof(h68_namests_t));
	memcpy(&(pkt->namests_new), ns_new, sizeof(h68_namests_t));
	
	return rasdrv_FsSendCmd(rd, FS_CMD_RENAME, pkt, sizeof(*pkt));
}


RASDRV_RESULT
rasdrvFsAttribute(rasdrv_t *rd, RSC_U32 unit, const h68_namests_t *ns, unsigned char attr_set)
{
	rasdrv_pkt_attribute *pkt = rasdrv_pkt_buf;
	
	memcpy(&(pkt->namests), ns, sizeof(pkt->namests));
	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->attr), attr_set);
	/*
		result
		0..0xff		attribute of the file (Human68k)
					(when attr_set == 0xff, attribute not modified)
		other		somthing err
	*/
	return rasdrv_FsSendCmd(rd, FS_CMD_ATTRIBUTE, pkt, sizeof(*pkt));
}




RASDRV_RESULT
rasdrvFsClose(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb)
{
	rasdrv_pkt_close  *pkt = rasdrv_pkt_buf;

	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	
	return rasdrv_FsCalCmd(rd, FS_CMD_CLOSE, pkt, sizeof(*pkt), fcb, sizeof(*fcb));
}


RASDRV_RESULT
rasdrvFsRead(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, void *mem, size_t length)
{
	RASDRV_RESULT  rc;
	rasdrv_pkt_rw  *pkt = rasdrv_pkt_buf;

	(void)unit; /* pokeBE32(&(pkt->unit), unit); */
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	pokeBE32(&(pkt->length), length);

	rc = rasdrv_FsCalCmd(rd, FS_CMD_READ, pkt, sizeof(*pkt), fcb, sizeof(*fcb));
	if (rc != 0 && !RASDRV_ISERROR(rc)) {
		if (rc > length) rc = length;
		rasdrv_FsReadOpt(rd, FS_CMD_READ, mem, rc);
	}
	
	return rc;
}

RASDRV_RESULT
rasdrvFsWrite(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const void *mem, size_t length)
{
	rasdrv_pkt_rw  *pkt = rasdrv_pkt_buf;

	(void)unit; /* pokeBE32(&(pkt->unit), unit); */
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	pokeBE32(&(pkt->length), length);
	
	if (length) {
		RASDRV_RESULT rc = rasdrv_FsWriteOpt(rd, FS_CMD_WRITE, mem, length);
		if (rc != 0) return FS_FATAL_MEDIAOFFLINE;
	}
	
	return rasdrv_FsSendCmd(rd, FS_CMD_WRITE, pkt, sizeof(*pkt));
}


RASDRV_RESULT
rasdrvFsSeek(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, long offset, int whence)
{
	rasdrv_pkt_seek  *pkt = rasdrv_pkt_buf;

	(void)unit; /* pokeBE32(&(pkt->unit), unit); */
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	pokeBE32(&(pkt->whence), whence);
	pokeBE32(&(pkt->offset), (unsigned long)offset);
	
	return rasdrv_FsCalCmd(rd, FS_CMD_SEEK, pkt, sizeof(*pkt), fcb, sizeof(*fcb));
}


RASDRV_RESULT
rasdrvFsTimeStamp(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, unsigned dosdate, unsigned dostime)
{
	rasdrv_pkt_timestamp  *pkt = rasdrv_pkt_buf;

	pokeBE32(&(pkt->unit), unit);
	pokeBE32(&(pkt->key), key);
	memcpy(&(pkt->fcb), fcb, sizeof(pkt->fcb));
	pokeBE16(&(pkt->time), dostime);
	pokeBE16(&(pkt->date), dosdate);
	
	return rasdrv_FsCalCmd(rd, FS_CMD_TIMESTAMP, pkt, sizeof(*pkt), fcb, sizeof(*fcb));
}



/*
---------------------------------------
*/

char *
rasdrv_filenameptr(const char *s_top)
{
	char *s = (char *)s_top;
	char *p = NULL;
	unsigned char sc;
	
	while((sc = *s) != '\0') {
		if (sc == ':' || sc == '/' || sc == '\\') p = (char *)s + 1;
		/* charnext for sjis (cp932) */
		if ((sc >= 0x81 && sc <= 0x9f)||(sc >= 0xe0 && sc <= 0xfc)) {
			if (s[1] != '\0') ++s;
		}
		++s;
	}
	if (p == NULL) p = (char *)s_top;
	
	return p;
}

static
RASDRV_RESULT
pathname_to_namests(h68_namests_t *ns, const char *pathname, int pathonly)
{
	char *name;
	size_t np;
	size_t ipath;
	
	memset(ns->path, 0, sizeof(ns->path));
	memset(ns->name, ' ', sizeof(ns->name)+sizeof(ns->ext));
	memset(ns->add, 0, sizeof(ns->add));

	if (pathonly) {
		name = (char *)pathname + strlen(pathname);
	} else {
		name = rasdrv_filenameptr(pathname);
		if (*name == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
			name += strlen(name);
			pathonly = 1;
		}
	}
	
	np = name - (char *)pathname;
	ipath = 0;
	if (np >= 2 && pathname[1] == ':') {
		char c = pathname[0];
		ipath = 2;
		if (c >= '0' && c <= '9') {
			ns->drive = c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			ns->drive = c - 'A';
		} else if (c >= 'a' && c <= 'z') {
			ns->drive = c - 'a';
		} else {
			return FS_INVALIDDRV;
		}
	}
	np -= ipath;
	if (np >= sizeof(ns->path)) return FS_INVALIDPRM;
	if (np > 0) memcpy(ns->path, pathname + ipath, np);
	if (!pathonly) {
		char *pp;
		size_t nf, ne;
		
		pp = strchr(name, '.');
		nf = ne = 0;
		if (pp) {
			nf = pp - name;
			++pp;
			ne = strlen(pp);
			if (nf > sizeof(ns->name) || ne > sizeof(ns->ext))
				return FS_INVALIDPRM;
			if (ne > 0) memcpy(ns->ext, pp, ne);
		} else {
			nf = strlen(name);
			if (nf > sizeof(ns->name) + sizeof(ns->ext))
				return FS_INVALIDPRM;
		}
		if (nf > 0) memcpy(ns->name, name, nf);
	}
	return 0;
}




static
RASDRV_RESULT
rasdrvFsCMR2(rasdrv_t *rd, unsigned char fs_cmd, RSC_U32 unit, const char *path)
{
	RASDRV_RESULT rc;
	h68_namests_t ns;
	
	memset(&ns, 0, sizeof(ns));
	rc = pathname_to_namests(&ns, path, 1);
	if (RASDRV_ISERROR(rc)) return rc;
	
	return rasdrvFsCMR(rd, fs_cmd, unit, &ns);
}

RASDRV_RESULT
rasdrvFsCheckDir2(rasdrv_t *rd, RSC_U32 unit, const char *path)
{
	return rasdrvFsCMR2(rd, FS_CMD_CHECKDIR, unit, path);
}
RASDRV_RESULT
rasdrvFsMakeDir2(rasdrv_t *rd, RSC_U32 unit, const char *path)
{
	return rasdrvFsCMR2(rd, FS_CMD_MAKEDIR, unit, path);
}
RASDRV_RESULT
rasdrvFsRemoveDir2(rasdrv_t *rd, RSC_U32 unit, const char *path)
{
	return rasdrvFsCMR2(rd, FS_CMD_REMOVEDIR, unit, path);
}
static
RASDRV_RESULT
rasdrv_FsCreatOpen2(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const char *pathname, int do_creat, unsigned attr)
{
	RASDRV_RESULT rc;
	h68_namests_t ns;
	
	memset(&ns, 0, sizeof(ns));
	rc = pathname_to_namests(&ns, pathname, 0);
	if (RASDRV_ISERROR(rc)) return rc;
	
	return rasdrv_FsCreatOpen(rd, unit, key, fcb, &ns, do_creat, attr);
}
RASDRV_RESULT
rasdrvFsCreate2(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const char *pathname, int do_force, unsigned attr)
{
	return rasdrv_FsCreatOpen2(rd, unit, key, fcb, pathname, do_force ? 2:1, attr);
}
RASDRV_RESULT
rasdrvFsOpen2(rasdrv_t *rd, RSC_U32 unit, RSC_U32 key, h68_fcb_t *fcb, const char *pathname)
{
	return rasdrv_FsCreatOpen2(rd, unit, key, fcb, pathname, 0, 0);
}



RASDRV_RESULT
rasdrvFsRename2(rasdrv_t *rd, RSC_U32 unit, const char *name, const char *name_new)
{
	RASDRV_RESULT rc;
	h68_namests_t  ns, ns_new;
	
	memset(&ns, 0, sizeof(ns));
	memset(&ns_new, 0, sizeof(ns_new));
	rc = pathname_to_namests(&ns, name, 0);
	if (!RASDRV_ISERROR(rc))
		rc = pathname_to_namests(&ns_new, name_new, 0);
	if (RASDRV_ISERROR(rc))
		return rc;
	
	return rasdrvFsRename(rd, unit, &ns, &ns_new);
}

RASDRV_RESULT
rasdrvFsDelete2(rasdrv_t *rd, RSC_U32 unit, const char *pathname)
{
	RASDRV_RESULT rc;
	h68_namests_t ns;
	
	rc = pathname_to_namests(&ns, pathname, 0);
	if (RASDRV_ISERROR(rc)) return rc;

	return rasdrvFsDelete(rd, unit, &ns);
}


RASDRV_RESULT
rasdrvFsAttribute2(rasdrv_t *rd, RSC_U32 unit, const char *pathname, unsigned char attr_set)
{
	RASDRV_RESULT rc;
	h68_namests_t ns;
	
	memset(&ns, 0, sizeof(ns));
	rc = pathname_to_namests(&ns, pathname, 0);
	if (RASDRV_ISERROR(rc)) return rc;
	
	return rasdrvFsAttribute(rd, unit, &ns, attr_set);
	
}







static int
ismatch_dos83(const char *fn)
{
	int rc = 0;
	char *p_ext;
	unsigned n_fn;
	
	n_fn = strlen(fn);
	if (n_fn == 0 || n_fn > 12) return 0;
	if (*fn == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')))
		return 1;
	
	p_ext = strchr(fn, '.');
	if (p_ext) {
		/* check 8.3 */
		unsigned n_body = ((const char *)p_ext - fn);
		unsigned n_ext = strlen(p_ext);
		char *pp = strchr(p_ext + 1, '.');
		rc = ((n_body >= 1 && n_body <= 8) && pp == NULL && n_ext <= 4);
	}
	else {
		/* check 11 (e.g. volume label) */
		rc = (n_fn >= 1 && n_fn <= 11);
	}

	return rc;
}


static
void
nto83_nameexp(char *name, size_t name_len, const char *name_src, int exp_wild)
{
	size_t pos = 0;
	
	if (name_len > 0) memset(name, ' ', name_len);
	while(pos < name_len) {
		char c = *name_src++;
		if (c == '\0') break;
		if (c == '*' && exp_wild) {
			while(pos < name_len) {
				name[pos++] = '?';
			}
			break;
		}
		name[pos++] = c;
	}
}

static int
nameto83(char *nameext11, const char *name, int exp_wild)
{
	const char *p, *p2;
	size_t n;
	char s[12];
	
	if (strcmp(name, "*")==0) name = "*.*";
	memset(nameext11, ' ', 11);
	
	p = strchr(name, '.');
	if (p == NULL) {
		n = strlen(p);
		if (n > 8+3) return -1;
		nto83_nameexp(nameext11, 8+3, name, exp_wild);
		return 0;
	}
	p2 = strrchr(name, '.');
	if (p != p2) return -1;
	n = p - name;
	if (n > 8) return -1;
	if (n > 0) memcpy(s, name, n);
	s[n] = '\0';
	nto83_nameexp(nameext11, 8, s, exp_wild);
	n = strlen(p + 1);
	if (n > 3) return -1;
	nto83_nameexp(nameext11 + 8, 3, p+1, exp_wild);
	
	return 0;
}



static
int
dosftime_to_tm(const unsigned dd, const unsigned dt, struct tm *tm)
{
	tm->tm_sec = (dt & 0x1f) * 2;
	tm->tm_min = (dt >> 5) & 0x3f;
	tm->tm_hour = (dt >> 11);
	tm->tm_mday = (dd & 0x1f);
	tm->tm_mon = (dd >> 5) & 0x0f;
	tm->tm_year = 1980 + (dd >> 9);
	
	return (tm->tm_mday >= 1 && tm->tm_mday <= 31 && tm->tm_mon >= 1 && tm->tm_mon <= 12);
}

static
int
tm_to_dosftime(const struct tm *tm, unsigned *dd, unsigned *dt)
{
	unsigned t;
	
	/* do DOSes support leap-second? */
	if (tm->tm_mday < 1 || tm->tm_mday > 31 || 
	    tm->tm_mon < 1 || tm->tm_mon > 12 ||
	    tm->tm_hour < 0 || tm->tm_hour > 24 ||
	    tm->tm_min < 0 || tm->tm_min > 60 ||
	    tm->tm_sec < 0 || tm->tm_sec > 62)
		return 0;
	
	if (tm->tm_year >= 1980 && tm->tm_year <= 1980 + 127)
		t = tm->tm_year - 1980;
	else if (tm->tm_year >= 0 && tm->tm_year <= 127)
		t = tm->tm_year;
	else
		return 0;
	
	if (dd) *dd = (t << 9) | (tm->tm_mon << 5) | tm->tm_mday;
	if (dt) *dt = ((unsigned)(tm->tm_hour)<<11) | (tm->tm_min << 5) | (tm->tm_sec >> 1);
	
	return 1;
}

static
int
h68files_to_tm(struct tm *tm, const h68_files_t *f)
{
	unsigned d, t;
	d = peekBE16(&(f->date));
	t = peekBE16(&(f->time));
	return dosftime_to_tm(d, t, tm);
}


static
int
tm_to_h68files(const struct tm *tm, h68_files_t *f)
{
	unsigned d, t;
	
	if (tm_to_dosftime(tm, &d, &t)) {
		pokeBE16(&(f->date), d);
		pokeBE16(&(f->time), t);
		return 1;
	}
	
	return 0;
}



/*
-----------------------------------------
*/

void rasdrv_handle_init(void)
{
#if 0
	memset(&(rashdl[0]), 0, sizeof(rashdl));
#endif
	rashdl[0].type = RASDRV_HANDLE_RESERVED;
	rashdl[0].handle = NULL;
}

int rasdrv_handle_new(int type)
{
	int hdl = -1;
	size_t bodylen = 0;
	int i;
	
	switch(type) {
		case RASDRV_HANDLE_FILES: bodylen = sizeof(h68_files_t); break;
		case RASDRV_HANDLE_FCB:   bodylen = sizeof(h68_fcb_t); break;
		default:
			break;
	}
	if (bodylen == 0) return hdl; /* -1 */
	
	for(i=0; i< sizeof(rashdl)/sizeof(rashdl[0]); ++i) {
		if (rashdl[i].type == RASDRV_HANDLE_EMPTY) {
			rashdl[i].handle = alloc_nobound64k(bodylen);
			hdl = i;
			break;
		}
	}

	return hdl;
}

int rasdrv_handle_delete(int hdl)
{
	int rc = -1;
	
	if (hdl >= 0 && hdl < sizeof(rashdl)/sizeof(rashdl[0])) {
		if (rashdl[hdl].type != RASDRV_HANDLE_EMPTY && rashdl[hdl].type != RASDRV_HANDLE_RESERVED) {
			if (rashdl[hdl].handle) {
				free(rashdl[hdl].handle);
				rashdl[hdl].handle = NULL;
			}
			rashdl[hdl].type = RASDRV_HANDLE_EMPTY;
			rc = 0;
		}
	}
	
	return rc;
}

static
RASDRV_HANDLE_BODY *
rasdrv_handle_tobody(int hdl)
{
	return (hdl >= 0 && hdl < sizeof(rashdl)/sizeof(rashdl[0]))
			? rashdl[hdl].handle
			: NULL;
}

h68_fcb_t *
rasdrv_handle_fcb(int hdl)
{
	return &((rasdrv_handle_tobody(hdl))->fcb);
}

h68_files_t *
rasdrv_handle_files(int hdl)
{
	return &((rasdrv_handle_tobody(hdl))->files);
}
