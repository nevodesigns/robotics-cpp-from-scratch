#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr std::uint8_t kStart = 0xAA;
constexpr std::size_t kStorageBytes = 16;

// A frame on the wire: start, length, payload.
std::vector<std::uint8_t> frame(const std::vector<std::uint8_t>& payload) {
  std::vector<std::uint8_t> wire{kStart, static_cast<std::uint8_t>(payload.size())};
  wire.insert(wire.end(), payload.begin(), payload.end());
  return wire;
}

// Feeds the whole stream and collects every payload the parser completes,
// which is what a caller actually does with it.
std::vector<std::vector<std::uint8_t>> parse_all(FrameParser& parser,
                                                 const std::vector<std::uint8_t>& stream) {
  std::vector<std::vector<std::uint8_t>> found;
  for (const std::uint8_t byte : stream) {
    if (!parser.push(byte)) continue;
    const rc::span<const std::uint8_t> got = parser.payload();
    found.push_back(std::vector<std::uint8_t>(got.data(), got.data() + got.size()));
  }
  return found;
}

}  // namespace

RC_TEST("a whole frame arriving at once is found") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  const auto found = parse_all(parser, frame({1, 2, 3}));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(found[0][0]), 1);
  RC_CHECK_EQ(static_cast<int>(found[0][2]), 3);
  RC_CHECK_EQ(parser.discarded(), static_cast<std::size_t>(0));
}

RC_TEST("the same frame is found however the stream is split") {
  // The check that catches a parser assuming one read is one message. Every
  // split point is tried, including between the start byte and the length,
  // which is the one that is hardest to reproduce on a desk and certain on a
  // robot.
  const std::vector<std::uint8_t> wire = frame({0x48, 0x65, 0x6C, 0x6C, 0x6F});

  for (std::size_t cut = 0; cut <= wire.size(); ++cut) {
    std::vector<std::uint8_t> storage(kStorageBytes, 0);
    FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

    std::vector<std::vector<std::uint8_t>> found;
    const std::vector<std::uint8_t> first(wire.begin(), wire.begin() + static_cast<long>(cut));
    const std::vector<std::uint8_t> second(wire.begin() + static_cast<long>(cut), wire.end());

    for (const auto& chunk : {first, second}) {
      const auto part = parse_all(parser, chunk);
      found.insert(found.end(), part.begin(), part.end());
    }

    RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
    RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(5));
    RC_CHECK_EQ(static_cast<int>(found[0][0]), 0x48);
    RC_CHECK_EQ(static_cast<int>(found[0][4]), 0x6F);
  }
}

RC_TEST("two frames in one chunk are both found") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  std::vector<std::uint8_t> stream = frame({1, 2});
  const std::vector<std::uint8_t> second = frame({9});
  stream.insert(stream.end(), second.begin(), second.end());

  const auto found = parse_all(parser, stream);
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(found[1].size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(found[1][0]), 9);
}

RC_TEST("an empty payload is a complete frame, not a stall") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  const auto found = parse_all(parser, frame({}));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(0));
}

RC_TEST("noise before a frame is discarded and counted, and the frame is still found") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  std::vector<std::uint8_t> stream{0x01, 0x02, 0x03};   // a cable plugged in mid transmission
  const std::vector<std::uint8_t> good = frame({7, 8});
  stream.insert(stream.end(), good.begin(), good.end());

  const auto found = parse_all(parser, stream);
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(found[0][0]), 7);
  RC_CHECK_EQ(parser.discarded(), static_cast<std::size_t>(3));
}

RC_TEST("a frame claiming more than fits is refused, and the next one is still found") {
  // The check that catches a parser trusting the length field. Storage is
  // sixteen bytes and the claim is two hundred, which is not a truncation to
  // recover from: a frame that does not fit is not a frame that was understood.
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  std::vector<std::uint8_t> stream{kStart, 200};
  const std::vector<std::uint8_t> good = frame({4, 5, 6});
  stream.insert(stream.end(), good.begin(), good.end());

  const auto found = parse_all(parser, stream);
  RC_CHECK_EQ(parser.oversized(), static_cast<std::size_t>(1));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(found[0][0]), 4);
}

RC_TEST("a payload exactly the size of the storage is accepted") {
  // The boundary the off by one lives on. Sixteen bytes into sixteen bytes of
  // storage fits, and a parser refusing it silently loses every largest frame.
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  const std::vector<std::uint8_t> payload(kStorageBytes, 0x5A);
  const auto found = parse_all(parser, frame(payload));

  RC_CHECK_EQ(parser.oversized(), static_cast<std::size_t>(0));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found[0].size(), kStorageBytes);
  RC_CHECK_EQ(static_cast<int>(found[0][kStorageBytes - 1]), 0x5A);
}

RC_TEST("one byte more than the storage is refused") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  const std::vector<std::uint8_t> payload(kStorageBytes + 1, 0x5A);
  const auto found = parse_all(parser, frame(payload));

  RC_CHECK_EQ(parser.oversized(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found.size(), static_cast<std::size_t>(0));
}

RC_TEST("a truncated frame does not swallow the frame after it forever") {
  // A frame claims five bytes, the sender dies after two, and a good frame
  // follows. The parser consumes the good frame's leading bytes finishing the
  // broken one, which is unavoidable with a length prefix and is why the next
  // lesson adds a checksum. What matters is that it recovers rather than
  // staying out of step for good.
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  std::vector<std::uint8_t> stream{kStart, 5, 1, 2};   // three bytes short
  for (int i = 0; i < 3; ++i) {
    const std::vector<std::uint8_t> good = frame({0x11, 0x22});
    stream.insert(stream.end(), good.begin(), good.end());
  }

  const auto found = parse_all(parser, stream);
  RC_CHECK(!found.empty());

  // The last frame of the stream is intact, whatever happened at the start.
  const std::vector<std::uint8_t>& last = found.back();
  RC_REQUIRE_EQ(last.size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(static_cast<int>(last[0]), 0x11);
  RC_CHECK_EQ(static_cast<int>(last[1]), 0x22);
}

RC_TEST("reset abandons a partial frame without losing the parser") {
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  parser.push(kStart);
  parser.push(4);
  parser.push(1);
  parser.reset();

  const auto found = parse_all(parser, frame({3}));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(static_cast<int>(found[0][0]), 3);
}

RC_TEST("a start byte inside a payload does not break the frame carrying it") {
  // Framing narrows the problem and does not close it. Inside a length
  // prefixed payload the start byte is data, and the parser must not treat it
  // as anything else.
  std::vector<std::uint8_t> storage(kStorageBytes, 0);
  FrameParser parser(kStart, rc::span<std::uint8_t>(storage.data(), storage.size()));

  const auto found = parse_all(parser, frame({1, kStart, 3}));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(found[0].size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(found[0][1]), static_cast<int>(kStart));
}
