#define CATCH_CONFIG_MAIN
#include "shared_memory.h"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <thread>

TEST_CASE("Base") {
#ifndef MEMORY_LOST
  SECTION("Allredy exist error") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";

    SharedMemory parent(mem_name, 10, sem_name, true);

    SharedMemory child(mem_name, 10, sem_name, false);
    REQUIRE_THROWS(child = SharedMemory(mem_name, 10, sem_name, true));
  }
#endif

  SECTION("json transmission") {
    json data;
    data["res"] = "HELLO";
    json new_data;
    size_t data_size = data.dump().size() + sizeof(uint32_t) + 1;
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    REQUIRE_NOTHROW(parent.write_json(data));
    REQUIRE_NOTHROW(new_data = child.read_json());
    REQUIRE(new_data == data);
    REQUIRE(new_data["res"] == "HELLO");
  }

  SECTION("Empty json transmission") {
    json data = json::object();
    json new_data;
    size_t data_size = data.dump().size() + sizeof(uint32_t) + 1;
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    REQUIRE_NOTHROW(parent.write_json(data));
    REQUIRE_NOTHROW(new_data = child.read_json());
    REQUIRE(new_data == data);
  }

  SECTION("Array json transmission") {
    json data = json::array({1, 2, 3, "four", 5.0});
    json new_data;
    size_t data_size = data.dump().size() + sizeof(uint32_t) + 1;
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    REQUIRE_NOTHROW(parent.write_json(data));
    REQUIRE_NOTHROW(new_data = child.read_json());
    REQUIRE(new_data == data);
  }

  SECTION("Nested json transmission") {
    json data;
    data["a"]["b"]["c"] = 42;
    data["arr"] = {1, 2, {3, 4}};
    json new_data;
    size_t data_size = data.dump().size() + sizeof(uint32_t) + 1;
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    REQUIRE_NOTHROW(parent.write_json(data));
    REQUIRE_NOTHROW(new_data = child.read_json());
    REQUIRE(new_data == data);
  }

  SECTION("Repeated write and read") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["counter"] = 0;
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    for (int i = 0; i < 10; ++i) {
      data["counter"] = i;
      REQUIRE_NOTHROW(parent.write_json(data));
      json new_data;
      REQUIRE_NOTHROW(new_data = child.read_json());
      REQUIRE(new_data["counter"] == i);
    }
  }

  SECTION("Write bigger than memory") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] = std::string(2048, 'X');
    size_t data_size = data.dump().size() / 2;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    REQUIRE_THROWS(parent.write_json(data));
  }

  SECTION("Read after write from another instance") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["msg"] = "from child";
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    REQUIRE_NOTHROW(child.write_json(data));
    json new_data;
    REQUIRE_NOTHROW(new_data = parent.read_json());
    REQUIRE(new_data["msg"] == "from child");
  }

  SECTION("Multiple children read same value") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["value"] = 123;
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent.write_json(data);
    for (int i = 0; i < 5; ++i) {
      SharedMemory child(mem_name, data_size, sem_name, false);
      json new_data = child.read_json();
      REQUIRE(new_data["value"] == 123);
    }
  }

  SECTION("Concurrent reading") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAA";
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent.write_json(data);
    std::atomic<int> counter(0);
    auto worker = [&counter, &mem_name, &sem_name, data_size]() {
      SharedMemory child(mem_name, data_size, sem_name, false);

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
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent.write_json(data);
    std::atomic<int> counter(0);
    data["payload"] = "(-|-)";
    auto worker = [&data, &mem_name, &sem_name, data_size]() {
      SharedMemory child(mem_name, data_size, sem_name, false);
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
    new_data = parent.read_json();
    REQUIRE(new_data["payload"] == "(-|-)");
  }

  SECTION("Move constructor") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["x"] = 1;
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory moved(std::move(parent));
    REQUIRE_NOTHROW(moved.write_json(data));
    SharedMemory child(mem_name, data_size, sem_name, false);
    json new_data = child.read_json();
    REQUIRE(new_data["x"] == 1);
  }

  SECTION("Move assignment") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["y"] = 2;
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory other(mem_name, data_size, sem_name, false);
    other = std::move(parent);
    REQUIRE_NOTHROW(other.write_json(data));
    SharedMemory child(mem_name, data_size, sem_name, false);
    json new_data = child.read_json();
    REQUIRE(new_data["y"] == 2);
  }

  SECTION("Self move assignment") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["z"] = 3;
    size_t data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent = std::move(parent);
    REQUIRE_NOTHROW(parent.write_json(data));
    SharedMemory child(mem_name, data_size, sem_name, false);
    json new_data = child.read_json();
    REQUIRE(new_data["z"] == 3);
  }

  SECTION("Bad memory name") {
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    REQUIRE_THROWS(SharedMemory("bad_name", 10, sem_name, true));
    REQUIRE_THROWS(SharedMemory("", 10, sem_name, true));
  }

  SECTION("Bad semaphore name") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    REQUIRE_THROWS(SharedMemory(mem_name, 10, "bad_sem", true));
  }

  SECTION("Zero size memory") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    REQUIRE_THROWS(SharedMemory(mem_name, 0, sem_name, true));
  }
}

