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
    // TODO: The key is (request.client, request.id). If answered_ already has it: add one to repeats_ and return the answer it holds, without calling handler_.
    // Otherwise call handler_ on the payload, put the result in a Response with the same client and id, count it in executions_, remember it in answered_ and return it.
    (void)request;
    (void)handler_;
    (void)answered_;
    (void)executions_;
    (void)repeats_;
    return Response();
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
    // TODO: a new Call whose request has this client, the next id (next_id_) and the payload; keep it in calls_ under that id; return the id and add one to next_id_
    (void)payload;
    (void)client_;
    (void)next_id_;
    (void)calls_;
    return 0;
  }

  // The requests to put on the wire at tick `now`: calls not yet sent, and calls whose wait has run out and have retries left.
  // A call whose last wait has run out is marked timed out.
  std::vector<Request> tick(int now) {
    // TODO: For every call still pending (calls_ is ordered by id):
    // - never sent (attempts == 0): send it: attempts = 1, last_sent = now, add its request to the result;
    // - otherwise, if now - last_sent >= timeout_: if attempts is already more than retries_ the call has timed out (status = timed_out);
    //   else send it again: add one to attempts, last_sent = now, add its request to the result.
    (void)now;
    (void)timeout_;
    (void)retries_;
    (void)calls_;
    return std::vector<Request>();
  }

  void on_response(const Response& response) {
    // TODO: Find the call by response.id. If the response is for another client, or there is no such call, or the call is not pending any more: add one to unwanted_ and stop.
    // Otherwise the call is done and keeps response.payload as its answer.
    (void)response;
    (void)client_;
    (void)unwanted_;
    (void)calls_;
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
