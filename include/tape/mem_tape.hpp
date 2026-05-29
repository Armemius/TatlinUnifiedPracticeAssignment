#pragma once

#include "tape/tape.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace tp {

class MemTape final : public Tape {
   public:
    explicit MemTape(size_t size);

    MemTape(size_t size, LatencyConfig config);

    MemTape(std::initializer_list<int32_t> list);

    [[nodiscard]] size_t size() const override;

    [[nodiscard]] size_t position() const override;

    [[nodiscard]] std::vector<int32_t>::const_iterator begin() const;

    [[nodiscard]] std::vector<int32_t>::const_iterator end() const;

   protected:
    [[nodiscard]] int32_t do_read() override;
    void do_write(int32_t value) override;
    void do_next() override;
    void do_prev() override;
    void do_rewind() override;

   private:
    std::vector<int32_t> cells_;
    std::vector<int32_t>::iterator cursor_;
};

class MemTapeFactory final : public TapeFactory {
   public:
    std::unique_ptr<Tape> create_temporary(size_t size) override;
};

}  // namespace tp
