#pragma once

#include "tape/tape.hpp"

#include <cstddef>

namespace tp::config {

enum class TapeSorterAlgorithm {
    BASIC_EXTERNAL_MERGE,
    K_WAY_EXTERNAL_MERGE,
    POLYPHASE_MERGE,
};

struct KWayExternalMergeConfig {
    size_t merge_order{16};
};

struct TapeSorterConfig {
    size_t memory_limit_bytes{256 * 1024 * 1024};
    TapeSorterAlgorithm algorithm{TapeSorterAlgorithm::BASIC_EXTERNAL_MERGE};
    Tape::LatencyConfig latency{};
    KWayExternalMergeConfig k_way_external_merge{};
};

struct Config {
    TapeSorterConfig tape_sorter{};
};

}  // namespace tp::config
