#include "tape/tape.hpp"

namespace tp {

Tape::Tape(Tape::LatencyConfig latency_config) : latency_config_(latency_config) {}

const Tape::TapeStats &Tape::stats() const noexcept {
    return stats_;
}

void Tape::reset_stats() noexcept {
    stats_ = {};
}

int32_t Tape::read() {
    account_read();
    return do_read();
}

void Tape::write(int32_t value) {
    account_write();
    do_write(value);
}

void Tape::next() {
    account_move();
    do_next();
}

void Tape::prev() {
    account_move();
    do_prev();
}

void Tape::rewind() {
    account_rewind();
    do_rewind();
}

void Tape::account_read() {
    ++stats_.reads;
    stats_.simulated_time += latency_config_.read_delay;
    delay(latency_config_.read_delay);
}

void Tape::account_write() {
    ++stats_.writes;
    stats_.simulated_time += latency_config_.write_delay;
    delay(latency_config_.write_delay);
}

void Tape::account_move() {
    ++stats_.moves;
    stats_.simulated_time += latency_config_.move_delay;
    delay(latency_config_.move_delay);
}

void Tape::account_rewind() {
    ++stats_.rewinds;
    stats_.simulated_time += latency_config_.rewind_delay;
    delay(latency_config_.rewind_delay);
}

}  // namespace tp
