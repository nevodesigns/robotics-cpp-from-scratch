#include <rc/test/rc_test.hpp>

#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/fault_port.hpp>
#include <rc/io/link.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kTimeoutMs = 50.0;
constexpr int kAttempts = 3;
constexpr int kRequests = 200;
constexpr double kPromptMs = 5.0;       // how long the device takes when nothing goes wrong
constexpr double kLateMs = 80.0;        // longer than the timeout, so the requester has already moved on
constexpr std::uint8_t kMove = 0x10;    // "move by this much", which is not safe to do twice

// Two ends of a link in memory. Whatever one end writes, the other end reads.
struct Bench {
  rc::io::MemoryPort host_port;
  rc::io::MemoryPort device_port;
  std::vector<std::uint8_t> host_storage = std::vector<std::uint8_t>(64, 0);
  std::vector<std::uint8_t> device_storage = std::vector<std::uint8_t>(64, 0);
  rc::io::Link host{host_port, rc::span<std::uint8_t>(host_storage.data(), host_storage.size()), 1e12};
  rc::io::Link device{device_port, rc::span<std::uint8_t>(device_storage.data(), device_storage.size()), 1e12};
  std::size_t host_moved = 0;
  std::size_t device_moved = 0;

  void to_device() {
    const std::deque<std::uint8_t>& out = host_port.written();
    while (host_moved < out.size()) {
      const std::uint8_t byte = out[host_moved++];
      device_port.feed(rc::span<const std::uint8_t>(&byte, 1));
    }
  }

  void to_host() {
    const std::deque<std::uint8_t>& out = device_port.written();
    while (device_moved < out.size()) {
      const std::uint8_t byte = out[device_moved++];
      host_port.feed(rc::span<const std::uint8_t>(&byte, 1));
    }
  }
};

std::vector<std::uint8_t> arguments_of(int delta) {
  std::vector<std::uint8_t> out(2, 0);
  rc::io::ByteWriter writer{rc::span<std::uint8_t>(out.data(), out.size())};
  writer.put_u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(delta)));
  return out;
}

// A device that does what it is asked, and sometimes answers late or not at all.
// The pattern is by count and not by chance, so the same numbers come out on
// every run and every machine: every tenth new request is answered after
// kLateMs, which is too late, and every fifteenth is not answered at all.
class SimDevice {
 public:
  SimDevice(Bench& bench, bool remembers) : bench_(bench), remembers_(remembers) {}

  void step(double now) {
    while (bench_.device.poll(now)) {
      const rc::span<const std::uint8_t> message = bench_.device.message();
      if (message.size() < 5 || message[0] != kRequestType) continue;
      const std::uint8_t id = message[1];
      std::uint16_t raw = 0;
      rc::io::ByteReader reader{rc::span<const std::uint8_t>(message.data() + 3, 2)};
      reader.get_u16(raw);
      const int delta = static_cast<std::int16_t>(raw);

      const bool is_repeat = seen_any_ && id == last_id_;

      // What the device says besides the type and the id: a status, and where it is.
      std::vector<std::uint8_t> body;
      rc::span<const std::uint8_t> remembered;
      if (remembers_ && cache_.lookup(id, remembered)) {
        body.assign(remembered.data(), remembered.data() + remembered.size());
        ++replays_;
      } else {
        position_ += delta;
        ++executions_;
        body.push_back(0);
        std::uint8_t pos[4] = {};
        rc::io::ByteWriter writer{rc::span<std::uint8_t>(pos, sizeof(pos))};
        writer.put_u32(static_cast<std::uint32_t>(position_));
        body.insert(body.end(), pos, pos + 4);
        if (remembers_) cache_.remember(id, rc::span<const std::uint8_t>(body.data(), body.size()));
      }

      // The id is added on the way out, so a replay carries the id of the repeat.
      std::vector<std::uint8_t> response{kResponseType, id};
      response.insert(response.end(), body.begin(), body.end());

      if (!is_repeat) {
        ++distinct_;
        seen_any_ = true;
      }
      last_id_ = id;

      double due = now + kPromptMs;
      bool dropped = false;
      if (!is_repeat) {
        if (distinct_ % 10 == 0) due = now + kLateMs;
        else if (distinct_ % 15 == 0) dropped = true;
      }
      if (!dropped) pending_.push_back({due, response});
    }

    for (std::size_t i = 0; i < pending_.size();) {
      if (pending_[i].due <= now) {
        std::uint8_t scratch[64] = {};
        bench_.device.send(rc::span<const std::uint8_t>(pending_[i].bytes.data(), pending_[i].bytes.size()),
                           rc::span<std::uint8_t>(scratch, sizeof(scratch)));
        pending_.erase(pending_.begin() + static_cast<long>(i));
      } else {
        ++i;
      }
    }
  }

