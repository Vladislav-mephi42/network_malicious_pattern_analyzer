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
  std::string w_path = "./2.fifo";
  std::string r_path = "./1.fifo";
  {
    FIFOWriter writer(w_path, false);
    json data;
    data["global_stat"] = "yes";
    writer.write_json(data);
  }
  {
    FIFOReader reader(r_path, false);
    auto res = reader.read_json();
    std::cout << res.dump(4) << std::endl;
  }
}