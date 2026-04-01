#include <stdexcept>

#include "Login.hpp"
#include "Server/Client.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Login::execute(std::shared_ptr<Client> client,
                    std::vector<std::string> &args) {
  LOG_DEBUG("Login command executed");
  if (!args.size())
    throw std::runtime_error("400 Bad request, no args");

  client->setLogged(true);
  client->sendMessage("200 Ok\n");
}
} // namespace commands
