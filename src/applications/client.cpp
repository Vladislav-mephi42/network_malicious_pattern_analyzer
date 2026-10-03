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
#include <tins/tins.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace Tins;

bool handle_func(const PDU &pdu) { return true; }
int main(int argc, char **argv) {

  std::string iface = (argc > 1) ? argv[1] : "eth0";
  std::string filter = (argc > 2) ? argv[2] : "ip";
  SnifferConfiguration cfg;
  cfg.set_promisc_mode(true);
  cfg.set_immediate_mode(true);
  cfg.set_snap_len(65535);
  cfg.set_filter(filter);
  Sniffer sniffer(iface, cfg);
  sniffer.sniff_loop(handle_func);
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