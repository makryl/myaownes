#pragma once

#include "myaownes.h"

enum
{
  APP_NET_STATE_DISCONNECTED,
  APP_NET_STATE_LISTENING,
  APP_NET_STATE_CONNECTING,
  APP_NET_STATE_CONNECTED,
  APP_NET_STATE_SYNC,
  APP_NET_STATE_PLAY,
};

void app_net_state(uint state);
void app_net_update();

void app_net_init();
void app_net_quit();
void app_net_server_listen(const char* address, uint port);
void app_net_server_close();
void app_net_server_accept();
void app_net_client_connect(const char* address, uint port);
void app_net_client_close();
void app_net_client_poll();
void app_net_send(const void* data, uint size);
uint app_net_recv(void* data, uint size);

#if !defined(__linux__)

void app_net_init() {}
void app_net_quit() {}
void app_net_server_listen(uint port) {}
void app_net_server_close() {}
void app_net_server_accept() {}
void app_net_client_connect(const char* ip, uint port) {}
void app_net_client_close() {}
void app_net_client_poll() {}
void app_net_send(const void* data, uint size) {}
uint app_net_recv(void* data, uint size) { return 0; }

#endif
