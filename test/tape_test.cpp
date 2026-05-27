#include <gtest/gtest.h>

#include "tape/file_tape.hpp"
#include "tape/mem_tape.hpp"
#include "tape/tape.hpp"
#include "utils/tmp_directory.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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

struct TapeHandle {
    std::unique_ptr<utils::TmpDirectory> directory;
    std::unique_ptr<Tape> tape;
};

std::filesystem::path unique_test_directory_path() {
    static size_t directory_id = 0;
    return std::filesystem::temp_directory_path() / ("tape_tests_" + std::to_string(directory_id++));
}

void write_values(Tape &tape, std::initializer_list<int32_t> values) {
    for (const int32_t value : values) {
        tape.write(value);
        tape.next();
    }
    tape.rewind();
}

std::vector<int32_t> read_values(Tape &tape) {
    std::vector<int32_t> values;
    tape.rewind();
    while (tape.position() != tape.size()) {
        values.push_back(tape.read());
        tape.next();
    }
    tape.rewind();
    return values;
}

struct MemTapeAdapter {
    static std::string name() { return "MemTape"; }

    static TapeHandle make(size_t size) { return {.tape = std::make_unique<MemTape>(size)}; }

    static TapeHandle make(size_t size, Tape::LatencyConfig config) {
        return {.tape = std::make_unique<MemTape>(size, config)};
    }

    static TapeHandle make(std::initializer_list<int32_t> values) {
        return {.tape = std::make_unique<MemTape>(values)};
    }
};

struct FileTapeAdapter {
    static std::string name() { return "FileTape"; }

    static TapeHandle make(size_t size) {
        auto directory = std::make_unique<utils::TmpDirectory>(unique_test_directory_path());
        auto tape = std::make_unique<FileTape>(directory->path() / "tape.bin", size);
        return {.directory = std::move(directory), .tape = std::move(tape)};
    }

    static TapeHandle make(size_t size, Tape::LatencyConfig config) {
        auto directory = std::make_unique<utils::TmpDirectory>(unique_test_directory_path());
        auto tape = std::make_unique<FileTape>(directory->path() / "tape.bin", size, config);
        return {.directory = std::move(directory), .tape = std::move(tape)};
    }

