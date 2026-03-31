#include <iostream>
#include <string>

#include "Server.hpp"
#include "Utils/Logger.hpp"

static void printHelp() {
  std::cout << "USAGE: ./myteams_server port\n\n";
  std::cout << "\tport is the port number on which the server socket listens.\n";
}

int main(int argc, char **argv) {
  if (argc != 2)
    return printHelp(), 84;

  uint16_t port;
  try {
    int parsedPort = std::stoi(argv[1]);
    if (parsedPort <= 0 || parsedPort > 65535)
      throw std::out_of_range("Port out of range");
    port = static_cast<uint16_t>(parsedPort);
  } catch (const std::exception &) {
    LOG_ERROR("Invalid port number.");
    return 84;
  }

  try {
    Server server(port);
    server.run();
  } catch (const std::exception &e) {
    return LOG_FATAL("Server fatal error: " + std::string(e.what())), 84;
  }

  return 0;
}
