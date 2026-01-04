#ifndef MSG_H
#define MSG_H

typedef enum {
    MSG_WIFI_CONNECTED,
    MSG_WIFI_DISCONNECTED,
    MSG_DPP_URI_READY,
    MSG_DPP_FAILED
} msg_type_t;

typedef struct {
    msg_type_t type;
    void *data;
    int data_len;
} msg_t;

#endif
