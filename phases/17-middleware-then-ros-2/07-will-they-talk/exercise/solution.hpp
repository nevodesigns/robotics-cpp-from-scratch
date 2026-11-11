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
  // TODO: For each property, if the offer does not meet the request set compatible = false and add a reason (any text):
  // reliability and durability: the offered enum value (cast to int) must be at least the requested one;
  // deadline: if requested.deadline > 0 then offered.deadline must be above 0 and no more than the requested one;
  // liveliness: the offered kind at least the requested; lease: if requested.lease > 0 the offered lease must be above 0 and no more than the requested one.
  (void)offered;
  (void)requested;
  return Compatibility();
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
  // TODO: For each publisher, in order, and each subscriber, in order, on the same topic: check(p.qos, s.qos).
  // Compatible: add Link{p.node, s.node, topic} to links. Otherwise add Mismatch{p.node, s.node, topic, reasons} to mismatches.
  (void)publishers;
  (void)subscribers;
  return MatchResult();
}

#endif  // LESSON_SOLUTION_HPP
