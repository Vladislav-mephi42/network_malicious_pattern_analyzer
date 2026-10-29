#include "fifo.h"

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

void FIFOWriter::swap(FIFOWriter &other) noexcept {
  std::swap(fd, other.fd);
  std::swap(fifo_name, other.fifo_name);
  std::swap(delete_flag, other.delete_flag);
}
FIFOWriter::FIFOWriter(const std::string &fifo_name, bool create_flag)
    : fifo_name(fifo_name), delete_flag(create_flag) {
  if (create_flag) {
    if (mkfifo(fifo_name.c_str(), 0644) < 0) {
      if (errno == EEXIST) {
        throw std::runtime_error("FIFO already exists: " + fifo_name);
      }
      throw std::system_error(errno, std::generic_category(), "mkfifo failed");
    }
    if ((fd = open(fifo_name.c_str(), O_WRONLY)) < 0) {
      unlink(fifo_name.c_str());
      throw std::system_error(errno, std::generic_category(), "bad open");
    }
  } else {
    if (!fifo_exists(fifo_name)) {
      throw std::runtime_error("fifo didn`t exist");
    }
    if ((fd = open(fifo_name.c_str(), O_WRONLY)) < 0) {
      throw std::system_error(errno, std::generic_category(), "bad open");
    }
  }
}
FIFOWriter::~FIFOWriter() {
  if (fd != -1) {
    if (delete_flag) {
      unlink(fifo_name.c_str());
    }
    close(fd);
  }
}

void FIFOWriter::write_n(const void *data, size_t n) {

  int total = 0;
  int current = 0;
  while (current < n) {
    total = write(fd, static_cast<const char *>(data) + current, n - current);
    if (total < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "write call failed");
    }
    current += total;
  }
}
void FIFOWriter::write_all(const char *data, size_t n) {
  uint32_t len = static_cast<uint32_t>(n);
  write_n(&len, sizeof(uint32_t));
  write_n(data, n);
}
void FIFOWriter::write_json(const json &data) {
  auto str = data.dump();

  write_all(str.c_str(), str.size());
}

void FIFOReader::swap(FIFOReader &other) noexcept {
  std::swap(fd, other.fd);
  std::swap(fifo_name, other.fifo_name);
  std::swap(delete_flag, other.delete_flag);
}

FIFOReader::FIFOReader(const std::string &fifo_name, bool create_flag)
    : fifo_name(fifo_name), delete_flag(create_flag) {
  if (create_flag) {
    if (fifo_exists(fifo_name)) {
      throw std::runtime_error("fifo exist");
    }
    if (mkfifo(fifo_name.c_str(), 0644) < 0) {
      throw std::system_error(errno, std::generic_category(), "mkfifo failed");
    }
    if ((fd = open(fifo_name.c_str(), O_RDONLY)) < 0) {
      unlink(fifo_name.c_str());
      throw std::system_error(errno, std::generic_category(), "bad open");
    }
  } else {
    if (!fifo_exists(fifo_name)) {
      throw std::runtime_error("fifo didn`t exist");
    }
    if ((fd = open(fifo_name.c_str(), O_RDONLY)) < 0) {
      throw std::system_error(errno, std::generic_category(), "bad open");
    }
  }
}
FIFOReader::~FIFOReader() {
  if (fd != -1) {
    close(fd);
    if (delete_flag) {
      unlink(fifo_name.c_str());
    }
  }
}

void FIFOReader::read_n(void *data, size_t n) {
  int total = 0;
  int current = 0;
  while (current < n) {

    total = read(fd, static_cast<char *>(data) + current, n - current);
    if (total < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "write call failed");
    }
    if (total == 0) {
      throw std::runtime_error("EOF");
    }
    current += total;
  }
}

json FIFOReader::read_json() {

  uint32_t len = 0;
  read_n(&len, sizeof(uint32_t));
  if (len > max_buf) {
    throw std::runtime_error("message len must be less ");
  }
  if (len == 0) {
    throw std::runtime_error("zero len messages are not available");
  }
  std::string payload;
  payload.resize(len);
  read_n(payload.data(), len);

  return json::parse(payload);
}
