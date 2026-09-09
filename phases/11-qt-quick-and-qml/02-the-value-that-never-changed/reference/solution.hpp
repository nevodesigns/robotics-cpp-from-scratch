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
  Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
  Q_PROPERTY(double battery READ battery WRITE setBattery NOTIFY batteryChanged)

 public:
  explicit Telemetry(QObject* parent = nullptr) : QObject(parent) {}

  double speed() const { return speed_; }
  double battery() const { return battery_; }

  // The guard is not a micro optimisation. A setter that emits whether or not
  // anything changed makes QML re-evaluate every binding that reads this
  // property, every time, and a telemetry field written at 100 Hz with an
  // unchanged value is the ordinary case rather than a rare one.
  void setSpeed(double value) {
    if (value == speed_) return;
    speed_ = value;
    emit speedChanged();
  }

  void setBattery(double value) {
    if (value == battery_) return;
    battery_ = value;
    emit batteryChanged();
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
inline std::vector<PropertyReport> inspect_properties(const QMetaObject& meta) {
  std::vector<PropertyReport> reports;
  for (int index = meta.propertyOffset(); index < meta.propertyCount(); ++index) {
    const QMetaProperty property = meta.property(index);
    PropertyReport report;
    report.name = QString::fromLatin1(property.name());
    report.notifiable = property.hasNotifySignal();
    report.constant = property.isConstant();
    report.writable = property.isWritable();
    reports.push_back(report);
  }
  return reports;
}

// The ones a QML binding will read once and never read again.
inline std::vector<QString> properties_qml_cannot_track(const QMetaObject& meta) {
  std::vector<QString> names;
  for (const PropertyReport& report : inspect_properties(meta))
    if (!report.trackable()) names.push_back(report.name);
  return names;
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
