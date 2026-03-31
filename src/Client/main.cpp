#include <exception>
#include <iostream>

#include "Client.hpp"

int main(int ac, char **av)
{
    Client client;

    try
    {
        client.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}