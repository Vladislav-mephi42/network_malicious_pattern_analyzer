#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <tins/tins.h>
#include <vector>

using namespace Tins;

int main(int argc, char *argv[]) {
  std::string iface = "lo";
  std::string dst_ip = "127.0.0.1";
  uint16_t dst_port = 7009;
  int packet_count = 6;

  if (argc > 1)
    iface = argv[1];
  if (argc > 2)
    dst_ip = argv[2];
  if (argc > 3)
    dst_port = std::stoi(argv[3]);
  if (argc > 4)
    packet_count = std::stoi(argv[4]);

  std::cout << "=== libtins Traffic Generator ===\n";
  std::cout << "Interface : " << iface << "\n";
  std::cout << "Target    : " << dst_ip << ":" << dst_port << "\n";
  std::cout << "Packets   : " << packet_count << "\n\n";

  PacketSender sender;

  TCP::Flags flag_sequence[] = {TCP::Flags(TCP::SYN),
                                TCP::Flags(TCP::SYN | TCP::ACK),
                                TCP::Flags(TCP::ACK | TCP::SYN),
                                TCP::Flags(TCP::PSH | TCP::ACK | TCP::SYN),
                                TCP::Flags(TCP::FIN | TCP::ACK | TCP::SYN),
                                TCP::Flags(TCP::RST | TCP::SYN),
                                TCP::Flags(TCP::URG | TCP::ACK | TCP::SYN)};
  int num_flags = sizeof(flag_sequence) / sizeof(flag_sequence[0]);

  for (int i = 0; i < packet_count; ++i) {
    IP ip_layer(dst_ip);
    TCP tcp_layer(dst_port);

    tcp_layer.sport(40000 + (i % 100));
    tcp_layer.flags(flag_sequence[i % num_flags]);
    tcp_layer.seq(1000 * (i + 1));
    tcp_layer.ack_seq(2000 * (i + 1));
    tcp_layer.window(65535);

    if (tcp_layer.flags() & TCP::URG) {
      tcp_layer.urg_ptr(100);
    }

    if (tcp_layer.flags() == (TCP::PSH | TCP::ACK)) {
      std::string payload_data = "Hello from libtins generator!";
      tcp_layer /= RawPDU(payload_data);
    }

    ip_layer /= tcp_layer;

    try {
      sender.send(ip_layer, iface);

      std::cout << "[+] Sent #" << (i + 1) << " | Flags: 0x" << std::hex
                << (int)tcp_layer.flags() << std::dec
                << " | Seq: " << tcp_layer.seq()
                << " | Size: " << tcp_layer.size() << " bytes\n";
    } catch (const std::exception &e) {
      std::cerr << "[-] Error sending packet: " << e.what() << "\n";
      std::cerr << "    (Did you run with sudo?)\n";
      return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }

  std::cout << "\n=== Generation Complete ===\n";
  return 0;
}