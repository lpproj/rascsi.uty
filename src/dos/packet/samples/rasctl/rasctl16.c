/*
    rasctl16: yet another rasctl



*/

#include "swrap.h"

#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(YA_GETOPT_DOS83_FILENAME)
#include "yagetopt.h"
#else
#include "ya_getopt.h"
#endif


static int  xstrcasecmp(const char *s0, const char *s1)
{
    int rc;
    
    while(1) {
        int c0 = *s0;
        int c1 = *s1;
        rc = toupper(c0) - toupper(c1);     /* note: not take care of DBCS  */
        if (rc != 0 || (c0 == '\0' && c1 == '\0')) break;
        ++s0;
        ++s1;
    }
    return rc;
}


#define RASCTL_SEND_BUFFER_SIZE 2048
char *Host = "127.0.0.1";
unsigned Port = 6868;
char *send_buf = NULL;

enum {
    CMD_ATTACH = 0,
    CMD_DETACH,
    CMD_INSERT,
    CMD_EJECT,
    CMD_PROTECT
};

enum {
    DEVICE_HD = 0,
    DEVICE_HD_SASI = 0,
    DEVICE_HD_SCSI = 1,
    DEVICE_MO = 2,
    DEVICE_MO_SCSI = 2,
    DEVICE_CD = 3,
    DEVICE_CD_SCSI = 3,
    DEVICE_BRIDGE = 4
};


int  check_image_type(const char *fname)
{
    int rc = -1;
    const char *ext;

    if (!fname || fname[0] == '\0') return -1;
    ext = strrchr(fname, '.');
    if (ext) {
        struct {
            char *ext;
            int type;
        } imgtype[] = {
            { "hdf", DEVICE_HD_SASI },
            { "hds", DEVICE_HD_SCSI },
            { "hdn", DEVICE_HD_SCSI },
            { "hdi", DEVICE_HD_SCSI },
            { "nhd", DEVICE_HD_SCSI },
            { "mos", DEVICE_MO },
            { "iso", DEVICE_CD },
            { NULL, -1 }
        };
        unsigned n;
        ++ext;
        for(n=0; imgtype[n].ext; ++n) {
            if (xstrcasecmp(ext, imgtype[n].ext) == 0) {
                rc = imgtype[n].type;
                break;
            }
        }
        
    }

    return rc;
}


int get_result(SOCKDESC sd, FILE *fo)
{
    int recv_cnt = 0;
    
    while(1) {
        char buf[128];
        int rc = swRecv(sd, buf, sizeof(buf) - 1);
        if (rc == 0) break;
        if (rc == -1) {
            fprintf(stderr, "error: error in swRecv().\n");
            break;
        }
        buf[rc] = '\0';
        fprintf(fo, "%s", buf);
        recv_cnt += rc;
        if (sd->is_timeout) {
            fprintf(stderr,"error: recv timeout.\n");
            break;
        }
    }
    
    return recv_cnt;
}

int send_cmd(SOCKDESC sd, const void *buf, int total_len)
{
    int send_cnt = 0;

    while(send_cnt < total_len) {
        int rc = swSend(sd, (const char *)buf + send_cnt, total_len - send_cnt);
        if (rc == -1) {
            fprintf(stderr, "error: error in swSend().\n");
            return -1;
        }
        send_cnt += rc;
        if (sd->is_timeout) {
            fprintf(stderr,"error: send timeout.\n");
            break;
        }
    }

    return send_cnt;
}


int  send_and_recv_cmd(const void *send_buf, FILE *fo)
{
    int rc;
    SOCKDESC sd;

    sd = swTcpConnect(Host, Port);
    if (!sd) {
        fprintf(stderr, "error: fail to connect %s:%u.\n", Host, Port);
        return -1;
    }
    
    rc = send_cmd(sd, send_buf, strlen(send_buf));
    if (rc >= 0) rc = get_result(sd, fo);

    swClose(sd);
    
    return rc;
}


void  put_banner(void)
{
    fprintf(stderr, "Yet another rasctl (for %s", SOCKWRAP_PLATFORM);
    fprintf(stderr, ", built at " __DATE__ " " __TIME__ ")\n");
}

void  put_help(void)
{
    const char msg[] =
        "Usage: rasctl [-h ADDR] [-p PORT] -i ID -c CMD [-t TYPE] [-f FILE] [-l]\n"
        "  or:  rasctl -l\n"
        "\n"
        "  -h ADDR  IPv4 addr for rascsi (127.0.0.1 as default)\n"
        "  -p PORT  IPv4 port for rascsi (6868 as default)\n"
        "  -i ID    device ID (0..6)\n"
        "  -c CMD   command for the ID:\n"
        "             attach    assign a device\n"
        "             detach    remove device (and a medium)\n"
        "             insert    insert a medium to the device\n"
        "             eject     eject a medium in the drive\n"
        "             protect   write-protect for the medium\n"
        "  -t TYPE  device type:\n"
        "             hd        HDD (SASI/SCSI)\n"
        "             mo        3.5 MO drive\n"
        "             cd        CD drive\n"
        "             bridge    Bridge device (RASETHER, RASDRV)\n"
        "  -l       list all devices\n";

    printf("%s", msg);
}


