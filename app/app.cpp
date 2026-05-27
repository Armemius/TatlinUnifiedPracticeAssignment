#include "config/parser.hpp"
#include "tape/external_k_way_merge_tape_sorter.hpp"
#include "tape/external_merge_tape_sorter.hpp"
#include "tape/file_tape.hpp"
#include "tape/tape_sorter.hpp"
#include "utils/tmp_directory.hpp"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace {

void add_stats(tp::Tape::TapeStats &target, const tp::Tape::TapeStats &source) {
    target.reads += source.reads;
    target.writes += source.writes;
    target.moves += source.moves;
    target.rewinds += source.rewinds;
    target.simulated_time += source.simulated_time;
}

std::string format_duration(std::chrono::nanoseconds duration) {
    std::ostringstream stream;
    stream << duration.count() << " ns";

    if (duration != std::chrono::nanoseconds{}) {
        const auto milliseconds = std::chrono::duration<double, std::milli>(duration);
        stream << " (" << std::fixed << std::setprecision(3) << milliseconds.count() << " ms)";
    }

    return stream.str();
}

void print_execution_report(const tp::Tape::TapeStats &stats, size_t temporary_tape_count) {
    std::cout << "\nExecution stats:\n";
    std::cout << "Total copies: " << stats.reads + stats.writes << '\n';
    std::cout << "Total reads: " << stats.reads << '\n';
    std::cout << "Total writes: " << stats.writes << '\n';
    std::cout << "Total moves: " << stats.moves << '\n';
    std::cout << "Total rewinds: " << stats.rewinds << '\n';
    std::cout << "Total execution time: " << format_duration(stats.simulated_time) << '\n';
    std::cout << "Temporary tapes used: " << temporary_tape_count << '\n';
}

}  // namespace

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
        auto tape_factory = std::make_shared<tp::FileTapeFactory>(temp_dir.path(), config.tape_sorter.latency);

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

        tp::Tape::TapeStats total_stats;
        add_stats(total_stats, input_tape->stats());
        add_stats(total_stats, output_tape->stats());
        add_stats(total_stats, tape_factory->temporary_stats());
        print_execution_report(total_stats, tape_factory->created_tape_count());
    } catch (const std::exception &ex) {
        std::cerr << "Error while processing: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
