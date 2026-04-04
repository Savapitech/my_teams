#pragma once
#include "ICommand.hpp"
#include "Server/Models.hpp"

namespace commands {
class Create : public ICommand {
public:
  void execute(std::shared_ptr<Client> client,
               std::vector<std::string> &args) override;

private:
  void createTeam(std::shared_ptr<Client> client,
                  std::vector<std::string> &args, const User &user);
  void createChannel(std::shared_ptr<Client> client,
                     std::vector<std::string> &args,
                     const std::string &teamUuid);
  void createThread(std::shared_ptr<Client> client,
                    std::vector<std::string> &args,
                    const std::string &channelUuid, const User &user);
  void createReply(std::shared_ptr<Client> client,
                   std::vector<std::string> &args,
                   const std::string &threadUuid, const User &user);
};
} // namespace commands