  long position() const { return position_; }
  long executions() const { return executions_; }
  long replays() const { return replays_; }

 private:
  struct Pending {
    double due;
    std::vector<std::uint8_t> bytes;
  };

  Bench& bench_;
  bool remembers_;
  ReplyCache cache_;
  std::vector<Pending> pending_;
  long position_ = 0;
  long executions_ = 0;
  long replays_ = 0;
  long distinct_ = 0;
  bool seen_any_ = false;
  std::uint8_t last_id_ = 0;
};

enum class Mode { kNaive, kIds, kIdsAndMemory };

struct Outcome {
  int answered = 0;
  int timed_out = 0;
  int wrong = 0;       // taken as the answer, and it was an answer to another request
  long position = 0;   // where the device ended up; it should be kRequests
  long executions = 0;
  std::size_t stale = 0;
  std::size_t retries = 0;
};

Outcome run_mode(Mode mode) {
  Bench bench;
  SimDevice device(bench, mode == Mode::kIdsAndMemory);
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  Outcome out;
  double now = 0.0;

  // The baseline requester: it takes the next response it sees. It resends on a
  // timeout exactly as the real one does, and has no idea which request an
  // answer belongs to.
  std::uint8_t naive_id = 0;
  int naive_attempts = 0;
  double naive_sent = 0.0;
  bool naive_waiting = false;
  std::uint8_t naive_scratch[64] = {};

  const auto naive_send = [&](double at) {
    const std::vector<std::uint8_t> args = arguments_of(1);
    std::uint8_t body[8] = {kRequestType, naive_id, kMove, args[0], args[1]};
    bench.host.send(rc::span<const std::uint8_t>(body, 5), rc::span<std::uint8_t>(naive_scratch, sizeof(naive_scratch)));
    naive_sent = at;
  };

  for (int request = 0; request < kRequests; ++request) {
    const std::vector<std::uint8_t> args = arguments_of(1);
    if (mode == Mode::kNaive) {
      ++naive_id;
      naive_attempts = 1;
      naive_waiting = true;
      naive_send(now);
    } else {
      requester.begin(kMove, rc::span<const std::uint8_t>(args.data(), args.size()), now);
    }

    for (int guard = 0; guard < 1000; ++guard) {
      now += 1.0;
      bench.to_device();
      device.step(now);
      bench.to_host();

      while (bench.host.poll(now)) {
        const rc::span<const std::uint8_t> m = bench.host.message();
        if (mode == Mode::kNaive) {
          if (naive_waiting && m.size() >= 2 && m[0] == kResponseType) {
            naive_waiting = false;
            ++out.answered;
            if (m[1] != naive_id) ++out.wrong;
          }
        } else {
          requester.handle(m);
        }
      }

      if (mode == Mode::kNaive) {
        if (naive_waiting && now - naive_sent >= kTimeoutMs) {
          if (naive_attempts >= kAttempts) {
            naive_waiting = false;
            ++out.timed_out;
          } else {
            ++naive_attempts;
            ++out.retries;
            naive_send(now);
          }
        }
        if (!naive_waiting) break;
      } else {
        requester.tick(now);
        if (requester.state() == RequestState::Done) {
          ++out.answered;
          break;
        }
        if (requester.state() == RequestState::TimedOut) {
          ++out.timed_out;
          break;
        }
      }
    }
  }

  out.position = device.position();
  out.executions = device.executions();
  if (mode != Mode::kNaive) {
    out.stale = requester.stale();
    out.retries = requester.retries();
  }
  return out;
}

