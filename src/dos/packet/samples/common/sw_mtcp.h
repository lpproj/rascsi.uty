#ifndef SOCKWRAP_MD_H
#define SOCKWRAP_MD_H

#ifdef __cplusplus
extern "C" {
#endif

#define SOCKWRAP_PLATFORM  "mTCP"

typedef union mtcpSocket {
    void *value;
#if defined(BUILD_SOCKWRAP_MTCP)
    TcpSocket *tcp;
#endif
} mtcpSocket;

struct SOCKDESC_mtcp {
    int sd_type;
    mtcpSocket sd;
    int error;
    int is_timeout;
    long timeout_ms;
    void *send_buf;
    void *recv_buf;
    unsigned long tick_begin;
};

typedef struct SOCKDESC_mtcp *  SOCKDESC;


#ifdef __cplusplus
}
#endif

#endif

