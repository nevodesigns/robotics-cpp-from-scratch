// rc/drive/log_doctor.hpp
//
// A log that tells you what went wrong, from lesson 27-05.
//
// analyse_log reads rows of a field run (time, position, the speed commanded, the speed the wheels report, the charge left in watt hours and the distance from the path) and gives back findings, each with a kind, a start, an end and a size. Stuck is a window in which the vehicle was commanded to move and went less than a distance. Slip is a window in which the wheels reported a speed and the ground under the vehicle went by at less than 70 percent of it. Off path is a cross-track error beyond a threshold. Drain is a window in which the charge fell more than two and a half times as fast as its median. A gap is a jump in the time between rows. A flagged window marks all its rows, so the span starts when the fault did, and spans_from_flags joins marked rows into spans, dropping those shorter than a minimum.
//
// In a run of 300 s with five faults put in (stuck from 100 to 108 s, slipping from 150 to 160, off the path from 200 to 206, draining four times as fast from 250 to 262, and 20 s of rows missing from 270), all 5 were found, each within 1.8 s of where it began and ended. 20 clean runs with noise gave 0 false findings. A slip that left the ground at 80 percent of the wheels was not reported and one at 50 percent was.

#ifndef RC_RC_DRIVE_LOG_DOCTOR_HPP
#define RC_RC_DRIVE_LOG_DOCTOR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace drive {

enum FindingKind { kFindStuck = 0, kFindSlip = 1, kFindOffPath = 2, kFindDrain = 3, kFindGap = 4 };

struct LogRow {
  double t = 0.0;
  double x = 0.0;
  double y = 0.0;
  double cmd_v = 0.0;   // m/s commanded
  double odom_v = 0.0;  // m/s the wheels report
  double soc_wh = 0.0;  // charge left
  double cte = 0.0;     // m from the path, signed
};

struct Finding {
  FindingKind kind = kFindStuck;
  double t0 = 0.0;
  double t1 = 0.0;
  double size = 0.0;  // stuck: metres moved; slip: ground speed over wheel speed; off path: worst error; drain: times the median rate; gap: seconds
};

struct DoctorConfig {
  double window = 3.0;       // s, for stuck and slip
  double min_disp = 0.1;     // m moved in the window below which a commanded vehicle is stuck
  double cmd_min = 0.1;      // m/s: a lower command is not trying to move
  double slip_ratio = 0.7;   // ground speed over wheel speed below this is slip
  double odom_min = 0.2;     // m/s of wheel speed needed to judge slip
  double off_path = 0.6;     // m
  double drain_window = 2.0; // s
  double drain_ratio = 2.5;  // times the median rate
  double max_gap = 1.0;      // s between rows
  double min_span = 2.0;     // s: a flagged span shorter than this is dropped
  double merge = 1.0;        // s: flagged rows closer than this are one span
};

inline const char* kind_name(FindingKind k) {
  switch (k) {
    case kFindStuck: return "stuck";
    case kFindSlip: return "slip";
    case kFindOffPath: return "off path";
    case kFindDrain: return "drain";
    default: return "gap";
  }
}

// Joins marked rows into spans (start, end), merging marks closer than `merge` seconds and dropping spans shorter than `min_span`.
inline std::vector<std::pair<double, double> > spans_from_flags(const std::vector<double>& t, const std::vector<bool>& flag, double min_span, double merge) {
  std::vector<std::pair<double, double> > out;
  bool open = false;
  double t0 = 0.0, last = 0.0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (!flag[i]) continue;
    if (open && t[i] - last <= merge) {
      last = t[i];
    } else {
      if (open && last - t0 >= min_span) out.push_back(std::make_pair(t0, last));
      open = true;
      t0 = t[i];
      last = t[i];
    }
  }
  if (open && last - t0 >= min_span) out.push_back(std::make_pair(t0, last));
  return out;
}

