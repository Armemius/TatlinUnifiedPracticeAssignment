#pragma once

#include <filesystem>

namespace tp::utils {

/**
 * @brief RAII wrapper for temporary directory
 * 
 */
class TmpDirectory {
   public:
    TmpDirectory();

    explicit TmpDirectory(std::filesystem::path path);

    [[nodiscard]] const std::filesystem::path &path() const noexcept;

    TmpDirectory(const TmpDirectory &) = delete;
    TmpDirectory &operator=(const TmpDirectory &) = delete;

    TmpDirectory(TmpDirectory &&) = default;
    TmpDirectory &operator=(TmpDirectory &&) = default;

    ~TmpDirectory();

   private:
    std::filesystem::path path_;
};

}  // namespace tp::utils
