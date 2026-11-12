#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// A probability, brought into 0 to 1; a value that is not a number is taken as 1, the worst: not knowing is not a small chance.
inline double clamp_probability(double p) {
  if (std::isnan(p)) return 1.0;
  return std::max(0.0, std::min(1.0, p));
}

// All of independent events at once: the product. Of nothing at all: 1 (nothing is needed for it to happen).
inline double p_and(const std::vector<double>& ps) {
  double p = 1.0;
  for (double x : ps) p *= clamp_probability(x);
  return p;
}

// At least one of independent events: one minus the chance that none happens. Of nothing at all: 0.
inline double p_or(const std::vector<double>& ps) {
  double none = 1.0;
  for (double x : ps) none *= 1.0 - clamp_probability(x);
  return 1.0 - none;
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
  if (n.kind == Node::Kind::basic) return clamp_probability(n.p);
  std::vector<double> ps;
  for (const Node& k : n.kids) ps.push_back(probability(k));
  return n.kind == Node::Kind::all_of ? p_and(ps) : p_or(ps);
}

// The basic events that, on their own, make the top event happen: those reached from the top through `or` nodes only. An `and` node with more than one child needs the others too, and so passes none up; with one child it is no protection and passes that child's on.
inline void single_points(const Node& n, std::vector<std::string>& out) {
  if (n.kind == Node::Kind::basic) {
    out.push_back(n.name);
    return;
  }
  if (n.kind == Node::Kind::all_of && n.kids.size() != 1) return;
  for (const Node& k : n.kids) single_points(k, out);
}

// The chance, averaged over the time between two tests, that a safety function with a dangerous undetected failure rate (per hour) has failed: the mean over 0 to T of 1 - exp(-rate t) = 1 - (1 - exp(-rate T)) / (rate T).
// For a small rate times T this is rate T / 2. Zero for a rate or an interval that is not above zero.
inline double pfd_avg(double rate_per_hour, double test_interval_hours) {
  if (rate_per_hour <= 0.0 || test_interval_hours <= 0.0) return 0.0;
  const double x = rate_per_hour * test_interval_hours;
  return 1.0 - (1.0 - std::exp(-x)) / x;
}

// Two redundant channels, either of which is enough to do the job, each with the same pfd. A fraction beta of the failures are of a common cause (power, a shared wire, one design error, one maintenance mistake) and take both: ((1 - beta) pfd)^2 + beta pfd.
inline double pfd_1oo2(double pfd, double beta) {
  const double b = std::max(0.0, std::min(1.0, beta));
  return ((1.0 - b) * pfd) * ((1.0 - b) * pfd) + b * pfd;
}

// How often the hazard happens (per year): the demands on the safety function come this often, and each is met unless it has failed.
inline double hazard_frequency(double demands_per_year, double pfd) { return std::max(0.0, demands_per_year) * clamp_probability(pfd); }

// The likelihood class of lesson 19-01 for a frequency per year: 5 at once a year or more, 4 from once in ten years, 3 from once in a hundred, 2 from once in a thousand, 1 below that.
inline int likelihood_from_frequency(double per_year) {
  if (std::isnan(per_year) || per_year >= 1.0) return 5;
  if (per_year >= 0.1) return 4;
  if (per_year >= 0.01) return 3;
  if (per_year >= 0.001) return 2;
  return 1;
}

#endif  // LESSON_SOLUTION_HPP
