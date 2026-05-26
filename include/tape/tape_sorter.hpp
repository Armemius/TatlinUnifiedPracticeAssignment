#pragma once

#include "tape/tape.hpp"

#include <cstddef>
#include <memory>

namespace tp {

class TapeSorter {
   public:
    TapeSorter(std::shared_ptr<TapeFactory> temporary_factory, size_t mem_limit);

    virtual void sort(std::shared_ptr<Tape> input, std::shared_ptr<Tape> output) = 0;

    virtual ~TapeSorter() = default;

   protected:
    size_t mem_limit_{};
    std::shared_ptr<TapeFactory> temporary_factory_;
};

};  // namespace tp