#pragma once

#include "tape/tape.hpp"
#include "tape/tape_sorter.hpp"

#include <memory>
#include <vector>

namespace tp {

class ExternalMergeTapeSorter final : public TapeSorter {
   public:
    ExternalMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_factory, size_t mem_limit);

    void sort(std::shared_ptr<Tape> input, std::shared_ptr<Tape> output) override;

   private:
    struct TemporaryBuffer {
        std::unique_ptr<Tape> tape;
        size_t elements{};
    };

    void clear();
    void merge_tape_with_buffer(Tape *input, size_t input_size, Tape *output);

    std::vector<int32_t> buffer_;
};

}  // namespace tp
