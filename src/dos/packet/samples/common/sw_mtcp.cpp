#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "types.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "udp.h"
#include "dns.h"
#include "tcp.h"
#include "tcpsockm.h"

#define BUILD_SOCKWRAP_MTCP
#include "swrap.h"

#define PORT_BASE 2048


#ifdef __cplusplus
extern "C" {
#endif
struct sockmodule * SW_API  sw_init_mtcp(void);
int SW_API  sw_exit_mtcp(void);

static const char * SW_API  sw_get_module_description(int n);
static long SW_API  sw_get_default_value(int index);
static long SW_API  sw_set_default_timeout(long);
static SOCKDESC SW_API  sw_tcp_connect(const char *, unsigned);
static int SW_API  sw_recv(SOCKDESC, void *, int);
static int SW_API  sw_send(SOCKDESC, const void *, int);
static int SW_API  sw_shutdown(SOCKDESC, int);
static int SW_API  sw_close(SOCKDESC);


static struct sockmodule  sockmodule_mtcp = {
    sw_get_module_description,
    sw_init_mtcp,
    sw_exit_mtcp,
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


static int default_timeout_ms = 0;
static unsigned send_buf_size = 512;
static unsigned recv_buf_size = 512;


static
const char *
sw_get_module_description(int n)
{
    const char *s = NULL;
    switch(n) {
        case 0: s = "mTCP"; break;
    }
    return s;
}

struct sockmodule *
SW_API
sw_init_mtcp(void)
{
    if (Utils::parseEnv() != 0 || Utils::initStack(1, TCP_SOCKET_RING_SIZE)) {
        fprintf(stderr, "mTCP: failed to initialize tcpip\n");
        return NULL;
    }

    return &sockmodule_mtcp;
}


int
SW_API
sw_exit_mtcp(void)
{
    Utils::endStack();
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
unsigned getSourcePort()
{
    static unsigned port = PORT_BASE;
    unsigned p = port;
    if ((++port) >= PORT_BASE + 256) port = PORT_BASE;
    return p;
}


static
TcpSocket *  tcp_connect(const char *host, int port, long timeout_ms)
{
    unsigned long prev_tick;
    int8_t rc;
    IpAddr_t ipaddr = { 0 };
    TcpSocket *sock;

    if (Dns::resolve(host, &(ipaddr[0]), 1) < 0) return 0;
    while(Dns::isQueryPending()) {
        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();
        Dns::drivePendingQuery();
        // sw_yield_dos();
    }
    if (Dns::resolve(host, &(ipaddr[0]), 0) < 0) {
        fprintf(stderr, "mTCP: Can't resolve %s\n", host);
        return 0;
    }
    sock = TcpSocketMgr::getSocket();
    if (!sock || sock->setRecvBuffer(recv_buf_size) != 0) {
        fprintf(stderr, "mTCP: Can't create a socket\n");
        return 0;
    }
    if (sock->connectNonBlocking(getSourcePort(), ipaddr, port)) return 0;
    prev_tick = sw_get_tickms_dos();
    do {
        PACKET_PROCESS_SINGLE;
        Tcp::drivePackets();
        Arp::driveArp();
        if (sock->isConnectComplete()) {
            return sock;
        }
        if (sock->isClosed()) break;
        sw_yield_dos();
    } while(1);

    return 0;
}

static
SOCKDESC
sw_tcp_connect(const char *host, unsigned port)
{
    SOCKDESC sd;
    TcpSocket *s;

    sd = sw_prepare_new_sockdesc(SD_TCP);
    if (!sd) return NULL;
    if (sw_allocate_sock_buffer(sd) < 0) {
        sw_remove_sockdesc(sd);
        return NULL;
    }
    
    sd->is_timeout = 0;
    s = tcp_connect(host, port, sd->timeout_ms);
    
    if (s) {
        sd->sd_type = SD_TCP;
        sd->desc.p = (void *)s;
        sd->m = &sockmodule_mtcp;
    }
    else {
        sw_remove_sockdesc(sd);
        sd = NULL;
    }

    return sd;
}


static
int
sw_recv(SOCKDESC sd, void *buf, int length)
{
    TcpSocket *s;
    long tick_begin;
    int len;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    if (sd->timeout_ms > 0) {
        sd->is_timeout = 0;
        tick_begin = sw_get_tickms_dos();
    }
    s = (TcpSocket *)(sd->desc.p);
    while(1) {
        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();
        len = s->recv((uint8_t *)buf, length);
        if (len > 0) break;
        if (len == 0 && s->isRemoteClosed()) break;
        if (len < 0) {
            sd->error = -1; /* somethin' wrong */
            break;
        }
        
        if (sd->timeout_ms > 0) {
            if (sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= sd->timeout_ms) {
                sd->is_timeout = !0;
                break;
            }
        }
        // sw_yield_dos();
    }

    return len;
}


static
int
sw_send(SOCKDESC sd, const void *buf, int length)
{
    long tick_begin;
    int len;
    TcpSocket *s;

    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    
    sd->is_timeout = 0;
    if (sd->timeout_ms > 0) {
        tick_begin = sw_get_tickms_dos();
    }
    s = (TcpSocket *)(sd->desc.p);
    while(1) {
        char st;
        len = s->send((uint8_t *)buf, length);
        if (len > 0) break;
        if (len < 0) {
            sd->error = -1; /* somethin' wrong */
            break;
        }
        if (sd->timeout_ms > 0) {
            if (sw_diff_tickms_dos(sw_get_tickms_dos(), tick_begin) >= sd->timeout_ms) {
                sd->is_timeout = !0;
                break;
            }
        }
        // sw_yield_dos();
    }

    return len;
}


static
int
sw_shutdown(SOCKDESC sd, int how)
{
    if (!sd) return -1;

    if (sd->sd_type == SD_TCP) {
        ((TcpSocket *)(sd->desc.p))->shutdown(how);
    }
    return 0;
}

static
int
sw_close(SOCKDESC sd)
{
    TcpSocket *s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    if ((s = (TcpSocket *)(sd->desc.p))) {
        /* s->shutdown(1); */
        s->close();
    }
    
    return sw_remove_sockdesc(sd);
}

