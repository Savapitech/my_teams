#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include "Socket.hpp"
#include "Utils/Logger.hpp"

Socket::Socket(uint16_t port) {
  this->_port = port;
  this->_addr = (sockaddr_in){.sin_family = AF_INET,
                              .sin_port = htons(port),
                              .sin_addr = {.s_addr = INADDR_ANY},
                              .sin_zero = {0}};
  this->_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (this->_fd < 0)
    throw std::runtime_error("Bad socket return");

  int opt = 1;
  if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    throw std::runtime_error("Setsockopt failed");
}

Socket::~Socket() {
  try {
    this->close();
  } catch (const std::exception &e) {
    LOG_ERROR(e.what());
  }
}

int Socket::getFd() { return this->_fd; }

uint16_t Socket::getPort() { return this->_port; }

void Socket::listen() {
  if (bind(this->_fd, reinterpret_cast<sockaddr *>(&this->_addr),
           sizeof(this->_addr)) < 0)
    throw std::runtime_error("Cannot bind socket to port");
  if (::listen(this->_fd, 10) < 0)
    throw std::runtime_error("Cannot listen on socket");
}

void Socket::close() {
  if (this->_fd < 0)
    return;

  if (::close(this->_fd) < 0)
    throw std::runtime_error("Socket close failed");
  this->_fd = -1;
}
