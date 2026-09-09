#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QMetaObject>
#include <QMetaProperty>
#include <QObject>
#include <QString>

#include <vector>

// What a robot tells the panel about itself.
//
// Every property here is declared with NOTIFY, and every setter guards on
// equality before emitting. Both halves matter and they fail differently: a
// missing NOTIFY is visible in the meta object, a missing emit is not.
class Telemetry : public QObject {
  Q_OBJECT
  // TODO 1: let QML follow these.
  //
  // As written, both properties are readable and writable and neither
  // announces a change. QML will read each one when it builds the binding and
  // never read it again, and it will not warn, because nothing about this is
  // an error. Add NOTIFY to both, naming the signals declared below.
  Q_PROPERTY(double speed READ speed WRITE setSpeed)
  Q_PROPERTY(double battery READ battery WRITE setBattery)

 public:
  explicit Telemetry(QObject* parent = nullptr) : QObject(parent) {}

  double speed() const { return speed_; }
  double battery() const { return battery_; }

  // TODO 2: store the value, and say so, but only when there is something to
  // say.
  //
  // Two halves. Emit the NOTIFY signal, or QML never learns the value moved:
  // a declaration with NOTIFY and a setter that does not emit it fails exactly
  // like having no NOTIFY at all, and the meta object cannot tell the
  // difference.
  //
  // And return early when the value is unchanged. That guard is not a micro
  // optimisation. The test writes the same value a thousand times and counts
  // how often QML re-evaluated the binding: without the guard it is over a
  // thousand, with it, two.
  void setSpeed(double value) {
    speed_ = value;
  }

  void setBattery(double value) {
    battery_ = value;
  }

 signals:
  void speedChanged();
  void batteryChanged();

 private:
  double speed_ = 0.0;
  double battery_ = 1.0;
};

// What the meta object knows about one property, without running anything.
struct PropertyReport {
  QString name;
  bool notifiable = false;
  bool constant = false;
  bool writable = false;

  // QML can follow a property that announces its changes, and it can follow one
  // that promises never to change. It cannot follow anything else.
  bool trackable() const { return notifiable || constant; }
};

// Every property a class declares itself, ignoring the ones it inherits from
// QObject, which are not the ones a panel binds to.
// TODO 3: ask the meta object what it knows.
//
// Walk from meta.propertyOffset() to meta.propertyCount(). Starting at the
// offset rather than at zero is what skips the properties inherited from
// QObject, which a panel does not bind to and which would drown the answer.
//
// QMetaProperty answers hasNotifySignal(), isConstant() and isWritable(), and
// name() gives a const char* that QString::fromLatin1 will take.
inline std::vector<PropertyReport> inspect_properties(const QMetaObject& meta) {
  (void)meta;
  return {};
}

// The ones a QML binding will read once and never read again.
// TODO 4: the ones a QML binding will read once and never read again.
//
// Everything inspect_properties reports that is not trackable(). Returning the
// names rather than a count is what lets a failing test say which property is
// the problem.
inline std::vector<QString> properties_qml_cannot_track(const QMetaObject& meta) {
  (void)meta;
  return {};
}

// Worth calling in a test for every type you hand to QML.
//
// Understand what this does and does not prove. It reads the declaration, so it
// catches a property with no NOTIFY at all. It cannot catch a setter that
// declares NOTIFY and forgets to emit it, because that is a fact about the
// function body and the meta object has never seen the function body. That bug
// needs a test that changes the value and looks.
inline bool every_property_is_trackable(const QMetaObject& meta) {
  return properties_qml_cannot_track(meta).empty();
}

#endif  // LESSON_SOLUTION_HPP