std::vector<std::uint8_t> first_message_written(Bench& bench) {
  bench.to_device();
  bench.device.poll(0.0);
  const rc::span<const std::uint8_t> m = bench.device.message();
  return std::vector<std::uint8_t>(m.data(), m.data() + m.size());
}

void send_response(Bench& bench, std::uint8_t id, std::uint8_t tag) {
  const std::uint8_t body[4] = {kResponseType, id, 0, tag};
  std::uint8_t scratch[32] = {};
  bench.device.send(rc::span<const std::uint8_t>(body, sizeof(body)), rc::span<std::uint8_t>(scratch, sizeof(scratch)));
  bench.to_host();
}

}  // namespace

RC_TEST("a request goes out as a framed message carrying its type, an id, the operation and the arguments") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  const std::vector<std::uint8_t> args = arguments_of(-3);

  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(args.data(), args.size()), 0.0));
  RC_CHECK(requester.state() == RequestState::Waiting);

  const std::vector<std::uint8_t> sent = first_message_written(bench);
  RC_REQUIRE_EQ(sent.size(), static_cast<std::size_t>(5));
  RC_CHECK_EQ(static_cast<int>(sent[0]), static_cast<int>(kRequestType));
  RC_CHECK_EQ(static_cast<int>(sent[1]), static_cast<int>(requester.id()));
  RC_CHECK_EQ(static_cast<int>(sent[2]), static_cast<int>(kMove));
  RC_CHECK_EQ(static_cast<int>(sent[3]), static_cast<int>(args[0]));
  RC_CHECK_EQ(requester.sent(), static_cast<std::size_t>(1));
  RC_CHECK(requester.id() != 0);
}

RC_TEST("the response with the matching id completes the request and hands over its body") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));

  send_response(bench, requester.id(), 0x77);
  RC_REQUIRE(bench.host.poll(1.0));
  RC_CHECK(requester.handle(bench.host.message()));

  RC_REQUIRE(requester.state() == RequestState::Done);
  RC_REQUIRE_EQ(requester.response().size(), static_cast<std::size_t>(2));
  RC_CHECK_EQ(static_cast<int>(requester.response()[1]), 0x77);
  RC_CHECK_EQ(requester.stale(), static_cast<std::size_t>(0));
}

RC_TEST("a response to another request is stale, is counted, and does not complete this one") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));
  const std::uint8_t wrong_id = static_cast<std::uint8_t>(requester.id() + 5);

  send_response(bench, wrong_id, 0x11);
  RC_REQUIRE(bench.host.poll(1.0));
  RC_CHECK(requester.handle(bench.host.message()));   // it was a response, so it is dealt with

  RC_CHECK(requester.state() == RequestState::Waiting);
  RC_CHECK_EQ(requester.stale(), static_cast<std::size_t>(1));

  // The right one still completes it afterwards.
  send_response(bench, requester.id(), 0x22);
  RC_REQUIRE(bench.host.poll(2.0));
  requester.handle(bench.host.message());
  RC_CHECK(requester.state() == RequestState::Done);
}

RC_TEST("a message that is not a response is left for the caller to route") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));

  const std::uint8_t telemetry[5] = {0x02, 1, 2, 3, 4};
  RC_CHECK(!requester.handle(rc::span<const std::uint8_t>(telemetry, sizeof(telemetry))));
  RC_CHECK(!requester.handle(rc::span<const std::uint8_t>(telemetry, 1)));   // too short to be anything
  RC_CHECK(requester.state() == RequestState::Waiting);
  RC_CHECK_EQ(requester.stale(), static_cast<std::size_t>(0));
}

