#pragma once

#include <queue>
#include <string>

class ICommand {
public:
  virtual ~ICommand() = default;
  virtual void sendCommand(const std::string &command, int fd,
                           std::queue<std::string> &_pendingCommands) = 0;
  virtual void logCommand(const std::string &severResponse) = 0;

protected:
  ICommand() = default;
};

class ACommand : public ICommand {
public:
  void sendCommand(const std::string &command, int fd,
                   std::queue<std::string> &_pendingCommands) override;

protected:
  ACommand() = default;
};

class CommandTest : public ACommand {
public:
  CommandTest() = default;
  virtual void logCommand(const std::string &severResponse) override {}
};

class HelpCommand : public ACommand {
public:
  HelpCommand() = default;
  ~HelpCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class LoginCommand : public ACommand {
public:
  LoginCommand() = default;
  ~LoginCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class LogoutCommand : public ACommand {
public:
  LogoutCommand() = default;
  ~LogoutCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class UsersCommand : public ACommand {
public:
  UsersCommand() = default;
  ~UsersCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class UserCommand : public ACommand {
public:
  UserCommand() = default;
  ~UserCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class SendCommand : public ACommand {
public:
  SendCommand() = default;
  ~SendCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class MessagesCommand : public ACommand {
public:
  MessagesCommand() = default;
  ~MessagesCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class SubscribeCommand : public ACommand {
public:
  SubscribeCommand() = default;
  ~SubscribeCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class SubscribedCommand : public ACommand {
public:
  SubscribedCommand() = default;
  ~SubscribedCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class UnsubscribeCommand : public ACommand {
public:
  UnsubscribeCommand() = default;
  ~UnsubscribeCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class UseCommand : public ACommand {
public:
  UseCommand() = default;
  ~UseCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class CreateCommand : public ACommand {
public:
  CreateCommand() = default;
  ~CreateCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class ListCommand : public ACommand {
public:
  ListCommand() = default;
  ~ListCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

class InfoCommand : public ACommand {
public:
  InfoCommand() = default;
  ~InfoCommand() = default;
  void logCommand(const std::string &serverResponse) override;
};

void handleEvent(const std::string &buffer);