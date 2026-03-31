#include <csignal>
#include <iostream>
#include <string>

#include <termios.h>

#include "Server.hpp"
#include "Utils/Logger.hpp"

Server *g_server = nullptr;

static void handleSignal(int signum) {
  if (g_server) {
    LOG_INFO("Signal received, shutting down server gracefully...");
    g_server->stop();
  }
}

static void printHelp() {
  std::cout
      << "USAGE: ./myteams_server port\n\n"
      << "\tport is the port number on which the server socket listens.\n";
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
    return LOG_ERROR("Invalid port number."), 84;
  }

  try {
    Server server(port);
    g_server = &server;

    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    server.run();
    g_server = nullptr;
  } catch (const std::exception &e) {
    return LOG_FATAL("Server fatal error: " + std::string(e.what())), 84;
  }

  return 0;
}
