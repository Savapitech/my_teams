#pragma once

#include <functional>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <string>

#include "Commands/ICommand.hpp"
#include "Models.hpp"

class Server;

class Client : public std::enable_shared_from_this<Client> {
private:
  int _fd;
  sockaddr_in _addr;
  bool _isConnected = true;
  bool _isLoggedIn = false;
  std::string _buffer;
  std::reference_wrapper<Server> _server;
  std::map<std::string, std::shared_ptr<commands::ICommand>> _commands;
  User _actualUser;

  void registerCommands();

public:
  Client(int fd, sockaddr_in addr, std::reference_wrapper<Server> server);
  ~Client();

  void close();
  int getFd() const;
  sockaddr_in getAddr() const;

  void handleMessage();
  void sendMessage(const std::string &msg);
  bool isConnected() const;
  void disconnect();
  void setLogged(bool log);
  bool isLoggedIn();
  std::reference_wrapper<Server> getServer();
  User getActualUser();
  void setActualUser(User user);

private:
  void processCommand(const std::string &commandLine);
};
