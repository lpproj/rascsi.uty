/*
rascpy.c: a sample of using RASDRV on DOS


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
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aspi_d16.h"
#include "rasdrv.h"

static RSC_U32 rasdrv_unit = 0;


static
void *
my_malloc(size_t n, unsigned l)
{
	void *p;
	
	if (n == 0) n = 1;
	p = malloc(n);
	if (!p) {
		fprintf(stderr, "fatal: memory allocation failure at line %u.\n", l);
		exit(1);
	}
	memset(p, 0, n);
	
	return p;
}
#define malloc(n)  my_malloc(n,__LINE__)


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
#if 1
	{
		/*
		    workaround for the difference between
		        unixtime  (begin at 1970-01-01)
		    and dostime   (begin at 1980-01-01)
		*/
		unsigned y = (dd >> 9);
		if (y >= 0x76 && y <= 0x7f) {
			tm->tm_year = 1970 + y - 0x76;
		}
		else {
			tm->tm_year = 1980 + y;
		}
	}
#else
	tm->tm_year = 1980 + (dd >> 9);
#endif
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

typedef struct {
	unsigned handle;
	unsigned errcode;
	unsigned long length;
	unsigned attr;
	unsigned dostime;
	unsigned dosdate;
} dos_handle_info;

enum {
	mydos_O_RDONLY = 0,
	mydos_O_WRONLY = 1,
	mydos_O_RDWR = 2,
	mydos_O_NOINHERIT = 0x80U
};
#define mydos_O_BINARY 0


int mydos_open(dos_handle_info *h, const char *pathname, unsigned mode)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.dx = FP_OFF(pathname);
	sr.ds = FP_SEG(pathname);
	r.h.ah = 0x3d; /* open */
	r.h.al = (unsigned char)mode;
	r.x.cx = 0; /* for safe */
	memset(h, 0, sizeof(*h));
	intdosx(&r, &r, &sr);
	if (r.x.cflag) return (h->errcode = r.x.ax);
	h->handle = r.x.ax;
	
	return 0;
}

int mydos_creat(dos_handle_info *h, const char *pathname, unsigned attr, int do_force)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.dx = FP_OFF(pathname);
	sr.ds = FP_SEG(pathname);
	r.h.ah = do_force ? 0x3c : 0x5b;
	r.x.cx = attr;
	memset(h, 0, sizeof(*h));
	intdosx(&r, &r, &sr);
	if (r.x.cflag) return (h->errcode = r.x.ax);
	h->handle = r.x.ax;
	h->attr = attr;
	
	return 0;
}

int mydos_close(dos_handle_info *h)
{
	union REGS r;
	r.h.ah = 0x3e;
	r.x.bx = h->handle;
	intdos(&r, &r);
	
	return r.x.cflag ? r.x.ax : 0;
}

int mydos_rw(dos_handle_info *h, int do_write, void *mem, size_t len)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.dx = FP_OFF(mem);
	sr.ds = FP_SEG(mem);
	r.x.bx = h->handle;
	r.x.cx = len;
	r.h.ah = do_write ? 0x40 : 0x3f;
	intdosx(&r, &r, &sr);
	if (r.x.cflag) {
		h->errcode = r.x.ax;
		r.x.ax = 0;
	}
	
	return r.x.ax;
}

int mydos_getattr(const char *pathname, unsigned *attr)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.dx = FP_OFF(pathname);
	sr.ds = FP_SEG(pathname);
	r.x.ax = 0x4300;
	intdosx(&r, &r, &sr);
	if (r.x.cflag) return r.x.ax;
	*attr = r.x.cx;
	
	return 0;
}

int mydos_setattr(const char *pathname, unsigned attr)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.dx = FP_OFF(pathname);
	sr.ds = FP_SEG(pathname);
	r.x.ax = 0x4301;
	r.x.cx = attr;
	intdosx(&r, &r, &sr);
	
	return (r.x.cflag) ? r.x.ax : 0;
}

int mydos_getftime(dos_handle_info *h)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.ax = 0x5700;
	r.x.bx = h->handle;
	intdosx(&r, &r, &sr);
	if (r.x.cflag) return (h->errcode = r.x.ax);
	h->dostime = r.x.cx;
	h->dosdate = r.x.dx;
	
	return 0;
}

