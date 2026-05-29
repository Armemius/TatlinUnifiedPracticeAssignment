#pragma once

#include "config/config.hpp"

#include <chrono>
#include <filesystem>
#include <string_view>

namespace tp::config {

[[nodiscard]] Config parse_config(const std::filesystem::path &path);

[[nodiscard]] size_t parse_memory_size(std::string_view value);

[[nodiscard]] std::chrono::nanoseconds parse_duration(std::string_view value);

[[nodiscard]] TapeSorterAlgorithm parse_tape_sorter_algorithm(std::string_view value);

}  // namespace tp::config
