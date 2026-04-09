#include "Subscribe.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

extern "C" {
#include "logging_server.h"
}
#include <cstring>

namespace commands {
void Subscribe::execute(std::shared_ptr<Client> client,
                        std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  if (args.size() != 1) {
    client->sendMessage("400 Bad request: /subscribe <team_uuid>\n");
    return;
  }

  std::string const &team_uuid = args[0];
  auto &teams = client->getServer().get().getDatabase().getTeams();
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

  User const actualUser = client->getActualUser();
  auto &subscriptions =
      client->getServer().get().getDatabase().getSubscriptions();

  for (const auto &sub : subscriptions) {
    if (std::string(sub.team_uuid) == team_uuid &&
        std::string(sub.user_uuid) == std::string(actualUser.uuid)) {
      client->sendMessage("409 Conflict: Already subscribed to team\n");
      return;
    }
  }

  Subscription newSub;
  std::strncpy(newSub.team_uuid, team_uuid.c_str(), MAX_UUID_LENGTH - 1);
  newSub.team_uuid[MAX_UUID_LENGTH - 1] = '\0';

  std::strncpy(newSub.user_uuid, actualUser.uuid, MAX_UUID_LENGTH - 1);
  newSub.user_uuid[MAX_UUID_LENGTH - 1] = '\0';

  subscriptions.push_back(newSub);

  server_event_user_subscribed(team_uuid.c_str(), actualUser.uuid);
  LOG_DEBUG("User " + std::string(actualUser.uuid) + " subscribed to team " +
            team_uuid);

  client->sendMessage("200 OK \"" + std::string(actualUser.uuid) + "\" \"" +
                      team_uuid + "\"\n");
}
} // namespace commands
