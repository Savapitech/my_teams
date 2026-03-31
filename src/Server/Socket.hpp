#pragma once

#include <netinet/in.h>

class Socket {
private:
  int _fd;
  sockaddr_in _addr;
  uint16_t _port;

public:
  Socket(uint16_t port);
  ~Socket();
  int getFd();
  uint16_t getPort();
  void close();
  void listen();
};
