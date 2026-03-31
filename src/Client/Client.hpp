#pragma once

#include <string>

class Client
{
private:
    int _fd;
    
    const std::string &_port;
    const std::string &_ip;
public:
    Client( const std::string &ip, const std::string &port);
    ~Client() = default;
    void run();
};
