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

  virtual bool can_check(const json &log_array) const {

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
  virtual json check(const json &log_array) const {

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
      report["res"] = "[A LOT OF SYNC FROM ONE IP]";
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

  virtual bool can_check(const json &log_array) const {

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
  virtual json check(const json &log_array) const {
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
        report["res"] = "[A LOT OF SYNC FROM ONE IP]";
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

class ICMPDDosCheck : public CheckStrategy {
private:
  size_t max_sync = 0;

public:
  ICMPDDosCheck(size_t max_sync) : max_sync(max_sync) {}
  virtual bool can_check(const json &log_array) const {

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
  virtual json check(const json &log_array) const {
    std::unordered_map<std::string, int> map;

    for (const auto &elem : log_array) {

      if (!elem.contains("protocol")) {
        throw std::runtime_error("bad json format");
      }
      if (elem["protocol"] == "ICMP") {

        if (elem.contains("flags")) {
          if (elem["type"] == "ECHO_REQUEST") {
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
        report["res"] = "[A LOT OF ECHO REQUESTS FROM ONE IP]";
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