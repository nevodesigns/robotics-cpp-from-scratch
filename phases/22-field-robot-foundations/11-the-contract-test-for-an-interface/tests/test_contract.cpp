#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

// A drive that forwards everything to a FakeDrive, so that a broken one only has to change the one thing it breaks.
class Forward : public Drive {
 public:
  explicit Forward(const DriveLimits& l = DriveLimits()) : inner_(l) {}
  void command(const DriveCommand& c) override { inner_.command(c); }
  void stop() override { inner_.stop(); }
  void step(double dt) override { inner_.step(dt); }
  DriveState state() const override { return inner_.state(); }

 protected:
  FakeDrive inner_;
};

DriveLimits no_watchdog() {
  DriveLimits l;
  l.watchdog = 1.0e9;
  return l;
}

DriveLimits no_limit() {
  DriveLimits l;
  l.max_v = 1.0e9;
  l.max_w = 1.0e9;
  return l;
}

class NoWatchdog : public Forward {
 public:
  NoWatchdog() : Forward(no_watchdog()) {}
};

class NoLimit : public Forward {
 public:
  NoLimit() : Forward(no_limit()) {}
};

class WrongTurn : public Forward {
 public:
  void command(const DriveCommand& c) override {
    DriveCommand d = c;
    d.w = -c.w;
    inner_.command(d);
  }
};

class Lying : public Forward {
 public:
  DriveState state() const override {
    DriveState s = inner_.state();
    s.x *= 1.1;
    s.y *= 1.1;
    return s;
  }
};

class NoStop : public Forward {
 public:
  void stop() override {}
};

// The clamp as it is first written: no lag, no watchdog, and std::min and std::max, which turn a NaN into the limit.
class Naive : public Drive {
 public:
  void command(const DriveCommand& c) override {
    s_.v = std::max(-1.0, std::min(1.0, c.v));
    s_.w = std::max(-2.0, std::min(2.0, c.w));
  }
  void stop() override {
    s_.v = 0.0;
    s_.w = 0.0;
  }
  void step(double dt) override {
    s_.x += s_.v * std::cos(s_.theta) * dt;
    s_.y += s_.v * std::sin(s_.theta) * dt;
    s_.theta += s_.w * dt;
  }
  DriveState state() const override { return s_; }

 private:
  DriveState s_;
};

template <class D>
DriveFactory making() {
  return []() { return std::unique_ptr<Drive>(new D()); };
}

bool has(const std::vector<std::string>& v, const std::string& tag) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i].compare(0, tag.size() + 1, tag + ":") == 0) return true;
  return false;
}

void show(const char* name, const std::vector<std::string>& v) {
  for (std::size_t i = 0; i < v.size(); ++i) std::cout << "  " << name << " broke " << v[i] << "\n";
}

// Three buses that each break one promise.
class JumpBus {
 public:
  explicit JumpBus(std::size_t n) : p_(n, 0.0), t_(n, 0.0) {}
  std::size_t joint_count() const { return p_.size(); }
  bool set_targets(const std::vector<double>& t) {
    if (t.size() != t_.size()) return false;
    for (std::size_t i = 0; i < t.size(); ++i)
      if (!std::isfinite(t[i])) return false;
    t_ = t;
    return true;
  }
  void step(double) { p_ = t_; }
  const std::vector<double>& positions() const { return p_; }
  bool at_target(double tol) const {
    for (std::size_t i = 0; i < p_.size(); ++i)
      if (std::fabs(p_[i] - t_[i]) > tol) return false;
    return true;
  }

 private:
  std::vector<double> p_, t_;
};

class SloppyBus {
 public:
  explicit SloppyBus(std::size_t n) : p_(n, 0.0), t_(n, 0.0) {}
  std::size_t joint_count() const { return p_.size(); }
  bool set_targets(const std::vector<double>& t) {
    t_ = t;
    t_.resize(p_.size(), 0.0);
    return true;
  }
  void step(double dt) {
    for (std::size_t i = 0; i < p_.size(); ++i) {
      const double gap = t_[i] - p_[i];
      p_[i] += std::max(-0.5 * dt, std::min(0.5 * dt, gap));
    }
  }
  const std::vector<double>& positions() const { return p_; }
  bool at_target(double tol) const {
    for (std::size_t i = 0; i < p_.size(); ++i)
      if (std::fabs(p_[i] - t_[i]) > tol) return false;
    return true;
  }

 private:
  std::vector<double> p_, t_;
};

