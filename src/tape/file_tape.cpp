#include "tape/file_tape.hpp"

#include "tape/tape.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

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

FileTapeFactory::FileTapeFactory(std::filesystem::path path) : temporary_path_(std::move(path)) {
    std::filesystem::create_directories(temporary_path_);
}

std::unique_ptr<Tape> FileTapeFactory::create_temporary(size_t size) {
    std::ostringstream tape_name;
    tape_name << "tmp_tape_" << created_tapes_++ << ".bin";  // No std::format in my WSL stdc++ :(
    std::filesystem::path tmp_tape_path = temporary_path_ / tape_name.str();
    return std::make_unique<FileTape>(tmp_tape_path, size);
}

}  // namespace tp
