#include "Users.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

extern "C" {
#include "logging_server.h"
}

namespace commands {
void Users::execute(std::shared_ptr<Client> client,
                    std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  auto const &users = client->getServer().get().getDatabase().getUsers();
  std::string response = "200 Users list:\n";

  for (const auto &user : users)
    response += "UUID: " + std::string(user.uuid) +
                ", Name: " + std::string(user.name) + "\n";

  client->sendMessage(response);
  LOG_DEBUG("Users listed [" + std::to_string(users.size()) + "] users sent to client.");
}
} // namespace commands
