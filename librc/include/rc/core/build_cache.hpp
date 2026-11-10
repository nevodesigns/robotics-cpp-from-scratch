// rc/core/build_cache.hpp
//
// When to rebuild, and a cache that cannot go stale, from lesson 04-11, graduated.
//
// A timestamp says only that a file was written. Touching a file rebuilds it by time and is a cache hit by content. Editing and reverting compiled three times by timestamp
// and twice by content. A file restored from a backup with an old timestamp and different text was judged up to date by time, which is wrong, and was not found by content. A key made of the
// name, the text and the flags separates builds that differ in any of them. Over 300 random builds of a file with four versions and two sets of flags, the cache compiled 8 times instead of 300,
// hit 292 times and served 0 stale objects.

#ifndef RC_CORE_BUILD_CACHE_HPP
#define RC_CORE_BUILD_CACHE_HPP

#include <map>
#include <string>

#include <rc/core/content_id.hpp>

namespace rc {
namespace core {

// The decision a simple build tool makes: is the output older than the source?
//
// Times are whole seconds since some starting point. The output is stale when the source was changed
// after it was made. Equal times count as up to date, since a source and the output built from it a
// moment later can share a second.
inline bool needs_rebuild_by_time(long source_time, long output_time) { return source_time > output_time; }

// What a build output depends on: the file's name, what is in it, and the flags it was built with.
// Change any of the three and the output may differ, so the key must too.
inline Id build_key(const char* name, const char* source_text, const char* flags) {
  const Id a = content_id(name);
  const Id b = content_id(source_text);
  const Id c = content_id(flags);
  return scramble(a ^ (b * 0x9E3779B97F4A7C15ULL) ^ (c * 0xC2B2AE3D27D4EB4FULL));
}

// Outputs already built, found by key.
class BuildCache {
 public:
  // Copy the stored output into *output and return true, or return false and count a miss.
  bool lookup(Id key, std::string* output) {
    const std::map<Id, std::string>::const_iterator it = outputs_.find(key);
    if (it == outputs_.end()) {
      ++misses_;
      return false;
    }
    ++hits_;
    *output = it->second;
    return true;
  }

  void store(Id key, const std::string& output) { outputs_[key] = output; }

  int hits() const { return hits_; }
  int misses() const { return misses_; }
  std::size_t size() const { return outputs_.size(); }

 private:
  std::map<Id, std::string> outputs_;
  int hits_ = 0;
  int misses_ = 0;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BUILD_CACHE_HPP
