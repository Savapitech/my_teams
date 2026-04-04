#include "Send.hpp"
#include "Server/Client.hpp"
#include "Server/Models.hpp"
#include "Server/Server.hpp"
#include <cstring>
#include <ctime>
#include <string>

#ifdef DARWIN_KERNEL
#include <uuid/uuid.h>
#else
#include <uuid/uuid.h>
#endif

namespace commands {
void Send::execute(std::shared_ptr<Client> client,
                   std::vector<std::string> &args) {
  if (!client->isLoggedIn())
    return client->sendMessage("401 Unauthorized\n");
  if (args.size() != 2)
    return client->sendMessage(
        "400 Bad request, need user_uuid and message_body\n");
  Message msg;
  uuid_t raw_uuid;
  uuid_generate(raw_uuid);
  uuid_unparse_lower(raw_uuid, msg.uuid);
  std::strncpy(msg.sender_uuid, client->getActualUser().uuid,
               MAX_UUID_LENGTH - 1);
  msg.sender_uuid[MAX_UUID_LENGTH - 1] = '\0';
  std::strncpy(msg.receiver_uuid, args[0].c_str(), MAX_UUID_LENGTH - 1);
  msg.receiver_uuid[MAX_UUID_LENGTH - 1] = '\0';
  std::strncpy(msg.body, args[1].c_str(), MAX_BODY_LENGTH - 1);
  msg.body[MAX_BODY_LENGTH - 1] = '\0';
  msg.timestamp = time(nullptr);
  client->getServer().get().getDatabase().getMessages().push_back(msg);
  client->sendMessage("200 Message sent\n");
}
} // namespace commands