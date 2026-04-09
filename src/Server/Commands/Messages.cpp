#include "Messages.hpp"
#include "Server/Client.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"

#include <string>
#include <vector>

namespace commands {
void Messages::execute(std::shared_ptr<Client> client,
                       std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized: Please login first\n");
    return;
  }

  if (args.size() != 1) {
    client->sendMessage("400 Bad request: /messages <user_uuid>\n");
    return;
  }

  std::string const &targetUuid = args[0];
  bool userExists = false;
  for (const auto &user : client->getServer().get().getDatabase().getUsers()) {
    if (std::string(user.uuid) == targetUuid) {
      userExists = true;
      break;
    }
  }

  if (!userExists) {
    client->sendMessage("404 \"" + targetUuid + "\"\n");
    return;
  }

  User const actualUser = client->getActualUser();
  auto const &messages = client->getServer().get().getDatabase().getMessages();

  std::string response = "200 OK ";

  for (const auto &msg : messages) {
    std::string s_uuid(msg.sender_uuid);
    std::string r_uuid(msg.receiver_uuid);
    std::string a_uuid(actualUser.uuid);

    if ((s_uuid == a_uuid && r_uuid == targetUuid) ||
        (s_uuid == targetUuid && r_uuid == a_uuid)) {

      response += "\"" + s_uuid + "\" \"" + std::to_string(msg.timestamp) +
                  "\" \"" + std::string(msg.body) + "\" ";
    }
  }

  response += "\n";
  client->sendMessage(response);
}
} // namespace commands
