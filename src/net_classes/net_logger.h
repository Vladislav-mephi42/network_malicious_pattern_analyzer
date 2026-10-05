#ifndef NET_LOGGER_H
#define NET_LOGGER_H

#include <nlohmann/json.hpp>
#include <tins/tins.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace Tins;

class LogStrategy {
public:
  virtual bool can_log(const PDU &pdu) const = 0;
  virtual json log(const PDU &pdu) const = 0;
  virtual ~LogStrategy() = default;
};

class TCPLog : public LogStrategy {

public:
  virtual bool can_log(const PDU &pdu) const override {
    auto tcp = pdu.find_pdu<TCP>();
    if (tcp == nullptr) {
      return false;
    }
    return true;
  }
  virtual json log(const PDU &pdu) const override {
    auto tcp = pdu.find_pdu<TCP>();
    if (tcp == nullptr) {

      throw std::runtime_error("logging failed");
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
    info["flags"]["FIN"] = static_cast<bool>(tcp->get_flag(TCP::FIN));
    info["flags"]["SYN"] = static_cast<bool>(tcp->get_flag(TCP::SYN));
    info["flags"]["RST"] = static_cast<bool>(tcp->get_flag(TCP::RST));
    info["flags"]["PSH"] = static_cast<bool>(tcp->get_flag(TCP::PSH));
    info["flags"]["ACK"] = static_cast<bool>(tcp->get_flag(TCP::ACK));
    info["flags"]["URG"] = static_cast<bool>(tcp->get_flag(TCP::URG));
    info["flags"]["ECE"] = static_cast<bool>(tcp->get_flag(TCP::ECE));
    info["flags"]["CWR"] = static_cast<bool>(tcp->get_flag(TCP::CWR));

    return info;
  }
  ~TCPLog() override {}
};

class TCPIPLog : public LogStrategy {

public:
  virtual bool can_log(const PDU &pdu) const override {
    auto tcp = pdu.find_pdu<TCP>();
    auto ip = pdu.find_pdu<IP>();
    if (tcp == nullptr || ip == nullptr) {
      return false;
    }
    return true;
  }
  virtual json log(const PDU &pdu) const override {
    auto tcp = pdu.find_pdu<TCP>();
    auto ip = pdu.find_pdu<IP>();
    if (tcp == nullptr || ip == nullptr) {

      throw std::runtime_error("logging failed");
    }
    json info;

    info["protocol"] = "TCP-IP";
    info["src_ip"] = (ip->src_addr()).to_string();
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
    info["flags"]["FIN"] = static_cast<bool>(tcp->get_flag(TCP::FIN));
    info["flags"]["SYN"] = static_cast<bool>(tcp->get_flag(TCP::SYN));
    info["flags"]["RST"] = static_cast<bool>(tcp->get_flag(TCP::RST));
    info["flags"]["PSH"] = static_cast<bool>(tcp->get_flag(TCP::PSH));
    info["flags"]["ACK"] = static_cast<bool>(tcp->get_flag(TCP::ACK));
    info["flags"]["URG"] = static_cast<bool>(tcp->get_flag(TCP::URG));
    info["flags"]["ECE"] = static_cast<bool>(tcp->get_flag(TCP::ECE));
    info["flags"]["CWR"] = static_cast<bool>(tcp->get_flag(TCP::CWR));

    return info;
  }
  ~TCPIPLog() override {}
};

class NetLogger {
private:
  std::vector<std::shared_ptr<LogStrategy>> strategies;

public:
  NetLogger() {
    TCPIPLog tcpip;
    TCPLog tcp;
    strategies.push_back(std::make_shared<TCPLog>(tcp));
    strategies.push_back(std::make_shared<TCPIPLog>(tcpip));
  }
  NetLogger(const std::vector<std::shared_ptr<LogStrategy>> &strategies)
      : strategies(strategies) {}
  json log(const PDU &pdu) {
    json data = json::array();
    for (const auto &elem : strategies) {
      if (elem->can_log(pdu)) {
        auto m = elem->log(pdu);
        data.push_back(m);
      }
    }
    return data;
  }
};

#endif