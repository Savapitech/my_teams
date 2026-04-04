#include "Use.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Use::execute(std::shared_ptr<Client> client,
                  std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  if (args.size() > 3) {
    client->sendMessage("400 Bad request, too many args\n");
    return;
  }

  client->clearCtx();

  if (args.empty()) {
    client->sendMessage("200 Context cleared\n");
    return;
  }

  auto const &teams = client->getServer().get().getDatabase().getTeams();
  auto const &channels = client->getServer().get().getDatabase().getChannels();
  auto const &threads = client->getServer().get().getDatabase().getThreads();

  std::string lastMatchedUuid = "";
  Client::ContextType matchedType = Client::ContextType::NONE;
  bool found = false;

  for (const auto &th : threads) {
    if (std::string(th.uuid) == args[0]) {
      matchedType = Client::ContextType::THREAD;
      lastMatchedUuid = args[0];
      found = true;
      break;
    }
  }

  for (const auto &c : channels) {
    if (std::string(c.uuid) == args[0]) {
      matchedType = Client::ContextType::CHANNEL;
      lastMatchedUuid = args[0];
      found = true;
      break;
    }
  }

  for (const auto &t : teams) {
    if (std::string(t.uuid) == args[0]) {
      matchedType = Client::ContextType::TEAM;
      lastMatchedUuid = args[0];
      found = true;
      break;
    }
  }

  if (!found) {
    client->sendMessage("404 Not Found: Unknown UUID inside use command\n");
    client->clearCtx();
    return;
  }

  if (matchedType != Client::ContextType::NONE)
    client->setContext(matchedType, lastMatchedUuid);
  client->sendMessage("200 Context updated\n");
}
} // namespace commands
