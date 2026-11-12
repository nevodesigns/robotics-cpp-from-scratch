// rc/field/score.hpp
//
// Scoring a run, from lesson 22-10.
//
// score_navigation reads the "pose" channel of a log against its scenario and gives the goals reached in the order asked, the time of the last, the length of the path, how straight it was, the least clearance to a wall or post (less the robot's radius), the collisions as events (a stretch of samples inside an obstacle is one), and the top speed. summarise_trials gives a success rate and the mean and 95th percentile of the times and the worst error for a set of manipulation trials. gait_metrics gives the duty factor, the stride period and its regularity from a foot's contact signal. failures() lists the criteria a score does not meet, in words.
//
// A straight 10 m run scored an efficiency of 1.0000 and a V shaped detour 0.9285; a path through two posts was 110 samples in contact and 2 collisions; 20 trials gave 90.0 percent success, a mean time of 4.72 s and a 95th percentile of 9.00 s; a steady trot had a duty factor of 0.60 and a period of 0.500 s, and a stumble raised the irregularity from 0.000 to 0.222.

#ifndef RC_RC_FIELD_SCORE_HPP
#define RC_RC_FIELD_SCORE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/field/field_scenario.hpp>
#include <rc/field/run_log.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace rc {
namespace field {

struct NavScore {
  int goals_reached = 0;       // goals reached in the order they were given
  double time_to_last = -1.0;  // when the last of those was reached, or -1 if none was
  double path_length = 0.0;
  double straightness = 1.0;   // the straight distance from the first pose to the last, over the path length
  double min_clearance = 1.0e9;  // the least distance to anything, less the robot's radius (negative: inside it)
  int collisions = 0;          // events: a stretch of samples inside an obstacle counts once
  double max_speed = 0.0;
};

inline double distance_to_segment(double px, double py, const rc::field::Segment& s) {
  const double ex = s.x1 - s.x0;
  const double ey = s.y1 - s.y0;
  const double len2 = ex * ex + ey * ey;
  double u = len2 > 0.0 ? ((px - s.x0) * ex + (py - s.y0) * ey) / len2 : 0.0;
  u = std::max(0.0, std::min(1.0, u));
  return std::hypot(px - (s.x0 + u * ex), py - (s.y0 + u * ey));
}

// The distance from a point to the nearest wall, or to the edge of the nearest post.
inline double clearance_at(const FieldScenario& sc, double x, double y) {
  double best = 1.0e9;
  for (std::size_t i = 0; i < sc.walls.size(); ++i) best = std::min(best, distance_to_segment(x, y, sc.walls[i]));
  for (std::size_t i = 0; i < sc.posts.size(); ++i) best = std::min(best, std::hypot(x - sc.posts[i].x, y - sc.posts[i].y) - sc.posts[i].r);
  return best;
}

inline NavScore score_navigation(const FieldScenario& sc, const RunLog& log, double robot_radius) {
  NavScore s;
  bool have_previous = false;
  double px = 0.0, py = 0.0, pt = 0.0;
  double first_x = 0.0, first_y = 0.0;
  bool inside = false;
  std::size_t next_goal = 0;
  for (std::size_t i = 0; i < log.entries.size(); ++i) {
    const LogEntry& e = log.entries[i];
    if (e.channel != "pose" || e.values.size() < 2) continue;
    const double x = e.values[0];
    const double y = e.values[1];
    if (have_previous) {
      const double step = std::hypot(x - px, y - py);
      s.path_length += step;
      if (e.t > pt) s.max_speed = std::max(s.max_speed, step / (e.t - pt));
    } else {
      first_x = x;
      first_y = y;
    }
    const double clear = clearance_at(sc, x, y) - robot_radius;
    s.min_clearance = std::min(s.min_clearance, clear);
    if (clear < 0.0 && !inside) ++s.collisions;
    inside = clear < 0.0;
    while (next_goal < sc.goals.size() && std::hypot(x - sc.goals[next_goal].x, y - sc.goals[next_goal].y) <= sc.goals[next_goal].tolerance) {
      ++s.goals_reached;
      s.time_to_last = e.t;
      ++next_goal;
    }
    px = x;
    py = y;
    pt = e.t;
    have_previous = true;
  }
  if (have_previous && s.path_length > 0.0) s.straightness = std::hypot(px - first_x, py - first_y) / s.path_length;
  return s;
}

struct Trial {
  bool success = false;
  double seconds = 0.0;
  double error_mm = 0.0;
};

struct TrialSummary {
  int count = 0;
  double success_rate = 0.0;  // percent
  double mean_time = 0.0;     // of the successful ones
  double p95_time = 0.0;      // nearest rank, of the successful ones
  double worst_error_mm = 0.0;
};

inline TrialSummary summarise_trials(const std::vector<Trial>& trials) {
  TrialSummary s;
  s.count = static_cast<int>(trials.size());
  std::vector<double> times;
  for (std::size_t i = 0; i < trials.size(); ++i) {
    if (!trials[i].success) continue;
    times.push_back(trials[i].seconds);
    s.mean_time += trials[i].seconds;
    s.worst_error_mm = std::max(s.worst_error_mm, trials[i].error_mm);
  }
  if (!trials.empty()) s.success_rate = 100.0 * static_cast<double>(times.size()) / static_cast<double>(trials.size());
  if (!times.empty()) {
    s.mean_time /= static_cast<double>(times.size());
    std::sort(times.begin(), times.end());
    const std::size_t rank = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(times.size())));
    s.p95_time = times[(rank > 0 ? rank : 1) - 1];
  }
  return s;
}

