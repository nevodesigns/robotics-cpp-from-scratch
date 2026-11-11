// rc/qt/connections.hpp
//
// Connecting once, and ending connections together, from lesson 11-10, graduated.
//
// The same slot connected three times ran three times for one signal and connect_once ran it once; after three reloads of an item that connected in JavaScript one signal ran the live handler once and three handlers of items that no longer existed, each throwing a ReferenceError; Connections inside the item left none.

#ifndef RC_QT_CONNECTIONS_HPP
#define RC_QT_CONNECTIONS_HPP

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <cstddef>
#include <vector>

namespace rc {
namespace qt {

// A connection is a promise to call something when a signal is emitted, and it lasts until it is broken. Two things go wrong with that in a program that creates and destroys
// parts of itself while it runs. Connecting the same slot twice is two calls for each signal, and a loop or a handler that connects each time it is entered adds one more every
// time. And a connection to something that has been destroyed is a call into nothing: in QML a handler made by `signal.connect(function)` in an item that is gone still runs,
// and throws, because the names it uses went with the item.
//
// The answers are to connect once, to keep the connections of a scope in one place so they end with it, and, in QML, to say what a handler belongs to.

// Connects `signal` of `sender` to `slot` of `receiver` unless that connection already exists. Returns true if this call made it and false if it was there already.
// For member function pointers only: a connection to a lambda is a new function each time, and cannot be recognised.
template <typename Sender, typename Signal, typename Receiver, typename Slot>
bool connect_once(const Sender* sender, Signal signal, const Receiver* receiver, Slot slot) {
  const QMetaObject::Connection connection = QObject::connect(sender, signal, receiver, slot, Qt::UniqueConnection);
  return static_cast<bool>(connection);
}

// The connections of one scope, kept so that they can be ended together: when it is cleared, or when it is destroyed.
class ScopedConnections {
 public:
  ScopedConnections() = default;
  ~ScopedConnections() { clear(); }
  ScopedConnections(const ScopedConnections&) = delete;
  ScopedConnections& operator=(const ScopedConnections&) = delete;

  // Keeps a connection (what QObject::connect returns). An invalid one, from a connect that failed, is not kept.
  void add(const QMetaObject::Connection& connection) {
    if (connection) connections_.push_back(connection);
  }

  // How many connections are kept.
  std::size_t count() const { return connections_.size(); }

  // Ends every connection that is kept.
  void clear() {
    for (const QMetaObject::Connection& c : connections_) QObject::disconnect(c);
    connections_.clear();
  }

 private:
  std::vector<QMetaObject::Connection> connections_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_CONNECTIONS_HPP
