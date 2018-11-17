#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teen.h"
#include "inet.h"
#include "ip.h"
#include "ns.h"
#include "tcp.h"

#define BUILD_SOCKWRAP_TEEN
#include "swrap.h"

static int default_timeout_ms = 0;
static unsigned send_buf_size = 512;
static unsigned recv_buf_size = 512;

#ifdef __cplusplus
extern "C" {
#endif
struct sockmodule * SW_API  sw_init_teen(void);
int SW_API  sw_exit_teen(void);

static const char * SW_API  sw_get_module_description(int n);
static long SW_API  sw_get_default_value(int index);
static long SW_API  sw_set_default_timeout(long);
static SOCKDESC SW_API  sw_tcp_connect(const char *, unsigned);
static int SW_API  sw_recv(SOCKDESC, void *, int);
static int SW_API  sw_send(SOCKDESC, const void *, int);
static int SW_API  sw_shutdown(SOCKDESC, int);
static int SW_API  sw_close(SOCKDESC);


static struct sockmodule  sockmodule_teen = {
    sw_get_module_description,
    sw_init_teen,
    sw_exit_teen,
    sw_get_default_value,
    sw_set_default_timeout,
    sw_tcp_connect,
    sw_recv,
    sw_send,
    sw_shutdown,
    sw_close
};

#ifdef __cplusplus
}
#endif


static
const char *
sw_get_module_description(int n)
{
    const char *s = NULL;
    switch(n) {
        case 0: s = "TEEN"; break;
    }
    return s;
}

struct sockmodule *
sw_init_teen(void)
{
    if (!teen_available()) return NULL;

    return &sockmodule_teen;
}

int
sw_exit_teen(void)
{
    return 0;
}


static
long
sw_get_default_value(int index)
{
    switch(index) {
        case SW_DEFAULT_SEND_BUFFER:
            return send_buf_size;
        case SW_DEFAULT_RECV_BUFFER:
            return recv_buf_size;
        default:
            break;
    }
    return 0;
}

static
long
sw_set_default_timeout(long ms)
{
    long rc = default_timeout_ms;
    default_timeout_ms = (ms > 0) ? ms : 0;
    return rc;
}


static
signed char  tcp_connect(TCPPRM *tp, long timeout_ms)
{
    long tick_begin;
    signed char sd;
    
    if (timeout_ms > 0) tick_begin = sw_get_tickms_dos();
    sd = tcp_activeopen(tp);
    if (sd != -1) {
        char st;
        while (1) {
            st = tcp_state(sd);
            if (st == TCP_OPEN || st == TCP_CLOSED) break;
            if (timeout_ms > 0 && sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= timeout_ms) {
                break;
            }
            sw_yield_dos();
        }
        if (st != TCP_OPEN) {
            tcp_close(sd);
            sd = -1;
        }
    }
    
    return sd;
}



static
SOCKDESC
sw_tcp_connect(const char *host, unsigned port)
{
    SOCKDESC sd;
    TCPPRM tp;
    signed char s;

    sd = sw_prepare_new_sockdesc(SD_TCP);
    if (!sd) return NULL;
    if (sw_allocate_sock_buffer(sd) < 0) {
        sw_remove_sockdesc(sd);
        return NULL;
    }
    memset(&tp, 0, sizeof(tp));
    tp.rbuf = sd->recv_buf;
    tp.rbufsize = recv_buf_size;
    tp.rbank = 0;
    tp.sbuf = sd->send_buf;
    tp.sbufsize = send_buf_size;
    tp.sbank = 0;
    tp.host = (char *)host;
    tp.peerport = (WORD)port;
    
    sd->is_timeout = 0;
    s = tcp_connect(&tp, sd->timeout_ms);
    
    if (s == -1) {
        sw_remove_sockdesc(sd);
        sd = NULL;
    }
    else {
        sd->sd_type = SD_TCP;
        sd->desc.i8 = s;
        sd->m = &sockmodule_teen;
    }

    return sd;
}



static
int
sw_recv(SOCKDESC sd, void *buf, int length)
{
    long tick_begin;
    int len;
    signed char s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    if (sd->timeout_ms > 0) {
        sd->is_timeout = 0;
        tick_begin = sw_get_tickms_dos();
    }
    s = sd->desc.i8;
    while(1) {
        char st;
        len = tcp_recv(s, buf, length);
        if (len > 0) break;
        st = tcp_state(s);
        if (len <= 0 && st != TCP_OPEN) {
            sd->error = tcperrno;
            break;
        }
        if (sd->timeout_ms > 0) {
            if (sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= sd->timeout_ms) {
                sd->is_timeout = !0;
                break;
            }
        }
        /* sw_yield_dos(); */
    }

    return len;
}


static
int
sw_send(SOCKDESC sd, const void *buf, int length)
{
    long tick_begin;
    int len;
    signed char s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    
    sd->is_timeout = 0;
    if (sd->timeout_ms > 0) {
        tick_begin = sw_get_tickms_dos();
    }
    s = sd->desc.i8;
    while(1) {
        char st;
        len = tcp_send(s, (char *)buf, length, 0);
        if (len > 0) break;
        st = tcp_state(s);
        if (len <= 0 && !(st == TCP_OPEN || st == TCP_OPENING || st == TCP_HISCLOSING)) {
            sd->error = tcperrno;
            break;
        }
        if (sd->timeout_ms > 0) {
            if (sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= sd->timeout_ms) {
                sd->is_timeout = !0;
                break;
            }
        }
        /* sw_yield_dos(); */
    }

    return len;
}


static
int
sw_shutdown(SOCKDESC sd, int how)
{
    signed char s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    if ((s = sd->desc.i8) != -1) {
        long tick_begin = -1;
        if (sd->timeout_ms > 0) {
            tick_begin = sw_get_tickms_dos();
        }
        if (how == 0 /* SD_RECEIVE */) return 0;
        if (tcp_shutdown(s) == (char)-1) return -1;
        while(1) {
            char st = tcp_state(s);
            if (st == TCP_CLOSED) break;
            if (st == (char)-1 || (tick_begin != -1 && sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= sd->timeout_ms)) return -1;
            sw_yield_dos();
        }
    }
    return 0;
}

static
int
sw_close(SOCKDESC sd)
{
    signed char s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    s = sd->desc.i8;
    if (s != -1) {
        swShutDown(sd, 1 /* SD_SEND */);
        tcp_close(s);
    }
    
    return sw_remove_sockdesc(sd);
}

