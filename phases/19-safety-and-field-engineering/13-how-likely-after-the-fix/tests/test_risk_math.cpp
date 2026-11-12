#include <rc/test/rc_test.hpp>
#include <rc/safety/hazards.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

const double kNaN = std::numeric_limits<double>::quiet_NaN();

}  // namespace

RC_TEST("and multiplies, or is one minus none, nothing known is certain, and the empty cases are the safe ones") {
  RC_CHECK_NEAR(p_and({0.1, 0.2}), 0.02, 1e-12);
  RC_CHECK_NEAR(p_or({0.1, 0.2}), 0.28, 1e-12);
  RC_CHECK_NEAR(p_and({0.5, 0.5, 0.5}), 0.125, 1e-12);
  RC_CHECK_NEAR(p_or({0.5, 0.5, 0.5}), 0.875, 1e-12);
  RC_CHECK(p_and({}) == 1.0);
  RC_CHECK(p_or({}) == 0.0);
  RC_CHECK(clamp_probability(kNaN) == 1.0);
  RC_CHECK(clamp_probability(-0.3) == 0.0);
  RC_CHECK(clamp_probability(1.7) == 1.0);
  RC_CHECK(p_and({0.001, kNaN}) == 0.001);        // a protection whose chance of failing is unknown is counted as failed
  RC_CHECK(p_or({0.001, kNaN}) == 1.0);
  std::cout << "  and of 0.1 and 0.2: 0.02; or: 0.28; a probability that is not a number counts as 1\n\n";
}

RC_TEST("the average chance that a tested function is dead is about half of rate times interval, and the approximation is only good while that is small") {
  const double rate = 1e-5;
  std::cout << "  a dangerous undetected failure rate of 1e-5 per hour\n  test every   exact        rate T / 2\n";
  const struct { const char* what; double hours; } rows[] = {{"1 day", 24.0}, {"1 week", 168.0}, {"1 month", 730.0}, {"1 year", 8766.0}, {"10 years", 87660.0}};
  double previous = 0.0;
  for (const auto& r : rows) {
    const double exact = pfd_avg(rate, r.hours);
    const double approx = rate * r.hours / 2.0;
    std::cout << "  " << r.what << std::string(11 - std::string(r.what).size(), ' ') << std::scientific << std::setprecision(3) << exact << "    " << approx << "\n";
    RC_CHECK(exact > previous);
    RC_CHECK(exact <= approx + 1e-15);
    previous = exact;
  }
  std::cout << "\n";
  RC_CHECK_NEAR(pfd_avg(rate, 24.0) / (rate * 24.0 / 2.0), 1.0, 1e-3);
  RC_CHECK(pfd_avg(rate, 87660.0) < 0.9 * (rate * 87660.0 / 2.0));
  RC_CHECK(pfd_avg(0.0, 100.0) == 0.0);
  RC_CHECK(pfd_avg(rate, 0.0) == 0.0);
  RC_CHECK(pfd_avg(rate, 1e12) < 1.0);
}

RC_TEST("two channels are better than one, and the part of their failures that they share sets the limit") {
  const double single = pfd_avg(1e-5, 730.0);
  std::cout << "  one channel tested monthly: " << std::scientific << std::setprecision(2) << single << "\n  common cause share beta   two channels     better than one by\n";
  const double betas[] = {0.0, 0.01, 0.05, 0.1, 0.2};
  double previous = 0.0;
  double at0 = 0.0;
  double at10 = 0.0;
  for (double b : betas) {
    const double p = pfd_1oo2(single, b);
    std::cout << "  " << std::fixed << std::setprecision(2) << b << std::string(21, ' ') << std::scientific << std::setprecision(2) << p << "     " << std::fixed << std::setprecision(0) << single / p << " times\n";
    RC_CHECK(p > previous);
    previous = p;
    if (b == 0.0) at0 = p;
    if (b == 0.1) at10 = p;
  }
  std::cout << "\n";
  RC_CHECK(at0 < single * single * 1.0001);
  RC_CHECK(at10 > 0.9 * 0.1 * single);                 // the common part is nearly all of it
  RC_CHECK(single / at10 < 11.0);
  RC_CHECK(single / at0 > 200.0);
  RC_CHECK_NEAR(pfd_1oo2(single, 1.0), single, 1e-15);   // all common cause: no gain at all
  RC_CHECK_NEAR(pfd_1oo2(single, 7.0), single, 1e-15);
  RC_CHECK_NEAR(pfd_1oo2(single, -2.0), single * single, 1e-15);
}

