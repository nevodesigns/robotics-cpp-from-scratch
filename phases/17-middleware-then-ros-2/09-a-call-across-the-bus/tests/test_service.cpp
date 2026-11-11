#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

struct Wire {
  int arrives;
  Request request;
  Response response;
  bool is_request;
};

struct Outcome {
  int done = 0;
  int timed_out = 0;
  int executions = 0;
  int repeats = 0;
  int transmissions = 0;
  int unwanted = 0;
};

// `calls` calls over a link with the given loss each way and delay. `dedup` says whether the server remembers what it answered; if it does
// not, it is simulated by a server that executes everything it is sent.
Outcome run(int calls, int timeout, int retries, double loss, int delay, bool dedup, std::uint32_t seed) {
  Rng rng{seed};
  int count = 0;
  ServiceServer server([&count](const std::string& p) {
    ++count;
    return "done:" + p;
  });
  ServiceClient client(7, timeout, retries);
  std::vector<int> ids;
  for (int i = 0; i < calls; ++i) ids.push_back(client.call(std::to_string(i)));
  std::deque<Wire> wire;
  Outcome o;
  int fresh_id = 1000000;
  for (int now = 0; now < 5000; ++now) {
    while (!wire.empty() && wire.front().arrives <= now) {
      const Wire w = wire.front();
      wire.pop_front();
      if (w.is_request) {
        Request req = w.request;
        if (!dedup) req.id = fresh_id++;                         // the server cannot tell it from a new request
        const Response r = server.handle(req);
        Response back = r;
        back.id = w.request.id;
        if (rng.uniform() >= loss) wire.push_back(Wire{now + delay, Request(), back, false});
      } else {
        client.on_response(w.response);
      }
    }
    for (const Request& r : client.tick(now)) {
      ++o.transmissions;
      if (rng.uniform() >= loss) wire.push_back(Wire{now + delay, r, Response(), true});
    }
  }
  for (int id : ids) {
    if (client.status(id) == CallStatus::done) ++o.done;
    if (client.status(id) == CallStatus::timed_out) ++o.timed_out;
  }
  o.executions = server.executions();
  o.repeats = server.repeats();
  o.unwanted = client.unwanted();
  return o;
}

}  // namespace

RC_TEST("an answer is matched to its call by number, so calls in flight together get the right answers back in any order") {
  ServiceServer server([](const std::string& p) { return "got " + p; });
  ServiceClient client(1, 100, 0);
  const int a = client.call("open");
  const int b = client.call("close");
  const std::vector<Request> out = client.tick(0);
  RC_CHECK(out.size() == 2);
  RC_CHECK(client.status(a) == CallStatus::pending);
  if (out.size() != 2) return;
  const Response rb = server.handle(out[1]);
  const Response ra = server.handle(out[0]);
  client.on_response(rb);                                  // the second call is answered first
  client.on_response(ra);
  std::cout << "  two calls, answers back in the other order: call " << a << " got '" << client.answer(a) << "', call " << b << " got '" << client.answer(b)
            << "'\n\n";
  RC_CHECK(client.status(a) == CallStatus::done);
  RC_CHECK(client.status(b) == CallStatus::done);
  RC_CHECK(client.answer(a) == "got open");
  RC_CHECK(client.answer(b) == "got close");
  RC_CHECK(client.status(99) == CallStatus::unknown);
}

RC_TEST("a call with no answer is sent again after the timeout, and when the last wait runs out it has timed out and is not left pending") {
  ServiceClient client(1, 10, 2);
  const int id = client.call("ping");
  RC_CHECK(client.tick(0).size() == 1);
  RC_CHECK(client.tick(9).empty());
  RC_CHECK(client.tick(10).size() == 1);                   // the first retry
  RC_CHECK(client.tick(20).size() == 1);                   // the second
  RC_CHECK(client.tick(29).empty());
  RC_CHECK(client.status(id) == CallStatus::pending);
  RC_CHECK(client.tick(30).empty());                       // the last wait has run out
  std::cout << "  timeout 10, 2 retries: sent at 0, 10, 20, timed out at 30; attempts " << client.attempts(id) << "\n\n";
  RC_CHECK(client.status(id) == CallStatus::timed_out);
  RC_CHECK(client.attempts(id) == 3);
  RC_CHECK(client.tick(40).empty());
}

