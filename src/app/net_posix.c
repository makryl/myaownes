#ifndef __EMSCRIPTEN__

#include "net.h"

#include <string.h>
#include <stdio.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

typedef SOCKET socket_t;
typedef u_long ioctl_arg_t;
#define close closesocket
#define ioctl ioctlsocket
#define poll WSAPoll
#define MSG_NOSIGNAL 0
#define net_wouldblock() (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS)
static const char* net_strerror_impl(int err)
{
  static char buf[256];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err,
                 MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
  return buf;
}
#define net_strerror(...) net_strerror_impl((__VA_ARGS__ + 0) ? (__VA_ARGS__ + 0) : WSAGetLastError())

#else

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>

typedef int socket_t;
typedef int ioctl_arg_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define net_wouldblock() (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
#define net_strerror(...) strerror((__VA_ARGS__ + 0) ? (__VA_ARGS__ + 0) : errno)

#endif

enum
{
  NET_RELIABLE_STATE_DISCONNECTED = 0,
  NET_RELIABLE_STATE_ACCEPTING,
  NET_RELIABLE_STATE_POLLING,
  NET_RELIABLE_STATE_CONNECTED,
};

enum
{
  NET_UNRELIABLE_STATE_DISCONNECTED = 0,
  NET_UNRELIABLE_STATE_CONNECTING,
  NET_UNRELIABLE_STATE_PINGING,
  NET_UNRELIABLE_STATE_PONGING,
  NET_UNRELIABLE_STATE_CONNECTED,
};

static struct
{
  socket_t reliable_fd;
  socket_t unreliable_fd;
  unsigned int reliable_state;
  unsigned int unreliable_state;
} net;

void net_init()
{
#ifdef _WIN32
  WSADATA wsa;
  int err = WSAStartup(WINSOCK_VERSION, &wsa);
  if (err != 0) {
    fprintf(stderr, "WSAStartup() error: %s\n", net_strerror(err));
  }
#endif
  memset(&net, 0, sizeof(net));
  net.reliable_fd = INVALID_SOCKET;
  net.unreliable_fd = INVALID_SOCKET;
}

void net_disconnect()
{
  if (net.reliable_fd != INVALID_SOCKET) {
    close(net.reliable_fd);
    net.reliable_fd = INVALID_SOCKET;
  }
  if (net.unreliable_fd != INVALID_SOCKET) {
    close(net.unreliable_fd);
    net.unreliable_fd = INVALID_SOCKET;
  }
  net.reliable_state = NET_RELIABLE_STATE_DISCONNECTED;
  net.unreliable_state = NET_UNRELIABLE_STATE_DISCONNECTED;
}

void net_quit()
{
  net_disconnect();
#ifdef _WIN32
  WSACleanup();
#endif
}

static bool net_socket_nonblock_nodelay(socket_t fd, int type)
{
  ioctl_arg_t arg = 1;
  if (ioctl(fd, FIONBIO, &arg) == SOCKET_ERROR) {
    fprintf(stderr, "ioctl(FIONBIO) error: %s\n", net_strerror());
    return false;
  }
  if (type == SOCK_STREAM) {
    int opt = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void*)&opt, sizeof(opt)) == SOCKET_ERROR) {
      fprintf(stderr, "setsockopt(IPPROTO_TCP, TCP_NODELAY) error: %s\n", net_strerror());
      return false;
    }
  }
  return true;
}

static socket_t net_socket(const char* address, unsigned int port, int type, struct sockaddr_in* addr)
{
  socket_t fd = socket(AF_INET, type, 0);
  if (fd == INVALID_SOCKET) {
    fprintf(stderr, "socket(SOCK_STREAM) error: %s\n", net_strerror());
    return INVALID_SOCKET;
  }
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt)) == SOCKET_ERROR) {
    fprintf(stderr, "setsockopt(SOL_SOCKET, SO_REUSEADDR) error: %s\n", net_strerror());
    close(fd);
    return INVALID_SOCKET;
  }
  if (!net_socket_nonblock_nodelay(fd, type)) {
    close(fd);
    return INVALID_SOCKET;
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  if (!address || !address[0]) {
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    int result = inet_pton(AF_INET, address, &addr->sin_addr);
    if (result <= 0) {
      fprintf(stderr, "inet_pton() error: %s\n", result == 0 ? "invalid address" : net_strerror());
      close(fd);
      return INVALID_SOCKET;
    }
  }
  return fd;
}

static socket_t net_socket_server(const char* address, unsigned int port, int type)
{
  struct sockaddr_in addr = {};
  socket_t fd = net_socket(address, port, type, &addr);
  if (fd == INVALID_SOCKET) {
    return INVALID_SOCKET;
  }
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    fprintf(stderr, "bind() error: %s\n", net_strerror());
    close(fd);
    return INVALID_SOCKET;
  }
  return fd;
}

static socket_t net_socket_client(const char* address, unsigned int port, int type)
{
  struct sockaddr_in addr = {};
  socket_t fd = net_socket(address, port, type, &addr);
  if (fd == INVALID_SOCKET) {
    return INVALID_SOCKET;
  }
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "connect() error: %s\n", net_strerror());
      close(fd);
      return INVALID_SOCKET;
    }
  }
  return fd;
}

