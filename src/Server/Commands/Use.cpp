#include "Use.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Use::execute(std::shared_ptr<Client> client,
                  std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  if (args.size() > 3) {
    client->sendMessage("400 Bad request\n");
    return;
  }

  client->clearCtx();

  if (args.empty()) {
    client->sendMessage("200 OK\n");
    return;
  }

  auto &db = client->getServer().get().getDatabase();

  const std::string &teamUuid = args[0];
  bool teamFound = false;
  for (const auto &t : db.getTeams()) {
    if (std::string(t.uuid) == teamUuid) {
      teamFound = true;
      break;
    }
  }
  if (!teamFound) {
    client->sendMessage("404 TEAM \"" + teamUuid + "\"\n");
    return;
  }
  client->setTeamUuid(teamUuid);
  client->setContextType(Client::ContextType::TEAM);

  if (args.size() == 1) {
    client->sendMessage("200 OK\n");
    return;
  }

  const std::string &channelUuid = args[1];
  bool channelFound = false;
  for (const auto &c : db.getChannels()) {
    if (std::string(c.uuid) == channelUuid &&
        std::string(c.team_uuid) == teamUuid) {
      channelFound = true;
      break;
    }
  }
  if (!channelFound) {
    client->sendMessage("404 CHANNEL \"" + channelUuid + "\"\n");
    client->clearCtx();
    return;
  }
  client->setChannelUuid(channelUuid);
  client->setContextType(Client::ContextType::CHANNEL);

  if (args.size() == 2) {
    client->sendMessage("200 OK\n");
    return;
  }

  const std::string &threadUuid = args[2];
  bool threadFound = false;
  for (const auto &th : db.getThreads()) {
    if (std::string(th.uuid) == threadUuid &&
        std::string(th.channel_uuid) == channelUuid) {
      threadFound = true;
      break;
    }
  }
  if (!threadFound) {
    client->sendMessage("404 THREAD \"" + threadUuid + "\"\n");
    client->clearCtx();
    return;
  }
  client->setThreadUuid(threadUuid);
  client->setContextType(Client::ContextType::THREAD);

  client->sendMessage("200 OK\n");
}
} // namespace commands
