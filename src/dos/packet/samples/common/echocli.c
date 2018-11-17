/*
    echocli: a simple echo client.

*/


#include "swrap.h"

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#if !defined(_IOLBF)
# define _IOLBF  _IONBF
#endif
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(DOS) || defined(HAVE_WINSOCK)
# include <io.h>
#endif
#if defined(O_BINARY)
# define my_setmode  setmode
#else
# define O_BINARY 0
static int my_setmode(int fd, int mode) { (void)fd; (void)mode; return 0; }
#endif

#if !defined(ECHO_BUFFER_LENGTH)
# define ECHO_BUFFER_LENGTH 1024
#endif

int verbose = 1;
int binary_mode = 0;
static void * echo_buf = NULL;
static size_t echo_buflen = 0;

static
int
my_fsetmode(FILE *f, int mode)
{
    fflush(f);
    return my_setmode(fileno(f), mode);
}

static
size_t
my_fgetn(char *buf, size_t buflen, FILE *fi)
{
    size_t rlen = 0;

    if (buflen >= INT_MAX) buflen = INT_MAX - 1;
    while(rlen + 1 < buflen) {
        int c = fgetc(fi);
        if (c == EOF && sw_is_break()) break;
        if (buf) buf[rlen] = (char)c;
        ++rlen;
        if (c == '\n') break;
        if (binary_mode && c == '\r') break;
    }
    if (buf && buflen > 0) buf[rlen] = '\0';
    return rlen;
}

static
int sendall(SOCKDESC sd, const void *p, int len)
{
    const char *s = p;
    int tlen = 0;
    long tm_org;

    tm_org = sd->timeout_ms;
    sd->timeout_ms = 5001;  /* 5s at least */
    while(tlen < len) {
        int n = swSend(sd, s + tlen, len - tlen);
        if (n < 0 || (n == 0 && sd->is_timeout)) break;
        tlen += n;
    }
    sd->timeout_ms = tm_org;
    return tlen;
}

int echo_client(const char *host, unsigned port, FILE *fi, FILE *fo)
{
    int rc = 0;
    SOCKDESC sd;
    char ibuf[128];
    char *buf;
    size_t buflen;

    if (verbose) fprintf(stderr, "Connecting %s:%u...", host, port);
    sd = swTcpConnect(host, port);
    if (!sd) {
        fprintf(stderr, "error: fail to connect %s:%u.\n", host, port);
        return -1;
    }
    if (verbose) fprintf(stderr, "ok\n");
    
    if (!fi) fi = stdin;
    if (!fo) fo = stdout;

    
    buf = echo_buf;
    buflen = echo_buflen;
    if (!buf || !buflen) {
        buf = ibuf;
        buflen = sizeof(ibuf);
    }
    while(1) {
        int tcnt;
        
        if (verbose > 1) fprintf(stderr, "send phase...");
        tcnt = my_fgetn(buf, buflen-1, fi);
        if (tcnt <= 0) break; /* EOF or error */
        if (sw_is_break()) break; /* check ctrl-c */
        buf[tcnt] = '\0';   /* to be safe */
        if (verbose > 1) {
            fprintf(stderr, " sent %u chars : %s\n", tcnt, buf);
        }
        if (tcnt > sendall(sd, buf, tcnt)) {
            if (verbose) fprintf(stderr, "\nerror: error in swSend().\n");
            break;
        }
        if (sw_is_break()) break; /* check ctrl-c */
        if (verbose > 1) fprintf(stderr, "recv phase...");
        sd->timeout_ms = 1001; /* 1s at least */
        while(1) {
            tcnt = swRecv(sd, buf, buflen-1);
            if (tcnt > 0) {
                buf[tcnt] = '\0';
                if (verbose > 1) {
                    fprintf(stderr, "recieved %u chars : %s\n", tcnt, buf);
                }
                fprintf(fo, "%s", buf);
                fflush(fo);
                break;
            }
            if (tcnt < 0) {
                rc = -1;
                fprintf(stderr, "\nerror: error in swRecv().\n");
                break;
            }
            if (tcnt == 0) {
                if (sd->is_timeout) {
                    if (verbose > 1) {
                        fprintf(stderr, "recv timeout\n");
                    }
                    break;
                }
            }
            if (sw_is_break()) break; /* check ctrl-c */
#if defined(DOS)
            sw_yield_dos();
#endif
        }
        if (rc == -1) break;
    }
    /* swShutDown(sd, 2); */
    swClose(sd);
    if (verbose) {
        if (rc == 0 && sw_is_break()) {
            fprintf(stderr, "\nechocli: aborted by user.\n");
        }
    }

    return rc;
}



static
void banner(void)
{
    const char msg[] =
        "echocli - a simple echo client"
        " for " SOCKWRAP_PLATFORM
        " (built at " __DATE__ " " __TIME__ ")"
        ;
    printf("%s\n", msg);
}

static
void usage(void)
{
    const char msg[] =
        "Usage: echocli server_address [server_port]\n"
        ;
    banner();
    printf("%s", msg);
}


int optHelp;
char *server_addr_str = NULL;
char *server_port_str = NULL;
unsigned server_port;

static
int mygetopt(int argc, char *argv[])
{
    int rc = 0;
    while(argc > 0) {
        char *s = *argv;
        if (*s == '-' || *s == '/') {
            switch(*++s) {
                case 'B': case 'b':
                    binary_mode = 1;
                    break;
                case 'V': case 'v':
                    ++verbose;
                    break;
                case '?':
                case 'H': case 'h':
                    optHelp = 1;
                    break;
            }
        }
        else if (!server_addr_str) server_addr_str = s;
        else if (!server_port_str) server_port_str = s;
        
        --argc;
        ++argv;
    }
    if (!optHelp && server_addr_str) {
        long l;
        if (!server_addr_str) server_addr_str = "7";
        l = strtol(server_port_str, NULL, 0);
        if (l > 0 && l < 65535L) {
            server_port = (unsigned)l;
        }
        else {
            fprintf(stderr, "error: invalid port number.\n");
            rc = -1;
        }
    }
    return rc;
}


int main(int argc, char *argv[])
{
    int rc = 0;
    FILE *fi, *fo;
    int fmi, fmo;
    
    rc = mygetopt(argc - 1, argv + 1);
    if (rc < 0) return 1;

    if (optHelp) {
        usage();
        return 0;
    }
    if (!server_addr_str) {
        banner();
        printf("Type 'echocli -?' to help.\n");
        return 1;
    }

    echo_buflen = ECHO_BUFFER_LENGTH;
    echo_buf = sw_malloc(echo_buflen);

    rc = swInit();
    if (rc < 0) {
        fprintf(stderr, "fatal: socket initialization failure.\n");
        return rc;
    }
    swSetDefaultTimeout(5000);

    fi = stdin;
    fo = stdout;
    if (binary_mode) {
        if (verbose) fprintf(stderr, "binary mode.\n");
        setvbuf(fi, NULL, _IONBF, 0);
        setvbuf(fo, NULL, _IONBF, 0);
        fmi = my_fsetmode(fi, O_BINARY);
        fmo = my_fsetmode(fo, O_BINARY);
    }

    sw_install_break_handler();

    rc = echo_client(server_addr_str, server_port, fi, fo);

    sw_release_break_handler();

    if (binary_mode) {
        my_fsetmode(fi, fmi);
        my_fsetmode(fo, fmo);
    }

    swReleaseModule();

    return rc;
}

