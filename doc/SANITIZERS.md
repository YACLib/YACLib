# Sanitizers

### Useful links

1. https://github.com/google/sanitizers/wiki

### Options

[All Sanitizer options and their description](
https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags)

#### Useful options

1. verbosity=
    * 0 by default - silent
    * 1 - a bit of output
    * 2+ - more output
2. color=always
    * auto by default
3. coverage=1
    * If set, coverage information will be dumped at program shutdown
    * 0 by default
4. help=1
    * Print the flag ptions
    * 0 by default

## Address Sanitizer

### Useful links

1. [Supported platforms](
   https://github.com/google/sanitizers/wiki/AddressSanitizer#introduction)
2. [FAQ](
   https://github.com/google/sanitizers/wiki/AddressSanitizer#faq)
3. [Report format examples](
   https://github.com/google/sanitizers/wiki/AddressSanitizer#introduction), check list of refs

### Overhead

[Typical slowdown](
https://github.com/google/sanitizers/wiki/AddressSanitizerPerformanceNumbers): 2 Typical memory overhead: 3 for stack
memory and some for heap

### ASAN_OPTIONS

[All Address Sanitizer options and their description](
https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags)

#### Useful options

1. check_initialization_order=1
    * 0 by default
2. detect_stack_use_after_return=1
    * 0 by default
3. strict_init_order=1
    * If true, assume that dynamic initializers can never access globals from other modules, even if the latter are
      already initialized
    * 0 by default
4. strict_string_checks=1
    * If set check that string arguments are properly null-terminated
    * 0 by default
5. debug=1
    * If set, prints some debugging information and does additional checks
    * 0 by default
6. report_globals=2
    * Controls the way to handle globals
    * 0 - don't detect buffer overflow on globals
    * 1 by default - detect buffer overflow
    * 2 - print data about registered globals
7. sleep_after_init/sleep_before_dying=
    * Number of seconds to sleep after AddressSanitizer is initialized or between printing an error report and
      terminating the program
    * Useful for debugging purposes (e.g. when one needs to attach gdb)
    * 0 by default
8. atexit=1
    * If set, prints ASan exit stats even after program terminates successfully
    * 0 by default
9. print_stats=1
    * Print various statistics after printing an error message or if atexit=1
    * 0 by default
10. print_scariness=1
    * Print the scariness score
    * Experimental
    * 0 by default
11. halt_on_error=0
    * Crash the program after printing the first error report
    * WARNING: USE AT YOUR OWN RISK!
    * 1 by default

## Thread Sanitizer

### Useful links

1. [About](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerAboutRaces)
2. [Supported platforms](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual#supported-platforms)
3. [Supported atomic operations](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerAtomicOperations)
4. [FAQ](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual#faq)
5. [Report format](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerReportFormat)
6. [Popular data races](
   https://github.com/google/sanitizers/wiki/ThreadSanitizerPopularDataRaces)

### Overhead

Typical slowdown: [2..20] or [5..15]
Typical memory overhead: [3..10] or [5..10]

### TSAN_OPTIONS

[All Thread Sanitizer options and their description](
https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags)

#### Useful options

1. detect_deadlocks=1
    * [Description](
      https://github.com/google/sanitizers/wiki/ThreadSanitizerDeadlockDetector)
    * by default 0, because experimental
3. force_seq_cst_atomics=1
    * Turns all atomic operations into sequentially consistent operations
    * 0 by default
4. stop_on_start=1
    * Pause the program during ThreadSanitizer initialization
    * 0 by default
5. io_sync=[0..2]
    * Controls level of synchronization implied by IO operations
    * 0 - no synchronization
    * 1 by default - reasonable level of synchronization (write->read on the same fd)
    * 2 - global synchronization of all IO operations
6. history_size=[2..7]
    * 2 by default
7. flush_memory_ms=[1000..20000]
    * This can reduce memory consumption, but may be false reports
    * Flush information about previous memory accesses every X ms
    * 0 ms by default

### DSAN_OPTIONS

1. second_deadlock_stack=1
    * Report two stacks per edge instead of one
      (where edge destination mutex is locked and where edge source mutex is locked)
    * 0 by default

## Undefined Behavior Sanitizer

TODO

## Memory Sanitizer

TODO
