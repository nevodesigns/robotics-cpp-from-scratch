#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// What "compiling" does here: the output says exactly what went into it, so a stale one is recognisable.
std::string compile(const std::string& name, const std::string& text, const std::string& flags) {
  return "obj[" + name + "|" + text + "|" + flags + "]";
}

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

}  // namespace

RC_TEST("the timestamp rule rebuilds when the source is newer, and not when it is the same age") {
  RC_CHECK(needs_rebuild_by_time(200, 100));
  RC_CHECK(!needs_rebuild_by_time(100, 200));
  RC_CHECK(!needs_rebuild_by_time(100, 100));
}

RC_TEST("touching a file rebuilds it by time and is a hit by content") {
  BuildCache cache;
  const Id key = build_key("motor.cpp", "int run() { return 1; }", "-O2");
  cache.store(key, compile("motor.cpp", "int run() { return 1; }", "-O2"));

  // touch motor.cpp: the time moves on and the text does not
  const bool by_time = needs_rebuild_by_time(500, 100);
  std::string out;
  const bool hit = cache.lookup(build_key("motor.cpp", "int run() { return 1; }", "-O2"), &out);
  std::cout << "  after a touch: timestamps say rebuild " << (by_time ? "yes" : "no") << ", content says cache hit "
            << (hit ? "yes" : "no") << "\n";
  RC_CHECK(by_time);
  RC_CHECK(hit);
  RC_CHECK_EQ(out, compile("motor.cpp", "int run() { return 1; }", "-O2"));
}

RC_TEST("build, edit, revert: the timestamp rule compiles three times and the content cache twice") {
  BuildCache cache;
  int time_rebuilds = 0;
  int content_builds = 0;
  long source_time = 100;
  long output_time = 100;

  const std::string original = "int speed() { return 10; }";
  const std::string edited = "int speed() { return 20; }";
  const std::string steps[] = {original, edited, original};      // build, edit, revert
  for (const std::string& text : steps) {
    // timestamps: every change of the file moves its time forward
    source_time += 10;
    if (needs_rebuild_by_time(source_time, output_time)) {
      ++time_rebuilds;
      output_time = source_time + 1;
    }
    // content
    const Id key = build_key("speed.cpp", text.c_str(), "-O2");
    std::string out;
    if (!cache.lookup(key, &out)) {
      ++content_builds;
      cache.store(key, compile("speed.cpp", text, "-O2"));
    }
  }
  std::cout << "  build, edit, revert: " << time_rebuilds << " compiles by timestamp, " << content_builds
            << " by content (the revert was a hit: " << cache.hits() << ")\n";
  RC_CHECK_EQ(time_rebuilds, 3);
  RC_CHECK_EQ(content_builds, 2);
  RC_CHECK_EQ(cache.hits(), 1);
}

RC_TEST("a file restored with an old timestamp is stale by time and caught by content") {
  BuildCache cache;
  const std::string old_text = "int limit() { return 5; }";
  cache.store(build_key("limit.cpp", old_text.c_str(), "-O2"), compile("limit.cpp", old_text, "-O2"));
  const long output_time = 1000;                    // the object was built at 1000

  // A backup is restored: the file has different text and an old timestamp, from before the object was built.
  const std::string restored_text = "int limit() { return 50; }";
  const long restored_time = 400;

  const bool by_time = needs_rebuild_by_time(restored_time, output_time);
  std::string out;
  const bool hit = cache.lookup(build_key("limit.cpp", restored_text.c_str(), "-O2"), &out);
  std::cout << "  text changed, timestamp older than the object: by time rebuild " << (by_time ? "yes" : "no")
            << " (the old object would be used), by content hit " << (hit ? "yes" : "no") << "\n";
  RC_CHECK(!by_time);        // the timestamp rule says the old object is fine, and it is wrong
  RC_CHECK(!hit);            // the content rule does not find it

  // And both rules still work when nothing is odd: the unchanged text is found, and a newer source is rebuilt.
  RC_CHECK(cache.lookup(build_key("limit.cpp", old_text.c_str(), "-O2"), &out));
  RC_CHECK(needs_rebuild_by_time(2000, output_time));
}

RC_TEST("the same source with different flags, or a different name, is a different build") {
  const char* text = "int f() { return 1; }";
  const Id a = build_key("a.cpp", text, "-O2");
  RC_CHECK(a == build_key("a.cpp", text, "-O2"));
  RC_CHECK(a != build_key("a.cpp", text, "-O0"));
  RC_CHECK(a != build_key("a.cpp", text, "-O2 -DNDEBUG"));
  RC_CHECK(a != build_key("b.cpp", text, "-O2"));
  RC_CHECK(a != build_key("a.cpp", "int f() { return 2; }", "-O2"));

  // Swapping what two fields hold does not give the same key.
  RC_CHECK(build_key("x", "y", "z") != build_key("y", "x", "z"));
  RC_CHECK(build_key("x", "y", "z") != build_key("z", "y", "x"));

  BuildCache cache;
  cache.store(a, "debug object");
  std::string out;
  RC_CHECK(!cache.lookup(build_key("a.cpp", text, "-O0"), &out));
  RC_CHECK(cache.lookup(a, &out));
  RC_CHECK_EQ(out, std::string("debug object"));
  RC_CHECK_EQ(cache.hits(), 1);
  RC_CHECK_EQ(cache.misses(), 1);
  RC_CHECK_EQ(cache.size(), static_cast<std::size_t>(1));
}

RC_TEST("over three hundred random edits the cache never serves a stale object, and saves most of the compiles") {
  Rng rng{31337};
  BuildCache cache;
  const char* versions[] = {"v1", "v2", "v3", "v4"};
  const char* flag_sets[] = {"-O0", "-O2"};
  int compiles = 0;
  int always_rebuild = 0;
  int stale = 0;

  for (int i = 0; i < 300; ++i) {
    const std::string text = versions[rng.next() % 4];
    const std::string flags = flag_sets[rng.next() % 2];
    ++always_rebuild;
    const Id key = build_key("unit.cpp", text.c_str(), flags.c_str());
    std::string out;
    if (!cache.lookup(key, &out)) {
      ++compiles;
      out = compile("unit.cpp", text, flags);
      cache.store(key, out);
    }
    if (out != compile("unit.cpp", text, flags)) ++stale;
  }
  std::cout << "  300 builds of a file with 4 versions and 2 sets of flags: " << compiles << " compiles instead of "
            << always_rebuild << ", " << stale << " stale objects served, " << cache.hits() << " hits\n\n";
  RC_CHECK_EQ(stale, 0);
  RC_CHECK_EQ(compiles, 8);
  RC_CHECK_EQ(cache.hits(), 292);
  RC_CHECK_EQ(cache.size(), static_cast<std::size_t>(8));
}
