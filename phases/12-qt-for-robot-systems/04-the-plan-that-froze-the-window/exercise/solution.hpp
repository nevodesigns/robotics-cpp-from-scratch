#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
#include <QtCore/QPromise>
#include <QtCore/QThread>

// A control station plans: where to drive, how an arm gets to a pose, which of a thousand paths is best. Some plans take a tenth of a second and some take ten. If the plan is
// worked out in the thread that runs the interface, the interface does not run while it is: the window does not repaint, buttons do not click, the heartbeat monitor of the last
// lesson does not look, and the link is not read. To the person at the screen the station has hung. A clock that was ticking every ten milliseconds stops for as long as the plan takes.
//
// The work belongs on another thread, started in one line and reported back on the first by a signal. And a long job that cannot be stopped is a job the person has to wait
// for, so it checks for a request to stop as it goes and says how far it got.

// The work of a plan, as a stand-in: `steps` steps of `step_ms` each. It reports its progress as it goes, checks every step whether it has been cancelled, and its result, if it
// finishes, is the number of steps it did.
inline void plan_work(QPromise<int>& promise, int steps, int step_ms) {
  promise.setProgressRange(0, steps);
  for (int i = 0; i < steps; ++i) {
    if (promise.isCanceled()) return;
    QThread::msleep(static_cast<unsigned long>(step_ms));
    promise.setProgressValue(i + 1);
  }
  promise.addResult(steps);
}

class PlanRunner : public QObject {
  Q_OBJECT

 public:
  explicit PlanRunner(QObject* parent = nullptr) : QObject(parent) {
    connect(&watcher_, &QFutureWatcher<int>::finished, this, &PlanRunner::on_finished);
    connect(&watcher_, &QFutureWatcher<int>::progressValueChanged, this, &PlanRunner::progress);
  }
  ~PlanRunner() override {
    watcher_.cancel();
    watcher_.waitForFinished();                       // the work uses nothing of ours, but a running job is a job that has to be finished before its watcher goes
  }

  // Starts a plan on another thread. Returns false, and does nothing, if one is running.
  bool start(int steps, int step_ms) {
    // TODO: return false if running_. Otherwise running_ = true, and hand the work to another thread and watch it:
    //   watcher_.setFuture(QtConcurrent::run(plan_work, steps, step_ms));
    // plan_work takes the QPromise first, and QtConcurrent::run supplies it. Return true.
    (void)steps;
    (void)step_ms;
    return false;
  }

  // Asks the plan that is running to stop. It stops at its next step.
  void cancel() {
    // TODO: watcher_.cancel().
  }

  bool running() const { return running_; }
  int finished_count() const { return finished_count_; }
  int canceled_count() const { return canceled_count_; }

 signals:
  void progress(int value);       // steps done so far, on the thread of this object
  void finished(int result);      // the plan ran to the end
  void canceled();                // the plan was stopped before the end

 private slots:
  void on_finished() {
    // TODO: running_ = false. If watcher_.isCanceled(): ++canceled_count_ and emit canceled(). Otherwise ++finished_count_ and emit finished(watcher_.result()).
  }

 private:
  QFutureWatcher<int> watcher_;
  bool running_ = false;
  int finished_count_ = 0;
  int canceled_count_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
