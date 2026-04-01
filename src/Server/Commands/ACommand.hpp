#pragma once

#include "ICommand.hpp"

namespace commands {

class ACommand : public ICommand {
public:
  virtual ~ACommand() = default;
};

} // namespace commands
