#include "tape/mem_tape.hpp"
#include "tape/tape.hpp"

namespace tp {

MemTape::MemTape(size_t size) : MemTape(size, {}) {}

MemTape::MemTape(size_t size, LatencyConfig config) : memory_(size), cursor_(memory_.begin()), Tape(config) {}

MemTape::MemTape(std::initializer_list<int32_t> list) : memory_(list.begin(), list.end()), cursor_(memory_.begin()) {}

size_t MemTape::size() const {
    return memory_.size();
}

size_t MemTape::position() const {
    return std::ranges::distance(memory_.begin(), cursor_);
}

int32_t MemTape::do_read() {
    return *cursor_;
}

void MemTape::do_write(int32_t value) {
    *cursor_ = value;
}

void MemTape::do_next() {
    if (cursor_ != memory_.end()) {
        ++cursor_;
    }
}

void MemTape::do_prev() {
    if (cursor_ != memory_.begin()) {
        --cursor_;
    }
}

void MemTape::do_rewind() {
    cursor_ = memory_.begin();
}

std::vector<int32_t>::const_iterator MemTape::begin() {
    return memory_.cbegin();
}

std::vector<int32_t>::const_iterator MemTape::end() {
    return memory_.cend();
}

}  // namespace tp
