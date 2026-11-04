#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Your programs have printed things and ended. This lesson is about what the
// printing and the ending actually mean to whatever ran your program, which
// is sometimes you at a terminal and increasingly often a script, a build
// server or the thing that starts your robot when power comes up.
//
// There are exactly two ways out, and they are smaller than you would guess.
//
// Two streams of text. One number.

// The one number: what main hands back.
//
// Zero means it worked. Anything else means it did not. That convention is
// older than C++ and every shell, build system and process supervisor in
// existence is built on it.
//
// The trap is that the number is not an int, whatever the signature of main
// says. Only the low eight bits survive, so the value a shell sees is your
// return value modulo 256, and lesson 00-04's arithmetic applies in a place
// you were not expecting it. Measured:
//
//   main returned      0  ->  shell saw    0
//   main returned      1  ->  shell saw    1
//   main returned      2  ->  shell saw    2
//   main returned    255  ->  shell saw  255
//   main returned    256  ->  shell saw    0        <- reports success
//   main returned    257  ->  shell saw    1
//   main returned     -1  ->  shell saw  255
//
// The fifth row is the one that bites. `return number_of_failures;` is a
// natural thing to write and it reports success at exactly 256 failures.
//
// So collapse the count deliberately rather than letting the operating system
// do it for you.

// The conventional codes. Named, because 0 and 1 and 2 scattered through the
// bottom of main are three magic numbers nobody can grep for.
const int kSuccess = 0;   // it did what it was asked
const int kFailed = 1;    // it tried and could not
const int kMisused = 2;   // it was asked for something that makes no sense

// Turns any count of problems into a status.
//
// One problem and a thousand problems are the same answer to the only
// question a shell asks, which is whether to carry on. The count belongs in a
// message on the second stream, where a person can read it, and not in the
// number, where it would be truncated.
inline int status_for(int problem_count) {
  return problem_count == 0 ? kSuccess : kFailed;
}

// The same, for the case where the program was asked for something
// impossible rather than failing at something possible. Keeping those apart
// is what lets a script tell a broken input from a broken machine.
inline int status_for_misuse() { return kMisused; }

// Whether a status means the program succeeded, written once so that no
// caller has to remember which way round zero is. This is the one piece of
// the convention that is genuinely counter-intuitive: zero is true here, and
// every other number is false.
inline bool succeeded(int status) { return status == kSuccess; }

#endif  // LESSON_SOLUTION_HPP
