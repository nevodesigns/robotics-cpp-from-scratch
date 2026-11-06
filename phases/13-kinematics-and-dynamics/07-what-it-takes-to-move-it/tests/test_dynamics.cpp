#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kPi = 3.141592653589793;
const double kPayload = 0.5;

// ---------------------------------------------------------------------------
// The arm as physics, written down a different way from the code under test: the
// positions of the centres of mass, and the kinetic and potential energy of each.
// ---------------------------------------------------------------------------

struct Point {
  double x = 0.0;
  double y = 0.0;
};

struct Centres {
  Point link1;
  Point link2;
  Point tool;
};

Centres centres(const ArmBody& a, double q1, double q2) {
  const double q12 = q1 + q2;
  Centres c;
  c.link1 = {a.c1 * std::cos(q1), a.c1 * std::sin(q1)};
  c.link2 = {a.l1 * std::cos(q1) + a.c2 * std::cos(q12), a.l1 * std::sin(q1) + a.c2 * std::sin(q12)};
  c.tool = {a.l1 * std::cos(q1) + a.l2 * std::cos(q12), a.l1 * std::sin(q1) + a.l2 * std::sin(q12)};
  return c;
}

// The kinetic energy for joint velocities (w1, w2). A velocity of a point is how fast
// it moves as the joints move, found by moving them a little each way, and the
// links also spin about their own centres.
double kinetic(const ArmBody& a, const LinkInertia& inertia, double q1, double q2, double w1, double w2,
               double payload) {
  const double h = 1e-6;
  const Centres ahead = centres(a, q1 + h * w1, q2 + h * w2);
  const Centres behind = centres(a, q1 - h * w1, q2 - h * w2);

  const auto speed_squared = [h](const Point& p, const Point& q) {
    const double dx = (p.x - q.x) / (2.0 * h);
    const double dy = (p.y - q.y) / (2.0 * h);
    return dx * dx + dy * dy;
  };

  return 0.5 * a.m1 * speed_squared(ahead.link1, behind.link1) +
         0.5 * a.m2 * speed_squared(ahead.link2, behind.link2) +
         0.5 * payload * speed_squared(ahead.tool, behind.tool) + 0.5 * inertia.i1 * w1 * w1 +
         0.5 * inertia.i2 * (w1 + w2) * (w1 + w2);
}

double potential(const ArmBody& a, double q1, double q2, double payload) {
  const Centres c = centres(a, q1, q2);
  return a.gravity * (a.m1 * c.link1.y + a.m2 * c.link2.y + payload * c.tool.y);
}

double total_energy(const ArmBody& a, const LinkInertia& inertia, double q1, double q2, double w1, double w2,
                    double payload) {
  return kinetic(a, inertia, q1, q2, w1, w2, payload) + potential(a, q1, q2, payload);
}

// ---------------------------------------------------------------------------
// A plant to control: the arm itself, moved by motor torques. It uses the
// functions under test for the dynamics, and that is deliberate: the energy test
// below establishes they are right, and a plant that is wrong would make every
// control result meaningless.
// ---------------------------------------------------------------------------

struct State {
  double q1 = 0.0;
  double q2 = 0.0;
  double v1 = 0.0;
  double v2 = 0.0;
};

void accelerations(const ArmBody& a, const LinkInertia& inertia, const State& s, double u1, double u2,
                   double payload, double& a1, double& a2) {
  const JointTorques bias = inverse_dynamics(a, inertia, s.q1, s.q2, s.v1, s.v2, 0.0, 0.0, payload);
  const MassMatrix m = mass_matrix(a, inertia, s.q2, payload);
  const double det = m.m11 * m.m22 - m.m12 * m.m12;
  const double r1 = u1 - bias.shoulder;
  const double r2 = u2 - bias.elbow;
  a1 = (m.m22 * r1 - m.m12 * r2) / det;
  a2 = (-m.m12 * r1 + m.m11 * r2) / det;
}

