#include <iostream>

#include "Client.hpp"

void Client::run()
{
    std::string line;

    while (std::getline(std::cin, line))
    {
        std::cout << "line: " << line << std::endl;
    }
}