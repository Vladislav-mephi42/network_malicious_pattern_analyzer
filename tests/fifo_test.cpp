#define CATCH_CONFIG_MAIN
#include "fifo.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("FIFO Basic Operations") {
  SECTION("JSON transmission with RAII") {
    const std::string fifo_path = "./test_fifo_basic";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["hello"] = "!";
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input.contains("hello"));
    REQUIRE(input["hello"] == "!");
  }

  SECTION("Multiple JSON messages") {
    const std::string fifo_path = "./test_fifo_multi_msg";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      for (int i = 0; i < 5; ++i) {
        json output;
        output["message_id"] = i;
        output["data"] = "test_data_" + std::to_string(i);
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);

    for (int i = 0; i < 5; ++i) {
      json input = reader.read_json();
      REQUIRE(input["message_id"] == i);
      REQUIRE(input["data"] == "test_data_" + std::to_string(i));
    }

    writer_thread.join();
  }

  SECTION("Empty json transmission") {
    const std::string fifo_path = "./test_fifo_empty";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output = json::object();
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input == json::object());
  }

  SECTION("Array json transmission") {
    const std::string fifo_path = "./test_fifo_array";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output = json::array({1, 2, 3, "four", 5.0});
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input == json::array({1, 2, 3, "four", 5.0}));
  }

  SECTION("Nested json transmission") {
    const std::string fifo_path = "./test_fifo_nested";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["a"]["b"]["c"] = 42;
      output["arr"] = {1, 2, {3, 4}};
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["a"]["b"]["c"] == 42);
    REQUIRE(input["arr"][2][1] == 4);
  }
}

TEST_CASE("FIFO Large Messages") {
  SECTION("Large JSON payload") {
    const std::string fifo_path = "./test_fifo_large";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["large_array"] = json::array();
      for (int i = 0; i < 10000; ++i) {
        output["large_array"].push_back(
            {{"index", i}, {"value", "data_" + std::to_string(i)}});
      }
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["large_array"].size() == 10000);
    REQUIRE(input["large_array"][9999]["value"] == "data_9999");
  }

  SECTION("Many small messages") {
    const std::string fifo_path = "./test_fifo_many_small";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      for (int i = 0; i < 100; ++i) {
        json output;
        output["i"] = i;
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    for (int i = 0; i < 100; ++i) {
      json input = reader.read_json();
      REQUIRE(input["i"] == i);
    }

    writer_thread.join();
  }

  SECTION("Large string payload") {
    const std::string fifo_path = "./test_fifo_large_string";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["data"] = std::string(100000, 'X');
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["data"].get<std::string>().size() == 100000);
    REQUIRE(input["data"].get<std::string>().front() == 'X');
    REQUIRE(input["data"].get<std::string>().back() == 'X');
  }
}

TEST_CASE("FIFO Different Data Types") {
  SECTION("All JSON types") {
    const std::string fifo_path = "./test_fifo_types";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["string"] = "text";
      output["integer"] = 42;
      output["float"] = 3.14;
      output["boolean_true"] = true;
      output["boolean_false"] = false;
      output["null"] = nullptr;
      output["array"] = json::array({1, 2, 3, "four"});
      output["nested"] = {{"key", "value"}};
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

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

  SECTION("Numeric edge cases") {
    const std::string fifo_path = "./test_fifo_numeric";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["zero"] = 0;
      output["negative"] = -42;
      output["big"] = 9223372036854775807LL;
      output["small"] = -9223372036854775807LL;
      output["double"] = 1.7976931348623157e308;
      output["tiny"] = 2.2250738585072014e-308;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["zero"] == 0);
    REQUIRE(input["negative"] == -42);
    REQUIRE(input["big"] == 9223372036854775807LL);
    REQUIRE(input["double"] == 1.7976931348623157e308);
  }

  SECTION("Deeply nested json") {
    const std::string fifo_path = "./test_fifo_deep";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      json *cur = &output;
      for (int i = 0; i < 50; ++i) {
        (*cur)["level"] = json::object();
        cur = &(*cur)["level"];
      }
      (*cur)["value"] = "deep";
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    json *cur = &input;
    for (int i = 0; i < 50; ++i) {
      cur = &(*cur)["level"];
    }
    REQUIRE((*cur)["value"] == "deep");
  }
}

