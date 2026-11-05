// rc/io/drain.hpp
//
// The drain from lesson 08-09, graduated.
//
// A Link hands back one message per poll, and a loop that reads one a tick is
// correct only while the device sends no faster than the loop runs. Faster, and
// the queue grows to the size of the driver's buffer, which then keeps what it
// holds and drops what arrives. The loop acts on the oldest data there is.
//
// Measured, a 500 Hz device, a 100 Hz loop and a 4096 byte buffer, the data the
// robot acted on was 2472 ms old on average and 3741 ms at worst. Draining and
// acting on the newest made it 1 ms.
//
// The newest is right for a state and wrong for a change. A message that says how
// much something moved has to be added up, and a skipped one is lost.

#ifndef RC_IO_DRAIN_HPP
#define RC_IO_DRAIN_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <rc/core/compat.hpp>
#include <rc/io/link.hpp>

namespace rc {
namespace io {

// What one call to drain_latest found.
struct Drained {
  bool got = false;               // a valid message was found, and it is in the buffer you gave
  std::size_t size = 0;           // how many bytes of it
  std::size_t skipped = 0;        // older valid messages passed over because a newer one was waiting
  std::size_t oversized = 0;      // valid messages too big for the buffer, which were not copied
  bool more = false;              // stopped at the limit, so there may be more waiting
};

// Reads everything that is waiting and keeps the newest.
//
// The Link hands back one message per poll, which makes the loop
//
//   if (link.poll(now)) act(link.message());
//
// look finished. It is not. A device that sends faster than the loop runs
// leaves a queue behind every call, the queue grows until the driver's buffer is
// full, and from then on the loop acts on what the device said seconds ago while
// the driver throws away what it is saying now.
//
// The limit is what keeps this from being its own bug. A device that never stops
// sending would keep an unbounded drain in the loop for ever, and the loop has a
// deadline.
inline Drained drain_latest(Link& link, double now, rc::span<std::uint8_t> out, std::size_t limit) {
  Drained result;
  std::size_t taken = 0;

  while (taken < limit && link.poll(now)) {
    ++taken;

    // The Link's message is only good until the next poll, so the newest is
    // copied out as it goes, and not remembered as a view.
    const rc::span<const std::uint8_t> message = link.message();
    if (message.size() > out.size()) {
      ++result.oversized;
      continue;
    }

    if (result.got) ++result.skipped;   // the one held until now has been replaced
    if (message.size() != 0) std::memcpy(out.data(), message.data(), message.size());
    result.size = message.size();
    result.got = true;
  }

  result.more = limit != 0 && taken == limit;
  return result;
}

}  // namespace io
}  // namespace rc

#endif  // RC_IO_DRAIN_HPP
