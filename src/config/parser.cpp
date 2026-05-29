#include "config/parser.hpp"

#include <toml++/toml.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tp::config {
namespace {

std::string lower_ascii(std::string_view value) {
    std::string result(value);
    std::ranges::transform(result, result.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return result;
}

[[nodiscard]] std::string_view trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

[[nodiscard]] std::pair<uint64_t, std::string_view> split_number_and_unit(std::string_view value) {
    value = trim(value);
    if (value.empty()) {
        throw std::invalid_argument("value must not be empty");
    }

    size_t unit_start = 0;
    while (unit_start < value.size() && std::isdigit(static_cast<unsigned char>(value[unit_start]))) {
        ++unit_start;
    }
    if (unit_start == 0) {
        throw std::invalid_argument("value must start with a non-negative integer");
    }

    const uint64_t number = std::stoull(std::string(value.substr(0, unit_start)));
    return {number, trim(value.substr(unit_start))};
}

[[nodiscard]] size_t checked_size_product(uint64_t value, uint64_t multiplier, std::string_view source) {
    if (value > std::numeric_limits<size_t>::max() / multiplier) {
        throw std::invalid_argument(std::string(source) + " is too large");
    }
    return static_cast<size_t>(value * multiplier);
}

[[nodiscard]] std::chrono::nanoseconds checked_duration_product(uint64_t value, uint64_t multiplier,
                                                                std::string_view source) {
    if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) / multiplier) {
        throw std::invalid_argument(std::string(source) + " is too large");
    }
    return std::chrono::nanoseconds(static_cast<int64_t>(value * multiplier));
}

template <class T>
[[nodiscard]] T required_value(const toml::table &table, std::string_view path) {
    std::optional<T> value = table.at_path(path).value<T>();
    if (!value.has_value()) {
        throw std::runtime_error("missing or invalid config value: " + std::string(path));
    }
    return *value;
}

template <class T>
[[nodiscard]] std::optional<T> optional_value(const toml::table &table, std::string_view path) {
    return table.at_path(path).value<T>();
}

[[nodiscard]] size_t parse_memory_size_value(const toml::table &table, std::string_view path) {
    if (std::optional<int64_t> value = optional_value<int64_t>(table, path); value.has_value()) {
        if (*value <= 0) {
            throw std::runtime_error(std::string(path) + " must be positive");
        }
        return static_cast<size_t>(*value);
    }

    const size_t result = parse_memory_size(required_value<std::string>(table, path));
    if (result == 0) {
        throw std::runtime_error(std::string(path) + " must be positive");
    }
    return result;
}

[[nodiscard]] Tape::LatencyConfig parse_latency_config(const toml::table &document) {
    Tape::LatencyConfig latency;
    constexpr std::string_view PREFIX = "tape_sorter.latency.";

    if (std::optional<bool> emulate_delays = optional_value<bool>(document, std::string(PREFIX) + "emulate_delays");
        emulate_delays.has_value()) {
        latency.enable_sleep_delays = *emulate_delays;
    }
    if (std::optional<std::string> read = optional_value<std::string>(document, std::string(PREFIX) + "read");
        read.has_value()) {
        latency.read_delay = parse_duration(*read);
    }
    if (std::optional<std::string> write = optional_value<std::string>(document, std::string(PREFIX) + "write");
        write.has_value()) {
        latency.write_delay = parse_duration(*write);
    }
    if (std::optional<std::string> move = optional_value<std::string>(document, std::string(PREFIX) + "move");
        move.has_value()) {
        latency.move_delay = parse_duration(*move);
    }
    if (std::optional<std::string> rewind = optional_value<std::string>(document, std::string(PREFIX) + "rewind");
        rewind.has_value()) {
        latency.rewind_delay = parse_duration(*rewind);
    }

    return latency;
}

}  // namespace

TapeSorterAlgorithm parse_tape_sorter_algorithm(std::string_view value) {
    const std::string normalized = lower_ascii(trim(value));
    if (normalized == "basic_external_merge") {
        return TapeSorterAlgorithm::BASIC_EXTERNAL_MERGE;
    }
    if (normalized == "k_way_external_merge") {
        return TapeSorterAlgorithm::K_WAY_EXTERNAL_MERGE;
    }
    if (normalized == "polyphase_merge" || normalized == "polyphase") {
        return TapeSorterAlgorithm::POLYPHASE_MERGE;
    }

    throw std::invalid_argument("unknown tape sorter algorithm: " + std::string(value));
}

size_t parse_memory_size(std::string_view value) {
    const auto [number, unit] = split_number_and_unit(value);
    const std::string normalized_unit = lower_ascii(unit);

    if (normalized_unit.empty() || normalized_unit == "b") {
        return checked_size_product(number, 1, value);
    }
    if (normalized_unit == "kb" || normalized_unit == "kib") {
        return checked_size_product(number, 1024, value);
    }
    if (normalized_unit == "mb" || normalized_unit == "mib") {
        return checked_size_product(number, 1024 * 1024, value);
    }
    if (normalized_unit == "gb" || normalized_unit == "gib") {
        return checked_size_product(number, 1024 * 1024 * 1024, value);
    }

    throw std::invalid_argument("unsupported memory size unit: " + std::string(unit));
}

std::chrono::nanoseconds parse_duration(std::string_view value) {
    const auto [number, unit] = split_number_and_unit(value);
    const std::string normalized_unit = lower_ascii(unit);

    if (normalized_unit == "ns") {
        return checked_duration_product(number, 1, value);
    }
    if (normalized_unit == "us") {
        return checked_duration_product(number, 1'000, value);
    }
    if (normalized_unit == "ms") {
        return checked_duration_product(number, 1'000'000, value);
    }
    if (normalized_unit == "s") {
        return checked_duration_product(number, 1'000'000'000, value);
    }

    throw std::invalid_argument("unsupported duration unit: " + std::string(unit));
}

Config parse_config(const std::filesystem::path &path) {
    toml::table document = toml::parse_file(path.string());

    if (!document["tape_sorter"].is_table()) {
        throw std::runtime_error("missing config table: tape_sorter");
    }

    Config config;
    config.tape_sorter.memory_limit_bytes = parse_memory_size_value(document, "tape_sorter.memory_limit");
    config.tape_sorter.algorithm =
        parse_tape_sorter_algorithm(required_value<std::string>(document, "tape_sorter.algorithm"));
    config.tape_sorter.latency = parse_latency_config(document);

    if (std::optional<int64_t> merge_order =
            optional_value<int64_t>(document, "tape_sorter.k_way_external_merge.merge_order");
        merge_order.has_value()) {
        if (*merge_order < 2) {
            throw std::runtime_error("tape_sorter.k_way_external_merge.merge_order must be at least 2");
        }
        config.tape_sorter.k_way_external_merge.merge_order = static_cast<size_t>(*merge_order);
    }

    return config;
}

}  // namespace tp::config
