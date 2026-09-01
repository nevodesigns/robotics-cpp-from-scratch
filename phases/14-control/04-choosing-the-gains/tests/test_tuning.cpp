#include <rc/test/rc_test.hpp>

#include <rc/control/pid.hpp>
#include <rc/plot/series.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kTarget = 1.0;
constexpr double kDt = 0.002;
constexpr int kSteps = 4000;      // eight seconds
constexpr double kBand = 0.02;    // settled means within two percent

rc::span<const double> view(const std::vector<double>& v) {
  return rc::span<const double>(v.data(), v.size());
}

std::vector<double> run(double kp, double ki, double kd, double load) {
  rc::control::Pid pid(kp, ki, kd, -20.0, 20.0);
  Mass plant(1.0, 0.6, load);

  std::vector<double> output;
  output.reserve(static_cast<std::size_t>(kSteps));
  for (int i = 0; i < kSteps; ++i)
    output.push_back(plant.step(pid.update(kTarget, plant.position(), kDt), kDt));
  return output;
}

std::string chart(const std::vector<double>& output, int columns, int rows) {
  rc::plot::Series series(4000, static_cast<double>(kSteps) * kDt);
  for (std::size_t i = 0; i < output.size(); ++i)
    series.add(static_cast<double>(i) * kDt, output[i]);

  const rc::plot::Range range =
      rc::plot::at_least(rc::plot::padded(rc::plot::range_of(series), 0.1), 0.5);

  std::vector<std::string> grid(static_cast<std::size_t>(rows),
                                std::string(static_cast<std::size_t>(columns), ' '));
  for (std::size_t i = 0; i < series.size(); ++i) {
    const rc::plot::Point point =
        rc::plot::place_sample(series.at(i), series.newest_time(), series.window(), range,
                               static_cast<double>(columns), static_cast<double>(rows), 0.0);
    const int column = static_cast<int>(point.across);
    const int row = static_cast<int>(point.down);
    if (column < 0 || column >= columns || row < 0 || row >= rows) continue;
    grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = '.';
  }

  // Where the target sits, so overshoot is visible rather than inferred.
  //
  // Scaled by rows and not by rows minus one, because that is what place_sample
  // does, and a marker computed the other way sits one line off the data it is
  // supposed to be marking.
  const double up = (kTarget - range.low) / rc::plot::span(range);
  const int target_row = static_cast<int>((1.0 - up) * static_cast<double>(rows));

  std::ostringstream out;
  for (int r = 0; r < rows; ++r) {
    out << "    " << (r == target_row ? '-' : ' ') << "|"
        << grid[static_cast<std::size_t>(r)] << "\n";
  }
  return out.str();
}

std::string seconds_or_never(double value) {
  if (value < 0.0) return "never";
  std::ostringstream out;
  out << std::fixed << std::setprecision(2) << value;
  return out.str();
}

struct Gains {
  const char* name;
  double kp, ki, kd;
};

const Gains kSweep[] = {
    {"P 2",       2.0, 0.0, 0.0},
    {"P 8",       8.0, 0.0, 0.0},
    {"P 20",     20.0, 0.0, 0.0},
    {"PD 20/3",  20.0, 0.0, 3.0},
    {"PD 20/8",  20.0, 0.0, 8.0},
    {"PID",      20.0, 6.0, 8.0},
};

void report(double load) {
  std::cout << "\n  " << (load == 0.0 ? "no load on the joint"
                                      : "a steady load of five newtons, such as gravity")
            << "\n\n    " << std::left << std::setw(10) << "gains"
            << std::right << std::setw(9) << "rise s" << std::setw(11) << "overshoot"
            << std::setw(10) << "settle s" << std::setw(12) << "final error" << "\n";

  for (const Gains& gains : kSweep) {
    const std::vector<double> output = run(gains.kp, gains.ki, gains.kd, load);
    const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
    std::cout << "    " << std::left << std::setw(10) << gains.name << std::right
              << std::setw(9) << seconds_or_never(response.rise_time)
              << std::fixed << std::setprecision(1)
              << std::setw(10) << response.overshoot * 100.0 << "%"
              << std::setw(10) << seconds_or_never(response.settling_time)
              << std::setw(12) << std::setprecision(4) << response.final_error << "\n";
  }
}

}  // namespace

RC_TEST("a response that never moves reports nothing rather than something wrong") {
  const std::vector<double> flat(100, 0.0);
  const StepResponse response = analyse(view(flat), kTarget, kDt, kBand);

  RC_CHECK(response.rise_time < 0.0);        // ninety percent never reached
  RC_CHECK_NEAR(response.overshoot, 0.0, 1e-12);
  RC_CHECK(response.settling_time < 0.0);    // and it never settled
  RC_CHECK_NEAR(response.final_error, kTarget, 1e-12);
}

RC_TEST("an empty recording says nothing at all") {
  const std::vector<double> nothing;
  const StepResponse response = analyse(view(nothing), kTarget, kDt, kBand);
  RC_CHECK(response.rise_time < 0.0);
  RC_CHECK(response.settling_time < 0.0);
}

RC_TEST("overshoot is measured from the target, not from zero") {
  // The check that catches a peak divided by the target. A peak of 1.8 against
  // a target of 1.0 is eighty percent past it, not a hundred and eighty.
  std::vector<double> output;
  for (int i = 0; i < 50; ++i) output.push_back(1.8);
  for (int i = 0; i < 50; ++i) output.push_back(1.0);

  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_CHECK_NEAR(response.overshoot, 0.8, 1e-9);
}

RC_TEST("a response that stays under the target has no overshoot, not a negative one") {
  std::vector<double> output(100, 0.6);
  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_CHECK_NEAR(response.overshoot, 0.0, 1e-12);
}

