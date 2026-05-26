#include "tape/external_k_way_merge_tape_sorter.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tp {

namespace {

[[nodiscard]] bool has_unread_values(const Tape &tape) {
    return tape.position() < tape.size();
}

[[nodiscard]] std::vector<int32_t> read_sorted_chunk(Tape &input_tape, size_t max_value_count) {
    std::vector<int32_t> chunk;
    chunk.reserve(max_value_count);

    while (chunk.size() < max_value_count && has_unread_values(input_tape)) {
        chunk.push_back(input_tape.read());
        input_tape.next();
    }

    std::ranges::sort(chunk);
    return chunk;
}

}  // namespace

ExternalKWayMergeTapeSorter::ExternalKWayMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory,
                                                         size_t memory_limit_bytes, size_t merge_order)
    : TapeSorter(std::move(temporary_tape_factory), memory_limit_bytes), merge_order_(merge_order) {
    if (merge_order_ < 2) {
        throw std::invalid_argument("K-way merge order must be at least 2");
    }
}

void ExternalKWayMergeTapeSorter::sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) {
    input_tape->rewind();
    output_tape->rewind();

    const size_t max_chunk_size = max_chunk_value_count(*input_tape);
    std::vector<SortedRun> runs = create_initial_runs(*input_tape, max_chunk_size);

    while (runs.size() > 1) {
        runs = merge_pass(std::move(runs));
    }

    if (!runs.empty()) {
        copy_run(*runs.front().tape, runs.front().value_count, *output_tape);
    }
}

bool ExternalKWayMergeTapeSorter::MinValueFirst::operator()(const HeapEntry &left, const HeapEntry &right) const {
    if (left.value == right.value) {
        return left.run_index > right.run_index;
    }
    return left.value > right.value;
}

size_t ExternalKWayMergeTapeSorter::max_chunk_value_count(const Tape &input_tape) const {
    const size_t max_value_count = memory_limit_bytes_ / sizeof(int32_t);
    if (max_value_count == 0 && input_tape.size() != 0) {
        throw std::invalid_argument("memory limit is too small to hold one tape element");
    }
    return max_value_count;
}

std::vector<ExternalKWayMergeTapeSorter::SortedRun> ExternalKWayMergeTapeSorter::create_initial_runs(
    Tape &input_tape, size_t max_chunk_value_count) const {
    std::vector<SortedRun> runs;

    while (has_unread_values(input_tape)) {
        const std::vector<int32_t> sorted_chunk = read_sorted_chunk(input_tape, max_chunk_value_count);
        SortedRun run = create_empty_run(sorted_chunk.size());
        write_values_to_tape(sorted_chunk, *run.tape);
        runs.push_back(std::move(run));
    }

    return runs;
}

std::vector<ExternalKWayMergeTapeSorter::SortedRun> ExternalKWayMergeTapeSorter::merge_pass(
    std::vector<SortedRun> runs) const {
    std::vector<SortedRun> merged_runs;
    merged_runs.reserve((runs.size() + merge_order_ - 1) / merge_order_);

    for (size_t first_run = 0; first_run < runs.size(); first_run += merge_order_) {
        const size_t run_count = std::min(merge_order_, runs.size() - first_run);
        std::span<SortedRun> merge_group{runs.data() + first_run, run_count};

        if (run_count == 1) {
            merged_runs.push_back(std::move(merge_group.front()));
            continue;
        }

        SortedRun merged_run = create_empty_run(total_value_count(merge_group));
        merge_runs_to_tape(merge_group, *merged_run.tape);
        merged_runs.push_back(std::move(merged_run));
    }

    return merged_runs;
}

ExternalKWayMergeTapeSorter::SortedRun ExternalKWayMergeTapeSorter::create_empty_run(size_t value_count) const {
    return {.tape = temporary_tape_factory_->create_temporary(value_count), .value_count = value_count};
}

size_t ExternalKWayMergeTapeSorter::total_value_count(std::span<const SortedRun> runs) {
    return std::accumulate(runs.begin(), runs.end(), size_t{},
                           [](size_t total, const SortedRun &run) { return total + run.value_count; });
}

void ExternalKWayMergeTapeSorter::write_values_to_tape(std::span<const int32_t> values, Tape &output_tape) {
    output_tape.rewind();

    for (const int32_t value : values) {
        output_tape.write(value);
        output_tape.next();
    }
}

void ExternalKWayMergeTapeSorter::merge_runs_to_tape(std::span<SortedRun> runs, Tape &output_tape) {
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, MinValueFirst> min_heap;
    std::vector<size_t> values_read_from_run(runs.size());

    for (size_t run_index = 0; run_index < runs.size(); ++run_index) {
        SortedRun &run = runs[run_index];
        run.tape->rewind();

        if (run.value_count != 0) {
            min_heap.push({.value = run.tape->read(), .run_index = run_index});
        }
    }

    output_tape.rewind();

    while (!min_heap.empty()) {
        const HeapEntry next_value = min_heap.top();
        min_heap.pop();

        output_tape.write(next_value.value);
        output_tape.next();

        SortedRun &source_run = runs[next_value.run_index];
        ++values_read_from_run[next_value.run_index];

        if (values_read_from_run[next_value.run_index] < source_run.value_count) {
            source_run.tape->next();
            min_heap.push({.value = source_run.tape->read(), .run_index = next_value.run_index});
        }
    }
}

void ExternalKWayMergeTapeSorter::copy_run(Tape &source_tape, size_t value_count, Tape &output_tape) {
    source_tape.rewind();
    output_tape.rewind();

    for (size_t copied_values = 0; copied_values < value_count; ++copied_values) {
        output_tape.write(source_tape.read());
        source_tape.next();
        output_tape.next();
    }
}

}  // namespace tp
