#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// A disk with two slots, each holding a whole release as a list of chunks, the hash that was stored for it and its version, and a boot pointer that says which slot to try first.
struct Slot {
  std::vector<std::string> chunks;
  std::uint64_t stored_hash = 0;
  std::string version;
};

struct Disk {
  Slot slot[2];
  int boot_pointer = 0;
};

// FNV-1a over the chunks, each followed by a byte of value 0xFF so that moving a boundary between chunks changes the hash.
inline std::uint64_t image_hash(const std::vector<std::string>& chunks) {
  std::uint64_t h = 14695981039346656037ULL;
  auto mix = [&h](unsigned char b) {
    h ^= b;
    h *= 1099511628211ULL;
  };
  for (const std::string& c : chunks) {
    for (char ch : c) mix(static_cast<unsigned char>(ch));
    mix(0xFF);
  }
  return h;
}

// A slot is valid if it has chunks and the hash stored for it is the hash of what it holds.
inline bool slot_valid(const Slot& s) {
  // TODO: true if there are chunks and stored_hash equals image_hash(chunks)
  (void)s;
  return false;
}

struct Boot {
  bool ok = false;
  int slot = -1;
  std::string version;
};

// What the robot boots: the slot the pointer names if it is valid, otherwise the other one if that is valid, otherwise nothing (ok false, slot -1). A pointer that is not 0 or 1 is read as 0.
inline Boot boot(const Disk& d) {
  // TODO: try the slot the pointer names (a pointer other than 1 is 0), then the other; the first valid one boots, with its slot and version; none: ok false and slot -1
  (void)d;
  return Boot();
}

// One write that the disk does completely or not at all. Writing a chunk, writing the hash (with the version) and moving the boot pointer are each one step; a power cut falls between steps and never inside one.
enum class StepKind { write_chunk, write_hash, switch_pointer };

struct WriteStep {
  StepKind kind = StepKind::write_chunk;
  int slot = 0;
  std::size_t chunk = 0;
  std::string data;            // the chunk, or the version
  std::uint64_t hash = 0;      // for write_hash
};

inline void apply(Disk& d, const WriteStep& s) {
  // TODO: slot 1 if s.slot is 1 else 0. write_chunk: grow the chunks if needed and set chunk s.chunk to s.data. write_hash: stored_hash = s.hash and version = s.data. switch_pointer: boot_pointer = that slot
  (void)d;
  (void)s;
}

// The first way to install: overwrite the slot the robot is running from. Its steps are, in order, the chunks written one by one into the slot that is booted now (slot 0 if nothing boots), then the hash and version of the new image. The boot pointer is left alone.
inline std::vector<WriteStep> plan_in_place(const Disk& d, const std::vector<std::string>& image, const std::string& version) {
  // TODO: the target is the booted slot (0 if nothing boots); one write_chunk step per chunk of the image, in order, then one write_hash step with the version and image_hash(image); no pointer step
  (void)d;
  (void)image;
  (void)version;
  return std::vector<WriteStep>();
}

// The second: write the slot that is NOT running. Its steps are the chunks written one by one into the other slot (slot 0 if nothing boots), then that slot's hash and version (the commit), and last of all the boot pointer moved to it.
// Nothing before the last step changes what the robot boots, and the last is one write.
inline std::vector<WriteStep> plan_ab(const Disk& d, const std::vector<std::string>& image, const std::string& version) {
  // TODO: the target is the slot that is not booted (0 if nothing boots); one write_chunk per chunk, then write_hash with the version and image_hash(image), then switch_pointer to the target
  (void)d;
  (void)image;
  (void)version;
  return std::vector<WriteStep>();
}

#endif  // LESSON_SOLUTION_HPP
