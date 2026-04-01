#include "Login.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Login::execute(std::shared_ptr<Client> client,
                    std::vector<std::string> &args) {
  LOG_DEBUG("Login command executed");
}
} // namespace commands
