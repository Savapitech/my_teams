#include <format>

#include <arpa/inet.h>
#include <unistd.h>

#include "Client.hpp"
#include "Server/Parser.hpp"
#include "Utils/Logger.hpp"

Client::Client(int fd, sockaddr_in addr) : _fd(fd), _addr(addr) {}

Client::~Client() {
  try {
    this->close();
  } catch (const std::exception &e) {
    LOG_ERROR(e.what());
  }
}

void Client::close() {
  if (this->_fd < 0)
    return;

  if (::close(this->_fd) < 0)
    LOG_ERROR("Socket close failed");
  this->_fd = -1;
  this->_isConnected = false;
}

void Client::disconnect() { this->_isConnected = false; }

int Client::getFd() const { return this->_fd; }

sockaddr_in Client::getAddr() const { return this->_addr; }

bool Client::isConnected() const { return this->_isConnected; }

void Client::sendMessage(const std::string &msg) {
  if (this->_fd < 0)
    return;
  write(this->_fd, msg.c_str(), msg.length());
}

void Client::handleMessage() {
  char buffer[4096];
  ssize_t bytesRead = ::read(this->_fd, buffer, sizeof(buffer) - 1);

  if (bytesRead <= 0) {
    this->_isConnected = false;
    return;
  }

  buffer[bytesRead] = '\0';
  this->_buffer += buffer;

  size_t pos;
  while ((pos = this->_buffer.find("\n")) != std::string::npos) {
    std::string commandLine = this->_buffer.substr(0, pos);
    this->_buffer.erase(0, pos + 1);

    if (!commandLine.empty() && commandLine.back() == '\r')
      commandLine.pop_back();

    if (!commandLine.empty())
      this->processCommand(commandLine);
  }
}

void Client::processCommand(const std::string &commandLine) {
  LOG_DEBUG(std::format("Received from {} [{}]",
                        inet_ntoa(this->_addr.sin_addr), commandLine));

  std::string cmd = commandLine.substr(0, commandLine.find_first_of(" \t"));
  std::vector<std::string> args = ParseArgs(commandLine.substr(cmd.size()));

  LOG_DEBUG(
      std::format("Parsed command name [{}] args size [{}]", cmd, args.size()));
  this->sendMessage("200 Command received\n");
}
