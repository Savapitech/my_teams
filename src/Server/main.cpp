#include <iostream>
#include <string>

#include "Utils/Logger.hpp"

static void printHelp() {
  std::cout << "USAGE: ./myteams_server port\n\n";
  std::cout << "\tport is the port number on which the server socket listens.\n";
}

int main(int argc, char **argv) {
  if (argc != 2) {
    if (argc >= 2 && std::string(argv[1]) == "-h")
      printHelp();
    else
      LOG_ERROR("Invalid number of arguments. Try -help for more information.\n");
    return 84;
  }

  return 0;
}
