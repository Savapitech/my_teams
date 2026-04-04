#pragma once

#include "ICommand.hpp"
#include <memory>
#include <string>
#include <vector>

class Client;

namespace commands {
class Subscribed : public ICommand {
public:
  void execute(std::shared_ptr<Client> client,
               std::vector<std::string> &args) override;
};
} // namespace commands
