#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"hidden", 16.0},
    {"epochs", 20.0},
    {"lr_small", 0.001},
    {"lr_good", 0.02},
    {"lr_big", 1.0},
    {"test_small", 0.002431216724903075},
    {"test_good", 0.00130027342615121},
    {"n_small", 30.0},
    {"long_epochs", 3000.0},
    {"tr_first", 0.04969814491441352},
    {"tr_last", 0.00012079950826982893},
    {"best_val", 0.027998049558575818},
    {"best_epoch", 346.0},
    {"val_last", 0.04480221327879137},
    {"patience", 200.0},
    {"stop_epoch", 546.0},
    {"net_test", 0.00034479563749007273},
    {"line_test", 0.012403614251111224},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


RC_TEST("a step too small learns slowly, a good one learns, and one too large makes the weights blow up") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  std::vector<Sample> train, test;
  split_by_episode(d, 5, train, test);
  const Normaliser n = fit_normaliser(train);
  const int epochs = 20;
  double result[3];
  const double lrs[3] = {0.001, 0.02, 1.0};
  for (int k = 0; k < 3; ++k) {
    Mlp m = make_mlp(16, 1.0, 3);
    Rng r = make_rng(5);
    for (int e = 0; e < epochs; ++e) train_epoch(m, n, train, lrs[k], r);
    result[k] = mlp_mse(m, n, test);
  }
  rec("hidden", 16, 0);
  rec("epochs", epochs, 0);
  rec("lr_small", lrs[0], 3);
  rec("lr_good", lrs[1], 2);
  rec("lr_big", lrs[2], 1);
  rec("test_small", result[0], 5);
  rec("test_good", result[1], 5);
  RC_CHECK(result[1] < result[0]);
  RC_CHECK(!(result[2] < 1.0));   // NaN or enormous: it blew up
}

RC_TEST("on a small noisy set the training error keeps falling while the validation error turns and rises, and stopping on it keeps the best weights") {
  LaneTask t;
  const std::vector<Sample> small = demonstrations(t, 2, 15, 0.6, 0.4, 0.3, 0.3, 11);
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  std::vector<Sample> rest, val;
  split_by_episode(d, 5, rest, val);
  const Normaliser n = fit_normaliser(small);
  Mlp m = make_mlp(32, 1.0, 3);
  Rng r = make_rng(100);
  const int long_epochs = 3000;
  double first_tr = 0.0, last_tr = 0.0, last_val = 0.0, best_val = 1e9;
  int best_epoch = 0;
  for (int e = 1; e <= long_epochs; ++e) {
    train_epoch(m, n, small, 0.05, r);
    const double v = mlp_mse(m, n, val);
    if (e == 1) first_tr = mlp_mse(m, n, small);
    if (v < best_val) {
      best_val = v;
      best_epoch = e;
    }
    if (e == long_epochs) {
      last_tr = mlp_mse(m, n, small);
      last_val = v;
    }
  }
  rec("n_small", static_cast<double>(small.size()), 0);
  rec("long_epochs", long_epochs, 0);
  rec("tr_first", first_tr, 5);
  rec("tr_last", last_tr, 5);
  rec("best_val", best_val, 4);
  rec("best_epoch", best_epoch, 0);
  rec("val_last", last_val, 4);
  RC_CHECK(last_tr < 0.1 * first_tr);
  RC_CHECK(last_val > 1.5 * best_val);
  RC_CHECK(best_epoch < long_epochs / 2);
  // stopping: the same training, ended by the validation set
  Mlp s = make_mlp(32, 1.0, 3);
  const int patience = 200;
  const TrainResult tr = train_with_stopping(s, n, small, val, long_epochs, 0.05, patience, 100);
  rec("patience", patience, 0);
  rec("stop_epoch", tr.stopped_at, 0);
  RC_CHECK(tr.best_epoch == best_epoch);
  RC_CHECK(tr.stopped_at == best_epoch + patience);
  RC_CHECK(std::fabs(mlp_mse(s, n, val) - best_val) < 1e-12);
}

RC_TEST("where the expert's turn is clipped, the network beats the straight line on episodes neither has seen") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.8, 0.05, 0.0, 22);
  std::vector<Sample> train, test;
  split_by_episode(d, 5, train, test);
  const Normaliser n = fit_normaliser(train);
  Mlp m = make_mlp(16, 1.0, 3);
  Rng r = make_rng(9);
  for (int e = 0; e < 100; ++e) train_epoch(m, n, train, 0.02, r);
  const double net = mlp_mse(m, n, test), line = linear_mse(fit_linear(train, 0.0), test);
  rec("net_test", net, 5);
  rec("line_test", line, 5);
  RC_CHECK(net < 0.5 * line);
}
