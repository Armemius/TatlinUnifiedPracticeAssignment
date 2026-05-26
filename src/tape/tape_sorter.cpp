#include "tape/tape_sorter.hpp"

namespace tp {

TapeSorter::TapeSorter(std::shared_ptr<TapeFactory> temporary_factory, size_t mem_limit)
    : temporary_factory_(std::move(temporary_factory)), mem_limit_(mem_limit) {}

}  // namespace tp
