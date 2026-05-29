# Tatlin.Unified DataPath

*A tape-based data storage device is designed for sequential
data writing and reading. The read/write magnetic head remains stationary
during reading and writing, while the tape can move in both
directions. Writing and reading information is possible at the tape cell
where the magnetic head is currently located*

*Tape movement is a time-consuming operation—the tape is not designed
for random access*

There is an input tape of length N (where N is large) containing elements of
type integer. There is an output tape of the same length. You must write to
the output tape the elements from the input tape, sorted in ascending order.
There is a restriction on RAM usage—no more than M bytes
(M may be < N, i.e., it will not be possible to load all data from the tape
into RAM). To implement the algorithm, you may use a reasonable number of
temporary buffers, i.e., buffers in which you can store some temporary
information needed during the algorithm's execution.
You must create a C++ project that compiles into a console application which
implements the algorithm for sorting data from the input tape to the output
tape

## The following steps are required

1. Define an interface for working with a tape-type device
2. Write a class that implements this interface and emulates tape operations
   using a regular file. It must be possible to configure
   (without recompiling—for example, via an external configuration file that
   will be read at application startup) the delays for writing/reading an
   element from the tape, rewinding the tape, and shifting the tape by one
   position
3. Temporary tape files can be saved to the tmp directory
4. Write a class that implements an algorithm for sorting data from the input
   tape to the output
5. The console application should accept the names of the input and output
   files as input and perform the sorting
6. It is recommended to write unit tests

## How to Submit an Assignment for Review

Solve the problems in your GitHub repository, return to the test assignment,
paste the link to your repository into the text field on the right, and submit
the assignment
Before submitting, make sure your repository contains all the necessary files,
that you are pasting the correct link, and that it provides public access
to your solution
