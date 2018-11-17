/*


*/

/* include Windows header at first for some compilers */
#if defined(_WIN32) || defined(WIN32) || defined(_WINDOWS) || defined(_WINDOWS_)
# ifndef USE_WINSOCK1
#   include <winsock2.h>
#   include <ws2tcpip.h>
# endif
# include <winsock.h>
#endif


#if (deifned(DOS) || defined(MSDOS) || defined(__DOS__) || defined(TEEN) || defined(MTCP))
# include <dos.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define BUILD_SOCKWRAP
#include "swrap.h"

static struct sockdesc_body  sd_slots[SOCKDESC_MAX];



static struct sockmodule *default_module;
static long default_timeout_ms;

int
swInitModule(struct sockmodule * (*initfunc)(void))
{
    struct sockmodule *m = initfunc();
    if (m) {
        default_module = m;
    }
    return m ? 0 : -1;
}

int
swReleaseModule(void)
{
    int rc = 0;
    
    if (default_module) {
        struct sockmodule *m = default_module;
        default_module = NULL;
        rc = m->exit();
    }
    return rc;
}

const char *
swGetModuleId(void)
{
    if (!default_module) return NULL;
    return default_module->get_module_description(0);
}


long
swSetDefaultTimeout(long millisecond)
{
    if (default_module) {
        default_timeout_ms = default_module->set_default_timeout(millisecond);
        return default_timeout_ms;
    }
    return -1L;
}


SOCKDESC
swTcpConnect(const char *hostname, unsigned port)
{
    if (!default_module) return NULL;
    
    return default_module->tcp_connect(hostname, port);
}


int
swClose(SOCKDESC sd)
{
    if (!sd) return -1;
    return sd->m->close(sd);
}

int
swRecv(SOCKDESC sd, void *buffer, int length)
{
    if (!sd) return -1;
    return sd->m->recv(sd, buffer, length);
}

int
swSend(SOCKDESC sd, const void *buffer, int length)
{
    if (!sd) return -1;
    return sd->m->send(sd, buffer, length);
}


int
swShutDown(SOCKDESC sd, int how)
{
    if (!sd) return -1;
    return sd->m->shutdown(sd, how);
}


/*
    init dispatcher
*/


int
swInit(void)
{
#ifdef HAVE_WINSOCK
    extern struct sockmodule * SW_API  sw_init_ws(void);
#endif
#ifdef MTCP
    extern struct sockmodule * SW_API  sw_init_mtcp(void);
#endif
#ifdef TEEN
    extern struct sockmodule * SW_API  sw_init_teen(void);
#endif
    struct sockmodule *m = NULL;

#ifdef HAVE_WINSOCK
    if (!m) m = sw_init_ws();
#endif
#ifdef TEEN
    if (!m) m = sw_init_teen();
#endif
#ifdef MTCP
    if (!m) m = sw_init_mtcp();
#endif

    if (m) sw_set_default_module(m);

    return m ? 0 : -1;
}




/*
    helper
*/

void * sw_malloc_l(unsigned s, unsigned l)
{
    void *p;

    if (s == 0) s = 1;
    p = malloc(s);
    if (!p) {
        fprintf(stderr, "fatal: memory allocation failure (%u bytes at line %u\n", s, l);
        exit(-1);
    }
    memset(p, 0, s);
    return p;
}


void sw_free(void *mem)
{
    if (mem) free(mem);
}


SOCKDESC
sw_prepare_new_sockdesc(int socktype)
{
    SOCKDESC sd = NULL;
    unsigned n;
    for(n=0; n<SOCKDESC_MAX; ++n) {
        if (sd_slots[n].sd_type == SD_EMPTY) {
            sd = &(sd_slots[n]);
            sd->sd_type = socktype;
            sd->timeout_ms = default_timeout_ms;
            sd->m = default_module;
            break;
        }
    }
    return sd;
}

