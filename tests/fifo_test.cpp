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

  SECTION("Empty json transmission") {
    const std::string fifo_path = "./test_fifo_empty";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output = json::object();
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

    writer_thread.join();

    REQUIRE(input["a"]["b"]["c"] == 42);
    REQUIRE(input["arr"][2][1] == 4);
  }

  SECTION("Two writers sequential") {
    const std::string fifo_path = "./test_fifo_two_writers";

    std::thread writer_thread([&fifo_path]() {
      {
        FIFOWriter writer(fifo_path);
        json output;
        output["id"] = 1;
        writer.write_json(output);
      }
      {
        FIFOWriter writer(fifo_path, false);
        json output;
        output["id"] = 2;
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input1 = reader.read_json();
    json input2 = reader.read_json();

    writer_thread.join();

    REQUIRE(input1["id"] == 1);
    REQUIRE(input2["id"] == 2);
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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = reader.read_json());

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

  SECTION("Numeric edge cases") {
    const std::string fifo_path = "./test_fifo_numeric";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter w(fifo_path);
      json output;
      output["zero"] = 0;
      output["negative"] = -42;
      output["big"] = 9223372036854775807LL;
      output["small"] = -9223372036854775807LL;
      output["double"] = 1.7976931348623157e308;
      output["tiny"] = 2.2250738585072014e-308;
      w.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader r(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = r.read_json());

    writer_thread.join();

    REQUIRE(input["zero"] == 0);
    REQUIRE(input["negative"] == -42);
    REQUIRE(input["big"] == 9223372036854775807LL);
    REQUIRE(input["double"] == 1.7976931348623157e308);
  }

  SECTION("Deeply nested json") {
    const std::string fifo_path = "./test_fifo_deep";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter w(fifo_path);
      json output;
      json *cur = &output;
      for (int i = 0; i < 50; ++i) {
        (*cur)["level"] = json::object();
        cur = &(*cur)["level"];
      }
      (*cur)["value"] = "deep";
      w.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader r(fifo_path);
    json input;
    REQUIRE_NOTHROW(input = r.read_json());

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
    const std::string fifo_path = "./test_fifo_err_writer_twice";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      REQUIRE_THROWS(FIFOWriter(fifo_path));
      json output;
      output["ok"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input = reader.read_json();
    REQUIRE(input["ok"] == true);

    writer_thread.join();
  }

  SECTION("Reader create twice throws") {
    const std::string fifo_path = "./test_fifo_err_reader_twice";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      json output;
      output["ok"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    REQUIRE_THROWS(FIFOReader(fifo_path));

    json input = reader.read_json();
    REQUIRE(input["ok"] == true);

    writer_thread.join();
  }

  SECTION("Reader on non existing fifo throws") {
    REQUIRE_THROWS(FIFOReader("./test_fifo_no_such_file"));
  }

  SECTION("Writer on non existing fifo with create=false throws") {
    REQUIRE_THROWS(FIFOWriter("./test_fifo_no_such_file_w", false));
  }

  SECTION("Empty message throws on read") {
    const std::string fifo_path = "./test_fifo_empty_msg";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      uint32_t len = 0;
      writer.write_n(&len, sizeof(uint32_t));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    REQUIRE_THROWS(reader.read_json());

    writer_thread.join();
  }
}

TEST_CASE("FIFO Move semantics") {
  SECTION("Writer move constructor") {
    const std::string fifo_path = "./test_fifo_move_writer";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      FIFOWriter moved(std::move(writer));
      json output;
      output["moved"] = true;
      moved.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input = reader.read_json();
    REQUIRE(input["moved"] == true);

    writer_thread.join();
  }

  SECTION("Writer move assignment") {
    const std::string fifo_path = "./test_fifo_move_assign_writer";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      FIFOWriter other(fifo_path, false);
      other = std::move(writer);
      json output;
      output["assigned"] = true;
      other.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    json input = reader.read_json();
    REQUIRE(input["assigned"] == true);

    writer_thread.join();
  }

  SECTION("Reader move constructor") {
    const std::string fifo_path = "./test_fifo_move_reader";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["reader_moved"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    FIFOReader moved(std::move(reader));
    json input = moved.read_json();
    REQUIRE(input["reader_moved"] == true);

    writer_thread.join();
  }

  SECTION("Reader move assignment") {
    const std::string fifo_path = "./test_fifo_move_assign_reader";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      json output;
      output["reader_assigned"] = true;
      writer.write_json(output);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    FIFOReader other(fifo_path, false);
    other = std::move(reader);
    json input = other.read_json();
    REQUIRE(input["reader_assigned"] == true);

    writer_thread.join();
  }
}

TEST_CASE("FIFO Concurrent") {
  SECTION("Multiple writers sequential order") {
    const std::string fifo_path = "./test_fifo_concurrent_writers";

    std::thread writer_thread([&fifo_path]() {
      for (int i = 0; i < 3; ++i) {
        FIFOWriter writer(fifo_path, i == 0);
        json output;
        output["writer"] = i;
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FIFOReader reader(fifo_path);
    std::set<int> seen;
    for (int i = 0; i < 3; ++i) {
      json input = reader.read_json();
      seen.insert(input["writer"].get<int>());
    }
    REQUIRE(seen.size() == 3);
    REQUIRE(seen.count(0) == 1);
    REQUIRE(seen.count(1) == 1);
    REQUIRE(seen.count(2) == 1);

    writer_thread.join();
  }

  SECTION("Multiple readers same fifo") {
    const std::string fifo_path = "./test_fifo_multi_readers";

    std::thread writer_thread([&fifo_path]() {
      FIFOWriter writer(fifo_path);
      for (int i = 0; i < 5; ++i) {
        json output;
        output["i"] = i;
        writer.write_json(output);
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::atomic<int> counter(0);
    auto worker = [&counter, &fifo_path]() {
      FIFOReader reader(fifo_path, false);
      json input = reader.read_json();
      counter.fetch_add(1);
    };

    std::vector<std::thread> th;
    for (int i = 0; i < 5; ++i) {
      th.push_back(std::thread(worker));
    }
    for (auto &elem : th) {
      elem.join();
    }

    REQUIRE(counter.load() == 5);

    writer_thread.join();
  }
}