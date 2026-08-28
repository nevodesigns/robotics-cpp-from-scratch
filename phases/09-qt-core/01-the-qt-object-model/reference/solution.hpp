#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QObject>

class BatteryMonitor : public QObject {
  Q_OBJECT

 public:
  explicit BatteryMonitor(int low_percent, int clear_percent, QObject* parent = nullptr)
      : QObject(parent), low_percent_(low_percent), clear_percent_(clear_percent) {}

  void update(int percent) {
    percent_ = percent;

    // Two separate thresholds, and the state decides which one is currently in
    // play. A single threshold here would emit on every flicker across it.
    if (!is_low_ && percent <= low_percent_) {
      is_low_ = true;
      emit lowBattery(percent);
      return;
    }

    if (is_low_ && percent >= clear_percent_) {
      is_low_ = false;
      emit batteryRecovered(percent);
    }

    // Everything else is a reading inside the band, or a reading that does not
    // change the state. Emitting nothing is the correct behaviour, not an
    // omission.
  }

  bool isLow() const { return is_low_; }
  int percent() const { return percent_; }

 signals:
  void lowBattery(int percent);
  void batteryRecovered(int percent);

 private:
  int low_percent_ = 15;
  int clear_percent_ = 20;
  int percent_ = 100;
  bool is_low_ = false;
};

#endif  // LESSON_SOLUTION_HPP
