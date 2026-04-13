#include "User.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

#include <iostream>

namespace commands {
void UserCommand::execute(std::shared_ptr<Client> client,
                          std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  if (args.size() != 1) {
    client->sendMessage("400 Bad request: /user <user_uuid>\n");
    return;
  }

  std::string const &targetUuid = args[0];
  auto const &users = client->getServer().get().getDatabase().getUsers();
  auto const &activeClients = client->getServer().get().getClients();

  for (const auto &user : users) {
    if (std::string(user.uuid) == targetUuid) {
      bool isConnected = false;

      for (const auto &c : activeClients) {
        if (c->isLoggedIn() &&
            std::string(c->getActualUser().uuid) == targetUuid) {
          isConnected = true;
          break;
        }
      }

      client->sendMessage("200 OK \"" + std::string(user.uuid) + "\" \"" +
                          std::string(user.name) + "\" \"" +
                          (isConnected ? "1" : "0") + "\"\n");

      LOG_DEBUG("User info requested for UUID [" + targetUuid +
                "] - Status: " + std::to_string(isConnected));
      return;
    }
  }
  client->sendMessage("404 \"" + args[0] + "\"\n");
}
} // namespace commands
