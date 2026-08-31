#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>

#include <cstdint>
#include <deque>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kTimeout = 500.0;

// A port that misbehaves on request, which is the only way the awkward cases
// become tests rather than hopes.
class FakePort : public BytePort {
 public:
  void queue(const std::vector<std::uint8_t>& bytes) {
    for (const std::uint8_t b : bytes) incoming_.push_back(b);
  }

  // How many bytes a single read will hand back. One is the case that splits
  // every frame; zero means the port has nothing and says so.
  void set_read_chunk(std::size_t bytes) { read_chunk_ = bytes; }
  void set_write_chunk(std::size_t bytes) { write_chunk_ = bytes; }
  void fail_reads(int times) { failing_reads_ = times; }
  void fail_writes(int times) { failing_writes_ = times; }
  void stop_accepting_writes() { write_chunk_ = 0; }

  int read(rc::span<std::uint8_t> into) override {
    if (failing_reads_ > 0) { --failing_reads_; return -1; }
    if (incoming_.empty()) return 0;   // nothing has arrived, which is not news

    std::size_t take = incoming_.size();
    if (read_chunk_ > 0 && read_chunk_ < take) take = read_chunk_;
    if (take > into.size()) take = into.size();

    for (std::size_t i = 0; i < take; ++i) {
      into[i] = incoming_.front();
      incoming_.pop_front();
    }
    return static_cast<int>(take);
  }

  int write(rc::span<const std::uint8_t> from) override {
    if (failing_writes_ > 0) { --failing_writes_; return -1; }

    std::size_t take = from.size();
    if (write_chunk_ < take) take = write_chunk_;
    for (std::size_t i = 0; i < take; ++i) written_.push_back(from[i]);
    ++write_calls_;
    return static_cast<int>(take);
  }

  const std::vector<std::uint8_t>& written() const { return written_; }
  int write_calls() const { return write_calls_; }

 private:
  std::deque<std::uint8_t> incoming_;
  std::vector<std::uint8_t> written_;
  std::size_t read_chunk_ = 0;      // zero means as much as is available
  std::size_t write_chunk_ = 1024;  // plenty, until a test says otherwise
  int failing_reads_ = 0;
  int failing_writes_ = 0;
  int write_calls_ = 0;
};

// A frame as it appears on the wire: start, length, payload, crc.
std::vector<std::uint8_t> wire_frame(const std::vector<std::uint8_t>& body) {
  std::vector<std::uint8_t> out{Link::kStartByte,
                                static_cast<std::uint8_t>(body.size() + 1)};
  out.insert(out.end(), body.begin(), body.end());
  out.push_back(rc::io::crc8(rc::span<const std::uint8_t>(body.data(), body.size())));
  return out;
}

std::vector<std::uint8_t> storage_for_link() { return std::vector<std::uint8_t>(64, 0); }

}  // namespace

RC_TEST("a read of zero is not an error and not a disconnection") {
  // Nine reads in ten are zero on a link polled faster than it talks. A link
  // that reports failure on an idle bus is the most common way to get this
  // wrong.
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  for (int i = 0; i < 100; ++i) RC_CHECK(!link.poll(static_cast<double>(i)));

  RC_CHECK_EQ(link.port_errors(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(0));
  RC_CHECK(!link.stale(100.0));
}

RC_TEST("a whole frame arriving at once is delivered") {
  FakePort port;
  port.queue(wire_frame({1, 2, 3}));
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  RC_REQUIRE(link.poll(10.0));
  RC_CHECK_EQ(link.message().size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(link.message()[0]), 1);
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(1));
  RC_CHECK_NEAR(link.last_message_at(), 10.0, 1e-12);
}

RC_TEST("a frame arriving one byte per read is still delivered") {
  // The case a real port produces constantly and a desk never does.
  FakePort port;
  port.set_read_chunk(1);
  port.queue(wire_frame({0x11, 0x22, 0x33, 0x44}));

  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  bool got = false;
  for (int i = 0; i < 50 && !got; ++i) got = link.poll(static_cast<double>(i));

  RC_REQUIRE(got);
  RC_CHECK_EQ(link.message().size(), static_cast<std::size_t>(4));
  RC_CHECK_EQ(static_cast<int>(link.message()[3]), 0x44);
}

