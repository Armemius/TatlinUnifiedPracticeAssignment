#pragma once

#include "tape/tape.hpp"
#include "tape/tape_sorter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <vector>

namespace tp {

class PolyphaseMergeTapeSorter final : public TapeSorter {
   public:
    PolyphaseMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory, size_t memory_limit_bytes);

    void sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) override;

   private:
    struct SortedRun {
        size_t start_position{};
        size_t value_count{};
        bool dummy{};
    };

    struct WorkTape {
        std::unique_ptr<Tape> tape;
        std::deque<SortedRun> runs;
    };

    struct InitialRunDistribution {
        std::array<size_t, 2> real_runs{};
        size_t dummy_runs{};
    };

    using WorkTapes = std::array<WorkTape, 3>;

    [[nodiscard]] size_t max_chunk_value_count(const Tape &input_tape) const;
    [[nodiscard]] static InitialRunDistribution initial_run_distribution(size_t input_value_count,
                                                                  size_t max_chunk_value_count);
    [[nodiscard]] WorkTapes create_initial_work_tapes(Tape &input_tape, size_t max_chunk_value_count) const;
    [[nodiscard]] static size_t total_run_count(const WorkTapes &tapes);
    [[nodiscard]] static size_t real_run_count(const WorkTapes &tapes);
    [[nodiscard]] static size_t empty_tape_index(const WorkTapes &tapes);
    [[nodiscard]] static size_t phase_output_value_count(const WorkTape &left, const WorkTape &right,
                                                         size_t merge_count);
    [[nodiscard]] WorkTape merge_phase(WorkTape &left, WorkTape &right, size_t merge_count) const;
    [[nodiscard]] static std::vector<int32_t> read_sorted_chunk(Tape &input_tape, size_t max_value_count);

    static void write_values_to_work_tape(std::span<const int32_t> values, WorkTape &output_tape);
    static void merge_runs_to_work_tape(Tape &left_tape, SortedRun left_run, Tape &right_tape, SortedRun right_run,
                                        WorkTape &output_tape);
    static void copy_run_to_work_tape(Tape &source_tape, SortedRun source_run, WorkTape &output_tape);
    static void copy_run(Tape &source_tape, SortedRun source_run, Tape &output_tape);
    static void move_to_position(Tape &tape, size_t position);
};

}  // namespace tp
