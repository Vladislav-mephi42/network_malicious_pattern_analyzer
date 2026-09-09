#define CATCH_CONFIG_MAIN
#include "shared_memory.h"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <thread>
TEST_CASE("Default") {
  SECTION("json transmission") {
    json data;
    data["res"] = "HELLO";
    json new_data;
    size_t data_size = data.dump().size() + sizeof(uint32_t) + 1;
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    MemoryParent parent(mem_name, data_size, sem_name);
    MemoryChild child(mem_name, data_size, sem_name);
    REQUIRE_NOTHROW(parent.write_json(data));
    REQUIRE_NOTHROW(new_data = child.read_json());
    REQUIRE(new_data == data);
    REQUIRE(new_data["res"] == "HELLO");
  }
  SECTION("Concurrent reading") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAA";
    auto data_size = data.dump().size() + 1024;
    MemoryParent parent(mem_name, data_size, sem_name);
    parent.write_json(data);
    std::atomic<int> counter(0);
    auto worker = [&counter, &mem_name, &sem_name, data_size]() {
      MemoryChild child(mem_name, data_size, sem_name);

      auto data = child.read_json();
      if (data.contains("secret")) {
        counter.store(1);
      }
    };
    std::vector<std::thread> th;
    for (int i = 0; i < 5; i++) {
      th.push_back(std::thread(worker));

      if (i == 2) {
        data["secret"] = 1;
        parent.write_json(data);
      }
    }

    for (auto &elem : th) {
      elem.join();
    }
    REQUIRE(counter.load() == 1);
  }
  SECTION("Concurrent writing") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAA";
    auto data_size = data.dump().size() + 1024;
    MemoryParent parent(mem_name, data_size, sem_name);
    parent.write_json(data);
    std::atomic<int> counter(0);
    data["payload"] = "(-|-)";
    auto worker = [&data, &mem_name, &sem_name, data_size]() {
      MemoryChild child(mem_name, data_size, sem_name);

      child.write_json(data);
    };
    std::vector<std::thread> th;
    json new_data;
    for (int i = 0; i < 5; i++) {
      th.push_back(std::thread(worker));
      new_data = parent.read_json();
    }

    for (auto &elem : th) {
      elem.join();
    }
    REQUIRE(new_data["payload"] == "(-|-)");
  }
}