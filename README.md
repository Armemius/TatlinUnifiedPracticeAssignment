# Tatlin.Unified practice assignment

Task: [RU](assets/TASK.ru.md) / [EN](assets/TASK.en.md)

## How to run

To run the solution, you need to have CMake and a C++ compiler installed on the
system. Then, you can follow these steps:

```bash
git clone https://github.com/Armemius/TatlinUnifiedPracticeAssignment
cd TatlinUnifiedPracticeAssignment
mkdir build
cd build
cmake ..
cmake --build .
```

Then you can run the program with:

```bash
./task <input_tape> <output_tape> [config]
```

Where `<input_tape>` is the path to the input file containing the tape data.
`<output_tape>` is the path to the output file where the results will be
written. `[config]` is an optional argument that specifies the path to a
configuration file

## Features

The solution implements external sorting algorithms for the emulated tape
storage. Implemented algorithms include:

- **External Merge Sort**: A classic external sorting algorithm that divides
  the data into manageable chunks, sorts each chunk in memory, and then merges
  the sorted chunks together
- **K-way Merge Sort**: An extension of the merge sort algorithm that merges
  multiple sorted runs simultaneously, reducing the number of passes needed to
  sort the data
- **Polyphase Merge Sort**: A tape-oriented merge algorithm that distributes
  sorted runs by Fibonacci counts and uses dummy runs to reduce idle merge
  passes

Some utility scripts were also implemented, such as:

- `scripts/check_sorted_tape.py`: checks if the output tape is sorted correctly
- `scripts/generate_input_tape.py`: generates random input tapes for testing

## Configuration

The program can be configured using a TOML configuration file. The
configuration file allows you to specify parameters such as the sorting
algorithm to use, the size of the memory buffer, and other relevant settings.
An example configuration file (`config.toml`) is provided in the repository

```toml
[tape_sorter]
memory_limit = "4mb"

# Available
# - basic_external_merge
# - k_way_external_merge
# - polyphase_merge
algorithm = "k_way_external_merge"

[tape_sorter.latency]
emulate_delays = false
read = "1us"
write = "2us"
move = "100us"
rewind = "1ms"

[tape_sorter.k_way_external_merge]
merge_order = 16
```
