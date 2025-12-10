#include <iostream>
#include <memory>
#include <string>

#include "net/net.hpp"

int main(int argc, char *argv[]) {
  std::string host = "data-stream.binance.vision";
  int port = 9443;
  std::string target = "/ws/pepeusdt@trade";

  std::unique_ptr<WebSocket> ws = Net::init_websocket();
  int rc = ws->connect_to_server(host, port, target);
  if (rc < 0) {
    std::cout << "\nSomething went wrong!\n";
  } else {
    std::cout << "\nSuccessfully connected to: " << port << ":" << host
              << target << "\n";
  }

  while (true) {
    std::cout << "\nRead: " << ws->read_buffer();
  }

  return EXIT_SUCCESS;
}
