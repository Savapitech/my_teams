#include "Utils/Logger.hpp"
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ICommand.hpp"

extern "C" {
#include "logging_client.h"
}

std::vector<std::string> extractArgs(const std::string &response) {
  std::vector<std::string> args;
  std::stringstream ss(response);
  std::string word;

  while (ss >> word) {
    args.push_back(word);
  }

  return args;
}

std::vector<std::string> extractComplexArgs(const std::string &response) {
  std::vector<std::string> args;
  bool inQuotes = false;
  std::string currentArg = "";

  for (size_t i = 0; i < response.length(); ++i) {
    char c = response[i];

    if (c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ' ' && !inQuotes) {
      if (!currentArg.empty()) {
        args.push_back(currentArg);
        currentArg = "";
      }
    } else if (c != '\n' && c != '\r') {
      currentArg += c;
    }
  }
  if (!currentArg.empty()) {
    args.push_back(currentArg);
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
  std::cout << "Available commands:\n"
            << "  /help\n"
            << "  /login \"user_name\"\n"
            << "  /logout\n"
            << "  /users\n"
            << "  /user \"user_uuid\"\n"
            << "  /send \"user_uuid\" \"message_body\"\n"
            << "  /messages \"user_uuid\"\n"
            << "  /subscribe \"team_uuid\"\n"
            << "  /subscribed [\"team_uuid\"]\n"
            << "  /unsubscribe \"team_uuid\"\n"
            << "  /use [\"team_uuid\"] [\"channel_uuid\"] [\"thread_uuid\"]\n"
            << "  /create ...\n"
            << "  /list\n"
            << "  /info\n";
}

void LoginCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  if (status == 200) {
    std::vector<std::string> args = extractComplexArgs(serverResponse);
    if (args.size() >= 4)
      client_event_logged_in(args[2].c_str(), args[3].c_str());
  } else if (status == 400 || status == 401) {
    client_error_unauthorized();
  }
}

void LogoutCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  if (status == 200) {
    std::vector<std::string> args = extractComplexArgs(serverResponse);
    if (args.size() >= 5)
      client_event_logged_out(args[4].c_str(), args[3].c_str());
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void UsersCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);

  if (status == 200) {
    std::vector<std::string> args = extractComplexArgs(serverResponse);
    for (size_t i = 2; i + 2 < args.size(); i += 3) {
      try {
        client_print_users(args[i].c_str(), args[i + 1].c_str(),
                           std::stoi(args[i + 2]));
      } catch (...) {
      }
    }
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void UserCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() >= 4) {
    try {
      client_print_user(args[2].c_str(), args[3].c_str(), std::stoi(args[4]));
    } catch (...) {
    }
  } else if (status == 404 && args.size() >= 2) {
    client_error_unknown_user(args[1].c_str());
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void SendCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  if (status == 404) {
    std::vector<std::string> args = extractComplexArgs(serverResponse);
    if (args.size() >= 2)
      client_error_unknown_user(args[1].c_str());
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void MessagesCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200) {
    for (size_t i = 2; i + 2 < args.size(); i += 3) {
      try {
        time_t timestamp = std::stoll(args[i + 1]);
        client_private_message_print_messages(args[i].c_str(), timestamp,
                                              args[i + 2].c_str());
      } catch (...) {
      }
    }
  } else if (status == 404) {
    if (args.size() >= 2) {
      client_error_unknown_user(args[1].c_str());
    }
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void SubscribeCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() >= 4)
    client_print_subscribed(args[2].c_str(), args[3].c_str());
  else if (status == 404 && args.size() >= 2)
    client_error_unknown_team(args[1].c_str());
  else if (status == 401)
    client_error_unauthorized();
}

void UnsubscribeCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() >= 4) {
    client_print_unsubscribed(args[2].c_str(), args[3].c_str());
  } else if (status == 404 && args.size() >= 2) {
    client_error_unknown_team(args[1].c_str());
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void SubscribedCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() >= 3) {
    if (args[2] == "TEAM") {
      for (size_t i = 3; i + 2 < args.size(); i += 3) {
        client_print_teams(args[i].c_str(), args[i + 1].c_str(),
                           args[i + 2].c_str());
      }
    } else if (args[2] == "USER") {
      for (size_t i = 3; i + 2 < args.size(); i += 3) {
        try {
          client_print_users(args[i].c_str(), args[i + 1].c_str(),
                             std::stoi(args[i + 2]));
        } catch (...) {
        }
      }
    }
  } else if (status == 404 && args.size() >= 2) {
    client_error_unknown_team(args[1].c_str());
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void UseCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);

  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 404 && args.size() >= 3) {
    if (args[1] == "TEAM") {
      client_error_unknown_team(args[2].c_str());
    } else if (args[1] == "CHANNEL") {
      client_error_unknown_channel(args[2].c_str());
    } else if (args[1] == "THREAD") {
      client_error_unknown_thread(args[2].c_str());
    }
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void CreateCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);

  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 201 && args.size() > 2) {
    if (args[2] == "TEAM" && args.size() >= 6) {
      client_print_team_created(args[3].c_str(), args[4].c_str(),
                                args[5].c_str());
    } else if (args[2] == "CHANNEL" && args.size() >= 6) {
      client_print_channel_created(args[3].c_str(), args[4].c_str(),
                                   args[5].c_str());
    } else if (args[2] == "THREAD" && args.size() >= 8) {
      try {
        client_print_thread_created(args[3].c_str(), args[4].c_str(),
                                    std::stoll(args[5]), args[6].c_str(),
                                    args[7].c_str());
      } catch (...) {
      }
    } else if (args[2] == "REPLY" && args.size() >= 7) {
      try {
        client_print_reply_created(args[3].c_str(), args[4].c_str(),
                                   std::stoll(args[5]), args[6].c_str());
      } catch (...) {
      }
    }
  } else if (status == 409) {
    client_error_already_exist();
  } else if (status == 401 || status == 403) {
    client_error_unauthorized();
  }
}

void ListCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() > 2) {
    if (args[2] == "TEAM") {
      for (size_t i = 3; i + 2 < args.size(); i += 3) {
        client_print_teams(args[i].c_str(), args[i + 1].c_str(),
                           args[i + 2].c_str());
      }
    } else if (args[2] == "CHANNEL") {
      for (size_t i = 3; i + 2 < args.size(); i += 3) {
        client_team_print_channels(args[i].c_str(), args[i + 1].c_str(),
                                   args[i + 2].c_str());
      }
    } else if (args[2] == "THREAD") {
      for (size_t i = 3; i + 4 < args.size(); i += 5) {
        try {
          client_channel_print_threads(
              args[i].c_str(), args[i + 1].c_str(), std::stoll(args[i + 2]),
              args[i + 3].c_str(), args[i + 4].c_str());
        } catch (...) {
        }
      }
    } else if (args[2] == "REPLY") {
      for (size_t i = 3; i + 3 < args.size(); i += 4) {
        try {
          client_thread_print_replies(args[i].c_str(), args[i + 1].c_str(),
                                      std::stoll(args[i + 2]),
                                      args[i + 3].c_str());
        } catch (...) {
        }
      }
    }
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void InfoCommand::logCommand(const std::string &serverResponse) {
  int status = getStatusCode(serverResponse);
  std::vector<std::string> args = extractComplexArgs(serverResponse);

  if (status == 200 && args.size() > 2) {
    if (args[2] == "USER" && args.size() >= 6) {
      try {
        client_print_user(args[3].c_str(), args[4].c_str(), std::stoi(args[5]));
      } catch (...) {
      }
    } else if (args[2] == "TEAM" && args.size() >= 6) {
      client_print_team(args[3].c_str(), args[4].c_str(), args[5].c_str());
    } else if (args[2] == "CHANNEL" && args.size() >= 6) {
      client_print_channel(args[3].c_str(), args[4].c_str(), args[5].c_str());
    } else if (args[2] == "THREAD" && args.size() >= 8) {
      try {
        client_print_thread(args[3].c_str(), args[4].c_str(),
                            std::stoll(args[5]), args[6].c_str(),
                            args[7].c_str());
      } catch (...) {
      }
    }
  } else if (status == 401) {
    client_error_unauthorized();
  }
}

void handleEvent(const std::string &serverResponse) {
  std::vector<std::string> args = extractComplexArgs(serverResponse);
  if (args.size() < 2)
    return;

  if (args[1] == "received:") {
    if (args.size() >= 4)
      client_event_private_message_received(args[2].c_str(), args[3].c_str());
  } else if (args[1] == "thread_reply:") {
    if (args.size() >= 6)
      client_event_thread_reply_received(args[2].c_str(), args[3].c_str(),
                                         args[4].c_str(), args[5].c_str());
  } else if (args[1] == "team_created:") {
    if (args.size() >= 5)
      client_event_team_created(args[2].c_str(), args[3].c_str(),
                                args[4].c_str());
  } else if (args[1] == "channel_created:") {
    if (args.size() >= 5)
      client_event_channel_created(args[2].c_str(), args[3].c_str(),
                                   args[4].c_str());
  } else if (args[1] == "thread_created:") {
    if (args.size() >= 7) {
      try {
        client_event_thread_created(args[2].c_str(), args[3].c_str(),
                                    std::stoll(args[4]), args[5].c_str(),
                                    args[6].c_str());
      } catch (...) {
      }
    }
  }
}