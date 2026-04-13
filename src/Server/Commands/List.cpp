#include "List.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}

namespace commands {
void List::execute(std::shared_ptr<Client> client,
                   std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  Client::ContextType ctxType = client->getContextType();
  auto &db = client->getServer().get().getDatabase();

  if (ctxType == Client::ContextType::NONE) {
    std::string response = "200 OK TEAM ";
    for (const auto &t : db.getTeams()) {
      response += "\"" + std::string(t.uuid) + "\" \"" + std::string(t.name) +
                  "\" \"" + std::string(t.description) + "\" ";
    }
    response += "\n";
    client->sendMessage(response);
  }

  else if (ctxType == Client::ContextType::TEAM) {
    std::string response = "200 OK CHANNEL ";
    for (const auto &c : db.getChannels()) {
      if (std::string(c.team_uuid) == client->getTeamUuid()) {
        response += "\"" + std::string(c.uuid) + "\" \"" + std::string(c.name) +
                    "\" \"" + std::string(c.description) + "\" ";
      }
    }
    response += "\n";
    client->sendMessage(response);
  }

  else if (ctxType == Client::ContextType::CHANNEL) {
    std::string response = "200 OK THREAD ";
    for (const auto &th : db.getThreads()) {
      if (std::string(th.channel_uuid) == client->getChannelUuid()) {
        response += "\"" + std::string(th.uuid) + "\" \"" +
                    std::string(th.creator_uuid) + "\" \"" +
                    std::to_string(th.timestamp) + "\" \"" +
                    std::string(th.title) + "\" \"" + std::string(th.body) +
                    "\" ";
      }
    }
    response += "\n";
    client->sendMessage(response);
  }

  else if (ctxType == Client::ContextType::THREAD) {
    std::string response = "200 OK REPLY ";
    for (const auto &r : db.getReplies()) {
      if (std::string(r.thread_uuid) == client->getThreadUuid()) {
        response += "\"" + std::string(r.thread_uuid) + "\" \"" +
                    std::string(r.creator_uuid) + "\" \"" +
                    std::to_string(r.timestamp) + "\" \"" +
                    std::string(r.body) + "\" ";
      }
    }
    response += "\n";
    client->sendMessage(response);
  }
}
} // namespace commands