int mydos_setftime(dos_handle_info *h, unsigned t, unsigned d)
{
	union REGS r;
	struct SREGS sr;
	
	r.x.ax = 0x5701;
	r.x.bx = h->handle;
	r.x.cx = t;
	r.x.dx = d;
	intdosx(&r, &r, &sr);
	if (r.x.cflag) return (h->errcode = r.x.ax);
	h->dostime = t;
	h->dosdate = d;
	
	return 0;
}

int mydos_commit_files(void)
{
	union REGS r;
	r.x.ax = 0x5d01;
	intdos(&r, &r);
	
	return r.x.cflag ? r.x.ax : 0;
}


enum {
	ANS_NO = 0,
	ANS_YES = 1,
	ANS_ALL = 2,
	ASK_NO = 0,
	ASK_YESNO = 1,
	ASK_YESNOALL = 2,
	ASK_YESNO_FLAG_ENTER = 0x10,
	ASK_YESNO_ENTER = 0x11,
	ASK_YESNOALL_ENTER = 0x12
};

int mydos_yesno(int flag)
{
	union REGS r;
	char yn = '\0';
	int check_a, need_enter;
	
	if (flag == ASK_NO) return ANS_NO;
	check_a = (flag == ASK_YESNOALL || flag == ASK_YESNOALL_ENTER);
	need_enter = (flag & ASK_YESNO_FLAG_ENTER);
	
	fflush(stdout);
	
	while(1) {
		r.h.ah = 0x0c;
		r.h.al = 0x08;
		intdos(&r, &r);
		if (r.h.al >= 'a' && r.h.al <= 'z') r.h.al -= 'a' - 'A';
		if (r.h.al == 0x1b) {
			yn = 'N';
			if (need_enter) printf("\n");
			break;
		}
		if (r.h.al == 'Y' || r.h.al == 'N' || (check_a && r.h.al == 'A')) {
			if (need_enter && yn != '\0') printf("%c", 0x08);
			yn = r.h.al;
		} else if (!(need_enter && r.h.al == 0x0d)) {
			continue;
		}
		if (need_enter) {
			if (r.h.al == 0x0d) {
				if (yn != '\0') {
					printf("\n");
					break;
				}
				else {
					continue;
				}
			} else {
				printf("%c", r.h.al);
				fflush(stdout);
			}
		}
		else {
			break;
		}
		
	}
	
	if (yn == 'A') return ANS_ALL;
	return (yn == 'Y') ? ANS_YES : ANS_NO;
}

static
int
ask_file_overwrite(const char *filename, int the_all)
{
	printf("Overwrite '%s' (Yes/No%s)? ", filename, the_all ? "/All" : "");
	return mydos_yesno(the_all ? ASK_YESNOALL_ENTER : ASK_YESNO_ENTER);
}

/*
-----------------------------------------
*/

static size_t  get_buflen(void)
{
    size_t n = rasdrv_tfr_buflen & (~0x1ffU);
    if (n > 16384) n = 16384;
    if (n == 0) {
        fprintf(stderr, "fatal: need rasdrv_mem_init()\n");
        exit(1);
    }
    return n;
}


int
copyfile_from_rasdrv(const char *rasfile, const char *destfilename)
{
	dos_handle_info dh;
	RASDRV_RESULT rc;
	int fd_ras;
	h68_fcb_t *fcb = NULL;
	
	fd_ras = rasdrv_handle_new(RASDRV_HANDLE_FCB);
	if (fd_ras >= 0) {
		fcb = rasdrv_handle_fcb(fd_ras);
		pokeBE16(&(fcb->mode), OP_READ | OP_SHARE_READ);
		rc = rasdrvFsOpen2(h_rasdrv, rasdrv_unit, fd_ras, fcb, rasfile);
	}
	if (fd_ras < 0 || RASDRV_ISERROR(rc)) {
		fprintf(stderr, "can't open file '%s'\n", rasfile);
		return -1;
	}
	
	if (mydos_creat(&dh, destfilename, 0, 1) == 0) {
		unsigned long filelen = peekBE32(&(fcb->size));
		while (filelen > 0)
		{
			char *b = rasdrv_tfr_buf;
			size_t bufsiz = get_buflen();
			unsigned rcnt, wcnt;
			
			rcnt = (filelen > bufsiz) ? bufsiz : (unsigned)filelen;
			rc = rasdrvFsRead(h_rasdrv, rasdrv_unit, fd_ras, fcb, b, rcnt);
			if (RASDRV_ISERROR(rc)) {
				fprintf(stderr, "error on reading from rasdrv (rc=%ld)\n", rc);
				break;
			}
			rcnt = wcnt = (unsigned)rc;
			while(wcnt > 0) {
				unsigned tcnt = mydos_rw(&dh, 1, b, wcnt);
				if (tcnt == 0 || tcnt > wcnt) break;
				wcnt -= tcnt;
			}
			if (dh.errcode != 0) {
				fprintf(stderr, "error on writing a file (rc=%d)\n", rc);
				break;
			}
			filelen -= rcnt;
		}
		if (dh.errcode == 0) {
			mydos_setftime(&dh, peekBE16(&(fcb->time)), peekBE16(&(fcb->date)));
		}
		mydos_close(&dh);
	} else {
		fprintf(stderr, "can't create file '%s'\n", destfilename);
		return -1;
	}
	rasdrvFsClose(h_rasdrv, rasdrv_unit, fd_ras, fcb);
	rasdrv_handle_delete(fd_ras);
	
	return 0;
}

