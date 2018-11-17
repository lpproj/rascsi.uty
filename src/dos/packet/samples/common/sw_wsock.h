#ifndef SOCKWRAP_MD_H
#define SOCKWRAP_MD_H

#ifndef USE_WINSOCK1
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <winsock.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_WINSOCK1
# define SOCKWRAP_PLATFORM  "Winsock"
#else
# define SOCKWRAP_PLATFORM  "Winsock2"
#endif

struct SOCKDESC_win {
    int sd_type;
    SOCKET sd;
    int error;
    int is_timeout;
    long timeout_ms;
};

typedef struct SOCKDESC_win *  SOCKDESC;


#ifdef __cplusplus
}
#endif

#endif

