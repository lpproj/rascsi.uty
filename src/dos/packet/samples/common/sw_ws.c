
/* include Windows header at first for some compilers */
#if defined(_WIN32) || defined(WIN32) || defined(_WINDOWS) || defined(_WINDOWS_)
# ifndef USE_WINSOCK1
#   include <winsock2.h>
#   include <ws2tcpip.h>
# endif
# include <winsock.h>
# define HAVE_WINSOCK  1
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swrap.h"

#ifdef __cplusplus
extern "C" {
#endif
struct sockmodule * SW_API  sw_init_ws(void);
int SW_API  sw_exit_ws(void);

static const char *  SW_API  sw_get_module_description(int n);
static long SW_API  sw_get_default_value(int index);
static long SW_API  sw_set_default_timeout(long);
static SOCKDESC SW_API  sw_tcp_connect(const char *, unsigned);
static int SW_API  sw_recv(SOCKDESC, void *, int);
static int SW_API  sw_send(SOCKDESC, const void *, int);
static int SW_API  sw_shutdown(SOCKDESC, int);
static int SW_API  sw_close(SOCKDESC);


static struct sockmodule  sockmodule_ws = {
    sw_get_module_description,
    sw_init_ws,
    sw_exit_ws,
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




static WSADATA wsadata;

static
unsigned long  resolve_ipv4(const char *host)
{
    unsigned long a = inet_addr(host);
    struct hostent *h;

    if (a != 0 && a != INADDR_NONE) return a;
    h = gethostbyname(host);
    a = INADDR_NONE;
    if (h->h_addrtype == AF_INET) {
        a = *(u_long *)(h->h_addr);
    }

    return a;
}

static
int  my_recv(SOCKET s, void *buf, int length, int flags)
{
    int rc;

    WSASetLastError(0);
    while(1) {
        rc = recv(s, buf, length, flags);
        if (rc > 0 || WSAGetLastError() != WSAEWOULDBLOCK) break;
        Sleep(0);
    }

    return rc;
}

static
int  my_send(SOCKET s, const void *buf, int length, int flags)
{
    int rc;

    WSASetLastError(0);
    while(1) {
        rc = send(s, buf, length, flags);
        if (rc > 0 || WSAGetLastError() != WSAEWOULDBLOCK) break;
        Sleep(0);
    }

    return rc;
}


static
const char *
sw_get_module_description(int n)
{
    const char *s = NULL;
    switch(n) {
        case 0: s = "Winsock"; break;
    }
    return s;
}


struct sockmodule *
sw_init_ws(void)
{
#ifdef USE_WINSOCK1
    WORD wVer = MAKEWORD(1, 0);
#else
    WORD wVer = MAKEWORD(2, 0);
#endif

    if (WSAStartup(wVer, &wsadata) != 0) return NULL;

    return &sockmodule_ws;
}


int
sw_exit_ws(void)
{
    int rc = 0;

    if (wsadata.wVersion) {
#ifdef USE_WINSOCK1
        if (wsadata.wVersion == MAKEWORD(1, 1)) {
            WSACancelBlockingCall();
        }
#endif
        rc = WSACleanup();
        wsadata.wVersion = 0;
    }
    return rc;
}


static
long
sw_get_default_value(int index)
{
    switch(index) {
        default:
            break;
    }
    return 0;
}


static
long
sw_set_default_timeout(long ms)
{
    return ms;
}

static
SOCKDESC
sw_tcp_connect(const char *host, unsigned port)
{
    struct sockaddr_in saddr;
    SOCKDESC sd;
    SOCKET s;

    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    if ((saddr.sin_addr.S_un.S_addr = resolve_ipv4(host)) == INADDR_NONE) return NULL;
    sd = sw_prepare_new_sockdesc(SD_TCP);
    if (!sd) return sd;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
        u_long iomode = 0;
        ioctlsocket(s, FIONBIO, &iomode);   /* guess not needed */
        if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == 0) {
            sd->m = &sockmodule_ws;
            sd->sd_type = SD_TCP;
            sd->desc.s = s;
            return sd;
        }
        closesocket(s);
    }
    sw_remove_sockdesc(sd);
    return NULL;
}


static
int
sw_recv(SOCKDESC sd, void *buf, int length)
{
    int len = 0;
    fd_set fds;
    struct timeval tv;
    int n;
    SOCKET s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    s = sd->desc.s;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    
    sd->is_timeout = 0;
    tv.tv_sec = sd->timeout_ms / 1000;
    tv.tv_usec = sd->timeout_ms * 1000;
    
    n = select(1 /* + (int)s */, &fds, NULL, NULL, sd->timeout_ms > 0 ? &tv : NULL);
    if (n > 0) {
        if (FD_ISSET(s, &fds)) {
            len = my_recv(s, buf, length, 0);
        }
        else {
            sd->is_timeout = !0;
        }
    }
    else if (n == 0) {
        sd->is_timeout = !0;
    }

    if (n < 0 || len < 0) {
        sd->error = WSAGetLastError();
    }

    return len;

}


static
int
sw_send(SOCKDESC sd, const void *buf, int length)
{
    int len = 0;
    fd_set fds;
    struct timeval tv;
    SOCKET s;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    s = sd->desc.s;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    
    tv.tv_sec = sd->timeout_ms / 1000;
    tv.tv_usec = sd->timeout_ms * 1000;

    if (select(1 /* + (int)s */, NULL, &fds, NULL, sd->timeout_ms > 0 ? &tv : NULL) > 0) {
        if (FD_ISSET(s, &fds)) {
            len = my_send(s, buf, length, 0);
        }
        else {
            sd->is_timeout = !0;
        }
    }

    if (len <= 0) {
        sd->error = WSAGetLastError();
    }

    return len;

}

static
int
sw_close(SOCKDESC sd)
{
    SOCKET s;

    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    s = sd->desc.s;
    if (s != INVALID_SOCKET) {
        sw_shutdown(sd, 1 /* SD_SEND */);
        closesocket(s);
    }

    return sw_remove_sockdesc(sd);
}

static
int
sw_shutdown(SOCKDESC sd, int how)
{
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    
    return shutdown(sd->desc.s, how);

}

