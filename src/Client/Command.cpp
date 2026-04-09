#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ICommand.hpp"

extern "C" {
#include "logging_client.h"
}

#define CLR_BOLD_INFO "\033[1;34m"
#define CLR_BOLD_WARNING "\033[1;35m"
#define CLR_BOLD_ERROR "\033[1;31m"
#define CLR_BOLD_DEBUG "\033[1;32m"
#define CLR_INFO "\033[0;34m"
#define CLR_WARNING "\033[0;35m"
#define CLR_ERROR "\033[0;31m"
#define CLR_DEBUG "\033[0;32m"
#define CLR_RESET "\033[0m"

std::vector<std::string> extractArgs(const std::string &response) {
  std::vector<std::string> args;
  std::stringstream ss(response);
  std::string word;

  while (ss >> word) {
    args.push_back(word);
  }

  return args;
}
int getStatusCode(const std::string &response) {
  if (response.empty())
    return 0;
  try {
    return std::stoi(response.substr(0, 3));
  } catch (const std::exception &e) {
    std::cout << e.what() << "\n";
    return 0;
  }
}

void HelpCommand::logCommand(const std::string &serverResponse) {
  std::cout << "How can i help you today ?\n";
}

void LoginCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::cout << CLR_DEBUG << "LoginCommand:" << status << CLR_RESET << std::endl;
  if (status == 200) {
    std::vector<std::string> args = extractArgs(serverResponse);
    if (args.size() >= 2)
      client_event_logged_in(args[6].c_str(), args[4].c_str());
  } else if (status == 400) {
    client_error_unauthorized();
  }
}

void LogoutCommand::logCommand(const std::string &serverResponse) {
  std::cout << CLR_DEBUG << "LogoutCommand" << CLR_RESET << std::endl;
  int status = getStatusCode(serverResponse);
  if (status == 200) {
    std::vector<std::string> args = extractArgs(serverResponse);
    if (args.size() >= 2)
      client_event_logged_out(args[4].c_str(), args[3].c_str());
  }
}

void UsersCommand::logCommand(const std::string &serverResponse) {
  std::cout << CLR_DEBUG << "Users Command" << CLR_RESET << std::endl;
  int status = getStatusCode(serverResponse);
  if (status == 200) {
    std::vector<std::string> args = extractArgs(serverResponse);
    for (size_t i = 3; i + 5 < args.size(); i += 6) {
      std::cout << CLR_DEBUG << "User UUID: [" << args[i + 1].c_str()
                << "], User Name: [" << args[i + 3].c_str()
                << "], IsConnected: [" << args[i + 5] << "]" << CLR_RESET
                << std::endl;
      client_print_users(args[i + 1].c_str(), args[i + 3].c_str(),
                         std::stoi(args[i + 5]));
    }
  }
}

void UserCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200 && args.size() >= 3) {
    client_print_user(args[6].c_str(), args[4].c_str(), std::stoi(args[8]));
  } else if (status == 404 && args.size() >= 1) {
    client_error_unknown_user(args[4].c_str());
  }
}

void SendCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  if (status == 404) {
    std::vector<std::string> args = extractArgs(serverResponse);
    if (!args.empty())
      client_error_unknown_user(
          args[0].c_str()); // Il faut que le server renvoie une 400 quand ça
                            // failed avec le uuid
  }
}

void MessagesCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200) {
    for (size_t i = 0; i + 2 < args.size(); i += 3) {
      time_t timestamp = std::stoll(args[i + 1]);
      client_private_message_print_messages(
          args[i].c_str(), timestamp,
          args[i + 2].c_str()); // On peux pas envoyé de message
    }
  } else if (status == 404 && !args.empty()) {
    client_error_unknown_user(args[0].c_str());
  }
}

void SubscribeCommand::logCommand(
    const std::string &serverResponse) { // Faut que le server renvoit l'uuid
                                         // que le client a essayé d'envoyer
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200 && args.size() >= 2) {
    client_print_subscribed(args[0].c_str(), args[1].c_str());
  } else if (status == 404 && !args.empty()) {
    client_error_unknown_team(args[0].c_str());
  }
}

void UnsubscribeCommand::logCommand(
    const std::string
        &serverResponse) { // Faut que le server renvoit l'uuid que le client a
                           // essayé d'envoyer et le user si ça marche
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200 && args.size() >= 2) {
    client_print_unsubscribed(args[0].c_str(), args[1].c_str());
  } else if (status == 404 && !args.empty()) {
    client_error_unknown_team(args[0].c_str());
  }
}

