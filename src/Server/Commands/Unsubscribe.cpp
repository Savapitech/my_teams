#include "Unsubscribe.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

extern "C" {
#include "logging_server.h"
}
#include <algorithm>

namespace commands {
void Unsubscribe::execute(std::shared_ptr<Client> client,
                          std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  if (args.size() != 1) {
    client->sendMessage("400 Bad request: /unsubscribe <team_uuid>\n");
    return;
  }

  std::string const &team_uuid = args[0];
  User const actualUser = client->getActualUser();
  auto &subscriptions =
      client->getServer().get().getDatabase().getSubscriptions();

  auto it = std::remove_if(
      subscriptions.begin(), subscriptions.end(), [&](const Subscription &sub) {
        return std::string(sub.team_uuid) == team_uuid &&
               std::string(sub.user_uuid) == std::string(actualUser.uuid);
      });

  if (it == subscriptions.end()) {
    client->sendMessage("404 Not Found: Subscription not found\n");
    return;
  }

  subscriptions.erase(it, subscriptions.end());

  server_event_user_unsubscribed(team_uuid.c_str(), actualUser.uuid);
  LOG_DEBUG("User " + std::string(actualUser.uuid) +
            " unsubscribed from team " + team_uuid);

  client->sendMessage("200 Unsubscribed successfully from team " + team_uuid +
                      "\n");
}
} // namespace commands
