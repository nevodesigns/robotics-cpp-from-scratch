// rc/qt/bindable.hpp
//
// Which properties QML can follow, from lesson 11-02, graduated.
//
// A QML binding is not a poll. It reads a property once, and then waits to be
// told the value moved. If nothing tells it, it shows the value the object had
// when the panel was built, for as long as the panel exists, and nothing
// anywhere reports it.
//
// Measured with one double exposed four ways, changed from 1.0 to 7.5 in C++:
//
//   NOTIFY, emitted        QML saw 7.50, 2 binding evaluations, 0 warnings
//   no NOTIFY              QML saw 1.00, 1 evaluation,          0 warnings
//   NOTIFY, not emitted    QML saw 1.00, 1 evaluation,          0 warnings
//   CONSTANT               QML saw 1.00, 1 evaluation,          0 warnings
//
// Three of four wrong, none of them reported.
//
// What this header does is ask the meta object, which knows before anything
// runs. Read the next paragraph before relying on it, because it is precise
// about what it can see.
//
// The meta object holds the declaration, not the function body. It catches a
// property with no NOTIFY, because that is written in the Q_PROPERTY line. It
// cannot catch a setter that declares NOTIFY and forgets to emit it, and it
// cannot catch a CONSTANT property on an object that changes it: both of those
// declarations are well formed. Of the three broken classes above this check
// clears two. Those two need a test that changes the value and looks, which is
// why lesson 11-02 ships both kinds of check and not just this one.
//
// One more measurement worth carrying. A setter that emits without comparing
// first makes QML re-run every binding that reads the property, every time.
// Writing the same value a thousand times cost 1002 binding evaluations
// unguarded against 2 guarded.

#ifndef RC_QT_BINDABLE_HPP
#define RC_QT_BINDABLE_HPP

#include <QMetaObject>
#include <QMetaProperty>
#include <QString>

#include <vector>

namespace rc {
namespace qt {

// What the meta object knows about one property.
struct PropertyReport {
  QString name;
  bool notifiable = false;
  bool constant = false;
  bool writable = false;

  // QML can follow a property that announces its changes, and one that
  // promises never to change. Nothing else.
  bool trackable() const { return notifiable || constant; }
};

// Every property a class declares itself.
//
// Starting at propertyOffset rather than zero skips what it inherited from
// QObject, which is not what a panel binds to and which would bury the answer.
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

// The ones a binding will read once and never read again.
//
// Names rather than a count, so a failing test can say which property.
inline std::vector<QString> properties_qml_cannot_track(const QMetaObject& meta) {
  std::vector<QString> names;
  for (const PropertyReport& report : inspect_properties(meta))
    if (!report.trackable()) names.push_back(report.name);
  return names;
}

// Worth asserting in a test for every type handed to QML, remembering that a
// pass here means the declarations are right and says nothing about the
// setters.
inline bool every_property_is_trackable(const QMetaObject& meta) {
  return properties_qml_cannot_track(meta).empty();
}

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_BINDABLE_HPP
