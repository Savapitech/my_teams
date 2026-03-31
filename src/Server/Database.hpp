#pragma once

#include <string>
#include <vector>

#include "Models.hpp"

class Database {
private:
  std::vector<User> _users;
  std::vector<Team> _teams;
  std::vector<Channel> _channels;
  std::vector<Thread> _threads;
  std::vector<Reply> _replies;
  std::vector<Message> _messages;
  std::vector<Subscription> _subscriptions;

public:
  Database() = default;
  ~Database() = default;

  void load(const std::string &filename);
  void save(const std::string &filename) const;

  std::vector<Channel> &getChannels() { return _channels; }
  std::vector<Message> &getMessages() { return _messages; }
  std::vector<Reply> &getReplies() { return _replies; }
  std::vector<Subscription> &getSubscriptions() { return _subscriptions; }
  std::vector<Team> &getTeams() { return _teams; }
  std::vector<Thread> &getThreads() { return _threads; }
  std::vector<User> &getUsers() { return _users; }

  const std::vector<Channel> &getChannels() const { return _channels; }
  const std::vector<Message> &getMessages() const { return _messages; }
  const std::vector<Reply> &getReplies() const { return _replies; }
  const std::vector<Subscription> &getSubscriptions() const {
    return _subscriptions;
  }
  const std::vector<Team> &getTeams() const { return _teams; }
  const std::vector<Thread> &getThreads() const { return _threads; }
  const std::vector<User> &getUsers() const { return _users; }
};
