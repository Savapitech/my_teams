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
#include <ctime>

namespace commands {
void Create::execute(std::shared_ptr<Client> client,
                     std::vector<std::string> &args) {
  if (!client->isLoggedIn()) {
    client->sendMessage("401 Unauthorized\n");
    return;
  }

  std::string teamCtx = client->getTeamContext();
  std::string channelCtx = client->getChannelContext();
  std::string threadCtx = client->getThreadContext();

  auto &db = client->getServer().get().getDatabase();

  if (teamCtx.empty() && channelCtx.empty() && threadCtx.empty()) {
    if (args.size() != 2) {
      client->sendMessage("400 Bad request, need team name and description\n");
      return;
    }

    auto &teams = db.getTeams();
    for (auto &team : teams) {
      if (std::string(team.name) == args[0]) {
        client->sendMessage("409 Team already exist\n");
        return;
      }
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
    client->sendMessage("201 Team created: " + std::string(newTeam.uuid) + " " +
                        std::string(newTeam.name) + " " +
                        std::string(newTeam.description) + "\n");
  } else if (!teamCtx.empty() && channelCtx.empty() && threadCtx.empty()) {
    if (args.size() != 2) {
      client->sendMessage(
          "400 Bad request, need channel name and description\n");
      return;
    }

    Channel newChannel;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newChannel.uuid);
    std::strncpy(newChannel.team_uuid, teamCtx.c_str(), MAX_UUID_LENGTH - 1);
    newChannel.team_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newChannel.name, args[0].c_str(), MAX_NAME_LENGTH - 1);
    newChannel.name[MAX_NAME_LENGTH - 1] = '\0';
    std::strncpy(newChannel.description, args[1].c_str(),
                 MAX_DESCRIPTION_LENGTH - 1);
    newChannel.description[MAX_DESCRIPTION_LENGTH - 1] = '\0';

    db.getChannels().push_back(newChannel);
    client->sendMessage("201 Channel created: " + std::string(newChannel.uuid) +
                        " " + std::string(newChannel.name) + " " +
                        std::string(newChannel.description) + "\n");
  } else if (!teamCtx.empty() && !channelCtx.empty() && threadCtx.empty()) {
    if (args.size() != 2) {
      client->sendMessage("400 Bad request, need thread title and body\n");
      return;
    }

    Thread newThread;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newThread.uuid);
    std::strncpy(newThread.channel_uuid, channelCtx.c_str(),
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

    db.getThreads().push_back(newThread);
    client->sendMessage("201 Thread created: " + std::string(newThread.uuid) +
                        " " + std::string(newThread.creator_uuid) + " " +
                        std::to_string(newThread.timestamp) + " " +
                        std::string(newThread.title) + " " +
                        std::string(newThread.body) + "\n");
  } else if (!teamCtx.empty() && !channelCtx.empty() && !threadCtx.empty()) {
    if (args.size() != 1) {
      client->sendMessage("400 Bad request, need reply body\n");
      return;
    }

    Reply newReply;
    uuid_t raw_uuid;
    uuid_generate(raw_uuid);
    uuid_unparse_lower(raw_uuid, newReply.uuid);
    std::strncpy(newReply.thread_uuid, threadCtx.c_str(), MAX_UUID_LENGTH - 1);
    newReply.thread_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newReply.creator_uuid, client->getActualUser().uuid,
                 MAX_UUID_LENGTH - 1);
    newReply.creator_uuid[MAX_UUID_LENGTH - 1] = '\0';
    std::strncpy(newReply.body, args[0].c_str(), MAX_BODY_LENGTH - 1);
    newReply.body[MAX_BODY_LENGTH - 1] = '\0';
    newReply.timestamp = time(nullptr);

    db.getReplies().push_back(newReply);
    client->sendMessage(
        "201 Reply created: " + std::string(newReply.thread_uuid) + " " +
        std::string(newReply.creator_uuid) + " " +
        std::to_string(newReply.timestamp) + " " + std::string(newReply.body) +
        "\n");
  } else {
    client->sendMessage("400 Bad context\n");
  }
}
} // namespace commands
