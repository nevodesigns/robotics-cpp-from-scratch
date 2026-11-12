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
inline bool slot_valid(const Slot& s) { return !s.chunks.empty() && s.stored_hash == image_hash(s.chunks); }

struct Boot {
  bool ok = false;
  int slot = -1;
  std::string version;
};

// What the robot boots: the slot the pointer names if it is valid, otherwise the other one if that is valid, otherwise nothing (ok false, slot -1). A pointer that is not 0 or 1 is read as 0.
inline Boot boot(const Disk& d) {
  Boot b;
  const int first = d.boot_pointer == 1 ? 1 : 0;
  const int order[2] = {first, 1 - first};
  for (int i = 0; i < 2; ++i) {
    if (slot_valid(d.slot[order[i]])) {
      b.ok = true;
      b.slot = order[i];
      b.version = d.slot[order[i]].version;
      return b;
    }
  }
  return b;
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
  const int i = s.slot == 1 ? 1 : 0;
  switch (s.kind) {
    case StepKind::write_chunk:
      if (d.slot[i].chunks.size() <= s.chunk) d.slot[i].chunks.resize(s.chunk + 1);
      d.slot[i].chunks[s.chunk] = s.data;
      break;
    case StepKind::write_hash:
      d.slot[i].stored_hash = s.hash;
      d.slot[i].version = s.data;
      break;
    case StepKind::switch_pointer:
      d.boot_pointer = i;
      break;
  }
}

// The first way to install: overwrite the slot the robot is running from. Its steps are, in order, the chunks written one by one into the slot that is booted now (slot 0 if nothing boots), then the hash and version of the new image. The boot pointer is left alone.
inline std::vector<WriteStep> plan_in_place(const Disk& d, const std::vector<std::string>& image, const std::string& version) {
  const Boot b = boot(d);
  const int target = b.ok ? b.slot : 0;
  std::vector<WriteStep> steps;
  for (std::size_t c = 0; c < image.size(); ++c) steps.push_back({StepKind::write_chunk, target, c, image[c], 0});
  steps.push_back({StepKind::write_hash, target, 0, version, image_hash(image)});
  return steps;
}

// The second: write the slot that is NOT running. Its steps are the chunks written one by one into the other slot (slot 0 if nothing boots), then that slot's hash and version (the commit), and last of all the boot pointer moved to it.
// Nothing before the last step changes what the robot boots, and the last is one write.
inline std::vector<WriteStep> plan_ab(const Disk& d, const std::vector<std::string>& image, const std::string& version) {
  const Boot b = boot(d);
  const int target = b.ok ? 1 - b.slot : 0;
  std::vector<WriteStep> steps;
  for (std::size_t c = 0; c < image.size(); ++c) steps.push_back({StepKind::write_chunk, target, c, image[c], 0});
  steps.push_back({StepKind::write_hash, target, 0, version, image_hash(image)});
  steps.push_back({StepKind::switch_pointer, target, 0, "", 0});
  return steps;
}

#endif  // LESSON_SOLUTION_HPP
