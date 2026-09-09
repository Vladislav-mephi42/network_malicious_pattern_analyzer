#define CATCH_CONFIG_MAIN
#include "fifo.h"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <set>
#include <thread>
#include <vector>

TEST_CASE("FIFO Basic Operations") {
  SECTION("JSON transmission with RAII") {

    std::thread writer_thread([]() {
      FIFOWriter writer("./test_fifo1");
      json output;
      output["hello"] = "!";
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader("./test_fifo1");
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

    writer_thread.join();

    REQUIRE(input.contains("hello"));
    REQUIRE(input["hello"] == "!");
  }

  SECTION("Multiple JSON messages") {

    std::thread writer_thread([]() {
      FIFOWriter writer("./test_fifo2");
      for (int i = 0; i < 5; ++i) {
        json output;
        output["message_id"] = i;
        output["data"] = "test_data_" + std::to_string(i);
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader("./test_fifo2");

    for (int i = 0; i < 5; ++i) {
      json input;
      REQUIRE_NOTHROW(input = reader.read_json());
      REQUIRE(input["message_id"] == i);
      REQUIRE(input["data"] == "test_data_" + std::to_string(i));
    }

    writer_thread.join();
  }
}

TEST_CASE("FIFO Large Messages") {
  SECTION("Large JSON payload") {

    std::thread writer_thread([]() {
      FIFOWriter writer("./test_fifo_large");
      json output;
      output["large_array"] = json::array();

      for (int i = 0; i < 10000; ++i) {
        output["large_array"].push_back(
            {{"index", i}, {"value", "data_" + std::to_string(i)}});
      }
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader("./test_fifo_large");
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

    writer_thread.join();

    REQUIRE(input["large_array"].size() == 10000);
    REQUIRE(input["large_array"][9999]["value"] == "data_9999");
  }
}

TEST_CASE("FIFO Different Data Types") {
  SECTION("All JSON types") {
    const std::string fifo_path = "./test_fifo_types";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter w(fifo_path);
      json output;
      output["string"] = "text";
      output["integer"] = 42;
      output["float"] = 3.14;
      output["boolean_true"] = true;
      output["boolean_false"] = false;
      output["null"] = nullptr;
      output["array"] = json::array({1, 2, 3, "four"});
      output["nested"] = {{"key", "value"}};
      w.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader r(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = r.read_json());

    writer_thread.join();

    REQUIRE(input["string"] == "text");
    REQUIRE(input["integer"] == 42);
    REQUIRE(input["float"] == 3.14);
    REQUIRE(input["boolean_true"] == true);
    REQUIRE(input["boolean_false"] == false);
    REQUIRE(input["null"] == nullptr);
    REQUIRE(input["array"] == json::array({1, 2, 3, "four"}));
    REQUIRE(input["nested"]["key"] == "value");
  }
}