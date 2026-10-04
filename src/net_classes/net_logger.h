#ifndef NET_LOGGER_H
#define NET_LOGGER_H

#include <nlohmann/json.hpp>
#include <tins/tins.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace Tins;

class LogStrategy {
public:
  virtual bool can_log(const PDU &pdu) const noexcept = 0;
  virtual json log(const PDU &pdu) const = 0;
  virtual ~LogStrategy();
};

class TCPLog : public LogStrategy {
  const PDU *tcp = nullptr;

public:
  virtual bool can_log(const PDU &pdu) const override {
    tcp = pdu.find_pdu<TCP>();
    if (tcp == nullptr) {
      return false;
    }
    return true;
  }
  virtual json log(const PDU &pdu) const override {
    if (tcp == nullptr) {
      tcp = pdu.find_pdu<TCP>();
      if (tcp == nullptr) {
        throw std::runtime_error("logging failed");
      }
    }
    json info;

    info["protocol"] = "TCP";

    info["sport"] = tcp->sport();
    info["dport"] = tcp->dport();
    info["seq"] = tcp->seq();
    info["ack_seq"] = tcp->ack_seq();
    info["header_len"] = tcp->header_size();
    info["total_len"] = tcp->size();
    info["payload_len"] = tcp->size() - tcp->header_size();
    info["window"] = tcp->window();
    info["checksum"] = tcp->checksum();
    info["urg_ptr"] = tcp->urg_ptr();

    uint8_t flags = tcp->flags();
    info["flags"]["raw"] = flags;
    info["flags"]["FIN"] = tcp->get_flag(TCP::FIN);
    info["flags"]["SYN"] = tcp->get_flag(TCP::SYN);
    info["flags"]["RST"] = tcp->get_flag(TCP::RST);
    info["flags"]["PSH"] = tcp->get_flag(TCP::PSH);
    info["flags"]["ACK"] = tcp->get_flag(TCP::ACK);
    info["flags"]["URG"] = tcp->get_flag(TCP::URG);
    info["flags"]["ECE"] = tcp->get_flag(TCP::ECE);
    info["flags"]["CWR"] = tcp->get_flag(TCP::CWR);
    info["flags"]["NS"] = tcp->get_flag(TCP::NS);
    return info;
  }
  ~TCPLog() override {}
};

class NetLogger {
private:
  std::vector<std::shared_ptr<LogStrategy>> strategies;

public:
  NetLogger(const std::vector<std::shared_ptr<LogStrategy>> &strategies)
      : strategies(strategies) {}
  json log(const PDU &pdu) {
    json data = json::array();
    for (const auto &elem : strategies) {
      if (elem->can_log(pdu)) {
        data.push_back(elem->log(pdu));
      }
    }
    return data;
  }
};

#endif