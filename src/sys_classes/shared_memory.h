#ifndef SHARED_H
#define SHARED_H
#include <string>

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <semaphore.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utility>
using json = nlohmann::json;

void write_n_to_memory(caddr_t memptr, off_t byte_size, off_t offset,
                       const void *buf, size_t n) {

  if (offset + n > static_cast<size_t>(byte_size)) {
    throw std::runtime_error("Data too large for shared memory");
  }

  memcpy(memptr + offset, buf, n);
}

void write_json(caddr_t memptr, off_t byte_size, const json &data) {
  auto str = data.dump();
  uint32_t n = str.size();

  if (sizeof(uint32_t) + n > static_cast<size_t>(byte_size)) {
    throw std::runtime_error("JSON data too large for shared memory");
  }

  memcpy(memptr, &n, sizeof(uint32_t));

  memcpy(memptr + sizeof(uint32_t), str.c_str(), n);
}

void read_n_from_memory(caddr_t memptr, off_t byte_size, off_t offset,
                        void *buf, size_t n) {

  if (offset + n > static_cast<size_t>(byte_size)) {
    throw std::runtime_error("Read beyond shared memory bounds");
  }

  memcpy(buf, memptr + offset, n);
}

json read_json(caddr_t memptr, off_t byte_size) {
  uint32_t n = 0;

  memcpy(&n, memptr, sizeof(uint32_t));

  if (sizeof(uint32_t) + n > static_cast<size_t>(byte_size)) {
    throw std::runtime_error("Invalid data size in shared memory");
  }

  std::string str;
  str.resize(n);
  memcpy(str.data(), memptr + sizeof(uint32_t), n);

  return json::parse(str);
}

class MemoryParent {
private:
  int fd = -1;

  bool delete_flag = true;
  caddr_t memptr = nullptr;
  sem_t *semptr = nullptr;
  std::string memory_name;
  off_t byte_size = 0;

public:
  MemoryParent(const std::string &mem_name, off_t size,
               const std::string &sem_name)
      : memory_name(mem_name), byte_size(size) {

    if (std::filesystem::exists("file.txt")) {
      throw std::runtime_error(
          "This shared memory is used by other instance of MemoryParent");
    }

    sem_t *sem = sem_open(sem_name.c_str(), O_EXCL);
    if (sem == SEM_FAILED) {
      if (errno == EEXIST) {
        throw std::runtime_error(
            "This semaphore is used by other instance of MemoryParent");
      }
    }

    fd = shm_open(memory_name.c_str(), O_RDWR | O_CREAT, 0664);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "shm_open failed");
    }
    if (ftruncate(fd, byte_size) < 0) {
      ::close(fd);

      shm_unlink(memory_name.c_str());

      throw std::system_error(errno, std::generic_category(),
                              "ftruncate failed");
    }
    memptr = static_cast<caddr_t>(
        mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (memptr == MAP_FAILED) {
      ::close(fd);

      shm_unlink(memory_name.c_str());

      throw std::system_error(errno, std::generic_category(), "mmap failed");
    }
    semptr = sem_open(sem_name.c_str(), O_CREAT, 0600, 1);
    if (semptr == SEM_FAILED) {

      shm_unlink(memory_name.c_str());

      munmap(memptr, byte_size);
      ::close(fd);
      throw std::system_error(errno, std::generic_category(),
                              "sem_open failed");
    }
  }
  void safe_memory() { delete_flag = false; }
  ~MemoryParent() {

    if (semptr != nullptr && semptr != SEM_FAILED)
      sem_close(semptr);
    if (memptr != nullptr && memptr != MAP_FAILED)
      munmap(memptr, byte_size);
    if (fd >= 0)
      ::close(fd);
    if (!memory_name.empty())
      if (delete_flag) {
        shm_unlink(memory_name.c_str());
      }
  }

  MemoryParent(const MemoryParent &) = delete;
  MemoryParent &operator=(const MemoryParent &) = delete;

  MemoryParent(MemoryParent &&other) noexcept
      : fd(other.fd), memptr(other.memptr), semptr(other.semptr),
        memory_name(std::move(other.memory_name)), byte_size(other.byte_size) {
    other.fd = -1;
    other.memptr = nullptr;
    other.semptr = nullptr;
    other.byte_size = 0;
  }

  MemoryParent &operator=(MemoryParent &&other) noexcept {
    if (this != &other) {
      MemoryParent tmp(std::move(other));
      swap(tmp);
    }
    return *this;
  }

  void swap(MemoryParent &other) noexcept {

    std::swap(fd, other.fd);
    std::swap(memptr, other.memptr);
    std::swap(semptr, other.semptr);
    std::swap(byte_size, other.byte_size);
    std::swap(memory_name, other.memory_name);
  }

  void write_json(const json &data) {
    if (sem_wait(semptr) < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "sem_wait failed");
    }
    try {
      ::write_json(memptr, byte_size, data);
      if (sem_post(semptr) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "sem_post failed");
      }
    } catch (...) {
      sem_post(semptr);
      throw;
    }
  }
  json read_json() {
    json data;
    if (sem_wait(semptr) < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "sem_wait failed");
    }
    try {
      data = ::read_json(memptr, byte_size);

      if (sem_post(semptr) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "sem_post failed");
      }
    } catch (...) {
      sem_post(semptr);
      throw;
    }
    return data;
  }
};

