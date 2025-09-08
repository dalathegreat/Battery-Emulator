#include "../../Software/src/devboard/utils/types.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool ends_with(const std::string& str, const std::string& suffix);
std::vector<std::string> split(const std::string& text, char sep);
std::string snake_case_to_camel_case(const std::string& str);

std::vector<CAN_frame> parse_can_log_file(const fs::path& filePath);
