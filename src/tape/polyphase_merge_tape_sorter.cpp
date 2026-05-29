#include "tape/polyphase_merge_tape_sorter.hpp"

#include "sort_progress_logger.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tp {

namespace {

[[nodiscard]] bool has_unread_values(const Tape &tape) {
    return tape.position() < tape.size();
}

[[nodiscard]] size_t run_count_for_input(size_t value_count, size_t max_chunk_value_count) {
    if (value_count == 0) {
        return 0;
    }
    return (value_count + max_chunk_value_count - 1) / max_chunk_value_count;
}

[[nodiscard]] size_t run_value_count(size_t run_index, size_t input_value_count, size_t max_chunk_value_count) {
    const size_t run_start = run_index * max_chunk_value_count;
    return std::min(max_chunk_value_count, input_value_count - run_start);
}

}  // namespace

PolyphaseMergeTapeSorter::PolyphaseMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory,
                                                   size_t memory_limit_bytes)
    : TapeSorter(std::move(temporary_tape_factory), memory_limit_bytes) {}

void PolyphaseMergeTapeSorter::sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) {
    input_tape->rewind();
    output_tape->rewind();

    const size_t max_chunk_size = max_chunk_value_count(*input_tape);
    spdlog::info("polyphase merge sort started: input_size={}, memory_limit_bytes={}, max_chunk_size={}",
                 input_tape->size(), memory_limit_bytes_, max_chunk_size);

    WorkTapes tapes = create_initial_work_tapes(*input_tape, max_chunk_size);
    spdlog::info("polyphase merge sort initial runs created: run_count={}, real_run_count={}", total_run_count(tapes),
                 real_run_count(tapes));

    size_t pass_index = 0;
    while (total_run_count(tapes) > 1) {
        const size_t output_index = empty_tape_index(tapes);
        const size_t left_index = (output_index + 1) % tapes.size();
        const size_t right_index = (output_index + 2) % tapes.size();
        const size_t merge_count = std::min(tapes[left_index].runs.size(), tapes[right_index].runs.size());

        spdlog::info(
            "polyphase merge sort pass started: pass={}, left_tape={}, right_tape={}, output_tape={}, "
            "merge_count={}",
            pass_index, left_index, right_index, output_index, merge_count);

        tapes[output_index] = merge_phase(tapes[left_index], tapes[right_index], merge_count);

        spdlog::info("polyphase merge sort pass finished: pass={}, run_count={}, real_run_count={}", pass_index,
                     total_run_count(tapes), real_run_count(tapes));
        ++pass_index;
    }

    for (WorkTape &tape : tapes) {
        if (!tape.runs.empty() && !tape.runs.front().dummy) {
            copy_run(*tape.tape, tape.runs.front(), *output_tape);
            break;
        }
    }

    spdlog::info("polyphase merge sort finished: passes={}, output_size={}", pass_index, input_tape->size());
}

size_t PolyphaseMergeTapeSorter::max_chunk_value_count(const Tape &input_tape) const {
    const size_t max_value_count = memory_limit_bytes_ / sizeof(int32_t);
    if (max_value_count == 0 && input_tape.size() != 0) {
        throw std::invalid_argument("memory limit is too small to hold one tape element");
    }
    return max_value_count;
}

PolyphaseMergeTapeSorter::InitialRunDistribution PolyphaseMergeTapeSorter::initial_run_distribution(
    size_t input_value_count, size_t max_chunk_value_count) {
    const size_t real_run_total = run_count_for_input(input_value_count, max_chunk_value_count);
    if (real_run_total == 0) {
        return {};
    }

    size_t previous = 0;
    size_t current = 1;
    while (previous + current < real_run_total) {
        const size_t next = previous + current;
        previous = current;
        current = next;
    }

    const size_t dummy_runs = previous + current - real_run_total;
    return {.real_runs = {current - dummy_runs, previous}, .dummy_runs = dummy_runs};
}

