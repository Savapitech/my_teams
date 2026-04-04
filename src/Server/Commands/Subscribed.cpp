#include "Subscribed.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}

namespace commands {
void Subscribed::execute(std::shared_ptr<Client> client,
                         std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  auto const &subscriptions =
      client->getServer().get().getDatabase().getSubscriptions();
  auto const &teams = client->getServer().get().getDatabase().getTeams();
  auto const &users = client->getServer().get().getDatabase().getUsers();

  std::string response = "200 Subscribed:\n";

  if (args.empty()) {
    User const actualUser = client->getActualUser();
    for (const auto &sub : subscriptions) {
      if (std::string(sub.user_uuid) == std::string(actualUser.uuid)) {
        for (const auto &t : teams) {
          if (std::string(t.uuid) == std::string(sub.team_uuid)) {
            response += "Team UUID: " + std::string(t.uuid) +
                        ", Name: " + std::string(t.name) + "\n";
          }
        }
      }
    }
  } else {
    std::string const &team_uuid = args[0];
    for (const auto &sub : subscriptions) {
      if (std::string(sub.team_uuid) == team_uuid) {
        for (const auto &u : users) {
          if (std::string(u.uuid) == std::string(sub.user_uuid)) {
            response += "User UUID: " + std::string(u.uuid) +
                        ", Name: " + std::string(u.name) + "\n";
          }
        }
      }
    }
  }

  client->sendMessage(response);
}
} // namespace commands
