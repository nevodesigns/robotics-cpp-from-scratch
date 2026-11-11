#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <cstddef>
#include <vector>

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
  // TODO: QObject::connect(sender, signal, receiver, slot, Qt::UniqueConnection) returns a QMetaObject::Connection that is false (invalid) when the connection was
  // there already. Return that as a bool (static_cast<bool>).
  (void)sender;
  (void)signal;
  (void)receiver;
  (void)slot;
  return true;
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
    // TODO: if the connection is valid (it converts to true), push_back it onto connections_.
    (void)connection;
  }

  // How many connections are kept.
  std::size_t count() const {
    // TODO: connections_.size().
    return 999;
  }

  // Ends every connection that is kept.
  void clear() {
    // TODO: QObject::disconnect(c) for each kept connection, then connections_.clear().
    connections_.clear();
  }

 private:
  std::vector<QMetaObject::Connection> connections_;
};

#endif  // LESSON_SOLUTION_HPP
