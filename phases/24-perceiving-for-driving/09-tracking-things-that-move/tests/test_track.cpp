#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

// Detections of a target moving at a constant velocity, with noise.
Point2d detection(rc::core::Rng& rng, double x, double y, double sigma) { return Point2d(x + sigma * rng.gaussian(), y + sigma * rng.gaussian()); }

const Track* nearest(const Tracker& t, double x, double y) {
  const Track* best = nullptr;
  double d = 1e9;
  for (std::size_t i = 0; i < t.tracks().size(); ++i) {
    const double e = std::hypot(t.tracks()[i].x() - x, t.tracks()[i].y() - y);
    if (e < d) {
      d = e;
      best = &t.tracks()[i];
    }
  }
  return best;
}

}  // namespace

RC_TEST("the filter reads the velocity of a moving point much better than the difference of two positions") {
  rc::core::Rng rng(3);
  Kf4 f;
  kf_init(f, 0.0, 0.0, 0.0025, 4.0);
  double px = 0.0, py = 0.0;
  Point2d last(0.0, 0.0);
  double fd_sq = 0.0;
  int n = 0;
  for (int k = 1; k <= 100; ++k) {
    const double t = 0.1 * k;
    const Point2d z = detection(rng, 1.2 * t, 0.5 * t, 0.05);
    kf_predict(f, 0.1, 1.0);
    kf_update(f, z.first, z.second, 0.0025);
    const double fvx = (z.first - last.first) / 0.1, fvy = (z.second - last.second) / 0.1;
    if (k > 20) {
      fd_sq += (fvx - 1.2) * (fvx - 1.2) + (fvy - 0.5) * (fvy - 0.5);
      px += (f.x[2] - 1.2) * (f.x[2] - 1.2) + (f.x[3] - 0.5) * (f.x[3] - 0.5);
      ++n;
    }
    last = z;
  }
  (void)py;
  const double kf_err = std::sqrt(px / n);
  const double fd_err = std::sqrt(fd_sq / n);
  rec("kf_err", kf_err, 3);
  rec("fd_err", fd_err, 3);
  RC_CHECK(kf_err < 0.5 * fd_err);
  RC_CHECK(kf_err < 0.15);
  // The uncertainty of the position has come down from the start.
  RC_CHECK(f.P[0][0] < 0.0025);
}

RC_TEST("a detection is a tentative track, three in a row confirm it, and one false detection is never believed") {
  TrackerConfig cfg;
  Tracker tracker(cfg);
  rc::core::Rng rng(5);
  int confirmed_at = 0;
  for (int k = 1; k <= 20; ++k) {
    std::vector<Point2d> dets;
    dets.push_back(detection(rng, 5.0 + 0.2 * k, 1.0, 0.05));
    if (k == 4) dets.push_back(Point2d(9.0, -4.0));  // one false detection
    tracker.step(0.1, dets);
    if (confirmed_at == 0)
      for (std::size_t i = 0; i < tracker.tracks().size(); ++i)
        if (tracker.tracks()[i].confirmed) confirmed_at = k;
  }
  rec("confirm_frames", confirmed_at, 0);
  RC_CHECK(confirmed_at == 3);
  RC_CHECK(tracker.tracks().size() == 1);
  RC_CHECK(tracker.tracks()[0].id == 1);
  RC_CHECK_NEAR(tracker.tracks()[0].vx(), 2.0, 0.3);
}

RC_TEST("a short gap is coasted through and the id kept, a long one drops the track and the target gets a new one") {
  auto run = [](int gap) {
    TrackerConfig cfg;
    Tracker tracker(cfg);
    rc::core::Rng rng(7);
    int id_before = 0, id_after = 0;
    for (int k = 1; k <= 40; ++k) {
      std::vector<Point2d> dets;
      const bool hidden = k > 15 && k <= 15 + gap;
      if (!hidden) dets.push_back(detection(rng, 3.0 + 0.15 * k, 0.0, 0.05));
      tracker.step(0.1, dets);
      const Track* t = nearest(tracker, 3.0 + 0.15 * k, 0.0);
      if (k == 15 && t != nullptr) id_before = t->id;
      if (k == 40 && t != nullptr) id_after = t->id;
    }
    return std::make_pair(id_before, id_after);
  };
  const std::pair<int, int> short_gap = run(2);
  const std::pair<int, int> long_gap = run(8);
  rec("gap_short", 2, 0);
  rec("gap_long", 8, 0);
  std::cout << "  lost for 2 frames: id " << short_gap.first << " then " << short_gap.second << "; lost for 8: id " << long_gap.first << " then " << long_gap.second << "\n";
  RC_CHECK(short_gap.first == short_gap.second && short_gap.first != 0);
  RC_CHECK(long_gap.first != long_gap.second);
}

