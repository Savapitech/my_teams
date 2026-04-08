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
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  Client::ContextType ctxType = client->getContextType();

  std::string response = "200 List:\n";
  auto db = client->getServer().get().getDatabase();

  if (ctxType == Client::ContextType::NONE) {
    for (const auto &t : db.getTeams()) {
      response += "Team UUID: " + std::string(t.uuid) +
                  ", Name: " + std::string(t.name) +
                  ", Desc: " + std::string(t.description) + "\n";
    }
  } else if (ctxType == Client::ContextType::TEAM) {
    for (const auto &c : db.getChannels()) {
      if (std::string(c.team_uuid) == client->getTeamUuid()) {
        response += "Channel UUID: " + std::string(c.uuid) +
                    ", Name: " + std::string(c.name) +
                    ", Desc: " + std::string(c.description) + "\n";
      }
    }
  } else if (ctxType == Client::ContextType::CHANNEL) {
    for (const auto &th : db.getThreads()) {
      if (std::string(th.channel_uuid) == client->getChannelUuid()) {
        response += "Thread UUID: " + std::string(th.uuid) +
                    ", Title: " + std::string(th.title) +
                    ", Body: " + std::string(th.body) + "\n";
      }
    }
  } else if (ctxType == Client::ContextType::THREAD) {
    for (const auto &r : db.getReplies()) {
      if (std::string(r.thread_uuid) == client->getThreadUuid()) {
        response += "Reply from: " + std::string(r.creator_uuid) + " [" +
                    std::to_string(r.timestamp) + "]: " + std::string(r.body) +
                    "\n";
      }
    }
  }

  client->sendMessage(response);
}
} // namespace commands