void net_server(const char* address, unsigned int port)
{
  net.reliable_fd = net_socket_server(address, port, SOCK_STREAM);
  if (net.reliable_fd == INVALID_SOCKET) {
    net_disconnect();
    return;
  }
  if (listen(net.reliable_fd, 1) == SOCKET_ERROR) {
    fprintf(stderr, "listen() error: %s\n", net_strerror());
    net_disconnect();
    return;
  }
  net.unreliable_fd = net_socket_server(address, port, SOCK_DGRAM);
  if (net.unreliable_fd == INVALID_SOCKET) {
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_ACCEPTING;
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTING;
}

void net_client(const char* address, unsigned int port)
{
  net.reliable_fd = net_socket_client(address, port, SOCK_STREAM);
  if (net.reliable_fd == INVALID_SOCKET) {
    net_disconnect();
    return;
  }
  net.unreliable_fd = net_socket_client(address, port, SOCK_DGRAM);
  if (net.unreliable_fd == INVALID_SOCKET) {
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_POLLING;
  net.unreliable_state = NET_UNRELIABLE_STATE_PINGING;
}

static void net_server_accept()
{
  socket_t fd = accept(net.reliable_fd, nullptr, nullptr);
  if (fd == INVALID_SOCKET) {
    if (!net_wouldblock()) {
      fprintf(stderr, "accept() error: %s\n", net_strerror());
      net_disconnect();
    }
    return;
  }
  if (!net_socket_nonblock_nodelay(fd, SOCK_STREAM)) {
    close(fd);
    net_disconnect();
    return;
  }
  close(net.reliable_fd);
  net.reliable_fd = fd;
  net.reliable_state = NET_RELIABLE_STATE_CONNECTED;
}

static void net_client_poll()
{
  struct pollfd pfd = {
    .fd = net.reliable_fd,
    .events = POLLOUT,
  };
  if (poll(&pfd, 1, 0) <= 0) {
    return;
  }
  int err = 0;
  socklen_t len = sizeof(err);
  if (getsockopt(net.reliable_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) == SOCKET_ERROR) {
    fprintf(stderr, "getsockopt(SOL_SOCKET, SO_ERROR): %s\n", net_strerror());
    net_disconnect();
    return;
  }
  if (err != 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
    // fprintf(stderr, "poll() error: %s\n", err == 0 ? "hangup or invalid" : net_strerror(err));
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_CONNECTED;
}

unsigned int net_send_stream(const void* data, unsigned int size)
{
  if (net.reliable_fd == INVALID_SOCKET) {
    return 0;
  }
  int sent = send(net.reliable_fd, data, size, MSG_NOSIGNAL);
  if (sent == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "send() error: %s\n", net_strerror());
      net_disconnect();
    }
    return 0;
  }
  return sent;
}

unsigned int net_recv_stream(void* data, unsigned int size)
{
  if (net.reliable_fd == INVALID_SOCKET) {
    return 0;
  }
  int received = recv(net.reliable_fd, data, size, 0);
  if (received <= 0) {
    if (received == 0 || !net_wouldblock()) {
      if (received != 0) {
        fprintf(stderr, "recv() error: %s\n", net_strerror());
      }
      net_disconnect();
    }
    return 0;
  }
  return received;
}

unsigned int net_send_datagram(const void* data, unsigned int size)
{
  if (net.unreliable_fd == INVALID_SOCKET) {
    return 0;
  }
  int sent = send(net.unreliable_fd, data, size, 0);
  if (sent == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "send() error: %s\n", net_strerror());
      net_disconnect();
    }
    return 0;
  }
  return sent;
}

unsigned int net_recv_datagram(void* data, unsigned int size)
{
  if (net.unreliable_fd == INVALID_SOCKET) {
    return 0;
  }
  int received = recv(net.unreliable_fd, data, size, 0);
  if (received == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "recv() error: %s\n", net_strerror());
      net_disconnect();
    }
    return 0;
  }
  return received;
}

static void net_server_connect()
{
  struct sockaddr_in addr = {};
  socklen_t addr_size = sizeof(addr);
  char data;
  if (recvfrom(net.unreliable_fd, &data, sizeof(data), 0, (struct sockaddr*)&addr, &addr_size) == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "recvfrom() error: %s\n", net_strerror());
      net_disconnect();
    }
    return;
  }
  if (connect(net.unreliable_fd, (struct sockaddr*)&addr, addr_size) == SOCKET_ERROR) {
    if (!net_wouldblock()) {
      fprintf(stderr, "connect() error: %s\n", net_strerror());
      net_disconnect();
      return;
    }
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_PONGING;
}

static void net_client_ping()
{
  char data = 0;
  net_send_datagram(&data, sizeof(data));
  if (net_recv_stream(&data, sizeof(data)) == 0) {
    return;
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTED;
}

static void net_server_pong()
{
  char data = 0;
  if (net_send_stream(&data, sizeof(data)) == 0) {
    return;
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTED;
}

unsigned int net_update()
{
  if (net.reliable_state == NET_RELIABLE_STATE_DISCONNECTED
      && net.unreliable_state == NET_UNRELIABLE_STATE_DISCONNECTED)
  {
    return NET_STATE_DISCONNECTED;
  }
  if (net.reliable_state == NET_RELIABLE_STATE_CONNECTED && net.unreliable_state == NET_UNRELIABLE_STATE_CONNECTED) {
    return NET_STATE_CONNECTED;
  }
  switch (net.reliable_state) {
    case NET_RELIABLE_STATE_ACCEPTING: net_server_accept(); break;
    case NET_RELIABLE_STATE_POLLING: net_client_poll(); break;
  }
  switch (net.unreliable_state) {
    case NET_UNRELIABLE_STATE_CONNECTING: net_server_connect(); break;
    case NET_UNRELIABLE_STATE_PINGING: net_client_ping(); break;
    case NET_UNRELIABLE_STATE_PONGING: net_server_pong(); break;
  }
  return NET_STATE_CONNECTING;
}

#endif
