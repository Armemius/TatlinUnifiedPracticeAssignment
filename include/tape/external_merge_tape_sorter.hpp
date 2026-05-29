#pragma once

#include "tape/tape.hpp"
#include "tape/tape_sorter.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tp {

class ExternalMergeTapeSorter final : public TapeSorter {
   public:
    ExternalMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory, size_t memory_limit_bytes);

    void sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) override;

   private:
    struct SortedRun {
        std::unique_ptr<Tape> tape;
        size_t value_count{};
    };

    [[nodiscard]] size_t max_chunk_value_count(const Tape &input_tape) const;
    [[nodiscard]] SortedRun create_empty_run(size_t tape_size) const;
    [[nodiscard]] static std::vector<int32_t> read_sorted_chunk(Tape &input_tape, size_t max_value_count);

    static void merge_run_with_chunk(Tape &run_tape, size_t run_value_count, std::span<const int32_t> sorted_chunk,
                                     Tape &output_tape);
    static void copy_run(Tape &source_tape, size_t value_count, Tape &output_tape);
};

}  // namespace tp
