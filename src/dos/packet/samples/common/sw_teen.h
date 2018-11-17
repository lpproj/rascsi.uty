#ifndef SOCKWRAP_MD_H
#define SOCKWRAP_MD_H

#ifdef __cplusplus
extern "C" {
#endif

#define SOCKWRAP_PLATFORM  "TEEN"


struct SOCKDESC_teen {
    int sd_type;
    signed char sd;
    int error;
    int is_timeout;
    long timeout_ms;
    void *send_buf;
    void *recv_buf;
    unsigned long tick_begin;
};

typedef struct SOCKDESC_teen *  SOCKDESC;


#ifdef __cplusplus
}
#endif

#endif

