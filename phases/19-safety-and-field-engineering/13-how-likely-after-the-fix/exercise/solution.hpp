#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// A probability, brought into 0 to 1; a value that is not a number is taken as 1, the worst: not knowing is not a small chance.
inline double clamp_probability(double p) {
  // TODO: NaN is 1; otherwise within 0 to 1
  (void)p;
  return 1.0;
}

// All of independent events at once: the product. Of nothing at all: 1 (nothing is needed for it to happen).
inline double p_and(const std::vector<double>& ps) {
  // TODO: the product of the clamped probabilities; 1 for none
  (void)ps;
  return 1.0;
}

// At least one of independent events: one minus the chance that none happens. Of nothing at all: 0.
inline double p_or(const std::vector<double>& ps) {
  // TODO: one minus the product of (1 - clamped p); 0 for none
  (void)ps;
  return 0.0;
}

// A fault tree: the top event happens through a tree of events. A basic event has a probability; an `and` node happens when all its children do, an `or` node when any does.
struct Node {
  enum class Kind { basic, all_of, any_of };
  Kind kind = Kind::basic;
  std::string name;
  double p = 0.0;
  std::vector<Node> kids;
};

inline Node basic(const std::string& name, double p) {
  Node n;
  n.kind = Node::Kind::basic;
  n.name = name;
  n.p = p;
  return n;
}

inline Node all_of(const std::string& name, std::vector<Node> kids) {
  Node n;
  n.kind = Node::Kind::all_of;
  n.name = name;
  n.kids = std::move(kids);
  return n;
}

inline Node any_of(const std::string& name, std::vector<Node> kids) {
  Node n;
  n.kind = Node::Kind::any_of;
  n.name = name;
  n.kids = std::move(kids);
  return n;
}

// The probability of the top event, taking the events as independent.
inline double probability(const Node& n) {
  // TODO: a basic event is its clamped p; otherwise p_and (all_of) or p_or (any_of) of the children's probability
  (void)n;
  return 0.0;
}

// The basic events that, on their own, make the top event happen: those reached from the top through `or` nodes only. An `and` node with more than one child needs the others too, and so passes none up; with one child it is no protection and passes that child's on.
inline void single_points(const Node& n, std::vector<std::string>& out) {
  // TODO: a basic event adds its name; an all_of node with other than one child adds none; otherwise go through the children
  (void)n;
  (void)out;
}

// The chance, averaged over the time between two tests, that a safety function with a dangerous undetected failure rate (per hour) has failed: the mean over 0 to T of 1 - exp(-rate t) = 1 - (1 - exp(-rate T)) / (rate T).
// For a small rate times T this is rate T / 2. Zero for a rate or an interval that is not above zero.
inline double pfd_avg(double rate_per_hour, double test_interval_hours) {
  // TODO: zero unless both are above zero; with x = rate * T: 1 - (1 - exp(-x)) / x
  (void)rate_per_hour;
  (void)test_interval_hours;
  return 0.0;
}

// Two redundant channels, either of which is enough to do the job, each with the same pfd. A fraction beta of the failures are of a common cause (power, a shared wire, one design error, one maintenance mistake) and take both: ((1 - beta) pfd)^2 + beta pfd.
inline double pfd_1oo2(double pfd, double beta) {
  // TODO: beta limited to 0..1; ((1 - b) pfd)^2 + b pfd
  (void)pfd;
  (void)beta;
  return 0.0;
}

// How often the hazard happens (per year): the demands on the safety function come this often, and each is met unless it has failed.
inline double hazard_frequency(double demands_per_year, double pfd) {
  // TODO: demands (not below 0) times the clamped pfd
  (void)demands_per_year;
  (void)pfd;
  return 0.0;
}

// The likelihood class of lesson 19-01 for a frequency per year: 5 at once a year or more, 4 from once in ten years, 3 from once in a hundred, 2 from once in a thousand, 1 below that.
inline int likelihood_from_frequency(double per_year) {
  // TODO: 5 if NaN or 1 or more a year; 4 from 0.1; 3 from 0.01; 2 from 0.001; else 1
  (void)per_year;
  return 1;
}

#endif  // LESSON_SOLUTION_HPP
