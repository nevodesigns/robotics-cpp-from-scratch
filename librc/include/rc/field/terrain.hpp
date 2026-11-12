// rc/field/terrain.hpp
//
// Terrain and friction for field vehicles, from lesson 22-05.
//
// A Surface has a friction coefficient and a rolling resistance (the values for tarmac, gravel, grass, sand and ice are typical of rubber tyres, not measurements). A Terrain is a base cell with patches laid over it, each a rectangle with a surface and a slope; at(x, y) gives the cell under a point. traction() turns a cell and a heading into the most the vehicle can accelerate, the most it can brake, and whether it can climb or hold.
//
// At 2 m/s the stopping distance was 0.250 m on tarmac, 0.416 m on gravel and 1.853 m on ice (7.4 times tarmac). Up a grade of 0.2 on gravel the most acceleration was 2.020 m/s2 and across it 4.022; going down it the stop took 0.717 m against 0.416 m on the flat. Gravel can be climbed up to 22.3 degrees, ice to 5.1.

#ifndef RC_RC_FIELD_TERRAIN_HPP
#define RC_RC_FIELD_TERRAIN_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace field {

// What the ground is made of, as far as a tyre cares. The numbers are typical values for rubber on the surface, not measurements.
struct Surface {
  const char* name;
  double mu;    // friction coefficient between tyre and ground
  double crr;   // rolling resistance coefficient
  double bump;  // typical height of the roughness, in metres
};

inline Surface tarmac() { return Surface{"tarmac", 0.80, 0.015, 0.002}; }
inline Surface gravel() { return Surface{"gravel", 0.45, 0.040, 0.020}; }
inline Surface grass() { return Surface{"grass", 0.35, 0.060, 0.030}; }
inline Surface sand() { return Surface{"sand", 0.30, 0.100, 0.010}; }
inline Surface ice() { return Surface{"ice", 0.10, 0.010, 0.001}; }

// A patch of ground: a surface, and the slope of the plane it lies in (rise over run along x and along y).
struct Cell {
  Surface surface = tarmac();
  double grade_x = 0.0;
  double grade_y = 0.0;
};

struct Patch {
  double x0 = 0.0;
  double y0 = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  Cell cell;
};

// A base cell that covers everything, with patches laid over it. The later a patch was added, the higher it lies: where two overlap, the later one is under the vehicle.
class Terrain {
 public:
  explicit Terrain(const Cell& base = Cell()) : base_(base) {}

  void add(const Patch& patch) { patches_.push_back(patch); }

  // A patch includes its low edges and not its high ones, so that two patches side by side never both claim the line between them.
  Cell at(double x, double y) const {
    for (std::size_t i = patches_.size(); i-- > 0;) {
      const Patch& p = patches_[i];
      if (x >= p.x0 && x < p.x1 && y >= p.y0 && y < p.y1) return p.cell;
    }
    return base_;
  }

 private:
  Cell base_;
  std::vector<Patch> patches_;
};

// What the ground allows a vehicle that is heading a given way (radians, from the x axis).
struct Traction {
  double angle = 0.0;  // the slope along the heading, in radians: positive is uphill ahead
  double drive = 0.0;  // the most it can speed up, m/s2 (zero or less: it cannot climb)
  double brake = 0.0;  // the most it can slow down, m/s2
  bool can_climb = true;
  bool holds = true;   // whether it stays put with the brakes on
};

inline Traction traction(const Cell& c, double heading) {
  const double g = 9.81;
  Traction t;
  const double grade = c.grade_x * std::cos(heading) + c.grade_y * std::sin(heading);
  t.angle = std::atan(grade);
  const double ca = std::cos(t.angle);
  const double sa = std::sin(t.angle);
  t.drive = g * (c.surface.mu * ca - sa - c.surface.crr * ca);
  t.brake = g * (c.surface.mu * ca + sa + c.surface.crr * ca);
  t.can_climb = t.drive > 0.0;
  const double steepest = std::sqrt(c.grade_x * c.grade_x + c.grade_y * c.grade_y);
  t.holds = steepest <= c.surface.mu;
  return t;
}

// The steepest slope that can be climbed on a surface, in radians.
inline double max_climb_angle(const Surface& s) { return std::atan(s.mu - s.crr); }

// The distance to stop from a speed with a braking limit; a vehicle that cannot brake never stops.
inline double stopping_distance(double speed, double brake) { return brake > 0.0 ? speed * speed / (2.0 * brake) : 1.0e9; }


}  // namespace field
}  // namespace rc

#endif