State rk4(const ArmBody& a, const LinkInertia& inertia, const State& s, double u1, double u2, double payload,
          double dt) {
  double k1a, k1b, k2a, k2b, k3a, k3b, k4a, k4b;
  accelerations(a, inertia, s, u1, u2, payload, k1a, k1b);

  State s2{s.q1 + 0.5 * dt * s.v1, s.q2 + 0.5 * dt * s.v2, s.v1 + 0.5 * dt * k1a, s.v2 + 0.5 * dt * k1b};
  accelerations(a, inertia, s2, u1, u2, payload, k2a, k2b);

  State s3{s.q1 + 0.5 * dt * s2.v1, s.q2 + 0.5 * dt * s2.v2, s.v1 + 0.5 * dt * k2a, s.v2 + 0.5 * dt * k2b};
  accelerations(a, inertia, s3, u1, u2, payload, k3a, k3b);

  State s4{s.q1 + dt * s3.v1, s.q2 + dt * s3.v2, s.v1 + dt * k3a, s.v2 + dt * k3b};
  accelerations(a, inertia, s4, u1, u2, payload, k4a, k4b);

  State out;
  out.q1 = s.q1 + dt * s.v1 + dt * dt / 6.0 * (k1a + k2a + k3a);
  out.q2 = s.q2 + dt * s.v2 + dt * dt / 6.0 * (k1b + k2b + k3b);
  out.v1 = s.v1 + dt / 6.0 * (k1a + 2.0 * k2a + 2.0 * k3a + k4a);
  out.v2 = s.v2 + dt / 6.0 * (k1b + 2.0 * k2b + 2.0 * k3b + k4b);
  return out;
}

// A smooth move from 0 to 1 that starts and ends at rest, and its first two
// derivatives: a quintic, so that the acceleration is continuous.
double blend(double t, double duration) {
  if (t >= duration) return 1.0;
  const double x = t / duration;
  return x * x * x * (10.0 - 15.0 * x + 6.0 * x * x);
}
double blend_rate(double t, double duration) {
  if (t >= duration) return 0.0;
  const double x = t / duration;
  return (30.0 * x * x - 60.0 * x * x * x + 30.0 * x * x * x * x) / duration;
}
double blend_acceleration(double t, double duration) {
  if (t >= duration) return 0.0;
  const double x = t / duration;
  return (60.0 * x - 180.0 * x * x + 120.0 * x * x * x) / (duration * duration);
}

}  // namespace

RC_TEST("the mass matrix is the kinetic energy, found from the positions of the masses") {
  const ArmBody arm;
  const LinkInertia inertia;
  double worst = 0.0;
  int tried = 0;

  for (int j = -6; j <= 6; ++j) {
    for (const double payload : {0.0, 0.5, 2.0}) {
      const double q1 = 0.4;
      const double q2 = static_cast<double>(j) * 0.5;

      // Kinetic energy is one half of velocity times M times velocity, so three
      // velocities give the three numbers.
      const double e11 = kinetic(arm, inertia, q1, q2, 1.0, 0.0, payload);
      const double e22 = kinetic(arm, inertia, q1, q2, 0.0, 1.0, payload);
      const double e12 = kinetic(arm, inertia, q1, q2, 1.0, 1.0, payload);
      const double m11 = 2.0 * e11;
      const double m22 = 2.0 * e22;
      const double m12 = e12 - 0.5 * (m11 + m22);

      const MassMatrix mine = mass_matrix(arm, inertia, q2, payload);
      worst = std::fmax(worst, std::fmax(std::fabs(mine.m11 - m11),
                                         std::fmax(std::fabs(mine.m12 - m12), std::fabs(mine.m22 - m22))));
      ++tried;
    }
  }

  std::cout << "  " << tried << " elbow angles and payloads, against the kinetic energy of the same arm:\n"
            << "  the worst disagreement in any entry was " << std::scientific << std::setprecision(1) << worst
            << ".\n\n";

  RC_CHECK_EQ(tried, 39);
  RC_CHECK(worst < 1e-6);
}

RC_TEST("the inertia depends on the elbow and on nothing else, and it is always a real inertia") {
  const ArmBody arm;
  const LinkInertia inertia;

  std::cout << "  the inertia the shoulder sees, as the elbow folds in (0.5 kg payload):\n"
            << "    elbow angle   M11 (kg m^2)\n";
  double stretched = 0.0;
  double folded = 0.0;
  bool always_positive = true;
  for (int j = 0; j <= 12; ++j) {
    const double q2 = static_cast<double>(j) * kPi / 12.0;
    const MassMatrix m = mass_matrix(arm, inertia, q2, kPayload);
    if (j % 4 == 0) {
      std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(8) << q2 << std::setw(14)
                << std::setprecision(3) << m.m11 << "\n";
    }
    if (j == 0) stretched = m.m11;
    if (j == 12) folded = m.m11;

    // Positive definite: a mass matrix that is not describes an arm that speeds
    // up when pushed back.
    if (!(m.m11 > 0.0 && m.m22 > 0.0 && m.m11 * m.m22 - m.m12 * m.m12 > 0.0)) always_positive = false;
  }
  std::cout << "  folded all the way it is " << std::setprecision(1) << stretched / folded
            << " times easier to swing than stretched out.\n\n";

  RC_CHECK(always_positive);
  RC_CHECK(stretched / folded > 3.0);

  // Turning the whole arm does not change how its parts are arranged.
  const MassMatrix a = mass_matrix(arm, inertia, 0.7, kPayload);
  const MassMatrix b = mass_matrix(arm, inertia, 0.7, kPayload);
  RC_CHECK_EQ(a.m11, b.m11);
  RC_CHECK(a.m12 > 0.0);
}

