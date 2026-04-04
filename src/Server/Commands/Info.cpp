#include "Info.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}

namespace commands {
void Info::execute(std::shared_ptr<Client> client,
                   std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  Client::ContextType ctxType = client->getContextType();
  std::string ctxUuid = client->getContextUuid();
  auto db = client->getServer().get().getDatabase();

  std::string response = "200 Info:\n";

  if (ctxType == Client::ContextType::NONE) {
    User const actualUser = client->getActualUser();
    response += "Current User UUID: " + std::string(actualUser.uuid) +
                ", Name: " + std::string(actualUser.name) + "\n";
  } else if (ctxType == Client::ContextType::TEAM) {
    for (const auto &t : db.getTeams()) {
      if (std::string(t.uuid) == ctxUuid) {
        response += "Team UUID: " + std::string(t.uuid) +
                    ", Name: " + std::string(t.name) +
                    ", Desc: " + std::string(t.description) + "\n";
        break;
      }
    }
  } else if (ctxType == Client::ContextType::CHANNEL) {
    for (const auto &c : db.getChannels()) {
      if (std::string(c.uuid) == ctxUuid) {
        response += "Channel UUID: " + std::string(c.uuid) +
                    ", Name: " + std::string(c.name) +
                    ", Desc: " + std::string(c.description) + "\n";
        break;
      }
    }
  } else if (ctxType == Client::ContextType::THREAD) {
    for (const auto &th : db.getThreads()) {
      if (std::string(th.uuid) == ctxUuid) {
        response += "Thread UUID: " + std::string(th.uuid) +
                    ", Title: " + std::string(th.title) +
                    ", Body: " + std::string(th.body) + "\n";
        break;
      }
    }
  }

  client->sendMessage(response);
}
} // namespace commands
