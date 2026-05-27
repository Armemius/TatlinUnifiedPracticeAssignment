#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include "tape/external_merge_tape_sorter.hpp"
#include "tape/file_tape.hpp"
#include "utils/tmp_directory.hpp"

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: task <input> <output> [config]\n";
        return EXIT_FAILURE;
    }
    std::filesystem::path input_tape_path = argv[1];
    std::filesystem::path output_tape_path = argv[2];
    std::optional<std::filesystem::path> config_path = std::nullopt;
    if (argc == 4) {
        config_path = argv[3];
    } else if (auto path = std::filesystem::current_path() / "config.toml"; std::filesystem::exists(path)) {
        config_path = std::move(path);
    }

    try {
        auto temp_dir = tp::utils::TmpDirectory();
        auto input_tape = std::make_shared<tp::FileTape>(input_tape_path);
        auto output_tape = std::make_shared<tp::FileTape>(output_tape_path, input_tape->size());
        auto tape_factory = std::make_shared<tp::FileTapeFactory>(temp_dir.path());

        auto sorter = tp::ExternalMergeTapeSorter(tape_factory, 1024 * 1024 * 256);
        sorter.sort(input_tape, output_tape);
    } catch (const std::exception &ex) {
        std::cerr << "Error while processing: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
