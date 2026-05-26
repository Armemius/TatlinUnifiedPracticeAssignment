#include "tape/external_merge_tape_sorter.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

namespace tp {

ExternalMergeTapeSorter::ExternalMergeTapeSorter(std::shared_ptr<TapeFactory> temporary_factory, size_t mem_limit)
    : TapeSorter(std::move(temporary_factory), mem_limit) {}

void ExternalMergeTapeSorter::sort(std::shared_ptr<Tape> input, std::shared_ptr<Tape> output) {
    clear();
    input->rewind();
    output->rewind();

    const size_t values_limit = mem_limit_ / sizeof(int32_t);
    if (values_limit == 0 && input->size() != 0) {
        throw std::invalid_argument("memory limit is too small to hold one tape element");
    }

    std::pair<TemporaryBuffer, TemporaryBuffer> buffers{
        TemporaryBuffer{.tape = temporary_factory_->create_temporary(input->size()), .elements = 0},
        TemporaryBuffer{.tape = temporary_factory_->create_temporary(input->size()), .elements = 0},
    };

    while (input->position() < input->size()) {
        while (buffer_.size() < values_limit && input->position() < input->size()) {
            buffer_.push_back(input->read());
            input->next();
        }
        std::ranges::sort(buffer_);
        merge_tape_with_buffer(buffers.first.tape.get(), buffers.first.elements, buffers.second.tape.get());
        buffers.second.elements = buffers.first.elements + buffer_.size();
        std::ranges::swap(buffers.first, buffers.second);
        buffer_.clear();
    }

    merge_tape_with_buffer(buffers.first.tape.get(), buffers.first.elements, output.get());
    clear();
}

void ExternalMergeTapeSorter::clear() {
    buffer_.clear();
}

void ExternalMergeTapeSorter::merge_tape_with_buffer(Tape *input, size_t size, Tape *output) {
    input->rewind();
    output->rewind();
    size_t input_tape_pos{};
    size_t buffer_pos{};

    while (input_tape_pos < size && buffer_pos < buffer_.size()) {
        int32_t tape_value = input->read();
        int32_t buffer_value = buffer_[buffer_pos];
        int32_t output_value{};

        if (tape_value < buffer_value) {
            output_value = tape_value;
            ++input_tape_pos;
            input->next();
        } else {
            output_value = buffer_value;
            ++buffer_pos;
        }

        output->write(output_value);
        output->next();
    }
    while (input_tape_pos < size) {
        output->write(input->read());
        ++input_tape_pos;
        input->next();
        output->next();
    }
    while (buffer_pos < buffer_.size()) {
        output->write(buffer_[buffer_pos]);
        ++buffer_pos;
        output->next();
    }
}

}  // namespace tp
