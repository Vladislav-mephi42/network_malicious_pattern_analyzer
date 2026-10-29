#ifndef FIFO_H
#define FIFO_H

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

constexpr uint32_t max_buf = 1000000000;

inline bool fifo_exists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0 && S_ISFIFO(buffer.st_mode));
}

class FIFOWriter {
private:
  int fd = -1;
  std::string fifo_name;
  bool delete_flag = true;
  FIFOWriter() {}

public:
  void swap(FIFOWriter &other) noexcept;
  FIFOWriter(const std::string &fifo_name, bool create_flag = true);
  ~FIFOWriter();
  FIFOWriter(FIFOWriter &) = delete;
  FIFOWriter &operator=(FIFOWriter &) = delete;
  FIFOWriter(FIFOWriter &&other) noexcept { swap(other); }
  FIFOWriter &operator=(FIFOWriter &&other) noexcept {
    if (this != &other) {
      swap(other);
    }
    return *this;
  }

  void write_n(const void *data, size_t n);
  void write_all(const char *data, size_t n);
  void write_json(const json &data);
};

class FIFOReader {
private:
  int fd = -1;
  std::string fifo_name;
  bool delete_flag = false;
  FIFOReader() {}

public:
  void swap(FIFOReader &other) noexcept;

  FIFOReader(const std::string &fifo_name, bool create_flag = false);
  ~FIFOReader();

  FIFOReader(FIFOReader &) = delete;
  FIFOReader &operator=(FIFOReader &) = delete;
  FIFOReader(FIFOReader &&other) noexcept { swap(other); }
  FIFOReader &operator=(FIFOReader &&other) noexcept {
    if (this != &other) {
      swap(other);
    }
    return *this;
  }
  void read_n(void *data, size_t n);
  json read_json();
};

#endif