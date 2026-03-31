#pragma once

#include <cstddef>
#include <cstdint>

constexpr std::size_t MAX_NAME_LENGTH = 32;
constexpr std::size_t MAX_DESCRIPTION_LENGTH = 255;
constexpr std::size_t MAX_BODY_LENGTH = 512;
constexpr std::size_t UUID_STR_LEN = 37;

#pragma pack(push, 1)

struct User {
  char uuid[UUID_STR_LEN];
  char name[MAX_NAME_LENGTH];
};

struct Team {
  char uuid[UUID_STR_LEN];
  char name[MAX_NAME_LENGTH];
  char description[MAX_DESCRIPTION_LENGTH];
};

struct Channel {
  char uuid[UUID_STR_LEN];
  char team_uuid[UUID_STR_LEN];
  char name[MAX_NAME_LENGTH];
  char description[MAX_DESCRIPTION_LENGTH];
};

struct Thread {
  char uuid[UUID_STR_LEN];
  char channel_uuid[UUID_STR_LEN];
  char creator_uuid[UUID_STR_LEN];
  char title[MAX_NAME_LENGTH];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Reply {
  char uuid[UUID_STR_LEN];
  char thread_uuid[UUID_STR_LEN];
  char creator_uuid[UUID_STR_LEN];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Message {
  char uuid[UUID_STR_LEN];
  char sender_uuid[UUID_STR_LEN];
  char receiver_uuid[UUID_STR_LEN];
  char body[MAX_BODY_LENGTH];
  uint64_t timestamp;
};

struct Subscription {
  char user_uuid[UUID_STR_LEN];
  char team_uuid[UUID_STR_LEN];
};

#pragma pack(pop)