RC_TEST("a hazard's likelihood after each fix, from the demands on a function and its chance of being dead, and a catastrophe stays at medium whatever") {
  const double demands = 20.0;                         // link losses a year
  struct Step { const char* what; double pfd; };
  const double monthly = pfd_avg(1e-5, 730.0);
  const Step steps[] = {
      {"software stop, tested yearly", pfd_avg(1e-5, 8766.0)},
      {"software stop, tested monthly", monthly},
      {"plus a hardware cut-off, beta 0.10", pfd_1oo2(monthly, 0.10)},
      {"plus a hardware cut-off, beta 0.02", pfd_1oo2(monthly, 0.02)},
      {"plus a hardware cut-off, beta 0.00", pfd_1oo2(monthly, 0.0)},
  };
  std::cout << "  G2, keeps driving when the link is lost: 20 link losses a year, severity 4\n";
  std::cout << "  design                                  pfd         per year     likelihood  score  class\n";
  int likelihood[5];
  rc::safety::RiskClass classes[5];
  for (int i = 0; i < 5; ++i) {
    const double f = hazard_frequency(demands, steps[i].pfd);
    likelihood[i] = likelihood_from_frequency(f);
    classes[i] = rc::safety::risk_class(4, likelihood[i]);
    std::cout << "  " << steps[i].what << std::string(40 - std::string(steps[i].what).size(), ' ') << std::scientific << std::setprecision(2) << steps[i].pfd << "    " << f << "     " << likelihood[i] << "           "
              << std::setw(2) << rc::safety::risk_score(4, likelihood[i]) << "     " << rc::safety::describe(classes[i]) << "\n";
  }
  std::cout << "\n";
  RC_CHECK(likelihood[0] == 4);
  RC_CHECK(classes[0] == rc::safety::RiskClass::unacceptable);
  RC_CHECK(likelihood[1] == 3);
  RC_CHECK(classes[1] == rc::safety::RiskClass::high);
  RC_CHECK(likelihood[2] == 2);
  RC_CHECK(classes[2] == rc::safety::RiskClass::medium);
  RC_CHECK(likelihood[4] == 1);
  RC_CHECK(rc::safety::risk_score(4, likelihood[4]) == 4);                     // by the product, low
  RC_CHECK(classes[4] == rc::safety::RiskClass::medium);                         // and by the floor of lesson 19-01, still medium
  RC_CHECK(likelihood_from_frequency(kNaN) == 5);
  RC_CHECK(likelihood_from_frequency(1.0) == 5);
  RC_CHECK(likelihood_from_frequency(0.1) == 4);
  RC_CHECK(likelihood_from_frequency(0.0099) == 2);
  RC_CHECK(hazard_frequency(-5.0, 0.5) == 0.0);
}

RC_TEST("a fault tree gives the chance of the top event and names the single points that would cause it alone") {
  // The robot moves after the e-stop loop opens: the contactor is welded, or the software stop fails together with the brake.
  Node tree = any_of("robot moves after the loop opens", {basic("contactor welded", 1e-4), all_of("no software stop and no brake", {basic("software stop fails", 1e-3), basic("brake fails", 1e-2)})});
  const double p = probability(tree);
  std::vector<std::string> sp;
  single_points(tree, sp);
  std::cout << "  contactor welded 1e-4, or (software stop fails 1e-3 and brake fails 1e-2): top event " << std::scientific << std::setprecision(3) << p << " per demand; single points: ";
  for (const std::string& s : sp) std::cout << s;
  std::cout << "\n";
  RC_CHECK_NEAR(p, 1.0 - (1.0 - 1e-4) * (1.0 - 1e-5), 1e-15);
  RC_CHECK(sp.size() == 1);
  if (sp.size() != 1) return;
  RC_CHECK(sp[0] == "contactor welded");
  // A second contactor in series: both have to be welded.
  Node better = any_of("robot moves after the loop opens", {all_of("both contactors welded", {basic("contactor A welded", 1e-4), basic("contactor B welded", 1e-4)}),
                                                             all_of("no software stop and no brake", {basic("software stop fails", 1e-3), basic("brake fails", 1e-2)})});
  std::vector<std::string> sp2;
  single_points(better, sp2);
  std::cout << "  a second contactor: top event " << probability(better) << " per demand, " << sp2.size() << " single points\n\n";
  RC_CHECK(sp2.empty());
  RC_CHECK(probability(better) < probability(tree) / 5.0);
  Node lone = all_of("a redundancy of one", {basic("only channel", 1e-3)});
  std::vector<std::string> sp3;
  single_points(lone, sp3);
  RC_CHECK(sp3.size() == 1);                              // an and of one child protects nothing
  RC_CHECK_NEAR(probability(all_of("empty", {})), 1.0, 1e-15);
}
