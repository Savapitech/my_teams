#include <stdexcept>

#include <cstring>

#ifdef DARWIN_KERNEL
#include <sys/syslimits.h>
#include <uuid/uuid.h>
#else
#include <limits.h>
#include <uuid/uuid.h>
#endif

#include "Create.hpp"
#include "Server/Client.hpp"
#include "Server/Models.hpp"
#include "Server/Server.hpp"
#include "Utils/Logger.hpp"
extern "C" {
#include "logging_server.h"
}
#include <ctime>

namespace commands {
void Create::execute(std::shared_ptr<Client> client,
                     std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  Client::ContextType ctxType = client->getContextType();
  auto &db = client->getServer().get().getDatabase();

  if (ctxType == Client::ContextType::NONE) {
    if (args.size() != 2)
      return client->sendMessage("400 Bad request\n");

    auto &teams = db.getTeams();
    for (auto &team : teams) {
      if (std::string(team.name) == args[0])
        return client->sendMessage("409 Conflict\n");
    }

    Team newTeam;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newTeam.uuid);
    std::strncpy(newTeam.name, args[0].c_str(), MAX_NAME_LENGTH - 1);
    newTeam.name[MAX_NAME_LENGTH - 1] = '\0';
    std::strncpy(newTeam.description, args[1].c_str(),
                 MAX_DESCRIPTION_LENGTH - 1);
    newTeam.description[MAX_DESCRIPTION_LENGTH - 1] = '\0';

    teams.push_back(newTeam);
    server_event_team_created(newTeam.uuid, newTeam.name,
                              client->getActualUser().uuid);

    client->sendMessage("201 CREATED TEAM \"" + std::string(newTeam.uuid) +
                        "\" \"" + std::string(newTeam.name) + "\" \"" +
                        std::string(newTeam.description) + "\"\n");
  } else if (ctxType == Client::ContextType::TEAM) {
    if (args.size() != 2)
      return client->sendMessage("400 Bad request\n");

    auto &channels = db.getChannels();
    for (auto &channel : channels) {
      if (std::string(channel.team_uuid) == client->getTeamUuid() &&
          std::string(channel.name) == args[0]) {
        return client->sendMessage("409 Conflict\n");
      }
    }

    Channel newChannel;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newChannel.uuid);
    std::strncpy(newChannel.team_uuid, client->getTeamUuid().c_str(),
                 MAX_UUID_LENGTH - 1);
    newChannel.team_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newChannel.name, args[0].c_str(), MAX_NAME_LENGTH - 1);
    newChannel.name[MAX_NAME_LENGTH - 1] = '\0';
    std::strncpy(newChannel.description, args[1].c_str(),
                 MAX_DESCRIPTION_LENGTH - 1);
    newChannel.description[MAX_DESCRIPTION_LENGTH - 1] = '\0';

    channels.push_back(newChannel);
    server_event_channel_created(newChannel.team_uuid, newChannel.uuid,
                                 newChannel.name);
    client->sendMessage("201 CREATED CHANNEL \"" +
                        std::string(newChannel.uuid) + "\" \"" +
                        std::string(newChannel.name) + "\" \"" +
                        std::string(newChannel.description) + "\"\n");
  } else if (ctxType == Client::ContextType::CHANNEL) {
    if (args.size() != 2)
      return client->sendMessage("400 Bad request\n");

    auto &threads = db.getThreads();
    for (auto &thread : threads) {
      if (std::string(thread.channel_uuid) == client->getChannelUuid() &&
          std::string(thread.title) == args[0]) {
        return client->sendMessage("409 Conflict\n");
      }
    }

    Thread newThread;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newThread.uuid);
    std::strncpy(newThread.channel_uuid, client->getChannelUuid().c_str(),
                 MAX_UUID_LENGTH - 1);
    newThread.channel_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newThread.creator_uuid, client->getActualUser().uuid,
                 MAX_UUID_LENGTH - 1);
    newThread.creator_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newThread.title, args[0].c_str(), MAX_NAME_LENGTH - 1);
    newThread.title[MAX_NAME_LENGTH - 1] = '\0';
    std::strncpy(newThread.body, args[1].c_str(), MAX_BODY_LENGTH - 1);
    newThread.body[MAX_BODY_LENGTH - 1] = '\0';
    newThread.timestamp = time(nullptr);

    threads.push_back(newThread);
    server_event_thread_created(newThread.channel_uuid, newThread.uuid,
                                newThread.creator_uuid, newThread.title,
                                newThread.body);

    client->sendMessage("201 CREATED THREAD \"" + std::string(newThread.uuid) +
                        "\" \"" + std::string(newThread.creator_uuid) +
                        "\" \"" + std::to_string(newThread.timestamp) +
                        "\" \"" + std::string(newThread.title) + "\" \"" +
                        std::string(newThread.body) + "\"\n");
  } else if (ctxType == Client::ContextType::THREAD) {
    if (args.size() != 1)
      return client->sendMessage("400 Bad request\n");

    Reply newReply;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newReply.uuid);
    std::strncpy(newReply.thread_uuid, client->getThreadUuid().c_str(),
                 MAX_UUID_LENGTH - 1);
    newReply.thread_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newReply.creator_uuid, client->getActualUser().uuid,
                 MAX_UUID_LENGTH - 1);
    newReply.creator_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newReply.body, args[0].c_str(), MAX_BODY_LENGTH - 1);
    newReply.body[MAX_BODY_LENGTH - 1] = '\0';
    newReply.timestamp = time(nullptr);

    db.getReplies().push_back(newReply);
    server_event_reply_created(newReply.thread_uuid, newReply.creator_uuid,
                               newReply.body);

    client->sendMessage("201 CREATED REPLY \"" +
                        std::string(newReply.thread_uuid) + "\" \"" +
                        std::string(newReply.creator_uuid) + "\" \"" +
                        std::to_string(newReply.timestamp) + "\" \"" +
                        std::string(newReply.body) + "\"\n");
  } else {
    client->sendMessage("400 Bad context\n");
  }
}
} // namespace commands