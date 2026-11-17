#include "net.h"

#ifdef __linux__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

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
  uint reliable_state;
  int reliable_fd;
  uint unreliable_state;
  int unreliable_fd;
} net;

void net_init()
{
  memset(&net, 0, sizeof(net));
  net.reliable_fd = -1;
  net.unreliable_fd = -1;
}

void net_disconnect()
{
  close(net.reliable_fd);
  close(net.unreliable_fd);
  net.reliable_fd = -1;
  net.unreliable_fd = -1;
  net.reliable_state = NET_RELIABLE_STATE_DISCONNECTED;
  net.unreliable_state = NET_UNRELIABLE_STATE_DISCONNECTED;
}

void net_quit() { net_disconnect(); }

static bool net_socket_nonblock_nodelay(int fd, int type)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    fprintf(stderr, "fcntl(F_GETFL) error: %s\n", strerror(errno));
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    fprintf(stderr, "fcntl(F_SETFL, O_NONBLOCK) error: %s\n", strerror(errno));
    return false;
  }
  if (type == SOCK_STREAM) {
    int opt = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
      fprintf(stderr, "setsockopt(IPPROTO_TCP, TCP_NODELAY) error: %s\n", strerror(errno));
      return false;
    }
  }
  return true;
}

static int net_socket(const char* address, uint port, int type, struct sockaddr_in* addr)
{
  int fd = socket(AF_INET, type, 0);
  if (fd < 0) {
    fprintf(stderr, "socket(SOCK_STREAM) error: %s\n", strerror(errno));
    return -1;
  }
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    fprintf(stderr, "setsockopt(SOL_SOCKET, SO_REUSEADDR) error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  if (!net_socket_nonblock_nodelay(fd, type)) {
    close(fd);
    return -1;
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  int result = inet_pton(AF_INET, address, &addr->sin_addr);
  if (result <= 0) {
    fprintf(stderr, "inet_pton() error: %s\n", result == 0 ? "invalid address" : strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int net_socket_server(const char* address, uint port, int type)
{
  struct sockaddr_in addr = {};
  int fd = net_socket(address, port, type, &addr);
  if (fd < 0) {
    return -1;
  }
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "bind() error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int net_socket_client(const char* address, uint port, int type)
{
  struct sockaddr_in addr = {};
  int fd = net_socket(address, port, type, &addr);
  if (fd < 0) {
    return -1;
  }
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 && errno != EINPROGRESS) {
    fprintf(stderr, "connect() error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

void net_server(const char* address, uint port)
{
  net.reliable_fd = net_socket_server(address, port, SOCK_STREAM);
  if (net.reliable_fd < 0) {
    net_disconnect();
    return;
  }
  if (listen(net.reliable_fd, 1) < 0) {
    fprintf(stderr, "listen() error: %s\n", strerror(errno));
    net_disconnect();
    return;
  }
  net.unreliable_fd = net_socket_server(address, port, SOCK_DGRAM);
  if (net.unreliable_fd < 0) {
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_ACCEPTING;
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTING;
}

void net_client(const char* address, uint port)
{
  net.reliable_fd = net_socket_client(address, port, SOCK_STREAM);
  if (net.reliable_fd < 0) {
    net_disconnect();
    return;
  }
  net.unreliable_fd = net_socket_client(address, port, SOCK_DGRAM);
  if (net.unreliable_fd < 0) {
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_POLLING;
  net.unreliable_state = NET_UNRELIABLE_STATE_PINGING;
}

static void net_server_accept()
{
  int fd = accept(net.reliable_fd, nullptr, nullptr);
  if (fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "accept() error: %s\n", strerror(errno));
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
  getsockopt(net.reliable_fd, SOL_SOCKET, SO_ERROR, &err, &len);
  if (err != 0) {
    fprintf(stderr, "getsockopt(SOL_SOCKET, SO_ERROR): %s\n", strerror(err));
    net_disconnect();
    return;
  }
  net.reliable_state = NET_RELIABLE_STATE_CONNECTED;
}

uint net_send_stream(const void* data, uint size)
{
  ssize_t sent = send(net.reliable_fd, data, size, MSG_NOSIGNAL);
  if (sent < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "send() error: %s\n", strerror(errno));
      net_disconnect();
    }
    return 0;
  }
  return sent;
}

uint net_recv_stream(void* data, uint size)
{
  ssize_t received = recv(net.reliable_fd, data, size, 0);
  if (received <= 0) {
    if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      fprintf(stderr, "recv() error: %s\n", received == 0 ? "disconnected" : strerror(errno));
      net_disconnect();
    }
    return 0;
  }
  return received;
}

uint net_send_datagram(const void* data, uint size)
{
  ssize_t sent = send(net.unreliable_fd, data, size, 0);
  if (sent < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "send() error: %s\n", strerror(errno));
      net_disconnect();
    }
    return 0;
  }
  return sent;
}

uint net_recv_datagram(void* data, uint size)
{
  ssize_t received = recv(net.unreliable_fd, data, size, 0);
  if (received < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "recv() error: %s\n", strerror(errno));
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
  ssize_t received = recvfrom(net.unreliable_fd, nullptr, 0, 0, (struct sockaddr*)&addr, &addr_size);
  if (received < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "recvfrom() error: %s\n", strerror(errno));
      net_disconnect();
    }
    return;
  }
  if (connect(net.unreliable_fd, (struct sockaddr*)&addr, addr_size) < 0 && errno != EINPROGRESS) {
    fprintf(stderr, "connect() error: %s\n", strerror(errno));
    net_disconnect();
    return;
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_PONGING;
}

static void net_client_ping()
{
  u8 data = 0;
  net_send_datagram(&data, sizeof(data));
  if (net_recv_stream(&data, sizeof(data)) == 0) {
    return;
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTED;
}

static void net_server_pong()
{
  u8 data = 0;
  if (net_send_stream(&data, sizeof(data)) == 0) {
    return;
  }
  net.unreliable_state = NET_UNRELIABLE_STATE_CONNECTED;
}

bool net_connected()
{
  if (net.reliable_state == NET_RELIABLE_STATE_CONNECTED && net.unreliable_state == NET_UNRELIABLE_STATE_CONNECTED) {
    return true;
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
  return false;
}

#endif
