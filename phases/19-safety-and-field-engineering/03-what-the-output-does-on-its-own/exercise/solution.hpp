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
  // TODO: hold keeps last_command, zero gives 0, safe_value gives the channel's safe_value
  (void)c;
  return last_command;
}

// The names of the channels that are NOT at their safe value when the controller dies, given the last command on each (same order as the channels).
inline std::vector<std::string> unsafe_on_failure(const std::vector<Channel>& channels, const std::vector<double>& last_commands) {
  // TODO: for each channel (and its last command, same index; stop at the shorter list), if output_on_failure differs from its safe_value add its name
  (void)channels;
  (void)last_commands;
  return std::vector<std::string>();
}

// A brake has a coil. The two designs:
//   spring_applied: a spring applies the brake and the coil, when energised (12 V or more), releases it.
//   power_applied:  the coil, when energised, applies the brake, and with nothing in it the brake is off.
enum class BrakeDesign { spring_applied, power_applied };

// Is the brake engaged, given the voltage across the coil and whether the circuit to the coil is intact (a cut wire is not).
inline bool brake_engaged(BrakeDesign design, double coil_volts, bool circuit_intact) {
  // TODO: energised means the circuit is intact and the coil is at 12 V or more. A spring applied brake is engaged when NOT energised, a power applied brake when energised
  (void)design;
  (void)coil_volts;
  (void)circuit_intact;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