RC_TEST("two frames in one read are both delivered, one poll at a time") {
  FakePort port;
  port.queue(wire_frame({7}));
  port.queue(wire_frame({8, 9}));

  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  RC_REQUIRE(link.poll(1.0));
  RC_CHECK_EQ(static_cast<int>(link.message()[0]), 7);

  RC_REQUIRE(link.poll(2.0));
  RC_REQUIRE_EQ(link.message().size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(static_cast<int>(link.message()[1]), 9);
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(2));
}

RC_TEST("a corrupted frame is counted and does not become a message") {
  FakePort port;
  std::vector<std::uint8_t> bad = wire_frame({5, 6, 7});
  bad[3] = static_cast<std::uint8_t>(bad[3] ^ 0x40u);   // a noisy line
  port.queue(bad);

  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  RC_CHECK(!link.poll(5.0));
  RC_CHECK_EQ(link.checksum_failures(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(0));
}

RC_TEST("a failing read is counted as a port error, not as silence") {
  FakePort port;
  port.fail_reads(3);
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  for (int i = 0; i < 3; ++i) RC_CHECK(!link.poll(static_cast<double>(i)));
  RC_CHECK_EQ(link.port_errors(), static_cast<std::size_t>(3));
}

RC_TEST("noise that never becomes a frame leaves the link stale") {
  // The check that catches a watchdog fed by bytes. This port is producing
  // plenty of them and not one message, which is what a crashed device holding
  // its line looks like.
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  link.poll(0.0);   // starts the clock

  for (int i = 1; i <= 1000; ++i) {
    port.queue({0x01, 0x02, 0x03, 0x04});
    link.poll(static_cast<double>(i));
  }

  RC_CHECK(link.bytes_read() > 3000);
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(0));
  RC_CHECK(link.stale(1000.0));
  RC_CHECK_NEAR(link.last_message_at(), 0.0, 1e-12);
}

RC_TEST("a link that was healthy goes stale when the device stops talking") {
  FakePort port;
  port.queue(wire_frame({1}));
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  RC_REQUIRE(link.poll(100.0));
  RC_CHECK(!link.stale(500.0));
  RC_CHECK(!link.stale(600.0));    // exactly the timeout is not yet stale
  RC_CHECK(link.stale(601.0));
}

RC_TEST("a link is not stale before it has ever been polled") {
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  // Otherwise every link is born dead, and a robot refuses to start because
  // the clock reads later than zero.
  RC_CHECK(!link.stale(1e9));
}

RC_TEST("send writes a frame the link itself can read back") {
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> scratch(64, 0);
  const std::vector<std::uint8_t> body{0xDE, 0xAD};
  RC_REQUIRE(link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                       rc::span<std::uint8_t>(scratch.data(), scratch.size())));

  // Feed what was written back in, which is the cheapest round trip there is.
  FakePort loopback;
  loopback.queue(port.written());
  auto storage2 = storage_for_link();
  Link back(loopback, rc::span<std::uint8_t>(storage2.data(), storage2.size()), kTimeout);

  RC_REQUIRE(back.poll(1.0));
  RC_REQUIRE_EQ(back.message().size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(static_cast<int>(back.message()[0]), 0xDE);
  RC_CHECK_EQ(static_cast<int>(back.message()[1]), 0xAD);
}

RC_TEST("a write that accepts one byte at a time still sends the whole frame") {
  // The check that catches a partial write left unfinished. A truncated frame
  // is worse than none: the far end resynchronises onto the wrong byte.
  FakePort port;
  port.set_write_chunk(1);
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> scratch(64, 0);
  const std::vector<std::uint8_t> body{1, 2, 3, 4, 5};
  RC_REQUIRE(link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                       rc::span<std::uint8_t>(scratch.data(), scratch.size())));

  RC_CHECK_EQ(port.written().size(), body.size() + 3);
  RC_CHECK(port.write_calls() > 1);

  FakePort loopback;
  loopback.queue(port.written());
  auto storage2 = storage_for_link();
  Link back(loopback, rc::span<std::uint8_t>(storage2.data(), storage2.size()), kTimeout);
  RC_REQUIRE(back.poll(1.0));
  RC_CHECK_EQ(back.message().size(), static_cast<std::size_t>(5));
}

