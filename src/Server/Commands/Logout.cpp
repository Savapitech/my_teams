#include "Logout.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}
#include <stdexcept>

namespace commands {
void Logout::execute(std::shared_ptr<Client> client,
                     std::vector<std::string> &args) {
  if (args.size() > 0) {
    client->sendMessage("400 Bad request, too many args\n");
    return;
  }

  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  User const &u = client->getActualUser();
  server_event_user_logged_out(u.uuid);
  LOG_DEBUG("User \"" + std::string(u.name) + "\" is logged out, uuid \"" +
            std::string(u.uuid) + "\"");
  client->sendMessage("200 Logged out \"" + std::string(u.name) + "\" \"" +
                      std::string(u.uuid) + "\"\n");

  auto const &activeClients = client->getServer().get().getClients();

  std::string broadcastMsg =
      "100 user_logged_out: \"" + std::string(client->getActualUser().uuid) +
      "\" \"" + std::string(client->getActualUser().name) + "\"\n";

  for (auto &c : activeClients)
    if (c->isLoggedIn() && c.get() != client.get())
      c->sendMessage(broadcastMsg);
  client->setLogged(false);

  client->clearCtx();
  client->disconnect();
}
} // namespace commands
