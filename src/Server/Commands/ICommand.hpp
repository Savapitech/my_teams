#pragma once

#include <vector>

#include "Server/Client.hpp"

namespace commands {

class ICommand {
public:
  virtual ~ICommand() = default;
  virtual void execute(std::shared_ptr<Client> client,
                       std::vector<std::string> &args) = 0;
};

} // namespace commands
