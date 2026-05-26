#include "tape/external_merge_tape_sorter.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tp {

namespace {

[[nodiscard]] bool has_unread_values(const Tape &tape) {
    return tape.position() < tape.size();
}

}  // namespace

ExternalMergeTapeSorter::ExternalMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory,
                                                 size_t memory_limit_bytes)
    : TapeSorter(std::move(temporary_tape_factory), memory_limit_bytes) {}

void ExternalMergeTapeSorter::sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) {
    input_tape->rewind();
    output_tape->rewind();

    const size_t max_chunk_size = max_chunk_value_count(*input_tape);
    SortedRun sorted_run = create_empty_run(input_tape->size());
    SortedRun merge_target = create_empty_run(input_tape->size());

    while (has_unread_values(*input_tape)) {
        const std::vector<int32_t> sorted_chunk = read_sorted_chunk(*input_tape, max_chunk_size);
        merge_run_with_chunk(*sorted_run.tape, sorted_run.value_count, sorted_chunk, *merge_target.tape);
        merge_target.value_count = sorted_run.value_count + sorted_chunk.size();
        std::ranges::swap(sorted_run, merge_target);
    }

    copy_run(*sorted_run.tape, sorted_run.value_count, *output_tape);
}

size_t ExternalMergeTapeSorter::max_chunk_value_count(const Tape &input_tape) const {
    const size_t max_value_count = memory_limit_bytes_ / sizeof(int32_t);
    if (max_value_count == 0 && input_tape.size() != 0) {
        throw std::invalid_argument("memory limit is too small to hold one tape element");
    }
    return max_value_count;
}

ExternalMergeTapeSorter::SortedRun ExternalMergeTapeSorter::create_empty_run(size_t tape_size) const {
    return {.tape = temporary_tape_factory_->create_temporary(tape_size), .value_count = 0};
}

std::vector<int32_t> ExternalMergeTapeSorter::read_sorted_chunk(Tape &input_tape, size_t max_value_count) {
    std::vector<int32_t> chunk;
    chunk.reserve(max_value_count);

    while (chunk.size() < max_value_count && has_unread_values(input_tape)) {
        chunk.push_back(input_tape.read());
        input_tape.next();
    }

    std::ranges::sort(chunk);
    return chunk;
}

void ExternalMergeTapeSorter::merge_run_with_chunk(Tape &run_tape, size_t run_value_count,
                                                   std::span<const int32_t> sorted_chunk, Tape &output_tape) {
    run_tape.rewind();
    output_tape.rewind();

    size_t copied_from_run{};
    size_t copied_from_chunk{};

    while (copied_from_run < run_value_count && copied_from_chunk < sorted_chunk.size()) {
        const int32_t run_value = run_tape.read();
        const int32_t chunk_value = sorted_chunk[copied_from_chunk];

        if (run_value < chunk_value) {
            output_tape.write(run_value);
            ++copied_from_run;
            run_tape.next();
        } else {
            output_tape.write(chunk_value);
            ++copied_from_chunk;
        }

        output_tape.next();
    }

    while (copied_from_run < run_value_count) {
        output_tape.write(run_tape.read());
        ++copied_from_run;
        run_tape.next();
        output_tape.next();
    }

    while (copied_from_chunk < sorted_chunk.size()) {
        output_tape.write(sorted_chunk[copied_from_chunk]);
        ++copied_from_chunk;
        output_tape.next();
    }
}

void ExternalMergeTapeSorter::copy_run(Tape &source_tape, size_t value_count, Tape &output_tape) {
    source_tape.rewind();
    output_tape.rewind();

    for (size_t copied_values = 0; copied_values < value_count; ++copied_values) {
        output_tape.write(source_tape.read());
        source_tape.next();
        output_tape.next();
    }
}

}  // namespace tp