void SubscribedCommand::logCommand(
    const std::string
        &serverResponse) { // Faut renvoyer les infos du teams ou des clients
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200) {
    if (serverResponse.find("TEAM") != std::string::npos) {
      for (size_t i = 0; i + 2 < args.size(); i += 3) {
        client_print_teams(args[i].c_str(), args[i + 1].c_str(),
                           args[i + 2].c_str());
      }
    } else if (serverResponse.find("USER") != std::string::npos) {
      for (size_t i = 0; i + 2 < args.size(); i += 3) {
        client_print_users(args[i].c_str(), args[i + 1].c_str(),
                           std::stoi(args[i + 2]));
      }
    }
  } else if (status == 404 && !args.empty()) {
    client_error_unknown_team(args[0].c_str());
  }
}

void UseCommand::logCommand(const std::string &serverResponse) { //
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 404 && !args.empty()) {
    if (serverResponse.find("TEAM") != std::string::npos)
      client_error_unknown_team(args[0].c_str());
    else if (serverResponse.find("CHANNEL") != std::string::npos)
      client_error_unknown_channel(args[0].c_str());
    else if (serverResponse.find("THREAD") != std::string::npos)
      client_error_unknown_thread(args[0].c_str());
  }
}

void CreateCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 201) {
    if (serverResponse.find("TEAM") != std::string::npos && args.size() >= 3)
      client_print_team_created(args[0].c_str(), args[1].c_str(),
                                args[2].c_str());
    else if (serverResponse.find("CHANNEL") != std::string::npos &&
             args.size() >= 3)
      client_print_channel_created(args[0].c_str(), args[1].c_str(),
                                   args[2].c_str());
    else if (serverResponse.find("THREAD") != std::string::npos &&
             args.size() >= 5)
      client_print_thread_created(args[0].c_str(), args[1].c_str(),
                                  std::stoll(args[2]), args[3].c_str(),
                                  args[4].c_str());
    else if (serverResponse.find("REPLY") != std::string::npos &&
             args.size() >= 4)
      client_print_reply_created(args[0].c_str(), args[1].c_str(),
                                 std::stoll(args[2]), args[3].c_str());
  } else if (status == 409) {
    client_error_already_exist();
  } else if (status == 401 || status == 403) {
    client_error_unauthorized();
  }
}

void ListCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200) {
    if (serverResponse.find("TEAM") != std::string::npos) {
      for (size_t i = 0; i + 2 < args.size(); i += 3)
        client_print_teams(args[i].c_str(), args[i + 1].c_str(),
                           args[i + 2].c_str());
    } else if (serverResponse.find("CHANNEL") != std::string::npos) {
      for (size_t i = 0; i + 2 < args.size(); i += 3)
        client_team_print_channels(args[i].c_str(), args[i + 1].c_str(),
                                   args[i + 2].c_str());
    } else if (serverResponse.find("THREAD") != std::string::npos) {
      for (size_t i = 0; i + 4 < args.size(); i += 5)
        client_channel_print_threads(args[i].c_str(), args[i + 1].c_str(),
                                     std::stoll(args[i + 2]),
                                     args[i + 3].c_str(), args[i + 4].c_str());
    } else if (serverResponse.find("REPLY") != std::string::npos) {
      for (size_t i = 0; i + 3 < args.size(); i += 4)
        client_thread_print_replies(args[i].c_str(), args[i + 1].c_str(),
                                    std::stoll(args[i + 2]),
                                    args[i + 3].c_str());
    }
  }
}

void InfoCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractArgs(serverResponse);

  if (status == 200) {
    if (serverResponse.find("USER") != std::string::npos && args.size() >= 3)
      client_print_user(args[0].c_str(), args[1].c_str(), std::stoi(args[2]));
    else if (serverResponse.find("TEAM") != std::string::npos &&
             args.size() >= 3)
      client_print_team(args[0].c_str(), args[1].c_str(), args[2].c_str());
    else if (serverResponse.find("CHANNEL") != std::string::npos &&
             args.size() >= 3)
      client_print_channel(args[0].c_str(), args[1].c_str(), args[2].c_str());
    else if (serverResponse.find("THREAD") != std::string::npos &&
             args.size() >= 5)
      client_print_thread(args[0].c_str(), args[1].c_str(), std::stoll(args[2]),
                          args[3].c_str(), args[4].c_str());
  }
}