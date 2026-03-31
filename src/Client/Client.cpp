#include <iostream>

#include <exception>
#include <arpa/inet.h>


#include "Client.hpp"

Client::Client(const std::string &ip, const std::string &port) :
    _port(port),
    _ip(ip)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
        throw std::runtime_error("socket failed");

    sockaddr_in server = {};

    server.sin_family = AF_INET;
    server.sin_port = htons(std::stoi(_port));

    if (inet_pton(AF_INET, ip.c_str(), &server.sin_addr) <= 0)
        throw std::runtime_error("invalid ip");

    if (connect(_fd, (sockaddr*)&server, sizeof(server)) < 0)
        throw std::runtime_error("connect failed: IP[" + _ip + "], PORT[" + _port + "]");
}

void Client::run()
{
    std::string line;

    while (std::getline(std::cin, line))
    {
        std::cout << "line: " << line << std::endl;

    }
}