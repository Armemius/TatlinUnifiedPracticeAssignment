#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include "tape/tape.hpp"

namespace tp {

class FileTape final : public Tape {
   public:
    /**
     * @brief Construct a new File Tape object from existing tape in FS
     * 
     */
    explicit FileTape(const std::filesystem::path &path);

    /**
     * @brief Construct a new File Tape object from existing tape in FS or creates new one
     * 
     */
    explicit FileTape(const std::filesystem::path &path, size_t size);

    FileTape(const std::filesystem::path &path, LatencyConfig config);

    FileTape(const std::filesystem::path &path, size_t size, LatencyConfig config);

    [[nodiscard]] size_t size() const override;

    [[nodiscard]] size_t position() const override;

   protected:
    [[nodiscard]] int32_t do_read() override;
    void do_write(int32_t value) override;
    void do_next() override;
    void do_prev() override;
    void do_rewind() override;

   private:
    std::streamoff offset() const;

    void sync_get();

    void sync_put();

    size_t size_{};
    size_t pos_{};
    std::fstream stream_;
};

class FileTapeFactory : public TapeFactory {
   public:
    explicit FileTapeFactory(std::filesystem::path path);

    FileTapeFactory(std::filesystem::path path, Tape::LatencyConfig config);

    std::unique_ptr<Tape> create_temporary(size_t size) override;

    [[nodiscard]] size_t created_tape_count() const noexcept;

    [[nodiscard]] const Tape::TapeStats &temporary_stats() const noexcept;

   private:
    inline static size_t next_tape_id_ = 0;

    std::filesystem::path temporary_path_;
    Tape::LatencyConfig latency_config_;
    size_t created_tape_count_{};
    Tape::TapeStats temporary_stats_;
};

}  // namespace tp
