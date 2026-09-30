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
  SharedMemory(std::string memory_name, uint32_t size, std::string sem_name,
               bool creat_flag = true)
      : memory_name(std::move(memory_name)), sem_name(std::move(sem_name)),
        creat_flag(creat_flag), size(size) {

    if (memory_name.empty() || memory_name[0] != '/') {
      throw std::runtime_error("bad memory name");
    }

    if (creat_flag) {
      if ((fd = shm_open(memory_name.c_str(), O_RDWR | O_CREAT | O_EXCL,
                         0644)) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "shm_open failed");
      }

      if (ftruncate(fd, total_mapped_size()) < 0) {
        close(fd);
        shm_unlink(memory_name.c_str());
        throw std::system_error(errno, std::generic_category(),
                                "ftruncate failed");
      }

      sem = sem_open(sem_name.c_str(), O_CREAT | O_EXCL, 0644, 1);
      if (sem == SEM_FAILED) {
        shm_unlink(memory_name.c_str());
        close(fd);
        throw std::system_error(errno, std::generic_category(),
                                "sem_open failed");
      }

      memptr =
          static_cast<char *>(mmap(NULL, total_mapped_size(),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
      if (memptr == MAP_FAILED) {
        shm_unlink(memory_name.c_str());
        close(fd);
        sem_unlink(sem_name.c_str());
        sem_close(sem);
        throw std::system_error(errno, std::generic_category(), "bad mmap");
      }

      main_memptr = memptr;
      memptr = main_memptr + sizeof(uint32_t);
      memcpy(main_memptr, &size, sizeof(uint32_t));

    } else {
      if ((fd = shm_open(memory_name.c_str(), O_RDWR, 0)) < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "shm_open failed");
      }

      sem = sem_open(sem_name.c_str(), 0);
      if (sem == SEM_FAILED) {
        close(fd);
        throw std::system_error(errno, std::generic_category(),
                                "sem_open failed");
      }

      memptr =
          static_cast<char *>(mmap(NULL, total_mapped_size(),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
      if (memptr == MAP_FAILED) {
        close(fd);
        sem_close(sem);
        throw std::system_error(errno, std::generic_category(), "bad mmap");
      }

      main_memptr = memptr;
      memptr = main_memptr + sizeof(uint32_t);

      uint32_t real_size = 0;
      memcpy(&real_size, main_memptr, sizeof(uint32_t));

      if (real_size < size) {
        munmap(main_memptr, total_mapped_size());
        sem_close(sem);
        close(fd);
        throw std::runtime_error("Size of memory is less than requested size");
      }
    }
  }

  ~SharedMemory() {
    if (fd != -1) {
      if (main_memptr != nullptr && main_memptr != MAP_FAILED) {
        munmap(main_memptr, total_mapped_size());
      }
      if (sem != nullptr && sem != SEM_FAILED) {
        sem_close(sem);
      }
      close(fd);

      if (creat_flag) {
        shm_unlink(memory_name.c_str());
        sem_unlink(sem_name.c_str());
      }
    }
  }

  void swap(SharedMemory &other) noexcept {
    std::swap(memory_name, other.memory_name);
    std::swap(sem_name, other.sem_name);
    std::swap(creat_flag, other.creat_flag);
    std::swap(sem, other.sem);
    std::swap(memptr, other.memptr);
    std::swap(fd, other.fd);
    std::swap(size, other.size);
    std::swap(main_memptr, other.main_memptr);
  }

  SharedMemory(const SharedMemory &) = delete;
  SharedMemory &operator=(const SharedMemory &) = delete;

  SharedMemory(SharedMemory &&other) noexcept { swap(other); }

  SharedMemory &operator=(SharedMemory &&other) noexcept {
    if (this != &other) {
      swap(other);
    }
    return *this;
  }

  void write_json(const json &data) {
    auto payload = data.dump();
    uint32_t len = payload.size();
    uint32_t max_size = 0;
    memcpy(&max_size, main_memptr, sizeof(uint32_t));

    if (len > max_size) {
      throw std::runtime_error("payload size is bigger than memory size");
    }

    if (sem_wait(sem) == 0) {
      try {
        memcpy(memptr, &len, sizeof(uint32_t));
        memcpy(memptr + sizeof(uint32_t), payload.data(), len);
        if (sem_post(sem) < 0) {
          throw std::system_error(errno, std::generic_category(),
                                  "bad sem_post");
        }
        return;
      } catch (...) {
        sem_post(sem);
        throw;
      }
    }
    throw std::system_error(errno, std::generic_category(), "bad sem_wait");
  }

  json read_json() {
    if (sem_wait(sem) == 0) {
      try {
        uint32_t len = 0;
        memcpy(&len, memptr, sizeof(uint32_t));

        if (len > size) {
          throw std::runtime_error("bad buff len");
        }

        std::string payload;
        payload.resize(len);
        memcpy(payload.data(), memptr + sizeof(uint32_t), len);

        if (sem_post(sem) < 0) {
          throw std::system_error(errno, std::generic_category(),
                                  "bad sem_post");
        }
        return json::parse(payload);
      } catch (...) {
        sem_post(sem);
        throw;
      }
    }
    throw std::system_error(errno, std::generic_category(), "bad sem_wait");
  }
};

#endif