#include <gtest/gtest.h>

#include "config/parser.hpp"
#include "utils/tmp_directory.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace tp::config {
namespace {

std::filesystem::path write_config(utils::TmpDirectory &directory, std::string_view contents) {
    const std::filesystem::path path = directory.path() / "config.toml";
    std::ofstream stream(path);
    stream << contents;
    return path;
}

TEST(ConfigParserTests, ParsesSampleConfig) {
    Config config = parse_config(std::filesystem::path(PROJECT_SOURCE_DIR) / "assets/config.toml");

    ASSERT_EQ(config.tape_sorter.memory_limit_bytes, 256 * 1024 * 1024);
    ASSERT_EQ(config.tape_sorter.algorithm, TapeSorterAlgorithm::BASIC_EXTERNAL_MERGE);
    ASSERT_FALSE(config.tape_sorter.latency.enable_sleep_delays);
    ASSERT_EQ(config.tape_sorter.latency.read_delay, std::chrono::microseconds(1));
    ASSERT_EQ(config.tape_sorter.latency.write_delay, std::chrono::microseconds(2));
    ASSERT_EQ(config.tape_sorter.latency.move_delay, std::chrono::microseconds(100));
    ASSERT_EQ(config.tape_sorter.latency.rewind_delay, std::chrono::milliseconds(1));
    ASSERT_EQ(config.tape_sorter.k_way_external_merge.merge_order, 16);
}

TEST(ConfigParserTests, ParsesKWaySorterConfig) {
    utils::TmpDirectory directory;
    const std::filesystem::path path = write_config(directory, R"toml(
[tape_sorter]
memory_limit = "4kb"
algorithm = "k_way_external_merge"

[tape_sorter.latency]
emulate_delays = true
read = "7us"

[tape_sorter.k_way_external_merge]
merge_order = 3
)toml");

    Config config = parse_config(path);

    ASSERT_EQ(config.tape_sorter.memory_limit_bytes, 4 * 1024);
    ASSERT_EQ(config.tape_sorter.algorithm, TapeSorterAlgorithm::K_WAY_EXTERNAL_MERGE);
    ASSERT_TRUE(config.tape_sorter.latency.enable_sleep_delays);
    ASSERT_EQ(config.tape_sorter.latency.read_delay, std::chrono::microseconds(7));
    ASSERT_EQ(config.tape_sorter.k_way_external_merge.merge_order, 3);
}

TEST(ConfigParserTests, ParsesPolyphaseSorterConfig) {
    utils::TmpDirectory directory;
    const std::filesystem::path path = write_config(directory, R"toml(
[tape_sorter]
memory_limit = "4kb"
algorithm = "polyphase_merge"
)toml");

    Config config = parse_config(path);

    ASSERT_EQ(config.tape_sorter.memory_limit_bytes, 4 * 1024);
    ASSERT_EQ(config.tape_sorter.algorithm, TapeSorterAlgorithm::POLYPHASE_MERGE);
}

TEST(ConfigParserTests, ParsesPolyphaseSorterAlias) {
    ASSERT_EQ(parse_tape_sorter_algorithm("polyphase"), TapeSorterAlgorithm::POLYPHASE_MERGE);
}

TEST(ConfigParserTests, ParsesMemorySizeUnits) {
    ASSERT_EQ(parse_memory_size("42"), 42);
    ASSERT_EQ(parse_memory_size("2kb"), 2 * 1024);
    ASSERT_EQ(parse_memory_size("3mb"), 3 * 1024 * 1024);
    ASSERT_EQ(parse_memory_size("4gib"), 4ULL * 1024 * 1024 * 1024);
}

TEST(ConfigParserTests, ParsesDurationUnits) {
    ASSERT_EQ(parse_duration("42ns"), std::chrono::nanoseconds(42));
    ASSERT_EQ(parse_duration("3us"), std::chrono::microseconds(3));
    ASSERT_EQ(parse_duration("5ms"), std::chrono::milliseconds(5));
    ASSERT_EQ(parse_duration("2s"), std::chrono::seconds(2));
}

TEST(ConfigParserTests, RejectsUnknownAlgorithm) {
    ASSERT_THROW(
        {
            const TapeSorterAlgorithm algorithm = parse_tape_sorter_algorithm("unknown");
            static_cast<void>(algorithm);
        },
        std::invalid_argument);
}

TEST(ConfigParserTests, RejectsInvalidMergeOrder) {
    utils::TmpDirectory directory;
    const std::filesystem::path path = write_config(directory, R"toml(
[tape_sorter]
memory_limit = "1mb"
algorithm = "k_way_external_merge"

[tape_sorter.k_way_external_merge]
merge_order = 1
)toml");

    ASSERT_THROW(
        {
            const Config config = parse_config(path);
            static_cast<void>(config);
        },
        std::runtime_error);
}

}  // namespace
}  // namespace tp::config
