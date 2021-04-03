/*
DOS (ASPI/55BIOS) version of rasctl

author: lporoj (https://github.com/lpproj/)

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

#include "bitscsi.h"

#ifdef JAPANESE
# include "msg_ja.c"
#else
# include "msg_en.c"
#endif

static unsigned buffer_max = 2048;
static unsigned char *buffer;


void *mymalloc(size_t n)
{
    void *p;
    if (n == 0) n=1;
    p = malloc(n);
    if (!p) {
        fprintf(stderr, "FATAL: memory allocation failure\n");
        exit(-1);
    }
    memset(p, 0, n);
    return p;
}

void myfree(void *p)
{
    if (p) free(p);
}


/*

*/

enum {
    CMD_UNSPECIFIED = -1,
    CMD_ATTACH = 0,
    CMD_DETACH,
    CMD_INSERT,
    CMD_EJECT,
    CMD_PROTECT
};

enum {
    DRV_UNSPECIFIED = -1,
    DRV_DEFAULT = 0,
    DRV_HD = 0,
    DRV_MO = 2,
    DRV_CD,
    DRV_BRIDGE
};

typedef struct RASCTL {
    int id;
    int lun;
    int cmd;
    int drive_type;
    const char *file;
    int do_list;
    int do_list_after_cmd;
    int do_stop;
    int do_shutdown;
} RASCTL;



#define MAX_ARGS 16
#define MAX_OPTPARAMS 16
int optV = 1;
int optHelp;
char *myargv[MAX_ARGS];
char *prmA;
char *prmI;
char *prmLun;
char *prmC;
char *prmT;
char *prmF;
int optI = -1;
int optA = 0;
int optLun = 0;
int optList;
int optStop;
int optShutdown;
int optDryRun;
int optDispInq;

struct {
    char *shortopt;
    char *shortopt2;
    char *longopt;
    char **paramptr;
} paramopt[] = {
    { "-a", "/a", "--adapter", &prmA }, 
    { "-i", "/i", "--id", &prmI },
    { "-u", "/u", "--lun", &prmLun },
    { "-c", "/c", "--command", &prmC },
    { "-t", "/t", "--type", &prmT },
    { "-f", "/f", "--file", &prmF },
    { NULL, NULL, NULL }
};

static int my_strtoint(const char *s, int*i)
{
    long l;
    if (!s || !i) return -1;
    l = strtol(s, NULL, 0);
    if (l == LONG_MAX || l == LONG_MIN) return -1;
#if (INT_MAX < LONG_MAX)
    if (l > INT_MAX) return -1;
    if (l < INT_MIN) return -1;
#endif
    *i = (int)l;
    return 0;
}

/* strcasecmp */
static int my_sc(const char *s0, const char *s1)
{
    while(*s0 || *s1) {
        int rc = toupper((int)*(unsigned char *)s0) - toupper((int)*(unsigned char *)s1);
        if (rc != 0) return rc;
        ++s0;
        ++s1;
    }
    return 0;
}

/* strncasecmp */
static int my_snc(const void *m0, const void *m1, size_t len)
{
    int rc = 0;
    const unsigned char *s0 = m0;
    const unsigned char *s1 = m1;
    while(len && (*s0 || *s1)) {
        rc = toupper(*s0) - toupper(*s1);
        if (rc != 0) break;
        ++s0;
        ++s1;
        --len;
    }
    return rc;
}

static int ck_popt(const char *opt, const char *arg)
{
    if (opt) {
        int n = strlen(opt);
        if (my_snc(opt, arg, n) == 0) {
            if (isdigit(arg[n])) return n;
            if ((arg[n] == ':' || arg[n] == '=')) {
                ++n;
                return arg[n] ? n : 0;
            }
            if (arg[n] == '\0') return 0;
        }
    }
    return -1;
}

static int my_optcmp(const char *opt, const char *arg)
{
    if (*opt == '-' && opt[1] == '-') return my_sc(opt, arg);
    if (*arg == '/') return my_sc(opt +1, arg + 1);
    return my_sc(opt, arg);
}

int mygetopt(int argc, char *argv[])
{
    int rc = 0;
    int narg = 0;
    
    while(--argc > 0) {
        char *s = *++argv;
        int iop = 0;

        while(1) {
            int n;
            if (!paramopt[iop].shortopt && !paramopt[iop].longopt) {
                iop = -1;
                break;
            }
            n = ck_popt(paramopt[iop].shortopt, s);
            if (n == -1) n = ck_popt(paramopt[iop].shortopt2, s);
            if (n == -1) n = ck_popt(paramopt[iop].longopt, s);
            if (n > 0) {
                *(paramopt[iop].paramptr) = s + n;
                break;
            }
            if (n == 0) {
                if (argc >= 1) {
                    --argc;
                    *(paramopt[iop].paramptr) = *++argv;
                    break;
                }
            }
            ++iop;
        }

        if (iop < 0) {
            if (my_optcmp("-?", s)==0 || my_optcmp("-h", s)==0 || my_optcmp("--help", s)==0) optHelp = 1;
            else if (my_optcmp("--dry-run", s)==0) optDryRun = 1;
            else if (my_optcmp("--stop", s)==0) optStop = 1;
            else if (my_optcmp("--shutdown", s)==0) optShutdown = 1;
            else if (my_optcmp("--inquiry", s)==0) optDispInq = 1;
            else if (my_optcmp("-l", s)==0 || my_optcmp("--list", s)==0) optList = 1;
            else if (my_optcmp("-v", s)==0 || my_optcmp("--verbose", s)==0) ++optV;
            else if (my_optcmp("-q", s)==0 || my_optcmp("--quiet", s)==0) optV=0;
            else if (narg < MAX_ARGS) {
                myargv[narg++] = s;
            }
        }
    }
    my_strtoint(prmA, &optA);
    my_strtoint(prmI, &optI);
    my_strtoint(prmLun, &optLun);
    return rc < 0 ? rc : narg;
}


