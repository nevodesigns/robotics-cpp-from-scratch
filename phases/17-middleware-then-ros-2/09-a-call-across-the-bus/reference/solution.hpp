#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

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

#endif  // LESSON_SOLUTION_HPP
