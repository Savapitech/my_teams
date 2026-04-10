#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>

#include "ICommand.hpp"

void ACommand::sendCommand(const std::string &command, int fd,
                           std::queue<std::string> &_pendingCommands) {
  size_t firstSpace = command.find(' ');
  std::string cmdName = command.substr(0, firstSpace);

  std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);
  std::string commandToSend = cmdName.substr(1);

  if (firstSpace != std::string::npos) {
    std::string argsPart = command.substr(firstSpace);
    commandToSend += argsPart;
  }

  int quoteCount = std::count(commandToSend.begin(), commandToSend.end(), '"');
  if (quoteCount % 2 != 0) {
    std::cout << "Error : missing quote." << std::endl;
    return;
  }
  std::cout << "Send to the server: [" << commandToSend << "]" << std::endl;
  _pendingCommands.push(cmdName.c_str() + 1);
  write(fd, commandToSend.c_str(), commandToSend.size());
}