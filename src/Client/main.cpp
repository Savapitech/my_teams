#include <exception>
#include <iostream>

#include "Client.hpp"

int main(int ac, char **av) {
  if (ac != 3)
    std::cout << "USAGE: ./myteams_cli ip port" << std::endl;

  try {
    Client client(av[1], av[2]);

    client.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}