RC_TEST("an answer that arrives after the call has timed out is not wanted, and does not bring the call back to life") {
  ServiceServer server([](const std::string& p) { return p; });
  ServiceClient client(1, 5, 0);
  const int id = client.call("late");
  const std::vector<Request> out = client.tick(0);
  client.tick(5);                                           // times out
  RC_CHECK(client.status(id) == CallStatus::timed_out);
  RC_CHECK(out.size() == 1);
  if (out.size() != 1) return;   // not RC_REQUIRE: it would skip the destructors
  client.on_response(server.handle(out[0]));
  Response stranger;
  stranger.client = 2;
  stranger.id = id;
  client.on_response(stranger);                             // somebody else's
  Response unknown;
  unknown.client = 1;
  unknown.id = 77;
  client.on_response(unknown);                              // a call never made
  std::cout << "  an answer after the timeout, one for another client and one for a call never made: " << client.unwanted() << " unwanted, call is "
            << (client.status(id) == CallStatus::timed_out ? "still timed out" : "changed") << "\n\n";
  RC_CHECK(client.status(id) == CallStatus::timed_out);
  RC_CHECK(client.unwanted() == 3);
  RC_CHECK(client.answer(id).empty());
}

RC_TEST("a server that is asked twice does the work once and gives the same answer twice") {
  int closed = 0;
  ServiceServer server([&closed](const std::string&) {
    ++closed;
    return "gripper closed";
  });
  Request r;
  r.client = 3;
  r.id = 1;
  r.payload = "close";
  const Response a = server.handle(r);
  const Response b = server.handle(r);
  Request other = r;
  other.id = 2;
  server.handle(other);
  Request another_client = r;
  another_client.client = 4;
  server.handle(another_client);
  std::cout << "  the same request twice, then a new id, then another client with the same id: the gripper was closed " << closed << " times, "
            << server.repeats() << " repeat answered from memory\n\n";
  RC_CHECK(a.payload == b.payload);
  RC_CHECK(closed == 3);
  RC_CHECK(server.executions() == 3);
  RC_CHECK(server.repeats() == 1);
}

RC_TEST("over a link that loses 30 percent each way, retries make calls complete, and a server that remembers keeps them from running twice") {
  const int n = 300;
  std::cout << "  " << n << " calls, 30 percent loss each way, delay 3 ticks, timeout 20\n"
            << "  retries   server remembers   completed   timed out   times the work ran   requests sent\n";
  Outcome none;
  Outcome many;
  Outcome many_forgetful;
  for (int retries : {0, 1, 3, 6}) {
    const Outcome o = run(n, 20, retries, 0.30, 3, true, 5);
    std::cout << "  " << std::setw(7) << retries << "   " << std::setw(16) << "yes" << "   " << std::setw(9) << o.done << "   " << std::setw(9) << o.timed_out
              << "   " << std::setw(18) << o.executions << "   " << std::setw(13) << o.transmissions << "\n";
    if (retries == 0) none = o;
    if (retries == 6) many = o;
  }
  many_forgetful = run(n, 20, 6, 0.30, 3, false, 5);
  std::cout << "  " << std::setw(7) << 6 << "   " << std::setw(16) << "no" << "   " << std::setw(9) << many_forgetful.done << "   " << std::setw(9)
            << many_forgetful.timed_out << "   " << std::setw(18) << many_forgetful.executions << "   " << std::setw(13) << many_forgetful.transmissions << "\n\n";
  RC_CHECK(none.done > 0.4 * n && none.done < 0.6 * n);
  RC_CHECK(many.done > 0.97 * n);
  RC_CHECK(many.executions <= n);
  RC_CHECK(many_forgetful.executions > 1.2 * many.executions);
}
