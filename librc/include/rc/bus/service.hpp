// rc/bus/service.hpp
//
// Calling a function across the bus, from lesson 17-09, graduated.
//
// A client numbers each call, sends it again after a timeout up to a number of retries, marks it timed out when the last wait runs out, and does not want an answer to a call that is no longer waiting, or one
// for another client, or for a call never made. A server that is asked the same request twice does the work once and gives the same answer.
//
// Two calls answered in the other order each got their own answer. With timeout 10 and 2 retries a call was sent at 0, 10 and 20 and timed out at 30. Over a link losing 30 percent each way, 300 calls
// completed 145, 218, 281 and 298 with 0, 1, 3 and 6 retries; with no retries the work still ran 214 times, for calls the client was told had failed. With 6 retries a server that forgot what it had done ran the work 446 times
// for 300 calls, against 300 for one that remembered.

#ifndef RC_BUS_SERVICE_HPP
#define RC_BUS_SERVICE_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace rc {
namespace bus {

struct Request {
  int client = 0;
  int id = 0;
  std::string payload;
};

struct Response {
  int client = 0;
  int id = 0;
  std::string payload;
};

// The side that does the work. A request that arrives again (the client did not get the answer and tried again) is answered with the answer
// it already gave, and is not done a second time.
class ServiceServer {
 public:
  using Handler = std::function<std::string(const std::string&)>;
  explicit ServiceServer(Handler handler) : handler_(std::move(handler)) {}

  Response handle(const Request& request) {
    const std::pair<int, int> key(request.client, request.id);
    const auto found = answered_.find(key);
    if (found != answered_.end()) {
      ++repeats_;
      return found->second;
    }
    Response r;
    r.client = request.client;
    r.id = request.id;
    r.payload = handler_(request.payload);
    ++executions_;
    answered_[key] = r;
    return r;
  }

  int executions() const { return executions_; }
  int repeats() const { return repeats_; }

 private:
  Handler handler_;
  std::map<std::pair<int, int>, Response> answered_;
  int executions_ = 0;
  int repeats_ = 0;
};

enum class CallStatus { unknown, pending, done, timed_out };

// The side that asks. Each call gets a number. A call that has had no answer for `timeout` ticks is sent again, up to `retries` more times,
// and after the last wait it has timed out. An answer to a call that is not waiting any more is not wanted, and is counted.
class ServiceClient {
 public:
  ServiceClient(int client_id, int timeout, int retries) : client_(client_id), timeout_(timeout < 1 ? 1 : timeout), retries_(retries < 0 ? 0 : retries) {}

  int call(const std::string& payload) {
    Call c;
    c.request.client = client_;
    c.request.id = next_id_;
    c.request.payload = payload;
    calls_[next_id_] = c;
    return next_id_++;
  }

  // The requests to put on the wire at tick `now`: calls not yet sent, and calls whose wait has run out and have retries left.
  // A call whose last wait has run out is marked timed out.
  std::vector<Request> tick(int now) {
    std::vector<Request> out;
    for (auto& item : calls_) {
      Call& c = item.second;
      if (c.status != CallStatus::pending) continue;
      if (c.attempts == 0) {
        c.attempts = 1;
        c.last_sent = now;
        out.push_back(c.request);
      } else if (now - c.last_sent >= timeout_) {
        if (c.attempts > retries_) {
          c.status = CallStatus::timed_out;
        } else {
          ++c.attempts;
          c.last_sent = now;
          out.push_back(c.request);
        }
      }
    }
    return out;
  }

  void on_response(const Response& response) {
    const auto found = calls_.find(response.id);
    if (response.client != client_ || found == calls_.end() || found->second.status != CallStatus::pending) {
      ++unwanted_;
      return;
    }
    found->second.status = CallStatus::done;
    found->second.answer = response.payload;
  }

  CallStatus status(int id) const {
    const auto found = calls_.find(id);
    return found == calls_.end() ? CallStatus::unknown : found->second.status;
  }
  std::string answer(int id) const {
    const auto found = calls_.find(id);
    return found == calls_.end() ? std::string() : found->second.answer;
  }
  int attempts(int id) const {
    const auto found = calls_.find(id);
    return found == calls_.end() ? 0 : found->second.attempts;
  }
  int unwanted() const { return unwanted_; }

 private:
  struct Call {
    Request request;
    CallStatus status = CallStatus::pending;
    int attempts = 0;
    int last_sent = 0;
    std::string answer;
  };
  int client_ = 0;
  int timeout_ = 1;
  int retries_ = 0;
  int next_id_ = 1;
  int unwanted_ = 0;
  std::map<int, Call> calls_;
};

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_SERVICE_HPP
