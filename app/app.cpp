#include "config/parser.hpp"
#include "tape/external_k_way_merge_tape_sorter.hpp"
#include "tape/external_merge_tape_sorter.hpp"
#include "tape/file_tape.hpp"
#include "tape/tape_sorter.hpp"
#include "utils/tmp_directory.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>

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
        tp::config::Config config;
        if (config_path.has_value()) {
            config = tp::config::parse_config(*config_path);
        }

        auto temp_dir = tp::utils::TmpDirectory();
        auto input_tape = std::make_shared<tp::FileTape>(input_tape_path, config.tape_sorter.latency);
        auto output_tape =
            std::make_shared<tp::FileTape>(output_tape_path, input_tape->size(), config.tape_sorter.latency);
        auto tape_factory = std::make_shared<tp::FileTapeFactory>(temp_dir.path());

        std::unique_ptr<tp::TapeSorter> sorter;
        switch (config.tape_sorter.algorithm) {
            case tp::config::TapeSorterAlgorithm::BASIC_EXTERNAL_MERGE:
                sorter =
                    std::make_unique<tp::ExternalMergeTapeSorter>(tape_factory, config.tape_sorter.memory_limit_bytes);
                break;
            case tp::config::TapeSorterAlgorithm::K_WAY_EXTERNAL_MERGE:
                sorter = std::make_unique<tp::ExternalKWayMergeTapeSorter>(
                    tape_factory, config.tape_sorter.memory_limit_bytes,
                    config.tape_sorter.k_way_external_merge.merge_order);
                break;
        }

        sorter->sort(input_tape, output_tape);
    } catch (const std::exception &ex) {
        std::cerr << "Error while processing: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
