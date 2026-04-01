#include <csignal>
#include <iostream>
#include <string>

#include <termios.h>
#include <unistd.h>

#include "Server.hpp"
#include "Utils/Logger.hpp"

static Server *g_server = nullptr;
static struct termios saved_term_settings;

static void initTerm() {
  if (isatty(STDIN_FILENO))
    tcgetattr(STDIN_FILENO, &saved_term_settings);

  struct termios new_settings = saved_term_settings;

  if (isatty(STDIN_FILENO)) {
    new_settings.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
  }
}

static void restoreTerm() {
  if (isatty(STDIN_FILENO))
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_term_settings);
}

static void handleSignal(int signum) {
  if (g_server != nullptr) {
    LOG_DEBUG("Signal received, shutting down server gracefully...");
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
    initTerm();

    server.run();
    g_server = nullptr;
  } catch (const std::exception &e) {
    restoreTerm();
    handleSignal(0);
    return LOG_FATAL("Server fatal error: " + std::string(e.what())), 84;
  }

  restoreTerm();
  return 0;
}
