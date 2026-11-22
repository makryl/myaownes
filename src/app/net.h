#pragma once

#include "myaownes.h"

void net_init();
void net_quit();
void net_server(const char* address, uint port);
void net_client(const char* address, uint port);
bool net_connected();
void net_disconnect();
uint net_send_stream(const void* data, uint size);
uint net_recv_stream(void* data, uint size);
uint net_send_datagram(const void* data, uint size);
uint net_recv_datagram(void* data, uint size);

void app_net_update();
bool app_net_input();