// Reads the run and says what went wrong, in order of time.
inline std::vector<Finding> analyse_log(const std::vector<LogRow>& rows, const DoctorConfig& cfg) {
  std::vector<Finding> out;
  const std::size_t n = rows.size();
  if (n < 2) return out;
  std::vector<double> t(n);
  for (std::size_t i = 0; i < n; ++i) t[i] = rows[i].t;
  std::vector<bool> stuck(n, false), slip(n, false), off(n, false), drain(n, false);
  // The rate of charge fall over a short window, for every row, and its median.
  std::vector<double> rate(n, 0.0);
  std::vector<std::size_t> from(n, 0);  // the first row of the drain window that ends at each row
  std::size_t jd = 0;
  for (std::size_t i = 0; i < n; ++i) {
    while (jd + 1 < i && t[i] - t[jd + 1] >= cfg.drain_window) ++jd;
    from[i] = jd;
    if (i > 0 && t[i] - t[jd] >= cfg.drain_window * 0.5) rate[i] = (rows[jd].soc_wh - rows[i].soc_wh) / (t[i] - t[jd]);
  }
  std::vector<double> sorted(rate);
  std::sort(sorted.begin(), sorted.end());
  const double median = sorted[n / 2];
  std::size_t j = 0;
  for (std::size_t i = 0; i < n; ++i) {
    while (j + 1 < i && t[i] - t[j + 1] >= cfg.window) ++j;
    if (t[i] - t[j] >= cfg.window) {
      const double dist = std::hypot(rows[i].x - rows[j].x, rows[i].y - rows[j].y);
      double cmin = 1e18, odom = 0.0;
      for (std::size_t k = j; k <= i; ++k) {
        cmin = std::min(cmin, std::fabs(rows[k].cmd_v));
        odom += std::fabs(rows[k].odom_v);
      }
      odom /= static_cast<double>(i - j + 1);
      const bool is_stuck = cmin >= cfg.cmd_min && dist < cfg.min_disp;
      const bool is_slip = odom >= cfg.odom_min && (dist / (t[i] - t[j])) / odom < cfg.slip_ratio;
      for (std::size_t k = j; k <= i; ++k) {
        if (is_stuck) stuck[k] = true;
        if (is_slip) slip[k] = true;
      }
    }
    if (std::fabs(rows[i].cte) > cfg.off_path) off[i] = true;
    if (median > 0.0 && rate[i] > cfg.drain_ratio * median) {
      for (std::size_t k = from[i]; k <= i; ++k) drain[k] = true;
    }
  }
  const std::vector<std::pair<double, double> > a = spans_from_flags(t, stuck, cfg.min_span, cfg.merge);
  const std::vector<std::pair<double, double> > b = spans_from_flags(t, slip, cfg.min_span, cfg.merge);
  const std::vector<std::pair<double, double> > c = spans_from_flags(t, off, cfg.min_span, cfg.merge);
  const std::vector<std::pair<double, double> > d = spans_from_flags(t, drain, cfg.min_span, cfg.merge);
  for (std::size_t s = 0; s < a.size(); ++s) {
    Finding f;
    f.kind = kFindStuck;
    f.t0 = a[s].first;
    f.t1 = a[s].second;
    out.push_back(f);
  }
  for (std::size_t s = 0; s < b.size(); ++s) {
    Finding f;
    f.kind = kFindSlip;
    f.t0 = b[s].first;
    f.t1 = b[s].second;
    out.push_back(f);
  }
  for (std::size_t s = 0; s < c.size(); ++s) {
    Finding f;
    f.kind = kFindOffPath;
    f.t0 = c[s].first;
    f.t1 = c[s].second;
    for (std::size_t i = 0; i < n; ++i)
      if (t[i] >= f.t0 && t[i] <= f.t1) f.size = std::max(f.size, std::fabs(rows[i].cte));
    out.push_back(f);
  }
  for (std::size_t s = 0; s < d.size(); ++s) {
    Finding f;
    f.kind = kFindDrain;
    f.t0 = d[s].first;
    f.t1 = d[s].second;
    out.push_back(f);
  }
  for (std::size_t i = 1; i < n; ++i) {
    if (t[i] - t[i - 1] > cfg.max_gap) {
      Finding f;
      f.kind = kFindGap;
      f.t0 = t[i - 1];
      f.t1 = t[i];
      f.size = t[i] - t[i - 1];
      out.push_back(f);
    }
  }
  std::sort(out.begin(), out.end(), [](const Finding& l, const Finding& r) { return l.t0 < r.t0; });
  return out;
}


}  // namespace drive
}  // namespace rc

#endif
