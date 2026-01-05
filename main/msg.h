#ifndef MSG_H
#define MSG_H

#include "sensors_bme680.h"

typedef enum {
  GUI_MSG_SHOW_QR,
  GUI_MSG_HIDE_QR,
  GUI_MSG_UPDATE_SENSORS,
} gui_msg_type_t;

typedef struct {
  gui_msg_type_t type;
  union {
    bme680_state_t sensor_data;
    char *text_data;
  } value;
} gui_msg_t;

typedef enum { NET_MSG_SYNC_TIME, NET_MSG_INIT_WIFI } net_msg_type_t;

typedef struct {
  net_msg_type_t type;
  union {
  } value;
} net_msg_t;

#endif
