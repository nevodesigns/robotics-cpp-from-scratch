// rc/bus/reliable.hpp
//
// Reliable delivery over a link that loses packets, from lesson 17-05, graduated.
//
// A sender gives each message a number, puts at most a window of the oldest unacknowledged ones on the wire, and sends one again after a timeout with no acknowledgement. An acknowledgement carries the
// next number the receiver expects, which says that everything below it has arrived. The receiver hands messages up in order, once each, buffering early ones and counting repeats.
//
// Arrivals 2, 0, 1, 1, 0 were acknowledged 0 1 3 3 3, delivered a b c, with 2 duplicates. Over a link with 20 percent loss each way, 500 messages sent once gave 398 arrivals at 1.00 packets per message,
// and with acknowledgement and retransmission 500 in order at 1.79 packets per message. On a link with a one way delay of 50 ticks, 100 messages took 10000, 5000, 2000, 1000 and 500 ticks at windows of 1, 2, 5, 10 and 20.

#ifndef RC_BUS_RELIABLE_HPP
#define RC_BUS_RELIABLE_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace rc {
namespace bus {

// A message with its place in the sequence.
struct DataPacket {
  int seq = 0;
  std::string payload;
};

// The sender: gives each message a number, sends up to `window` of the oldest unacknowledged ones, and sends one again if no
// acknowledgement has come within `retransmit_after` ticks. An acknowledgement carries the next number the receiver expects, which
// says that every number below it has arrived.
class ReliableSender {
 public:
  ReliableSender(int window, int retransmit_after) : window_(window < 1 ? 1 : window), rto_(retransmit_after < 1 ? 1 : retransmit_after) {}

  // Queue a message; it gets the next number and goes out when the window has room.
  int send(const std::string& payload) {
    Slot s;
    s.packet.seq = next_seq_;
    s.packet.payload = payload;
    slots_[next_seq_] = s;
    return next_seq_++;
  }

  // The packets to put on the wire at tick `now`: any in the window never sent, and any sent and not acknowledged for `retransmit_after` ticks.
  std::vector<DataPacket> tick(int now) {
    std::vector<DataPacket> out;
    int in_window = 0;
    for (auto& item : slots_) {
      if (in_window >= window_) break;
      ++in_window;
      Slot& s = item.second;
      if (s.sent_count == 0 || now - s.last_sent >= rto_) {
        if (s.sent_count > 0) ++retransmissions_;
        s.last_sent = now;
        ++s.sent_count;
        ++transmissions_;
        out.push_back(s.packet);
      }
    }
    return out;
  }

  // The receiver expects `next_expected` next: everything below it is done.
  void on_ack(int next_expected) {
    while (!slots_.empty() && slots_.begin()->first < next_expected) slots_.erase(slots_.begin());
  }

  bool idle() const { return slots_.empty(); }
  std::size_t unacknowledged() const { return slots_.size(); }
  int retransmissions() const { return retransmissions_; }
  int transmissions() const { return transmissions_; }

 private:
  struct Slot {
    DataPacket packet;
    int last_sent = 0;
    int sent_count = 0;
  };
  int window_ = 1;
  int rto_ = 1;
  int next_seq_ = 0;
  int retransmissions_ = 0;
  int transmissions_ = 0;
  std::map<int, Slot> slots_;
};

// The receiver: hands messages up in order, once each, however they arrive.
class ReliableReceiver {
 public:
  // A packet arrived. Returns the acknowledgement to send back: the next number expected.
  int on_data(const DataPacket& packet) {
    if (packet.seq < expected_) {
      ++duplicates_;
      return expected_;
    }
    if (packet.seq > expected_) {
      if (waiting_.count(packet.seq)) ++duplicates_;
      else waiting_[packet.seq] = packet.payload;
      return expected_;
    }
    delivered_.push_back(packet.payload);
    ++expected_;
    while (!waiting_.empty() && waiting_.begin()->first == expected_) {
      delivered_.push_back(waiting_.begin()->second);
      waiting_.erase(waiting_.begin());
      ++expected_;
    }
    return expected_;
  }

  const std::vector<std::string>& delivered() const { return delivered_; }
  int duplicates() const { return duplicates_; }
  int expected() const { return expected_; }

 private:
  int expected_ = 0;
  int duplicates_ = 0;
  std::map<int, std::string> waiting_;
  std::vector<std::string> delivered_;
};

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_RELIABLE_HPP
