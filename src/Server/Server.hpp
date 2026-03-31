#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <sys/poll.h>

#include "Client.hpp"
#include "Socket.hpp"

class Server {
private:
  Socket _socket;
  std::vector<pollfd> _fds;
  std::vector<std::shared_ptr<Client>> _clients;
  bool _isRunning = true;

public:
  Server(uint16_t port);
  void run();
  void handleNewConnection();
  void handleClientMessage(int clientFd);
  void disconnectClient(int fd);
};
