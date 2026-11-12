#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <rc/deploy/bundle.hpp>
#include <rc/deploy/fit.hpp>
#include <rc/deploy/slots.hpp>
#include <rc/deploy/target.hpp>
#include <rc/deploy/trial.hpp>
#include <string>
#include <vector>

// A version as three whole numbers: major.minor.patch.
struct Version {
  bool valid = false;
  int major = 0;
  int minor = 0;
  int patch = 0;
};

// Read "1.4.2": exactly three parts separated by dots, each one to nine digits and nothing else. Anything else is not valid.
inline Version parse_version(const std::string& s) {
  // TODO: exactly three parts split on '.', each one to nine digits only; otherwise an invalid Version (valid false)
  (void)s;
  return Version();
}

// -1, 0 or 1 as a is older than, the same as or newer than b: by major, then minor, then patch.
inline int compare(const Version& a, const Version& b) {
  // TODO: -1, 0 or 1 by major, then minor, then patch
  (void)a;
  (void)b;
  return 0;
}

enum class VersionDecision { install, refuse_invalid, refuse_below_minimum, refuse_same, refuse_older };

// Should the robot take the offered release? Checked in this order: any of the three versions not valid is refuse_invalid; an offered version below the minimum is refuse_below_minimum (a floor that nobody can override, since the releases below it have a fault that must not come back);
// the same as the installed is refuse_same (nothing to do); older than the installed is refuse_older unless a rollback has been authorised, when it is install; newer is install.
inline VersionDecision decide(const std::string& installed, const std::string& offered, const std::string& minimum, bool rollback_authorised) {
  // TODO: the order in the comment above: invalid, below the minimum, same, older (unless authorised), else install
  (void)installed;
  (void)offered;
  (void)minimum;
  (void)rollback_authorised;
  return VersionDecision::refuse_invalid;
}

// A release as it is offered: what it is for, what is in it, how big it is and, for the simulation, how it will behave once it runs.
struct Release {
  std::string version;
  std::string triple;
  std::string manifest_text;
  std::vector<rc::deploy::FileData> files;
  std::vector<std::string> image_chunks;
  double size_mb = 0.0;
  rc::deploy::Behaviour behaviour;
};

// A robot as the pipeline sees it.
struct RobotState {
  rc::deploy::Disk disk;
  std::string installed_version;
  std::string triple;
  rc::deploy::Board board;
  double data_mb = 0.0;
  double log_mb = 0.0;
  std::string minimum_version = "0.0.0";
};

struct StageResult {
  std::string stage;
  bool passed = false;
  std::string detail;
};

struct Outcome {
  std::vector<StageResult> stages;
  std::string failed_stage;          // empty if all passed
  std::string running_version;       // what the robot boots now, empty if nothing boots
  bool bricked = false;
  bool rolled_back = false;
  bool confirmed = false;
  bool up = false;                   // the trial ended with the robot staying up
};

// Take a release to a robot, stage by stage, stopping at the first that fails. Nothing on the robot's disk changes before the "install" stage. The stages, in order, and what passes them:
//  "target"    the release's triple and the robot's both read, and have the same architecture and operating system; otherwise the detail is "built for <release triple>, robot is <robot triple>";
//  "manifest"  the manifest parses, names the same version as the release, and the files match it; the detail is the parse error, "wrong version" or "files differ";
//  "version"   decide(...) says install; the detail is "refused";
//  "fit"       size_mb is not over max_release_mb for the robot's board with two slots; the detail is "too big";
//  "install"   plan_ab from the robot's disk with the release's image and version, with the first `cut_after` steps applied if cut_after is from 0 to one less than the number of steps (the power was lost then), all of them otherwise.
//              A cut fails the stage with the detail "power lost after <k> steps", and ends the pipeline: running_version is what boots (empty and bricked if nothing does);
//  "trial"     the release on trial: the slot that was booted is the previous, three tries, a healthy old release (healthy at 10 s, never crashes), the new one behaving as the release says, a soak of 30 s and a deadline of 60 s, at most 20 boots.
//              If it was rolled back the boot pointer is put back on the previous slot, the stage fails with "rolled back" and running_version is the old one; otherwise robot.installed_version becomes the release's, and confirmed and up are set from the run.
inline Outcome deploy(RobotState& robot, const Release& rel, int cut_after, bool rollback_authorised) {
  // TODO: the six stages in the comment above, in that order, stopping at the first that fails. Use rc::deploy::parse_triple, parse_manifest, verify_files, max_release_mb (two slots, rc::deploy::Budget()), boot, plan_ab, apply, start_trial and simulate. Record each passed stage in out.stages and set failed_stage on a failure
  (void)robot;
  (void)rel;
  (void)cut_after;
  (void)rollback_authorised;
  return Outcome();
}

#endif  // LESSON_SOLUTION_HPP
