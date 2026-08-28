#pragma once

#include <string>
#include <vector>
#include <filesystem>

std::vector<std::string> toml_parse(std::filesystem::path path);
std::filesystem::path find_file(const std::string& file);
