#include <iostream>

#include <arpa/inet.h>
#include <exception>
#include <poll.h>

#include <memory>


#include "Client.hpp"
#include "ICommand.hpp"
#include "unistd.h"

Client::Client(const std::string &ip, const std::string &port)
    : _port(port), _ip(ip) {
  _fd = socket(AF_INET, SOCK_STREAM, 0);
  if (_fd < 0)
    throw std::runtime_error("socket failed");

  sockaddr_in server = {};

  server.sin_family = AF_INET;
  server.sin_port = htons(std::stoi(_port));

  if (inet_pton(AF_INET, ip.c_str(), &server.sin_addr) <= 0)
    throw std::runtime_error("invalid ip");

  if (connect(_fd, (sockaddr *)&server, sizeof(server)) < 0)
    throw std::runtime_error("connect failed: IP[" + _ip + "], PORT[" + _port +
                             "]");
  fds[0].fd = 0;
  fds[0].events = POLLIN;

  fds[1].fd = _fd;
  fds[1].events = POLLIN;

  _commandMap["HELP"] = std::make_unique<HelpCommand>();
  _commandMap["LOGIN"] = std::make_unique<LoginCommand>();
  _commandMap["LOGOUT"] = std::make_unique<LogoutCommand>();
  _commandMap["USERS"] = std::make_unique<UsersCommand>();
  _commandMap["USER"] = std::make_unique<UserCommand>();
  _commandMap["SEND"] = std::make_unique<SendCommand>();
  _commandMap["MESSAGES"] = std::make_unique<MessagesCommand>();
  _commandMap["SUBSCRIBE"] = std::make_unique<SubscribeCommand>();
  _commandMap["SUBSCRIBED"] = std::make_unique<SubscribedCommand>();
  _commandMap["UNSUBSCRIBE"] = std::make_unique<UnsubscribeCommand>();
  _commandMap["USE"] = std::make_unique<UseCommand>();
  _commandMap["CREATE"] = std::make_unique<CreateCommand>();
  _commandMap["LISR"] = std::make_unique<ListCommand>();
  _commandMap["INFO"] = std::make_unique<InfoCommand>();
  
}

void Client::handleCommand(const std::string &buffer, int fd)
{

}

void Client::run() {
  CommandTest cmdtest;
  std::string line;

  while (true) {
    int ret = poll(fds, 2, -1);

    if (fds[0].revents & POLLIN) {
        char buffer[1024];
        read(0, buffer, 1024);
        std::cout << buffer;
        cmdtest.sendCommand(buffer, this->_fd);
    }
    if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
      throw std::runtime_error("Server disconect");
    }
    if (fds[1].revents & POLLIN) {
        int r_value = 0;
        char buffer[1024];
        r_value = read(this->_fd, buffer, 1024);
        if (r_value == 0) 
          throw std::runtime_error("Server disconnected");
        std::cout << buffer;
        this->handleCommand(buffer, this->_fd);
    }
  }
}

/*
// map [std::string command] -> [std::function<void (std::vector<std::string>)>]
// 
//
//
*/