int send_command(int adapter, int bridge_id, void *buffer, unsigned tfr_recv_length)
{
    int rc;
    BS_CMDPKT pkt;

    bs_clear_cmdpkt(&pkt);
    pkt.cdb_length = 10;
    pkt.cdb[0] = 0x2a;
    bs_w16be(&(pkt.cdb[7]), tfr_recv_length);
    rc = bs_cmd(adapter, bridge_id, 0, &pkt, BS_TO_DEVICE, buffer, tfr_recv_length);
    if (rc >= 0) {
        memset(buffer, 0, tfr_recv_length);
        bs_clear_cmdpkt(&pkt);
        pkt.cdb_length = 10;
        pkt.cdb[0] = 0x28;
        bs_w16be(&(pkt.cdb[7]), tfr_recv_length);
        rc = bs_cmd(adapter, bridge_id, 0, &pkt, BS_FROM_DEVICE, buffer, tfr_recv_length);
    }
    if (rc < 0) {
        strcpy(buffer, err_scsi_trans);
        strcat(buffer, "\n");
    }
    return rc;
}


#if defined __STDC_VERSION__ || ((__STDC_VERSION__)>=199901L)
# define mysnprintf snprintf
# define with_buflim ,(buflim)
#else
# define mysnprintf sprintf
# define with_buflim
#endif
int build_command(char *buf, unsigned buflim, const RASCTL *ctl, int do_list)
{
    int n_wr;

    memset(buf, 0, buflim);
    if (ctl->do_stop || ctl->do_shutdown) {
        n_wr = mysnprintf(buf with_buflim, "%s\n", ctl->do_shutdown ? "shutdown" : "stop");
    }
    else if (do_list) {
        n_wr = mysnprintf(buf with_buflim, "list\n");
    }
    else {
        n_wr = mysnprintf
        (
            buf with_buflim,
            "%u %u %u %u %s\n",
            ctl->id,
            ctl->lun,
            ctl->cmd,
            ctl->drive_type == DRV_UNSPECIFIED ? DRV_DEFAULT : ctl->drive_type,
            (ctl->file && ctl->file[0]) ? ctl->file : "-"
        );
    }
    if (n_wr < 0 || (unsigned)n_wr >= buflim) {
        fprintf(stderr, "FATAL: buffer not enough (parameter too long)\n");
        exit(-1);
    }

    return n_wr;
}

int build_rasctl(RASCTL *ctl)
{
    int cmd_set = CMD_UNSPECIFIED;
    int type_set = DRV_UNSPECIFIED;
    int type_fall = DRV_UNSPECIFIED;
    char *file_set = prmF;
    int need_cmd;

    need_cmd = !optStop && !optShutdown && !optList || (optList && optI >= 0);
    if (prmC) {
        switch(toupper(*prmC)) {
            case 'A': cmd_set = CMD_ATTACH; break;
            case 'D': cmd_set = CMD_DETACH; file_set = NULL; break;
            case 'I': cmd_set = CMD_INSERT; break;
            case 'E': cmd_set = CMD_EJECT; file_set = NULL; break;
            case 'P': cmd_set = CMD_PROTECT; break;
            default:
                fprintf(stderr, "%s\n", err_invalid_cmd);
                return -1;
        }
    }
    if (prmT) {
        switch(toupper(*prmT)) {
            case 'S': case 'H': type_set = DRV_HD; break;
            case 'M': type_set = DRV_MO; break;
            case 'C': type_set = DRV_CD; break;
            case 'B': type_set = DRV_BRIDGE; file_set = NULL; break;
            default:
                fprintf(stderr, "%s\n", err_invalid_dev);
                return -1;
        }
    }
    if (prmF) {
        unsigned n = strlen(prmF);
        if (n >= 4 && prmF[n - 4] == '.') {
            char *ext = prmF + n - 3;
            if (my_sc(ext, "HDF")==0 ||
                my_sc(ext, "HDS")==0 ||
                my_sc(ext, "HDN")==0 ||
                my_sc(ext, "HDI")==0 ||
                my_sc(ext, "NHD")==0 ||
                my_sc(ext, "HDA")==0)
            {
                type_fall = DRV_HD;
            }
            else if (my_sc(ext, "MOS")==0) {
                type_fall = DRV_MO;
            }
            else if (my_sc(ext, "ISO")==0) {
                type_fall = DRV_CD;
            }
        }
    }
    if (need_cmd) {
        if (cmd_set == CMD_UNSPECIFIED) {
            fprintf(stderr, "%s\n", err_invalid_cmd);
            return -1;
        }
        if (optI < 0 || optI > 6) {
            fprintf(stderr, "%s\n", err_invalid_id);
            return -1;
        }
        if (optLun < 0 || optLun > 7) {
            fprintf(stderr, "%s\n", err_invalid_id);
            return -1;
        }
    }
    if (type_set == DRV_UNSPECIFIED) {
        type_set = type_fall;
    }
    if ((cmd_set == CMD_INSERT || (cmd_set == CMD_ATTACH && (type_set == DRV_UNSPECIFIED || type_set == DRV_HD))) && !prmF) {
        fprintf(stderr, err_file_not_specified_s, cmd_set == CMD_INSERT ? "insert" : "attach");
        fprintf(stderr, "\n");
        return -1;
    }

    ctl->do_stop = optStop;
    ctl->do_shutdown = optShutdown;

    if (need_cmd) {
        ctl->id = optI;
        ctl->lun = optLun;
        ctl->cmd = cmd_set;
        ctl->drive_type = type_set;
        ctl->file = file_set;
        ctl->do_list = 0;
        ctl->do_list_after_cmd = optList;
    }
    else {
        ctl->id = 0;
        ctl->lun = 0;
        ctl->cmd = 0;
        ctl->drive_type = 0;
        ctl->file = NULL;
        ctl->do_list = optList;
        ctl->do_list_after_cmd = 0;
    }

    return 0;
}


