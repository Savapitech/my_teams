#pragma once

#include <cstddef>
#include <cstdint>

constexpr std::size_t MAX_NAME_LENGTH = 32;
constexpr std::size_t MAX_DESCRIPTION_LENGTH = 255;
constexpr std::size_t MAX_BODY_LENGTH = 512;
constexpr std::size_t MAX_UUID_LENGTH = 37;

#pragma pack(push, 1)

struct User {
  char uuid[MAX_UUID_LENGTH];
  char name[MAX_NAME_LENGTH];
};

struct Team {
  char uuid[MAX_UUID_LENGTH];
  char name[MAX_NAME_LENGTH];
  char description[MAX_DESCRIPTION_LENGTH];
};

struct Channel {
  char uuid[MAX_UUID_LENGTH];
  char team_uuid[MAX_UUID_LENGTH];
  char name[MAX_NAME_LENGTH];
  char description[MAX_DESCRIPTION_LENGTH];
};

struct Thread {
  char uuid[MAX_UUID_LENGTH];
  char channel_uuid[MAX_UUID_LENGTH];
  char creator_uuid[MAX_UUID_LENGTH];
  char title[MAX_NAME_LENGTH];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Reply {
  char uuid[MAX_UUID_LENGTH];
  char thread_uuid[MAX_UUID_LENGTH];
  char creator_uuid[MAX_UUID_LENGTH];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Message {
  char uuid[MAX_UUID_LENGTH];
  char sender_uuid[MAX_UUID_LENGTH];
  char receiver_uuid[MAX_UUID_LENGTH];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Subscription {
  char user_uuid[MAX_UUID_LENGTH];
  char team_uuid[MAX_UUID_LENGTH];
};

#pragma pack(pop)
