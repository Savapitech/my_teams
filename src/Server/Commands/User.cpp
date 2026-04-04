#include "User.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

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

  for (const auto &user : users) {
    if (std::string(user.uuid) == targetUuid) {
      client->sendMessage("200 User info: Name: " + std::string(user.name) +
                          ", UUID: " + std::string(user.uuid) + "\n");
      LOG_DEBUG("User info requested for UUID [" + targetUuid + "]");
      return;
    }
  }

  client->sendMessage("404 Not Found: User not found\n");
}
} // namespace commands
