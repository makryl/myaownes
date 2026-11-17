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
  int server_fd;
  int client_fd;
} app_net;

void app_net_init()
{
  memset(&app_net, 0, sizeof(app_net));
  app_net.server_fd = -1;
  app_net.client_fd = -1;
  app_net_state(APP_NET_STATE_DISCONNECTED);
}

void app_net_quit()
{
  app_net_server_close();
  app_net_client_close();
  app_net_state(APP_NET_STATE_DISCONNECTED);
}

static bool app_net_socket_nonblock_nodelay(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    fprintf(stderr, "fcntl(F_GETFL) error: %s\n", strerror(errno));
    close(fd);
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    fprintf(stderr, "fcntl(F_SETFL, O_NONBLOCK) error: %s\n", strerror(errno));
    close(fd);
    return false;
  }

  int flag = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
    fprintf(stderr, "setsockopt(IPPROTO_TCP, TCP_NODELAY) error: %s\n", strerror(errno));
    close(fd);
    return false;
  }

  return true;
}

static int app_net_socket_host(const char* address, uint port)
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

  if (!app_net_socket_nonblock_nodelay(fd)) {
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

static int app_net_socket_client(const char* address, uint port)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "socket() error: %s\n", strerror(errno));
    return -1;
  }

  if (!app_net_socket_nonblock_nodelay(fd)) {
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

void app_net_server_listen(const char* address, uint port)
{
  int fd = app_net_socket_host(address, port);
  if (fd < 0) {
    return;
  }
  app_net.server_fd = fd;
  app_net_state(APP_NET_STATE_LISTENING);
}

void app_net_server_close()
{
  close(app_net.server_fd);
  app_net.server_fd = -1;
}

void app_net_server_accept()
{
  int fd = accept(app_net.server_fd, nullptr, nullptr);
  if (fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "accept() error: %s\n", strerror(errno));
    }
    return;
  }
  if (!app_net_socket_nonblock_nodelay(fd)) {
    return;
  }
  app_net.client_fd = fd;
  app_net_server_close();
  app_net_state(APP_NET_STATE_CONNECTED);
}

void app_net_client_connect(const char* address, uint port)
{
  int fd = app_net_socket_client(address, port);
  if (fd < 0) {
    return;
  }
  app_net.client_fd = fd;
  app_net_state(APP_NET_STATE_CONNECTING);
}

void app_net_client_close()
{
  close(app_net.client_fd);
  app_net.client_fd = -1;
  app_net_state(APP_NET_STATE_DISCONNECTED);
}

void app_net_client_poll()
{
  struct pollfd pfd = {
    .fd = app_net.client_fd,
    .events = POLLOUT,
  };
  if (poll(&pfd, 1, 0) <= 0) {
    return;
  }
  int err = 0;
  socklen_t len = sizeof(err);
  getsockopt(app_net.client_fd, SOL_SOCKET, SO_ERROR, &err, &len);
  if (err != 0) {
    fprintf(stderr, "getsockopt(SOL_SOCKET, SO_ERROR): %s\n", strerror(err));
    app_net_client_close();
    return;
  }
  app_net_state(APP_NET_STATE_CONNECTED);
}

void app_net_send(const void* data, uint size)
{
  if (send(app_net.client_fd, data, size, MSG_NOSIGNAL) < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      fprintf(stderr, "send() error: %s\n", strerror(errno));
      app_net_client_close();
    }
  }
}

uint app_net_recv(void* data, uint size)
{
  ssize_t received = recv(app_net.client_fd, data, size, 0);
  if (received <= 0) {
    if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      fprintf(stderr, "recv() error: %s\n", received == 0 ? "disconnected" : strerror(errno));
      app_net_client_close();
    }
    return 0;
  }
  return received;
}

#endif
