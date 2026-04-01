#include <stdexcept>

#include <cstring>
#include <sys/syslimits.h>
#include <uuid/uuid.h>

#include "Create.hpp"
#include "Server/Client.hpp"
#include "Server/Models.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

namespace commands {
void Create::execute(std::shared_ptr<Client> client,
                     std::vector<std::string> &args) {
  if (!args.size() || args.size() > 1)
    throw std::runtime_error("400 Bad request, no args or too much args");

  auto &users = client->getServer().get().getDatabase().getUsers();

  for (auto &user : users) {
    if (user.name == args[0]) {
      client->sendMessage("409 User already exist\n");
      LOG_DEBUG("Cannot create a new user, user " + std::string(user.name) +
                " already exist");
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

  LOG_DEBUG("Created new user " + std::string(newUser.name) + " with uuid " +
            std::string(newUser.uuid));
  client->sendMessage("201 User created: " + std::string(newUser.name) +
                      " uuid " + std::string(newUser.uuid) + "\n");
}
} // namespace commands
