#ifndef NET_CHECK_H
#define NET_CHECK_H

#include <iostream>
#include <nlohmann/json.hpp>
#include <tins/tins.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace Tins;

class CheckStrategy {
public:
  virtual bool can_check(const json &log_array) const = 0;
  virtual json check(const json &log_array) const = 0;
  virtual ~CheckStrategy() = default;
};

class ToMuchSYNCheck : public CheckStrategy {
private:
  size_t max_sync;

public:
  ToMuchSYNCheck(size_t max_sync) : max_sync(max_sync) {}

  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "TCP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {

    int i = 0;
    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "TCP") {

        if (elem.contains("flags")) {
          if ((elem["flags"]).contains("SYN")) {
            i++;
          }
        } else {
          throw std::runtime_error("bad json format(flags)");
        }
      }
    }

    json global_report = json::array();
    if (i >= max_sync) {
      json report;
      ;
      report["res"] = "High rate of SYN packets detected";
      report["level"] = "[LOW]";
      report["header"] = "High rate of SYN packets detected";
      report["flag"] = true;
      global_report.push_back(report);
    }
    return global_report;
  }
  ~ToMuchSYNCheck() override {}
};

class ToMuchSYNCheckWithSameIP : public CheckStrategy {
private:
  size_t max_sync;

public:
  ToMuchSYNCheckWithSameIP(size_t max_sync) : max_sync(max_sync) {}

  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "TCP-IP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "TCP-IP") {

        if (elem.contains("flags")) {
          if ((elem["flags"]).contains("SYN")) {
            map[elem["src_ip"]]++;
          }
        } else {
          throw std::runtime_error("bad json format(flags)");
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_sync) {
        json report;
        flag = true;
        std::string res = "High rate of SYN packets from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[LOW]";
        std::string header = "High rate of SYN packets from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ToMuchSYNCheckWithSameIP() override{};
};

class WrongCombOfTCPFlagsWithSameIP : public CheckStrategy {

private:
  size_t max_number = 0;

public:
  WrongCombOfTCPFlagsWithSameIP(size_t max_number) : max_number(max_number) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "TCP-IP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "TCP-IP") {

        if (elem.contains("flags")) {
          if ((elem["flags"]).contains("SYN") &&
              (elem["flags"]).contains("FIN")) {
            map[elem["src_ip"]]++;
          }
          if ((elem["flags"]).contains("SYN") &&
              (elem["flags"]).contains("RST")) {
            map[elem["src_ip"]]++;
          }
          if ((elem["flags"]).contains("FIN") &&
              (elem["flags"]).contains("RST")) {
            map[elem["src_ip"]]++;
          }
        } else {
          throw std::runtime_error("bad json format(flags)");
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_number) {
        json report;
        flag = true;
        std::string res = "Invalid TCP flag combination "
                          "from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[MEDIUM]";
        std::string header = "Invalid TCP flag combination from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~WrongCombOfTCPFlagsWithSameIP() override{};
};

class ZeroPortVulnerability : public CheckStrategy {

private:
  size_t max_number = 0;

public:
  ZeroPortVulnerability(size_t max_number) : max_number(max_number) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "TCP-IP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "TCP-IP") {

        if (elem["dport"] == 0) {
          map[elem["src_ip"]]++;
        }
      }
    }

    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_number) {
        json report;
        flag = true;
        std::string res = "Traffic targeting reserved port 0 "
                          "from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[HIGH]";
        std::string header =
            "Traffic targeting reserved port 0 from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ZeroPortVulnerability() override{};
};

class ICMPDDosCheck : public CheckStrategy {
private:
  size_t max_sync = 0;

public:
  ICMPDDosCheck(size_t max_sync) : max_sync(max_sync) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "ICMP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "ICMP") {

        if (elem["type"] == ICMP::ECHO_REQUEST) {
          map[elem["src_ip"]]++;
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_sync) {
        json report;
        flag = true;
        std::string res = "ICMP Echo Request flood from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[MEDIUM]";
        std::string header = "ICMP Echo Request flood from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ICMPDDosCheck() override {}
};

class ICMPVulnerableOldProtocol : public CheckStrategy {
private:
  size_t max_sync = 0;

public:
  ICMPVulnerableOldProtocol(size_t max_sync) : max_sync(max_sync) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "ICMP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "ICMP") {

        if (elem["type"] == ICMP::ADDRESS_MASK_REQUEST ||
            elem["type"] == ICMP::ADDRESS_MASK_REPLY ||
            elem["type"] == ICMP::INFO_REQUEST ||
            elem["type"] == ICMP::INFO_REPLY ||
            elem["type"] == ICMP::SOURCE_QUENCH) {
          map[elem["src_ip"]]++;
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_sync) {
        json report;
        flag = true;
        std::string res = "Insecure legacy protocol usage from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[HIGH]";
        std::string header = "Insecure legacy protocol usage from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ICMPVulnerableOldProtocol() override {}
};

class ICMPTunneling : public CheckStrategy {
private:
  size_t max_sync = 0;

public:
  ICMPTunneling(size_t max_sync) : max_sync(max_sync) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "ICMP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "ICMP") {

        if (elem["type"] == ICMP::ECHO_REQUEST &&
            elem["size"].get<uint32_t>() >= 1000) {
          map[elem["src_ip"]]++;
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_sync) {
        json report;
        flag = true;
        std::string res = "Oversized ICMP payload (Tunneling/Ping of Death) "
                          "from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[HIGH]";
        std::string header = "Oversized ICMP payload (Tunneling/Ping of Death) "
                             "from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ICMPTunneling() override {}
};

class ICMPMITMAttack : public CheckStrategy {
private:
  size_t max_sync = 0;

public:
  ICMPMITMAttack(size_t max_sync) : max_sync(max_sync) {}
  virtual bool can_check(const json &log_array) const override {

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {

        throw std::runtime_error("bad json format(protocol field)");
      }
      if (elem["protocol"] == "ICMP") {
        return true;
      }
    }
    return false;
  }
  virtual json check(const json &log_array) const override {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "ICMP") {

        if (elem["type"] == ICMP::REDIRECT) {
          map[elem["src_ip"]]++;
        }
      }
    }
    bool flag = false;
    json global_report = json::array();
    for (const auto &elem : map) {
      if (elem.second >= max_sync) {
        json report;
        flag = true;
        std::string res = "ICMP Redirect message detected from one ip. IP: ";
        res += elem.first;
        res += " Number of packages: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[HIGH]";
        std::string header = "ICMP Redirect message detected from the same ip/";
        header += elem.first;
        report["header"] = header;
        report["flag"] = true;
        global_report.push_back(report);
      }
    }

    return global_report;
  }
  ~ICMPMITMAttack() override {}
};

#endif