RC_TEST("a port that accepts nothing fails the send rather than reporting success") {
  FakePort port;
  port.stop_accepting_writes();
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> scratch(64, 0);
  const std::vector<std::uint8_t> body{1, 2, 3};
  RC_CHECK(!link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                      rc::span<std::uint8_t>(scratch.data(), scratch.size())));
  RC_CHECK_EQ(port.written().size(), static_cast<std::size_t>(0));
}

RC_TEST("a failing write is reported and counted") {
  FakePort port;
  port.fail_writes(1);
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> scratch(64, 0);
  const std::vector<std::uint8_t> body{1};
  RC_CHECK(!link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                      rc::span<std::uint8_t>(scratch.data(), scratch.size())));
  RC_CHECK_EQ(link.port_errors(), static_cast<std::size_t>(1));
}

RC_TEST("a scratch buffer too small is refused rather than overrun") {
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> scratch(4, 0);   // needs body plus three
  const std::vector<std::uint8_t> body{1, 2, 3, 4, 5};
  RC_CHECK(!link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                      rc::span<std::uint8_t>(scratch.data(), scratch.size())));
}

RC_TEST("the counters separate the four ways a link goes wrong") {
  // A link that reports only ok or failed cannot tell a wrong baud rate from a
  // bad ground from a device that has stopped talking. These four numbers can.
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  port.queue({0x00, 0x01, 0x02});             // noise, before any frame
  std::vector<std::uint8_t> bad = wire_frame({9, 9});
  bad[2] = static_cast<std::uint8_t>(bad[2] ^ 0x08u);
  port.queue(bad);                            // frames, checksum fails
  port.queue(wire_frame({4}));                // a good one

  bool got = false;
  for (int i = 0; i < 10 && !got; ++i) got = link.poll(static_cast<double>(i));

  RC_REQUIRE(got);
  RC_CHECK_EQ(link.discarded(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(link.checksum_failures(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(link.frames(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(link.port_errors(), static_cast<std::size_t>(0));
}

RC_TEST("a message carrying real fields survives the whole link") {
  FakePort port;
  auto storage = storage_for_link();
  Link link(port, rc::span<std::uint8_t>(storage.data(), storage.size()), kTimeout);

  std::vector<std::uint8_t> body(11, 0);
  rc::io::ByteWriter encoder{rc::span<std::uint8_t>(body.data(), body.size())};
  encoder.put_u8(3);
  encoder.put_u16(60000);
  encoder.put_i32(-7);
  encoder.put_f32(-0.25f);
  RC_REQUIRE(encoder.ok());

  std::vector<std::uint8_t> scratch(64, 0);
  RC_REQUIRE(link.send(rc::span<const std::uint8_t>(body.data(), body.size()),
                       rc::span<std::uint8_t>(scratch.data(), scratch.size())));

  FakePort loopback;
  loopback.set_read_chunk(1);          // as awkwardly as a real port would
  loopback.queue(port.written());
  auto storage2 = storage_for_link();
  Link back(loopback, rc::span<std::uint8_t>(storage2.data(), storage2.size()), kTimeout);

  bool got = false;
  for (int i = 0; i < 60 && !got; ++i) got = back.poll(static_cast<double>(i));
  RC_REQUIRE(got);

  rc::io::ByteReader decoder{back.message()};
  std::uint8_t id = 0;
  std::uint16_t sequence = 0;
  std::int32_t offset = 0;
  float value = 0.0f;
  decoder.get_u8(id);
  decoder.get_u16(sequence);
  decoder.get_i32(offset);
  decoder.get_f32(value);

  RC_REQUIRE(decoder.ok());
  RC_CHECK_EQ(static_cast<int>(id), 3);
  RC_CHECK_EQ(sequence, static_cast<std::uint16_t>(60000));
  RC_CHECK_EQ(offset, -7);
  RC_CHECK_NEAR(static_cast<double>(value), -0.25, 1e-12);
}
