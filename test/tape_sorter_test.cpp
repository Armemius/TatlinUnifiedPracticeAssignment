#include <gtest/gtest.h>

#include "tape/external_merge_tape_sorter.hpp"
#include "tape/mem_tape.hpp"
#include "tape/tape.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tp {
namespace {

TEST(TapeSorterTests, SortsWhenBufferIsLargerThanInputTape) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), 4096};

    auto input_range = {9, 4, 3, 7, 6, 1, 2, 5, 8};
    std::shared_ptr<Tape> input_tape = std::make_shared<MemTape>(input_range);
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());
    sorter.sort(input_tape, output_tape);

    std::vector expected(input_range.begin(), input_range.end());
    std::ranges::sort(expected);
    std::vector result(output_tape->begin(), output_tape->end());
    ASSERT_EQ(result, expected);
}

TEST(TapeSorterTests, SortsWhenBufferIsSmallerThanInputTape) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), sizeof(int32_t) * 4};

    auto input_range = {9, 4, 3, 7, 6, 1, 2, 5, 8};
    std::shared_ptr<Tape> input_tape = std::make_shared<MemTape>(input_range);
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());
    sorter.sort(input_tape, output_tape);

    std::vector expected(input_range.begin(), input_range.end());
    std::ranges::sort(expected);
    std::vector result(output_tape->begin(), output_tape->end());
    ASSERT_EQ(result, expected);
}

TEST(TapeSorterTests, SortsOneElementChunksWithRepeatedAndNegativeValues) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), sizeof(int32_t)};

    auto input_range = {5, -1, 5, 0, -7, 3, 3, -1};
    std::shared_ptr<Tape> input_tape = std::make_shared<MemTape>(input_range);
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());
    sorter.sort(input_tape, output_tape);

    std::vector expected(input_range.begin(), input_range.end());
    std::ranges::sort(expected);
    std::vector result(output_tape->begin(), output_tape->end());
    ASSERT_EQ(result, expected);
}

TEST(TapeSorterTests, RejectsMemoryLimitBelowOneElement) {
    std::shared_ptr<TapeFactory> factory = std::make_shared<MemTapeFactory>();
    ExternalMergeTapeSorter sorter{std::move(factory), sizeof(int32_t) - 1};

    std::shared_ptr<Tape> input_tape = std::make_shared<MemTape>(std::initializer_list<int32_t>{1});
    std::shared_ptr<MemTape> output_tape = std::make_shared<MemTape>(input_tape->size());

    ASSERT_THROW(sorter.sort(input_tape, output_tape), std::invalid_argument);
}

}  // namespace
}  // namespace tp