RC_TEST("a silent device is asked again with the same id, and then given up on") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));
  const std::uint8_t id = requester.id();

  requester.tick(49.0);
  RC_CHECK_EQ(requester.retries(), static_cast<std::size_t>(0));   // not yet

  requester.tick(50.0);
  RC_CHECK_EQ(requester.retries(), static_cast<std::size_t>(1));
  RC_CHECK(requester.state() == RequestState::Waiting);
  RC_CHECK_EQ(static_cast<int>(requester.id()), static_cast<int>(id));   // a retry is the same request

  requester.tick(100.0);
  RC_CHECK_EQ(requester.retries(), static_cast<std::size_t>(2));
  requester.tick(150.0);   // the third attempt has had its time
  RC_CHECK(requester.state() == RequestState::TimedOut);
  RC_CHECK_EQ(requester.timeouts(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(requester.sent(), static_cast<std::size_t>(1));      // one request, however many times it was sent

  // Three copies of the same frame went out.
  bench.to_device();
  int received = 0;
  while (bench.device.poll(0.0)) {
    ++received;
    RC_CHECK_EQ(static_cast<int>(bench.device.message()[1]), static_cast<int>(id));
  }
  RC_CHECK_EQ(received, 3);
}

RC_TEST("one request at a time: a second is refused while the first is outstanding") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));
  const std::uint8_t id = requester.id();

  RC_CHECK(!requester.begin(kMove, rc::span<const std::uint8_t>(), 1.0));
  RC_CHECK_EQ(static_cast<int>(requester.id()), static_cast<int>(id));
  RC_CHECK_EQ(requester.sent(), static_cast<std::size_t>(1));

  const std::vector<std::uint8_t> too_long(kMaxBody + 10, 0);
  requester.tick(500.0);
  requester.tick(1000.0);
  requester.tick(1500.0);
  RC_REQUIRE(requester.state() == RequestState::TimedOut);
  RC_CHECK(!requester.begin(kMove, rc::span<const std::uint8_t>(too_long.data(), too_long.size()), 2000.0));
}

RC_TEST("a send that fails does not start a request, and does not use up an id") {
  class DeadPort : public rc::io::BytePort {
   public:
    int read(rc::span<std::uint8_t>) override { return 0; }
    int write(rc::span<const std::uint8_t>) override { return -1; }
  };

  DeadPort dead;
  std::vector<std::uint8_t> storage(64, 0);
  rc::io::Link link(dead, rc::span<std::uint8_t>(storage.data(), storage.size()), 1e9);
  Requester requester(link, kTimeoutMs, kAttempts);

  RC_CHECK(!requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));
  RC_CHECK(requester.state() == RequestState::Idle);
  RC_CHECK_EQ(requester.send_failures(), static_cast<std::size_t>(1));
  RC_CHECK_EQ(requester.sent(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(static_cast<int>(requester.id()), 0);
}

RC_TEST("a late answer to a request given up on is not the answer to the next one") {
  Bench bench;
  Requester requester(bench.host, 10.0, 1);   // one attempt, so it gives up quickly
  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 0.0));
  const std::uint8_t first = requester.id();
  requester.tick(10.0);
  RC_REQUIRE(requester.state() == RequestState::TimedOut);

  RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), 11.0));
  const std::uint8_t second = requester.id();
  RC_CHECK(second != first);

  // Now the first one's answer turns up.
  send_response(bench, first, 0xAA);
  RC_REQUIRE(bench.host.poll(12.0));
  requester.handle(bench.host.message());
  RC_CHECK(requester.state() == RequestState::Waiting);
  RC_CHECK_EQ(requester.stale(), static_cast<std::size_t>(1));

  send_response(bench, second, 0xBB);
  RC_REQUIRE(bench.host.poll(13.0));
  requester.handle(bench.host.message());
  RC_REQUIRE(requester.state() == RequestState::Done);
  RC_CHECK_EQ(static_cast<int>(requester.response()[1]), 0xBB);
}

