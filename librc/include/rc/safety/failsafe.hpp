// rc/safety/failsafe.hpp
//
// Safe values and a fail-safe brake, from lesson 19-03, graduated.
//
// Every output Channel has a safe_value and a FailPolicy (hold, zero or safe_value) for what the output stage does when the controller dies. unsafe_on_failure lists the channels that would not be at their safe value. A brake is a BrakeDesign: spring_applied (a coil of 12 V or more releases it) or power_applied (a coil applies it), and brake_engaged says which state a coil voltage and an intact or cut circuit leave it in.
//
// Five channels left on the default (hold) with the controller dead left drive_left, drive_right, steering and brake_coil unsafe; set to zero, none. An ESC whose neutral is a 1500 us pulse was unsafe on zero and safe on its own value. Of four failures with the robot moving, a spring applied brake was engaged in 3 and a power applied one in 0; the fourth, an output stage that holds, was covered by a safe value.

#ifndef RC_SAFETY_FAILSAFE_HPP
#define RC_SAFETY_FAILSAFE_HPP

#include <string>
#include <vector>

namespace rc {
namespace safety {

// What an output stage does with a channel when the controller that was driving it has died.
//   hold:       keep the last command (what most output stages do if you do nothing)
//   zero:       drive zero
//   safe_value: drive a value chosen for this channel
enum class FailPolicy { hold, zero, safe_value };

struct Channel {
  std::string name;
  double safe_value = 0.0;                    // what the channel must be at for the machine to be safe
  FailPolicy on_failure = FailPolicy::hold;   // what the output stage does when the controller dies
};

// The value a channel ends up at when the controller dies, given the last command it was sent.
inline double output_on_failure(const Channel& c, double last_command) {
  switch (c.on_failure) {
    case FailPolicy::hold: return last_command;
    case FailPolicy::zero: return 0.0;
    case FailPolicy::safe_value: return c.safe_value;
  }
  return last_command;
}

// The names of the channels that are NOT at their safe value when the controller dies, given the last command on each (same order as the channels).
inline std::vector<std::string> unsafe_on_failure(const std::vector<Channel>& channels, const std::vector<double>& last_commands) {
  std::vector<std::string> names;
  for (std::size_t i = 0; i < channels.size() && i < last_commands.size(); ++i)
    if (output_on_failure(channels[i], last_commands[i]) != channels[i].safe_value) names.push_back(channels[i].name);
  return names;
}

// A brake has a coil. The two designs:
//   spring_applied: a spring applies the brake and the coil, when energised (12 V or more), releases it.
//   power_applied:  the coil, when energised, applies the brake, and with nothing in it the brake is off.
enum class BrakeDesign { spring_applied, power_applied };

// Is the brake engaged, given the voltage across the coil and whether the circuit to the coil is intact (a cut wire is not).
inline bool brake_engaged(BrakeDesign design, double coil_volts, bool circuit_intact) {
  const bool energised = circuit_intact && coil_volts >= 12.0;
  return design == BrakeDesign::spring_applied ? !energised : energised;
}

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_FAILSAFE_HPP