RC_TEST("settling is the last time it left the band, not the first time it entered") {
  // The check that matters most. A response oscillating through the target
  // passes through the band on the way past, and reporting that as settled
  // says a loop that is still ringing has already finished.
  std::vector<double> output;
  for (int i = 0; i < 10; ++i) output.push_back(1.0);    // inside the band early
  for (int i = 0; i < 10; ++i) output.push_back(2.0);    // and back out again
  for (int i = 0; i < 80; ++i) output.push_back(1.0);    // before finally settling

  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_REQUIRE(response.settling_time > 0.0);
  RC_CHECK_NEAR(response.settling_time, 20.0 * kDt, 1e-9);
}

RC_TEST("a response still outside the band at the end never settled") {
  std::vector<double> output(100, 1.0);
  output.back() = 5.0;

  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_CHECK(response.settling_time < 0.0);
}

RC_TEST("a response that is inside the band throughout settled immediately") {
  const std::vector<double> output(100, 1.0);
  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_CHECK_NEAR(response.settling_time, 0.0, 1e-12);
}

RC_TEST("rise time is measured between ten and ninety percent") {
  std::vector<double> output;
  for (int i = 0; i < 100; ++i) output.push_back(static_cast<double>(i) / 99.0);

  const StepResponse response = analyse(view(output), kTarget, kDt, kBand);
  RC_REQUIRE(response.rise_time > 0.0);
  // Eighty of the ninety nine steps separate the two thresholds.
  RC_CHECK_NEAR(response.rise_time, 80.0 * kDt, 2.0 * kDt);
}

// ---------------------------------------------------------------------------
// What the gains actually cost, measured on a real loop.
// ---------------------------------------------------------------------------

RC_TEST("more proportional gain buys a faster rise and pays in overshoot") {
  const StepResponse gentle = analyse(view(run(2.0, 0.0, 0.0, 0.0)), kTarget, kDt, kBand);
  const StepResponse firm = analyse(view(run(20.0, 0.0, 0.0, 0.0)), kTarget, kDt, kBand);

  RC_REQUIRE(gentle.rise_time > 0.0);
  RC_REQUIRE(firm.rise_time > 0.0);

  RC_CHECK(firm.rise_time < gentle.rise_time);       // faster
  RC_CHECK(firm.overshoot > gentle.overshoot);       // and worse
  RC_CHECK(firm.overshoot > 0.5);                    // considerably worse
}

RC_TEST("derivative gain removes the overshoot and slows the rise") {
  const StepResponse without = analyse(view(run(20.0, 0.0, 0.0, 0.0)), kTarget, kDt, kBand);
  const StepResponse with = analyse(view(run(20.0, 0.0, 8.0, 0.0)), kTarget, kDt, kBand);

  RC_REQUIRE(with.rise_time > 0.0);
  RC_CHECK(with.overshoot < 0.05);          // essentially gone
  RC_CHECK(with.rise_time > without.rise_time);   // and it cost some speed
  RC_CHECK(with.settling_time > 0.0);       // but it does settle, which P alone did not
  RC_CHECK(without.settling_time < 0.0);
}

RC_TEST("without a load, adding integral gain only makes things worse") {
  // The measurement most tutorials leave out. There is no steady offset to
  // remove here, so the integral term has nothing to do but wind up on the way
  // to the target and carry the output past it.
  const StepResponse pd = analyse(view(run(20.0, 0.0, 8.0, 0.0)), kTarget, kDt, kBand);
  const StepResponse pid = analyse(view(run(20.0, 6.0, 8.0, 0.0)), kTarget, kDt, kBand);

  RC_REQUIRE(pd.settling_time > 0.0);
  RC_REQUIRE(pid.settling_time > 0.0);

  RC_CHECK(pid.overshoot > pd.overshoot);
  RC_CHECK(pid.settling_time > pd.settling_time);
  RC_CHECK(std::fabs(pd.final_error) < 0.01);   // and there was nothing to fix
}

RC_TEST("with a load, no amount of proportional or derivative gain arrives") {
  // A steady load needs a steady force, and a proportional term only produces
  // force while there is error, so it stops short by exactly the error that
  // produces the force the load needs.
  const StepResponse pd = analyse(view(run(20.0, 0.0, 8.0, 5.0)), kTarget, kDt, kBand);

  RC_CHECK(std::fabs(pd.final_error) > 0.2);
  RC_CHECK_NEAR(pd.final_error, 5.0 / 20.0, 0.02);   // the load over the gain
}

RC_TEST("with a load, integral gain is what arrives") {
  const StepResponse pd = analyse(view(run(20.0, 0.0, 8.0, 5.0)), kTarget, kDt, kBand);
  const StepResponse pid = analyse(view(run(20.0, 6.0, 8.0, 5.0)), kTarget, kDt, kBand);

  RC_CHECK(std::fabs(pid.final_error) < 0.02);
  RC_CHECK(std::fabs(pid.final_error) < std::fabs(pd.final_error) / 10.0);
}

RC_TEST("the whole sweep, printed") {
  report(0.0);
  report(5.0);

  std::cout << "\n  P 20 with no load: fast, and it never stops ringing\n\n"
            << chart(run(20.0, 0.0, 0.0, 0.0), 62, 9)
            << "\n  PD 20/8 with no load: the same speed, and it arrives\n\n"
            << chart(run(20.0, 0.0, 8.0, 0.0), 62, 9)
            << "\n  PD 20/8 against a load: steady, and permanently short\n\n"
            << chart(run(20.0, 0.0, 8.0, 5.0), 62, 9)
            << "\n  PID against the same load: it gets there\n\n"
            << chart(run(20.0, 6.0, 8.0, 5.0), 62, 9) << "\n";

  RC_CHECK(true);
}
