// A Qt object that announces battery trouble without knowing who listens.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QObject>

class BatteryMonitor : public QObject {
  Q_OBJECT

 public:
  // low_percent    at or below this, the battery is low
  // clear_percent  at or above this, the battery has recovered
  //
  // The gap between them is the hysteresis band. It stops a reading that sits
  // on the threshold from producing a storm of warnings.
  explicit BatteryMonitor(int low_percent, int clear_percent, QObject* parent = nullptr)
      : QObject(parent), low_percent_(low_percent), clear_percent_(clear_percent) {}

  // Records a new reading and emits at most one signal.
  void update(int percent) {
    // TODO
    // 1. remember the reading in percent_
    // 2. if not currently low and percent has fallen to or below low_percent_,
    //    become low and emit lowBattery(percent)
    // 3. if currently low and percent has risen to or above clear_percent_,
    //    stop being low and emit batteryRecovered(percent)
    // 4. otherwise emit nothing at all
    (void)percent;
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
