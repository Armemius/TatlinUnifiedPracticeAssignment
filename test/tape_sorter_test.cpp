#include <gtest/gtest.h>

#include "tape/external_k_way_merge_tape_sorter.hpp"
#include "tape/external_merge_tape_sorter.hpp"
#include "tape/mem_tape.hpp"
#include "tape/polyphase_merge_tape_sorter.hpp"
#include "tape/tape.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tp {
namespace {

std::shared_ptr<MemTape> make_tape(const std::vector<int32_t> &values) {
    std::shared_ptr<MemTape> tape = std::make_shared<MemTape>(values.size());

    for (const int32_t value : values) {
        tape->write(value);
        tape->next();
    }
    tape->rewind();

    return tape;
}

std::vector<std::vector<int32_t>> sorting_inputs() {
    return {
        {},
        {42},
        {-5, -1, 0, 7, 12},
        {12, 7, 0, -1, -5},
        {5, -1, 5, 0, -7, 3, 3, -1},
        {34, -12, 0, 99, 17, -100, 42, 42, 8, 3, 3, 11, -5, 76, 21, 1, 0, -33, 18, 18, 7},
    };
}

void assert_sorts_values(TapeSorter &sorter, const std::vector<int32_t> &input_values) {
    std::shared_ptr<Tape> input_tape = make_tape(input_values);
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());

    sorter.sort(input_tape, output_tape);

    std::vector<int32_t> expected(input_values.begin(), input_values.end());
    std::ranges::sort(expected);
    std::vector result(output_tape->begin(), output_tape->end());
    ASSERT_EQ(result, expected);
}

template <class SorterFactory>
void assert_sorter_handles_common_inputs(SorterFactory create_sorter) {
    for (const std::vector<int32_t> &input_values : sorting_inputs()) {
        std::unique_ptr<TapeSorter> sorter = create_sorter();
        assert_sorts_values(*sorter, input_values);
    }
}

TEST(ExternalMergeSorterTests, SortsCommonInputsWhenBufferIsLargerThanInputTape) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), 4096};

    assert_sorts_values(sorter, {9, 4, 3, 7, 6, 1, 2, 5, 8});
}

TEST(ExternalMergeSorterTests, SortsCommonInputsWithMultiChunkRuns) {
    assert_sorter_handles_common_inputs([] {
        return std::make_unique<ExternalMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t) * 3);
    });
}

TEST(ExternalMergeSorterTests, SortsCommonInputsWithOneValueChunks) {
    assert_sorter_handles_common_inputs(
        [] { return std::make_unique<ExternalMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t)); });
}

TEST(ExternalMergeSorterTests, RejectsMemoryLimitBelowOneElement) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), sizeof(int32_t) - 1};

    std::shared_ptr<Tape> input_tape = make_tape({1});
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());

    ASSERT_THROW(sorter.sort(input_tape, output_tape), std::invalid_argument);
}

TEST(ExternalKWayMergeSorterTests, SortsCommonInputsWithBinaryHeapMerge) {
    assert_sorter_handles_common_inputs([] {
        return std::make_unique<ExternalKWayMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t) * 2,
                                                             2);
    });
}

TEST(ExternalKWayMergeSorterTests, SortsCommonInputsWithTernaryHeapMerge) {
    assert_sorter_handles_common_inputs([] {
        return std::make_unique<ExternalKWayMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t) * 3,
                                                             3);
    });
}

TEST(ExternalKWayMergeSorterTests, SortsWhenFinalMergeGroupIsNotFull) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalKWayMergeTapeSorter sorter{std::move(factory), sizeof(int32_t), 4};

    assert_sorts_values(sorter, {8, 2, 5, 1, 7, 3, 6, 4, 0});
}

TEST(ExternalKWayMergeSorterTests, RejectsMergeOrderBelowTwo) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();

    ASSERT_THROW(ExternalKWayMergeTapeSorter sorter(std::move(factory), sizeof(int32_t), 1), std::invalid_argument);
}

TEST(ExternalKWayMergeSorterTests, RejectsMemoryLimitBelowOneElement) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalKWayMergeTapeSorter sorter{std::move(factory), sizeof(int32_t) - 1, 3};

    std::shared_ptr<Tape> input_tape = make_tape({1});
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());

    ASSERT_THROW(sorter.sort(input_tape, output_tape), std::invalid_argument);
}

TEST(PolyphaseMergeSorterTests, SortsCommonInputsWithMultiChunkRuns) {
    assert_sorter_handles_common_inputs([] {
        return std::make_unique<PolyphaseMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t) * 3);
    });
}

TEST(PolyphaseMergeSorterTests, SortsCommonInputsWithOneValueChunks) {
    assert_sorter_handles_common_inputs(
        [] { return std::make_unique<PolyphaseMergeTapeSorter>(std::make_shared<MemTapeFactory>(), sizeof(int32_t)); });
}

TEST(PolyphaseMergeSorterTests, SortsWhenFibonacciDistributionNeedsDummyRuns) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    PolyphaseMergeTapeSorter sorter{std::move(factory), sizeof(int32_t)};

    assert_sorts_values(sorter, {13, 5, 8, 3, 2, 21, 1});
}

TEST(PolyphaseMergeSorterTests, SortsManyFibonacciDistributionBoundaries) {
    for (size_t value_count = 1; value_count <= 32; ++value_count) {
        std::vector<int32_t> input_values;
        input_values.reserve(value_count);

        for (size_t index = 0; index < value_count; ++index) {
            input_values.push_back(static_cast<int32_t>(((index * 17) + (value_count * 5)) % 41) - 20);
        }

        std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
        PolyphaseMergeTapeSorter sorter{std::move(factory), sizeof(int32_t)};
        assert_sorts_values(sorter, input_values);
    }
}

TEST(PolyphaseMergeSorterTests, RejectsMemoryLimitBelowOneElement) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    PolyphaseMergeTapeSorter sorter{std::move(factory), sizeof(int32_t) - 1};

    std::shared_ptr<Tape> input_tape = make_tape({1});
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());

    ASSERT_THROW(sorter.sort(input_tape, output_tape), std::invalid_argument);
}

}  // namespace
}  // namespace tp
