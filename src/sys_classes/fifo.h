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

bool fifo_exists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0 && S_ISFIFO(buffer.st_mode));
}

class FIFOWriter {
private:
  int fd = -1;
  std::string fifo_name;
  static std::mutex global_mutex;
  static std::mutex fifo_creation_mutex;

public:
  FIFOWriter(const std::string &fifo_name, bool creation_flag = true)
      : fifo_name(fifo_name) {
    std::lock_guard<std::mutex> lock(fifo_creation_mutex);
    if (creation_flag && !fifo_exists(fifo_name)) {
      if (mkfifo(fifo_name.c_str(), 0666) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Failed to create FIFO");
      }
    }
    fd = open(fifo_name.c_str(), O_WRONLY);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "Failed to open FIFO for writing");
    }
  }

  ~FIFOWriter() {
    if (fd != -1) {
      close(fd);
      fd = -1;
    }

    std::lock_guard<std::mutex> lock(fifo_creation_mutex);
    if (!fifo_name.empty() && fifo_exists(fifo_name)) {
      unlink(fifo_name.c_str());
    }
  }

  FIFOWriter(const FIFOWriter &other) = delete;
  FIFOWriter &operator=(const FIFOWriter &other) = delete;

  FIFOWriter(FIFOWriter &&other) noexcept {
    std::lock_guard<std::mutex> lock(fifo_creation_mutex);
    fd = other.fd;
    fifo_name = std::move(other.fifo_name);
    other.fd = -1;
    other.fifo_name.clear();
  }

  FIFOWriter &operator=(FIFOWriter &&other) noexcept {
    if (this != &other) {

      if (fd != -1) {
        close(fd);
        unlink(fifo_name.c_str());
      }

      std::lock_guard<std::mutex> lock(fifo_creation_mutex);
      fd = other.fd;
      fifo_name = std::move(other.fifo_name);
      other.fd = -1;
      other.fifo_name.clear();
    }
    return *this;
  }

  void write_all(const char *buf, size_t len) {
    size_t total_written = 0;
    while (total_written < len) {
      ssize_t written = write(fd, buf + total_written, len - total_written);
      if (written < 0) {
        if (errno == EINTR)
          continue;
        throw std::system_error(errno, std::generic_category(),
                                "Failed to write to FIFO");
      }
      if (written == 0) {
        throw std::runtime_error("Write returned 0 bytes");
      }
      total_written += written;
    }
  }

  void write_json(const json &data) {
    std::lock_guard<std::mutex> lock(global_mutex);

    std::string str = data.dump();
    uint64_t len = str.size();

    write_all(reinterpret_cast<const char *>(&len), sizeof(len));

    write_all(str.data(), str.size());
  }

  const std::string &get_name() const { return fifo_name; }
};

class FIFOReader {
private:
  int fd = -1;
  std::string fifo_name;
  static std::mutex fifo_creation_mutex;

public:
  FIFOReader(const std::string &fifo_name, bool creation_flag = false)
      : fifo_name(fifo_name) {
    if (creation_flag && !fifo_exists(fifo_name)) {
      if (mkfifo(fifo_name.c_str(), 0666) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Failed to create FIFO");
      }
    }
    fd = open(fifo_name.c_str(), O_RDONLY);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "Failed to open FIFO for reading");
    }
  }

  ~FIFOReader() {
    if (fd != -1) {
      close(fd);
    }
    std::lock_guard<std::mutex> lock(fifo_creation_mutex);
    if (!fifo_name.empty() && fifo_exists(fifo_name)) {
      unlink(fifo_name.c_str());
    }
  }

  FIFOReader(const FIFOReader &other) = delete;
  FIFOReader &operator=(const FIFOReader &other) = delete;

  FIFOReader(FIFOReader &&other) noexcept {
    fd = other.fd;
    fifo_name = std::move(other.fifo_name);
    other.fd = -1;
  }

  FIFOReader &operator=(FIFOReader &&other) noexcept {
    if (this != &other) {
      if (fd != -1) {
        close(fd);
      }
      fd = other.fd;
      fifo_name = std::move(other.fifo_name);
      other.fd = -1;
    }
    return *this;
  }

  void read_all(char *buf, size_t len) {
    size_t total_read = 0;
    while (total_read < len) {
      ssize_t read_bytes = read(fd, buf + total_read, len - total_read);
      if (read_bytes < 0) {
        if (errno == EINTR)
          continue;
        throw std::system_error(errno, std::generic_category(),
                                "Failed to read from FIFO");
      }
      if (read_bytes == 0) {
        throw std::runtime_error("EOF reached");
      }
      total_read += read_bytes;
    }
  }

  json read_json() {
    uint64_t len = 0;

    read_all(reinterpret_cast<char *>(&len), sizeof(len));

    if (len > 100 * 1024 * 1024) {
      throw std::runtime_error("Message too large: " + std::to_string(len) +
                               " bytes");
    }

    std::string buf;
    buf.resize(len);
    read_all(buf.data(), len);

    return json::parse(buf);
  }

  int get_fd() const { return fd; }
  const std::string &get_name() const { return fifo_name; }
};

std::mutex FIFOWriter::global_mutex;
std::mutex FIFOWriter::fifo_creation_mutex;
std::mutex FIFOReader::fifo_creation_mutex;

#endif