struct GaitMetrics {
  double duty_factor = 0.0;    // the fraction of the time the foot is on the ground
  double stride_period = 0.0;  // the mean time between touchdowns, in seconds
  double irregularity = 0.0;   // the standard deviation of the periods over their mean
  int strides = 0;
};

// From one foot's contact signal (nonzero for on the ground), sampled every dt seconds.
inline GaitMetrics gait_metrics(const std::vector<int>& contact, double dt) {
  GaitMetrics g;
  if (contact.empty()) return g;
  int down = 0;
  std::vector<double> touchdowns;
  for (std::size_t i = 0; i < contact.size(); ++i) {
    if (contact[i] != 0) ++down;
    if (contact[i] != 0 && i > 0 && contact[i - 1] == 0) touchdowns.push_back(static_cast<double>(i) * dt);
  }
  g.duty_factor = static_cast<double>(down) / static_cast<double>(contact.size());
  if (touchdowns.size() >= 2) {
    std::vector<double> periods;
    double mean = 0.0;
    for (std::size_t i = 1; i < touchdowns.size(); ++i) {
      periods.push_back(touchdowns[i] - touchdowns[i - 1]);
      mean += periods.back();
    }
    mean /= static_cast<double>(periods.size());
    double var = 0.0;
    for (std::size_t i = 0; i < periods.size(); ++i) var += (periods[i] - mean) * (periods[i] - mean);
    var /= static_cast<double>(periods.size());
    g.stride_period = mean;
    g.irregularity = mean > 0.0 ? std::sqrt(var) / mean : 0.0;
    g.strides = static_cast<int>(periods.size());
  }
  return g;
}

struct Criteria {
  int goals_needed = 1;
  double max_time = 1.0e9;
  double min_straightness = 0.0;
  double min_clearance = 0.0;
  int max_collisions = 0;
};

// What the score does not meet, in words. An empty list is a pass.
inline std::vector<std::string> failures(const NavScore& s, const Criteria& c) {
  std::vector<std::string> out;
  std::ostringstream o;
  if (s.goals_reached < c.goals_needed) {
    o << "reached " << s.goals_reached << " goals of " << c.goals_needed;
    out.push_back(o.str());
  }
  if (s.goals_reached >= c.goals_needed && s.time_to_last > c.max_time) {
    o.str("");
    o << "took " << s.time_to_last << " s, more than " << c.max_time;
    out.push_back(o.str());
  }
  if (s.straightness < c.min_straightness) {
    o.str("");
    o << "straightness " << s.straightness << " is below " << c.min_straightness;
    out.push_back(o.str());
  }
  if (s.min_clearance < c.min_clearance) {
    o.str("");
    o << "came within " << s.min_clearance << " m, less than " << c.min_clearance;
    out.push_back(o.str());
  }
  if (s.collisions > c.max_collisions) {
    o.str("");
    o << s.collisions << " collisions, more than " << c.max_collisions;
    out.push_back(o.str());
  }
  return out;
}


}  // namespace field
}  // namespace rc

#endif
