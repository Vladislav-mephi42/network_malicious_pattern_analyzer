#ifndef NET_SNIFFER_H
#define NET_SNIFFER_H

#include "net_logger.h"
#include <nlohmann/json.hpp>
#include <tins/tins.h>
#include <unistd.h>
using namespace Tins;

class NetSniffer {
private:
  Sniffer sniffer;
  std::string iface = iface;
  std::string filter;
  int packages_number = 0;
  NetLogger logger;
  std::string tmp_file;
  json data;

public:
  NetSniffer(const std::string &iface, const std::string &filter,
             int packages_number, const NetLogger &logger,
             const std::string &file_name = "/tmp.json")
      : iface(iface), filter(filter), packages_number(packages_number),
        logger(logger), tmp_file(file_name) {
    sniffer(iface);
    sniffer.set_filter(filter);
  }
  void run() {
    sniffer.sniff_loop(make_sniffer_handler(this, &NetSniffer::handle_func),
                       packages_number);
  }

  bool handle_func(const PDU &pdu) {
    json new_data = logger.log(pdu);
    data.insert(data.begin(), new_data.begin(), new_data.end());
  }

  std::string get_iface() { return iface; }
  std::string get_filter() { return filter; }
  json get_data() { return data; }
};

#endif