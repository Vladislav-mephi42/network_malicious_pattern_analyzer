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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <time.h>
#include <unistd.h>

using json = nlohmann::json;

int main() {
  Client_socket client;
  client.connect("127.0.0.1", 7009);
  json data;
  int counter = 0;
  std::cout << "Enter number: " << std::flush;
  std::cin >> counter;
  std::cout << std::endl;
  data["counter"] = counter;
  client.send_json(data);
  auto new_data = client.recv_json();

  std::cout << "Counter from client: " << counter << std::endl;
  std::cout << "Counter from server: " << new_data["counter"].get<int>()
            << std::endl;
  std::cout << std::endl;
}