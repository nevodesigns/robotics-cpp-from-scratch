#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>
#include <vector>

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

#endif  // LESSON_SOLUTION_HPP
