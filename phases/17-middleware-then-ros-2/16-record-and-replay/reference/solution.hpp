#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <rc/bus/message.hpp>
#include <rc/bus/topic_bus.hpp>

using rc::bus::ByteReader;
using rc::bus::ByteWriter;
using rc::bus::TopicBus;

// One recorded message: where it was published, when, and what it said.
struct BagEntry {
  std::string topic;
  double stamp = 0.0;
  std::string payload;
};

enum class BagStatus { ok, bad_magic, truncated, too_many };

// A recording of messages in the order they were made.
class Bag {
 public:
  void add(const std::string& topic, double stamp, const std::string& payload) { entries_.push_back(BagEntry{topic, stamp, payload}); }

  const std::vector<BagEntry>& entries() const { return entries_; }
  double duration() const { return entries_.size() < 2 ? 0.0 : entries_.back().stamp - entries_.front().stamp; }
  int count(const std::string& topic) const {
    int n = 0;
    for (const BagEntry& e : entries_)
      if (e.topic == topic) ++n;
    return n;
  }

  // As bytes: the six characters RCBAG1, a count of entries (four bytes), then for each its topic, its stamp (a double) and its payload.
  std::vector<std::uint8_t> serialize() const {
    ByteWriter w;
    const char magic[] = {'R', 'C', 'B', 'A', 'G', '1'};
    for (char c : magic) w.put_u8(static_cast<std::uint8_t>(c));
    w.put_u32(static_cast<std::uint32_t>(entries_.size()));
    for (const BagEntry& e : entries_) {
      w.put_string(e.topic);
      w.put_f64(e.stamp);
      w.put_string(e.payload);
    }
    return w.bytes();
  }

  // Read a bag back. A file that does not start with the magic is bad_magic; one that ends before it should is truncated; one that claims more
  // entries than there could possibly be bytes for is too_many. In any failure `out` is left empty.
  static BagStatus parse(const std::vector<std::uint8_t>& bytes, Bag& out) {
    out.entries_.clear();
    if (bytes.size() < 6) return BagStatus::bad_magic;
    const char magic[] = {'R', 'C', 'B', 'A', 'G', '1'};
    for (std::size_t i = 0; i < 6; ++i)
      if (bytes[i] != static_cast<std::uint8_t>(magic[i])) return BagStatus::bad_magic;
    ByteReader r(bytes.data() + 6, bytes.size() - 6);
    const std::uint32_t n = r.get_u32();
    if (!r.ok()) return BagStatus::truncated;
    // Every entry takes at least 12 bytes (two lengths and a double), so a count above what the rest of the file could hold is a lie.
    if (static_cast<std::size_t>(n) > r.remaining() / 12) return BagStatus::too_many;
    Bag parsed;
    for (std::uint32_t i = 0; i < n; ++i) {
      BagEntry e;
      e.topic = r.get_string();
      e.stamp = r.get_f64();
      e.payload = r.get_string();
      if (!r.ok()) return BagStatus::truncated;
      parsed.entries_.push_back(e);
    }
    out.entries_ = parsed.entries_;
    return BagStatus::ok;
  }

 private:
  std::vector<BagEntry> entries_;
};

// Puts a bag back on a bus at the pace it was recorded, times `rate`. Time is counted from the first message, whatever its stamp was.
class Player {
 public:
  Player(const Bag& bag, double rate) : bag_(bag), rate_(rate > 0.0 ? rate : 1.0) {}

  // Let `dt` seconds go by and publish every message that has come due, in order. Returns how many. Nothing is published early or twice.
  int step(double dt, TopicBus& bus) {
    if (bag_.entries().empty()) return 0;
    elapsed_ += dt;
    const double start = bag_.entries().front().stamp;
    int published = 0;
    while (next_ < bag_.entries().size()) {
      const BagEntry& e = bag_.entries()[next_];
      if ((e.stamp - start) / rate_ > elapsed_ + 1e-12) break;
      bus.publish(e.topic, e.payload);
      ++next_;
      ++published;
    }
    return published;
  }

  bool finished() const { return next_ >= bag_.entries().size(); }
  double elapsed() const { return elapsed_; }

 private:
  Bag bag_;
  double rate_ = 1.0;
  double elapsed_ = 0.0;
  std::size_t next_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
