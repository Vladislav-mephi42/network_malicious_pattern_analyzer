#include "../net_classes/net_check.h"
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
volatile sig_atomic_t keep_running = 0;
volatile sig_atomic_t keep_fifo_running = 0;
constexpr size_t size = 500 * 40;

class Fifo_file {
private:
  std::string fifo_name;

public:
  Fifo_file(const std::string &fifo_name) : fifo_name(fifo_name) {
    if (!fifo_exists(fifo_name)) {
      if (mkfifo(fifo_name.c_str(), 0666) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "Failed to create FIFO");
      }
    }
  }
  ~Fifo_file() {
    if (fifo_exists(fifo_name)) {
      unlink(fifo_name.c_str());
    }
  }
};

bool set_pdeathsig_or_die(int sig = SIGTERM) {
  pid_t parent_before = getppid();
  if (prctl(PR_SET_PDEATHSIG, sig) == -1) {
    perror("prctl(PR_SET_PDEATHSIG)");
    return false;
  }
  pid_t parent_after = getppid();
  if (parent_after != parent_before || parent_after == 1) {
    return false;
  }
  return true;
}

void handle_fifo() {
  try {
    if (!set_pdeathsig_or_die()) {
      return;
    }

    try {
      std::string w_path = "./1.fifo";
      std::string r_path = "./2.fifo";
      Fifo_file f_w(w_path);
      Fifo_file f_r(r_path);
      json data;
      data["Messages"] = "packages numbers";

      SharedMemory parent("/memory.shm", size, "/semophore.sem", true);
      parent.write_json(data);
      while (keep_fifo_running == 0) {
        {
          FIFOReader reader(r_path, false);
          auto data = reader.read_json();
          if (!data.contains("global_stat")) {
            throw std::runtime_error("Bad fifo message");
          }
        }
        {
          FIFOWriter writer(w_path, false);
          auto message = parent.read_json();
          writer.write_json(message);
        }
      }
    } catch (const std::exception &e) {
      return;
    }

  } catch (...) {
    return;
  }
  return;
}

void handle_client(Client_socket &socket, size_t size) {
  try {
    std::vector<std::shared_ptr<CheckStrategy>> checks;
    checks.push_back(std::make_shared<ToMuchSYNCheck>(1));
    checks.push_back(std::make_shared<ToMuchSYNCheckWithSameIP>(1));
    checks.push_back(std::make_shared<ICMPDDosCheck>(1));
    checks.push_back(std::make_shared<WrongCombOfTCPFlagsWithSameIP>(1));
    checks.push_back(std::make_shared<ZeroPortVulnerability>(1));
    auto recv_data = socket.recv_json();

    SharedMemory child("/memory.shm", size, "/semophore.sem", false);
    auto new_data = child.read_json();
    json response = json::array();
    for (const auto &check : checks) {
      if (check->can_check(recv_data)) {
        json data_array = check->check(recv_data);
        for (auto &data : data_array) {
          data["res"] =
              data["level"].get<std::string>() + data["res"].get<std::string>();
          response.push_back(data["res"]);
          if (new_data.contains(data["header"])) {
            int n = new_data[data["header"]];
            n++;
            new_data[data["header"]] = n;

          } else {
            new_data[data["header"]] = 1;
          }
        }
      }
    }

    child.write_json(new_data);
    socket.send_json(response);

  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return;
  }
  return;
}

void sigchld_handler(int sig) {
  while (waitpid(-1, nullptr, WNOHANG) > 0) {
  }
}

void sigint_handler(int sig) {
  std::string finish = "Finish work......\n";
  write(1, finish.c_str(), strlen(finish.c_str()));
  keep_running = 1;
  keep_fifo_running = 1;
}

void sigterm_handler(int sig) {
  keep_running = 1;
  keep_fifo_running = 1;
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
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, nullptr) == -1) {
    std::cout << "Error with sigaction" << std::endl;
  }

  sa.sa_handler = sigterm_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTERM, &sa, nullptr) == -1) {
    std::cout << "Error with sigaction" << std::endl;
  }

  std::cout << "Start working..." << std::endl;

  auto pid_1 = fork();
  if (pid_1 == 0) {
    handle_fifo();
    exit(1);
  }
  std::cout << "Create fifo handle process" << std::endl;
  try {
    json data;
    data["counter"] = 0;

    Server_socket server;
    server.bind(7009);
    server.listen(2);
    while (keep_running == 0) {
      auto client = server.accept();
      std::cout << "accept client" << std::endl;
      auto pid = fork();
      if (pid > 0) {
        client.manualy_close();
      }
      if (pid == 0) {
        server.manualy_close();
        handle_client(client, size);
        exit(0);
      }
    }

  } catch (const std::exception &e) {
    if (keep_running == 0) {
      std::cout << e.what() << std::endl;
    }
  }
  kill(pid_1, SIGTERM);

  struct sigaction sa_default = {};
  sa_default.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &sa_default, nullptr);

  while (waitpid(-1, nullptr, 0) > 0 || errno == EINTR) {
  }

  return 0;
}