RC_TEST("at rest the torque is the weight, and nothing else is left") {
  const ArmBody arm;
  const LinkInertia inertia;
  for (int i = -4; i <= 4; ++i) {
    const double q1 = static_cast<double>(i) * 0.4;
    const double q2 = 0.3 + static_cast<double>(i) * 0.2;
    const JointTorques moving = inverse_dynamics(arm, inertia, q1, q2, 0.0, 0.0, 0.0, 0.0, kPayload);
    const JointTorques holding = holding_torques(arm, q1, q2, kPayload);
    RC_CHECK_NEAR(moving.shoulder, holding.shoulder, 1e-12);
    RC_CHECK_NEAR(moving.elbow, holding.elbow, 1e-12);
  }
}

RC_TEST("the velocity terms are exactly right, because a swinging arm keeps its energy") {
  // With no motor torque and no friction, nothing adds or removes energy: the arm
  // swings and the total of kinetic and potential stays what it started as. That
  // holds only if the inertia, the weight and the Coriolis terms agree with one
  // another, and a sign wrong in any of them makes it drift.
  const ArmBody arm;
  const LinkInertia inertia;
  State s{0.5, 0.8, 0.0, 0.0};
  const double start = total_energy(arm, inertia, s.q1, s.q2, s.v1, s.v2, kPayload);

  double worst = 0.0;
  double fastest = 0.0;
  for (int step = 0; step < 5000; ++step) {
    s = rk4(arm, inertia, s, 0.0, 0.0, kPayload, 0.001);
    worst = std::fmax(worst, std::fabs(total_energy(arm, inertia, s.q1, s.q2, s.v1, s.v2, kPayload) - start));
    fastest = std::fmax(fastest, std::fmax(std::fabs(s.v1), std::fabs(s.v2)));
  }

  std::cout << "  five seconds of a free swing from rest, which reaches " << std::fixed << std::setprecision(1)
            << fastest << " rad/s at a joint:\n"
            << "  the total energy, " << std::setprecision(3) << start << " J at the start, drifted by at most "
            << std::scientific << std::setprecision(1) << worst << " J.\n\n";

  RC_CHECK(fastest > 3.0);       // it really did swing hard, so the velocity terms mattered
  RC_CHECK(worst < 1e-4);
}

RC_TEST("the same gains settle differently at different poses, because the inertia is different") {
  const ArmBody arm;
  const LinkInertia inertia;
  const double kp = 60.0;
  const double kd = 12.0;

  struct Result {
    double settle = 0.0;
    double overshoot = 0.0;
  };

  const auto run = [&](double elbow) {
    State s{0.5, elbow, 0.0, 0.0};
    const double target = 0.8;
    const JointTorques hold = holding_torques(arm, target, elbow, kPayload);
    Result r;
    double peak = s.q1;
    for (int i = 0; i < 4000; ++i) {
      const double t = static_cast<double>(i) * 0.001;
      const double u1 = kp * (target - s.q1) - kd * s.v1 + hold.shoulder;
      const double u2 = kp * (elbow - s.q2) - kd * s.v2 + hold.elbow;
      s = rk4(arm, inertia, s, u1, u2, kPayload, 0.001);
      peak = std::fmax(peak, s.q1);
      if (std::fabs(s.q1 - target) > 0.02 * 0.3) r.settle = t;
    }
    r.overshoot = 100.0 * (peak - target) / 0.3;
    return r;
  };

  const Result stretched = run(0.0);
  const Result folded = run(2.8);

  std::cout << "  a 0.3 rad shoulder step with gains of 60 and 12, settling to within two percent:\n\n"
            << "    elbow        M11     overshoot   settled after\n"
            << "    stretched  " << std::fixed << std::setprecision(3) << mass_matrix(arm, inertia, 0.0, kPayload).m11
            << std::setprecision(1) << std::setw(10) << stretched.overshoot << " %" << std::setprecision(3)
            << std::setw(11) << stretched.settle << " s\n"
            << "    folded     " << std::setprecision(3) << mass_matrix(arm, inertia, 2.8, kPayload).m11
            << std::setprecision(1) << std::setw(10) << folded.overshoot << " %" << std::setprecision(3)
            << std::setw(11) << folded.settle << " s\n\n";

  // One set of gains, tuned for neither: lightly damped where the arm is heavy, and
  // sluggish where it is light, because damping over the square root of the
  // stiffness times the inertia is a different number at each.
  RC_CHECK(stretched.overshoot > 0.5);
  RC_CHECK(folded.overshoot < 0.5);
  RC_CHECK(folded.settle > stretched.settle + 0.1);
}

