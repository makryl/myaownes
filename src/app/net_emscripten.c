#ifdef __EMSCRIPTEN__

#include "net.h"

void net_init() {}
void net_quit() {}
void net_server(const char* address, unsigned int port) {}
void net_client(const char* address, unsigned int port) {}
void net_disconnect() {}
unsigned int net_update() { return NET_STATE_DISCONNECTED; }
unsigned int net_send_stream(const void* data, unsigned int size) {}
unsigned int net_recv_stream(void* data, unsigned int size) {}
unsigned int net_send_datagram(const void* data, unsigned int size) {}
unsigned int net_recv_datagram(void* data, unsigned int size) {}

#endif
