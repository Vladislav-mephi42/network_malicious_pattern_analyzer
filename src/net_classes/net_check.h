#ifndef NET_CHECK_H
#define NET_CHECK_H

#include <nlohmann/json.hpp>
#include <tins/tins.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace Tins;

class CheckStrategy {
public:
  virtual bool can_check(const json &log_array) const = 0;
  virtual json check(const json &log_array) const = 0;
};

class ToMuchSYNCheck {
  virtual bool can_check(const json &log_array) const = 0;
  virtual json check(const json &log_array) const = 0;
}

#endif