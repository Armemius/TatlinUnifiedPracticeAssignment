#pragma once

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace tp::detail {

class SortProgressLogger final {
   public:
    SortProgressLogger(std::string operation, size_t total_value_count)
        : operation_(std::move(operation)),
          total_value_count_(total_value_count),
          progress_interval_(calculate_progress_interval(total_value_count)),
          next_logged_value_count_(progress_interval_) {}

    void mark(size_t processed_value_count) {
        if (total_value_count_ == 0 || final_progress_logged_) {
            return;
        }

        processed_value_count = std::min(processed_value_count, total_value_count_);
        if (processed_value_count < next_logged_value_count_ && processed_value_count != total_value_count_) {
            return;
        }

        const double percentage =
            static_cast<double>(processed_value_count) * 100.0 / static_cast<double>(total_value_count_);
        spdlog::info("{} progress: {}/{} values ({:.1f}%)", operation_, processed_value_count, total_value_count_,
                     percentage);

        while (next_logged_value_count_ <= processed_value_count && next_logged_value_count_ < total_value_count_) {
            next_logged_value_count_ += progress_interval_;
        }

        if (processed_value_count == total_value_count_) {
            final_progress_logged_ = true;
        }
    }

   private:
    static size_t calculate_progress_interval(size_t total_value_count) {
        constexpr size_t TARGET_PROGRESS_LOG_COUNT = 10;
        constexpr size_t MIN_PROGRESS_INTERVAL = 1'000'000;
        if (total_value_count == 0) {
            return 1;
        }
        return std::max(MIN_PROGRESS_INTERVAL,
                        (total_value_count + TARGET_PROGRESS_LOG_COUNT - 1) / TARGET_PROGRESS_LOG_COUNT);
    }

    std::string operation_;
    size_t total_value_count_{};
    size_t progress_interval_{};
    size_t next_logged_value_count_{};
    bool final_progress_logged_{false};
};

}  // namespace tp::detail
