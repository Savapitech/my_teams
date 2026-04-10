#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>

#include "ICommand.hpp"

void ACommand::sendCommand(const std::string &command, int fd,
                           std::queue<std::string> &_pendingCommands) {
  std::string cmd = command;
  cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
  cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());

  if (cmd.empty())
    return;
  size_t firstSpace = cmd.find(' ');
  std::string cmdName = cmd.substr(0, firstSpace);
  std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);
  std::string commandToSend = cmdName.substr(1);

  if (firstSpace != std::string::npos) {
    std::string argsPart = cmd.substr(firstSpace + 1);
    std::vector<std::string> args;
    std::string currentArg = "";
    bool inQuotes = false;

    for (char c : argsPart) {
      if (c == '"') {
        inQuotes = !inQuotes;
      } else if (c == ' ' && !inQuotes) {
        if (!currentArg.empty()) {
          args.push_back(currentArg);
          currentArg = "";
        }
      } else {
        currentArg += c;
      }
    }
    if (!currentArg.empty()) {
      args.push_back(currentArg);
    }

    for (const auto &arg : args) {
      commandToSend += " \"" + arg + "\"";
    }
  }
  commandToSend += "\n";
  _pendingCommands.push(cmdName.substr(1));
  write(fd, commandToSend.c_str(), commandToSend.size());
}