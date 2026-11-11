#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

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
    // TODO: a Slot whose packet has seq = next_seq_ and this payload, stored in slots_[next_seq_]; return the number and add one to next_seq_
    (void)payload;
    (void)next_seq_;
    (void)slots_;
    return 0;
  }

  // The packets to put on the wire at tick `now`: any in the window never sent, and any sent and not acknowledged for `retransmit_after` ticks.
  std::vector<DataPacket> tick(int now) {
    // TODO: Walk slots_ from the oldest (the map is ordered by number), looking at no more than window_ of them.
    // A slot never sent (sent_count == 0), or last sent rto_ or more ticks ago (now - last_sent >= rto_), goes out: if it had been sent before count a retransmission,
    // set last_sent = now, add one to its sent_count and to transmissions_, and add its packet to the result.
    (void)now;
    (void)window_;
    (void)rto_;
    (void)retransmissions_;
    (void)transmissions_;
    (void)slots_;
    return std::vector<DataPacket>();
  }

  // The receiver expects `next_expected` next: everything below it is done.
  void on_ack(int next_expected) {
    // TODO: erase every slot with a number below next_expected (the oldest come first)
    (void)next_expected;
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
    // TODO: A number below expected_ is a duplicate: count it and return expected_.
    // A number above expected_ is early: keep it in waiting_ unless it is already there (then it is a duplicate) and return expected_.
    // The number expected: deliver it, add one to expected_, then deliver any waiting packets that now follow on in order. Return expected_.
    (void)packet;
    (void)duplicates_;
    (void)waiting_;
    (void)delivered_;
    return 0;
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

#endif  // LESSON_SOLUTION_HPP
