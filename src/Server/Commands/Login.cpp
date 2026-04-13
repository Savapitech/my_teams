#include <cstring>
#include <stdexcept>

#ifdef DARWIN_KERNEL
#include <sys/syslimits.h>
#include <uuid/uuid.h>
#else
#include <limits.h>
#include <uuid/uuid.h>
#endif

#include "Login.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}

namespace commands {
void Login::execute(std::shared_ptr<Client> client,
                    std::vector<std::string> &args) {
  if (!args.size() || args.size() > 1) {
    client->sendMessage("400 Bad request, no args or too much args\n");
    return;
  }

  auto &users = client->getServer().get().getDatabase().getUsers();

  for (auto &user : users) {
    if (std::string(user.name) == args[0]) {
      client->setLogged(true);
      client->setActualUser(user);
      server_event_user_logged_in(user.uuid);
      LOG_DEBUG("User " + std::string(user.name) + " is logged in, uuid " +
                std::string(user.uuid));
      client->sendMessage("200 OK \"" + std::string(user.uuid) + "\" \"" +
                          std::string(user.name) + "\"\n");
      return;
    }
  }

  User newUser;
  uuid_t raw_uuid;
  uuid_generate(raw_uuid);
  uuid_unparse_lower(raw_uuid, newUser.uuid);
  std::strncpy(newUser.name, args[0].c_str(), MAX_NAME_LENGTH - 1);
  newUser.name[MAX_NAME_LENGTH - 1] = '\0';

  users.push_back(newUser);

  client->setLogged(true);
  client->setActualUser(newUser);
  server_event_user_created(newUser.uuid, newUser.name);
  server_event_user_logged_in(newUser.uuid);

  LOG_DEBUG("Created new user " + std::string(newUser.name) + " with uuid " +
            std::string(newUser.uuid));
  client->sendMessage("200 OK \"" + std::string(newUser.uuid) + "\" \"" +
                      std::string(newUser.name) + "\"\n");
}
} // namespace commands
