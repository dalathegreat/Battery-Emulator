#include "../../Software/src/devboard/utils/types.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool ends_with(const std::string& str, const std::string& suffix);

std::vector<CAN_frame> parse_can_log_file(const fs::path& filePath);
