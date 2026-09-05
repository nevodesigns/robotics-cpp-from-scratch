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

  // The force that would hold this motion exactly, with no error at all.
  //
  // Three terms and each is a different piece of physics: the load is there
  // whatever happens, the damping grows with speed, and the mass only matters
  // while the speed is changing.
  double force_for(double velocity, double acceleration) const {
    return load + damping * velocity + mass * acceleration;
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

// The steady error a loop is left with while following a ramp, with nothing fed
// forward.
//
// Two separate causes, and the second is the one people do not expect.
//
// The plant needs force to move at all, and a proportional term can only produce
// force from an error, so it holds an error of (load + damping * v) / kp for
// ever.
//
// And a derivative taken on the measurement, which is the right choice for a
// step, opposes any steady motion: at constant velocity it contributes -kd * v,
// which the proportional term must also overcome from an error. That is another
// (kd * v) / kp, and it is usually the larger of the two.
//
// Measured against a simulation, at v = 0.5 with kp = 20 and kd = 8: predicted
// 0.2350, simulated 0.2340, of which 0.2000 is the derivative term.
inline double ramp_error(const PlantModel& model, double kp, double kd, double velocity) {
  if (kp == 0.0) return 0.0;
  return (model.load + model.damping * velocity + kd * velocity) / kp;
}

// Feedback plus feedforward, limited once.
//
// Limited once, at the end, and not twice: clamping the feedback and then adding
// to it gives a total that can exceed the limit, and clamping both separately
// throws away authority the actuator has.
inline double command(double feedback, const PlantModel& model, const Setpoint& target,
                      double lowest, double highest) {
  const double total = feedback + model.force_for(target.velocity, target.acceleration);
  if (total < lowest) return lowest;
  if (total > highest) return highest;
  return total;
}

#endif  // LESSON_SOLUTION_HPP
