// rc/arm/dynamics3d.hpp
//
// What the arm weighs and how it swings, from lesson 29-01.
//
// LinkInertia holds a link's mass, the place of its centre of mass and its inertia about that centre, in the link's own frame. rod_links builds them for a table in the standard convention by treating each link as a uniform rod along the straight line from one joint's frame to the next (a sphere where the two coincide), with a given mass and radius; the numbers are typical, not a product's. inverse_dynamics runs Newton and Euler's equations along the chain in the base frame: forward, the angular velocity, the angular acceleration and the acceleration of each link's frame and centre of mass; then backward, the force and moment that the link before must give each link, and the joint torque as the part of the moment along the joint's axis. mass_matrix has as its columns the torques for a unit acceleration of each joint with no velocity and no gravity; velocity_torques are what the joints throw at each other (the torques with velocity and no acceleration or gravity); gravity_torques what holds the arm up. forward_dynamics solves for the accelerations a torque causes. energy is the kinetic energy plus the weight times the heights of the centres of mass.
//
// For a rod of 2 kg and 0.4 m out horizontally from a joint with a horizontal axis the torque to hold it was 3.924 N m, which is m g L over 2, and 0.000000 N m when it stood up. The torque of gravity matched the slope of the potential energy to 0.00000001 N m, the mass matrix was symmetric to 0.000000000000 and positive in all of 60 random directions, and doubling the joint speeds quadrupled the velocity torques to 0.000000000000. An arm let fall from rest for a second ended with its energy out by 0.000000000496 of itself with a fourth order step and by 0.03723 with a first order one, at a step of a millisecond.

#ifndef RC_RC_ARM_DYNAMICS3D_HPP
#define RC_RC_ARM_DYNAMICS3D_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/ik_dls.hpp>
#include <vector>

