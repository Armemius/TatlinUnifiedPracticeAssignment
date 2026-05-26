#include <gtest/gtest.h>

#include "tape/mem_tape.hpp"
#include "tape/tape.hpp"

#include <chrono>
#include <memory>

namespace tp {
namespace {

class MockTape : public Tape {
   public:
    using Tape::Tape;

    size_t size() const override { return 0; }

    size_t position() const override { return 0; }

   protected:
    [[nodiscard]] int32_t do_read() override { return 0; }

    void do_write(int32_t value) override {}

    void do_next() override {}

    void do_prev() override {}

    void do_rewind() override {}
};

TEST(TapeTests, CountsStats) {

    MockTape tape;
    ASSERT_EQ(tape.stats().moves, 0);
    ASSERT_EQ(tape.stats().reads, 0);
    ASSERT_EQ(tape.stats().writes, 0);
    ASSERT_EQ(tape.stats().rewinds, 0);

    tape.next();
    tape.next();
    (void)tape.read();
    tape.write(42);
    (void)tape.read();
    tape.prev();
    tape.rewind();

    ASSERT_EQ(tape.stats().moves, 3);
    ASSERT_EQ(tape.stats().reads, 2);
    ASSERT_EQ(tape.stats().writes, 1);
    ASSERT_EQ(tape.stats().rewinds, 1);
}

TEST(TapeTests, CalculatesExecutionTime) {
    MockTape tape({.read_delay = 5s, .write_delay = 10s, .move_delay = 15s, .rewind_delay = 20s});
    ASSERT_EQ(tape.stats().simulated_time, 0s);
    (void)tape.read();
    ASSERT_EQ(tape.stats().simulated_time, 5s);
    tape.write(42);
    ASSERT_EQ(tape.stats().simulated_time, 15s);
    tape.next();
    ASSERT_EQ(tape.stats().simulated_time, 30s);
    tape.rewind();
    ASSERT_EQ(tape.stats().simulated_time, 50s);
}

TEST(TapeTests, DelaysOnRequest) {
    MockTape tape({.enable_sleep_delays = true, .read_delay = 1s});
    auto start = std::chrono::steady_clock::now();
    (void)tape.read();
    auto end = std::chrono::steady_clock::now();
    ASSERT_LT(1s, end - start);
}

TEST(MemTapeTests, InitializesViaSizeCtor) {
    MemTape tape(10);
    ASSERT_EQ(tape.size(), 10);
    EXPECT_NO_THROW(tape.next());
    EXPECT_NO_THROW(tape.prev());
}

TEST(MemTapeTests, InitializesWithLatencyConfig) {
    MemTape tape(10, {.move_delay = 10s});
    ASSERT_EQ(tape.size(), 10);
    EXPECT_NO_THROW(tape.next());
    EXPECT_NO_THROW(tape.prev());
}

TEST(MemTapeTests, InitializesWithInitializerList) {
    auto range = {1, 2, 3, 4, 5, 6, 7};
    MemTape tape(range);
    ASSERT_EQ(tape.size(), range.size());
}

TEST(MemTapeTests, MovesBackAndForward) {
    MemTape tape{1, 2, 3, 4, 5, 6, 7};
    ASSERT_EQ(tape.read(), 1);
    while (tape.position() != tape.size() - 1) {
        tape.next();
    }
    ASSERT_EQ(tape.read(), 7);
    while (tape.position() != 0) {
        tape.prev();
    }
    ASSERT_EQ(tape.read(), 1);
}

TEST(MemTapeTests, ProcessesImpossibleMoves) {
    MemTape tape(10);
    ASSERT_NO_THROW(tape.prev());
    ASSERT_EQ(tape.position(), 0);
    while (tape.position() != tape.size()) {
        tape.next();
    }
    ASSERT_EQ(tape.position(), tape.size());
}

TEST(MemTapeTests, ReadsAndWrites) {
    auto range = {1, 2, 3, 4, 5, 6, 7};
    MemTape tape(range);
    for (auto it : range) {
        ASSERT_EQ(tape.read(), it);
        tape.write(-it);
        tape.next();
    }
    tape.rewind();
    for (auto it : range) {
        ASSERT_EQ(tape.read(), -it);
        tape.next();
    }
}

TEST(MemTapeTests, ShowsActualPosition) {
    MemTape tape{1, 2, 3, 4, 5, 6, 7};
    ASSERT_EQ(tape.position(), 0);
    tape.prev();
    ASSERT_EQ(tape.position(), 0);
    tape.next();
    ASSERT_EQ(tape.position(), 1);
    while (tape.position() != tape.size()) {
        tape.next();
    }
    ASSERT_EQ(tape.position(), tape.size());
    tape.prev();
    ASSERT_EQ(tape.position(), 6);
}

TEST(MemTapeTests, ExposesInternalStructureViaIterators) {
    std::vector<int32_t> src{1, 2, 3, 4, 5, 6, 7};
    MemTape tape{1, 2, 3, 4, 5, 6, 7};
    ASSERT_TRUE(std::ranges::equal(src, tape));
}

TEST(MemTapeTests, FactoryProducesTemporaryTapes) {
    MemTapeFactory factory;
    std::unique_ptr<Tape> tape = factory.create_temporary(42);
    ASSERT_EQ(tape->size(), 42);
}

}  // namespace
}  // namespace tp
