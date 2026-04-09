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

  if (args.empty()) {
    std::string response = "200 OK TEAM ";
    User const actualUser = client->getActualUser();

    for (const auto &sub : subscriptions) {
      if (std::string(sub.user_uuid) == std::string(actualUser.uuid)) {
        for (const auto &t : teams) {
          if (std::string(t.uuid) == std::string(sub.team_uuid)) {
            response += "\"" + std::string(t.uuid) + "\" \"" +
                        std::string(t.name) + "\" \"" +
                        std::string(t.description) + "\" ";
          }
        }
      }
    }
    response += "\n";
    client->sendMessage(response);
  }

  else {
    std::string const &team_uuid = args[0];
    bool teamExists = false;
    for (const auto &t : teams) {
      if (std::string(t.uuid) == team_uuid) {
        teamExists = true;
        break;
      }
    }

    if (!teamExists) {
      client->sendMessage("404 \"" + team_uuid + "\"\n");
      return;
    }
    std::string response = "200 OK USER ";
    for (const auto &sub : subscriptions) {
      if (std::string(sub.team_uuid) == team_uuid) {
        for (const auto &u : users) {
          bool isConnected = false;

          for (const auto &isActive : client->getServer().get().getClients()) {
            if (isActive->isLoggedIn() &&
                std::string(isActive->getActualUser().uuid) ==
                    std::string(u.uuid)) {
              isConnected = true;
              break;
            }
          }
          if (std::string(u.uuid) == std::string(sub.user_uuid)) {
            response += "\"" + std::string(u.uuid) + "\" \"" +
                        std::string(u.name) + "\" \"" +
                        (isConnected ? "1" : "0") + "\" ";
          }
        }
      }
    }
    response += "\n";
    client->sendMessage(response);
  }
}
} // namespace commands