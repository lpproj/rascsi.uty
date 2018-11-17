#include "swrap.h"

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int initialized = 0;
static int default_timeout_ms = 0;
static WSADATA wsadata;

static struct SOCKDESC_win  sd_slots[SOCKDESC_MAX];

#define LINE  { fprintf(stderr, "line %u\n", (unsigned)(__LINE__)); };

int swInit(void)
{
#ifdef USE_WINSOCK1
    WORD wVer = MAKEWORD(1, 0);
#else
    WORD wVer = MAKEWORD(2, 0);
#endif

    if (initialized <= 0) {
        if (WSAStartup(wVer, &wsadata) != 0) {
        return -1;
        }
        ++initialized;
    }
    return 0;
}

int swExit(void)
{
    int rc = 0;

    if (initialized) {
#ifdef USE_WINSOCK1
        if (wsadata.wVersion == MAKEWORD(1, 1)) {
            WSACancelBlockingCall();
        }
#endif
        rc = WSACleanup();
        initialized = 0;
    }
    return rc;
}

long swSetDefaultTimeout(long ms)
{
    long rc = default_timeout_ms;
    default_timeout_ms = (ms > 0) ? ms : 0;
    return rc;
}



static
SOCKDESC  new_socket_slot(int socktype)
{
    SOCKDESC sd = NULL;
    unsigned n;
    for(n=0; n<SOCKDESC_MAX; ++n) {
        if (sd_slots[n].sd_type == SD_EMPTY) {
            sd = &(sd_slots[n]);
            sd->sd_type = socktype;
            sd->timeout_ms = default_timeout_ms;
            break;
        }
    }
    return sd;
}

static
int  delete_socket(SOCKDESC sd)
{
    if (!sd) return -1;
    sd->sd = INVALID_SOCKET;
    sd->sd_type = 0;
    return 0;
}

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

#if 0
static int setTimeout(SOCKET s, long timeout_ms)
{
    DWORD dw = timeout_ms > 0 ? (DWORD)timeout_ms : 0;
    int rc;
    
    rc = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const void *)&dw, sizeof(dw));
    if (rc == 0) {
        rc = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const void *)&dw, sizeof(dw));
    }
    return rc;
}
#endif


SOCKDESC  swTcpConnect(const char *host, unsigned port)
{
    struct sockaddr_in saddr;
    SOCKDESC sd;
    SOCKET s;

    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    if ((saddr.sin_addr.S_un.S_addr = resolve_ipv4(host)) == INADDR_NONE) return NULL;
    sd = new_socket_slot(SD_TCP);
    if (!sd) return sd;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) {
        u_long iomode = 0;
        ioctlsocket(s, FIONBIO, &iomode);   /* guess not needed */
        if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == 0) {
            sd->sd = s;
            sd->sd_type = SD_TCP;
            return sd;
        }
        closesocket(s);
    }
    delete_socket(sd);
    return NULL;
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


#if 1
int  swRecv(SOCKDESC sd, void *buf, int length)
{
    int len = 0;
    fd_set fds;
    struct timeval tv;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    FD_ZERO(&fds);
    FD_SET(sd->sd, &fds);
    
    sd->is_timeout = 0;
    tv.tv_sec = sd->timeout_ms / 1000;
    tv.tv_usec = sd->timeout_ms * 1000;
    
    if (select(1 /* + (int)(sd->sd) */, &fds, NULL, NULL, sd->timeout_ms > 0 ? &tv : NULL) > 0) {
        if (FD_ISSET(sd->sd, &fds)) {
            len = my_recv(sd->sd, buf, length, 0);
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


int  swSend(SOCKDESC sd, const void *buf, int length)
{
    int len = 0;
    fd_set fds;
    struct timeval tv;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    FD_ZERO(&fds);
    FD_SET(sd->sd, &fds);
    
    tv.tv_sec = sd->timeout_ms / 1000;
    tv.tv_usec = sd->timeout_ms * 1000;

    if (select(1 /* + (int)(sd->sd) */, NULL, &fds, NULL, sd->timeout_ms > 0 ? &tv : NULL) > 0) {
        if (FD_ISSET(sd->sd, &fds)) {
            len = my_send(sd->sd, buf, length, 0);
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


#else
int  swRecv(SOCKDESC sd, void *buf, int length)
{
    int len;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;

    len = recv(sd->sd, buf, length, 0);
    if (len <= 0) {
        sd->error = WSAGetLastError();
    }

    return len;
}


int  swSend(SOCKDESC sd, const void *buf, int length)
{
    int len;
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    
    len = send(sd->sd, buf, length, 0);
    if (len <= 0) {
        sd->error = WSAGetLastError();
    }

    return len;
}
#endif


static
int
swShutDown(SOCKDESC sd, int how)
{
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    
    return shutdown(sd->sd, how);
}


int  swClose(SOCKDESC sd)
{
    if (!sd || !(sd->sd_type == SD_TCP || sd->sd_type == SD_UDP)) return -1;
    if (sd->sd != INVALID_SOCKET) {
        swShutDown(sd, 1 /* SD_SEND */);
        closesocket(sd->sd);
    }
    
    return delete_socket(sd);
}
