// rc/core/content_id.hpp
//
// A name made from the content, from lesson 00-11, graduated.
//
// Version control does not compare files byte by byte every time. It gives each piece of content a name made from the
// content, and equal names mean equal content. FNV-1a gives a 64 bit name. Changing one character of a 200 character text,
// in every place in turn, changed a mean of 32.4 of the 64 bits (between 16 and 44), a thousand different texts got a thousand
// different names, and two versions of a hundred files with one edited had 101 different contents between them, not 200.

#ifndef RC_CORE_CONTENT_ID_HPP
#define RC_CORE_CONTENT_ID_HPP

#include <cstddef>

namespace rc {
namespace core {

// A 64 bit number, which is as many bits as the identifier below has.
using Id = unsigned long long;

// A name for a piece of text, made from the text and nothing else.
//
// FNV-1a: start from a fixed number, and for each byte xor it in and multiply by a fixed prime.
// The same bytes give the same number on every machine and every run, and a different byte
// almost always gives a different one.
inline Id content_id(const char* text) {
  Id hash = 14695981039346656037ULL;
  for (std::size_t i = 0; text[i] != '\0'; ++i) {
    hash ^= static_cast<Id>(static_cast<unsigned char>(text[i]));
    hash *= 1099511628211ULL;
  }
  return hash;
}

// How many of the 64 bits two names disagree in.
inline int differing_bits(Id a, Id b) {
  Id x = a ^ b;
  int count = 0;
  while (x != 0) {
    count += static_cast<int>(x & 1ULL);
    x >>= 1;
  }
  return count;
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
  Id combined = 0;
  for (int i = 0; i < count; ++i) combined ^= file_id(files[i]);
  return scramble(combined ^ static_cast<Id>(count));
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CONTENT_ID_HPP