class DriftBus {
 public:
  explicit DriftBus(std::size_t n) : inner_(n, 0.5) {}
  std::size_t joint_count() const { return inner_.joint_count(); }
  bool set_targets(const std::vector<double>& t) { return inner_.set_targets(t); }
  void step(double dt) {
    inner_.step(dt);
    if (inner_.at_target(1e-3)) offset_ += 1e-5;
  }
  const std::vector<double>& positions() const {
    shown_ = inner_.positions();
    for (std::size_t i = 0; i < shown_.size(); ++i) shown_[i] += offset_;
    return shown_;
  }
  bool at_target(double tol) const { return inner_.at_target(tol); }

 private:
  ArmBus inner_;
  double offset_ = 0.0;
  mutable std::vector<double> shown_;
};

}  // namespace

RC_TEST("the fake drive of lesson 22-01 keeps every promise") {
  const std::vector<std::string> v = check_drive_contract(making<Forward>(), DriveExpect());
  show("the fake", v);
  rec("n_good", static_cast<double>(v.size()), 0);
  RC_CHECK(v.empty());
}

RC_TEST("each broken drive is caught, by the promise it breaks") {
  const DriveExpect e;
  const std::vector<std::string> a = check_drive_contract(making<NoWatchdog>(), e);
  const std::vector<std::string> b = check_drive_contract(making<NoLimit>(), e);
  const std::vector<std::string> c = check_drive_contract(making<WrongTurn>(), e);
  const std::vector<std::string> d = check_drive_contract(making<Lying>(), e);
  const std::vector<std::string> f = check_drive_contract(making<NoStop>(), e);
  const std::vector<std::string> g = check_drive_contract(making<Naive>(), e);
  show("no watchdog", a);
  show("no limit", b);
  show("wrong turn", c);
  show("lying", d);
  show("no stop", f);
  show("naive", g);
  rec("n_nowd", static_cast<double>(a.size()), 0);
  rec("n_nolimit", static_cast<double>(b.size()), 0);
  rec("n_wrongturn", static_cast<double>(c.size()), 0);
  rec("n_lying", static_cast<double>(d.size()), 0);
  rec("n_nostop", static_cast<double>(f.size()), 0);
  rec("n_naive", static_cast<double>(g.size()), 0);
  RC_CHECK(has(a, "watchdog"));
  RC_CHECK(has(b, "limit"));
  RC_CHECK(has(c, "turn"));
  RC_CHECK(has(d, "odometry"));
  RC_CHECK(has(f, "stop"));
  RC_CHECK(has(g, "watchdog") && has(g, "nan"));
  RC_CHECK(!has(a, "nan") && !has(c, "limit") && !has(d, "turn"));
}

RC_TEST("a change in what is expected changes the verdict: a drive that is slower than asked for is caught") {
  DriveExpect fast;
  fast.max_v = 0.5;  // the drive can do 1.0, so it breaks a limit of 0.5
  const std::vector<std::string> v = check_drive_contract(making<Forward>(), fast);
  RC_CHECK(has(v, "limit"));
  DriveExpect strict;
  strict.stop_seconds = 0.3;
  RC_CHECK(has(check_drive_contract(making<Forward>(), strict), "stop"));
}

RC_TEST("the joint buses: the real ones keep the promises and three broken ones are caught") {
  const std::vector<std::string> arm = check_joint_contract<ArmBus>([]() { return ArmBus(3, 0.5); }, 3, 0.5);
  const std::vector<std::string> legs = check_joint_contract<LegBus>([]() { return LegBus(4, 3, 2.0); }, 12, 2.0);
  show("arm", arm);
  show("legs", legs);
  RC_CHECK(arm.empty());
  RC_CHECK(legs.empty());

  const std::vector<std::string> jump = check_joint_contract<JumpBus>([]() { return JumpBus(3); }, 3, 0.5);
  const std::vector<std::string> sloppy = check_joint_contract<SloppyBus>([]() { return SloppyBus(3); }, 3, 0.5);
  const std::vector<std::string> drift = check_joint_contract<DriftBus>([]() { return DriftBus(3); }, 3, 0.5);
  show("jump", jump);
  show("sloppy", sloppy);
  show("drift", drift);
  rec("n_jump", static_cast<double>(jump.size()), 0);
  rec("n_sloppy", static_cast<double>(sloppy.size()), 0);
  rec("n_drift", static_cast<double>(drift.size()), 0);
  RC_CHECK(has(jump, "rate"));
  RC_CHECK(has(sloppy, "size") && has(sloppy, "nan"));
  RC_CHECK(has(drift, "hold"));
}
