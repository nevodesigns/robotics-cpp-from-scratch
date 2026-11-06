#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>

// A 64 bit number, which is as many bits as the identifier below has.
using Id = unsigned long long;

// A name for a piece of text, made from the text and nothing else.
//
// FNV-1a: start from a fixed number, and for each byte xor it in and multiply by a fixed prime.
// The same bytes give the same number on every machine and every run, and a different byte
// almost always gives a different one.
inline Id content_id(const char* text) {
  // TODO: FNV-1a. hash starts at 14695981039346656037ULL. For each byte of text, up to the
  // terminating '\0': hash ^= the byte (as an unsigned char, widened to Id), then
  // hash *= 1099511628211ULL. Return hash.
  (void)text;
  return 0;
}

// How many of the 64 bits two names disagree in.
inline int differing_bits(Id a, Id b) {
  // TODO: count the 1 bits of a ^ b. Test the lowest bit, shift right, repeat until nothing is left.
  (void)a;
  (void)b;
  return 0;
}

// One file in a snapshot: what it is called and what is in it.
struct File {
  const char* name;
  const char* text;
};

// A final scramble so that nearby inputs do not give nearby outputs (splitmix64).
inline Id scramble(Id x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

// The name of one file in one place: its name and its content together.
inline Id file_id(const File& file) {
  return scramble(content_id(file.name) ^ (content_id(file.text) * 0x9E3779B97F4A7C15ULL));
}

// The name of a whole snapshot, made from the names of its files.
//
// The order the files are listed in must not matter, since a folder has no order, so the
// names are combined with xor, which does not care. The count goes in as well so that an empty
// snapshot and a snapshot of cancelling files are not the same thing.
inline Id snapshot_id(const File* files, int count) {
  // TODO: xor the file_id of every file into one number, then return
  // scramble(combined ^ static_cast<Id>(count)). Xor, because it does not care what order the
  // files were listed in.
  (void)files;
  (void)count;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