int search_rascsi(int adapter, int id_search_fixed, int *rascsi_revision, int disp_inquiry)
{
    int id_found = -1, id_most = 6, id_least = 0;
    int id;

    if (id_search_fixed >= id_least && id_search_fixed <= id_most) {
        id_most = id_least = id_search_fixed;
    }

    for (id = id_most; id >= id_least; --id) {
        BSu8 inq_buf[SIZE_OF_BS_INQUIRY_STD + 1];
        BS_INQUIRY_STD *inq = (BS_INQUIRY_STD *)&(inq_buf[0]);
        int rc;

        memset(inq_buf, 0, sizeof(inq_buf));
        rc = bs_inquiry(adapter, id, 0, inq, SIZE_OF_BS_INQUIRY_STD);
        if (rc < 0 || inq->vendor[0] == '\0') continue;
        if (disp_inquiry) printf("ID:%x Type:%02X '%s'", id, inq->dev_type, inq->vendor);
        if (memcmp(inq->product_id, "RASCSI BRIDGE", 13) == 0) {
            id_found = id;
            if (rascsi_revision) {
                *rascsi_revision = (int)strtol(inq->product_revision, NULL, 10);
                if (disp_inquiry) printf(" revision=%d", *rascsi_revision);
            }
            if (!disp_inquiry) break;
        }
        if (disp_inquiry) printf("\n");
    }

    return id_found;
}




void usage(void)
{
    printf("%s", msg_usage);
}

int main(int argc, char *argv[])
{
    int rc;
    int id_bridge = -1;
    RASCTL ctl;
    const unsigned trasnfer_length = 1024;

#ifdef DEFAULT_55BIOS
    if (bs_get_machine_dos() == BS_MACHINE_NEC98) {
        optA = 0xc0;
    }
#endif
    rc = mygetopt(argc, argv);
    if (optHelp) {
        usage();
        return 0;
    }
    buffer = mymalloc(buffer_max);
    rc = build_rasctl(&ctl);
    if (rc < 0) {
        fprintf(stderr, "%s\n", msg_to_help);
        return -1;
    }
    rc = build_command(buffer, buffer_max, &ctl, ctl.do_list);
    if (!optDryRun) {
        int revision = 0;
        id_bridge = search_rascsi(optA, -1, &revision, optDispInq);
        if (id_bridge < 0) {
            fprintf(stderr, "%s\n", err_bridge_not_found);
            return -1;
        }
        if (revision < 152) {
            fprintf(stderr, "%s\n", err_bridge_old);
            return -1;
        }
    }
    if (rc > 0) {
        if (optDryRun) {
            rc = printf("%s", buffer);
            buffer[0] = '\0';
        }
        else {
            rc = send_command(optA, id_bridge, buffer, trasnfer_length);
        }
        if (rc >= 0 && ctl.do_list_after_cmd) {
            memset(buffer, 0, buffer_max);
            strcpy(buffer, "list\n");
            if (optDryRun) {
                rc = printf("%s", buffer);
                buffer[0] = '\0';
            }
            else {
                rc = send_command(optA, id_bridge, buffer, trasnfer_length);
            }
        }
        if (rc >= 0) {
            printf("%s", buffer);
            if (my_snc(buffer, "ERR", 3)==0) {
                return 1;
            }
        }
    }
    else {
        return -1;
    }

    return rc < 0;
}