PolyphaseMergeTapeSorter::WorkTapes PolyphaseMergeTapeSorter::create_initial_work_tapes(
    Tape &input_tape, size_t max_chunk_value_count) const {
    const InitialRunDistribution distribution = initial_run_distribution(input_tape.size(), max_chunk_value_count);
    WorkTapes tapes;

    std::array<size_t, 2> tape_value_counts{};
    size_t run_index = 0;
    for (size_t tape_index = 0; tape_index < tape_value_counts.size(); ++tape_index) {
        for (size_t run_on_tape = 0; run_on_tape < distribution.real_runs[tape_index]; ++run_on_tape) {
            tape_value_counts[tape_index] += run_value_count(run_index, input_tape.size(), max_chunk_value_count);
            ++run_index;
        }
    }

    tapes[0].tape = temporary_tape_factory_->create_temporary(tape_value_counts[0]);
    tapes[1].tape = temporary_tape_factory_->create_temporary(tape_value_counts[1]);
    tapes[2].tape = temporary_tape_factory_->create_temporary(0);

    for (size_t tape_index = 0; tape_index < distribution.real_runs.size(); ++tape_index) {
        WorkTape &work_tape = tapes[tape_index];
        work_tape.tape->rewind();

        for (size_t run_on_tape = 0; run_on_tape < distribution.real_runs[tape_index]; ++run_on_tape) {
            const size_t chunk_start_position = input_tape.position();
            const std::vector<int32_t> sorted_chunk = read_sorted_chunk(input_tape, max_chunk_value_count);
            write_values_to_work_tape(sorted_chunk, work_tape);
            spdlog::info("polyphase merge sort initial run created: tape={}, run={}, start_position={}, value_count={}",
                         tape_index, run_on_tape, chunk_start_position, sorted_chunk.size());
        }
    }

    for (size_t dummy_index = 0; dummy_index < distribution.dummy_runs; ++dummy_index) {
        tapes[0].runs.push_back({.dummy = true});
    }

    spdlog::info(
        "polyphase merge sort Fibonacci distribution prepared: left_real_runs={}, right_real_runs={}, dummy_runs={}",
        distribution.real_runs[0], distribution.real_runs[1], distribution.dummy_runs);

    return tapes;
}

size_t PolyphaseMergeTapeSorter::total_run_count(const WorkTapes &tapes) {
    return std::accumulate(tapes.begin(), tapes.end(), size_t{},
                           [](size_t total, const WorkTape &tape) { return total + tape.runs.size(); });
}

size_t PolyphaseMergeTapeSorter::real_run_count(const WorkTapes &tapes) {
    return std::accumulate(tapes.begin(), tapes.end(), size_t{}, [](size_t total, const WorkTape &tape) {
        return total + std::ranges::count_if(tape.runs, [](const SortedRun &run) { return !run.dummy; });
    });
}

size_t PolyphaseMergeTapeSorter::empty_tape_index(const WorkTapes &tapes) {
    for (size_t tape_index = 0; tape_index < tapes.size(); ++tape_index) {
        if (tapes[tape_index].runs.empty()) {
            return tape_index;
        }
    }

    throw std::logic_error("polyphase merge sort requires one empty work tape before every pass");
}

size_t PolyphaseMergeTapeSorter::phase_output_value_count(const WorkTape &left, const WorkTape &right,
                                                          size_t merge_count) {
    size_t value_count = 0;
    for (size_t run_index = 0; run_index < merge_count; ++run_index) {
        value_count += left.runs[run_index].value_count + right.runs[run_index].value_count;
    }
    return value_count;
}

PolyphaseMergeTapeSorter::WorkTape PolyphaseMergeTapeSorter::merge_phase(WorkTape &left, WorkTape &right,
                                                                         size_t merge_count) const {
    WorkTape output;
    output.tape = temporary_tape_factory_->create_temporary(phase_output_value_count(left, right, merge_count));
    output.tape->rewind();

    for (size_t merged_runs = 0; merged_runs < merge_count; ++merged_runs) {
        const SortedRun left_run = left.runs.front();
        const SortedRun right_run = right.runs.front();
        left.runs.pop_front();
        right.runs.pop_front();

        if (left_run.dummy && right_run.dummy) {
            output.runs.push_back({.dummy = true});
        } else if (left_run.dummy) {
            copy_run_to_work_tape(*right.tape, right_run, output);
        } else if (right_run.dummy) {
            copy_run_to_work_tape(*left.tape, left_run, output);
        } else {
            merge_runs_to_work_tape(*left.tape, left_run, *right.tape, right_run, output);
        }
    }

    return output;
}

