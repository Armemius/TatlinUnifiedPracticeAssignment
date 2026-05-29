#pragma once

#include "tape/tape.hpp"

#include <cstddef>
#include <memory>

namespace tp {

class TapeSorter {
   public:
    TapeSorter(std::shared_ptr<TapeFactory> temporary_tape_factory, size_t memory_limit_bytes);

    virtual void sort(std::shared_ptr<Tape> input_tape, std::shared_ptr<Tape> output_tape) = 0;

    virtual ~TapeSorter() = default;

   protected:
    size_t memory_limit_bytes_{};
    std::shared_ptr<TapeFactory> temporary_tape_factory_;
};

}  // namespace tp
