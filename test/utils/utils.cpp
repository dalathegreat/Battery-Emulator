#include "utils.h"

#include <fstream>

namespace fs = std::filesystem;

bool ends_with(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> split(const std::string& text, char sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

void print_frame(const CAN_frame& frame) {
  std::cout << "ID: " << std::hex << frame.ID << ", DLC: " << (int)frame.DLC << ", Data: ";
  for (int i = 0; i < frame.DLC; ++i) {
    std::cout << std::hex << (int)frame.data.u8[i] << " ";
  }
  std::cout << std::dec << "\n";
}

std::string snake_case_to_camel_case(const std::string& str) {
  std::string result;
  bool toUpper = false;
  for (char ch : str) {
    if (ch == '_') {
      toUpper = true;
    } else if (ch < '0' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < 'a') || ch > 'z') {
      // skip non-alphanumeric characters
      toUpper = true;
    } else {
      if (toUpper) {
        result += toupper(ch);
        toUpper = false;
      } else {
        result += ch;
      }
    }
  }
  return result;
}

CAN_frame parse_can_log_line(const std::string& logLine) {
  std::stringstream ss(logLine);
  CAN_frame frame = {};
  char dummy;

  double timestamp;
  std::string interfaceName;

  // timestamp and interface name are parsed but not used
  ss >> dummy >> timestamp >> dummy;
  ss >> interfaceName;

  // parse hexadecimal CAN ID
  ss >> std::hex >> frame.ID;
  if (ss.fail()) {
    throw std::runtime_error("Invalid format: Failed to parse CAN ID.");
  }
  // check whether the ID is in the extended range
  frame.ext_ID = (frame.ID > 0x7FF);

  // parse the data length
  int dlc_val;
  ss >> dummy;  // Consume '['
  if (ss.fail() || dummy != '[') {
    throw std::runtime_error("Invalid format: Missing opening bracket for data length.");
  }
  ss >> dlc_val;
  frame.DLC = static_cast<uint8_t>(dlc_val);
  ss >> dummy;  // Consume ']'
  if (ss.fail() || dummy != ']') {
    throw std::runtime_error("Invalid format: Missing closing bracket for data length.");
  }
  // crudely assume CAN FD if DLC > 8
  frame.FD = (frame.DLC > 8);

  // parse the actual data bytes
  unsigned int byte;
  for (int i = 0; i < frame.DLC; ++i) {
    ss >> std::hex >> byte;
    if (ss.fail()) {
      throw std::runtime_error("Fewer data bytes than specified by data length.");
    }
    frame.data.u8[i] = static_cast<uint8_t>(byte);
  }

  return frame;
}

std::vector<CAN_frame> parse_can_log_file(const fs::path& filePath) {
  std::ifstream logFile(filePath);
  if (!logFile.is_open()) {
    std::cerr << "Error: Could not open file " << filePath << std::endl;
    return {};
  }

  std::vector<CAN_frame> frames;
  std::string line;
  int lineNumber = 0;

  // read the file line by line
  while (std::getline(logFile, line)) {
    lineNumber++;
    if (line.empty()) {
      continue;
    }

    if (line[0] == '#' || line[0] == ';') {
      continue;  // skip comment lines
    }

    try {
      frames.push_back(parse_can_log_line(line));
    } catch (const std::runtime_error& e) {
      std::cerr << "Warning: Skipping malformed line " << lineNumber << " in " << filePath.filename()
                << ". Reason: " << e.what() << std::endl;
    }
  }

  return frames;
}
