#pragma once

#include "tape/tape.hpp"
#include "tape/tape_sorter.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tp {

class ExternalKWayMergeTapeSorter final : public TapeSorter {
   public:
    ExternalKWayMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory, size_t memory_limit_bytes,
                                size_t merge_order);

    void sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) override;

   private:
    struct SortedRun {
        std::unique_ptr<Tape> tape;
        size_t value_count{};
    };

    struct HeapEntry {
        int32_t value{};
        size_t run_index{};
    };

    struct MinValueFirst {
        [[nodiscard]] bool operator()(const HeapEntry &left, const HeapEntry &right) const;
    };

    [[nodiscard]] size_t max_chunk_value_count(const Tape &input_tape) const;
    [[nodiscard]] std::vector<SortedRun> create_initial_runs(Tape &input_tape, size_t max_chunk_value_count) const;
    [[nodiscard]] std::vector<SortedRun> merge_pass(std::vector<SortedRun> runs) const;
    [[nodiscard]] SortedRun create_empty_run(size_t value_count) const;
    [[nodiscard]] static size_t total_value_count(std::span<const SortedRun> runs);

    static void write_values_to_tape(std::span<const int32_t> values, Tape &output_tape);
    static void merge_runs_to_tape(std::span<SortedRun> runs, Tape &output_tape);
    static void copy_run(Tape &source_tape, size_t value_count, Tape &output_tape);

    size_t merge_order_{};
};

}  // namespace tp
