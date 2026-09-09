#include "../sys_classes/fifo.h"
#include "../sys_classes/shared_memory.h"
#include "../sys_classes/sockets.h"
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <thread>
#include <time.h>
#include <unistd.h>

using json = nlohmann::json;

void handle_fifo() {
  try {
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    json data;
    data["counter"] = 0;
    auto size = data.dump().size() + 1024;
    MemoryParent parent("/memory.shm", size, "/semophore.sem");
    parent.write_json(data);
    try {
      while (true) {
        std::string w_path = "./1.fifo";
        {

          std::string r_path = "./2.fifo";
          FIFOReader reader(r_path, true);
          auto data = reader.read_json();
          if (!data.contains("global_stat")) {
            throw std::runtime_error("Bad fifo message");
          }
        }
        {
          FIFOWriter writer(w_path, true);
          auto message = parent.read_json();
          writer.write_json(message);
        }
      }
    } catch (const std::exception &e) {
      std::cout << e.what() << std::endl;
    }
    _exit(0);
  } catch (...) {
    _exit(1);
  }
}

void handle_client(Client_socket &socket, size_t size) {
  try {
    auto data = socket.recv_json();
    if (!data.contains("counter")) {
      std::cout << "fail" << std::endl;
      return;
    }
    int counter = data["counter"].get<int>();
    MemoryChild child("/memory.shm", size, "/semophore.sem");
    auto new_data = child.read_json();
    auto new_counter = new_data["counter"].get<int>();
    new_counter += counter;
    new_data["counter"] = new_counter;
    child.write_json(new_data);
    socket.send_json(new_data);
    exit(0);
  } catch (...) {
    _exit(1);
  }
}

void sigchld_handler(int sig) {
  while (waitpid(-1, nullptr, WNOHANG) > 0) {
  }
}

void sigint_handler(int sig) {
  std::cout << "Finish work......" << std::endl;
  _exit(0);
}

int main() {
  struct sigaction sa = {};
  sa.sa_handler = sigchld_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
    std::cout << "Error with sigaction" << std::endl;
    return 1;
  }
  sa.sa_handler = sigint_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, nullptr) == -1) {
    std::cout << "Error with sigaction" << std::endl;
  }
  auto pid_1 = fork();
  if (pid_1 == 0) {
    handle_fifo();
  }
  try {
    json data;
    data["counter"] = 0;
    auto size = data.dump().size() + 1024;
    Server_socket server;
    server.bind(7009);
    server.listen(2);
    while (true) {
      auto client = server.accept();
      auto pid = fork();
      if (pid > 0) {
        client.manualy_close();
      }
      if (pid == 0) {

        server.manualy_close();
        handle_client(client, size);

        return 0;
      }
    }

  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}