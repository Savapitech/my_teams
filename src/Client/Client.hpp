#pragma once

#include <string>
#include <poll.h>
#include <map>
#include <memory>

#include "ICommand.hpp"


class Client {
private:
  int _fd;

  const std::string &_port;
  const std::string &_ip;
  std::string commandToSend;

  std::map<std::string, std::unique_ptr<ICommand>> _commandMap; 
  
  struct pollfd fds[2];

  void handleCommand(const std::string &buffer, int fd);

public:
  Client(const std::string &ip, const std::string &port);
  void sendMessage();
  ~Client() = default;
  void run();
};
