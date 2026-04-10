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

  client->setLogged(false);
  client->clearCtx();
  client->disconnect();
}
} // namespace commands