TEST_CASE("Advanced") {
  SECTION("Cuncurrent writing/reading") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json A_data, B_data;
    A_data["payload"] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    A_data["id"] = "A";
    B_data["payload"] =
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBB";
    B_data["id"] = "B";
    auto A_data_size = A_data.dump().size() + 1024;
    auto B_data_size = B_data.dump().size() + 1024;
    SharedMemory parent(mem_name, std::max(A_data_size, B_data_size), sem_name,
                        true);
    parent.write_json(A_data);
    std::atomic<int> counter(0);

    auto A_worker = [&A_data, &mem_name, &sem_name, A_data_size]() {
      SharedMemory child(mem_name, A_data_size, sem_name, false);
      child.write_json(A_data);
    };
    auto B_worker = [&B_data, &mem_name, &sem_name, B_data_size]() {
      SharedMemory child(mem_name, B_data_size, sem_name, false);
      child.write_json(B_data);
    };
    std::vector<std::thread> th;
    json new_data;
    for (int i = 0; i < 5; i++) {
      th.push_back(std::thread(A_worker));
      th.push_back(std::thread(B_worker));
      new_data = parent.read_json();
    }

    for (auto &elem : th) {
      elem.join();
    }
    REQUIRE((new_data["id"] == "A" || new_data["id"] == "B"));
  }

  SECTION("Many writers same payload") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] = std::string(512, 'Z');
    data["id"] = "same";
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent.write_json(data);
    std::atomic<int> counter(0);
    auto worker = [&data, &mem_name, &sem_name, data_size]() {
      SharedMemory child(mem_name, data_size, sem_name, false);
      child.write_json(data);
    };
    std::vector<std::thread> th;
    for (int i = 0; i < 10; ++i) {
      th.push_back(std::thread(worker));
    }
    for (auto &elem : th) {
      elem.join();
    }
    json new_data = parent.read_json();
    REQUIRE(new_data["id"] == "same");
    REQUIRE(new_data["payload"] == std::string(512, 'Z'));
  }

  SECTION("Many readers different sizes") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["payload"] = std::string(1024, 'R');
    data["id"] = "read";
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    parent.write_json(data);
    std::atomic<int> counter(0);
    auto worker = [&counter, &mem_name, &sem_name, data_size]() {
      SharedMemory child(mem_name, data_size, sem_name, false);
      json new_data = child.read_json();
      if (new_data["id"] == "read") {
        counter.fetch_add(1);
      }
    };
    std::vector<std::thread> th;
    for (int i = 0; i < 10; ++i) {
      th.push_back(std::thread(worker));
    }
    for (auto &elem : th) {
      elem.join();
    }
    REQUIRE(counter.load() == 10);
  }

  SECTION("Size error") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json A_data, B_data;
    A_data["payload"] = "A";
    A_data["id"] = "A";
    B_data["payload"] =
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBB";
    B_data["id"] = "B";
    auto A_data_size = A_data.dump().size() + 2;
    auto B_data_size = B_data.dump().size() + 2;
    SharedMemory parent(mem_name, std::min(A_data_size, B_data_size), sem_name,
                        true);
    parent.write_json(A_data);
    SharedMemory child(mem_name, A_data_size, sem_name, false);
