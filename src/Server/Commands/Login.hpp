#pragma once

#include "ACommand.hpp"

namespace commands {

class Login : public ACommand {
public:
  void execute(std::shared_ptr<Client> client,
               std::vector<std::string> &args) override;
};

} // namespace commands
