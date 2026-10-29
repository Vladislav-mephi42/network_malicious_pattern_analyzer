

#include "sockets.h"
#include <arpa/inet.h>
#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <time.h>
#include <unistd.h>

void TCPSocket::send_n(const char *buf, int len, int flags) {
  int current = 0;
  int total = 0;

  while (current != len) {
    total = send(fd, buf + current, len - current, flags);
    if (total < 0) {
      throw std::system_error(errno, std::generic_category(), "bad send");
    }
    current += total;
  }
}
void TCPSocket::recv_n(char *buf, int len, int flags) {
  int current = 0;
  int total = 0;

  while (current != len) {
    total = recv(fd, (void *)(buf + current), len - current, flags);
    if (total < 0) {
      throw std::system_error(errno, std::generic_category(), "bad send");
    }
    if (total == 0) {
      throw std::runtime_error("recv: connection closed by peer");
    }
    current += total;
  }
}
void TCPSocket::auto_send(const char *buf, int len, int flags) {
  uint32_t inet_len = htonl((uint32_t)len);
  int res = send(fd, &inet_len, sizeof(inet_len), 0);
  if (res <= 0) {
    throw std::system_error(errno, std::generic_category(), "bad send");
  }
  send_n(buf, len, flags);
}
int TCPSocket::auto_recv(char *buf, int len, int flags) {
  uint32_t inet_len;
  int res = recv(fd, &inet_len, sizeof(inet_len), 0);
  if (res <= 0) {
    throw std::system_error(errno, std::generic_category(), "bad recv");
  }
  inet_len = ntohl(inet_len);
  if (inet_len > len) {
    throw std::runtime_error("inet len is bigger than can be");
  }
  recv_n(buf, (int)inet_len, flags);
  return (int)inet_len;
}

void TCPSocket::send_json(const json &data) {
  auto ser = data.dump();

  auto_send(ser.c_str(), ser.size());
}
json TCPSocket::recv_json() {
  uint32_t inet_len;
  int res = recv(fd, &inet_len, sizeof(inet_len), 0);
  if (res <= 0) {
    throw std::system_error(errno, std::generic_category(), "bad recv");
  }
  int len = ntohl(inet_len);
  std::string buf;
  buf.resize(len);
  recv_n(buf.data(), len);
  return json::parse(buf);
}
