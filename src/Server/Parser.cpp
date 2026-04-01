#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

static void skipSpaces(const std::string &line, size_t &i) {
  while (i < line.size() && std::isspace(line[i]))
    i++;
}

static std::string parseQuoted(const std::string &line, size_t &i) {
  if (i >= line.size() || line[i] != '"')
    throw std::runtime_error("400 Bad request, malformed command args");

  i++;
  size_t start = i;

  while (i < line.size() && line[i] != '"')
    i++;

  if (i >= line.size())
    throw std::runtime_error("400 Bad request, malformed command args");

  std::string value = line.substr(start, i - start);
  i++;
  return value;
}

std::vector<std::string> ParseArgs(const std::string &line) {
  std::vector<std::string> args;
  size_t i = 0;

  while (i < line.size()) {
    skipSpaces(line, i);
    if (i >= line.size())
      break;

    if (line[i] != '"')
      throw std::runtime_error("400 Bad request, malformed command args");

    args.push_back(parseQuoted(line, i));
  }
  return args;
}
