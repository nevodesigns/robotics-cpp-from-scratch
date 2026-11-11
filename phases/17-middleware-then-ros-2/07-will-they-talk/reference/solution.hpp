#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <rc/bus/durable_bus.hpp>

using rc::bus::Durability;

// The properties of a topic that both ends must agree about. The order of each enum matters: a larger value is a stronger promise.
enum class Reliability { best_effort = 0, reliable = 1 };
enum class Liveliness { automatic = 0, manual_by_topic = 1 };

struct Qos {
  Reliability reliability = Reliability::reliable;
  Durability durability = Durability::volatile_;
  double deadline = 0.0;       // the longest gap between messages, in seconds; zero means no promise
  Liveliness liveliness = Liveliness::automatic;
  double lease = 0.0;          // how long before silence means the publisher is dead; zero means no promise
};

// The usual starting points.
inline Qos default_profile() { return Qos(); }
inline Qos sensor_data_profile() {
  Qos q;
  q.reliability = Reliability::best_effort;
  return q;
}
inline Qos latched_profile() {
  Qos q;
  q.durability = Durability::transient_local;
  return q;
}

struct Compatibility {
  bool compatible = true;
  std::vector<std::string> reasons;   // one for each property that does not fit
};

// Whether what a publisher offers meets what a subscriber requests.
//   reliability: a reliable offer meets either request; a best effort offer meets only a best effort request
//   durability:  a transient local offer meets either request; a volatile offer meets only a volatile request
//   deadline:    the offer must be at least as quick as the request. No deadline requested is met by anything; no deadline offered cannot meet one.
//   liveliness:  the offered kind must be at least as strong as the requested kind, and the offered lease no longer than the requested one.
//                A lease of zero offered is no promise and cannot meet one requested.
inline Compatibility check(const Qos& offered, const Qos& requested) {
  Compatibility c;
  if (static_cast<int>(offered.reliability) < static_cast<int>(requested.reliability)) {
    c.compatible = false;
    c.reasons.push_back("reliability: offered best effort, requested reliable");
  }
  if (static_cast<int>(offered.durability) < static_cast<int>(requested.durability)) {
    c.compatible = false;
    c.reasons.push_back("durability: offered volatile, requested transient local");
  }
  if (requested.deadline > 0.0 && (offered.deadline <= 0.0 || offered.deadline > requested.deadline)) {
    c.compatible = false;
    c.reasons.push_back("deadline: offered period is longer than requested, or none offered");
  }
  if (static_cast<int>(offered.liveliness) < static_cast<int>(requested.liveliness)) {
    c.compatible = false;
    c.reasons.push_back("liveliness: offered kind is weaker than requested");
  }
  if (requested.lease > 0.0 && (offered.lease <= 0.0 || offered.lease > requested.lease)) {
    c.compatible = false;
    c.reasons.push_back("liveliness lease: offered lease is longer than requested, or none offered");
  }
  return c;
}

struct Endpoint {
  std::string node;
  std::string topic;
  Qos qos;
};

struct Link {
  std::string publisher;
  std::string subscriber;
  std::string topic;
};

struct Mismatch {
  std::string publisher;
  std::string subscriber;
  std::string topic;
  std::vector<std::string> reasons;
};

struct MatchResult {
  std::vector<Link> links;
  std::vector<Mismatch> mismatches;
};

// Connect every publisher to every subscriber of the same topic whose request it meets. Pairs that do not fit are not connected and are reported
// with their reasons. Publishers are taken in order, then subscribers in order.
inline MatchResult match(const std::vector<Endpoint>& publishers, const std::vector<Endpoint>& subscribers) {
  MatchResult r;
  for (const Endpoint& p : publishers)
    for (const Endpoint& s : subscribers) {
      if (p.topic != s.topic) continue;
      const Compatibility c = check(p.qos, s.qos);
      if (c.compatible) {
        r.links.push_back(Link{p.node, s.node, p.topic});
      } else {
        r.mismatches.push_back(Mismatch{p.node, s.node, p.topic, c.reasons});
      }
    }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