int
copyfile_to_rasdrv(const char *srcpathname, const char *rasfile)
{
	dos_handle_info dh;
	RASDRV_RESULT rc;
	int fd_ras;
	h68_fcb_t *fcb = NULL;
	int err_in_copying = 0;
	int set_date;
	
	if (mydos_open(&dh, srcpathname, mydos_O_RDONLY) != 0) {
		fprintf(stderr, "can't open file '%s' (rc=%d)\n", srcpathname, dh.errcode);
		return -1;
	}
	set_date = (mydos_getftime(&dh) == 0);

	fd_ras = rasdrv_handle_new(RASDRV_HANDLE_FCB);
	if (fd_ras >= 0) {
		fcb = rasdrv_handle_fcb(fd_ras);
		pokeBE16(&(fcb->mode), OP_WRITE);
		rc = rasdrvFsCreate2(h_rasdrv, rasdrv_unit, fd_ras, fcb, rasfile, 1, 0);
	}
	if (fd_ras < 0 || RASDRV_ISERROR(rc)) {
		fprintf(stderr, "can't create file '%s'.\n", rasfile);
		mydos_close(&dh);
		return -1;
	}
	
	while(err_in_copying == 0) {
		char *b = rasdrv_tfr_buf;
		size_t bufsiz = get_buflen();
		unsigned rcnt, wcnt;
		
		wcnt = rcnt = mydos_rw(&dh, 0, b, bufsiz);
		if (rcnt == 0) {
			if (dh.errcode) {
				fprintf(stderr, "error on reading a file (rc=%d).\n", dh.errcode);
				err_in_copying = 1;
			}
			break;
		}
		while(wcnt > 0) {
			rc = rasdrvFsWrite(h_rasdrv, rasdrv_unit, fd_ras, fcb, b, wcnt);
			if (rc == 0 || rc > wcnt || RASDRV_ISERROR(rc)) {
				fprintf(stderr, "error on writing to rasdrv (rc=%ld)\n", rc);
				err_in_copying = 1;
				break;
			}
			wcnt -= (unsigned)rc;
		}
	}
	mydos_close(&dh);
	if (set_date && err_in_copying == 0) {
		rasdrvFsTimeStamp(h_rasdrv, rasdrv_unit, fd_ras, fcb, dh.dosdate, dh.dostime);
	}
	rasdrvFsClose(h_rasdrv, rasdrv_unit, fd_ras, fcb);
	rasdrv_handle_delete(fd_ras);

	return err_in_copying ? -1 : 0;
}



static char *smonth[16] = {
	"???", 
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	"???", "???", "???"
};

static int current_year = 0;
static int date_form = 1;

#define DISP_FLAG_DOSISH	0x80
#define DISP_FLAG_LONG		0x40
#define DISP_FLAG_ALL		0x02
#define DISP_FLAG_BARE		0x01
#define DISP_FLAG_DIRONLY	0x10

