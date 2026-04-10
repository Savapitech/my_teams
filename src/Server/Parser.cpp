#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<std::string> ParseArgs(const std::string &line) {
  std::vector<std::string> args;
  std::stringstream ss(line);
  std::string word;

  while (ss >> word) {
    args.push_back(word);
  }

  return args;
}