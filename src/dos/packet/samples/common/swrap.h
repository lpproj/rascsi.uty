#ifndef SOCKWRAP_H
#define SOCKWRAP_H

#define SOCKDESC_MAX  20

#if defined(_WIN32) || defined(WIN32) || defined(_WINDOWS) || defined(_WINDOWS_)
# ifndef USE_WINSOCK1
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   define HAVE_WINSOCK  2
# else
#   define HAVE_WINSOCK  1
# endif
# include <winsock.h>
#endif


#if defined(HAVE_WINSOCK)
# if HAVE_WINSOCK >= 2
#   define SOCKWRAP_PLATFORM "Winsock2"
# else
#   define SOCKWRAP_PLATFORM "Winsock"
# endif
#elif defined(TEEN) && defined(MTCP)
# define SOCKWRAP_PLATFORM "TEEN/mTCP"
#elif defined(MTCP)
# define SOCKWRAP_PLATFORM "mTCP"
#elif defined(TEEN)
# define SOCKWRAP_PLATFORM "TEEN"
#endif



#define SW_DBGLINE  { fprintf(stderr, "line %u\n", (unsigned)(__LINE__)); };

#define SW_API cdecl


enum { SD_EMPTY = 0, SD_TCP, SD_UDP }; /* udp not supported yet */

union sockdesc_i {
    signed char i8;
    int i;
    long l;
    void *p;
#ifdef HAVE_WINSOCK
    HANDLE h;
    SOCKET s;
#endif
};

struct sockmodule;

struct sockdesc_body {
    int sd_type;
    union sockdesc_i desc;
    struct sockmodule *m;
    int error;
    int is_timeout;
    long timeout_ms;
    void *send_buf;
    void *recv_buf;
    unsigned long tick_begin;
};

typedef struct sockdesc_body *SOCKDESC;

enum {
    SW_DEFAULT_SEND_BUFFER = 1,
    SW_DEFAULT_RECV_BUFFER
};

struct sockmodule {
    const char * (SW_API *get_module_description)(int n);
    struct sockmodule * (SW_API *init)(void);
    int (SW_API *exit)(void);
    long (SW_API *get_default_value)(int index);
    long (SW_API *set_default_timeout)(long);
    SOCKDESC (SW_API *tcp_connect)(const char *, unsigned);
    int (SW_API *recv)(SOCKDESC, void *, int);
    int (SW_API *send)(SOCKDESC, const void *, int);
    int (SW_API *shutdown)(SOCKDESC, int);
    int (SW_API *close)(SOCKDESC);
};


#ifdef __cplusplus
extern "C" {
#endif

/* minimal API */

int SW_API  swInitModule(struct sockmodule * (*initfunc)(void));
int SW_API  swReleaseModule(void);
const char * SW_API  swGetModuleId(void);
long SW_API  swSetDefaultTimeout(long millisecond);

SOCKDESC SW_API  swTcpConnect(const char *hostname, unsigned port);
int SW_API  swClose(SOCKDESC);

int SW_API  swRecv(SOCKDESC, void *, int);
int SW_API  swSend(SOCKDESC, const void *, int);

int SW_API  swShutDown(SOCKDESC, int);

int SW_API  swInit(void);


/*
    helper functions for module
*/

SOCKDESC SW_API  sw_prepare_new_sockdesc(int socktype);
int SW_API  sw_remove_sockdesc(SOCKDESC sd);
int SW_API  sw_allocate_sock_buffer(SOCKDESC sd);
struct sockmodule * SW_API  sw_set_default_module(struct sockmodule *m);
void * SW_API  sw_malloc_l(unsigned s, unsigned line);
void SW_API  sw_free(void *mem);
#define sw_malloc(s)  sw_malloc_l(s,__LINE__)

#if defined(DOS)
unsigned long SW_API  sw_get_tickms_dos(void);
unsigned long SW_API  sw_diff_tickms_dos(unsigned long ms_now, unsigned long ms_past);
void SW_API  sw_yield_dos(void);
int SW_API  sw_kbhit_dos(void);
#endif

int SW_API  sw_install_break_handler(void);
int SW_API  sw_release_break_handler(void);
int SW_API  sw_is_break(void);

#ifdef __cplusplus
}
#endif

#endif