    static TapeHandle make(std::initializer_list<int32_t> values) {
        TapeHandle handle = make(values.size());
        write_values(*handle.tape, values);
        return handle;
    }
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

template <class Adapter>
class TapeBehaviorTests : public ::testing::Test {};

using TapeImplementations = ::testing::Types<MemTapeAdapter, FileTapeAdapter>;

struct TapeImplementationNames {
    template <class Adapter>
    static std::string GetName(int) {
        return Adapter::name();
    }
};

TYPED_TEST_SUITE(TapeBehaviorTests, TapeImplementations, TapeImplementationNames);

TYPED_TEST(TapeBehaviorTests, InitializesViaSizeCtor) {
    TapeHandle handle = TypeParam::make(10);
    Tape &tape = *handle.tape;

    ASSERT_EQ(tape.size(), 10);
    EXPECT_NO_THROW(tape.next());
    EXPECT_NO_THROW(tape.prev());
}

TYPED_TEST(TapeBehaviorTests, InitializesWithLatencyConfig) {
    TapeHandle handle = TypeParam::make(10, {.move_delay = 10s});
    Tape &tape = *handle.tape;

    ASSERT_EQ(tape.size(), 10);
    EXPECT_NO_THROW(tape.next());
    EXPECT_NO_THROW(tape.prev());
}

TYPED_TEST(TapeBehaviorTests, MovesBackAndForward) {
    TapeHandle handle = TypeParam::make({1, 2, 3, 4, 5, 6, 7});
    Tape &tape = *handle.tape;

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

TYPED_TEST(TapeBehaviorTests, ProcessesImpossibleMoves) {
    TapeHandle handle = TypeParam::make(10);
    Tape &tape = *handle.tape;

    ASSERT_NO_THROW(tape.prev());
    ASSERT_EQ(tape.position(), 0);
    while (tape.position() != tape.size()) {
        tape.next();
    }
    ASSERT_EQ(tape.position(), tape.size());
}

TYPED_TEST(TapeBehaviorTests, ReadsAndWrites) {
    auto range = {1, 2, 3, 4, 5, 6, 7};
    TapeHandle handle = TypeParam::make(range);
    Tape &tape = *handle.tape;

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

TYPED_TEST(TapeBehaviorTests, ShowsActualPosition) {
    TapeHandle handle = TypeParam::make({1, 2, 3, 4, 5, 6, 7});
    Tape &tape = *handle.tape;

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

TEST(MemTapeTests, InitializesWithInitializerList) {
    auto range = {1, 2, 3, 4, 5, 6, 7};
    MemTape tape(range);
    ASSERT_EQ(tape.size(), range.size());
}

TEST(MemTapeTests, ExposesInternalStructureViaIterators) {
    std::vector<int32_t> src{1, 2, 3, 4, 5, 6, 7};
    MemTape tape{1, 2, 3, 4, 5, 6, 7};

    std::vector result(tape.begin(), tape.end());
    ASSERT_EQ(src, result);
}

TEST(MemTapeTests, FactoryProducesTemporaryTapes) {
    MemTapeFactory factory;
    std::unique_ptr<Tape> tape = factory.create_temporary(42);
    ASSERT_EQ(tape->size(), 42);
}

TEST(FileTapeTests, CreatesZeroFilledFileOfRequestedSize) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "tape.bin";

    FileTape tape(path, 3);

    ASSERT_EQ(tape.size(), 3);
    ASSERT_EQ(std::filesystem::file_size(path), 3 * sizeof(int32_t));
    ASSERT_EQ(read_values(tape), std::vector<int32_t>({0, 0, 0}));
}

TEST(FileTapeTests, OpensExistingFileAndReadsItsSize) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "tape.bin";
    {
        FileTape tape(path, 2);
        write_values(tape, {11, -7});
    }

    FileTape reopened(path);

    ASSERT_EQ(reopened.size(), 2);
    ASSERT_EQ(read_values(reopened), std::vector<int32_t>({11, -7}));
}

TEST(FileTapeTests, PersistsWritesToFile) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "tape.bin";
    {
        FileTape tape(path, 4);
        write_values(tape, {8, 6, 7, 5});
    }

    FileTape reopened(path);

    ASSERT_EQ(read_values(reopened), std::vector<int32_t>({8, 6, 7, 5}));
}

TEST(FileTapeTests, WritesValuesAsLittleEndianInt32Cells) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "tape.bin";
    {
        FileTape tape(path, 1);
        tape.write(0x01020304);
    }

    std::ifstream stream(path, std::ios::binary);
    std::vector<unsigned char> bytes(sizeof(int32_t));
    stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    ASSERT_EQ(bytes, std::vector<unsigned char>({0x04, 0x03, 0x02, 0x01}));
}

TEST(FileTapeTests, RejectsMissingFileWhenSizeIsNotProvided) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "missing.bin";

    ASSERT_THROW(FileTape tape(path), std::runtime_error);
}

TEST(FileTapeTests, RejectsFilesWithPartialCells) {
    utils::TmpDirectory directory(unique_test_directory_path());
    const std::filesystem::path path = directory.path() / "partial.bin";
    {
        std::ofstream stream(path, std::ios::binary);
        stream.put('\0');
    }

    ASSERT_THROW(FileTape tape(path), std::runtime_error);
}

TEST(FileTapeTests, RejectsWritesPastEnd) {
    TapeHandle handle = FileTapeAdapter::make(1);
    Tape &tape = *handle.tape;

    tape.next();

    ASSERT_THROW(tape.write(42), std::logic_error);
}

TEST(FileTapeTests, FactoryProducesTemporaryFileTapes) {
    utils::TmpDirectory directory(unique_test_directory_path());
    FileTapeFactory factory(directory.path());

    std::unique_ptr<Tape> tape = factory.create_temporary(2);
    write_values(*tape, {13, -13});

    ASSERT_EQ(tape->size(), 2);
    ASSERT_EQ(read_values(*tape), std::vector<int32_t>({13, -13}));
}

}  // namespace
}  // namespace tp
