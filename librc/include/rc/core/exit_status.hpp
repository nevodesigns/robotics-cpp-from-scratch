// rc/core/exit_status.hpp
//
// What a program tells whatever ran it, from lesson 00-09, graduated.
//
// There are exactly two ways out of a program, and they are smaller than you
// would guess. Two streams of text, and one number.
//
// The number is the only thing a shell, a build system or the supervisor that
// starts your robot can read. Zero means it worked, anything else means it
// did not, and that convention is older than C++.
//
// The trap is that it is not an int, whatever the signature of main says. Only
// the low eight bits survive, so what a shell sees is the return value modulo
// 256, and lesson 00-04's truncation applies somewhere nobody expects it.
// Measured:
//
//   main returned      0  ->  shell saw    0
//   main returned      1  ->  shell saw    1
//   main returned    255  ->  shell saw  255
//   main returned    256  ->  shell saw    0      <- reports success
//   main returned    257  ->  shell saw    1
//   main returned     -1  ->  shell saw  255
//
// `return number_of_failures;` is a natural thing to write and it reports
// success at exactly 256 failures. So status_for collapses the count on
// purpose rather than letting the operating system do it silently. The count
// belongs in a message on the second stream, where a person can read it.
//
// A death by signal is not an exit status at all. A segmentation fault was
// seen as 139, which is 128 plus SIGSEGV's 11, so a status above 128 usually
// means the program did not choose to stop.
//
// On the two streams, measured in the same lesson. Diagnostics belong on
// stderr, and the usual reason given is redirection hygiene, which is the
// less important one. The real reason is that stdout is block buffered when
// it is not a terminal, which is how it runs under every script and every
// continuous integration job, and a program that crashes loses whatever is
// still in that buffer.
//
// Four lines were printed before a deliberate segmentation fault, and three
// of them arrived: the write to std::cerr flushed what was waiting, because
// std::cerr is tied to std::cout, and the line printed after it was lost. So
// which of your diagnostics you see depends on where the unbuffered one was,
// and the line you never saw is not the line it stopped at.
//
// One further measurement worth having, because it is the reverse of the
// common advice. With printf and fprintf(stderr), the ordering really does
// break when stdout is not a terminal: stderr arrived first and stdout
// afterwards, in a block. With std::cout and std::cerr the order was kept,
// because of that tie. Audit rule L018 already forbids printf in this
// curriculum, and this is the measured reason rather than a style preference.

#ifndef RC_CORE_EXIT_STATUS_HPP
#define RC_CORE_EXIT_STATUS_HPP

namespace rc {
namespace core {

// The conventional codes, named, because 0 and 1 and 2 scattered through the
// bottom of main are three magic numbers nobody can grep for.
const int kSuccess = 0;   // it did what it was asked
const int kFailed = 1;    // it tried and could not
const int kMisused = 2;   // it was asked for something that makes no sense

// Turns any count of problems into a status.
//
// One problem and a thousand problems are the same answer to the only
// question a shell asks, which is whether to carry on. A negative count is a
// failure too: a count that came out negative is itself a problem and must
// not read as everything being fine.
inline int status_for(int problem_count) {
  return problem_count == 0 ? kSuccess : kFailed;
}

// For being asked something impossible rather than failing at something
// possible. Keeping those apart is what lets a script tell a broken input
// from a broken machine, which are two problems with two different people to
// tell about them.
inline int status_for_misuse() { return kMisused; }

// Whether a status means success, written once so no caller has to remember
// which way round zero is. This is the genuinely counter-intuitive part of
// the convention: zero is the success and every other number is a failure,
// which reads backwards from a bool.
inline bool succeeded(int status) { return status == kSuccess; }

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_EXIT_STATUS_HPP
