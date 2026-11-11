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
    // TODO: a ByteWriter: the six bytes R C B A G 1, put_u32 of the number of entries, then for each entry put_string(topic), put_f64(stamp), put_string(payload). Return w.bytes()
    return std::vector<std::uint8_t>();
  }

  // Read a bag back. A file that does not start with the magic is bad_magic; one that ends before it should is truncated; one that claims more
  // entries than there could possibly be bytes for is too_many. In any failure `out` is left empty.
  static BagStatus parse(const std::vector<std::uint8_t>& bytes, Bag& out) {
    // TODO: Clear out. Fewer than 6 bytes, or the first six are not RCBAG1: bad_magic. Then a ByteReader over the rest: get_u32 for the count; if that read failed, truncated.
    // A count larger than remaining() / 12 could not fit (an entry takes at least 12 bytes): too_many. Then read each entry (get_string, get_f64, get_string);
    // if the reader says it is not ok, truncated. Only when all are read put them in out and return ok.
    (void)bytes;
    (void)out;
    return BagStatus::truncated;
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
    // TODO: Nothing to do for an empty bag. Add dt to elapsed_. Time in the bag is counted from the first entry's stamp: an entry is due when (stamp - first stamp) / rate_ is no later than elapsed_
    // (allow 1e-12 of slack). Publish each due entry in turn on the bus, advance next_, and count it. Stop at the first entry not yet due. Return how many were published.
    (void)dt;
    (void)bus;
    (void)elapsed_;
    (void)next_;
    (void)rate_;
    return 0;
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
