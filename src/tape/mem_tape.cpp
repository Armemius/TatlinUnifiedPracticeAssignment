#include "tape/mem_tape.hpp"
#include <memory>
#include "tape/tape.hpp"

namespace tp {

MemTape::MemTape(size_t size) : MemTape(size, {}) {}

MemTape::MemTape(size_t size, LatencyConfig config) : Tape(config), cells_(size), cursor_(cells_.begin()) {}

MemTape::MemTape(std::initializer_list<int32_t> list) : cells_(list.begin(), list.end()), cursor_(cells_.begin()) {}

size_t MemTape::size() const {
    return cells_.size();
}

size_t MemTape::position() const {
    return std::ranges::distance(cells_.begin(), cursor_);
}

int32_t MemTape::do_read() {
    return *cursor_;
}

void MemTape::do_write(int32_t value) {
    *cursor_ = value;
}

void MemTape::do_next() {
    if (cursor_ != cells_.end()) {
        ++cursor_;
    }
}

void MemTape::do_prev() {
    if (cursor_ != cells_.begin()) {
        --cursor_;
    }
}

void MemTape::do_rewind() {
    cursor_ = cells_.begin();
}

std::vector<int32_t>::const_iterator MemTape::begin() const {
    return cells_.cbegin();
}

std::vector<int32_t>::const_iterator MemTape::end() const {
    return cells_.cend();
}

std::unique_ptr<Tape> MemTapeFactory::create_temporary(size_t size) {
    return std::make_unique<MemTape>(size);
}

}  // namespace tp