int  prepare_command(void *buf, int buflen, int dev_id, int cmd, int type, const char *img_name)
{
    size_t img_namelen;

    if (!buf || buflen < 8) return -1;
    img_namelen = img_name ? strlen(img_name) : 0;
    if (img_namelen > buflen - 8) {
        fprintf(stderr, "error: image filename too long.\n");
        return -1;
    }
    
    if (dev_id < 0 || dev_id >=7) {
        fprintf(stderr, "error: invalid ID.\n");
        return -1;
    }
    switch(cmd) {
        case CMD_ATTACH:
            switch(type) {
                case DEVICE_HD_SASI:
                case DEVICE_HD_SCSI:
                case DEVICE_MO:
                case DEVICE_CD:
                    if (!img_name) {
                        fprintf(stderr, "error: image file not specified.\n");
                        return -1;
                    }
                    break;
                case DEVICE_BRIDGE:
                    break;
                case -1:
                    if (img_name) {
                        type = check_image_type(img_name);
                        break;
                    }
                    /* fallthrough */
                default:
                    return -1;
            }
            break;
        case CMD_INSERT:
            if (img_name) {
                if (type < 0) type = 0;
                break;
            }
            fprintf(stderr, "error: image file not specified.\n");
            return -1;
        case CMD_DETACH:
        case CMD_EJECT:
        case CMD_PROTECT:
            if (type < 0) type = 0;
            break;
        default:
            fprintf(stderr, "error: invalid command.\n");
            return -1;
    }
    
    sprintf((char *)buf, "%c %c %c %s\n",
            '0' + dev_id,
            '0' + cmd,
            '0' + type,
            img_name ? img_name : "-");

    return 0;
}


int  main(int argc, char *argv[])
{
    int  rc = 0;
    int  cmd = -1;
    int  dev_id = -1;
    int  preferred_type = -1;
    char *imgfile = NULL;
    int  do_list = 0;
    int  do_help = 0;
    int  opt_error = 0;

    send_buf = sw_malloc(RASCTL_SEND_BUFFER_SIZE);
    send_buf[0] = '\0';

    opterr = 0;

    while(1) {
        int opt = getopt(argc, argv, "h:p:i:c:t:f:l");
        if (opt == -1) break;
        switch(toupper(opt)) {
            case 'H':
                Host = optarg;
                break;
            case 'P':
                Port = atoi(optarg);
                break;
            case 'I':
                dev_id = atoi(optarg);
                if (dev_id == 0 && optarg[0] != '0') dev_id = -1;
                break;
            case 'C':
                switch(toupper(optarg[0])) {
                    case 'A': cmd = CMD_ATTACH; break;
                    case 'D': cmd = CMD_DETACH; break;
                    case 'I': cmd = CMD_INSERT; break;
                    case 'E': cmd = CMD_EJECT; break;
                    case 'P': cmd = CMD_PROTECT; break;
                }
                break;
            case 'T':
                switch(toupper(optarg[0])) {
                    case 'S': preferred_type = DEVICE_HD_SASI; break;
                    case 'H': preferred_type = DEVICE_HD_SCSI; break;
                    case 'M': preferred_type = DEVICE_MO; break;
                    case 'C': preferred_type = DEVICE_CD; break;
                    case 'B': preferred_type = DEVICE_BRIDGE; break;
                    
                }
                break;
            case 'F':
                imgfile = optarg;
                break;
            case 'L':
                do_list = !0;
                break;
            case '?':
                if (optopt == '?') {
                    do_help = !0;
                    break;
                }
                /* fallthrough */
            default:
                opt_error = !0;
                break;
        }
    }

#if 0
    printf("id:%d cmd:%d type:%d list:%d help:%d err:%d file:%s\n", dev_id, cmd, preferred_type, do_list, do_help, opt_error, imgfile ? imgfile : "(none)");
#endif

    if (!opt_error && do_help) {
        put_banner();
        put_help();
        return 0;
    }

    if (opt_error || (cmd < 0 && preferred_type < 0 && !do_list)) {
        put_banner();
        printf("Invalid command.\n");
        printf("Type \"rasctl -?\" to help.\n");
        return 1;
    }

    rc = swInit();
    if (rc < 0) {
        fprintf(stderr, "fatal: socket initialization failure.\n");
        return rc;
    }
    swSetDefaultTimeout(5000);
    
    if (cmd >= 0) {
        rc = prepare_command(send_buf, RASCTL_SEND_BUFFER_SIZE, dev_id, cmd, preferred_type, imgfile);
        if (rc == 0) {
            rc = send_and_recv_cmd(send_buf, stdout);
        }
    }
    if (do_list) {
        strcpy(send_buf, "list\n");
        rc = send_and_recv_cmd(send_buf, stdout);
    }
    
    swReleaseModule();
    
    return rc;
}

