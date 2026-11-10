#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <map>
#include <string>

#include <rc/core/content_id.hpp>

using rc::core::content_id;
using rc::core::Id;
using rc::core::scramble;

// The decision a simple build tool makes: is the output older than the source?
//
// Times are whole seconds since some starting point. The output is stale when the source was changed
// after it was made. Equal times count as up to date, since a source and the output built from it a
// moment later can share a second.
inline bool needs_rebuild_by_time(long source_time, long output_time) {
  // TODO: true when the source is newer than the output: source_time > output_time.
  (void)source_time;
  (void)output_time;
  return false;
}

// What a build output depends on: the file's name, what is in it, and the flags it was built with.
// Change any of the three and the output may differ, so the key must too.
inline Id build_key(const char* name, const char* source_text, const char* flags) {
  // TODO: content_id of each of the three, then
  //   scramble(a ^ (b * 0x9E3779B97F4A7C15ULL) ^ (c * 0xC2B2AE3D27D4EB4FULL)).
  // The multiplications keep the three from cancelling when two of them are equal.
  (void)name;
  (void)source_text;
  (void)flags;
  return 0;
}

// Outputs already built, found by key.
class BuildCache {
 public:
  // Copy the stored output into *output and return true, or return false and count a miss.
  bool lookup(Id key, std::string* output) {
    // TODO: find the key in outputs_. Not found: ++misses_ and return false. Found: ++hits_, copy
    // the stored text into *output and return true.
    (void)key;
    (void)output;
    (void)misses_;
    (void)hits_;
    return false;
  }

  void store(Id key, const std::string& output) {
    // TODO: outputs_[key] = output.
    (void)key;
    (void)output;
    (void)outputs_;
  }

  int hits() const { return hits_; }
  int misses() const { return misses_; }
  std::size_t size() const { return outputs_.size(); }

 private:
  std::map<Id, std::string> outputs_;
  int hits_ = 0;
  int misses_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