class MemoryChild {
private:
  int fd = -1;
  caddr_t memptr = nullptr;
  sem_t *semptr = nullptr;
  std::string memory_name;
  off_t byte_size = 0;

public:
  MemoryChild(const std::string &mem_name, off_t size,
              const std::string &sem_name)
      : memory_name(mem_name), byte_size(size) {
    fd = shm_open(memory_name.c_str(), O_RDWR, 0664);
    if (fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "shm_open failed");
    }

    memptr = static_cast<caddr_t>(
        mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (memptr == MAP_FAILED) {
      ::close(fd);
      throw std::system_error(errno, std::generic_category(), "mmap failed");
    }
    semptr = sem_open(sem_name.c_str(), 0);
    if (semptr == SEM_FAILED) {
      munmap(memptr, byte_size);
      ::close(fd);
      throw std::system_error(errno, std::generic_category(),
                              "sem_open failed");
    }
  }

  ~MemoryChild() {
    if (semptr != nullptr && semptr != SEM_FAILED)
      sem_close(semptr);
    if (memptr != nullptr && memptr != MAP_FAILED)
      munmap(memptr, byte_size);
    if (fd >= 0)
      ::close(fd);
  }

  MemoryChild(const MemoryChild &) = delete;
  MemoryChild &operator=(const MemoryChild &) = delete;

  MemoryChild(MemoryChild &&other) noexcept
      : fd(other.fd), memptr(other.memptr), semptr(other.semptr),
        memory_name(std::move(other.memory_name)), byte_size(other.byte_size) {
    other.fd = -1;
    other.memptr = nullptr;
    other.semptr = nullptr;
    other.byte_size = 0;
  }

  MemoryChild &operator=(MemoryChild &&other) noexcept {
    if (this != &other) {
      MemoryChild tmp(std::move(other));
      swap(tmp);
    }
    return *this;
  }

  void swap(MemoryChild &other) noexcept {

    std::swap(fd, other.fd);
    std::swap(memptr, other.memptr);
    std::swap(semptr, other.semptr);
    std::swap(byte_size, other.byte_size);
    std::swap(memory_name, other.memory_name);
  }

  void write_json(const json &data) {
    if (sem_wait(semptr) < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "sem_wait failed");
    }
    try {
      ::write_json(memptr, byte_size, data);
      if (sem_post(semptr) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "sem_post failed");
      }
    } catch (...) {
      sem_post(semptr);
      throw;
    }
  }
  json read_json() {
    json data;
    if (sem_wait(semptr) < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "sem_wait failed");
    }
    try {
      data = ::read_json(memptr, byte_size);

      if (sem_post(semptr) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "sem_post failed");
      }
    } catch (...) {
      sem_post(semptr);
      throw;
    }
    return data;
  }
};

#endif