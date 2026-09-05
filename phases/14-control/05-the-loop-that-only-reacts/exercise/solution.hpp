#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// What a plant must be told to move as asked, before any error is measured.
//
// A feedback loop is a machine for reacting to being wrong. Following a moving
// target it has to be wrong, continuously, because the error is the only thing
// producing the command. Feedforward is the part of the command that comes from
// knowing what the target is doing rather than from the error.
struct PlantModel {
  double mass = 1.0;
  double damping = 0.0;
  double load = 0.0;

  // TODO 1: the force that would hold this motion exactly, with no error.
  //
  // Three terms, each a different piece of physics:
  //
  //   the load, which is there whatever happens;
  //   the damping, which grows with speed;
  //   the mass, which only matters while the speed is changing.
  //
  // Two lines of arithmetic, and it is the whole of feedforward.
  double force_for(double velocity, double acceleration) const {
    (void)velocity;
    (void)acceleration;
    return 0.0;
  }
};

// Where the target is, how fast it is going, and how hard it is accelerating.
//
// A setpoint that is only a position throws away the two things feedforward
// needs. Anything generating a trajectory already knows all three.
struct Setpoint {
  double position = 0.0;
  double velocity = 0.0;
  double acceleration = 0.0;
};

// TODO 2: the steady error a loop is left with while following a ramp, with
// nothing fed forward.
//
// Two separate causes, and the second is the one people do not expect.
//
// The plant needs force to move at all, and a proportional term can only
// produce force from an error, so it holds an error of
// (load + damping * v) / kp for ever.
//
// And a derivative taken on the measurement, which is the right choice for a
// step, opposes any steady motion: at constant velocity it contributes -kd * v,
// which the proportional term must also overcome from an error. That is another
// (kd * v) / kp, and it is usually the larger of the two.
//
// So the answer is the sum of the three forces, divided by kp. Return 0 when kp
// is zero rather than dividing by it.
//
// The test compares this prediction against a simulation, which is the point of
// writing it: at v = 0.5 with kp = 20 and kd = 8 it predicts 0.2350 and the
// simulation gives 0.2340.
inline double ramp_error(const PlantModel& model, double kp, double kd, double velocity) {
  (void)model;
  (void)kp;
  (void)kd;
  (void)velocity;
  return 0.0;
}

// TODO 3: feedback plus feedforward, limited once.
//
// Add the two, then clamp the total to [lowest, highest].
//
// Once, at the end, and not twice. Clamping the feedback and then adding to it
// gives a total that can exceed the limit; clamping both separately throws away
// authority the actuator has. And a feedforward that reaches the limit on its
// own is worth knowing about: it means the profile is asking for more than the
// machine has, before anything has gone wrong at all.
inline double command(double feedback, const PlantModel& model, const Setpoint& target,
                      double lowest, double highest) {
  (void)model;
  (void)target;
  (void)lowest;
  (void)highest;
  return feedback;
}

#endif  // LESSON_SOLUTION_HPP