std::vector<int32_t> PolyphaseMergeTapeSorter::read_sorted_chunk(Tape &input_tape, size_t max_value_count) {
    std::vector<int32_t> chunk;
    chunk.reserve(max_value_count);
    detail::SortProgressLogger progress("polyphase merge sort chunk read IO",
                                        std::min(max_value_count, input_tape.size() - input_tape.position()));

    while (chunk.size() < max_value_count && has_unread_values(input_tape)) {
        chunk.push_back(input_tape.read());
        input_tape.next();
        progress.mark(chunk.size());
    }

    std::ranges::sort(chunk);
    return chunk;
}

void PolyphaseMergeTapeSorter::write_values_to_work_tape(std::span<const int32_t> values, WorkTape &output_tape) {
    const size_t start_position = output_tape.tape->position();
    detail::SortProgressLogger progress("polyphase merge sort initial run write IO", values.size());

    for (size_t value_index = 0; value_index < values.size(); ++value_index) {
        output_tape.tape->write(values[value_index]);
        output_tape.tape->next();
        progress.mark(value_index + 1);
    }

    output_tape.runs.push_back({.start_position = start_position, .value_count = values.size()});
}

void PolyphaseMergeTapeSorter::merge_runs_to_work_tape(Tape &left_tape, SortedRun left_run, Tape &right_tape,
                                                       SortedRun right_run, WorkTape &output_tape) {
    move_to_position(left_tape, left_run.start_position);
    move_to_position(right_tape, right_run.start_position);

    const size_t start_position = output_tape.tape->position();
    size_t copied_from_left{};
    size_t copied_from_right{};
    detail::SortProgressLogger progress("polyphase merge sort run merge IO",
                                        left_run.value_count + right_run.value_count);

    while (copied_from_left < left_run.value_count && copied_from_right < right_run.value_count) {
        const int32_t left_value = left_tape.read();
        const int32_t right_value = right_tape.read();

        if (left_value <= right_value) {
            output_tape.tape->write(left_value);
            left_tape.next();
            ++copied_from_left;
        } else {
            output_tape.tape->write(right_value);
            right_tape.next();
            ++copied_from_right;
        }

        output_tape.tape->next();
        progress.mark(copied_from_left + copied_from_right);
    }

    while (copied_from_left < left_run.value_count) {
        output_tape.tape->write(left_tape.read());
        left_tape.next();
        output_tape.tape->next();
        ++copied_from_left;
        progress.mark(copied_from_left + copied_from_right);
    }

    while (copied_from_right < right_run.value_count) {
        output_tape.tape->write(right_tape.read());
        right_tape.next();
        output_tape.tape->next();
        ++copied_from_right;
        progress.mark(copied_from_left + copied_from_right);
    }

    output_tape.runs.push_back(
        {.start_position = start_position, .value_count = left_run.value_count + right_run.value_count});
}

void PolyphaseMergeTapeSorter::copy_run_to_work_tape(Tape &source_tape, SortedRun source_run, WorkTape &output_tape) {
    move_to_position(source_tape, source_run.start_position);

    const size_t start_position = output_tape.tape->position();
    detail::SortProgressLogger progress("polyphase merge sort dummy run copy IO", source_run.value_count);

    for (size_t copied_values = 0; copied_values < source_run.value_count; ++copied_values) {
        output_tape.tape->write(source_tape.read());
        source_tape.next();
        output_tape.tape->next();
        progress.mark(copied_values + 1);
    }

    output_tape.runs.push_back({.start_position = start_position, .value_count = source_run.value_count});
}

void PolyphaseMergeTapeSorter::copy_run(Tape &source_tape, SortedRun source_run, Tape &output_tape) {
    move_to_position(source_tape, source_run.start_position);
    output_tape.rewind();
    detail::SortProgressLogger progress("polyphase merge sort output copy IO", source_run.value_count);

    for (size_t copied_values = 0; copied_values < source_run.value_count; ++copied_values) {
        output_tape.write(source_tape.read());
        source_tape.next();
        output_tape.next();
        progress.mark(copied_values + 1);
    }
}

void PolyphaseMergeTapeSorter::move_to_position(Tape &tape, size_t position) {
    tape.rewind();
    while (tape.position() < position) {
        tape.next();
    }
}

}  // namespace tp
