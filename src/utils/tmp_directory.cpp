#include "utils/tmp_directory.hpp"
#include <filesystem>
#include <string>

namespace tp::utils {

TmpDirectory::TmpDirectory() : TmpDirectory(std::filesystem::temp_directory_path() / "tapes") {}

TmpDirectory::TmpDirectory(std::filesystem::path path) {
    if (std::filesystem::exists(path) && !std::filesystem::exists(path / "tapes")) {
        path = path / "tapes";
    } else if (std::filesystem::exists(path)) {
        size_t it = 0;
        while (std::filesystem::exists(path / ("tapes." + std::to_string(it)))) {
            ++it;
        }
        path = path / ("tapes." + std::to_string(it));
    }
    path_ = std::move(path);
    std::filesystem::create_directories(path_);
}

TmpDirectory::~TmpDirectory() {
    std::filesystem::remove_all(path_);
}

const std::filesystem::path &TmpDirectory::path() const noexcept {
    return path_;
}

}  // namespace tp::utils