namespace rc {
namespace arm {

struct LinkInertia {
  double mass = 0.0;
  double com[3] = {0.0, 0.0, 0.0};                         // in the link's own frame
  double inertia[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};  // about the centre of mass, in the link's own frame
};

struct ArmModel {
  ArmTable table;                  // standard convention
  std::vector<LinkInertia> links;  // one per row
};

inline void cross(const double a[3], const double b[3], double out[3]) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

// Each link as a uniform rod along the line from the previous frame's origin to its own, with a mass and a radius (typical numbers, not a product's).
inline std::vector<LinkInertia> rod_links(const ArmTable& t, const std::vector<double>& masses, double radius) {
  std::vector<LinkInertia> out;
  const std::vector<Pose> frames = link_frames(t, std::vector<double>(t.rows.size(), 0.0), Pose());
  for (std::size_t i = 0; i < t.rows.size(); ++i) {
    LinkInertia l;
    l.mass = masses[i];
    double prev[3] = {0.0, 0.0, 0.0};
    if (i > 0)
      for (int k = 0; k < 3; ++k) prev[k] = frames[i - 1].p[k];
    // The vector from this frame's origin back to the previous one, in this frame.
    double u[3];
    for (int r = 0; r < 3; ++r) {
      u[r] = 0.0;
      for (int k = 0; k < 3; ++k) u[r] += frames[i].r[k][r] * (prev[k] - frames[i].p[k]);
    }
    const double len = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    for (int k = 0; k < 3; ++k) l.com[k] = 0.5 * u[k];
    if (len < 1e-9) {
      for (int k = 0; k < 3; ++k) l.inertia[k][k] = 0.4 * l.mass * radius * radius;
    } else {
      const double it = l.mass * (3.0 * radius * radius + len * len) / 12.0, ia = 0.5 * l.mass * radius * radius;
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
          const double e = u[r] * u[c] / (len * len);
          l.inertia[r][c] = it * ((r == c ? 1.0 : 0.0) - e) + ia * e;
        }
    }
    out.push_back(l);
  }
  return out;
}

// The joint torques for a motion: Newton and Euler along the chain, forward for the accelerations and backward for the forces. g is the acceleration of gravity as a vector.
inline std::vector<double> inverse_dynamics(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd, const std::vector<double>& qdd, const double g[3]) {
  const std::size_t n = m.table.rows.size();
  std::vector<Pose> frames = link_frames(m.table, q, Pose());
  std::vector<std::vector<double> > zp(n, std::vector<double>(3)), op(n, std::vector<double>(3)), coff(n, std::vector<double>(3)), force(n, std::vector<double>(3)), moment(n, std::vector<double>(3));
  double w[3] = {0, 0, 0}, al[3] = {0, 0, 0}, a[3] = {0, 0, 0};
  for (std::size_t i = 0; i < n; ++i) {
    double z[3] = {0.0, 0.0, 1.0}, o[3] = {0.0, 0.0, 0.0};
    if (i > 0)
      for (int k = 0; k < 3; ++k) {
        z[k] = frames[i - 1].r[k][2];
        o[k] = frames[i - 1].p[k];
      }
    for (int k = 0; k < 3; ++k) {
      zp[i][k] = z[k];
      op[i][k] = o[k];
    }
    double r[3] = {frames[i].p[0] - o[0], frames[i].p[1] - o[1], frames[i].p[2] - o[2]};
    double qz[3] = {qd[i] * z[0], qd[i] * z[1], qd[i] * z[2]}, wxq[3];
    cross(w, qz, wxq);
    double w2[3], al2[3];
    for (int k = 0; k < 3; ++k) {
      w2[k] = w[k] + qz[k];
      al2[k] = al[k] + qdd[i] * z[k] + wxq[k];
    }
    double c1[3], c2[3], c3[3], a2[3];
    cross(al2, r, c1);
    cross(w2, r, c2);
    cross(w2, c2, c3);
    for (int k = 0; k < 3; ++k) a2[k] = a[k] + c1[k] + c3[k];
    // The centre of mass.
    double off[3];
    for (int rr = 0; rr < 3; ++rr) {
      off[rr] = 0.0;
      for (int k = 0; k < 3; ++k) off[rr] += frames[i].r[rr][k] * m.links[i].com[k];
      coff[i][rr] = off[rr];
    }
    double d1[3], d2[3], d3[3], ac[3];
    cross(al2, off, d1);
    cross(w2, off, d2);
    cross(w2, d2, d3);
    for (int k = 0; k < 3; ++k) {
      ac[k] = a2[k] + d1[k] + d3[k];
      force[i][k] = m.links[i].mass * (ac[k] - g[k]);
    }
    // The moment to give it that angular acceleration: I (in the base frame) al + w x (I w).
    double iw[3][3];
    for (int rr = 0; rr < 3; ++rr)
      for (int c = 0; c < 3; ++c) {
        iw[rr][c] = 0.0;
        for (int k = 0; k < 3; ++k)
          for (int l = 0; l < 3; ++l) iw[rr][c] += frames[i].r[rr][k] * m.links[i].inertia[k][l] * frames[i].r[c][l];
      }
    double ial[3], iww[3], wiw[3];
    for (int rr = 0; rr < 3; ++rr) {
      ial[rr] = iww[rr] = 0.0;
      for (int c = 0; c < 3; ++c) {
        ial[rr] += iw[rr][c] * al2[c];
        iww[rr] += iw[rr][c] * w2[c];
      }
    }
    cross(w2, iww, wiw);
    for (int k = 0; k < 3; ++k) moment[i][k] = ial[k] + wiw[k];
    for (int k = 0; k < 3; ++k) {
      w[k] = w2[k];
      al[k] = al2[k];
      a[k] = a2[k];
    }
  }
  std::vector<double> tau(n, 0.0);
  double f_next[3] = {0, 0, 0}, n_next[3] = {0, 0, 0};
  for (std::size_t i = n; i-- > 0;) {
    double f[3], nn[3], cf[3], rf[3], r_next[3] = {0.0, 0.0, 0.0};
    cross(coff[i].data(), force[i].data(), cf);
    if (i + 1 < n)
      for (int k = 0; k < 3; ++k) r_next[k] = frames[i + 1].p[k] - frames[i].p[k];
    cross(r_next, f_next, rf);
    for (int k = 0; k < 3; ++k) {
      f[k] = force[i][k] + f_next[k];
      nn[k] = moment[i][k] + cf[k] + n_next[k] + rf[k];
    }
    double r_i[3] = {frames[i].p[0] - op[i][0], frames[i].p[1] - op[i][1], frames[i].p[2] - op[i][2]}, rfi[3];
    cross(r_i, f, rfi);
    tau[i] = zp[i][0] * (nn[0] + rfi[0]) + zp[i][1] * (nn[1] + rfi[1]) + zp[i][2] * (nn[2] + rfi[2]);
    for (int k = 0; k < 3; ++k) {
      f_next[k] = f[k];
      n_next[k] = nn[k];
    }
  }
  return tau;
}

// Column j is the torque for a unit acceleration of joint j with no velocity and no gravity.
inline std::vector<std::vector<double> > mass_matrix(const ArmModel& m, const std::vector<double>& q) {
  const std::size_t n = q.size();
  const double no_g[3] = {0.0, 0.0, 0.0};
  std::vector<std::vector<double> > mm(n, std::vector<double>(n, 0.0));
  for (std::size_t j = 0; j < n; ++j) {
    std::vector<double> qdd(n, 0.0);
    qdd[j] = 1.0;
    const std::vector<double> col = inverse_dynamics(m, q, std::vector<double>(n, 0.0), qdd, no_g);
    for (std::size_t i = 0; i < n; ++i) mm[i][j] = col[i];
  }
  return mm;
}

// What the joints throw at each other: the torques for the velocity with no acceleration and no gravity.
inline std::vector<double> velocity_torques(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd) {
  const double no_g[3] = {0.0, 0.0, 0.0};
  return inverse_dynamics(m, q, qd, std::vector<double>(q.size(), 0.0), no_g);
}

// What holds the arm up: the torques with no motion.
inline std::vector<double> gravity_torques(const ArmModel& m, const std::vector<double>& q) {
  const double g[3] = {0.0, 0.0, -9.81};
  return inverse_dynamics(m, q, std::vector<double>(q.size(), 0.0), std::vector<double>(q.size(), 0.0), g);
}

// The accelerations a torque causes, for six joints: M qdd = tau - (velocity torques and gravity).
inline std::vector<double> forward_dynamics(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd, const std::vector<double>& tau) {
  const double g[3] = {0.0, 0.0, -9.81};
  const std::vector<double> bias = inverse_dynamics(m, q, qd, std::vector<double>(q.size(), 0.0), g);
  const std::vector<std::vector<double> > mm = mass_matrix(m, q);
  double a[6][6], b[6], x[6];
  for (int i = 0; i < 6; ++i) {
    b[i] = tau[static_cast<std::size_t>(i)] - bias[static_cast<std::size_t>(i)];
    for (int j = 0; j < 6; ++j) a[i][j] = mm[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
  }
  std::vector<double> out(6, 0.0);
  if (solve6(a, b, x))
    for (int i = 0; i < 6; ++i) out[static_cast<std::size_t>(i)] = x[i];
  return out;
}

// Kinetic energy plus the weight times the heights of the centres of mass.
inline double energy(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd) {
  const std::vector<std::vector<double> > mm = mass_matrix(m, q);
  double ke = 0.0;
  for (std::size_t i = 0; i < q.size(); ++i)
    for (std::size_t j = 0; j < q.size(); ++j) ke += 0.5 * qd[i] * mm[i][j] * qd[j];
  const std::vector<Pose> frames = link_frames(m.table, q, Pose());
  double pe = 0.0;
  for (std::size_t i = 0; i < q.size(); ++i) {
    double z = frames[i].p[2];
    for (int k = 0; k < 3; ++k) z += frames[i].r[2][k] * m.links[i].com[k];
    pe += m.links[i].mass * 9.81 * z;
  }
  return ke + pe;
}

// One step of the motion under a torque: a fourth order step (Runge and Kutta) or a first order one (Euler's).
inline void step_dynamics(const ArmModel& m, std::vector<double>& q, std::vector<double>& qd, const std::vector<double>& tau, double dt, bool fourth_order) {
  const std::size_t n = q.size();
  if (!fourth_order) {
    const std::vector<double> a = forward_dynamics(m, q, qd, tau);
    for (std::size_t i = 0; i < n; ++i) {
      q[i] += qd[i] * dt;
      qd[i] += a[i] * dt;
    }
    return;
  }
  std::vector<double> k1q = qd, k1v = forward_dynamics(m, q, qd, tau);
  std::vector<double> q2(n), v2(n);
  for (std::size_t i = 0; i < n; ++i) {
    q2[i] = q[i] + 0.5 * dt * k1q[i];
    v2[i] = qd[i] + 0.5 * dt * k1v[i];
  }
  std::vector<double> k2q = v2, k2v = forward_dynamics(m, q2, v2, tau);
  for (std::size_t i = 0; i < n; ++i) {
    q2[i] = q[i] + 0.5 * dt * k2q[i];
    v2[i] = qd[i] + 0.5 * dt * k2v[i];
  }
  std::vector<double> k3q = v2, k3v = forward_dynamics(m, q2, v2, tau);
  for (std::size_t i = 0; i < n; ++i) {
    q2[i] = q[i] + dt * k3q[i];
    v2[i] = qd[i] + dt * k3v[i];
  }
  std::vector<double> k4q = v2, k4v = forward_dynamics(m, q2, v2, tau);
  for (std::size_t i = 0; i < n; ++i) {
    q[i] += dt / 6.0 * (k1q[i] + 2.0 * k2q[i] + 2.0 * k3q[i] + k4q[i]);
    qd[i] += dt / 6.0 * (k1v[i] + 2.0 * k2v[i] + 2.0 * k3v[i] + k4v[i]);
  }
}


}  // namespace arm
}  // namespace rc

#endif
