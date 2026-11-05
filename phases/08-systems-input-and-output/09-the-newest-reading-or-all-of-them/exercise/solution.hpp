#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>
#include <rc/io/sequence.hpp>

using rc::io::BoundedPort;
using rc::io::Link;

// What one call to drain_latest found.
struct Drained {
  bool got = false;               // a valid message was found, and it is in the buffer you gave
  std::size_t size = 0;           // how many bytes of it
  std::size_t skipped = 0;        // older valid messages passed over because a newer one was waiting
  std::size_t oversized = 0;      // valid messages too big for the buffer, which were not copied
  bool more = false;              // stopped at the limit, so there may be more waiting
};

// Reads everything that is waiting and keeps the newest.
inline Drained drain_latest(Link& link, double now, rc::span<std::uint8_t> out, std::size_t limit) {
  // TODO: call link.poll(now) until it says nothing is waiting, or until you
  // have taken `limit` messages, whichever comes first.
  //
  // The Link's message is only good until the next poll, so copy each into `out`
  // as it arrives instead of remembering a view of it.
  //
  //   - a message that does not fit in `out` is not copied: count it in
  //     oversized and carry on. Copying it would write past the buffer.
  //   - one that fits replaces whatever was held. If something was held, count
  //     it in skipped, because a newer message has taken its place.
  //   - `more` is true when you stopped because of the limit and not because
  //     nothing was left. A limit of zero never polls and never says more.
  //
  // The limit is the part that matters in a real loop. A device that never stops
  // sending would keep an unbounded drain in the loop for ever.
  (void)link;
  (void)now;
  (void)out;
  (void)limit;
  (void)std::memcpy;
  return Drained{};
}

#endif  // LESSON_SOLUTION_HPP