TEST_CASE("FIFO Errors") {
  SECTION("Writer create twice throws") {
    const std::string fifo_path = "./test_fifo_err_twice";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      REQUIRE_THROWS_AS(FIFOWriter(fifo_path), std::runtime_error);
      json output;
      output["ok"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["ok"] == true);
  }

  SECTION("Reader on non existing fifo throws") {
    REQUIRE_THROWS_AS(FIFOReader("./test_fifo_no_such_file"),
                      std::runtime_error);
  }

  SECTION("Writer on non existing fifo with create=false throws") {
    REQUIRE_THROWS_AS(FIFOWriter("./test_fifo_no_such_file_w", false),
                      std::runtime_error);
  }

  SECTION("Empty message throws on read") {
    const std::string fifo_path = "./test_fifo_empty_msg";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      uint32_t len = 0;
      writer.write_n(&len, sizeof(uint32_t));
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    REQUIRE_THROWS(reader.read_json());

    writer_thread.join();
  }

  SECTION("Too large message throws on read") {
    const std::string fifo_path = "./test_fifo_too_large";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      uint32_t len = max_buf + 1;
      writer.write_n(&len, sizeof(uint32_t));
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    REQUIRE_THROWS(reader.read_json());

    writer_thread.join();
  }

  SECTION("Invalid json throws on read") {
    const std::string fifo_path = "./test_fifo_invalid_json";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      std::string bad = "{not valid json";
      writer.write_all(bad.c_str(), bad.size());
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    REQUIRE_THROWS(reader.read_json());

    writer_thread.join();
  }
}

TEST_CASE("FIFO Move semantics") {
  SECTION("Writer move constructor") {
    const std::string fifo_path = "./test_fifo_move_w_ctor";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      FIFOWriter moved(std::move(writer));
      json output;
      output["moved"] = true;
      moved.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    json input = reader.read_json();

    writer_thread.join();

    REQUIRE(input["moved"] == true);
  }

  SECTION("Writer move assignment") {
    const std::string fifo_path = "./test_fifo_move_w_assign";

    std::thread reader_thread([&fifo_path]() {
      FIFOReader reader(fifo_path, true);
      json input = reader.read_json();
      REQUIRE(input["assigned"] == true);
    });

    std::this_thread::sleep_for(100ms);

    FIFOWriter w1(fifo_path, false);
    FIFOWriter w2(fifo_path, false);
    w2 = std::move(w1);
    json output;
    output["assigned"] = true;
    w2.write_json(output);

    reader_thread.join();
  }

  SECTION("Reader move constructor") {
    const std::string fifo_path = "./test_fifo_move_r_ctor";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["reader_moved"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader reader(fifo_path);
    FIFOReader moved(std::move(reader));
    json input = moved.read_json();

    writer_thread.join();

    REQUIRE(input["reader_moved"] == true);
  }

  SECTION("Reader move assignment") {
    const std::string fifo_path = "./test_fifo_move_r_assign";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["reader_assigned"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(100ms);

    FIFOReader r1(fifo_path);
    FIFOReader r2(fifo_path, false);
    r2 = std::move(r1);
    json input = r2.read_json();

    writer_thread.join();

    REQUIRE(input["reader_assigned"] == true);
  }
}

TEST_CASE("FIFO RAII cleanup") {
  SECTION("Writer deletes fifo on destruction") {
    const std::string fifo_path = "./test_fifo_raii_del";

    {
      std::thread writer_thread([&fifo_path]() {
        FIFOWriter writer(fifo_path);
        json output;
        output["raii"] = true;
        writer.write_json(output);
      });

      std::this_thread::sleep_for(100ms);

      FIFOReader reader(fifo_path);
      json input = reader.read_json();
      REQUIRE(input["raii"] == true);

      writer_thread.join();
    }

    REQUIRE_FALSE(fifo_exists(fifo_path));
  }

  SECTION("Writer with create=false does not delete fifo") {
    const std::string fifo_path = "./test_fifo_raii_nodel";

    mkfifo(fifo_path.c_str(), 0644);

    {
      std::thread reader_thread(
          [&fifo_path]() { FIFOReader reader(fifo_path, false); });

      std::this_thread::sleep_for(100ms);

      FIFOWriter writer(fifo_path, false);

      reader_thread.join();
    }

    REQUIRE(fifo_exists(fifo_path));
    unlink(fifo_path.c_str());
  }
}