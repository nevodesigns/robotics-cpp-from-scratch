#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/drive/degraded.hpp>
#include <rc/drive/field_checklist.hpp>
#include <rc/drive/log_doctor.hpp>
#include <rc/drive/protective_field.hpp>
#include <rc/drive/range.hpp>
#include <rc/drive/unstick.hpp>
#include <rc/drive/vehicle_release.hpp>
#include <rc/drive/waypoint_mission.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace rc::drive;

// The one rule that joins the limits on the speed.
inline double combine_speed(double wanted, double mode_cap, FieldResult field, double creep) {
  double v = wanted;
  v = std::min(wanted, mode_cap);
  v = protective_speed(field, v, creep);
  return v;
}

struct DayConfig {
  double soc_wh = 2.0;
  double person_s = 40.0, person_t0 = 20.0, person_t1 = 40.0;
  double snag_s = 150.0;
  double gnss_t0 = 110.0, gnss_t1 = 210.0;
  double scanner_t0 = 300.0, scanner_t1 = 303.0;
  double release_t = 250.0;
  double dt = 0.1;
  double t_max = 1500.0;
};

struct DayResult {
  bool refused_at_dock = false;
  std::vector<std::string> blocked_by;
  bool completed = false;
  int legs_done = 0;
  double time = 0.0;
  double distance = 0.0;
  int contacts = 0;
  double field_stop_s = 0.0;
  int attempts = 0;
  bool stranded = false;
  double stranded_s = 0.0;
  double mode_s[4] = {0.0, 0.0, 0.0, 0.0};
  double used_wh = 0.0;
  double start_wh = 0.0;
  double end_wh = 0.0;
  bool release_refused_midday = false;
  bool release_accepted_at_dock = false;
  std::vector<Finding> findings;
  double final_s = 0.0;
};

