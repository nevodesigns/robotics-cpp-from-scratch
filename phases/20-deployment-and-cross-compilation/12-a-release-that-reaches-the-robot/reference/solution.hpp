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
  Version v;
  int parts[3] = {0, 0, 0};
  int n = 0;
  std::size_t start = 0;
  while (true) {
    const std::size_t dot = s.find('.', start);
    const std::string part = s.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
    if (n >= 3 || part.empty() || part.size() > 9) return v;
    int value = 0;
    for (char c : part) {
      if (c < '0' || c > '9') return v;
      value = value * 10 + (c - '0');
    }
    parts[n++] = value;
    if (dot == std::string::npos) break;
    start = dot + 1;
  }
  if (n != 3) return v;
  v.valid = true;
  v.major = parts[0];
  v.minor = parts[1];
  v.patch = parts[2];
  return v;
}

// -1, 0 or 1 as a is older than, the same as or newer than b: by major, then minor, then patch.
inline int compare(const Version& a, const Version& b) {
  if (a.major != b.major) return a.major < b.major ? -1 : 1;
  if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
  if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
  return 0;
}

enum class VersionDecision { install, refuse_invalid, refuse_below_minimum, refuse_same, refuse_older };

// Should the robot take the offered release? Checked in this order: any of the three versions not valid is refuse_invalid; an offered version below the minimum is refuse_below_minimum (a floor that nobody can override, since the releases below it have a fault that must not come back);
// the same as the installed is refuse_same (nothing to do); older than the installed is refuse_older unless a rollback has been authorised, when it is install; newer is install.
inline VersionDecision decide(const std::string& installed, const std::string& offered, const std::string& minimum, bool rollback_authorised) {
  const Version i = parse_version(installed);
  const Version o = parse_version(offered);
  const Version m = parse_version(minimum);
  if (!i.valid || !o.valid || !m.valid) return VersionDecision::refuse_invalid;
  if (compare(o, m) < 0) return VersionDecision::refuse_below_minimum;
  const int c = compare(o, i);
  if (c == 0) return VersionDecision::refuse_same;
  if (c < 0 && !rollback_authorised) return VersionDecision::refuse_older;
  return VersionDecision::install;
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
  Outcome out;
  out.running_version = robot.installed_version;
  auto fail = [&out](const std::string& stage, const std::string& detail) {
    out.stages.push_back({stage, false, detail});
    out.failed_stage = stage;
  };
  const rc::deploy::Target rt = rc::deploy::parse_triple(rel.triple);
  const rc::deploy::Target bt = rc::deploy::parse_triple(robot.triple);
  if (!rt.valid || !bt.valid || rt.arch != bt.arch || rt.os != bt.os) {
    fail("target", "built for " + rel.triple + ", robot is " + robot.triple);
    return out;
  }
  out.stages.push_back({"target", true, ""});
  const rc::deploy::ParseResult pr = rc::deploy::parse_manifest(rel.manifest_text);
  if (!pr.ok) {
    fail("manifest", pr.error);
    return out;
  }
  if (pr.manifest.version != rel.version) {
    fail("manifest", "wrong version");
    return out;
  }
  if (!rc::deploy::verify_files(pr.manifest, rel.files).empty()) {
    fail("manifest", "files differ");
    return out;
  }
  out.stages.push_back({"manifest", true, ""});
  if (decide(robot.installed_version, rel.version, robot.minimum_version, rollback_authorised) != VersionDecision::install) {
    fail("version", "refused");
    return out;
  }
  out.stages.push_back({"version", true, ""});
  if (rel.size_mb > rc::deploy::max_release_mb(robot.board, 2, robot.data_mb, robot.log_mb, rc::deploy::Budget())) {
    fail("fit", "too big");
    return out;
  }
  out.stages.push_back({"fit", true, ""});
  const rc::deploy::Boot before = rc::deploy::boot(robot.disk);
  const std::vector<rc::deploy::WriteStep> steps = rc::deploy::plan_ab(robot.disk, rel.image_chunks, rel.version);
  const bool cut = cut_after >= 0 && static_cast<std::size_t>(cut_after) < steps.size();
  const std::size_t run = cut ? static_cast<std::size_t>(cut_after) : steps.size();
  for (std::size_t i = 0; i < run; ++i) rc::deploy::apply(robot.disk, steps[i]);
  if (cut) {
    fail("install", "power lost after " + std::to_string(cut_after) + " steps");
    const rc::deploy::Boot now = rc::deploy::boot(robot.disk);
    out.bricked = !now.ok;
    out.running_version = now.ok ? now.version : "";
    return out;
  }
  out.stages.push_back({"install", true, ""});
  const rc::deploy::Boot after = rc::deploy::boot(robot.disk);
  if (!before.ok) {
    robot.installed_version = rel.version;
    out.stages.push_back({"trial", true, "no previous release"});
    out.running_version = rel.version;
    out.confirmed = true;
    out.up = true;
    return out;
  }
  rc::deploy::BootState s;
  s.active = before.slot;
  rc::deploy::start_trial(s, after.slot, 3);
  rc::deploy::Behaviour b[2];
  b[before.slot].healthy_at = 10.0;
  b[after.slot] = rel.behaviour;
  const rc::deploy::RunResult rr = rc::deploy::simulate(s, b, 30.0, 60.0, false, 20);
  if (rr.rolled_back) {
    robot.disk.boot_pointer = before.slot;
    fail("trial", "rolled back");
    out.rolled_back = true;
    out.running_version = before.version;
    out.up = rr.up;
    return out;
  }
  robot.installed_version = rel.version;
  out.stages.push_back({"trial", true, ""});
  out.running_version = rel.version;
  out.confirmed = !rr.on_trial;
  out.up = rr.up;
  return out;
}

#endif  // LESSON_SOLUTION_HPP