RC_TEST("feeding the inverse dynamics forward tracks a fast move that gravity alone cannot") {
  const ArmBody arm;
  const LinkInertia inertia;
  const double duration = 0.4;
  const double kp = 80.0;
  const double kd = 14.0;

  struct Result {
    double worst_mm = 0.0;
    double rms_mm = 0.0;
    double elbow_swing = 0.0;   // how far the elbow strayed from its commanded path, radians
  };

  // The shoulder swings 1.0 rad in 0.4 s and the elbow 1.4 rad, or, in the second
  // run, the elbow is told to stay where it is.
  const auto run = [&](bool full_dynamics, double elbow_travel) {
    State s{0.2, 0.2, 0.0, 0.0};
    Result r;
    double sum = 0.0;
    int n = 0;

    for (int i = 0; i < 1500; ++i) {
      const double t = static_cast<double>(i) * 0.001;
      const double k = blend(t, duration);
      const double d1 = 0.2 + 1.0 * k;
      const double d2 = 0.2 + elbow_travel * k;
      const double w1 = 1.0 * blend_rate(t, duration);
      const double w2 = elbow_travel * blend_rate(t, duration);
      const double z1 = 1.0 * blend_acceleration(t, duration);
      const double z2 = elbow_travel * blend_acceleration(t, duration);

      const JointTorques feed = full_dynamics ? inverse_dynamics(arm, inertia, d1, d2, w1, w2, z1, z2, kPayload)
                                              : holding_torques(arm, d1, d2, kPayload);
      const double u1 = kp * (d1 - s.q1) + kd * (w1 - s.v1) + feed.shoulder;
      const double u2 = kp * (d2 - s.q2) + kd * (w2 - s.v2) + feed.elbow;
      s = rk4(arm, inertia, s, u1, u2, kPayload, 0.001);

      const double ex = (arm.l1 * std::cos(s.q1) + arm.l2 * std::cos(s.q1 + s.q2)) -
                        (arm.l1 * std::cos(d1) + arm.l2 * std::cos(d1 + d2));
      const double ey = (arm.l1 * std::sin(s.q1) + arm.l2 * std::sin(s.q1 + s.q2)) -
                        (arm.l1 * std::sin(d1) + arm.l2 * std::sin(d1 + d2));
      const double e = std::hypot(ex, ey) * 1000.0;
      r.worst_mm = std::fmax(r.worst_mm, e);
      r.elbow_swing = std::fmax(r.elbow_swing, std::fabs(s.q2 - d2));
      sum += e * e;
      ++n;
    }
    r.rms_mm = std::sqrt(sum / static_cast<double>(n));
    return r;
  };

  const Result gravity_only = run(false, 1.4);
  const Result with_dynamics = run(true, 1.4);
  const Result pulled_gravity = run(false, 0.0);
  const Result pulled_dynamics = run(true, 0.0);

  std::cout << "  a shoulder swing of 1.0 rad in 0.4 s with the elbow moving too, gains of 80 and 14:\n\n"
            << "    feedforward                  worst tool error   rms tool error\n"
            << "    gravity only                 " << std::fixed << std::setprecision(1) << std::setw(10)
            << gravity_only.worst_mm << " mm" << std::setw(12) << gravity_only.rms_mm << " mm\n"
            << "    gravity and inverse dynamics " << std::setw(10) << with_dynamics.worst_mm << " mm"
            << std::setw(12) << with_dynamics.rms_mm << " mm\n\n"
            << "  and the same swing with the elbow told to hold still, how far it was pulled off:\n\n"
            << "    gravity only                 " << std::setprecision(3) << std::setw(10)
            << pulled_gravity.elbow_swing << " rad\n"
            << "    gravity and inverse dynamics " << std::setw(10) << pulled_dynamics.elbow_swing << " rad\n\n";

  RC_CHECK(gravity_only.worst_mm > 100.0);
  RC_CHECK(with_dynamics.worst_mm < 20.0);
  RC_CHECK(gravity_only.rms_mm > 10.0 * with_dynamics.rms_mm);

  // The joints are coupled: a shoulder that swings pulls the elbow with it, and
  // the controller that treats them as independent sees that as a disturbance.
  RC_CHECK(pulled_gravity.elbow_swing > 0.05);
  RC_CHECK(pulled_dynamics.elbow_swing < pulled_gravity.elbow_swing / 20.0);
}
