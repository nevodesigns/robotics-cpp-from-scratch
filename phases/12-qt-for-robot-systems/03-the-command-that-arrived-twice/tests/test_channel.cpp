#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iostream>
#include <set>
#include <vector>

#include "solution.hpp"

namespace {

// A channel between a sender and a robot, in steps of 10 ms. `lose_request(seq, attempt)` and `lose_ack(seq, arrival)` say which messages are lost.
struct Link {
  CommandSender sender{50, 5};
  CommandReceiver receiver{1024};
  long long now = 0;
  int transmissions = 0;                 // commands put on the wire
  int arrivals = 0;                      // commands that reached the robot
  int applied = 0;                       // commands the robot did, by its receiver
  int naive_applied = 0;                 // commands a robot that applies every arrival would have done
  int duplicates_refused = 0;
  std::vector<int> attempt_of;           // attempts so far, by sequence number
  bool (*lose_request)(std::uint16_t, int) = nullptr;
  bool (*lose_ack)(std::uint16_t, int) = nullptr;
  std::vector<int> arrivals_of;

  void transmit(std::uint16_t seq) {
    ++transmissions;
    if (attempt_of.size() <= seq) attempt_of.resize(seq + 1, 0);
    ++attempt_of[seq];
    if (lose_request && lose_request(seq, attempt_of[seq])) return;
    ++arrivals;
    ++naive_applied;
    if (arrivals_of.size() <= seq) arrivals_of.resize(seq + 1, 0);
    ++arrivals_of[seq];
    if (receiver.accept(seq)) ++applied;
    else ++duplicates_refused;
    if (lose_ack && lose_ack(seq, arrivals_of[seq])) return;
    sender.ack(seq);
  }

  void send_one() { transmit(sender.send(now)); }

  void run(long long until_ms) {
    for (; now <= until_ms; now += 10) {
      for (std::uint16_t seq : sender.due(now)) transmit(seq);
    }
  }
};

bool lose_first_request(std::uint16_t, int attempt) { return attempt == 1; }
bool lose_first_ack(std::uint16_t, int arrival) { return arrival == 1; }
bool lose_every_request(std::uint16_t, int) { return true; }

}  // namespace

RC_TEST("on a link that loses nothing every command is sent once and applied once") {
  Link link;
  for (int i = 0; i < 20; ++i) link.send_one();
  link.run(500);
  std::cout << "  20 commands, no loss: " << link.transmissions << " transmissions, " << link.applied << " applied, " << link.sender.pending() << " still waiting\n\n";
  RC_CHECK_EQ(link.transmissions, 20);
  RC_CHECK_EQ(link.applied, 20);
  RC_CHECK_EQ(link.sender.pending(), 0u);
  RC_CHECK_EQ(link.sender.failed().size(), 0u);
  RC_CHECK(!link.receiver.accept(0));          // command 0 was applied, and a second arrival of it is refused
  RC_CHECK(link.receiver.accept(500));         // a number it has not seen is not
  CommandSender numbers(50, 3);
  RC_CHECK_EQ(static_cast<int>(numbers.send(0)), 0);
  RC_CHECK_EQ(static_cast<int>(numbers.send(0)), 1);          // numbers go up from 0
  RC_CHECK_EQ(numbers.pending(), 2u);
}

RC_TEST("a command whose request is lost is sent again after the retry time, and applied once") {
  Link link;
  link.lose_request = lose_first_request;
  for (int i = 0; i < 10; ++i) link.send_one();
  RC_CHECK_EQ(link.applied, 0);                // nothing arrived yet
  RC_CHECK_EQ(link.sender.pending(), 10u);
  link.run(40);                                // not yet time
  RC_CHECK_EQ(link.transmissions, 10);
  link.run(200);
  std::cout << "  the first request of 10 commands lost: " << link.transmissions << " transmissions, " << link.applied << " applied, " << link.sender.pending() << " waiting\n\n";
  RC_CHECK_EQ(link.transmissions, 20);
  RC_CHECK_EQ(link.applied, 10);
  RC_CHECK_EQ(link.sender.pending(), 0u);
  RC_CHECK_EQ(link.sender.failed().size(), 0u);
}

RC_TEST("a command whose acknowledgement is lost arrives twice, and is applied once, where applying every arrival would do it twice") {
  Link link;
  link.lose_ack = lose_first_ack;
  for (int i = 0; i < 10; ++i) link.send_one();
  link.run(300);
  std::cout << "  the first acknowledgement of 10 commands lost: " << link.arrivals << " arrivals, " << link.applied << " applied by the receiver, " << link.duplicates_refused
            << " refused as duplicates; a robot applying every arrival would have done " << link.naive_applied << "\n\n";
  RC_CHECK_EQ(link.arrivals, 20);
  RC_CHECK_EQ(link.applied, 10);
  RC_CHECK_EQ(link.duplicates_refused, 10);
  RC_CHECK_EQ(link.naive_applied, 20);          // one metre each, applied twice: two metres
  RC_CHECK_EQ(link.sender.pending(), 0u);
}

RC_TEST("a command nobody answers is sent its number of times and then fails, and is not waiting any more") {
  Link link;
  link.lose_request = lose_every_request;
  const std::uint16_t seq = link.sender.send(0);
  link.transmit(seq);                              // the first attempt, lost
  link.run(1000);
  std::cout << "  a link that loses everything: " << link.transmissions << " transmissions, failed " << link.sender.failed().size() << ", waiting " << link.sender.pending() << "\n\n";
  RC_CHECK_EQ(link.transmissions, 5);              // the first, and four retries: five attempts
  RC_CHECK_EQ(link.sender.failed().size(), 1u);
  if (link.sender.failed().size() == 1) RC_CHECK_EQ(static_cast<int>(link.sender.failed()[0]), static_cast<int>(seq));
  RC_CHECK_EQ(link.sender.pending(), 0u);
  RC_CHECK_EQ(link.applied, 0);
}

RC_TEST("sequence numbers wrap at 65536 with no duplicate, and an answer that is not wanted is refused") {
  CommandSender sender(50, 3);
  std::set<int> seen;
  int repeated_in_a_row = 0;
  std::uint16_t last = 0;
  for (int i = 0; i < 70000; ++i) {
    const std::uint16_t seq = sender.send(i);
    if (i > 0 && seq == last) ++repeated_in_a_row;
    last = seq;
    seen.insert(seq);
    sender.ack(seq);
  }
  std::cout << "  70000 commands: " << seen.size() << " different numbers, the last was " << last << "\n";
  RC_CHECK_EQ(static_cast<int>(seen.size()), 65536);
  RC_CHECK_EQ(repeated_in_a_row, 0);
  RC_CHECK_EQ(static_cast<int>(last), 69999 - 65536);

  CommandSender s2(50, 3);
  const std::uint16_t a = s2.send(0);
  RC_CHECK(s2.ack(a));
  RC_CHECK(!s2.ack(a));                             // a second answer for it
  RC_CHECK(!s2.ack(999));                           // one that was never sent

  CommandReceiver r(2);                             // a window of two
  RC_CHECK(r.accept(1));
  RC_CHECK(!r.accept(1));
  RC_CHECK(r.accept(2));
  RC_CHECK(r.accept(3));                            // 1 falls out of the window
  RC_CHECK(r.accept(1));                            // and is no longer remembered: the limit of a window
  RC_CHECK(!r.accept(3));
  std::cout << "\n";
}