int
sw_remove_sockdesc(SOCKDESC sd)
{
    if (!sd) return -1;
    sd->desc.l = -1;
    sd->sd_type = SD_EMPTY;
    if (sd->send_buf) { sw_free(sd->send_buf); sd->send_buf = NULL; }
    if (sd->recv_buf) { sw_free(sd->recv_buf); sd->recv_buf = NULL; }
    return 0;
}

struct sockmodule *
sw_set_default_module(struct sockmodule *m)
{
    struct sockmodule *m_org = default_module;
    default_module = m;
    return m_org;
}


int
sw_allocate_sock_buffer(SOCKDESC sd)
{
    void *pbs = NULL;
    void *pbr = NULL;
    
    if (default_module && default_module->get_default_value) {
        size_t nbs = 0, nbr = 0;
        nbs = default_module->get_default_value(SW_DEFAULT_SEND_BUFFER);
        nbr = default_module->get_default_value(SW_DEFAULT_RECV_BUFFER);
        if (nbs) pbs = sw_malloc(nbs);
        if (nbr) pbr = sw_malloc(nbr);
        if ((nbs && !pbs) || (nbr && !pbr)) {
            if (pbs) sw_free(pbs);
            if (pbr) sw_free(pbr);
            return -1;
        }
    }
    sd->send_buf = pbs;
    sd->recv_buf = pbr;

    return 0;
}



#if (defined(MSDOS) || defined(__DOS__) || defined(TEEN) || defined(MTCP))

unsigned long
sw_get_tickms_dos(void)
{
    union REGS r;

    r.h.ah = 0x2c;
    intdos(&r, &r);
    
    return (unsigned long)((unsigned)(r.h.ch) * 60U + r.h.cl) * 60000U + 
    ((unsigned)(r.h.dh) * 1000U + (unsigned)(r.h.dl) * 10U);
}

unsigned long
sw_diff_tickms_dos(unsigned long ms_now, unsigned long ms_past)
{
    if (ms_now >= ms_past) return ms_now - ms_past;
    return ms_now + 86400000UL - ms_past;
}

void
sw_yield_dos(void)
{
    union REGS r;
    int86(0x28, &r, &r);
}


int
sw_kbhit_dos(void)
{
    union REGS r;
    r.x.ax = 0x0b00;
    intdos(&r, &r);
    return r.h.al;
}
#endif



static unsigned brkcnt = 0;

int
sw_is_break(void)
{
    return brkcnt;
}

#if defined(_WIN32)

static BOOL bHandlerInstalled = FALSE;

static
BOOL
WINAPI
sw_break_handler(DWORD dwType)
{
    switch(dwType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            brkcnt = 1;
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

int
sw_install_break_handler(void)
{
    if (!bHandlerInstalled) {
        bHandlerInstalled = SetConsoleCtrlHandler(sw_break_handler, TRUE);
    }
    return bHandlerInstalled;
}

int
sw_release_break_handler(void)
{
    if (bHandlerInstalled) {
        bHandlerInstalled = !SetConsoleCtrlHandler(sw_break_handler, FALSE);
    }
    return bHandlerInstalled;
}

#elif defined(DOS)
# if defined(__TURBOC__)
#   define SW_INTHANDLER_API interrupt
#   define my_getvect  getvect
#   define my_setvect  setvect
# else
#   define SW_INTHANDLER_API __interrupt __far
#   define my_getvect  _dos_getvect
#   define my_setvect  _dos_setvect
# endif
typedef void (SW_INTHANDLER_API *sw_inthandler_dos)(void);

static sw_inthandler_dos  break_handler_org = 0L;

static
void
SW_INTHANDLER_API
sw_break_handler(void)
{
    brkcnt = 1;
    /* ++brkcnt; */
}

int
sw_install_break_handler(void)
{
    if (!break_handler_org) {
        break_handler_org = (sw_inthandler_dos)my_getvect(0x23);
        my_setvect(0x23, sw_break_handler);
    }
    return 1;
}

int
sw_release_break_handler(void)
{
    if (break_handler_org) {
        my_setvect(0x23, break_handler_org);
        break_handler_org = (sw_inthandler_dos)(void far *)0L;
    }
    return 1;
}


#endif

