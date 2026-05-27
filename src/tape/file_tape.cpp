#include "tape/file_tape.hpp"

#include "tape/tape.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

int32_t to_little_endian(int32_t value) {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        auto u = static_cast<uint32_t>(value);
        u = ((u & 0x000000FFU) << 24) | ((u & 0x0000FF00U) << 8) | ((u & 0x00FF0000U) >> 8) | ((u & 0xFF000000U) >> 24);
        return static_cast<int32_t>(u);
    }
}

int32_t from_little_endian(int32_t value) {
    return to_little_endian(value);
}

void add_stats(tp::Tape::TapeStats &target, const tp::Tape::TapeStats &source) {
    target.reads += source.reads;
    target.writes += source.writes;
    target.moves += source.moves;
    target.rewinds += source.rewinds;
    target.simulated_time += source.simulated_time;
}

class TrackedTemporaryTape final : public tp::Tape {
   public:
    TrackedTemporaryTape(std::unique_ptr<tp::Tape> tape, LatencyConfig latency_config, TapeStats &stats_sink)
        : Tape(latency_config), tape_(std::move(tape)), stats_sink_(stats_sink) {}

    ~TrackedTemporaryTape() override { add_stats(stats_sink_, stats()); }

    [[nodiscard]] size_t size() const override { return tape_->size(); }

    [[nodiscard]] size_t position() const override { return tape_->position(); }

   protected:
    [[nodiscard]] int32_t do_read() override { return tape_->read(); }

    void do_write(int32_t value) override { tape_->write(value); }

    void do_next() override { tape_->next(); }

    void do_prev() override { tape_->prev(); }

    void do_rewind() override { tape_->rewind(); }

   private:
    std::unique_ptr<tp::Tape> tape_;
    TapeStats &stats_sink_;
};

}  // namespace

namespace tp {

FileTape::FileTape(const std::filesystem::path &path) : FileTape(path, LatencyConfig{}) {}

FileTape::FileTape(const std::filesystem::path &path, size_t size) : FileTape(path, size, LatencyConfig{}) {}

FileTape::FileTape(const std::filesystem::path &path, LatencyConfig config) : Tape(config) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File does not exist");
    }
    stream_ = std::fstream{path, std::ios::in | std::ios::out | std::ios::binary};
    if (!stream_.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    stream_.seekg(0, std::ios::end);
    auto bytes = stream_.tellg();
    if (bytes < 0) {
        throw std::runtime_error("Failed to get file size");
    }
    if (bytes % sizeof(int32_t) != 0) {
        throw std::runtime_error("Tape size is invalid (should be divisible by 4 bytes)");
    }
    size_ = bytes / sizeof(int32_t);
    stream_.seekg(0, std::ios::beg);
}

FileTape::FileTape(const std::filesystem::path &path, size_t size, LatencyConfig config) : size_(size), Tape(config) {
    if (!std::filesystem::exists(path)) {
        std::ofstream out{path, std::ios::app | std::ios::binary};
        if (!out.is_open()) {
            throw std::runtime_error("Failed to create file");
        }
        for (size_t it = 0; it < size; ++it) {
            int32_t value = 0;
            out.write(reinterpret_cast<const char *>(&value), sizeof(int32_t));
        }
    }

    stream_ = std::fstream{path, std::ios::in | std::ios::out | std::ios::binary};
    if (!stream_.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    stream_.seekg(0, std::ios::end);
    auto bytes = stream_.tellg();
    if (bytes < 0) {
        throw std::runtime_error("Failed to get file size");
    }
    if (bytes % sizeof(int32_t) != 0) {
        throw std::runtime_error("Tape size is invalid (should be divisible by 4 bytes)");
    }
    size_ = bytes / sizeof(int32_t);
    pos_ = 0;
    if (size_ < size) {
        stream_.seekg(0, std::ios::end);
        while (size_ < size) {
            int32_t value = 0;
            stream_.write(reinterpret_cast<const char *>(&value), sizeof(int32_t));
            ++size_;
        }
    }
    stream_.seekg(0, std::ios::beg);
}

size_t FileTape::size() const {
    return size_;
}

size_t FileTape::position() const {
    return pos_;
}

int32_t FileTape::do_read() {
    sync_get();

    std::int32_t value{};
    stream_.read(reinterpret_cast<char *>(&value), sizeof(value));
    if (!stream_) {
        throw std::runtime_error("Failed to read value from file");
    }

    return from_little_endian(value);
}

void FileTape::do_write(int32_t value) {
    sync_put();

    if (pos_ >= size_) {
        throw std::logic_error("Trying to write outside the tape boundaries");
    }
    int32_t normalized_value = to_little_endian(value);
    if (!stream_.write(reinterpret_cast<const char *>(&normalized_value), sizeof(int32_t))) {
        throw std::runtime_error("Failed to write value to file");
    }
}

void FileTape::do_next() {
    if (pos_ < size_) {
        ++pos_;
    }
}

void FileTape::do_prev() {
    if (pos_ > 0) {
        --pos_;
    }
}

void FileTape::do_rewind() {
    pos_ = 0;
}

std::streamoff FileTape::offset() const {
    return static_cast<std::streamoff>(pos_ * sizeof(std::int32_t));
}

void FileTape::sync_get() {
    stream_.clear();
    stream_.seekg(offset(), std::ios::beg);

    if (!stream_) {
        throw std::runtime_error("Failed to seek get pointer");
    }
}

void FileTape::sync_put() {
    stream_.clear();
    stream_.seekp(offset(), std::ios::beg);

    if (!stream_) {
        throw std::runtime_error("Failed to seek put pointer");
    }
}

FileTapeFactory::FileTapeFactory(std::filesystem::path path) : FileTapeFactory(std::move(path), {}) {}

FileTapeFactory::FileTapeFactory(std::filesystem::path path, Tape::LatencyConfig config)
    : temporary_path_(std::move(path)), latency_config_(config) {
    std::filesystem::create_directories(temporary_path_);
}

std::unique_ptr<Tape> FileTapeFactory::create_temporary(size_t size) {
    std::ostringstream tape_name;
    tape_name << "tmp_tape_" << next_tape_id_++ << ".bin";  // No std::format in my WSL stdc++ :(
    std::filesystem::path tmp_tape_path = temporary_path_ / tape_name.str();
    ++created_tape_count_;
    return std::make_unique<TrackedTemporaryTape>(std::make_unique<FileTape>(tmp_tape_path, size), latency_config_,
                                                  temporary_stats_);
}

size_t FileTapeFactory::created_tape_count() const noexcept {
    return created_tape_count_;
}

const Tape::TapeStats &FileTapeFactory::temporary_stats() const noexcept {
    return temporary_stats_;
}

}  // namespace tp
