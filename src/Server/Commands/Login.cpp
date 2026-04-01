#include <stdexcept>

#include "Login.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Login::execute(std::shared_ptr<Client> client,
                    std::vector<std::string> &args) {
  if (!args.size() || args.size() > 1)
    throw std::runtime_error("400 Bad request, no args or too much args");

  auto users = client->getServer().get().getDatabase().getUsers();

  for (auto &user : users) {
    if (user.name == args[0]) {
      client->setLogged(true);
      client->setActualUser(user);
      LOG_DEBUG("User " + std::string(user.name) + " is logged in, uuid " +
                std::string(user.uuid));
      client->sendMessage("200 Logged in as " + std::string(user.name) +
                          " uuid " + std::string(user.uuid));
      return;
    }
  }

  client->sendMessage("404 User not found\n");
}
} // namespace commands