static
int
disp_dirent1(FILE *fo, h68_files_t *f, unsigned flags)
{
	int do_count = 0;
	struct tm tm;
	
	h68files_to_tm(&tm, f);
	
	if (!(flags & DISP_FLAG_ALL) && f->full[0] == '.')
		return 0;
	if ((flags & (DISP_FLAG_BARE|DISP_FLAG_ALL))==DISP_FLAG_BARE && f->full[0] == '.' && (f->full[1] == '\0' || f->full[1] == '.'))
		return 0;
	if ((flags & DISP_FLAG_DIRONLY) && (f->attr & (AT_DIRECTORY|AT_VOLUME))!= AT_DIRECTORY)
		return 0;

	if (flags & DISP_FLAG_DOSISH) {
		switch(date_form) {
			case 1:
				fprintf(fo, "%04d/%02d/%02d  %02d:%02d", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min);
				break;
			default:
				fprintf(fo, "%02d/%02d/%04d  %02d:%02d %s", tm.tm_mon, tm.tm_mday, tm.tm_year, tm.tm_hour % 12, tm.tm_min, (tm.tm_hour / 12) ? "pm" : "am");
				break;
		}
		if (f->attr & AT_DIRECTORY) {
			fprintf(fo, "    <DIR>   ");
			do_count = (flags & DISP_FLAG_ALL) != 0;
		} else {
			fprintf(fo, " %11lu", peekBE32(&(f->size)));
			do_count = 1;
		}
		fprintf(fo, " %s\n", f->full);
	} else {
		if ((flags & DISP_FLAG_LONG)) {
			char s[12];
			strcpy(s, "-r--------");
			if (f->attr & AT_DIRECTORY) {
				s[0] = 'd';
				s[3] = /* s[6] = s[9] = */ 'x';
			} else {
				s[4] = s[7] = s[1]; /* 'r' */
			}
			if (!(f->attr & AT_READONLY)) {
				s[2] = 'w';
			}
			fprintf(fo, "%s", s);
			fprintf(fo, " %u", (f->attr&AT_DIRECTORY) ? 3:1); /* dummy dirents or hardlinks */
			fprintf(fo, " pi pi"); /* dummy owner & group */
			fprintf(fo, " %11lu", peekBE32(&(f->size)));
			fprintf(fo, " %s %2d", smonth[tm.tm_mon], tm.tm_mday);
			if (current_year > 0 && current_year == tm.tm_year) {
				fprintf(fo, " %02d:%02d", tm.tm_hour, tm.tm_min);
			} else {
				fprintf(fo, "  %4d", tm.tm_year);
			}
			fprintf(fo, " %s\n", f->full);
			
		} else {
			fprintf(fo, "%s\n", f->full);
		}
		do_count = 1;
	}
	
	return do_count;
}


static
long
disp_dir(FILE *fo, const char *path, const char *match_name, unsigned flags)
{
	static int hf = -1;
	long filecnt = 0;
	RASDRV_RESULT rc;
	h68_namests_t  n;
	h68_files_t *f;
	
	
	if (hf == -1)
		hf = rasdrv_handle_new(RASDRV_HANDLE_FILES);
	if (!path || strlen(path) >= sizeof(n.path) || hf == -1)
		return -1L;
	f = rasdrv_handle_files(hf);
	
	memset(&n, 0, sizeof(n));
	memset(f, 0, sizeof(h68_files_t));
	n.wildcard = 0x1;
	strcpy(n.path, path);
	nameto83(&(n.name[0]), match_name, 1);
	f->fatr = AT_ALL;
	
	rc = rasdrvFsFiles(h_rasdrv, rasdrv_unit, (unsigned)hf, &n, f);
	
	while(!RASDRV_ISERROR(rc)) {
		filecnt += disp_dirent1(fo, f, flags);
		rc = rasdrvFsNfiles(h_rasdrv, rasdrv_unit, (unsigned)hf, f);
	}
	return filecnt;
}



/*
-----------------------------------------
*/

#define COPY_FLAG_PROMPT 0x10U

