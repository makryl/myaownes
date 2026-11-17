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

static struct
{
  int reliable_server_fd;
  int reliable_client_fd;
  int unreliable_server_fd;
  int unreliable_client_fd;
  bool reliable_connecting;
  bool reliable_connected;
  bool unreliable_connecting;
  bool unreliable_connected;
} net;

void net_init()
{
  memset(&net, 0, sizeof(net));
  net.reliable_server_fd = -1;
  net.reliable_client_fd = -1;
  net.unreliable_server_fd = -1;
  net.unreliable_client_fd = -1;
}

void net_disconnect()
{
  close(net.reliable_server_fd);
  close(net.reliable_client_fd);
  close(net.unreliable_server_fd);
  close(net.unreliable_client_fd);
  net.reliable_server_fd = -1;
  net.reliable_client_fd = -1;
  net.unreliable_server_fd = -1;
  net.unreliable_client_fd = -1;
  net.reliable_connecting = false;
  net.reliable_connected = false;
  net.unreliable_connecting = false;
  net.unreliable_connected = false;
}

void net_quit() { net_disconnect(); }

static bool net_socket_nonblock_nodelay(int fd)
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

  int flag = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
    fprintf(stderr, "setsockopt(IPPROTO_TCP, TCP_NODELAY) error: %s\n", strerror(errno));
    return false;
  }

  return true;
}

static int net_socket_server_reliable(const char* address, uint port)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "socket() error: %s\n", strerror(errno));
    return -1;
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    fprintf(stderr, "setsockopt(SOL_SOCKET, SO_REUSEADDR) error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  if (!net_socket_nonblock_nodelay(fd)) {
    close(fd);
    return -1;
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
  };

  int result = inet_pton(AF_INET, address, &addr.sin_addr);
  if (result <= 0) {
    fprintf(stderr, "inet_pton() error: %s\n", result == 0 ? "invalid address" : strerror(errno));
    close(fd);
    return -1;
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "bind() error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  if (listen(fd, 1) < 0) {
    fprintf(stderr, "listen() error: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  return fd;
}

static int net_socket_client_reliable(const char* address, uint port)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "socket() error: %s\n", strerror(errno));
    return -1;
  }

  if (!net_socket_nonblock_nodelay(fd)) {
    close(fd);
    return -1;
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
  };
  int result = inet_pton(AF_INET, address, &addr.sin_addr);
  if (result <= 0) {
    fprintf(stderr, "inet_pton() error: %s\n", result == 0 ? "invalid address" : strerror(errno));
    close(fd);
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
  int fd = net_socket_server_reliable(address, port);
  if (fd < 0) {
    net_disconnect();
    return;
  }
  net.reliable_server_fd = fd;
  net.reliable_connecting = true;
}

void net_client(const char* address, uint port)
{
  int fd = net_socket_client_reliable(address, port);
  if (fd < 0) {
    net_disconnect();
    return;
  }
  net.reliable_client_fd = fd;
  net.reliable_connecting = true;
}

static void net_server_accept()
{
  int fd = accept(net.reliable_server_fd, nullptr, nullptr);
  if (fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "accept() error: %s\n", strerror(errno));
      net_disconnect();
    }
    return;
  }
  if (!net_socket_nonblock_nodelay(fd)) {
    net_disconnect();
    return;
  }
  close(net.reliable_server_fd);
  net.reliable_server_fd = -1;
  net.reliable_client_fd = fd;
  net.reliable_connected = true;
}

static void net_client_poll()
{
  struct pollfd pfd = {
    .fd = net.reliable_client_fd,
    .events = POLLOUT,
  };
  if (poll(&pfd, 1, 0) <= 0) {
    return;
  }
  int err = 0;
  socklen_t len = sizeof(err);
  getsockopt(net.reliable_client_fd, SOL_SOCKET, SO_ERROR, &err, &len);
  if (err != 0) {
    fprintf(stderr, "getsockopt(SOL_SOCKET, SO_ERROR): %s\n", strerror(err));
    net_disconnect();
    return;
  }
  net.reliable_connecting = false;
  net.reliable_connected = true;
}

bool net_connected()
{
  if (net.reliable_connecting) {
    if (net.reliable_server_fd >= 0) {
      net_server_accept();
    }
    if (net.reliable_client_fd >= 0) {
      net_client_poll();
    }
  }
  if (net.unreliable_connecting) {
    //
  }
  return net.reliable_connected; // && net.unreliable_connected;
}

uint net_send_stream(const void* data, uint size)
{
  ssize_t sent = send(net.reliable_client_fd, data, size, MSG_NOSIGNAL);
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
  ssize_t received = recv(net.reliable_client_fd, data, size, 0);
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
  //
}

uint net_recv_datagram(void* data, uint size)
{
  //
}

#endif
