#define CATCH_CONFIG_MAIN
#include "sockets.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <thread>

TEST_CASE("Data transmission") {
  SECTION("send_n/recv_n") {
    auto echo = []() {
      char buf[4096];
      Server_socket server;
      server.bind(7000);
      server.listen(1);
      auto client = server.accept();
      client.recv_n(buf, sizeof(buf));
      client.send_n(buf, sizeof(buf));
    };
    std::thread echo_thread(echo);
    sleep(1);
    Client_socket client;
    client.connect("127.0.0.1", 7000);
    char buf_out[4096];
    char buf_in[4096];
    strncpy(buf_out, "Hello, I'm Vlad. How echo works?", sizeof(buf_out) - 1);
    REQUIRE_NOTHROW(client.send_n(buf_out, sizeof(buf_out)));
    REQUIRE_NOTHROW(client.recv_n(buf_in, sizeof(buf_in)));
    std::string out(buf_out);
    std::string in(buf_in);
    REQUIRE(out == in);
    echo_thread.join();
  }
  SECTION("auto_send/auto_recv") {
    auto echo = []() {
      char buf[4096];
      Server_socket server;
      server.bind(7001);
      server.listen(1);
      auto client = server.accept();
      while (true) {
        int len = client.auto_recv(buf, 4096);
        if (buf[0] == 'e' && buf[1] == 'n' && buf[2] == 'd') {
          break;
        }
        client.auto_send(buf, len);
      }
    };
    std::thread echo_thread(echo);
    sleep(1);
    Client_socket client;
    client.connect("127.0.0.1", 7001);
    char buf_out[4096];
    char buf_in[4096];
    strncpy(buf_out, "Hello, I'm Vlad. How echo works?", sizeof(buf_out) - 1);
    REQUIRE_NOTHROW(client.auto_send(buf_out, sizeof(buf_out)));
    REQUIRE_NOTHROW(client.auto_recv(buf_in, 4096));
    std::string out(buf_out);
    std::string in(buf_in);
    REQUIRE(out == in);

    memset(buf_in, 0, sizeof(buf_in));
    REQUIRE_NOTHROW(client.auto_send(buf_out, 5));
    REQUIRE_NOTHROW(client.auto_recv(buf_in, 4096));
    out = std::string(buf_out);
    in = std::string(buf_in);
    REQUIRE(out != in);
    strncpy(buf_out, "end", sizeof(buf_out) - 1);
    REQUIRE_NOTHROW(client.auto_send(buf_out, 4));
    echo_thread.join();
  }
  SECTION("send_json/recv_json") {
    auto echo = []() {
      json data;
      Server_socket server;
      server.bind(7002);
      server.listen(1);
      auto client = server.accept();
      while (true) {
        data = client.recv_json();
        if (data.contains("end")) {
          break;
        }
        client.send_json(data);
      }
    };
    std::thread echo_thread(echo);
    sleep(1);
    Client_socket client;
    client.connect("127.0.0.1", 7002);
    json buf_out;
    json buf_in;
    buf_out["data"] = "Hello, I'm Vlad. How echo works?";
    REQUIRE_NOTHROW(client.send_json(buf_out));
    REQUIRE_NOTHROW(buf_in = client.recv_json());

    REQUIRE(buf_out["data"] == buf_in["data"]);
    buf_out["end"] = 1;
    REQUIRE_NOTHROW(client.send_json(buf_out));

    echo_thread.join();
  }
}