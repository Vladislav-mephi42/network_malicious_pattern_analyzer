#include "../net_classes/net_logger.h"
#include "../net_classes/net_sniffer.h"
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
  try {
    std::string iface = (argc > 1) ? argv[1] : "lo";
    std::string filter =
        (argc > 2) ? argv[2] : "tcp port 7009 or tcp port 0 or icmp";
    NetLogger logger;
    NetSniffer sniffer(iface, filter, 20, logger);
    sniffer.run();
    json data = sniffer.get_data();
    std::cout << data.dump(4) << std::endl;

    Client_socket client;
    client.connect("127.0.0.1", 7009);

    client.send_json(data);
    auto new_data = client.recv_json();
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Report from server: " << new_data.dump(4) << std::endl;
    std::cout << std::endl;
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}