inline DayResult run_day(const DayConfig& cfg, bool layers) {
  DayResult r;
  r.start_wh = cfg.soc_wh;
  // The road: 270 m of tarmac in 3 m cells, with sand from 100 to 130 m.
  rc::perceive::GridSpec spec;
  spec.origin_x = 0.0;
  spec.origin_y = 0.0;
  spec.resolution = 3.0;
  spec.width = 90;
  spec.height = 3;
  TerrainGrid terrain(spec);
  rc::field::Cell sand;
  sand.surface = rc::field::sand();
  terrain.paint(100.0, 0.0, 130.0, 9.0, sand);
  DrivePower power;
  power.speed = 1.2;
  std::vector<Waypoint> legs;
  const double sites[3] = {80.0, 160.0, 240.0};
  for (int k = 0; k < 3; ++k) {
    Waypoint w;
    w.x = sites[k];
    w.y = 0.0;
    w.tolerance = 1.0;
    w.dwell = 20.0;
    w.timeout = 400.0;
    w.speed = 1.5;
    legs.push_back(w);
  }
  Waypoint home;
  home.x = 0.0;
  home.tolerance = 1.0;
  home.timeout = 400.0;
  home.speed = 1.5;
  legs.push_back(home);
  WaypointMission mission(legs, kSkipOnTimeout);

  if (layers) {
    std::vector<CarPose> route;
    for (double x = 0.0; x <= 240.0; x += 3.0) {
      CarPose p;
      p.x = x;
      p.y = 1.5;
      route.push_back(p);
    }
    for (double x = 240.0; x >= 0.0; x -= 3.0) {
      CarPose p;
      p.x = x;
      p.y = 1.5;
      route.push_back(p);
    }
    VehicleState st;
    st.soc_wh = cfg.soc_wh;
    st.mission_wh = route_wh(terrain, route, power) + 3.0 * 20.0 * power.idle_w / 3600.0;
    st.reserve_wh = 0.1 * cfg.soc_wh;
    st.estop_tested_h = 3.0;
    st.position_sigma_m = 0.05;
    st.scan_age_s = 0.05;
    st.link_rtt_s = 0.1;
    st.geofence_loaded = true;
    st.log_free_mb = 4000.0;
    st.clock_error_s = 0.01;
    st.release = 7;
    st.release_expected = 7;
    st.motor_temp_c = 35.0;
    const std::vector<CheckResult> checks = run_checks(st, CheckLimits());
    if (!can_go(checks)) {
      r.refused_at_dock = true;
      for (std::size_t i = 0; i < checks.size(); ++i)
        if (!checks[i].ok && checks[i].severity == kBlock) r.blocked_by.push_back(checks[i].name);
      return r;
    }
  }

  const FieldConfig field;
  const ModePolicy mp;
  ModeManager manager(mp);
  Unstick unstick{UnstickConfig()};
  std::vector<LogRow> rows;
  double s = 0.0, v = 0.0, soc = cfg.soc_wh, dist_since_fix = 0.0, t = 0.0;
  bool snag_hit = false, released = false, person_hit = false, release_offered = false;
  int most_attempts = 0;
  for (t = 0.0; t < cfg.t_max; t += cfg.dt) {
    const bool scanner_ok = !(t >= cfg.scanner_t0 && t < cfg.scanner_t1);
    const bool gnss_ok = !(t >= cfg.gnss_t0 && t < cfg.gnss_t1);
    const double sigma = gnss_ok ? 0.05 : std::sqrt(0.05 * 0.05 + 0.0048 * dist_since_fix);
    VehicleHealth h;
    h.scanner_ok = scanner_ok;
    h.gnss_ok = gnss_ok;
    h.sigma_m = sigma;
    const Mode mode = layers ? manager.update(t, required_mode(h, mp)) : kFull;
    const double cap = layers ? mode_speed_cap(mode, mp) : 2.0;
    const MissionStatus ms = mission.step(t, s, 0.0);
    if (ms.done && std::fabs(v) < 0.01) break;
    const double dir = ms.target_x >= s ? 1.0 : -1.0;
    const bool person = t >= cfg.person_t0 && t < cfg.person_t1;
    FieldResult fr = kClear;
    if (layers && person && cfg.person_s - s > 0.0 && dir > 0.0) {
      std::vector<FieldPoint> pts(1);
      pts[0].x = cfg.person_s - s;
      pts[0].y = 0.0;
      fr = evaluate_field(pts, std::min(ms.speed, cap), 0.0, field);  // the field for the speed it is about to use
    }
    const double mag = combine_speed(ms.speed, cap, fr, field.creep);
    double v_cmd = dir * mag;
    if (layers) {
      CarPose pose;
      pose.x = s;
      DriveCmd normal;
      normal.v = v_cmd;
      const DriveCmd c = unstick.step(t, pose, normal);
      v_cmd = c.v;
      most_attempts = std::max(most_attempts, unstick.attempts());
      if (unstick.gave_up()) break;
    }
    const bool speeding_up = std::fabs(v_cmd) > std::fabs(v) && v_cmd * v >= 0.0;
    const double limit = (speeding_up ? 1.0 : 2.0) * cfg.dt;
    v += std::max(-limit, std::min(limit, v_cmd - v));
    double ds = v * cfg.dt;
    bool blocked = false;
    if (!released && s + ds > cfg.snag_s) {
      ds = std::max(0.0, cfg.snag_s - s);
      blocked = true;
      v = 0.0;
    }
    if (s >= cfg.snag_s - 0.01) snag_hit = true;
    if (snag_hit && s <= cfg.snag_s - 0.4) released = true;
    s += ds;
    if (person && std::fabs(s - cfg.person_s) < 0.1 && !person_hit) {
      ++r.contacts;
      person_hit = true;
    }
    const rc::field::Cell& cell = terrain.at(terrain.cell_i(s), terrain.cell_j(1.5));
    const double joules = energy_per_metre(cell, ds >= 0.0 ? 0.0 : 3.14159265358979323846, power.mass) / power.efficiency * std::fabs(ds) + power.idle_w * cfg.dt;
    soc -= joules / 3600.0;
    if (soc <= 0.0 && !r.stranded) {
      r.stranded = true;
      r.stranded_s = s;
    }
    if (!gnss_ok) dist_since_fix += std::fabs(ds);
    r.distance += std::fabs(ds);
    r.mode_s[static_cast<int>(mode)] += cfg.dt;
    if (fr == kStop) r.field_stop_s += cfg.dt;
    LogRow row;
    row.t = t;
    row.x = s;
    row.cmd_v = v_cmd;
    row.odom_v = blocked ? 0.0 : std::fabs(v);
    row.soc_wh = soc;
    rows.push_back(row);
    if (!release_offered && t >= cfg.release_t) {
      release_offered = true;
      OnBoard ob;
      ob.id = "ugv-07";
      ob.vehicle_class = "skid-10kg";
      ob.software = "1.4.2";
      ob.parked = std::fabs(v) < 0.01 && ms.speed == 0.0;
      ob.mission_active = !mission.done();
      ob.soc = soc / cfg.soc_wh;
      Bundle b;
      b.vehicle_class = "skid-10kg";
      b.software = "1.5.0";
      r.release_refused_midday = !check_update(ob, b, UpdatePolicy()).empty();
    }
  }
  r.time = t;
  r.completed = mission.done();
  for (std::size_t i = 0; i < mission.results().size(); ++i)
    if (mission.results()[i] == kReached) ++r.legs_done;
  r.attempts = most_attempts;
  r.end_wh = soc;
  r.used_wh = cfg.soc_wh - soc;
  r.final_s = s;
  {
    OnBoard ob;
    ob.id = "ugv-07";
    ob.vehicle_class = "skid-10kg";
    ob.software = "1.4.2";
    ob.parked = std::fabs(v) < 0.01;
    ob.mission_active = !mission.done();
    ob.soc = 0.6;  // on the charger by now
    Bundle b;
    b.vehicle_class = "skid-10kg";
    b.software = "1.5.0";
    r.release_accepted_at_dock = check_update(ob, b, UpdatePolicy()).empty();
  }
  r.findings = analyse_log(rows, DoctorConfig());
  return r;
}

inline std::string day_report(const DayResult& r) {
  std::ostringstream o;
  o << "legs done " << r.legs_done << " of 4, completed " << (r.completed ? "yes" : "no") << ", " << r.time << " s, " << r.distance << " m\n";
  o << "contacts " << r.contacts << ", stopped by the field " << r.field_stop_s << " s, unsticking attempts " << r.attempts << "\n";
  o << "modes: full " << r.mode_s[0] << " s, reduced " << r.mode_s[1] << " s, creep " << r.mode_s[2] << " s, safe stop " << r.mode_s[3] << " s\n";
  o << "energy used " << r.used_wh << " Wh of " << r.start_wh << "\n";
  o << "release: " << (r.release_refused_midday ? "refused in the middle of the job" : "taken in the middle of the job") << ", "
    << (r.release_accepted_at_dock ? "taken at the dock" : "refused at the dock") << "\n";
  for (std::size_t i = 0; i < r.findings.size(); ++i)
    o << "finding: " << kind_name(r.findings[i].kind) << " from " << r.findings[i].t0 << " to " << r.findings[i].t1 << " s\n";
  return o.str();
}


#endif