int
cmd_get(const char *n_from, const char *n_to, int copy_flag)
{
	int rcint;
	
	if (!n_to || !*n_to) {
		n_to = rasdrv_filenameptr(n_from);
	}
	if (copy_flag & COPY_FLAG_PROMPT) {
		RASDRV_RESULT rc;
		unsigned u = 0;
		
		rc = rasdrvFsAttributeGet2(h_rasdrv, rasdrv_unit, n_from);
		if (RASDRV_ISERROR(rc) || (rc & (AT_VOLUME | AT_DIRECTORY))) {
			fprintf(stderr, "rasdrv: can't open '%s'.\n", n_from, rc);
			return -1;
		}
		rcint = mydos_getattr(n_to, &u);
		if (rcint == 0) {
			u = ask_file_overwrite(n_to, 0);
			if (u == ANS_NO) {
				return 0;
			}
		}
	}
	printf("copying rasdrv %s => %s ... ", n_from, n_to);
	fflush(stdout);
	rcint = copyfile_from_rasdrv(n_from, n_to);
	printf(rcint == 0 ? "ok\n" : "failure\n");
	
	return rcint;
}

int
cmd_put(const char *n_from, const char *n_to, int copy_flag)
{
	int rcint;
	
	if (!n_to || !*n_to) {
		n_to = rasdrv_filenameptr(n_from);
	}
	if (copy_flag & COPY_FLAG_PROMPT) {
		RASDRV_RESULT rc;
		unsigned u = 0;
		
		rcint = mydos_getattr(n_from, &u);
		if (rcint != 0 && (u & 0x18)) {
			fprintf(stderr, "rasdrv: can't open local '%s'.\n", n_from);
			return -1;
		}
		rc = rasdrvFsAttributeGet2(h_rasdrv, rasdrv_unit, n_to);
		if (!RASDRV_ISERROR(rc)) {
			u = ask_file_overwrite(n_to, 0);
			if (u == ANS_NO) {
				return 0;
			}
		}
	}
	printf("copying local %s => %s ... ", n_from, n_to);
	fflush(stdout);
	rcint = copyfile_to_rasdrv(n_from, n_to);
	printf(rcint == 0 ? "ok\n" : "failure\n");
	
	return rcint;
}


long
cmd_dir(const char *pathname, int dir_flag)
{
	long filecnt;
	RASDRV_RESULT rc;
	char *p, *nam;
	
	p = NULL;
	nam = rasdrv_filenameptr(pathname); /* check the trail is / or \ */
	if (*nam == '\0') {
		/* dir only */
		p = strdup(pathname);
		nam = "*";
	}
	else {
		/* check dir + name or dironly */
		rc = rasdrvFsAttributeGet2(h_rasdrv, rasdrv_unit, pathname);
		if (!RASDRV_ISERROR(rc) && (rc & AT_DIRECTORY)) {
			/* dironly */
			size_t len = strlen(pathname);
			p = malloc(len + 2);
			if (len > 0) memcpy(p, pathname, len);
			strcpy(p + len, "/");
			nam = "*";
		}
		else {
			/* dir + name */
			size_t len = (const char *)nam - pathname;
			p = malloc(len + 1);
			if (len > 0) memcpy(p, pathname, len);
			p[len] = '\0';
		}
	}
	
	if (!(dir_flag & DISP_FLAG_BARE)) {
		printf("Files:\n");
	}
	filecnt = disp_dir(stdout, p, nam, dir_flag);
	free(p);
	if (!(dir_flag & DISP_FLAG_BARE)) {
		printf("\n");
		if (filecnt != -1) printf("%lu entries\n", (unsigned long)filecnt);
	}
	
	return filecnt;
}


/*
-----------------------------------------
*/

static int optI = 6;
static int optA = 0;
static int optHelp = 0;
static char *cmd = NULL;
static int cmd_opt;
static char **cmd_arg;

