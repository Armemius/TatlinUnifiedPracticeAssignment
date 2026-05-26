#include "tape/tape_sorter.hpp"

namespace tp {

TapeSorter::TapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory, size_t memory_limit_bytes)
    : memory_limit_bytes_(memory_limit_bytes), temporary_tape_factory_(std::move(temporary_tape_factory)) {}

}  // namespace tp