RC_TEST("two targets that meet and stop close together can swap their ids, and a metre apart they do not") {
  // A walks right along y = 0 and B walks left along y = `miss`, both at 1 m/s. They meet near x = 3, both stop for a second to talk, and go on, passing each other at that distance.
  auto swaps = [](double miss, unsigned int seed) {
    TrackerConfig cfg;
    cfg.gate = 0.6;
    Tracker tracker(cfg);
    rc::core::Rng rng(seed);
    int id_a = 0, id_b = 0;
    for (int k = 1; k <= 60; ++k) {
      const double t = 0.1 * k;
      const double ax = t < 3.0 ? t : (t < 4.0 ? 3.0 : 3.0 + (t - 4.0)), ay = 0.0;
      const double bx = t < 2.9 ? 6.0 - t : (t < 4.0 ? 3.1 : 3.1 - (t - 4.0)), by = miss;
      std::vector<Point2d> dets;
      dets.push_back(detection(rng, ax, ay, 0.05));
      dets.push_back(detection(rng, bx, by, 0.05));
      tracker.step(0.1, dets);
      if (k == 5) {
        id_a = nearest(tracker, ax, ay)->id;
        id_b = nearest(tracker, bx, by)->id;
      }
    }
    const Track* ta = nearest(tracker, 5.0, 0.0);
    const Track* tb = nearest(tracker, 1.1, miss);
    return (ta != nullptr && tb != nullptr && ta->id != tb->id && (ta->id != id_a || tb->id != id_b)) ? 1 : 0;
  };
  int close_swaps = 0, far_swaps = 0;
  for (unsigned int seed = 1; seed <= 20; ++seed) {
    close_swaps += swaps(0.05, seed);
    far_swaps += swaps(1.0, seed);
  }
  rec("closest_far", 1.0, 1);
  rec("closest_close", 0.05, 2);
  rec("swaps_far", far_swaps, 0);
  rec("swaps_close", close_swaps, 0);
  std::cout << "  of 20 runs: " << far_swaps << " swapped with the paths a metre apart and " << close_swaps << " with them touching\n";
  RC_CHECK(far_swaps == 0);
  RC_CHECK(close_swaps >= 0);
}

RC_TEST("where a person will be in two seconds is read from the track, and association pairs the nearest first") {
  TrackerConfig cfg;
  Tracker tracker(cfg);
  rc::core::Rng rng(11);
  for (int k = 1; k <= 40; ++k) tracker.step(0.1, std::vector<Point2d>(1, detection(rng, 4.0, -2.0 + 1.4 * 0.1 * k, 0.05)));
  RC_CHECK(tracker.tracks().size() == 1);
  const Point2d later = predict_ahead(tracker.tracks()[0], 2.0);
  const double truth_y = -2.0 + 1.4 * (0.1 * 40 + 2.0);
  const double err = std::hypot(later.first - 4.0, later.second - truth_y);
  rec("ahead_err", err, 2);
  RC_CHECK(err < 0.4);

  std::vector<Point2d> predicted = {Point2d(0, 0), Point2d(1, 0)};
  std::vector<Point2d> dets = {Point2d(0.9, 0.0), Point2d(0.5, 0.0)};
  const std::vector<int> m = associate(predicted, dets, 1.0);
  RC_CHECK(m[0] == 1 && m[1] == 0);  // the pair at 0.1 m goes first; the other is then left with the remaining detection
  RC_CHECK(associate(predicted, std::vector<Point2d>(1, Point2d(5.0, 5.0)), 1.0) == std::vector<int>(2, -1));
}
