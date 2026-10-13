#ifndef NET_LOGGER_H
#define NET_LOGGER_H

#include <fstream>
#include <iostream>
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

class ICMPLog : public LogStrategy {

public:
  virtual bool can_log(const PDU &pdu) const override {
    auto icmp = pdu.find_pdu<ICMP>();
    auto ip = pdu.find_pdu<IP>();

    if (icmp == nullptr || ip == nullptr) {
      return false;
    }
    return true;
  }
  virtual json log(const PDU &pdu) const override {
    auto icmp = pdu.find_pdu<ICMP>();
    auto ip = pdu.find_pdu<IP>();

    if (icmp == nullptr || ip == nullptr) {

      throw std::runtime_error("logging failed");
    }
    json info;

    info["protocol"] = "ICMP";
    info["type"] = icmp->type();
    info["code"] = icmp->code();
    info["size"] = icmp->size();
    info["src_ip"] = (ip->src_addr()).to_string();
    return info;
  }
  ~ICMPLog() override {}
};

std::string get_default_gateway_ip() {
  std::ifstream route_file("/proc/net/route");
  if (!route_file.is_open()) {
    std::cerr << "Cannot open /proc/net/route" << std::endl;
    return "";
  }

  std::string line;
  std::getline(route_file, line);

  std::string best_gateway_hex = "";
  uint32_t best_metric = std::numeric_limits<uint32_t>::max();

  while (std::getline(route_file, line)) {
    std::istringstream iss(line);
    std::string iface, dest_hex, gateway_hex, flags, refcnt, use, metric_str;

    if (iss >> iface >> dest_hex >> gateway_hex >> flags >> refcnt >> use >>
        metric_str) {
      if (dest_hex == "00000000") {
        try {
          uint32_t current_metric = std::stoul(metric_str);
          if (current_metric < best_metric) {
            best_metric = current_metric;
            best_gateway_hex = gateway_hex;
          }
        } catch (const std::exception &) {
          continue;
        }
      }
    }
  }

  if (best_gateway_hex.empty() || best_gateway_hex == "00000000") {
    return "";
  }

  try {

    uint32_t gateway_raw = std::stoul(best_gateway_hex, nullptr, 16);

    struct in_addr addr;
    addr.s_addr = gateway_raw;

    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN) != nullptr) {
      return std::string(ip_str);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error parsing gateway: " << e.what() << std::endl;
  }

  return "";
}

std::string get_gateway_mac(const std::string &gateway_ip) {
  std::ifstream arp_file("/proc/net/arp");
  if (!arp_file.is_open()) {
    std::cerr << "Cannot open /proc/net/arp" << std::endl;
    return "";
  }

  std::string line;
  std::getline(arp_file, line);

  while (std::getline(arp_file, line)) {
    std::istringstream iss(line);
    std::string ip, hw_type, flags, mac, mask, device;

    if (iss >> ip >> hw_type >> flags >> mac >> mask >> device) {

      if (ip == gateway_ip) {
        if (flags == "0x2") {
          return mac;

        } else {
          std::cerr << "Gateway IP found in ARP table, but state is incomplete "
                       "(flags: "
                    << flags << ")" << std::endl;
          return "";
        }
      }
    }
  }

  std::cerr << "Gateway IP " << gateway_ip << " not found in ARP table."
            << std::endl;
  return "";
}

class ARPLog : public LogStrategy {
private:
  std::string gateway_ip;
  std::string gateway_mac;

public:
  ARPLog() {
    gateway_ip = get_default_gateway_ip();
    if (gateway_ip == "") {
      throw std::runtime_error("get default gateway ip failed");
    }
    gateway_mac = get_gateway_mac(gateway_ip);
    if (gateway_mac == "") {
      throw std::runtime_error("get default gateway mac failed");
    }
  }
  virtual bool can_log(const PDU &pdu) const override {
    auto arp = pdu.find_pdu<ARP>();
    return arp != nullptr;
  }

  virtual json log(const PDU &pdu) const override {
    auto arp = pdu.find_pdu<ARP>();

    if (arp == nullptr) {
      throw std::runtime_error("logging failed: not an ARP packet");
    }

    json info;
    info["protocol"] = "ARP";
    info["opcode"] = arp->opcode();
    info["sender_ip"] = arp->sender_ip_addr().to_string();
    info["sender_mac"] = arp->sender_hw_addr().to_string();
    info["target_ip"] = arp->target_ip_addr().to_string();
    info["target_mac"] = arp->target_hw_addr().to_string();
    info["hw_type"] = arp->hw_addr_format();

    info["proto_format"] = arp->prot_addr_format();
    info["hw_len"] = arp->hw_addr_length();
    info["proto_len"] = arp->prot_addr_length();
    info["size"] = arp->size();
    info["gateway_ip"] = gateway_ip;
    info["gateway_mac"] = gateway_mac;
    return info;
  }

  ~ARPLog() override {}
};

class DNSLog : public LogStrategy {
private:
  std::string gateway_ip;

public:
  virtual bool can_log(const PDU &pdu) const override {
    auto dns = pdu.find_pdu<DNS>();
    auto udp = pdu.find_pdu<UDP>();
    return (dns != nullptr && udp != nullptr);
  }

  virtual json log(const PDU &pdu) const override {
    auto dns = pdu.find_pdu<DNS>();

    if (dns == nullptr) {
      throw std::runtime_error("logging failed: not an ARP packet");
    }

    json info;
    info["protocol"] = "DNS";
    info["query_type"] = dns->type();
    info["opcode"] = dns->opcode();
    info["A A"] = dns->authoritative_answer();
    info["TC"] = dns->truncated();
    info["RD"] = dns->recursion_desired();
    info["RA"] = dns->recursion_available();
    info["ID"] = dns->id();
    info["rcode"] = dns->rcode();
    info["authenticated_data"] = dns->authenticated_data();
    info["questions_count"] = dns->questions_count();
    info["answers_count"] = dns->answers_count();
    info["authority_count"] = dns->authority_count();
    info["additional_count"] = dns->additional_count();
    info["header_size"] = dns->header_size();
    info["size"] = dns->size();
    return info;
  }

  ~DNSLog() override {}
};

class NetLogger {
private:
  std::vector<std::shared_ptr<LogStrategy>> strategies;

public:
  NetLogger() {
    TCPIPLog tcpip;
    TCPLog tcp;
    ICMPLog icmp;
    strategies.push_back(std::make_shared<TCPLog>(tcp));
    strategies.push_back(std::make_shared<TCPIPLog>(tcpip));
    strategies.push_back(std::make_shared<ICMPLog>(icmp));
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