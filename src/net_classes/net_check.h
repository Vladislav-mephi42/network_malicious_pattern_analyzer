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
      report["res"] = "A lot of requests with SYNC flag from one ip";
      report["level"] = "[LOW]";
      report["header"] = "to much sync";
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
        std::string res = "A lot of requests with SYNC flag from one ip. IP: ";
        res += elem.first;
        res += " Number of packates: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[LOW]";
        std::string header = "to much sync from the same ip/";
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
        std::string res = "A lot of requests with wrong combination of flags "
                          "from one ip. IP: ";
        res += elem.first;
        res += " Number of packates: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[MEDIUM]";
        std::string header =
            "to much request with wrong combination of flags from the same ip/";
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
        std::string res = "A lot of requests with 0 dst port "
                          "from one ip. IP: ";
        res += elem.first;
        res += " Number of packates: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[HIGH]";
        std::string header =
            "to much request with 0 dst port from the same ip/";
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
        std::string res = "A lot of echo requests from one ip. IP: ";
        res += elem.first;
        res += " Number of packates: ";
        res += std::to_string(elem.second);
        report["res"] = res;
        report["level"] = "[MEDIUM]";
        std::string header = "to much echo requests from the same ip/";
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

#endif