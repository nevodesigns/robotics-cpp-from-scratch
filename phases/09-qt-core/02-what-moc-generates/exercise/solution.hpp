// This file does not link, on purpose.
//
// Run rcpp verify 09-02, read the error, run rcpp explain on it, and only then
// fix the class. Causing the error deliberately is the fastest way to stop
// fearing it.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QObject>

class ThresholdSensor : public QObject {
  // TODO: something belongs on this line.
  //
  // Without it, moc reads this header, finds nothing to generate, and writes
  // no body for the signal declared below. The compiler is satisfied because
  // the declaration exists. The linker is not.

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