#ifndef MEMORY_LOST
    REQUIRE_NOTHROW(child =
                        SharedMemory(mem_name, A_data_size, sem_name, false));
    REQUIRE_THROWS(child =
                       SharedMemory(mem_name, B_data_size, sem_name, false));
#endif
  }

  SECTION("Child larger than parent") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["x"] = 1;
    auto small_size = data.dump().size() + 2;
    auto big_size = small_size + 1024;
    SharedMemory parent(mem_name, small_size, sem_name, true);
    REQUIRE_THROWS(SharedMemory(mem_name, big_size, sem_name, false));
  }

  SECTION("Child smaller than parent") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["x"] = 1;
    auto big_size = data.dump().size() + 1024;
    auto small_size = data.dump().size() + 2;
    SharedMemory parent(mem_name, big_size, sem_name, true);
    REQUIRE_NOTHROW(SharedMemory(mem_name, small_size, sem_name, false));
  }

  SECTION("Multiple independent memories") {
    std::string mem_name1 = "/memory1_" + std::to_string(getpid()) + ".shm";
    std::string sem_name1 = "/sem1_" + std::to_string(getpid()) + ".sem";
    std::string mem_name2 = "/memory2_" + std::to_string(getpid()) + ".shm";
    std::string sem_name2 = "/sem2_" + std::to_string(getpid()) + ".sem";
    json data1;
    data1["id"] = 1;
    json data2;
    data2["id"] = 2;
    auto size1 = data1.dump().size() + 1024;
    auto size2 = data2.dump().size() + 1024;
    SharedMemory parent1(mem_name1, size1, sem_name1, true);
    SharedMemory parent2(mem_name2, size2, sem_name2, true);
    parent1.write_json(data1);
    parent2.write_json(data2);
    SharedMemory child1(mem_name1, size1, sem_name1, false);
    SharedMemory child2(mem_name2, size2, sem_name2, false);
    REQUIRE(child1.read_json()["id"] == 1);
    REQUIRE(child2.read_json()["id"] == 2);
  }

  SECTION("Payload exactly fits") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["msg"] = "exact";
    auto payload_size = data.dump().size();
    auto data_size = payload_size;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    REQUIRE_NOTHROW(parent.write_json(data));
    SharedMemory child(mem_name, data_size, sem_name, false);
    json new_data = child.read_json();
    REQUIRE(new_data["msg"] == "exact");
  }

  SECTION("Payload slightly too big") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["msg"] = "too big";
    auto payload_size = data.dump().size();
    auto data_size = payload_size - 1;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    REQUIRE_THROWS(parent.write_json(data));
  }

  SECTION("Large json transmission") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["arr"] = json::array();
    for (int i = 0; i < 1000; ++i) {
      data["arr"].push_back(i);
    }
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    parent.write_json(data);
    json new_data = child.read_json();
    REQUIRE(new_data == data);
  }

  SECTION("Boolean and null json transmission") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["flag"] = true;
    data["nothing"] = nullptr;
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    parent.write_json(data);
    json new_data = child.read_json();
    REQUIRE(new_data == data);
  }

  SECTION("Numeric json transmission") {
    std::string mem_name = "/memory_" + std::to_string(getpid()) + ".shm";
    std::string sem_name = "/sem_" + std::to_string(getpid()) + ".sem";
    json data;
    data["int"] = 42;
    data["double"] = 3.14159;
    data["negative"] = -7;
    auto data_size = data.dump().size() + 1024;
    SharedMemory parent(mem_name, data_size, sem_name, true);
    SharedMemory child(mem_name, data_size, sem_name, false);
    parent.write_json(data);
    json new_data = child.read_json();
    REQUIRE(new_data == data);
  }
}