#ifndef SHARED_H
#define SHARED_H

#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <semaphore.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

using json = nlohmann::json;

class SharedMemory {
private:
  std::string memory_name;
  std::string sem_name;
  int fd = -1;
  char *memptr = nullptr;
  char *main_memptr = nullptr;
  sem_t *sem = nullptr;
  bool creat_flag = false;
  uint32_t size = 0;

  size_t total_mapped_size() const { return size + 2 * sizeof(uint32_t); }

  SharedMemory() {}

public:
  SharedMemory(const std::string &memory_name, uint32_t size,
               const std::string &sem_name, bool creat_flag = true);

  ~SharedMemory();

  void swap(SharedMemory &other) noexcept;

  SharedMemory(const SharedMemory &) = delete;
  SharedMemory &operator=(const SharedMemory &) = delete;

  SharedMemory(SharedMemory &&other) noexcept { swap(other); }

  SharedMemory &operator=(SharedMemory &&other) noexcept {
    if (this != &other) {
      swap(other);
    }
    return *this;
  }

  void write_json(const json &data);

  json read_json();
};

#endif