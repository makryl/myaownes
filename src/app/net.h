#pragma once

enum net_state
{
  NET_STATE_DISCONNECTED = 0,
  NET_STATE_CONNECTING,
  NET_STATE_CONNECTED,
};

enum
{
  APP_NET_STATE_DISCONNECTED = 0,
  APP_NET_STATE_CONNECTING,
  APP_NET_STATE_SYNC_ROM,
  APP_NET_STATE_SYNC_STATE,
  APP_NET_STATE_SYNC_WAIT,
  APP_NET_STATE_PLAY,
};

void net_init();
void net_quit();
void net_server(const char* address, unsigned int port);
void net_client(const char* address, unsigned int port);
void net_disconnect();
unsigned int net_update();
unsigned int net_send_stream(const void* data, unsigned int size);
unsigned int net_recv_stream(void* data, unsigned int size);
unsigned int net_send_datagram(const void* data, unsigned int size);
unsigned int net_recv_datagram(void* data, unsigned int size);

void app_net_update();
bool app_net_input();