static
int
my_getopt(int argc, char *argv[])
{
	int iserr = 0;
	char *s;
	
	while((s = *argv) != NULL) {
		if (!cmd) {
			if (*s == '-' || *s == '/') {
				++s;
				switch(toupper(*s)) {
					case 'I':
						if (s[1] >= '0' && s[1] <= '9') { optI = s[1] - '0'; }
						break; 
					case 'A':
						if (s[1] >= '0' && s[1] <= '9') { optA = s[1] - '0'; }
						break; 
					case 'H': case '?':
						optHelp = 1;
						break;
				}
			}
			else {
				cmd = s;
				strlwr(cmd);
				if (strcmp(cmd, "help") == 0) {
					optHelp = 1;
				}
				else if (strcmp(cmd, "dir") == 0) {
					cmd_opt |= DISP_FLAG_DOSISH | DISP_FLAG_LONG;
				}
				else if (strcmp(cmd, "ls") == 0) {
					cmd_opt &= ~(unsigned)(DISP_FLAG_DOSISH);
				}
				else if (strcmp(cmd, "get") == 0 || strcmp(cmd, "put") == 0) {
					cmd_opt |= COPY_FLAG_PROMPT;
				}
				else {
					iserr = -1;
				}
			}
		}
		else if (strcmp(cmd, "dir") == 0 || strcmp(cmd, "ls") == 0) {
			if (*s == '-') {
				++s;
				while(*s) {
					switch(toupper(*s)) {
						case 'A': cmd_opt |= DISP_FLAG_ALL; break;
						case 'L': cmd_opt |= DISP_FLAG_LONG; break;
						case '1': cmd_opt |= DISP_FLAG_BARE; break;
						case 'B':
							cmd_opt &= ~(unsigned)(DISP_FLAG_LONG | DISP_FLAG_DOSISH);
							cmd_opt |= DISP_FLAG_BARE;
							break;
					}
					++s;
				}
			}
			else {
				cmd_arg = argv;
				break;
			}
		}
		else if (strcmp(cmd, "get") == 0 || strcmp(cmd, "put") == 0) {
			if (*s == '-') {
				++s;
				while(*s) {
					switch(toupper(*s)) {
						case 'F': case 'Y':
							cmd_opt &= ~(unsigned)COPY_FLAG_PROMPT;
							break;
#if 0
						default:
							iserr = -1;
							break;
#endif
					}
					++s;
				}
			}
			else {
				cmd_arg = argv;
				break;
			}
		}
		
		++argv;
	}
	
	if (cmd && (strcmp(cmd, "help")==0 || *cmd == '?')) {
		optHelp = 1;
	}
	
	return iserr;
}


static
int
get_current_year(void)
{
	time_t t;
	struct tm *tm;
	
	t = time(NULL);
	tm = localtime(&t);
	if (!tm) return 0;
	return (tm->tm_year < 1900) ? tm->tm_year + 1900 : tm->tm_year;
}


static
void
usage(void)
{
	const char msg[] = 
		"usage:  RASCPY [-In] [dir | ls | get | put] parameters...\n"
		"  -I    specify SCSI ID for RASDRV (-I6 as default)\n"
		"\n"
		"commands:\n"
		"RASCPY dir [-b] remote_path\n"
		"        list files and directories (DOS style)\n"
		"RASCPY ls [-1] [-l] [-a] remote_path\n"
		"        list files and directories (U*ix style)\n"
		"RASCPY get [-f] remote_file local_file\n"
		"        copy a file from RASDRV\n"
		"RASCPY put [-f] local_file remote_file\n"
		"        copy a file to RASDRV\n"
			;
	
	printf("%s", msg);
}


int main(int argc, char *argv[])
{
	RASDRV_RESULT rc;
	int rcint;
	
	aspiInit();

	rasdrv_mem_init();
	rasdrv_handle_init();

	if (my_getopt(argc -1, argv + 1) != 0) {
		fprintf(stderr, "Type 'RASCPY -?' to help.\n");
		exit(1);
	}
	if (!cmd || optHelp) {
		usage();
		exit(!optHelp);
	}

	rcint = find_rasdrv(optI);
	if (rcint < 0) {
		fprintf(stderr, "no RaSCSI bridge device.\n");
		return 1;
	}
	
	current_year = get_current_year();
	
	/* setup_rasdrv_t(h_rasdrv); */
	
	rc = rasdrvFsInitDevice(h_rasdrv, NULL);
	rcint = RASDRV_ISERROR(rc);
	
	if (rcint == 0) {
		char *p1 = NULL, *p2 = NULL;
		
		if (cmd_arg) p1 = *cmd_arg;
		if (p1) p2 = cmd_arg[1];
		if (strcmp(cmd, "dir")==0 || strcmp(cmd, "ls")==0) {
			rcint = (cmd_dir(p1 ? p1 : "/", cmd_opt) < 0);
		}
		if (p1 && strcmp(cmd, "get")==0) {
			rcint = cmd_get(p1, p2, cmd_opt);
		}
		if (p1 && strcmp(cmd, "put")==0) {
			rcint = cmd_put(p1, p2, cmd_opt);
		}
	}
	
	aspiExit();
	return rcint;
}
