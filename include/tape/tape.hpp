#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace tp {

using namespace std::literals::chrono_literals;

/**
 * @brief Interface representing the tape
 */
class Tape {
   public:
    struct LatencyConfig {
        bool enable_sleep_delays{false};
        std::chrono::nanoseconds read_delay{1us};
        std::chrono::nanoseconds write_delay{2us};
        std::chrono::nanoseconds move_delay{100us};
        std::chrono::nanoseconds rewind_delay{1ms};
    };

    enum class TapeOperation { READ, WRITE, MOVE, REWIND };

    struct TapeStats {
        size_t reads{};
        size_t writes{};
        size_t moves{};
        size_t rewinds{};

        std::chrono::nanoseconds simulated_time{};
    };

    Tape() = default;

    explicit Tape(LatencyConfig latency_config);

    virtual ~Tape() = default;

    [[nodiscard]] const TapeStats &stats() const noexcept;

    void reset_stats() noexcept;

    /**
     * @brief Reads the current element the cursor is pointing to
     */
    [[nodiscard]] int32_t read();

    /**
     * @brief Writes the value of element the cursor is pointing to
     */
    void write(int32_t value);

    /**
     * @brief Moves the cursor to the next element
     */
    void next();

    /**
     * @brief Moves the cursor to the prev element
     */
    void prev();

    /**
     * @brief Moves cursor to the start of the tape
     */
    void rewind();

    /**
     * @brief Returns the index of the cell, the cursor is currently on
     */
    [[nodiscard]] virtual size_t position() const = 0;

    /**
     * @brief Returns the number of cells on the tape
     */
    [[nodiscard]] virtual size_t size() const = 0;

   protected:
    // Implementation hooks. Public methods apply latency, then delegate here

    [[nodiscard]] virtual int32_t do_read() = 0;
    virtual void do_write(int32_t value) = 0;
    virtual void do_next() = 0;
    virtual void do_prev() = 0;
    virtual void do_rewind() = 0;

   private:
    void delay(std::chrono::nanoseconds duration) const;
    void account_read();
    void account_write();
    void account_move();
    void account_rewind();

    TapeStats stats_;
    LatencyConfig latency_config_;
};

}  // namespace tp
