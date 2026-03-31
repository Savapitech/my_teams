#include "Database.hpp"
#include "Utils/Logger.hpp"

#include <fstream>
#include <iostream>

template <typename T>
static void saveVector(std::ofstream &ofs, const std::vector<T> &vec) {
  uint64_t size = vec.size();
  ofs.write(reinterpret_cast<const char *>(&size), sizeof(size));
  if (size > 0)
    ofs.write(reinterpret_cast<const char *>(vec.data()), size * sizeof(T));
}

template <typename T>
static void loadVector(std::ifstream &ifs, std::vector<T> &vec) {
  uint64_t size = 0;
  if (ifs.read(reinterpret_cast<char *>(&size), sizeof(size))) {
    vec.resize(size);
    if (size > 0)
      ifs.read(reinterpret_cast<char *>(vec.data()), size * sizeof(T));
  }
}

void Database::save(const std::string &filename) const {
  std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);

  if (!ofs.is_open()) {
    LOG_ERROR("Failed to open database file for saving: " + filename);
    return;
  }

  saveVector(ofs, _users);
  saveVector(ofs, _teams);
  saveVector(ofs, _channels);
  saveVector(ofs, _threads);
  saveVector(ofs, _replies);
  saveVector(ofs, _messages);
  saveVector(ofs, _subscriptions);

  ofs.close();
  LOG_INFO("Database saved successfully to " + filename);
}

void Database::load(const std::string &filename) {
  std::ifstream ifs(filename, std::ios::binary);

  if (!ifs.is_open()) {
    LOG_INFO("Database file not found, file name [" + filename + "]");
    return;
  }

  try {
    loadVector(ifs, _users);
    loadVector(ifs, _teams);
    loadVector(ifs, _channels);
    loadVector(ifs, _threads);
    loadVector(ifs, _replies);
    loadVector(ifs, _messages);
    loadVector(ifs, _subscriptions);
  } catch (const std::exception &e) {
    LOG_ERROR(("Error loading database: " + std::string(e.what())).c_str());
    return;
  }

  ifs.close();
  LOG_INFO(("Database loaded successfully from " + filename).c_str());
  LOG_DEBUG("Database size [" + std::to_string(_users.size()) + "]");
}
