#ifndef SOCKETS_H
#define SOCKETS_H

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

using json = nlohmann::json;

class TCPSocket {
protected:
  int fd;
  TCPSocket(int fd_in) { fd = fd_in; }

public:
  TCPSocket() { fd = socket(AF_INET, SOCK_STREAM, 0); }

  ~TCPSocket() {
    if (fd != -1) {
      close(fd);
    }
  }
  void manualy_close() {
    if (fd != -1) {
      close(fd);
    }
    fd = -1;
  }
  TCPSocket(const TCPSocket &other) = delete;
  TCPSocket &operator=(const TCPSocket &other) = delete;

  TCPSocket(TCPSocket &&other) {
    fd = other.fd;
    other.fd = -1;
  }
  TCPSocket &operator=(TCPSocket &&other) {
    std::swap(fd, other.fd);
    return *this;
  }
  void send_n(const char *buf, int len, int flags = 0) {
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
  void recv_n(char *buf, int len, int flags = 0) {
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
  void auto_send(const char *buf, int len, int flags = 0) {
    uint32_t inet_len = htonl((uint32_t)len);
    int res = send(fd, &inet_len, sizeof(inet_len), 0);
    if (res <= 0) {
      throw std::system_error(errno, std::generic_category(), "bad send");
    }
    send_n(buf, len, flags);
  }
  int auto_recv(char *buf, int len, int flags = 0) {
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

  void send_json(const json &data) {
    auto ser = data.dump();

    auto_send(ser.c_str(), ser.size());
  }
  json recv_json() {
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
};

class Client_socket : public TCPSocket {

public:
  Client_socket() : TCPSocket() {}
  Client_socket(int fd) : TCPSocket(fd) {}
  ~Client_socket() {}

  void connect(const sockaddr *addr, socklen_t addrlen) {
    if (::connect(fd, addr, addrlen) == -1) {
      throw std::system_error(errno, std::generic_category(), "connect failed");
    }
  }
  void connect(const char *server_ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(server_ip);
    if (::connect(fd, reinterpret_cast<const sockaddr *>(&addr),
                  sizeof(addr)) == -1) {
      throw std::system_error(errno, std::generic_category(), "connect failed");
    }
  }
};

class Server_socket : public TCPSocket {

public:
  Server_socket() : TCPSocket() {}
  ~Server_socket() {}

  void bind(const sockaddr *addr, socklen_t addrlen) {
    if (::bind(fd, addr, addrlen) == -1) {
      throw std::system_error(errno, std::generic_category(), "bind failed");
    }
  }
  void bind(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) ==
        -1) {
      throw std::system_error(errno, std::generic_category(), "bind failed");
    }
  }
  void listen(int backlog) {
    if (backlog > SOMAXCONN) {
      throw std::logic_error("max backlog has been exceeded");
    }
    if (::listen(fd, backlog) == -1) {
      throw std::system_error(errno, std::generic_category(), "listen failed");
    }
  }
  Client_socket accept() {
    int client = ::accept(fd, NULL, NULL);
    if (client == -1) {
      throw std::system_error(errno, std::generic_category(), "bad_accept");
    }
    return Client_socket(client);
  }
};

#endif