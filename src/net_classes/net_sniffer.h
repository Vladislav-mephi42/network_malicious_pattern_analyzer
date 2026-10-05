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

  json data = json::array();
  SnifferConfiguration build_config(const std::string &filter) {
    Tins::SnifferConfiguration cfg;
    cfg.set_promisc_mode(true);
    cfg.set_immediate_mode(true);
    cfg.set_snap_len(65535);
    if (!filter.empty()) {
      cfg.set_filter(filter);
    }
    return cfg;
  }

public:
  NetSniffer(const std::string &iface, const std::string &input_filter,
             int packages_number, const NetLogger &logger)
      : iface(iface), filter(input_filter), packages_number(packages_number),
        logger(logger), sniffer(iface, build_config(input_filter)) {
    SnifferConfiguration cfg;
    cfg.set_promisc_mode(true);
    cfg.set_immediate_mode(true);
    cfg.set_snap_len(65535);
    cfg.set_filter(filter);
  }
  void run() {
    sniffer.sniff_loop(make_sniffer_handler(this, &NetSniffer::handle_func),
                       packages_number);
  }

  bool handle_func(PDU &pdu) {
    json new_data = logger.log(pdu);
    data.insert(data.begin(), new_data.begin(), new_data.end());
    return true;
  }

  std::string get_iface() { return iface; }
  std::string get_filter() { return filter; }
  json get_data() { return data; }
};

#endif