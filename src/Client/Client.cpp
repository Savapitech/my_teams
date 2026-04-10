#include <iostream>

#include <arpa/inet.h>
#include <exception>
#include <poll.h>

#include <exception>
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
  _commandMap["LIST"] = std::make_unique<ListCommand>();
  _commandMap["INFO"] = std::make_unique<InfoCommand>();
}

void Client::handleCommand(const std::string &buffer, int fd) {
  if (buffer.find("100") == 0) {
    std::cout << "EVENT" << buffer << std::endl;
    return;
  }
  if (_pendingCommands.empty()) {
    std::cout << "Warning : no command in the queu." << std::endl;
    return;
  }

  std::string currentCmd = _pendingCommands.front();
  _pendingCommands.pop();
  currentCmd.erase(currentCmd.find_last_not_of("\r\n ") + 1);
  auto it = _commandMap.find(currentCmd);

  if (it != _commandMap.end()) {
    it->second->logCommand(buffer);
  } else {
    std::cout << "Error: Commande [" << currentCmd << "] not find."
              << std::endl;
  }
}

void Client::run() {
  CommandTest cmdtest;
  std::string line;

  while (true) {
    int ret = poll(fds, 2, -1);

    if (ret == -1)
      throw std::runtime_error("Error poll");

    if (fds[0].revents & POLLIN) {
      int retValue = 0;
      char buffer[1024];
      retValue = read(0, buffer, 1024);
      if (retValue == -1)
        throw std::runtime_error("Error");
      buffer[retValue] = 0;
      cmdtest.sendCommand(buffer, this->_fd, this->_pendingCommands);
    }
    if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
      throw std::runtime_error("Server disconect");
    }
    if (fds[1].revents & POLLIN) {
      int retValue = 0;
      char buffer[1024];
      retValue = read(this->_fd, buffer, 1024);
      if (retValue == 0)
        throw std::runtime_error("Server disconnected");
      buffer[retValue] = 0;
      this->handleCommand(buffer, this->_fd);
    }
  }
}