RC_TEST("ids wrap and never come out as zero") {
  Bench bench;
  Requester requester(bench.host, kTimeoutMs, kAttempts);
  bool saw_zero = false;
  std::uint8_t previous = 0;
  bool repeated_too_soon = false;

  for (int i = 0; i < 600; ++i) {
    RC_REQUIRE(requester.begin(kMove, rc::span<const std::uint8_t>(), static_cast<double>(i)));
    if (requester.id() == 0) saw_zero = true;
    if (i > 0 && requester.id() == previous) repeated_too_soon = true;
    previous = requester.id();
    send_response(bench, requester.id(), 1);
    RC_REQUIRE(bench.host.poll(static_cast<double>(i)));
    requester.handle(bench.host.message());
    RC_REQUIRE(requester.state() == RequestState::Done);
  }

  RC_CHECK(!saw_zero);
  RC_CHECK(!repeated_too_soon);
}

RC_TEST("the device remembers the last id it answered, and only that one") {
  ReplyCache cache;
  rc::span<const std::uint8_t> found;
  RC_CHECK(!cache.lookup(7, found));   // nothing remembered yet, not even id zero

  const std::uint8_t reply[3] = {9, 8, 7};
  cache.remember(7, rc::span<const std::uint8_t>(reply, sizeof(reply)));
  RC_REQUIRE(cache.lookup(7, found));
  RC_REQUIRE_EQ(found.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(static_cast<int>(found[0]), 9);
  RC_CHECK_EQ(static_cast<int>(found[2]), 7);
  RC_CHECK(!cache.lookup(8, found));

  const std::uint8_t other[2] = {1, 2};
  cache.remember(8, rc::span<const std::uint8_t>(other, sizeof(other)));
  RC_CHECK(cache.lookup(8, found));
  RC_CHECK(!cache.lookup(7, found));   // replaced, one slot
}

RC_TEST("what an id buys, and what the device must do about a repeat") {
  const Outcome naive = run_mode(Mode::kNaive);
  const Outcome ids = run_mode(Mode::kIds);
  const Outcome both = run_mode(Mode::kIdsAndMemory);

  const auto row = [](const char* what, const Outcome& o) {
    std::cout << "    " << std::left << std::setw(34) << what << std::right << std::setw(9) << o.answered
              << std::setw(8) << o.timed_out << std::setw(8) << o.wrong << std::setw(9) << o.retries
              << std::setw(11) << o.position << "\n";
  };

  std::cout << "  " << kRequests << " requests, each \"move by one\", to a device that answers one in ten too\n"
            << "  late and one in fifteen not at all. Where it ends up should be " << kRequests << ".\n\n";
  std::cout << "    " << std::left << std::setw(34) << "requester" << std::right << std::setw(9) << "answered"
            << std::setw(8) << "gave up" << std::setw(8) << "wrong" << std::setw(9) << "resent"
            << std::setw(11) << "position" << "\n";
  row("takes the next response", naive);
  row("matches the id", ids);
  row("matches the id, device remembers", both);
  std::cout << "\n";

  // Without ids the answers shift: a late response is taken for the next
  // request's, and so is the one after it, until a retry resynchronises them.
  RC_CHECK(naive.wrong > 20);

  // With them, no answer is ever taken for another request's, and the late ones
  // are counted instead.
  RC_CHECK_EQ(ids.wrong, 0);
  RC_CHECK(ids.stale > 0);
  RC_CHECK_EQ(ids.timed_out, 0);

  // But a resent request is a repeated command. The answer was what was lost,
  // and the device did what it was asked a second time: it overshoots by
  // exactly the number of times the requester asked again.
  RC_CHECK(ids.position > kRequests);
  RC_CHECK_EQ(ids.position, static_cast<long>(kRequests) + static_cast<long>(ids.retries));

  // A device that remembers its last answer replays it, and ends up where it
  // was told to go.
  RC_CHECK_EQ(both.position, static_cast<long>(kRequests));
  RC_CHECK_EQ(both.answered, kRequests);
  RC_CHECK_EQ(both.wrong, 0);
}
