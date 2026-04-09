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
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  Client::ContextType ctxType = client->getContextType();
  auto db = client->getServer().get().getDatabase();

  if (ctxType == Client::ContextType::NONE) {
    User const actualUser = client->getActualUser();
    client->sendMessage("200 OK USER \"" + std::string(actualUser.uuid) +
                        "\" \"" + std::string(actualUser.name) + "\" \"1\"\n");
  } else if (ctxType == Client::ContextType::TEAM) {
    for (const auto &t : db.getTeams()) {
      if (std::string(t.uuid) == client->getTeamUuid()) {
        client->sendMessage("200 OK TEAM \"" + std::string(t.uuid) + "\" \"" +
                            std::string(t.name) + "\" \"" +
                            std::string(t.description) + "\"\n");
        break;
      }
    }
  } else if (ctxType == Client::ContextType::CHANNEL) {
    for (const auto &c : db.getChannels()) {
      if (std::string(c.uuid) == client->getChannelUuid()) {
        client->sendMessage("200 OK CHANNEL \"" + std::string(c.uuid) +
                            "\" \"" + std::string(c.name) + "\" \"" +
                            std::string(c.description) + "\"\n");
        break;
      }
    }
  } else if (ctxType == Client::ContextType::THREAD) {
    for (const auto &th : db.getThreads()) {
      if (std::string(th.uuid) == client->getThreadUuid()) {
        client->sendMessage("200 OK THREAD \"" + std::string(th.uuid) +
                            "\" \"" + std::string(th.creator_uuid) + "\" \"" +
                            std::to_string(th.timestamp) + "\" \"" +
                            std::string(th.title) + "\" \"" +
                            std::string(th.body) + "\"\n");
        break;
      }
    }
  }
}
} // namespace commands