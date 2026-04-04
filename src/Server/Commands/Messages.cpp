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
  User const actualUser = client->getActualUser();
  auto const &messages = client->getServer().get().getDatabase().getMessages();

  std::string response = "200 Messages:\n";
  bool hasMessages = false;

  for (const auto &msg : messages) {
    std::string s_uuid(msg.sender_uuid);
    std::string r_uuid(msg.receiver_uuid);
    std::string a_uuid(actualUser.uuid);

    if ((s_uuid == a_uuid && r_uuid == targetUuid) ||
        (s_uuid == targetUuid && r_uuid == a_uuid)) {
      hasMessages = true;
      response += s_uuid + " -> " + r_uuid + " [" +
                  std::to_string(msg.timestamp) +
                  "]: " + std::string(msg.body) + "\n";
    }
  }

  if (!hasMessages)
    response += "No messages found.\n";

  client->sendMessage(response);
}
} // namespace commands
