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

public:
  enum class ContextType { NONE, TEAM, CHANNEL, THREAD };

private:
  ContextType _contextType = ContextType::NONE;
  std::string _teamUuid;
  std::string _channelUuid;
  std::string _threadUuid;

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

  ContextType getContextType() const;
  std::string getTeamUuid() const;
  std::string getChannelUuid() const;
  std::string getThreadUuid() const;
  void setTeamUuid(const std::string &uuid);
  void setChannelUuid(const std::string &uuid);
  void setThreadUuid(const std::string &uuid);
  void setContextType(ContextType type);

  void clearCtx();

private:
  void processCommand(const std::string &commandLine);
};
