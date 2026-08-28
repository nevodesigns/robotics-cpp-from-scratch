#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QObject>

class ThresholdSensor : public QObject {
  // Q_OBJECT must come first in the class body. It declares the meta object
  // machinery and the signal bodies. moc writes the definitions into a separate
  // generated file, which is why a missing macro shows up at link time rather
  // than at compile time.
  Q_OBJECT

 public:
  explicit ThresholdSensor(double threshold, QObject* parent = nullptr)
      : QObject(parent), threshold_(threshold) {}

  void feed(double value) {
    last_ = value;

    if (!above_ && value > threshold_) {
      above_ = true;
      emit crossed(value);
      return;
    }

    // Returning to or below the threshold rearms the sensor without announcing
    // anything. Only the upward crossing is news.
    if (above_ && value <= threshold_) {
      above_ = false;
    }
  }

  bool isAbove() const { return above_; }
  double last() const { return last_; }

 signals:
  void crossed(double value);

 private:
  double threshold_ = 0.0;
  double last_ = 0.0;
  bool above_ = false;
};

#endif  // LESSON_SOLUTION_HPP
