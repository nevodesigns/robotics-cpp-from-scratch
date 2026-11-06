#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <utility>
#include <vector>

// Everything that has happened to every Packet since the last reset.
struct Tally {
  int made = 0;        // built from an id
  int copies = 0;      // built as a copy of another
  int moves = 0;       // built by taking another's contents
  int destroyed = 0;   // gone
};

inline Tally& tally() {
  static Tally t;
  return t;
}

inline void reset_tally() { tally() = Tally{}; }

// A packet of data that counts what is done to it.
//
// The payload is one number so the arithmetic stays visible. What matters is the five special members,
// and above all one word on the move constructor.
class Packet {
 public:
  explicit Packet(int id) : id_(id) { ++tally().made; }

  Packet(const Packet& other) : id_(other.id_) { ++tally().copies; }

  // noexcept is a promise that this cannot throw, and a vector that wants to grow checks for it. A move
  // that might throw cannot be used while growing, since a throw half way would leave elements in two
  // places, so a vector falls back to copying every element. Leave the word off and growth
  // quietly copies.
  Packet(Packet&& other) noexcept : id_(other.id_) {
    other.id_ = -1;   // the one it came from has given its contents away
    ++tally().moves;
  }

  Packet& operator=(const Packet& other) {
    id_ = other.id_;
    ++tally().copies;
    return *this;
  }

  Packet& operator=(Packet&& other) noexcept {
    id_ = other.id_;
    other.id_ = -1;
    ++tally().moves;
    return *this;
  }

  ~Packet() { ++tally().destroyed; }

  int id() const { return id_; }

 private:
  int id_;
};

// Three ways to fill a vector with n packets.

// No reserve, push_back of a packet made on the spot.
inline std::vector<Packet> build_plain(int n) {
  std::vector<Packet> v;
  for (int i = 0; i < n; ++i) v.push_back(Packet(i));
  return v;
}

// Say how many there will be, then push_back the same way.
inline std::vector<Packet> build_reserved(int n) {
  std::vector<Packet> v;
  v.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) v.push_back(Packet(i));
  return v;
}

// Say how many there will be, and build each one inside the vector.
inline std::vector<Packet> build_emplaced(int n) {
  std::vector<Packet> v;
  v.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) v.emplace_back(i);
  return v;
}

// The same total, asked two ways. These are given.
inline long total_by_value(std::vector<Packet> v) {
  long sum = 0;
  for (const Packet& p : v) sum += p.id();
  return sum;
}

inline long total_by_reference(const std::vector<Packet>& v) {
  long sum = 0;
  for (const Packet& p : v) sum += p.id();
  return sum;
}

#endif  // LESSON_SOLUTION_HPP
